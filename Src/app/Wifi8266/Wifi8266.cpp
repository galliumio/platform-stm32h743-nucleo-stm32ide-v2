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

#include <cstdio>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw_pipe.h"
#include "UartActInterface.h"
#include "UartOutInterface.h"
#include "UartInInterface.h"
#include "UartAct.h"
#include "WifiInterface.h"
#include "Wifi8266TxInterface.h"
#include "Wifi8266RxInterface.h"
#include "Wifi8266.h"

FW_DEFINE_THIS_FILE("Wifi8266.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "WIFI8266_TIMER_EVT_START",
    WIFI8266_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "WIFI8266_INTERNAL_EVT_START",
    WIFI8266_INTERNAL_EVT
};

// Shares the same interface event definition with all Wifi HSMs.
static char const * const interfaceEvtName[] = {
    "WIFI_INTERFACE_EVT_START",
    WIFI_INTERFACE_EVT
};

Wifi8266::Config const Wifi8266::CONFIG[] = {
    {
      WIFI8266, UART6_ACT, GPIOG, GPIO_PIN_0
    }
};
// Only support single instance.
Wifi8266::Config const *Wifi8266::m_config;

char const *Wifi8266::AT_CFG_CMDS[] = {
    "ATE0\r\n",                 // Turns off echo.
    "AT+GMR\r\n",               // Gets version (for testing).
    "AT+CWAUTOCONN=0\r\n"       // Disable auto connection to AP.
};

// Credentials defined outside this file.
extern char const *wifiSsid;
extern char const *wifiPwd;

void Wifi8266::InitGpio() {
    m_resetPin.Init(m_config->resetPort, m_config->resetPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0, GPIO_SPEED_FREQ_LOW);
}

void Wifi8266::DeInitGpio() {
    m_resetPin.DeInit();
}

void Wifi8266::DisableModule() {
    m_resetPin.Clear();
}

void Wifi8266::EnableModule() {
    m_resetPin.Set();
}

void Wifi8266::LogRsp(Wifi8266TxCmdCfm const &cfm) {
    Wifi8266 *me = this;
    for (uint32_t i = 0; i < cfm.GetRspLineCnt(); i++) {
        LOG("rsp[%i]: %s", i, cfm.GetRspStr(i));
    }
}

// Converts an IP addr string to uint32_t.
bool Wifi8266::Iptoul(char const *ipAddr, uint32_t &result) {
    FW_ASSERT(ipAddr);
    char addr[20];
    STRBUF_COPY(addr, ipAddr);

    char *start = addr;
    char *end;
    result = 0;
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t octet = strtoul(start, &end, 10);
        if ((start == end) || (octet > 0xff) || !((i == 3 && *end == 0) || (i < 3 && *end == '.'))) {
            return false;
        }
        result = (result << 8) | octet;
        if (end) {
            start = end + 1;
        }
    }
    return true;
}

// Returns nullptr if not found.
char const *Wifi8266::FindInRsp(Wifi8266TxCmdCfm const &cfm, char const *key) {
    FW_ASSERT(key);
    for (uint32_t i = 0; i < cfm.GetRspLineCnt(); i++) {
        char const *match = strstr(cfm.GetRspStr(i), key);
        if (match) {
            char const *result = match + strlen(key);
            FW_ASSERT(result < cfm.GetRspStr(i) + Wifi8266TxCmdCfm::RSP_STR_LEN);
            return result;
        }
    }
    return nullptr;
}

Wifi8266::Wifi8266() :
    Active((QStateHandler)&Wifi8266::InitialPseudoState, WIFI8266, "WIFI8266"),
        m_uartOutHsmn(HSM_UNDEF), m_consoleOutHsmn(HSM_UNDEF),
        m_uartOutFifo(m_uartOutFifoStor, UART_OUT_FIFO_ORDER),
        m_uartInFifo(m_uartInFifoStor, UART_IN_FIFO_ORDER),
        m_client(HSM_UNDEF), m_port(0),
        m_dataOutFifo(nullptr), m_dataInFifo(nullptr), m_configIdx(0), m_inEvt(QEvt::STATIC_EVT),
        m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER),
        m_closeWaitTimer(GetHsm().GetHsmn(), CLOSE_WAIT_TIMER) {
    SET_TIMER_EVT_NAME(WIFI8266);
    SET_INTERNAL_EVT_NAME(WIFI8266);
    SET_INTERFACE_EVT_NAME(WIFI);
    FW_ASSERT(CONFIG[0].hsmn == GetHsmn());
    m_config = &CONFIG[0];
    memset(&m_domain, 0, sizeof(m_domain));
    memset(&m_localIp, 0, sizeof(m_localIp));
}

QState Wifi8266::InitialPseudoState(Wifi8266 * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Wifi8266::Root);
}

QState Wifi8266::Root(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_wifi8266Tx.Init(me);
            me->m_wifi8266Rx.Init(me);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266::Stopped);
        }
        case WIFI_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WIFI_CONNECT_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiConnectCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WIFI_DISCONNECT_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiDisconnectCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }

        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Wifi8266::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Wifi8266::Stopped(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case WIFI_START_REQ: {
            EVENT(e);
            WifiStartReq const &req = static_cast<WifiStartReq const &>(*e);
            me->m_inEvt = req;
            return Q_TRAN(&Wifi8266::Starting);
        }
    }
    return Q_SUPER(&Wifi8266::Root);
}

QState Wifi8266::Starting(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = WifiStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartActStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new UartActStartReq(&me->m_uartOutFifo, &me->m_uartInFifo), me->m_config->uartActHsmn, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case UART_ACT_START_CFM: {
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
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new WifiStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new WifiStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Wifi8266::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new WifiStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Wifi8266::Started);
        }
    }
    return Q_SUPER(&Wifi8266::Root);
}

QState Wifi8266::Stopping(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = WifiStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartActStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new UartActStopReq(), me->m_config->uartActHsmn, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case UART_ACT_STOP_CFM: {
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
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Stopped);
        }
    }
    return Q_SUPER(&Wifi8266::Root);
}

QState Wifi8266::Started(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_uartOutHsmn = UartAct::GetUartOutHsmn(me->m_config->uartActHsmn);
            me->InitGpio();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->DeInitGpio();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266::Normal);
        }
    }
    return Q_SUPER(&Wifi8266::Root);
}


QState Wifi8266::Interactive(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            Log::AddInterface(me->m_uartOutHsmn, &me->m_uartOutFifo, UART_OUT_WRITE_REQ, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            Log::RemoveInterface(me->m_uartOutHsmn);
            return Q_HANDLED();
        }
        case WIFI_INTERACTIVE_OFF_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiInteractiveOffCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Wifi8266::Normal);
        }
        case UART_IN_DATA_IND: {
            char buf[100];
            while(uint32_t len = me->m_uartInFifo.Read(reinterpret_cast<uint8_t *>(buf), sizeof(buf))) {
                Log::Write(me->m_consoleOutHsmn, buf, len);
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266::Started);
}


QState Wifi8266::Normal(Wifi8266 * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266::Idle);
        }
        case WIFI_INTERACTIVE_ON_REQ: {
            EVENT(e);
            WifiInteractiveOnReq const &req = static_cast<WifiInteractiveOnReq const &>(*e);
            me->m_consoleOutHsmn = req.GetConsoleOutHsmn();
            FW_ASSERT(me->m_consoleOutHsmn != HSM_UNDEF);
            me->SendCfm(new WifiInteractiveOnCfm(me->m_uartOutHsmn), req);
            return Q_TRAN(&Wifi8266::Interactive);
        }
    }
    return Q_SUPER(&Wifi8266::Started);
}

QState Wifi8266::Idle(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->DisableModule();
            me->m_uartInFifo.Flush();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI_CONNECT_REQ: {
            EVENT(e);
            WifiConnectReq const &req = static_cast<WifiConnectReq const &>(*e);
            me->m_client = req.GetFrom();
            me->m_inEvt = req;
            STRBUF_COPY(me->m_domain, req.GetDomain());
            me->m_port = req.GetPort();
            me->m_dataOutFifo = req.GetDataOutFifo();
            me->m_dataInFifo = req.GetDataInFifo();
            FW_ASSERT(me->m_dataOutFifo && me->m_dataInFifo);
            return Q_TRAN(&Wifi8266::ConnectWait);
        }
    }
    return Q_SUPER(&Wifi8266::Normal);
}

QState Wifi8266::Running(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Must not reset m_uartInFifo or m_uartOutFifo since they are being used by UartIn or UartOut HSMs.
            me->SendReq(new Wifi8266TxStartReq(me->m_uartOutHsmn, &me->m_uartOutFifo, me->m_dataOutFifo), WIFI8266_TX, true);
            me->SendReq(new Wifi8266RxStartReq(&me->m_uartInFifo, me->m_dataInFifo), WIFI8266_RX, false);
            me->EnableModule();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->SendReq(new Wifi8266TxStopReq(), WIFI8266_TX, true);
            me->SendReq(new Wifi8266RxStopReq(), WIFI8266_RX, false);
            me->m_dataOutFifo = nullptr;
            me->m_dataInFifo = nullptr;
            memset(&me->m_domain, 0, sizeof(me->m_domain));
            memset(&me->m_localIp, 0, sizeof(me->m_localIp));
            me->m_port = 0;
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266::ConnectWait);
        }
        case UART_IN_DATA_IND: {
            UartInDataInd const &ind = static_cast<UartInDataInd const &>(*e);
            me->Send(new UartInDataInd(WIFI8266_RX, ind.GetFrom(), ind.GetSeq()));
            return Q_HANDLED();
        }
        case UART_OUT_EMPTY_IND: {
            Evt const &ind = EVT_CAST(*e);
            me->Send(new UartOutEmptyInd(WIFI8266_TX, ind.GetFrom(), ind.GetSeq()));
            return Q_HANDLED();
        }
        case WIFI8266_RX_RSP_IND: {
            EVENT(e);
            Wifi8266RxRspInd const &ind = static_cast<Wifi8266RxRspInd const &>(*e);
            LOG("Running - received async msg: %s", ind.GetRspStr());
            return Q_HANDLED();
        }
        case WIFI_DISCONNECT_REQ:
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Wifi8266::DisconnectWait);
        }
        case DISCONNECTED: {
            EVENT(e);
            me->Send(new WifiDisconnectInd(), me->m_client);
            return Q_TRAN(&Wifi8266::Idle);
        }
    }
    return Q_SUPER(&Wifi8266::Normal);
}

QState Wifi8266::ConnectWait(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(CONNECT_WAIT_TIMEOUT_MS);
            me->m_configIdx = 0;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi8266::ReadyWait);
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new WifiConnectCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new WifiConnectCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Wifi8266::DisconnectWait);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new WifiConnectCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Wifi8266::Connected);
        }
    }
    return Q_SUPER(&Wifi8266::Running);
}

QState Wifi8266::ReadyWait(Wifi8266 * const me, QEvt const * const e) {
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
            Wifi8266RxRspInd const &ind = static_cast<Wifi8266RxRspInd const &>(*e);
            if (STRING_EQUAL(ind.GetRspStr(), "ready\r\n")) {
                EVENT(e);
                LOG("ReadyWait - received async msg: %s", ind.GetRspStr());
                me->Raise(new Evt(DONE));
                return Q_HANDLED();
            }
            break;
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Configuring);
        }
    }
    return Q_SUPER(&Wifi8266::ConnectWait);
}

QState Wifi8266::Configuring(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            FW_ASSERT(me->m_configIdx < ARRAY_COUNT(me->AT_CFG_CMDS));
            me->SendReq(new Wifi8266TxCmdReq(me->AT_CFG_CMDS[me->m_configIdx]), WIFI8266_TX, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_CFM: {
            Wifi8266TxCmdCfm const &cfm = static_cast<Wifi8266TxCmdCfm const &>(*e);
            ERROR_EVENT(cfm);
            me->LogRsp(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                if (++me->m_configIdx < ARRAY_COUNT(me->AT_CFG_CMDS)) {
                    me->Raise(new Evt(MORE));
                } else {
                    me->Raise(new Evt(DONE));
                }
            }
            return Q_HANDLED();
        }
        case MORE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Configuring);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Joining);
        }
    }
    return Q_SUPER(&Wifi8266::ConnectWait);
}

QState Wifi8266::Joining(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", wifiSsid, wifiPwd);
            me->SendReq(new Wifi8266TxCmdReq(cmd), WIFI8266_TX, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_CFM: {
            Wifi8266TxCmdCfm const &cfm = static_cast<Wifi8266TxCmdCfm const &>(*e);
            ERROR_EVENT(cfm);
            me->LogRsp(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::GettingIp);
        }
    }
    return Q_SUPER(&Wifi8266::ConnectWait);
}

QState Wifi8266::GettingIp(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new Wifi8266TxCmdReq("AT+CIFSR\r\n"), WIFI8266_TX, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_CFM: {
            Wifi8266TxCmdCfm const &cfm = static_cast<Wifi8266TxCmdCfm const &>(*e);
            ERROR_EVENT(cfm);
            me->LogRsp(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                // Result is "+CIFSR:STAIP,<ip addr>"
                char const *ipStr = me->FindInRsp(cfm, "+CIFSR:STAIP,");
                if (ipStr) {
                    STRBUF_COPY(me->m_localIp, ipStr);
                    LOG("localIp = %s", me->m_localIp);
                } else {
                    WARNING("localIp not found");
                }
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Connecting);
        }
    }
    return Q_SUPER(&Wifi8266::ConnectWait);
}

QState Wifi8266::Connecting(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", me->m_domain, me->m_port);
            me->SendReq(new Wifi8266TxCmdReq(cmd), WIFI8266_TX, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_CFM: {
            Wifi8266TxCmdCfm const &cfm = static_cast<Wifi8266TxCmdCfm const &>(*e);
            ERROR_EVENT(cfm);
            me->LogRsp(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266::ConnectWait);
}

QState Wifi8266::DisconnectWait(Wifi8266 * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266::Disconnecting);
        }
        case WIFI_DISCONNECT_REQ:
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WIFI8266_TX_CMD_CFM: {
            Wifi8266TxCmdCfm const &cfm = static_cast<Wifi8266TxCmdCfm const &>(*e);
            ERROR_EVENT(cfm);
            me->LogRsp(cfm);
            // Don't care success or failed.
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Evt(DONE));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            // If timeout, continue to next state.
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Idle);
        }
    }
    return Q_SUPER(&Wifi8266::Running);
}

QState Wifi8266::Disconnecting(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE\r\n");
            me->SendReq(new Wifi8266TxCmdReq(cmd), WIFI8266_TX, true);
            me->m_stateTimer.Start(DISCONNECTING_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::CloseWait);
        }
    }
    return Q_SUPER(&Wifi8266::DisconnectWait);
}

QState Wifi8266::CloseWait(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_closeWaitTimer.Start(CLOSE_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_closeWaitTimer.Stop();
            return Q_HANDLED();
        }
        case CLOSE_WAIT_TIMER: {
            EVENT(e);
            return Q_TRAN(&Wifi8266::Unjoining);
        }
    }
    return Q_SUPER(&Wifi8266::DisconnectWait);
}

QState Wifi8266::Unjoining(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            char cmd[100];
            snprintf(cmd, sizeof(cmd), "AT+CWQAP\r\n");
            me->SendReq(new Wifi8266TxCmdReq(cmd), WIFI8266_TX, true);
            me->m_stateTimer.Start(UNJOINING_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266::DisconnectWait);
}

QState Wifi8266::Connected(Wifi8266 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI_WRITE_REQ: {
            EVENT(e);
            me->Send(new Wifi8266TxDataReq(), WIFI8266_TX);
            return Q_HANDLED();
        }
        case WIFI8266_TX_EMPTY_IND: {
            EVENT(e);
            me->Send(new WifiEmptyInd(), me->m_client);
            return Q_HANDLED();
        }
        case WIFI8266_RX_DATA_IND: {
            EVENT(e);
            me->Send(new WifiDataInd(), me->m_client);
            return Q_HANDLED();
        }
        case WIFI_READ_MORE_REQ: {
            EVENT(e);
            me->Send(new Wifi8266RxMoreReq(), WIFI8266_RX);
            return Q_HANDLED();
        }
        case WIFI8266_TX_FAULT_IND:
        case WIFI8266_RX_FAULT_IND: {
            me->Raise(new Evt(DISCONNECTED));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi8266::Running);
}

/*
QState Wifi8266::MyState(Wifi8266 * const me, QEvt const * const e) {
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
            return Q_TRAN(&Wifi8266::SubState);
        }
    }
    return Q_SUPER(&Wifi8266::SuperState);
}
*/

} // namespace APP
