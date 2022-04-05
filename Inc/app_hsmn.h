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

#ifndef HSM_NUM_H
#define HSM_NUM_H

#include "fw_def.h"

namespace APP {

#define APP_HSM \
    ADD_HSM(SYSTEM, 1) \
    ADD_HSM(CONSOLE, 2) \
    ADD_HSM(CMD_INPUT, 2) \
    ADD_HSM(CMD_PARSER, 2) \
    ADD_HSM(UART_ACT, 2) \
    ADD_HSM(UART_IN, 2) \
    ADD_HSM(UART_OUT, 2) \
    ADD_HSM(GPIO_IN_ACT, 1) \
    ADD_HSM(GPIO_IN, 1) \
    ADD_HSM(DEMO, 1) \
    ADD_HSM(GPIO_OUT_ACT, 1) \
    ADD_HSM(GPIO_OUT, 1) \
    ADD_HSM(LED_PANEL, 2) \
    ADD_HSM(LED_FRAME, 1) \
    ADD_HSM(GUI_MGR, 1) \
    ADD_HSM(WIFI, 1) \
    ADD_HSM(WIFI8266, 1) \
    ADD_HSM(WIFI8266_TX, 1) \
    ADD_HSM(WIFI8266_RX, 1) \
    ADD_HSM(NODE, 1) \
    ADD_HSM(NODE_PARSER, 1) \
    ADD_HSM(SIMPLE_ACT, 1) \
    ADD_HSM(SIMPLE_REG, 1) \
    ADD_HSM(COMPOSITE_ACT, 1) \
    ADD_HSM(COMPOSITE_REG, 4)

#define ALIAS_HSM \
    ADD_ALIAS(CONSOLE_UART3,    CONSOLE) \
    ADD_ALIAS(CONSOLE_UART6,    CONSOLE+1) \
    ADD_ALIAS(CMD_INPUT_UART3,  CMD_INPUT) \
    ADD_ALIAS(CMD_INPUT_UART6,  CMD_INPUT+1) \
    ADD_ALIAS(CMD_PARSER_UART3, CMD_PARSER) \
    ADD_ALIAS(CMD_PARSER_UART6, CMD_PARSER+1) \
    ADD_ALIAS(UART3_ACT, UART_ACT) \
    ADD_ALIAS(UART6_ACT, UART_ACT+1) \
    ADD_ALIAS(UART3_IN,  UART_IN) \
    ADD_ALIAS(UART6_IN,  UART_IN+1) \
    ADD_ALIAS(UART3_OUT, UART_OUT) \
    ADD_ALIAS(UART6_OUT, UART_OUT+1) \
    ADD_ALIAS(BTN0, GPIO_IN) \
    ADD_ALIAS(LED0, GPIO_OUT) \
    ADD_ALIAS(LED_PANEL0, LED_PANEL) \
    ADD_ALIAS(LED_PANEL1, LED_PANEL+1) \
    ADD_ALIAS(COMPOSITE_REG0, COMPOSITE_REG) \
    ADD_ALIAS(COMPOSITE_REG1, COMPOSITE_REG+1) \
    ADD_ALIAS(COMPOSITE_REG2, COMPOSITE_REG+2) \
    ADD_ALIAS(COMPOSITE_REG3, COMPOSITE_REG+3)

#undef ADD_HSM
#undef ADD_ALIAS
#define ADD_HSM(hsmn_, count_) hsmn_, hsmn_##_COUNT = count_, hsmn_##_LAST = hsmn_ + count_ - 1,
#define ADD_ALIAS(alias_, to_) alias_ = to_,

enum {
    APP_HSM_START = FW::HSM_UNDEF,
    APP_HSM
    HSM_COUNT,
    ALIAS_HSM
};

// Higher value corresponds to higher priority.
// The maximum priority is defined in qf_port.h as QF_MAX_ACTIVE (32)
enum
{
    PRIO_UART3_ACT      = 30,
    PRIO_UART6_ACT      = 29,
    PRIO_CONSOLE_UART3  = 28,
    PRIO_CONSOLE_UART6  = 27,
    PRIO_SYSTEM         = 26,
    PRIO_LED_PANEL0     = 24,
    PRIO_LED_PANEL1     = 23,
    PRIO_GUI_MGR        = 22,
    PRIO_WIFI8266       = 20,
    PRIO_NODE           = 18,
    PRIO_GPIO_IN_ACT    = 16,
    PRIO_GPIO_OUT_ACT   = 14,
    PRIO_DEMO           = 10,
    PRIO_SIMPLE_ACT     = 4,
    PRIO_COMPOSITE_ACT  = 3
};

} // namespace APP

#endif // HSM_NUM_H
