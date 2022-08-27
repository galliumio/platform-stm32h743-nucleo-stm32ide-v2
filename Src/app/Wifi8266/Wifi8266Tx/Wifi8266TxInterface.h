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

#ifndef WIFI8266_TX_INTERFACE_H
#define WIFI8266_TX_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_macro.h"
#include "app_hsmn.h"
#include "fw_assert.h"
#include "Wifi8266RxInterface.h"

#define WIFI8266_TX_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Wifi8266TxInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define WIFI8266_TX_INTERFACE_EVT \
    ADD_EVT(WIFI8266_TX_START_REQ) \
    ADD_EVT(WIFI8266_TX_START_CFM) \
    ADD_EVT(WIFI8266_TX_STOP_REQ) \
    ADD_EVT(WIFI8266_TX_STOP_CFM) \
    ADD_EVT(WIFI8266_TX_CMD_REQ) \
    ADD_EVT(WIFI8266_TX_CMD_CFM) \
    ADD_EVT(WIFI8266_TX_DATA_REQ) \
    ADD_EVT(WIFI8266_TX_EMPTY_IND) \
    ADD_EVT(WIFI8266_TX_FAULT_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    WIFI8266_TX_INTERFACE_EVT_START = INTERFACE_EVT_START(WIFI8266_TX),
    WIFI8266_TX_INTERFACE_EVT
};

enum {
    WIFI8266_TX_REASON_UNSPEC = 0,
};

class Wifi8266TxStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    Wifi8266TxStartReq(Hsmn uartOutHsmn, Fifo *uartOutFifo, Fifo *dataOutFifo) :
        Evt(WIFI8266_TX_START_REQ), m_uartOutHsmn(uartOutHsmn),
        m_uartOutFifo(uartOutFifo), m_dataOutFifo(dataOutFifo) {}
    Hsmn GetUartOutHsmn() const { return m_uartOutHsmn; }
    Fifo *GetUartOutFifo() const { return m_uartOutFifo; }
    Fifo *GetDataOutFifo() const { return m_dataOutFifo; }
private:
    Hsmn m_uartOutHsmn;
    Fifo *m_uartOutFifo;
    Fifo *m_dataOutFifo;
};

class Wifi8266TxStartCfm : public ErrorEvt {
public:
    Wifi8266TxStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI8266_TX_START_CFM, error, origin, reason) {}
};

class Wifi8266TxStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    Wifi8266TxStopReq() :
        Evt(WIFI8266_TX_STOP_REQ) {}
};

class Wifi8266TxStopCfm : public ErrorEvt {
public:
    Wifi8266TxStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI8266_TX_STOP_CFM, error, origin, reason) {}
};

class Wifi8266TxCmdReq : public Evt {
public:
    Wifi8266TxCmdReq(char const *cmdStr) :
        Evt(WIFI8266_TX_CMD_REQ) {
        WIFI8266_TX_INTERFACE_ASSERT(cmdStr);
        STRBUF_COPY(m_cmdStr, cmdStr);
    }
    enum {
        CMD_STR_LEN = 80
    };
    char const *GetCmdStr() const { return m_cmdStr; }
private:
    char m_cmdStr[CMD_STR_LEN];
};

class Wifi8266TxCmdCfm : public ErrorEvt {
public:
    Wifi8266TxCmdCfm(Error error = ERROR_UNSPEC, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WIFI8266_TX_CMD_CFM, error, origin, reason), m_rspLineCnt(0) {
        memset(m_rspStr[0], 0, sizeof(m_rspStr));
    }
    enum {
        RSP_STR_LEN = Wifi8266RxRspInd::RSP_STR_LEN,
        RSP_LINE_CNT = 5
    };
    void SetError(Error error) { m_error = error; }
    bool AddRspStr(char const *s) {
        WIFI8266_TX_INTERFACE_ASSERT(s);
        if (m_rspLineCnt < RSP_LINE_CNT) {
            STRBUF_COPY(m_rspStr[m_rspLineCnt], s);
            m_rspLineCnt++;
            return true;
        }
        return false;
    }
    bool IsFull() const { return m_rspLineCnt == RSP_LINE_CNT; }
    uint32_t GetRspLineCnt() const { return m_rspLineCnt; }
    char const *GetRspStr(uint32_t i) const {
        WIFI8266_TX_INTERFACE_ASSERT(i < m_rspLineCnt);
        return m_rspStr[i];
    }
private:
    uint32_t m_rspLineCnt;
    char m_rspStr[RSP_LINE_CNT][RSP_STR_LEN];
};

class Wifi8266TxDataReq : public Evt {
public:
    Wifi8266TxDataReq() :
        Evt(WIFI8266_TX_DATA_REQ) {
    }
};

class Wifi8266TxEmptyInd : public Evt {
public:
    Wifi8266TxEmptyInd() :
        Evt(WIFI8266_TX_EMPTY_IND) {
    }
};

class Wifi8266TxFaultInd : public Evt {
public:
    Wifi8266TxFaultInd() :
        Evt(WIFI8266_TX_FAULT_IND) {
    }
};

} // namespace APP

#endif // WIFI8266_TX_INTERFACE_H
