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

#include "qpcpp.h"
#include "fw_assert.h"
#include "periph.h"

FW_DEFINE_THIS_FILE("periph.cpp")

namespace APP {

TimHal Periph::m_timHalStor[MAX_TIM_COUNT];
TimHalMap Periph::m_timHalMap(m_timHalStor, ARRAY_COUNT(m_timHalStor), TimHal(NULL, NULL));

TIM_HandleTypeDef Periph::m_tim1Hal;
TIM_HandleTypeDef Periph::m_tim2Hal;
TIM_HandleTypeDef Periph::m_tim3Hal;
// Add more HAL handles here.

// Setup common peripherals for normal power mode.
// These common peripherals are shared among different HW blocks and cannot be setup individually
// USART3 - TX PD.8 DMA1 Stream 3 Request 4
//          RX PD.9 DMA1 Stream 1 Request 4
// ESP8266
// USART6 - TX PG.14 DMA2 Stream 6 Request 5
//          RX PG.9 DMA2 Stream 1 Request 5
// RST      PG.0

// UserBtn  PC.13
// LED0 - PB.0 PWM TIM3_CH3 (or TIM1_CH2N)
// Reserved for LEDs PB.7 PB.14
// Reserved for SWD PA.13 PA.14

// LedPanel 0
// DMA2 Stream 3 Channel 6 (TIM1_CH1)
// CLK PWM Timer    TIM1
// CLK Gate Timer   TIM4 (only if USE_SINGLE_PULSE_MODE is not defined in LedPanel.h)
// ENABLE Timer     TIM9 (primary)
// CLK              PE.9 (AF1 TIM1_CH1)
// LATCH            PC.4
// ENABLE PWM       PE.5 (AF4 TIM15 CH1)
// ADDR             PD.4 - PD.7 (Must be contiguous.)
// RGB_A            PA.0 - PA.5 (Must be contiguous. RGB group A. Contains 2 scan sets.)
// RGB_B            PA.6 - PA.11 (Must be contigous. RGB group B. Contains 2 scan sets.)

// LedPanel 1
// DMA2 Stream 2 Channel 7 (TIM8_CH1)
// CLK PWM Timer    TIM8
// CLK Gate Timer   TIM5 (only if USE_SINGLE_PULSE_MODE is not defined in LedPanel.h)
// ENABLE Timer     TIM9 (secondary)
// CLK              PC.6 (AF3 TIM8_CH1)
// LATCH            PC.12
// ENABLE PWM       PE.6 (AF4 TIM15 CH2)
// ADDR             PC.8 - PC.11 (Must be contiguous.)
// RGB_A            PF.0 - PF.5 (Must be contiguous. RGB group A. Contains 2 scan sets.)
// RGB_B            PF.6 - PF.11 (Must be contigous. RGB group B. Contains 2 scan sets.)
//
// TIM3 configuration:
#define TIM3CLK             (SystemCoreClock/4) // 100MHz (on APB1) @todo To verify.
#define TIM3_COUNTER_CLK    (20000000)          // 20MHz
#define TIM3_PWM_FREQ       (20000)             // 20kHz

void Periph::SetupNormal() {
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();
    __GPIOF_CLK_ENABLE();
    __GPIOG_CLK_ENABLE();
    __HAL_RCC_CRC_CLK_ENABLE();     // Required by emwin.
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    // Initialize TIM3 for PWM (shared by LED0...).
    HAL_StatusTypeDef status;
    m_tim3Hal.Instance = TIM3;
    m_tim3Hal.Init.Prescaler = (TIM3CLK / TIM3_COUNTER_CLK) - 1;
    m_tim3Hal.Init.Period = (TIM3_COUNTER_CLK / TIM3_PWM_FREQ) - 1;
    m_tim3Hal.Init.ClockDivision = 0;
    m_tim3Hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    m_tim3Hal.Init.RepetitionCounter = 0;
    status = HAL_TIM_PWM_Init(&m_tim3Hal);
    FW_ASSERT(status == HAL_OK);
    // Add timHandle to map.
    SetHal(TIM3, &m_tim3Hal);
}

// Setup common peripherals for low power mode.
void Periph::SetupLowPower() {
    // TBD.
}

// Reset common peripherals to startup state.
void Periph::Reset() {
    HAL_TIM_PWM_DeInit(&m_tim3Hal);
    __HAL_RCC_TIM3_CLK_DISABLE();
    __HAL_RCC_DMA2_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_CRC_CLK_DISABLE();
    __GPIOG_CLK_DISABLE();
    __GPIOF_CLK_DISABLE();
    __GPIOE_CLK_DISABLE();
    __GPIOD_CLK_DISABLE();
    __GPIOC_CLK_DISABLE();
    __GPIOB_CLK_DISABLE();
    __GPIOA_CLK_DISABLE();
    // TBD.
}

void Periph::EnableTimClk(TIM_TypeDef *tim) {
    switch((uint32_t)tim) {
        case TIM1_BASE: __HAL_RCC_TIM1_CLK_ENABLE(); break;
        case TIM2_BASE: __HAL_RCC_TIM2_CLK_ENABLE(); break;
        case TIM3_BASE: __HAL_RCC_TIM3_CLK_ENABLE(); break;
        case TIM4_BASE: __HAL_RCC_TIM4_CLK_ENABLE(); break;
        case TIM5_BASE: __HAL_RCC_TIM5_CLK_ENABLE(); break;
        case TIM6_BASE: __HAL_RCC_TIM6_CLK_ENABLE(); break;
        case TIM7_BASE: __HAL_RCC_TIM7_CLK_ENABLE(); break;
        case TIM8_BASE: __HAL_RCC_TIM8_CLK_ENABLE(); break;
        case TIM15_BASE: __HAL_RCC_TIM15_CLK_ENABLE(); break;
        case TIM16_BASE: __HAL_RCC_TIM16_CLK_ENABLE(); break;
        case TIM17_BASE: __HAL_RCC_TIM17_CLK_ENABLE(); break;
        default: FW_ASSERT(0); break;
    }
}

void Periph::DisableTimClk(TIM_TypeDef *tim) {
    switch((uint32_t)tim) {
        case TIM1_BASE: __HAL_RCC_TIM1_CLK_DISABLE(); break;
        case TIM2_BASE: __HAL_RCC_TIM2_CLK_DISABLE(); break;
        case TIM3_BASE: __HAL_RCC_TIM3_CLK_DISABLE(); break;
        case TIM4_BASE: __HAL_RCC_TIM4_CLK_DISABLE(); break;
        case TIM5_BASE: __HAL_RCC_TIM5_CLK_DISABLE(); break;
        case TIM6_BASE: __HAL_RCC_TIM6_CLK_DISABLE(); break;
        case TIM7_BASE: __HAL_RCC_TIM7_CLK_DISABLE(); break;
        case TIM8_BASE: __HAL_RCC_TIM8_CLK_DISABLE(); break;
        case TIM15_BASE: __HAL_RCC_TIM15_CLK_DISABLE(); break;
        case TIM16_BASE: __HAL_RCC_TIM16_CLK_DISABLE(); break;
        case TIM17_BASE: __HAL_RCC_TIM17_CLK_DISABLE(); break;
        default: FW_ASSERT(0); break;
    }
}

uint32_t Periph::GetTimFreq(TIM_TypeDef *tim) {
    switch((uint32_t)tim) {
        case TIM1_BASE:
        case TIM8_BASE:
        case TIM15_BASE:
        case TIM16_BASE:
        case TIM17_BASE: {
            return SystemCoreClock/2;   // 100000000 (on APB2). @todo Verify.
        }
        default: {
            return SystemCoreClock/4;   // 100000000 (on APB1). @todo Verify.
        }
    }
}

} // namespace APP
