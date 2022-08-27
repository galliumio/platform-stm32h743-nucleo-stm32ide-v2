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

#ifndef LED_PANEL_H
#define LED_PANEL_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_array.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"
#include "Pin.h"

using namespace QP;
using namespace FW;

#define LEDPANEL_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LedPanel.h", (int)__LINE__))

// Enable this macro to use a single timer to drive clk PWM.
// Disable it to use a separate gate time to control no. of clk cycles.
#define USE_SINGLE_PULSE_MODE

// Enable this macro to sub-bit support to reduce flickering.
#define USE_SUB_BIT

namespace APP {

class LedDmaBuf;

class LedPanel : public Active {
public:
    enum {
        ADDR_PIN_MASK   = (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3),
        RGB_PIN_MASK    = (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5),
        LINE_CNT        = 16,                       // No. of lines in a single scan set.
        BIT_CNT         = 7, //6,                       // No. of bits per color (e.g. 6 for 18-bit color).
        TIM_BASE_FREQ   = 100000000,                // Used by clk gate timer and slot timer.
        TIM_BASE_FREQ_H = (TIM_BASE_FREQ * 2),      // Used by clk pwm timer (for more clk freq choices).
        CLK_FREQ        = 18181818,                 // Other values tried: 33333333, 14285714, 25000000, 20000000, 16666666
                                                    // DMA FIFO error (FE) starts to occur at CLK freq > 14400000.
                                                    // Data shift starts showing up at CLK freq > 18000000 with UART traffic.
        CLK_CNT         = 128,                      // Clock cycles per line.
        FRAME_RATE      = 56, //59, // 97, //60,                       // Frame refresh rate in Hz.
        SLOT_FREQ       = (FRAME_RATE * LINE_CNT * (1 << BIT_CNT)),     // frame rate * 16 scan lines * 64 levels per color (6 bits).
                                                                        // Some typical values: 122880, 61440, 102400, 81920, 92160
        FRAME_PER_SYNC  = 2,                        // No. of frames per sync event.
        SYNC_RATE       = (FRAME_RATE / FRAME_PER_SYNC),
        MS_PER_SYNC     = (1000 / SYNC_RATE),
        SCAN_SETS       = 4,                        // Each panel supports 2 RGB groups, with each group containing 2 scan sets.
        HEIGHT          = (LINE_CNT * SCAN_SETS),   // Total number of lines per panel.
        WIDTH           = CLK_CNT

    };

    // Tables to look up bitIdx (0-5) and slotCnt for each sub-bit.
    static const uint32_t BIT_IDX[];
    static const uint32_t SLOT_CNT[];
    static const uint32_t SUB_BIT_CNT;

    LedPanel();
    // Called from stm32f7xx_it.cpp (TIM15_IRQHandler()).
    static void SlotTimIntCallback() {
        ObjArray objArray = GetObjArray();
        // To maintain synchronization, all panels must be ready.
        if (objArray.GetUsedCount() == LED_PANEL_COUNT) {
            uint32_t idx = LED_PANEL_COUNT;
            while (idx--) {
                // A -> B === !A || B
                if ((s_retryInst == INSTANCE_UNDEF) || (s_retryInst == idx)) {
                    LedPanel *obj = objArray[idx];
                    // LEDPANEL_ASSERT(obj);
                    obj->SlotTimIntHandler(EVT_SLOT_INT);
                }
            }
        }
    }

protected:
    static QState InitialPseudoState(LedPanel * const me, QEvt const * const e);
    static QState Root(LedPanel * const me, QEvt const * const e);
        static QState Stopped(LedPanel * const me, QEvt const * const e);
        static QState Starting(LedPanel * const me, QEvt const * const e);
        static QState Stopping(LedPanel * const me, QEvt const * const e);
        static QState Started(LedPanel * const me, QEvt const * const e);

    typedef Array<LedPanel *, LED_PANEL_COUNT> ObjArray;
    static ObjArray &GetObjArray();
    static uint32_t s_retryInst;        // Instance of LedPanel's currently requesting a retry. Other instances are suspended.

    // Events for slot interrupt handler.
    enum SlotEvt{
        EVT_UNDEF,          // Undefined.
        EVT_SLOT_INT,       // Slot interrupt.
        EVT_FIRST_SLOT,     // Short-circuited (shown in statechart).
        EVT_MID_SLOT,       // Short-circuited (shown in statechart).
        EVT_LAST_SLOT,      // Short-circuited (shown in statechart).
        EVT_RETRY_SLOT,     // Short-circuited (shown in statechart).
    };
    // States used in the slot timer ISR (We follow naming convention for states in statechart).
    enum SlotState {
        InitSlot,    // Initial state for synchronization.
        FirstSlot,   // First slot allocated for the current color bit.
        MidSlot,     // One of the middle slots alloated for the current color bit. (Does not apply if slot cnt <= 2.)
        LastSlot,    // Last slot allocated for the current color bit.
        RetrySlot,   // Slot while retry of DMA is to be initialiated
    };
    // Internal event queue and helpers for slot interrupt handler.
    enum {
        SLOT_EVT_QUEUE_ORDER = 2
    };
    Pipe<SlotEvt> m_slotEvtQueue;
    SlotEvt m_slotEvtQueueStor[1<<SLOT_EVT_QUEUE_ORDER];
    void raiseSlotEvt(SlotEvt const &e) { m_slotEvtQueue.WriteNoCrit(e); }
    bool getSlotEvt(SlotEvt &e) { return m_slotEvtQueue.ReadNoCrit(e); }
    // State checking helper functions.
    bool InInitSlot() { return m_slotState == InitSlot; }
    bool InFirstSlot() { return m_slotState == FirstSlot; }
    bool InMidSlot() { return m_slotState == MidSlot; }
    bool InLastSlot() { return m_slotState == LastSlot; }
    bool InRetrySlot() { return m_slotState == RetrySlot; }
    bool InStarted() { return true; }   // It is the parent state of all possible states in SlotTimIntHandler.

    void SlotTimIntHandler(SlotEvt evt);
    void SaveObj();
    void ClearObj();
    uint16_t GetInst() {
        uint16_t inst = GetHsmn() - LED_PANEL;
        //FW_ASSERT(inst < LED_PANEL_COUNT);
        return inst;
    }
    bool IsPrimary() { return GetInst() == 0; }

    bool IsTimOnApb2(TIM_TypeDef *tim);
    void TimPreloadConfig(TIM_TypeDef *tim, uint32_t ch, bool enable);
    void TimOCConfig(TIM_TypeDef *tim, uint32_t ch, TIM_OC_InitTypeDef *oc);
    void TimOCOutputEnable(TIM_TypeDef *tim, uint32_t ch, bool enable);
    void TimMainOutputEnable(TIM_TypeDef *tim, bool enable);
    void TimOnePulseEnable(TIM_TypeDef *tim, bool enable);
    void TimSlaveModeConfig(TIM_TypeDef *tim, uint32_t inputTrigger, uint32_t slaveMode);
    void TimMasterModeConfig(TIM_TypeDef *tim, uint32_t outputTrig);
    void TimDmaSrcConfig(TIM_TypeDef* tim, uint32_t dmaSrc, bool enable);

    void InitGpio();
    void DeInitGpio();
    void InitClkPwmTim();
    void DeInitClkPwmTim();
#ifndef USE_SINGLE_PULSE_MODE
    void InitClkGateTim();
    void DeInitClkGateTim();
#endif
    void InitClkDma();
    void DeInitClkDma();
    void StartDma(uint16_t const *buf, uint32_t cnt) {
        // Sets up and enables DMA transfer.
        //FW_ASSERT(m_clkDma.Instance == m_config->clkDmaStream);
        m_config->clkDmaStream->PAR = reinterpret_cast<uint32_t>(&m_config->dataPort->ODR);
        m_config->clkDmaStream->NDTR = cnt;
        m_config->clkDmaStream->M0AR = reinterpret_cast<uint32_t>(buf);
        //FW_CRIT_ENTRY();
        // Clear transfer completion status.
        __HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_TC_FLAG_INDEX(&m_clkDma));
        // Clear error flags (transfer error, fifo error and direct mode error).
        //__HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_TE_FLAG_INDEX(&m_clkDma));
        //__HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_FE_FLAG_INDEX(&m_clkDma));
        //__HAL_DMA_CLEAR_FLAG(&m_clkDma, __HAL_DMA_GET_DME_FLAG_INDEX(&m_clkDma));
        __HAL_DMA_ENABLE(&m_clkDma);
        //FW_CRIT_EXIT();
        // Generates CLK PWM.
#ifndef USE_SINGLE_PULSE_MODE
        m_config->clkGateTim->CR1 |= TIM_CR1_CEN;
#endif
        m_config->clkPwmTim->CR1 |= TIM_CR1_CEN;
    }
    void StopDma();
    void InitSlotTim();
    void DeInitSlotTim();
    void StartSlotTim();
    void StopSlotTim();
#ifdef USE_SUB_BIT
    void UpdateBitIdx() {
        if (IsPrimary()) { m_subBitIdx = (m_subBitIdx + 1) % SUB_BIT_CNT; }
        else  { m_subBitIdx = m_subBitIdx ? m_subBitIdx - 1 : (SUB_BIT_CNT - 1); }
        LEDPANEL_ASSERT(m_subBitIdx < SUB_BIT_CNT);
        m_bitIdx = BIT_IDX[m_subBitIdx];
    }
    void UpdateSlotCnt() {
        LEDPANEL_ASSERT(m_subBitIdx < SUB_BIT_CNT);
        m_slotCnt = SLOT_CNT[m_subBitIdx];
    }
#else
    void UpdateBitIdx() {
        if (IsPrimary()) { m_bitIdx = (m_bitIdx + 1) % BIT_CNT; }
        else  { m_bitIdx = m_bitIdx ? m_bitIdx - 1 : (BIT_CNT - 1); }
    }
    void UpdateSlotCnt() {
        m_slotCnt = 1 << m_bitIdx;
    }
#endif

    class Config {
    public:
        // Key
        Hsmn hsmn;
        // Addr pins.
        GPIO_TypeDef *addrPort;
        uint16_t addrShift;     // Lowest pin number (shift) for addr. Pin order (H to L) is AddrD AddrC AddrB AddrA.
        // Latch pin.
        GPIO_TypeDef *latchPort;
        uint16_t latchPin;
        // Data pins.
        GPIO_TypeDef *dataPort;
        uint16_t rgbAShift;     // Lowest pin number (shift) for RGB_A. Pin order (H to L) is B2 G2 R2 B1 G1 R1.
        uint16_t rgbBShift;     // Lowest pin number (shift) for RGB_B. Pin order (H to L) is B2 G2 R2 B1 G1 R1.
        // Enable pin and "slot" timer.
        GPIO_TypeDef *enablePort;
        uint16_t enablePin;
        uint32_t enableAf;
        TIM_TypeDef *slotTim;
        uint32_t slotCh;
        IRQn_Type slotIrq;
        uint32_t slotPrio;
        // Clk pin.
        GPIO_TypeDef *clkPort;
        uint16_t clkPin;
        uint32_t clkAf;
        TIM_TypeDef *clkPwmTim;
        uint32_t clkPwmCh;
        uint32_t clkPwmInputTrig;
#ifndef USE_SINGLE_PULSE_MODE
        TIM_TypeDef *clkGateTim; // Fixed to Ch1.
        uint32_t clkGateInputTrig;
#endif
        DMA_Stream_TypeDef *clkDmaStream;
        uint32_t clkDmaReq;
        GPIO_TypeDef *testPort;
        uint16_t testPin;
    };
    static Config const CONFIG[];
    Config const *m_config;

    Pin m_addrPins;
    Pin m_rgbAPins;
    Pin m_rgbBPins;
    Pin m_latchPin;
    Pin m_enablePin;
    Pin m_clkPin;
    Pin m_testPin;

    DMA_HandleTypeDef m_clkDma;
    uint32_t m_brightness;      // Brightness from 0 to 100.

    uint32_t m_waitCnt;         // Initial wait slots for synchronization. >=0 for delay in unit of slots.
    uint32_t m_slotIdx;         // Index from 0 to 62 (for 6-bit color). Totol no. of slots = 2^(bit cnt) - 1 = 2^6 - 1 = 63.
    uint32_t m_subBitIdx;       // Index from 0 to (sub-bit cnt - 1) to look up the current color bit (bitIdx) and slot duration (slotCnt).
    uint32_t m_bitIdx;          // Index to the current color bit being served. For 18-bit color depth, it ranges from 0 to 5 (6 bit per color).
    uint32_t m_lineIdx;         // Index from 0 to 15. It refers to the current scan line. It only supports 16 scan line.
    uint32_t m_slotCnt;         // Number of slots for the current color sub-bit.
    uint32_t m_frameCnt;        // Number of complete frames displayed. Okay to wrap around.
    SlotState m_slotState;      // State used in SlotTimIntHandler, as represented as substates of Started in statechart.

    LedDmaBuf const *m_currDmaBuf;  // Current dma buffer.
    LedDmaBuf const *m_newDmaBuf;   // Next dma buffer.
    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    Evt m_inSetBufReq;          // Static copy of incoming LedPanelSetBufReq to be confirmed.
    Sequence m_syncSeq;         // Dedicated sequence no. for LedPanelSyncInd sent in ISR.

    Timer m_stateTimer;

#define LED_PANEL_TIMER_EVT \
    ADD_EVT(STATE_TIMER)

#define LED_PANEL_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        LED_PANEL_TIMER_EVT_START = TIMER_EVT_START(LED_PANEL),
        LED_PANEL_TIMER_EVT
    };

    enum {
        LED_PANEL_INTERNAL_EVT_START = INTERNAL_EVT_START(LED_PANEL),
        LED_PANEL_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Hsmn hsmn, Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, hsmn, hsmn, 0, error, origin, reason) {}
    };
};

} // namespace APP

#endif // LED_PANEL_H
