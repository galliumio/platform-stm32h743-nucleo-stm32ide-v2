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

#ifndef WIFI8266_RX_INTERFACE_H
#define WIFI8266_RX_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "fw_assert.h"

#define WIFI8266_RX_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Wifi8266RxInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define WIFI8266_RX_INTERFACE_EVT \
    ADD_EVT(WIFI8266_RX_START_REQ) \
    ADD_EVT(WIFI8266_RX_START_CFM) \
    ADD_EVT(WIFI8266_RX_STOP_REQ) \
    ADD_EVT(WIFI8266_RX_STOP_CFM) \
    ADD_EVT(WIFI8266_RX_RSP_IND) \
    ADD_EVT(WIFI8266_RX_DATA_IND) \
    ADD_EVT(WIFI8266_RX_MORE_REQ) \
    ADD_EVT(WIFI8266_RX_FAULT_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    WIFI8266_RX_INTERFACE_EVT_START = INTERFACE_EVT_START(WIFI8266_RX),
    WIFI8266_RX_INTERFACE_EVT
};

enum {
    WIFI8266_RX_REASON_UNSPEC = 0,
};

class Wifi8266RxStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    Wifi8266RxStartReq(Fifo *uartInFifo, Fifo *dataInFifo) :
        Evt(WIFI8266_RX_START_REQ),
        m_uartInFifo(uartInFifo), m_dataInFifo(dataInFifo) {}
    Fifo *GetUartInFifo() const { return m_uartInFifo; }
    Fifo *GetDataInFifo() const { return m_dataInFifo; }
private:
    Fifo *m_uartInFifo;
    Fifo *m_dataInFifo;
};

class Wifi8266RxStartCfm : public ErrorEvt {
public:
    Wifi8266RxStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI8266_RX_START_CFM, error, origin, reason) {}
};

class Wifi8266RxStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    Wifi8266RxStopReq() :
        Evt(WIFI8266_RX_STOP_REQ) {}
};

class Wifi8266RxStopCfm : public ErrorEvt {
public:
    Wifi8266RxStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI8266_RX_STOP_CFM, error, origin, reason) {}
};

class Wifi8266RxRspInd : public Evt {
public:
    Wifi8266RxRspInd(char const *rspStr) :
        Evt(WIFI8266_RX_RSP_IND) {
        WIFI8266_RX_INTERFACE_ASSERT(rspStr);
        STRBUF_COPY(m_rspStr, rspStr);
    }
    enum {
        RSP_STR_LEN = 80    // Max len of a single line from the module, including null termination.
    };
    char const *GetRspStr() const { return m_rspStr; }
private:
    char m_rspStr[RSP_STR_LEN];
};

class Wifi8266RxDataInd : public Evt {
public:
    Wifi8266RxDataInd() :
        Evt(WIFI8266_RX_DATA_IND) {
    }
};

class Wifi8266RxMoreReq : public Evt {
public:
    Wifi8266RxMoreReq() :
        Evt(WIFI8266_RX_MORE_REQ) {
    }
};

class Wifi8266RxFaultInd : public Evt {
public:
    Wifi8266RxFaultInd() :
        Evt(WIFI8266_RX_FAULT_IND) {
    }
};

} // namespace APP

#endif // WIFI8266_RX_INTERFACE_H
