/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
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

#ifndef LED_PANEL_INTERFACE_H
#define LED_PANEL_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

#define LED_PANEL_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LedPanelInterface.h", (int_t)__LINE__))

namespace APP {

class LedDmaBuf;

#define LED_PANEL_INTERFACE_EVT \
    ADD_EVT(LED_PANEL_START_REQ) \
    ADD_EVT(LED_PANEL_START_CFM) \
    ADD_EVT(LED_PANEL_STOP_REQ) \
    ADD_EVT(LED_PANEL_STOP_CFM) \
    ADD_EVT(LED_PANEL_SET_BUF_REQ) \
    ADD_EVT(LED_PANEL_SET_BUF_CFM) \
    ADD_EVT(LED_PANEL_SYNC_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    LED_PANEL_INTERFACE_EVT_START = INTERFACE_EVT_START(LED_PANEL),
    LED_PANEL_INTERFACE_EVT
};

enum {
    LED_PANEL_REASON_UNSPEC = 0,
};

class LedPanelStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 50
    };
    LedPanelStartReq(LedDmaBuf const *dmaBuf) :
        Evt(LED_PANEL_START_REQ), m_dmaBuf(dmaBuf) {}
    LedDmaBuf const *GetDmaBuf() const { return m_dmaBuf; }
private:
    LedDmaBuf const *m_dmaBuf;
};

class LedPanelStartCfm : public ErrorEvt {
public:
    // Success case.
    LedPanelStartCfm(uint16_t rgbAShift = 0, uint16_t rgbBShift = 0) :
        ErrorEvt(LED_PANEL_START_CFM, ERROR_SUCCESS), m_rgbAShift(rgbAShift), m_rgbBShift(rgbBShift) {}
    // Failure case.
    LedPanelStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_PANEL_START_CFM, error, origin, reason), m_rgbAShift(0), m_rgbBShift(0) {
        LED_PANEL_INTERFACE_ASSERT(error != ERROR_SUCCESS);
    }
    uint16_t GetRgbAShift() const { return m_rgbAShift; }
    uint16_t GetRgbBShift() const { return m_rgbBShift; }
private:
    uint16_t m_rgbAShift;
    uint16_t m_rgbBShift;
};

class LedPanelStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 50
    };
    LedPanelStopReq() :
        Evt(LED_PANEL_STOP_REQ) {}
};

class LedPanelStopCfm : public ErrorEvt {
public:
    LedPanelStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_PANEL_STOP_CFM, error, origin, reason) {}
};

class LedPanelSetBufReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 50
    };
    LedPanelSetBufReq(LedDmaBuf const *dmaBuf) :
        Evt(LED_PANEL_SET_BUF_REQ), m_dmaBuf(dmaBuf) {}
    LedDmaBuf const *GetDmaBuf() const { return m_dmaBuf; }
private:
    LedDmaBuf const *m_dmaBuf;
};

class LedPanelSetBufCfm : public ErrorEvt {
public:
    LedPanelSetBufCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LED_PANEL_SET_BUF_CFM, error, origin, reason) {}
};

class LedPanelSyncInd : public Evt {
public:
    LedPanelSyncInd() :
        Evt(LED_PANEL_SYNC_IND) {}
};

} // namespace APP

#endif // LED_PANEL_INTERFACE_H
