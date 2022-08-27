/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

#include "app_hsmn.h"
#include "fw_macro.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LedPanelInterface.h"
#include "LedFrameInterface.h"
#include "LedFrame.h"
#include "LCDConf_Lin_Template.h"

FW_DEFINE_THIS_FILE("LedFrame.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LED_FRAME_TIMER_EVT_START",
    LED_FRAME_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LED_FRAME_INTERNAL_EVT_START",
    LED_FRAME_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LED_FRAME_INTERFACE_EVT_START",
    LED_FRAME_INTERFACE_EVT
};

LedFrame::LedFrame() :
    Region((QStateHandler)&LedFrame::InitialPseudoState, LED_FRAME, "LED_FRAME"),
    m_bufIdx(0),  m_frameBuf((uint8_t *)lcdBuf, sizeof(lcdBuf)), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(LED_FRAME);
    memset(m_rgbAShift, 0, sizeof(m_rgbAShift));
    memset(m_rgbBShift, 0, sizeof(m_rgbBShift));
}

QState LedFrame::InitialPseudoState(LedFrame * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&LedFrame::Root);
}

QState LedFrame::Root(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::Stopped);
        }
        case LED_FRAME_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedFrameStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&LedFrame::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState LedFrame::Stopped(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_FRAME_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedFrameStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LED_FRAME_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&LedFrame::Starting);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Starting(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedFrameStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > LedPanelStartReq::TIMEOUT_MS * 2);
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::Starting1);
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new LedFrameStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new LedFrameStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&LedFrame::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LedFrameStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LedFrame::Started);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Starting1(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Starts primary/master panel first since it controls the base slot timer.
            // Passes the pointer to the first dmaBuf for the first LedPanel.
            me->SendReq(new LedPanelStartReq(&me->m_dmaBuf[0][0]), LED_PANEL0, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_PANEL_START_CFM: {
            EVENT(e);
            LedPanelStartCfm const &cfm = static_cast<LedPanelStartCfm const &>(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->m_rgbAShift[0] = cfm.GetRgbAShift();
                me->m_rgbBShift[0] = cfm.GetRgbBShift();
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&LedFrame::Starting2);
        }
    }
    return Q_SUPER(&LedFrame::Starting);
}

QState LedFrame::Starting2(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Passes the pointer to the first dmaBuf for the second LedPanel.
            me->SendReq(new LedPanelStartReq(&me->m_dmaBuf[1][0]), LED_PANEL1, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_PANEL_START_CFM: {
            EVENT(e);
            LedPanelStartCfm const &cfm = static_cast<LedPanelStartCfm const &>(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->m_rgbAShift[1] = cfm.GetRgbAShift();
                me->m_rgbBShift[1] = cfm.GetRgbBShift();
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LedFrame::Starting);
}

QState LedFrame::Stopping(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedFrameStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > LedPanelStopReq::TIMEOUT_MS * 2);
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::Stopping1);
        }
        case LED_FRAME_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&LedFrame::Stopped);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Stopping1(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new LedPanelStopReq(), LED_PANEL1, true);
            return Q_HANDLED();
        }
        case LED_PANEL_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&LedFrame::Stopping2);
        }
    }
    return Q_SUPER(&LedFrame::Stopping);
}

QState LedFrame::Stopping2(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new LedPanelStopReq(), LED_PANEL0, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_PANEL_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LedFrame::Stopping);
}

QState LedFrame::Started(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // The first buffers have been passed to LedPanel's. Sets the index to refer to the second ones.
            me->m_bufIdx = 1;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::Idle);
        }
    }
    return Q_SUPER(&LedFrame::Root);
}

QState LedFrame::Idle(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_FRAME_UPDATE_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&LedFrame::Busy);
        }
    }
    return Q_SUPER(&LedFrame::Started);
}

QState LedFrame::Busy(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            FW_ASSERT(me->m_bufIdx < ARRAY_COUNT(me->m_dmaBuf[0]));
            for (uint32_t i = 0; i < LED_PANEL_COUNT; i++) {
                LedDmaBuf *dmaBuf = &me->m_dmaBuf[i][me->m_bufIdx];
                dmaBuf->ConvertFrame(me->m_frameBuf, Area(0, i * LedPanel::HEIGHT, LedPanel::WIDTH, LedPanel::HEIGHT), me->m_rgbAShift[i], me->m_rgbBShift[i]);
                me->SendReq(new LedPanelSetBufReq(dmaBuf), LED_PANEL + i, (i == 0));
            }
            me->m_bufIdx ^= 1;
            uint32_t timeout = LedFrameUpdateReq::TIMEOUT_MS;
            FW_ASSERT(timeout > LedPanelSetBufReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case LED_FRAME_UPDATE_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case LED_PANEL_SET_BUF_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                FW_ASSERT(0);
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LedFrameUpdateCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LedFrame::Idle);
        }
    }
    return Q_SUPER(&LedFrame::Started);
}

/*
QState LedFrame::MyState(LedFrame * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LedFrame::SubState);
        }
    }
    return Q_SUPER(&LedFrame::SuperState);
}
*/

} // namespace APP
