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

#include "fw_macro.h"
#include "fw_assert.h"
#include "LedDmaBuf.h"
#include "LedFrameBuf.h"

FW_DEFINE_THIS_FILE("LedDmaBuf.cpp")

namespace APP {

// Buffers can be allocated to TCM by uncommenting the section __attribute__ below. Each panel requires dual buffers - a current and a next buffer.
// Total size is 2*(2*2)*6*16*128 = 96KB, which fits in to TCM (128KB). The buffer is not declared as static to allow checking in map file.
// @todo TCM is not working with new version of STM32 lib. To be investigated.
// Best practice to ensure the beginning and end of buffers are aligned to cache line size (32 bytes).
// (Not strictly required for output buffer; Only required for input buffer that needs invalidation.)
uint16_t ledDmaBufStor[LED_PANEL_COUNT * 2][LedPanel::BIT_CNT][LedPanel::LINE_CNT][ROUND_UP_32(LedPanel::CLK_CNT)] __attribute__((aligned(32))); // __attribute__ ((section (".data.DTCM")));

static LedDmaBuf::Buffer& GetBuffer() {
    static uint32_t inst = 0;
    FW_ASSERT(inst < ARRAY_COUNT(ledDmaBufStor));
    return ledDmaBufStor[inst++];
}

LedDmaBuf::LedDmaBuf() : m_buf(GetBuffer()) {
    memset(m_buf, 0, sizeof(m_buf));
}

void LedDmaBuf::ConvertFrame(LedFrameBuf const &frame, Area const &area, uint16_t rgbAShift, uint16_t rgbBShift) {
    uint32_t startx = area.GetX();
    uint32_t starty = area.GetY();

    FW_ASSERT((area.GetWidth() == LedPanel::WIDTH) && (area.GetHeight() == LedPanel::HEIGHT));
    FW_ASSERT(LedPanel::BIT_CNT <= 8);
    // The following note is kept for reference. Reverse mapping is not needed any more by reversing the physical connection.
    //     Reverses the mapping for both x and y since (1) the first data shifted in shows up on the rightmost position, and
    //     (2) the first scan line shows up at the bottom of the panel.
    //     That is, the origin of the panel is at the lower-left corner whereas the origin of the frame buffer is at top-left corner.
    for (uint32_t lineIdx = 0; lineIdx < LedPanel::LINE_CNT; lineIdx++) {
        for (uint32_t clkIdx = 0; clkIdx < LedPanel::CLK_CNT; clkIdx++) {
            for (uint32_t bitIdx = 0; bitIdx < LedPanel::BIT_CNT; bitIdx++) {
                uint32_t y = starty + lineIdx;
                uint32_t x = startx + clkIdx;
                uint32_t bitMask = 1 << (bitIdx + (8 - LedPanel::BIT_CNT));              // Shifts left to skip over the lower significant bits.
                uint16_t data = GetRgb(frame, x, y, bitMask, rgbAShift );
                data |= GetRgb(frame, x, y + LedPanel::LINE_CNT, bitMask, rgbAShift + 3);
                data |= GetRgb(frame, x, y + LedPanel::LINE_CNT * 2, bitMask, rgbBShift);
                data |= GetRgb(frame, x, y + LedPanel::LINE_CNT * 3, bitMask, rgbBShift + 3);
                Set(bitIdx, lineIdx, clkIdx, data);
            }
        }
    }
    // Need to flush cache if write back policy is used.
    uint32_t addr = (uint32_t)m_buf;
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t *>(ROUND_DOWN_32(addr)), ROUND_UP_32(addr + sizeof(m_buf)) - ROUND_DOWN_32(addr));
}

}
