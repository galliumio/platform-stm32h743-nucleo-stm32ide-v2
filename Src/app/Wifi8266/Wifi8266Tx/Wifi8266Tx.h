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

#ifndef WIFI8266_TX_H
#define WIFI8266_TX_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Wifi8266TxInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Wifi8266Tx : public Region {
public:
    Wifi8266Tx();

protected:
    static QState InitialPseudoState(Wifi8266Tx * const me, QEvt const * const e);
    static QState Root(Wifi8266Tx * const me, QEvt const * const e);
        static QState Stopped(Wifi8266Tx * const me, QEvt const * const e);
        static QState Started(Wifi8266Tx * const me, QEvt const * const e);
            static QState Fault(Wifi8266Tx * const me, QEvt const * const e);
            static QState Idle(Wifi8266Tx * const me, QEvt const * const e);
            static QState Busy(Wifi8266Tx * const me, QEvt const * const e);
                static QState TxCmd(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxCmdBufWait(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxCmdRspWait(Wifi8266Tx * const me, QEvt const * const e);
                static QState TxData(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxDataBufWait(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxDataReadyWait(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxDataSendWait(Wifi8266Tx * const me, QEvt const * const e);
                    static QState TxDataRspWait(Wifi8266Tx * const me, QEvt const * const e);

    bool IsSpaceAvailForCmdStr() {
        return m_uartOutFifo->GetAvailCount() >= strlen(m_cmdStr);
    }
    bool IsRspSuccess(char const *s);
    bool IsRspFail(char const *s);
    bool WriteDataToUartOutFifo(uint32_t len);

    // Final rsp from module indicating AT cmd success or failure.
    static char const *SUCCESS_RSP[];
    static char const *FAIL_RSP[];

    Hsmn m_uartOutHsmn;
    Fifo *m_uartOutFifo;
    Fifo *m_dataOutFifo;
    char m_cmdStr[Wifi8266TxCmdReq::CMD_STR_LEN];
    Wifi8266TxCmdCfm *m_txCmdCfm;
    uint32_t m_dataLen;         // No. of bytes to send in one send data AT command.
    uint32_t m_cmdLen;          // Send data AT command length.
    Evt m_inEvt;                                // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    Timer m_stateTimer;
    Timer m_txCmdTimer;

    enum {
        TX_CMD_TIMEOUT_MS = 20000,
        TX_DATA_TIMEOUT_MS = 10000,
    };

#define WIFI8266_TX_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(TX_CMD_TIMER)

#define WIFI8266_TX_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(SEND_DATA) \
    ADD_EVT(FAULT_EVT)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        WIFI8266_TX_TIMER_EVT_START = TIMER_EVT_START(WIFI8266_TX),
        WIFI8266_TX_TIMER_EVT
    };

    enum {
        WIFI8266_TX_INTERNAL_EVT_START = INTERNAL_EVT_START(WIFI8266_TX),
        WIFI8266_TX_INTERNAL_EVT
    };
};

} // namespace APP

#endif // WIFI8266_TX_H
