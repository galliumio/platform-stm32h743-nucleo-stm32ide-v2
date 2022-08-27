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

/**
  ******************************************************************************
  * @file    UART/UART_TwoBoards_ComDMA/Src/stm32h7xx_it.c 
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    31-August-2017
  * @brief   Main Interrupt Service Routines.
  ******************************************************************************
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
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
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "bsp.h"
#include "qpcpp.h"
#include "app_hsmn.h"
#include "UartAct.h"
#include "GpioIn.h"
#include "LedPanel.h"
#include "fw_log.h"

/* USER CODE BEGIN 0 */

using namespace FW;
using namespace APP;

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/

extern "C" void SysTick_Handler(void){
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  QXK_ISR_ENTRY();
  QP::QF::tickX_(TICK_RATE_BSP);
  QXK_ISR_EXIT();
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f7xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

// Common handler for UART interrupts.
// It handles RX error and RXNE interrupts. It does NOT handle TX interrupts.
// It does NOT pass control to HAL handler.
void HandleUartIrq(Hsmn hsmn) {
    UartIn::HwError error = UartIn::HW_ERROR_NONE;
    UART_HandleTypeDef *hal = UartAct::GetHal(hsmn);
    volatile uint32_t isrflags   = READ_REG(hal->Instance->ISR);
    if (isrflags & USART_ISR_NE) {
        // START bit Noise detection flag. Must clear it or it will cause ISR to be re-entered.
        __HAL_UART_CLEAR_NEFLAG(hal);
        error |= UartIn::HW_ERROR_NOISE;
    }
    if (isrflags & USART_ISR_FE) {
        // Frame error flag. Must clear it or it will cause ISR to be re-entered.
        __HAL_UART_CLEAR_FEFLAG(hal);
        error |= UartIn::HW_ERROR_FRAME;
    }
    if (isrflags & USART_ISR_ORE) {
        // Overrun error. Reset by setting ORECF in ICR. Alternative, disable ORE by setting OVRDIS.
        // ORE will trigger interrupt when RXNE interrupt is enabled.
        // With DMA, overrun should not occur.
        __HAL_UART_CLEAR_OREFLAG(hal);
        error |= UartIn::HW_ERROR_OVERRUN;
    }
    // Do not check RXNEIE bit as it may have been cleared automatically by DMA.
    // It is okay to not check as we don't use other UART interrupts (except errors above).
    // In case an error interrupt occur, it is okay to treat it as if RXNE has occurred.
    // Disable interrupt to avoid re-entering ISR before event is processed.
    CLEAR_BIT(hal->Instance->CR1, USART_CR1_RXNEIE);
    UartIn::RxCallback(UartAct::GetUartInHsmn(hsmn), error);
    // TX does not use ISR. Do not call HAL handler.
    //HAL_UART_IRQHandler(hal);
}

// UART3 TX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA1_Stream3_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART3_ACT);
    HAL_DMA_IRQHandler(hal->hdmatx);
    QXK_ISR_EXIT();
}

// UART3 RX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA1_Stream1_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART3_ACT);
    HAL_DMA_IRQHandler(hal->hdmarx);
    QXK_ISR_EXIT();
}

// UART3 RX
// Must be declared as extern "C" in header.
extern "C" void USART3_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HandleUartIrq(UART3_ACT);
    QXK_ISR_EXIT();
}

// UART6 TX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Stream6_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART6_ACT);
    HAL_DMA_IRQHandler(hal->hdmatx);
    QXK_ISR_EXIT();
}

// UART6 RX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Stream1_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART6_ACT);
    HAL_DMA_IRQHandler(hal->hdmarx);
    QXK_ISR_EXIT();
}

// UART6 RX
// Must be declared as extern "C" in header.
extern "C" void USART6_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HandleUartIrq(UART6_ACT);
    QXK_ISR_EXIT();
}

// GPIO IN
// Must be declared as extern "C" in header.
extern "C" void EXTI15_10_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    QXK_ISR_EXIT();
}
extern "C" void EXTI9_5_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    QXK_ISR_EXIT();
}
extern "C" void EXTI4_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    QXK_ISR_EXIT();
}
extern "C" void EXTI3_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
    QXK_ISR_EXIT();
}
extern "C" void EXTI2_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    QXK_ISR_EXIT();
}
extern "C" void EXTI1_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    QXK_ISR_EXIT();
}
// User Button (PA.0)
extern "C" void EXTI0_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    QXK_ISR_EXIT();
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    GpioIn::GpioIntCallback(pin);
}

// Slot timer shared by all LedPanel objects.
extern "C" void TIM15_IRQHandler(void) {
    QXK_ISR_ENTRY();
    if (TIM15->SR & TIM_IT_UPDATE) {
        TIM15->SR = ~TIM_IT_UPDATE;
        LedPanel::SlotTimIntCallback();
    }
    QXK_ISR_EXIT();
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
