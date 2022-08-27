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
#include "fw_log.h"
#include "fw_assert.h"
#include "UartInInterface.h"
#include "Wifi8266RxInterface.h"
#include "Wifi8266Rx.h"

FW_DEFINE_THIS_FILE("Wifi8266Rx.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "WIFI8266_RX_TIMER_EVT_START",
    WIFI8266_RX_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "WIFI8266_RX_INTERNAL_EVT_START",
    WIFI8266_RX_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "WIFI8266_RX_INTERFACE_EVT_START",
    WIFI8266_RX_INTERFACE_EVT
};

char const *Wifi8266Rx::ASYNC_MSG[] = {
    "ready\r\n",
    "WIFI CONNECTED\r\n",
    "WIFI GOT IP\r\n",
    "WIFI DISCONNECT\r\n",
    "CONNECT\r\n",              // Actual message may be "<conn_id> CONNECT\r\n"
    "CLOSED\r\n",               // Actual message may be "<conn_id> CLOSED\r\n"
    // Add other async message here (as opposed to response to AT commands).
};

bool Wifi8266Rx::IsRspAsync() {
    for (uint32_t i = 0; i < ARRAY_COUNT(ASYNC_MSG); i++) {
        if (strstr(m_rspStr, ASYNC_MSG[i])) {
            return true;
        }
    }
    return false;
}

// Returns wasEmpty status of m_dataInFifo.
// It is assumed there is guaranteed space in dataInFifo for rxLen.
bool Wifi8266Rx::WriteDataToDataInFifo(uint32_t rxLen) {
    // Loop (at most 2) to read rxLen bytes from m_uartInFifo to m_dataInFifo.
    bool wasEmpty = false;
    while (rxLen) {
        uint32_t blockLen = m_uartInFifo->GetUsedBlockCount();
        blockLen = LESS(blockLen, rxLen);
        bool status;
        uint32_t result = m_dataInFifo->Write(reinterpret_cast<uint8_t *>(m_uartInFifo->GetReadAddr()), blockLen, &status);
        FW_ASSERT(result == blockLen);
        m_uartInFifo->IncReadIndex(blockLen);
        wasEmpty = wasEmpty || status;
        rxLen -= blockLen;
    }
    return wasEmpty;
}

Wifi8266Rx::Wifi8266Rx() :
    Region((QStateHandler)&Wifi8266Rx::InitialPseudoState, WIFI8266_RX, "WIFI8266_RX"),
    m_uartInFifo(nullptr), m_dataInFifo(nullptr), m_rspIdx(0), m_dataLen(0),
    m_stateTimer(this->GetHsm().GetHsmn(), STATE_TIMER),
    m_bufWaitTimer(this->GetHsm().GetHsmn(), BUF_WAIT_TIMER) {
    SET_EVT_NAME(WIFI8266_RX);
    ClearRspStr();
}

QState Wifi8266Rx::InitialPseudoState(Wifi8266Rx * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Wifi8266Rx::Root);
}

QState Wifi8266Rx::Root(Wifi8266Rx * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266Rx::Stopped);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Wifi8266Rx::Stopped(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_RX_START_REQ: {
            EVENT(e);
            Wifi8266RxStartReq const &req = static_cast<Wifi8266RxStartReq const &>(*e);
            me->m_uartInFifo = req.GetUartInFifo();
            me->m_dataInFifo = req.GetDataInFifo();
            FW_ASSERT(me->m_uartInFifo && me->m_dataInFifo);
            // Not cfm required.
            return Q_TRAN(&Wifi8266Rx::Started);
        }
    }
    return Q_SUPER(&Wifi8266Rx::Root);
}

QState Wifi8266Rx::Started(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_uartInFifo = nullptr;
            me->m_dataInFifo = nullptr;
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266Rx::CheckHeader);
        }
        case WIFI8266_RX_STOP_REQ: {
            EVENT(e);
            // Not cfm required.
            return Q_TRAN(&Wifi8266Rx::Stopped);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::CheckHeader);
        }
        case FAULT_EVT: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::Fault);
        }
    }
    return Q_SUPER(&Wifi8266Rx::Root);
}

QState Wifi8266Rx::Fault(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->Send(new Wifi8266RxFaultInd(), WIFI8266);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Rx::Started);
}

QState Wifi8266Rx::CheckHeader(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->ClearRspStr();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_IN_DATA_IND: {
            EVENT(e);
            // If use "char const *" here, use strlen() instead of sizeof().
            char const DATA_HDR[] = "+IPD,";
            const uint32_t DATA_HDR_LEN = sizeof(DATA_HDR) - 1;
            while (me->m_uartInFifo->GetUsedCount()) {
                me->ReadRspFromUart();
                // Spec says it sends ">" but it actually sends "> "
                if (me->m_rspStr[0] == '>') {
                    LOG_BUF(reinterpret_cast<uint8_t *>(me->m_rspStr), me->m_rspIdx, 1, 0);
                    me->Send(new Wifi8266RxRspInd(me->m_rspStr), WIFI8266_TX);
                    me->Raise(new Evt(DONE));
                    break;
                } else if (!strncmp(me->m_rspStr, DATA_HDR, me->m_rspIdx)) {
                    if (me->m_rspIdx == DATA_HDR_LEN) {
                        LOG_BUF(reinterpret_cast<uint8_t *>(me->m_rspStr), me->m_rspIdx, 1, 0);
                        me->Raise(new Evt(DATA_RX));
                        break;
                    }
                }
                else {
                    me->Raise(new Evt(RSP_RX));
                    break;
                }
            }
            if (me->m_uartInFifo->GetUsedCount()) {
                me->Send(new UartInDataInd(), me->GetHsmn());
            }
            return Q_HANDLED();
        }
        case RSP_RX: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::RspWait);
        }
        case DATA_RX: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::DataWait);
        }
    }
    return Q_SUPER(&Wifi8266Rx::Started);
}

QState Wifi8266Rx::RspWait(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(RSP_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Evt(FAULT_EVT));
            return Q_HANDLED();
        }
        case CONTINUE: {
            EVENT(e);
            me->ClearRspStr();
            return Q_TRAN(&Wifi8266Rx::RspWait);
        }
        case UART_IN_DATA_IND: {
            EVENT(e);
            while (me->m_uartInFifo->GetUsedCount()) {
                me->ReadRspFromUart();
                if (me->IsRspFullLine()) {
                    LOG_BUF(reinterpret_cast<uint8_t *>(me->m_rspStr), me->m_rspIdx, 1, 0);
                    me->Send(new Wifi8266RxRspInd(me->m_rspStr), me->IsRspAsync() ? WIFI8266 : WIFI8266_TX);
                    me->Raise(new Evt(DONE));
                    break;
                } else if (me->IsRspStrFull()) {
                    // After reset, the module sends a long binary string causing rspStr to become full. It will be discarded in RspWait.
                    // The remaining part ends with CRLF and will follow normal processing. It doesn't match as async msg and will be
                    // sent to WIFI8266_TX and be discarded in its idle state.
                    LOG_BUF(reinterpret_cast<uint8_t *>(me->m_rspStr), me->m_rspIdx, 1, 0);
                    me->Raise(new Evt(CONTINUE));
                    break;
                }
            }
            if (me->m_uartInFifo->GetUsedCount()) {
                me->Send(new UartInDataInd(), me->GetHsmn());
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Rx::Started);
}

QState Wifi8266Rx::DataWait(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(DATA_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Evt(FAULT_EVT));
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266Rx::DataLenWait);
        }
    }
    return Q_SUPER(&Wifi8266Rx::Started);
}

QState Wifi8266Rx::DataLenWait(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->ClearRspStr();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_IN_DATA_IND: {
            EVENT(e);
            while(me->m_uartInFifo->GetUsedCount()) {
                me->ReadRspFromUart();
                FW_ASSERT(me->m_rspIdx > 0);
                if (me->m_rspStr[me->m_rspIdx-1] == ':') {
                    me->m_rspIdx--;
                    me->m_dataLen = STRING_TO_NUM(me->m_rspStr, 10);
                    if (me->m_dataLen > 0) {
                        LOG("data len = %d", me->m_dataLen);
                        me->Raise(new Evt(GOT_LEN));
                    } else {
                        me->Raise(new Evt(FAULT_EVT));
                    }
                    break;
                }
            }
            if (me->m_uartInFifo->GetUsedCount()) {
                me->Send(new UartInDataInd(), me->GetHsmn());
            }
            return Q_HANDLED();
        }
        case GOT_LEN: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::PayloadWait);
        }
    }
    return Q_SUPER(&Wifi8266Rx::DataWait);
}

QState Wifi8266Rx::PayloadWait(Wifi8266Rx * const me, QEvt const * const e) {
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
            if (me->m_dataInFifo->GetAvailCount()) {
                return Q_TRAN(&Wifi8266Rx::UartWait);
            }
            return Q_TRAN(&Wifi8266Rx::BufWait);
        }
    }
    return Q_SUPER(&Wifi8266Rx::DataWait);
}

QState Wifi8266Rx::BufWait(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Starts a backup poll timer to check for space.
            me->m_bufWaitTimer.Start(BUF_WAIT_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_bufWaitTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case WIFI8266_RX_MORE_REQ:
        case BUF_WAIT_TIMER: {
            EVENT(e);
            if (me->m_dataInFifo->GetAvailCount()) {
                me->Raise(new Evt(GOT_SPACE));
            }
            return Q_HANDLED();
        }
        case GOT_SPACE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::UartWait);
        }
        case UART_IN_DATA_IND: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Rx::PayloadWait);
}

QState Wifi8266Rx::UartWait(Wifi8266Rx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_IN_DATA_IND: {
            EVENT(e);
            // Space is guaranteed in m_dataInFifo for m_dataLen.
            uint32_t rxLen = LESS(me->m_dataLen, me->m_uartInFifo->GetUsedCount());
            rxLen = LESS(rxLen, me->m_dataInFifo->GetAvailCount());
            if (me->WriteDataToDataInFifo(rxLen)) {
                me->Send(new Wifi8266RxDataInd(), WIFI8266);
            }
            me->m_dataLen -= rxLen;
            if (me->m_dataLen == 0) {
                me->PostSync(new Evt(DONE, me->GetHsmn()));
            } else if (me->m_dataInFifo->GetAvailCount() == 0) {
                me->Raise(new Evt(NO_SPACE));
            }
            if (me->m_uartInFifo->GetUsedCount()) {
                me->Send(new UartInDataInd(), me->GetHsmn());
            }
            return Q_HANDLED();
        }
        case NO_SPACE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Rx::BufWait);
        }

    }
    return Q_SUPER(&Wifi8266Rx::PayloadWait);
}

/*
QState Wifi8266Rx::MyState(Wifi8266Rx * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266Rx::SubState);
        }
    }
    return Q_SUPER(&Wifi8266Rx::SuperState);
}
*/

} // namespace APP

