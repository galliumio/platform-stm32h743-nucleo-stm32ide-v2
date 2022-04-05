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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LedPanelInterface.h"
#include "LedDmaBuf.h"
#include "fw.h"
#include "LedPanel.h"

FW_DEFINE_THIS_FILE("LedPanel.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LED_PANEL_TIMER_EVT_START",
    LED_PANEL_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LED_PANEL_INTERNAL_EVT_START",
    LED_PANEL_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LED_PANEL_INTERFACE_EVT_START",
    LED_PANEL_INTERFACE_EVT
};

static char const * const hsmName[] = {
    "LED_PANEL0",
    "LED_PANEL1",
};

static Hsmn &GetCurrHsmn() {
    static Hsmn hsmn = LED_PANEL;
    FW_ASSERT(hsmn <= LED_PANEL_LAST);
    return hsmn;
}

static char const * GetCurrName() {
    uint16_t inst = GetCurrHsmn() - LED_PANEL;
    FW_ASSERT(inst < ARRAY_COUNT(hsmName));
    return hsmName[inst];
}

static void IncCurrHsmn() {
    Hsmn &currHsmn = GetCurrHsmn();
    ++currHsmn;
    FW_ASSERT(currHsmn > 0);
}

// To reduce flickering, a higher-order color bit is distributed into multiple sub-bits,
// e.g. bit 5 is broken into 4 sub-bits 5A-5D and bit 4 is broken into 2 sub-bits 4A-4B.
// subBitIdx:    0    1    2    3    4    5    6    7    8    9
// bitIdx:       0    1    2    5A   4A   5B   3    5C   4B   5D
// slotCnt:      1    2    4    8    8    8    8    8    8    8
// Note - Total no. of slots is 63.

// 6-bit color.
//const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 2, 3, 4, 5 };
//const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 8, 16,32};

//const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 2, 5, 4, 5, 3, 5, 4, 5 };
//const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 8, 8, 8, 8, 8, 8, 8 };

//const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 5, 4, 5, 3, 5, 4, 5, 2, 5, 4, 5, 3, 5, 4, 5 };
//const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

// 7-bit color.
//const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 2, 6, 5, 6, 4, 6, 5, 6, 3, 6, 5, 6, 4, 6, 5, 6 };
//const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 6, 5, 6, 4, 6, 5, 6, 3, 6, 5, 6, 4, 6, 5, 6, 2, 6, 5, 6, 4, 6, 5, 6, 3, 6, 5, 6, 4, 6, 5, 6 };
const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

// 8-bit color.
//const uint32_t LedPanel::BIT_IDX[] =  { 0, 1, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7, 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7, 2, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7, 3, 7, 6, 7, 5, 7, 6, 7, 4, 7, 6, 7, 5, 7, 6, 7 };
//const uint32_t LedPanel::SLOT_CNT[] = { 1, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 7 };


const uint32_t LedPanel::SUB_BIT_CNT = ARRAY_COUNT(LedPanel::BIT_IDX);
FW_STATIC_ASSERT(ARRAY_COUNT(LedPanel::BIT_IDX) == ARRAY_COUNT(LedPanel::SLOT_CNT));

LedPanel::Config const LedPanel::CONFIG[] = {
    // Primary.
    { LED_PANEL0, GPIOD, 4, GPIOC, GPIO_PIN_4, GPIOA, 0, 6, GPIOE, GPIO_PIN_5, GPIO_AF4_TIM15, TIM15, TIM_CHANNEL_1,
      TIM15_IRQn, TIM15_IRQ_PRIO, GPIOE, GPIO_PIN_9, GPIO_AF1_TIM1, TIM1, TIM_CHANNEL_1, TIM_TS_ITR3,
#ifndef USE_SINGLE_PULSE_MODE
      TIM4, TIM_TS_ITR0,
#endif
      DMA2_Stream3, DMA_REQUEST_TIM1_CH1, GPIOB, GPIO_PIN_7},
    // Secondary.
    { LED_PANEL1, GPIOD, 0, GPIOC, GPIO_PIN_12, GPIOF, 0, 6, GPIOE, GPIO_PIN_6, GPIO_AF4_TIM15, TIM15, TIM_CHANNEL_2,
      TIM15_IRQn, TIM15_IRQ_PRIO, GPIOC, GPIO_PIN_6, GPIO_AF3_TIM8, TIM8, TIM_CHANNEL_1, TIM_TS_ITR3,
#ifndef USE_SINGLE_PULSE_MODE
      TIM5, TIM_TS_ITR1,
#endif
      DMA2_Stream2, DMA_REQUEST_TIM8_CH1, GPIOB, GPIO_PIN_14},
    // Add more LED panels here.
};

LedPanel::ObjArray &LedPanel::GetObjArray() {
    static ObjArray array(LED_PANEL_COUNT, NULL);
    return array;
}

uint32_t LedPanel::s_retryInst = INSTANCE_UNDEF;

void LedPanel::SlotTimIntHandler(SlotEvt evt) {
    do {
        switch(evt) {
        case EVT_SLOT_INT: {
            if (InFirstSlot()) {
                // Test code to detect DMA error.
                /*
                if (__HAL_DMA_GET_FLAG(&m_clkDma, __HAL_DMA_GET_FE_FLAG_INDEX(&m_clkDma))) {
                    __HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_FE_FLAG_INDEX(&m_clkDma));
                    m_testPin.Set();
                    m_testPin.Clear();
                    m_testPin.Set();
                    m_testPin.Clear();
                }
                */
                // The following two conditions are equivalent.
                //if (__HAL_DMA_GET_COUNTER(&m_clkDma) > 0) {
                if (m_config->clkDmaStream->CR & DMA_SxCR_EN) {
                    // DMA incomplete.
                    m_testPin.Set();
                    s_retryInst = GetInst();
                    //m_slotState = RetrySlot;
                    raiseSlotEvt(EVT_RETRY_SLOT);
                    m_testPin.Clear();
                } else {
                    m_addrPins.Write(m_lineIdx << m_config->addrShift);
                    m_latchPin.Set();
                    m_latchPin.Clear();
                    s_retryInst = INSTANCE_UNDEF;
                    ++m_slotIdx;
                    if (m_slotIdx < (m_slotCnt - 1)) {
                        //m_slotState = MidSlot;
                        raiseSlotEvt(EVT_MID_SLOT);
                    } else if (m_slotIdx == (m_slotCnt - 1)) {
                        //m_slotState = LastSlot;
                        raiseSlotEvt(EVT_LAST_SLOT);
                    } else {
                        //m_slotState = LastSlot;
                        raiseSlotEvt(EVT_LAST_SLOT);
                        raiseSlotEvt(EVT_SLOT_INT);
                    }
                }
            } else if (InLastSlot()) {
                bool sendCfm = false;
                bool sendSync = false;
                if (++m_lineIdx == LINE_CNT) {
                    m_lineIdx = 0;
                    UpdateBitIdx();
                    UpdateSlotCnt();
                    if (m_bitIdx == 0) {
                        if (m_newDmaBuf) {
                            m_currDmaBuf = m_newDmaBuf;
                            m_newDmaBuf = NULL;
                            sendCfm = true;
                        }
                        if (((++m_frameCnt % FRAME_PER_SYNC) == 0) && IsPrimary()) {
                            sendSync = true;
                        }
                    }
                }
                uint32_t cnt;
                uint16_t const *buf = m_currDmaBuf->GetBuf(m_bitIdx, m_lineIdx, cnt);
                //FW_ASSERT(cnt == CLK_CNT);
                StartDma(buf, cnt);
                m_slotIdx = 0;
                // Does these after StartDma() to reduce latency.
                if (sendCfm){
                    // OK to call SendCfm() in ISR. m_inSetBufReq protected by critical section.
                    SendCfm(new LedPanelSetBufCfm(ERROR_SUCCESS), m_inSetBufReq);
                }
                if (sendSync) {
                    // OK to call SendCfm() in ISR. Use dedicated sequence counter.
                    Send(new LedPanelSyncInd(), GUI_MGR, m_syncSeq++);
                }
                //m_slotState = FirstSlot;
                raiseSlotEvt(EVT_FIRST_SLOT);
            } else if (InMidSlot()) {
                if (++m_slotIdx == (m_slotCnt - 1)) {
                    //m_slotState = LastSlot;
                    raiseSlotEvt(EVT_LAST_SLOT);
                }
            } else if (InRetrySlot()) {
                if (m_config->clkDmaStream->CR & DMA_SxCR_EN) {
                    // DMA still incomplete. Restart DMA.
                    StopDma();
                    //InitClkPwmTim();      // This will recover DMA data shift, but hard to detect.
                    uint32_t cnt;
                    uint16_t const *buf = m_currDmaBuf->GetBuf(m_bitIdx, m_lineIdx, cnt);
                    StartDma(buf, cnt);
                    //m_slotState = FirstSlot;
                    raiseSlotEvt(EVT_FIRST_SLOT);
                } else {
                    //m_slotState = FirstSlot;
                    raiseSlotEvt(EVT_FIRST_SLOT);
                    raiseSlotEvt(EVT_SLOT_INT);
                }
            } else if (InInitSlot()) {
                if (m_waitCnt == 0) {
                    //m_slotState = LastSlot;
                    raiseSlotEvt(EVT_LAST_SLOT);
                } else {
                    m_waitCnt--;
                }
            }
            // Add more states here...
            break;
        }
        case EVT_FIRST_SLOT: {
            m_slotState = FirstSlot;
            break;
        }
        case EVT_MID_SLOT: {
            if (InFirstSlot()) {
                m_slotState = MidSlot;
            }
            break;
        }
        case EVT_LAST_SLOT: {
            m_slotState = LastSlot;
            break;
        }
        case EVT_RETRY_SLOT: {
            if (InFirstSlot()) {
                m_slotState = RetrySlot;
            }
            break;
        }
        // Add more event types here...
        default: {
            FW_ASSERT(0);
        }
        }
    } while(getSlotEvt(evt));
}

void LedPanel::ClearObj() {
    FW_CRIT_ENTRY();
    uint32_t instance = GetInst();
    GetObjArray()[instance] = NULL;
    FW_CRIT_EXIT();
}

void LedPanel::SaveObj() {
    FW_CRIT_ENTRY();
    uint32_t instance = GetInst();
    GetObjArray()[instance] = this;
    FW_CRIT_EXIT();
}

void LedPanel::InitGpio() {
    m_addrPins.Init(m_config->addrPort, ADDR_PIN_MASK << m_config->addrShift);
    m_rgbAPins.Init(m_config->dataPort, RGB_PIN_MASK << m_config->rgbAShift);
    m_rgbBPins.Init(m_config->dataPort, RGB_PIN_MASK << m_config->rgbBShift);
    m_latchPin.Init(m_config->latchPort, m_config->latchPin);
    m_clkPin.Init(m_config->clkPort, m_config->clkPin, GPIO_MODE_AF_PP, GPIO_PULLUP, m_config->clkAf);
    m_enablePin.Init(m_config->enablePort, m_config->enablePin, GPIO_MODE_AF_PP, GPIO_PULLUP, m_config->enableAf);
    m_testPin.Init(m_config->testPort, m_config->testPin);
}

void LedPanel::DeInitGpio() {
    m_addrPins.DeInit();
    m_rgbAPins.DeInit();
    m_rgbBPins.DeInit();
    m_latchPin.DeInit();
    m_clkPin.DeInit();
    m_enablePin.DeInit();
    m_testPin.DeInit();
}

bool LedPanel::IsTimOnApb2(TIM_TypeDef *tim) {
    switch((uint32_t)tim) {
        case TIM1_BASE:
        case TIM8_BASE:
        case TIM15_BASE:
        case TIM16_BASE:
        case TIM17_BASE: return true;
        default: return false;
    }
}

void LedPanel::TimPreloadConfig(TIM_TypeDef *tim, uint32_t ch, bool enable) {
    switch (ch)
    {
        case TIM_CHANNEL_1: {
            tim->CCMR1 &= ~TIM_CCMR1_OC1PE;
            tim->CCMR1 |= enable ? TIM_CCMR1_OC1PE : 0;
            break;
        }
        case TIM_CHANNEL_2: {
            tim->CCMR1 &= ~TIM_CCMR1_OC2PE;
            tim->CCMR1 |= enable ? TIM_CCMR1_OC2PE : 0;
            break;
        }
        case TIM_CHANNEL_3: {
            tim->CCMR2 &= ~TIM_CCMR2_OC3PE;
            tim->CCMR2 |= enable ? TIM_CCMR2_OC3PE : 0;
            break;
        }
        case TIM_CHANNEL_4: {
            tim->CCMR2 &= ~TIM_CCMR2_OC4PE;
            tim->CCMR2 |= enable ? TIM_CCMR2_OC4PE : 0;
            break;
        }
    }
    if (enable) {
        tim->CR1 |= TIM_CR1_ARPE;
    }
    else {
        tim->CR1 &= ~TIM_CR1_ARPE;
    }
}

void LedPanel::TimOCConfig(TIM_TypeDef *tim, uint32_t ch, TIM_OC_InitTypeDef *oc) {
    switch (ch)
    {
        case TIM_CHANNEL_1: {
            TIM_OC1_SetConfig(tim, oc);
            break;
        }
        case TIM_CHANNEL_2: {
            TIM_OC2_SetConfig(tim, oc);
            break;
        }
        case TIM_CHANNEL_3: {
            TIM_OC3_SetConfig(tim, oc);
            break;
        }
        case TIM_CHANNEL_4: {
            TIM_OC4_SetConfig(tim, oc);
            break;
        }
    }
}

void LedPanel::TimOCOutputEnable(TIM_TypeDef *tim, uint32_t ch, bool enable) {
    if (enable) {
        TIM_CCxChannelCmd(tim, ch, TIM_CCx_ENABLE);
    } else {
        TIM_CCxChannelCmd(tim, ch, TIM_CCx_DISABLE);
    }
}

void LedPanel::TimMainOutputEnable(TIM_TypeDef *tim, bool enable) {
    uint32_t bdtr = tim->BDTR;
    if (enable) {
        bdtr |= TIM_BDTR_MOE;
    } else {
        bdtr &= ~TIM_BDTR_MOE;
    }
    tim->BDTR = bdtr;
}

void LedPanel::TimOnePulseEnable(TIM_TypeDef *tim, bool enable) {
    if (enable) {
        tim->CR1 |= TIM_CR1_OPM;
    } else {
        tim->CR1 &= ~TIM_CR1_OPM;
    }
}

void LedPanel::TimSlaveModeConfig(TIM_TypeDef *tim, uint32_t inputTrig, uint32_t slaveMode) {
  uint32_t smcr = tim->SMCR;
  smcr &= ~TIM_SMCR_TS;
  smcr |= inputTrig;
  smcr &= ~TIM_SMCR_SMS;
  smcr |= slaveMode;
  tim->SMCR =smcr;
}

void LedPanel::TimMasterModeConfig(TIM_TypeDef *tim, uint32_t outputTrig) {
    uint32_t cr2 = tim->CR2;
    cr2 &= ~TIM_CR2_MMS;
    cr2 |= outputTrig;
    tim->CR2 = cr2;
}

void LedPanel::TimDmaSrcConfig(TIM_TypeDef* tim, uint32_t dmaSrc, bool enable) {
    if (enable) {
        tim->DIER |= dmaSrc;
    } else {
        tim->DIER &= ~dmaSrc;
    }
}

void LedPanel::InitClkPwmTim() {
    TIM_TypeDef *tim = m_config->clkPwmTim;
    uint32_t ch = m_config->clkPwmCh;
    Periph::EnableTimClk(tim);

    TIM_Base_InitTypeDef timBase;
    TIM_OC_InitTypeDef timOc;

    // Disables preload.
    TimPreloadConfig(tim, ch, false);
#ifdef USE_SINGLE_PULSE_MODE
    TimOnePulseEnable(tim, true);
#endif
    // Only TIM on APB2 supports TIM_BASE_FREQ_H.
    FW_ASSERT(IsTimOnApb2(tim));
    memset(&timBase, 0, sizeof(timBase));
    timBase.Prescaler = Periph::GetTimFreq(tim) / TIM_BASE_FREQ_H - 1;
    timBase.Period = TIM_BASE_FREQ_H / CLK_FREQ - 1;
    timBase.ClockDivision = 0;
    timBase.CounterMode = TIM_COUNTERMODE_UP;
    timBase.RepetitionCounter = CLK_CNT - 1;
    TIM_Base_SetConfig(tim, &timBase);

    // High then low.
    memset(&timOc, 0, sizeof(timOc));
    timOc.OCMode = TIM_OCMODE_PWM1;
    timOc.Pulse = timBase.Period / 2;

    timOc.OCPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCNPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCIdleState = TIM_OCIDLESTATE_RESET;
    timOc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timOc.OCFastMode = TIM_OCFAST_DISABLE;          // Not used.
    TimOCConfig(tim, ch, &timOc);
    Log::Debug(Log::TYPE_LOG, GetHsmn(), "CLK PWM Period=%d, Pulse=%d", timBase.Period, timOc.Pulse);

    // Enables OC pin output.
    TimOCOutputEnable(tim, ch, true);
    // Enables main timer output.
    TimMainOutputEnable(tim, true);

    // Configures triggering.
#ifndef USE_SINGLE_PULSE_MODE
    TimSlaveModeConfig(tim, m_config->clkPwmInputTrig, TIM_SLAVEMODE_GATED);
    TimMasterModeConfig(tim, TIM_TRGO_UPDATE);
#endif
    // Enables preload.
    TimPreloadConfig(tim, ch, true);
    TimDmaSrcConfig(tim, TIM_DMA_CC1, true);
}

void LedPanel::DeInitClkPwmTim() {
    TIM_TypeDef *tim = m_config->clkPwmTim;
    uint32_t ch = m_config->clkPwmCh;
    // Disable preload.
    TimPreloadConfig(tim, ch, false);
    TimDmaSrcConfig(tim, TIM_DMA_CC1, false);
    // Disable TIM output.
    TimOCOutputEnable(tim, ch, false);
    TimMainOutputEnable(tim, false);
    Periph::DisableTimClk(tim);
    // @TODO Check for more cleanup.
}

#ifndef USE_SINGLE_PULSE_MODE
void LedPanel::InitClkGateTim() {
    TIM_TypeDef *tim = m_config->clkGateTim;
    uint32_t ch = TIM_CHANNEL_1;        // Fixed to channel 1.
    Periph::EnableTimClk(tim);

    TIM_Base_InitTypeDef timBase;
    TIM_OC_InitTypeDef timOc;

    // Disables preload.
    TimPreloadConfig(tim, ch, false);

    timBase.Prescaler = Periph::GetTimFreq(tim) / TIM_BASE_FREQ - 1;
    // Sets period to longer than (double) the total CLK duration in each transfer.
    timBase.Period = (TIM_BASE_FREQ / CLK_FREQ) * CLK_CNT * 2 - 1;
    timBase.ClockDivision = 0;
    timBase.CounterMode = TIM_COUNTERMODE_UP;
    timBase.RepetitionCounter = 0;
    TIM_Base_SetConfig(tim, &timBase);

    // Low then high.
    timOc.OCMode = TIM_OCMODE_PWM2;
    timOc.Pulse = 1;
    timOc.OCPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCNPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCIdleState = TIM_OCIDLESTATE_RESET;
    timOc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timOc.OCFastMode = TIM_OCFAST_DISABLE;          // Not used.
    TimOCConfig(tim, ch, &timOc);
    Log::Debug(Log::TYPE_LOG, GetHsmn(), "CLK Gate Period=%d, Pulse=%d", timBase.Period, timOc.Pulse);

    // Enables OC pin output. (@TODO Check if needed.)
    TimOCOutputEnable(tim, ch, true);
    // Enable one pulse mode.
    TimOnePulseEnable(tim, true);

    // Configures triggering.
    // Reset by PWM TIM update (end of all repeat cycles.)
    TimSlaveModeConfig(tim, m_config->clkGateInputTrig, TIM_SLAVEMODE_RESET);
    TimMasterModeConfig(tim, TIM_TRGO_OC1REF);

    // Enables preload.
    TimPreloadConfig(tim, ch, true);
}

void LedPanel::DeInitClkGateTim() {
    TIM_TypeDef *tim = m_config->clkGateTim;
    uint32_t ch = TIM_CHANNEL_1;        // Fixed to channel 1.
    // Disable preload.
    TimPreloadConfig(tim, ch, false);
    // Disable TIM output.
    TimOCOutputEnable(tim, ch, false);
    Periph::DisableTimClk(tim);
    // @TODO Check for more cleanup.
}
#endif

void LedPanel::InitClkDma() {
    // Configure the CLK DMA
    m_clkDma.Instance                 = m_config->clkDmaStream;
    m_clkDma.Init.Request             = m_config->clkDmaReq;
    m_clkDma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    m_clkDma.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_clkDma.Init.MemInc              = DMA_MINC_ENABLE;
    m_clkDma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    m_clkDma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    m_clkDma.Init.Mode                = DMA_NORMAL;
    m_clkDma.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    m_clkDma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    m_clkDma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
    m_clkDma.Init.MemBurst            = DMA_MBURST_INC4;
    m_clkDma.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    //FW_CRIT_ENTRY();
    HAL_StatusTypeDef status = HAL_DMA_Init(&m_clkDma);
    //FW_CRIT_EXIT();
    FW_ASSERT(status == HAL_OK);
}

void LedPanel::DeInitClkDma() {
    //FW_CRIT_ENTRY();
    HAL_StatusTypeDef status = HAL_DMA_DeInit(&m_clkDma);
    //FW_CRIT_EXIT();
    FW_ASSERT(status == HAL_OK);
}

void LedPanel::StopDma() {
    // Stops CLK PWM.
#ifndef USE_SINGLE_PULSE_MODE
    m_config->clkGateTim->CR1 &= ~TIM_CR1_CEN;
#endif
    m_config->clkPwmTim->CR1 &= ~TIM_CR1_CEN;
    // Disables DMA transfer.
    //FW_CRIT_ENTRY();
    __HAL_DMA_DISABLE(&m_clkDma);
    __HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_TC_FLAG_INDEX(&m_clkDma));
    //FW_CRIT_EXIT();
}

void LedPanel::InitSlotTim() {
    TIM_TypeDef *tim = m_config->slotTim;
    uint32_t ch = m_config->slotCh;
    uint32_t period = TIM_BASE_FREQ_H / SLOT_FREQ - 1;

    // Timer base and interrupt are shared by all instances of LedPanel.
    // Each instance has its own pwm channel for brightness control (may combine via same physical connection).
    if (IsPrimary()) {
        Periph::EnableTimClk(tim);
        TIM_Base_InitTypeDef timBase;
        memset(&timBase, 0, sizeof(timBase));
        timBase.Prescaler = Periph::GetTimFreq(tim) / TIM_BASE_FREQ_H - 1;
        timBase.Period = period;
        timBase.ClockDivision = 0;
        timBase.CounterMode = TIM_COUNTERMODE_UP;
        timBase.RepetitionCounter = 0;
        TIM_Base_SetConfig(tim, &timBase);
    }

    TIM_OC_InitTypeDef timOc;
    memset(&timOc, 0, sizeof(timOc));
    // Disables preload.
    TimPreloadConfig(tim, ch, false);

    m_brightness = LESS(m_brightness, 100);
    // High then low.
    timOc.OCMode = TIM_OCMODE_PWM1;
    timOc.Pulse = (period + 1) * (100 - m_brightness) / 100;
    timOc.OCPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCNPolarity = TIM_OCPOLARITY_HIGH;
    timOc.OCIdleState = TIM_OCIDLESTATE_RESET;
    timOc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timOc.OCFastMode = TIM_OCFAST_DISABLE;          // Not used.
    TimOCConfig(tim, ch, &timOc);
    Log::Debug(Log::TYPE_LOG, GetHsmn(), "ENABLE PWM Period=%d, Pulse=%d", period, timOc.Pulse);

    // Enables OC pin output.
    TimOCOutputEnable(tim, ch, true);
    // Enables preload.
    TimPreloadConfig(tim, ch, true);

    if (IsPrimary()) {
        // Enables main timer output.
        TimMainOutputEnable(tim, true);
        // Enables TIM global interrupt
        NVIC_SetPriority(m_config->slotIrq, m_config->slotPrio);
        NVIC_EnableIRQ(m_config->slotIrq);
    }
}

void LedPanel::DeInitSlotTim() {
    TIM_TypeDef *tim = m_config->slotTim;
    uint32_t ch = m_config->slotCh;
    if (IsPrimary()) {
        NVIC_DisableIRQ(m_config->slotIrq);
    }
    // Disable preload.
    TimPreloadConfig(tim, ch, false);
    // Disable TIM output.
    TimOCOutputEnable(tim, ch, false);
    if (IsPrimary()) {
        TimMainOutputEnable(tim, false);
        Periph::DisableTimClk(tim);
    }
    // @TODO Check for more cleanup.
}

void LedPanel::StartSlotTim()
{
    if (IsPrimary()) {
        // Enables timer update interrupt.
        m_config->slotTim->DIER |= TIM_IT_UPDATE;
        // Enables timer counter.
        m_config->slotTim->CR1 |= TIM_CR1_CEN;
    }
}

void LedPanel::StopSlotTim()
{
    if (IsPrimary()) {
        // Disables timer update interrupt.
        m_config->slotTim->DIER &= ~TIM_IT_UPDATE;
        // Disables timer counter.
        m_config->slotTim->CR1 &= ~TIM_CR1_CEN;
    }
}

LedPanel::LedPanel() :
    Active((QStateHandler)&LedPanel::InitialPseudoState, GetCurrHsmn(), GetCurrName()),
    m_slotEvtQueue(m_slotEvtQueueStor, SLOT_EVT_QUEUE_ORDER),
    m_brightness(20), m_waitCnt(0), m_slotIdx(0), m_subBitIdx(0), m_bitIdx(0), m_lineIdx(0), m_slotCnt(0), m_frameCnt(0), m_slotState(MidSlot),
    m_currDmaBuf(NULL), m_newDmaBuf(NULL), m_inEvt(QEvt::STATIC_EVT), m_inSetBufReq(QEvt::STATIC_EVT), m_syncSeq(0),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(LED_PANEL);
    IncCurrHsmn();
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == GetHsm().GetHsmn()) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
}

QState LedPanel::InitialPseudoState(LedPanel * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&LedPanel::Root);
}

QState LedPanel::Root(LedPanel * const me, QEvt const * const e) {
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
            return Q_TRAN(&LedPanel::Stopped);
        }
        case LED_PANEL_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedPanelStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LED_PANEL_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&LedPanel::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState LedPanel::Stopped(LedPanel * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LED_PANEL_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LedPanelStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LED_PANEL_START_REQ: {
            EVENT(e);
            LedPanelStartReq const &req = static_cast<LedPanelStartReq const &>(*e);
            me->m_currDmaBuf = req.GetDmaBuf();
            me->m_newDmaBuf = NULL;
            FW_ASSERT(me->m_currDmaBuf);
            me->m_inEvt = req;
            return Q_TRAN(&LedPanel::Starting);
        }
    }
    return Q_SUPER(&LedPanel::Root);
}

QState LedPanel::Starting(LedPanel * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedPanelStartReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // Place-holder only. Sends DONE immediately. Do not use PostSync in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new LedPanelStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new LedPanelStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&LedPanel::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LedPanelStartCfm(me->m_config->rgbAShift, me->m_config->rgbBShift), me->m_inEvt);
            return Q_TRAN(&LedPanel::Started);
        }
    }
    return Q_SUPER(&LedPanel::Root);
}

QState LedPanel::Stopping(LedPanel * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LedPanelStopReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            me->m_currDmaBuf = NULL;
            me->m_newDmaBuf = NULL;
            // Place-holder only, so sends DONE immediately. Do not use PostSync in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case LED_PANEL_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
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
            return Q_TRAN(&LedPanel::Stopped);
        }
    }
    return Q_SUPER(&LedPanel::Root);
}

QState LedPanel::Started(LedPanel * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->InitGpio();
            me->InitClkPwmTim();
#ifndef USE_SINGLE_PULSE_MODE
            me->InitClkGateTim();
#endif
            me->InitClkDma();
            me->InitSlotTim();

            // Initializes parameters for SlotIntHandler().
#ifdef USE_SUB_BIT
            if (me->IsPrimary()) {
                me->m_subBitIdx = SUB_BIT_CNT - 1;
                me->m_waitCnt = 1;
            } else {
                me->m_subBitIdx = 0;
                me->m_waitCnt = 0;
            }
#else
            if (me->IsPrimary()) {
                me->m_bitIdx = BIT_CNT - 1;
                me->m_waitCnt = 1;
            } else {
                me->m_bitIdx = 0;
                me->m_waitCnt = 0;
            }
#endif
            me->m_lineIdx = LINE_CNT - 1;
            me->m_frameCnt = 0;
            me->m_slotState = InitSlot;

            // SaveObj must be called after the above initialization.
            me->SaveObj();
            me->StartSlotTim();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->StopSlotTim();
            me->ClearObj();
            me->DeInitSlotTim();
            // Brief wait for ongoing DMA transfer to complete (not necessary).
            DelayMs(5);
            me->StopDma();
            me->DeInitClkDma();
#ifndef USE_SINGLE_PULSE_MODE
            me->DeInitClkGateTim();
#endif
            me->DeInitClkPwmTim();
            me->DeInitGpio();
            return Q_HANDLED();
        }
        case LED_PANEL_SET_BUF_REQ:{
            EVENT(e);
            LedPanelSetBufReq const &req = static_cast<LedPanelSetBufReq const &>(*e);
            // Enforces critical section since the following members will be accessed by ISR.
            FW_CRIT_ENTRY();
            me->m_inSetBufReq = req;
            me->m_newDmaBuf = req.GetDmaBuf();
            FW_CRIT_EXIT();
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LedPanel::Root);
}

/*
QState LedPanel::MyState(LedPanel * const me, QEvt const * const e) {
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
            return Q_TRAN(&LedPanel::SubState);
        }
    }
    return Q_SUPER(&LedPanel::SuperState);
}
*/

} // namespace APP
