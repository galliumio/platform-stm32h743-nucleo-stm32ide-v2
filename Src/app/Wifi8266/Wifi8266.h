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

#ifndef WIFI8266_H
#define WIFI8266_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Pin.h"
#include "Wifi8266Tx.h"
#include "Wifi8266Rx.h"
#include "WifiInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Wifi8266 : public Active {
public:
    Wifi8266();

protected:
    static QState InitialPseudoState(Wifi8266 * const me, QEvt const * const e);
    static QState Root(Wifi8266 * const me, QEvt const * const e);
        static QState Stopped(Wifi8266 * const me, QEvt const * const e);
        static QState Starting(Wifi8266 * const me, QEvt const * const e);
        static QState Stopping(Wifi8266 * const me, QEvt const * const e);
        static QState Started(Wifi8266 * const me, QEvt const * const e);
        static QState Interactive(Wifi8266 * const me, QEvt const * const e);
            static QState Normal(Wifi8266 * const me, QEvt const * const e);
                static QState Idle(Wifi8266 * const me, QEvt const * const e);
                static QState Running(Wifi8266 * const me, QEvt const * const e);
                    static QState ConnectWait(Wifi8266 * const me, QEvt const * const e);
                        static QState ReadyWait(Wifi8266 * const me, QEvt const * const e);
                        static QState Configuring(Wifi8266 * const me, QEvt const * const e);
                        static QState Joining(Wifi8266 * const me, QEvt const * const e);
                        static QState GettingIp(Wifi8266 * const me, QEvt const * const e);
                        static QState Connecting(Wifi8266 * const me, QEvt const * const e);
                    static QState DisconnectWait(Wifi8266 * const me, QEvt const * const e);
                        static QState Disconnecting(Wifi8266 * const me, QEvt const * const e);
                        static QState CloseWait(Wifi8266 * const me, QEvt const * const e);
                        static QState Unjoining(Wifi8266 * const me, QEvt const * const e);
                    static QState Connected(Wifi8266 * const me, QEvt const * const e);

    void InitGpio();
    void DeInitGpio();
    void DisableModule();
    void EnableModule();
    void FlushUartInFifo();
    void LogRsp(Wifi8266TxCmdCfm const &cfm);
    char const *FindInRsp(Wifi8266TxCmdCfm const &cfm, char const *key);
    bool Iptoul(char const *ipAddr, uint32_t &result);

    class Config {
    public:
        // Key
        Hsmn hsmn;
        Hsmn uartActHsmn;
        GPIO_TypeDef *resetPort;
        uint32_t resetPin;
    };
    // The following static members only support single instance.
    static Config const CONFIG[];
    static Config const *m_config;
    static char const *AT_CFG_CMDS[];

    Pin m_resetPin;
    Wifi8266Tx m_wifi8266Tx;    // Auxiary region for AT command/data transmission.
    Wifi8266Rx m_wifi8266Rx;    // Auxiary region for rx response.data parsing.
    Hsmn m_uartOutHsmn;         // HSMN of the output interface region.
    Hsmn m_consoleOutHsmn;      // HSMN of the console output interface used to output
                                // WIFI responses in interactive state.

    enum {
        UART_OUT_FIFO_ORDER = 9,
        UART_IN_FIFO_ORDER = 12,
    };
    uint8_t m_uartOutFifoStor[ROUND_UP_32(1 << UART_OUT_FIFO_ORDER)] __attribute__((aligned(32)));
    uint8_t m_uartInFifoStor[ROUND_UP_32(1 << UART_IN_FIFO_ORDER)] __attribute__((aligned(32)));
    Fifo m_uartOutFifo;
    Fifo m_uartInFifo;
    Hsmn m_client;
    char m_domain[WifiConnectReq::DOMAIN_LEN];  // Domain or IP address string of remote server.
    uint16_t m_port;                            // Port of remote server.
    char m_localIp[20];                         // Local IP address string.
    Fifo *m_dataOutFifo;
    Fifo *m_dataInFifo;
    uint32_t m_configIdx;                       // Idx to AT config commands.
    Evt m_inEvt;                                // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    Timer m_stateTimer;
    Timer m_closeWaitTimer;

    enum {
        CONNECT_WAIT_TIMEOUT_MS = 10000,
        DISCONNECTING_TIMEOUT_MS = 5000,
        CLOSE_WAIT_TIMEOUT_MS = 250,
        UNJOINING_TIMEOUT_MS = 5000,
    };

protected:
#define WIFI8266_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(CLOSE_WAIT_TIMER)

#define WIFI8266_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(NEXT) \
    ADD_EVT(MORE) \
    ADD_EVT(DISCONNECTED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        WIFI8266_TIMER_EVT_START = TIMER_EVT_START(WIFI8266),
        WIFI8266_TIMER_EVT
    };

    enum {
        WIFI8266_INTERNAL_EVT_START = INTERNAL_EVT_START(WIFI8266),
        WIFI8266_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // WIFI8266_H
