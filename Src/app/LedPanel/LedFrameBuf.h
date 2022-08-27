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
#ifndef LEDFRAMEBUF_H
#define LEDFRAMEBUF_H

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "Graphics.h"
#include "fw_assert.h"

#define LEDFRAMEBUF_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LedFrameBuf.h", (int)__LINE__))

namespace APP {

class LedFrameBuf {
public:
    enum {
        X_PIXEL_CNT     = 128,      // Should match LedPanel::CLK_CNT.
        Y_PIXEL_CNT     = 128,      // Should match LedPanel::LINE_CNT
        BYTES_PER_PIXEL = 3
    };    
    // Color order of internal storage.
    enum {
        RED_IDX   = 0,
        GREEN_IDX = 1,
        BLUE_IDX  = 2
    };
    // Color order in BMP file for importing.
    enum {
        BMP_RED_IDX   = 2,
        BMP_GREEN_IDX = 1,
        BMP_BLUE_IDX  = 0        
    };

    // Reference to an array.
    typedef uint8_t (&PixelBuf)[Y_PIXEL_CNT][X_PIXEL_CNT][BYTES_PER_PIXEL];

    LedFrameBuf(uint8_t *buf, uint32_t len) : m_buf(reinterpret_cast<PixelBuf>(*buf)) {
        LEDFRAMEBUF_ASSERT(len >= sizeof(PixelBuf));
        memset(m_buf, 0, sizeof(m_buf));
        LEDFRAMEBUF_ASSERT(X_PIXEL_CNT > 0);
        LEDFRAMEBUF_ASSERT(Y_PIXEL_CNT > 0);
    }

    uint8_t const *GetBuf(uint32_t &len) const {
        len = sizeof(m_buf);
        return &m_buf[0][0][0];
    }
    void SetBuf(uint8_t const *buf, uint32_t len) {
        LEDFRAMEBUF_ASSERT(len == sizeof(m_buf));
        memcpy(m_buf, buf, len);
    }

    // Const version of accessor functions.
    uint8_t const &Red(uint32_t x, uint32_t y) const  { return m_buf[y][x][RED_IDX]; }
    uint8_t const &Green(uint32_t x, uint32_t y) const { return m_buf[y][x][GREEN_IDX]; }
    uint8_t const &Blue(uint32_t x, uint32_t y) const { return m_buf[y][x][BLUE_IDX]; }
    // Non-const version of accessor functions.
    uint8_t &Red(uint32_t x, uint32_t y)   { return m_buf[y][x][RED_IDX]; }
    uint8_t &Green(uint32_t x, uint32_t y) { return m_buf[y][x][GREEN_IDX]; }
    uint8_t &Blue(uint32_t x, uint32_t y)  { return m_buf[y][x][BLUE_IDX]; }
    
private:
    // Frame buffer with origin at the upper-left corner.
    // It supports up to 24-bit per pixel (8-bit per color).
    // It is a reference to an externally allocated memory buffer (e.g. lcdBuf in emwin).
    PixelBuf m_buf;
};

}

#endif
