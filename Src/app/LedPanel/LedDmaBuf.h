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

#ifndef LEDDMABUF_H
#define LEDDMABUF_H

#include <stdint.h>
#include <string.h>
#include "fw_assert.h"
#include "LedPanel.h"
#include "LedFrameBuf.h"

#define LEDDMABUF_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LedDmaBuf.h", (int)__LINE__))

namespace APP {

class Area;

// Helper class for LedPanel to manage DMA buffers.
class LedDmaBuf {
public:
    enum {
        R_MASK = 0x01,
        G_MASK = 0x02,
        B_MASK = 0x04,
        RGB_MASK = R_MASK | G_MASK | B_MASK,    // It is assumed the 6 bits for the two RGB scan sets in each group are contiguous.
    };
    
    // Reference to an array.
    // Each 16-bit entry stores 4 RGB values, 2 per group (RGB_A and RGB_B).
    typedef uint16_t (&Buffer)[LedPanel::BIT_CNT][LedPanel::LINE_CNT][LedPanel::CLK_CNT];

    LedDmaBuf();
    
    // color bit mask
    static uint8_t BitMask(uint32_t bitIdx) {
        //LEDDMABUF_ASSERT((bitIdx < LedPanel::BIT_CNT) && (LedPanel::BIT_CNT <= 8));
        return 1 << (bitIdx + (8 - LedPanel::BIT_CNT));
    }

    uint16_t const *GetBuf(uint32_t bitIdx, uint32_t lineIdx, uint32_t &cnt) const {
        cnt = LedPanel::CLK_CNT;
        //LEDDMABUF_ASSERT(cnt <= ARRAY_COUNT(m_buf[0][0]));
        //LEDDMABUF_ASSERT((bitIdx < LedPanel::BIT_CNT) && (lineIdx < LedPanel::LINE_CNT));
        return m_buf[bitIdx][lineIdx];
    }
    
    void Set(uint32_t bitIdx, uint32_t lineIdx, uint32_t clkIdx, uint16_t data) {
        //LEDDMABUF_ASSERT((bitIdx < LedPanel::BIT_CNT) && (lineIdx < LedPanel::LINE_CNT) && (clkIdx < LedPanel::CLK_CNT));
        m_buf[bitIdx][lineIdx][clkIdx] = data;
    }
    
    uint16_t GetRgb(LedFrameBuf const &frame, uint32_t x, uint32_t y, uint32_t bitMask, uint32_t shift) {
        uint16_t rgb = ((frame.Red(x, y) & bitMask) ? R_MASK : 0 ) |
                       ((frame.Green(x, y) & bitMask) ? G_MASK : 0) |
                       ((frame.Blue(x, y) & bitMask) ? B_MASK : 0);
        return rgb << shift;
    }

    LedDmaBuf &operator=(const LedDmaBuf &dmaBuf) {
        if (this != &dmaBuf) {
            memcpy(m_buf, dmaBuf.m_buf, sizeof(m_buf));          
        }
        return *this;        
    }
    
    void ConvertFrame(LedFrameBuf const &frame, Area const &area, uint16_t rgbAShift, uint16_t rgbBShift);
    
private:
    Buffer &m_buf;      // Reference to DMA buffer statically allocated to TCM.
};

}

#endif
