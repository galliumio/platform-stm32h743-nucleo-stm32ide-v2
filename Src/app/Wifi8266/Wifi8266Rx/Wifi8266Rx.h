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

#ifndef WIFI8266_RX_H
#define WIFI8266_RX_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Wifi8266RxInterface.h"

#define WIFI8266_RX_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("Wifi8266Rx.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

class Wifi8266Rx : public Region {
public:
    Wifi8266Rx();

protected:
    static QState InitialPseudoState(Wifi8266Rx * const me, QEvt const * const e);
    static QState Root(Wifi8266Rx * const me, QEvt const * const e);
        static QState Stopped(Wifi8266Rx * const me, QEvt const * const e);
        static QState Started(Wifi8266Rx * const me, QEvt const * const e);
            static QState Fault(Wifi8266Rx * const me, QEvt const * const e);
            static QState CheckHeader(Wifi8266Rx * const me, QEvt const * const e);
                static QState RspWait(Wifi8266Rx * const me, QEvt const * const e);
            static QState DataWait(Wifi8266Rx * const me, QEvt const * const e);
                static QState DataLenWait(Wifi8266Rx * const me, QEvt const * const e);
                static QState PayloadWait(Wifi8266Rx * const me, QEvt const * const e);
                    static QState BufWait(Wifi8266Rx * const me, QEvt const * const e);
                    static QState UartWait(Wifi8266Rx * const me, QEvt const * const e);

    void ClearRspStr() {
        memset(m_rspStr, 0, sizeof(m_rspStr));
        m_rspIdx = 0;
    }
    void ReadRspFromUart() {
        WIFI8266_RX_ASSERT(!IsRspStrFull());
        m_uartInFifo->Read(reinterpret_cast<uint8_t *>(&m_rspStr[m_rspIdx++]), 1);
    }
    // String buffer is full when null-terminator is at the end of buffer.
    bool IsRspStrFull() {
        return m_rspIdx >= (sizeof(m_rspStr) - 1);
    }
    bool IsRspFullLine() {
        return (m_rspIdx >= 2) && (m_rspStr[m_rspIdx - 2] == '\r') && (m_rspStr[m_rspIdx - 1] == '\n');
    }
    bool IsRspAsync();
    bool WriteDataToDataInFifo(uint32_t rxLen);

    enum {
        RSP_STR_LEN = Wifi8266RxRspInd::RSP_STR_LEN
    };

    static char const *ASYNC_MSG[];
    Fifo *m_uartInFifo;
    Fifo *m_dataInFifo;
    char m_rspStr[RSP_STR_LEN];
    uint32_t m_rspIdx;              // 0-based index to the next write position in rspStr
    uint32_t m_dataLen;             // No. of bytes of data to receive.
    Timer m_stateTimer;
    Timer m_bufWaitTimer;

    enum {
        RSP_WAIT_TIMEOUT_MS = 5000,
        DATA_WAIT_TIMEOUT_MS = 5000,
        BUF_WAIT_TIMEOUT_MS = 100,
    };

#define WIFI8266_RX_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(BUF_WAIT_TIMER)

#define WIFI8266_RX_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(RSP_RX) \
    ADD_EVT(DATA_RX) \
    ADD_EVT(GOT_LEN) \
    ADD_EVT(GOT_SPACE) \
    ADD_EVT(NO_SPACE) \
    ADD_EVT(FAULT_EVT) \
    ADD_EVT(CONTINUE)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        WIFI8266_RX_TIMER_EVT_START = TIMER_EVT_START(WIFI8266_RX),
        WIFI8266_RX_TIMER_EVT
    };

    enum {
        WIFI8266_RX_INTERNAL_EVT_START = INTERNAL_EVT_START(WIFI8266_RX),
        WIFI8266_RX_INTERNAL_EVT
    };
};

} // namespace APP

#endif // WIFI8266_RX_H
