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

#ifndef GUI_MGR_H
#define GUI_MGR_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_kv.h"
#include "fw_map.h"
#include "app_hsmn.h"
#include "LedFrame.h"
#include "WM.h"
#include "GuiBgWnd.h"
#include "GuiTicker.h"
#include "GuiText.h"
#include "GuiBmp.h"

using namespace QP;
using namespace FW;

namespace APP {

class GuiMgr : public Active {
public:
    GuiMgr();

protected:
    static QState InitialPseudoState(GuiMgr * const me, QEvt const * const e);
    static QState Root(GuiMgr * const me, QEvt const * const e);
        static QState Stopped(GuiMgr * const me, QEvt const * const e);
        static QState Starting(GuiMgr * const me, QEvt const * const e);
        static QState Stopping(GuiMgr * const me, QEvt const * const e);
        static QState Started(GuiMgr * const me, QEvt const * const e);
            static QState ColorTest(GuiMgr *me, QEvt const *e);
            static QState Signage(GuiMgr *me, QEvt const *e);
            static QState Ticker(GuiMgr *me, QEvt const *e);
                static QState TickerNormal(GuiMgr *me, QEvt const *e);
                static QState TickerAlert(GuiMgr *me, QEvt const *e);
            static QState Bmp(GuiMgr *me, QEvt const *e);
                static QState BmpNormal(GuiMgr *me, QEvt const *e);
                static QState BmpAlert(GuiMgr *me, QEvt const *e);

    enum {
        MAX_WINDOW_CNT = 32,
    };

    class TimeoutMap {
    public:
        uint32_t offset;
        uint32_t timeoutMs;
    };
    // Dynamic timeout for speed control.
    static const TimeoutMap m_ticker1Timeout[];
    static const TimeoutMap m_ticker2Timeout[];
    static const TimeoutMap m_ticker3Timeout[];
    static const TimeoutMap m_ticker4Timeout[];
    static const TimeoutMap m_text1Timeout[];
    static const TimeoutMap m_bmpTimeout[];

    // Helper function to map offset to timeout for speed control.
    static uint32_t GetTimeout(TimeoutMap const *map, uint32_t mapLen, uint32_t offsetLeft, uint32_t offsetRight);
    static uint32_t GetTimeout(TimeoutMap const *map, uint32_t mapLen, uint32_t offset);

    // Window Manager callback function.
    static void WmCallback(WM_MESSAGE *msg);
    void WmHandler(WM_MESSAGE *msg);
    void AddWin(WM_HWIN hwin, QP::QSignal sig);
    void RemoveWin(WM_HWIN hwin);

    typedef KeyValue<WM_HWIN, QP::QSignal> HwinSig;
    typedef Map<WM_HWIN, QP::QSignal> HwinSigMap;
    HwinSig m_hwinSigStor[MAX_WINDOW_CNT];
    HwinSigMap m_hwinSigMap;

    LedFrame m_ledFrame;
    bool m_dirty;

    GuiBgWnd  m_bgWnd;
    GuiTicker m_ticker1;
    GuiTicker m_ticker2;
    GuiTicker m_ticker3;
    GuiTicker m_ticker4;
    GuiText   m_text1;
    GuiBmp    m_bmp;

    uint32_t  m_ticker1CycleCnt;
    uint32_t  m_bmpCycleCnt;
    uint32_t  m_updateIdx;          // Generic for all UI states.
    Evt m_inEvt;                    // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    enum {
        BG_WND_TIMEOUT_MS = 40,
        COLOR_TEST_TIMEOUT_MS = 5000,
    };

    // Timer with tick rate = 0 (SysTick)
    Timer m_stateTimer;

    // Timer with tick rate = 1 (LED_PANEL_SYNC_IND event)
    Timer m_bgWndTimer;
    Timer m_ticker1Timer;
    Timer m_ticker2Timer;
    Timer m_ticker3Timer;
    Timer m_ticker4Timer;
    Timer m_text1Timer;
    Timer m_bmpTimer;
    Timer m_updateTimer;        // Generic use for all UI states.

#define GUI_MGR_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(BG_WND_TIMER) \
    ADD_EVT(TICKER1_TIMER) \
    ADD_EVT(TICKER2_TIMER) \
    ADD_EVT(TICKER3_TIMER) \
    ADD_EVT(TICKER4_TIMER) \
    ADD_EVT(TEXT1_TIMER) \
    ADD_EVT(BMP_TIMER) \
    ADD_EVT(UPDATE_TIMER)

#define GUI_MGR_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(PAINT_BGWND) \
    ADD_EVT(PAINT_TICKER1) \
    ADD_EVT(PAINT_TICKER2) \
    ADD_EVT(PAINT_TICKER3) \
    ADD_EVT(PAINT_TICKER4) \
    ADD_EVT(PAINT_TEXT1) \
    ADD_EVT(PAINT_BMP)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        GUI_MGR_TIMER_EVT_START = TIMER_EVT_START(GUI_MGR),
        GUI_MGR_TIMER_EVT
    };

    enum {
        GUI_MGR_INTERNAL_EVT_START = INTERNAL_EVT_START(GUI_MGR),
        GUI_MGR_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };

    class WmEvent : public Evt {
    public:
        WmEvent(QSignal sig, WM_MESSAGE const &msg) :
            Evt(sig), m_msg(msg) {}
        WM_MESSAGE const &GetMsg() const { return m_msg; }
        uint32_t GetMsgId() const { return m_msg.MsgId; }
        WM_HWIN  GetHandle() const { return m_msg.hWin; }
    private:
        WM_MESSAGE m_msg;
    };

};

} // namespace APP

#endif // GUI_MGR_H
