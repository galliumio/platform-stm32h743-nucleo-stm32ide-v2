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

#include <cstdio>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "UartOutInterface.h"
#include "Wifi8266RxInterface.h"
#include "Wifi8266TxInterface.h"
#include "Wifi8266Tx.h"

FW_DEFINE_THIS_FILE("Wifi8266Tx.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "WIFI8266_TX_TIMER_EVT_START",
    WIFI8266_TX_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "WIFI8266_TX_INTERNAL_EVT_START",
    WIFI8266_TX_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "WIFI8266_TX_INTERFACE_EVT_START",
    WIFI8266_TX_INTERFACE_EVT
};

char const *Wifi8266Tx::SUCCESS_RSP[] = {
    "OK\r\n",
    "SEND OK\r\n",
};

char const *Wifi8266Tx::FAIL_RSP[] = {
    "ERROR\r\n",
    "SEND FAIL\r\n",
    "ALREADY CONNECTED\r\n",
};

bool Wifi8266Tx::IsRspSuccess(char const *s) {
    for (uint32_t i = 0; i < ARRAY_COUNT(SUCCESS_RSP); i++) {
        // @todo Add trimming.
        if (STRING_EQUAL(s, SUCCESS_RSP[i])) {
            return true;
        }
    }
    return false;
}

bool Wifi8266Tx::IsRspFail(char const *s) {
    for (uint32_t i = 0; i < ARRAY_COUNT(FAIL_RSP); i++) {
        // @todo Add trimming.
        if (STRING_EQUAL(s, FAIL_RSP[i])) {
            return true;
        }
    }
    return false;
}

// Returns wasEmpty status of m_uartOutFifo.
// It is assumed there is guaranteed space in uartOutFifo for txLen.
bool Wifi8266Tx::WriteDataToUartOutFifo(uint32_t txLen) {
    // Loop (at most 2) to send out txLen bytes from m_dataOutFifo to m_uartOutFifo.
    bool wasEmpty = false;
    while (txLen) {
        uint32_t blockLen = m_dataOutFifo->GetUsedBlockCount();
        blockLen = LESS(blockLen, txLen);
        bool status;
        uint32_t result = m_uartOutFifo->Write(reinterpret_cast<uint8_t *>(m_dataOutFifo->GetReadAddr()), blockLen, &status);
        FW_ASSERT(result == blockLen);
        m_dataOutFifo->IncReadIndex(blockLen);
        wasEmpty = wasEmpty || status;
        txLen -= blockLen;
    }
    return wasEmpty;
}

Wifi8266Tx::Wifi8266Tx() :
    Region((QStateHandler)&Wifi8266Tx::InitialPseudoState, WIFI8266_TX, "WIFI8266_TX"),
    m_uartOutHsmn(HSM_UNDEF), m_uartOutFifo(nullptr), m_dataOutFifo(nullptr), m_txCmdCfm(nullptr),
    m_dataLen(0), m_cmdLen(0), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(this->GetHsm().GetHsmn(), STATE_TIMER),
    m_txCmdTimer(this->GetHsm().GetHsmn(), TX_CMD_TIMER) {
    SET_EVT_NAME(WIFI8266_TX);
    memset(m_cmdStr, 0, sizeof(m_cmdStr));
}

QState Wifi8266Tx::InitialPseudoState(Wifi8266Tx * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Wifi8266Tx::Root);
}

QState Wifi8266Tx::Root(Wifi8266Tx * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266Tx::Stopped);
        }
        case WIFI8266_TX_CMD_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new Wifi8266TxCmdCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Wifi8266Tx::Stopped(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_START_REQ: {
            EVENT(e);
            Wifi8266TxStartReq const &req = static_cast<Wifi8266TxStartReq const &>(*e);
            me->m_uartOutHsmn = req.GetUartOutHsmn();
            me->m_uartOutFifo = req.GetUartOutFifo();
            me->m_dataOutFifo = req.GetDataOutFifo();
            return Q_TRAN(&Wifi8266Tx::Started);
        }
    }
    return Q_SUPER(&Wifi8266Tx::Root);
}

QState Wifi8266Tx::Started(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_uartOutHsmn = HSM_UNDEF;
            me->m_uartOutFifo = nullptr;
            me->m_dataOutFifo = nullptr;
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266Tx::Idle);
        }
        case WIFI8266_TX_STOP_REQ: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::Stopped);
        }
    }
    return Q_SUPER(&Wifi8266Tx::Root);
}

QState Wifi8266Tx::Fault(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->Send(new Wifi8266TxFaultInd(), WIFI8266);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Tx::Started);
}

QState Wifi8266Tx::Idle(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_DATA_REQ: {
            EVENT(e);
            if (me->m_dataOutFifo->GetUsedCount()) {
                me->Raise(new Evt(SEND_DATA));
            } else {
                LOG("WIFI8266_TX_DATA_REQ ignored - dataOutFifo emtpy");
            }
            return Q_HANDLED();
        }
        case SEND_DATA: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::TxData);
        }
        case WIFI8266_TX_CMD_REQ: {
            EVENT(e);
            Wifi8266TxCmdReq const &req = static_cast<Wifi8266TxCmdReq const &>(*e);
            me->m_inEvt = req;
            STRBUF_COPY(me->m_cmdStr, req.GetCmdStr());
            LOG("Sending cmd: %s", me->m_cmdStr);
            if (me->IsSpaceAvailForCmdStr()) {
                return Q_TRAN(&Wifi8266Tx::TxCmdRspWait);
            }
            return Q_TRAN(&Wifi8266Tx::TxCmdBufWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::Started);
}

QState Wifi8266Tx::Busy(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266Tx::TxCmd);
        }
        case WIFI8266_TX_DATA_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::Idle);
        }
        case FAULT_EVT: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::Fault);
        }
    }
    return Q_SUPER(&Wifi8266Tx::Started);
}

QState Wifi8266Tx::TxCmd(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_txCmdTimer.Start(TX_CMD_TIMEOUT_MS);
            me->m_txCmdCfm = new Wifi8266TxCmdCfm();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_txCmdTimer.Stop();
            FW_ASSERT(me->m_txCmdCfm);
            me->SendCfm(me->m_txCmdCfm, me->m_inEvt);
            me->m_txCmdCfm = nullptr;
            return Q_HANDLED();
        }
        case TX_CMD_TIMER: {
            EVENT(e);
            me->m_txCmdCfm->SetError(ERROR_TIMEOUT);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266Tx::TxCmdBufWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::Busy);
}

QState Wifi8266Tx::TxCmdBufWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_OUT_EMPTY_IND: {
            EVENT(e);
            FW_ASSERT(me->IsSpaceAvailForCmdStr());
            return Q_TRAN(&Wifi8266Tx::TxCmdRspWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxCmd);
}

QState Wifi8266Tx::TxCmdRspWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t len = strlen(me->m_cmdStr);
            bool wasEmpty;
            uint32_t sentLen = me->m_uartOutFifo->Write(reinterpret_cast<uint8_t *>(me->m_cmdStr), len, &wasEmpty);
            FW_ASSERT(sentLen == len);
            if (wasEmpty) {
                me->Send(new Evt(UART_OUT_WRITE_REQ), me->m_uartOutHsmn);
            }
            return Q_HANDLED();
        }
        case WIFI8266_RX_RSP_IND: {
            EVENT(e);
            Wifi8266RxRspInd const &ind = static_cast<Wifi8266RxRspInd const &>(*e);
            char const *rsp = ind.GetRspStr();
            me->m_txCmdCfm->AddRspStr(rsp);
            if (me->IsRspSuccess(rsp)) {
                me->m_txCmdCfm->SetError(ERROR_SUCCESS);
                me->Raise(new Evt(DONE));
            } else if (me->IsRspFail(rsp)) {
                me->m_txCmdCfm->SetError(ERROR_HARDWARE);
                me->Raise(new Evt(DONE));
            } else if (me->m_txCmdCfm->IsFull()) {
                me->m_txCmdCfm->SetError(ERROR_HARDWARE);
                me->Raise(new Evt(FAULT_EVT));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxCmd);
}

QState Wifi8266Tx::TxData(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(TX_DATA_TIMEOUT_MS);
            me->m_dataLen = me->m_dataOutFifo->GetUsedCount();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            // Prepares send data cmd string and calculates its len.
            snprintf(me->m_cmdStr, sizeof(me->m_cmdStr), "AT+CIPSEND=%lu\r\n", me->m_dataLen);
            me->m_cmdLen = strlen(me->m_cmdStr);
            if (me->m_uartOutFifo->GetAvailCount() < me->m_cmdLen) {
                return Q_TRAN(&Wifi8266Tx::TxDataBufWait);
            }
            return Q_TRAN(&Wifi8266Tx::TxDataReadyWait);
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Evt(FAULT_EVT));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Tx::Busy);
}

QState Wifi8266Tx::TxDataBufWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_OUT_EMPTY_IND: {
            EVENT(e);
            FW_ASSERT(me->m_uartOutFifo->GetAvailCount() >= me->m_cmdLen);
            return Q_TRAN(&Wifi8266Tx::TxDataReadyWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxData);
}

QState Wifi8266Tx::TxDataReadyWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("Send data cmd = '%s'", me->m_cmdStr);
            bool wasEmpty;
            uint32_t sentLen = me->m_uartOutFifo->Write(reinterpret_cast<uint8_t *>(me->m_cmdStr), me->m_cmdLen, &wasEmpty);
            FW_ASSERT(sentLen == me->m_cmdLen);
            if (wasEmpty) {
                me->Send(new Evt(UART_OUT_WRITE_REQ), me->m_uartOutHsmn);
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_RX_RSP_IND: {
            EVENT(e);
            Wifi8266RxRspInd const &ind = static_cast<Wifi8266RxRspInd const &>(*e);
            INFO("rsp = %s", ind.GetRspStr());
            if (STRING_EQUAL(ind.GetRspStr(), ">")) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::TxDataSendWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxData);
}

QState Wifi8266Tx::TxDataSendWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t txLen = LESS(me->m_dataLen, me->m_uartOutFifo->GetAvailCount());
            if (me->WriteDataToUartOutFifo(txLen)) {
                me->Send(new Evt(UART_OUT_WRITE_REQ), me->m_uartOutHsmn);
            }
            me->m_dataLen -= txLen;
            if (me->m_dataLen == 0) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case UART_OUT_EMPTY_IND: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::TxDataSendWait);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266Tx::TxDataRspWait);
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxData);
}

QState Wifi8266Tx::TxDataRspWait(Wifi8266Tx * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_RX_RSP_IND: {
            EVENT(e);
            Wifi8266RxRspInd const &ind = static_cast<Wifi8266RxRspInd const &>(*e);
            INFO("rsp = %s", ind.GetRspStr());
            if (me->IsRspSuccess(ind.GetRspStr())) {
                me->Raise(new Evt(DONE));
                if (me->m_dataOutFifo->GetUsedCount()) {
                    LOG("m_dataOutFifo non-empty, send SEND_DATA");
                    me->Send(new Wifi8266TxDataReq(), WIFI8266_TX);
                } else {
                    LOG("m_dataOutFifo empty, send Wifi8266TxEmptyInd");
                    me->Send(new Wifi8266TxEmptyInd(), WIFI8266);
                }
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266Tx::TxData);
}

/*
QState Wifi8266Tx::MyState(Wifi8266Tx * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266Tx::SubState);
        }
    }
    return Q_SUPER(&Wifi8266Tx::SuperState);
}
*/

} // namespace APP

