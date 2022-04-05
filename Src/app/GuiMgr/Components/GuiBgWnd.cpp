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

#include "fw_log.h"
#include "fw_assert.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"
#include "GuiColor.h"
#include "GuiBgWnd.h"

FW_DEFINE_THIS_FILE("GuiBgWnd.cpp")

namespace APP {

WM_HWIN GuiBgWnd::Create(Active *owner, int x0, int y0, int xSize, int ySize, WM_CALLBACK *cb) {
    FW_ASSERT(xSize && ySize && cb);
    FW_ASSERT(owner);
    FW_ASSERT(m_hWin == 0);

    m_hsmn = owner->GetHsmn();       // For logging.
    m_xSize = xSize;
    m_ySize = ySize;
    m_hWin = WM_CreateWindow(x0, y0, xSize, ySize, WM_CF_SHOW | WM_CF_LATE_CLIP, cb, sizeof(owner));
    FW_ASSERT(m_hWin);
    WM_SetUserData(m_hWin, &owner, sizeof(owner));   
    
    m_color0Idx = INVALID_COLOR_IDX;
    m_color1Idx = INVALID_COLOR_IDX;    
    m_color0 = GUI_CUSTOM_DARK_BLUE;
    m_color1 = GUI_CUSTOM_DARK_BLUE;    
    return m_hWin;
}

void GuiBgWnd::SetColorIdx(uint32_t colorIdx0, uint32_t colorIdx1, GradientDir dir) {
    FW_ASSERT(m_hWin);
    m_color0Idx = colorIdx0;
    m_color1Idx = colorIdx1;    
    m_color0 = GetRgb(colorIdx0);
    m_color1 = GetRgb(colorIdx1);
    m_dir = dir;
    WM_Invalidate(m_hWin);        
}

void GuiBgWnd::SetColor(GUI_COLOR color0, GUI_COLOR color1, GradientDir dir)
{
    FW_ASSERT(m_hWin);
    m_color0Idx = INVALID_COLOR_IDX;
    m_color1Idx = INVALID_COLOR_IDX;        
    m_color0 = color0;
    m_color1 = color1;
    m_dir = dir;
    WM_Invalidate(m_hWin);        
}

void GuiBgWnd::Update(int dc) {
    if ((m_color0Idx != INVALID_COLOR_IDX) && (m_color1Idx != INVALID_COLOR_IDX)) {
        FW_ASSERT((abs(dc) <= (int)MAX_COLOR_DELTA));
        IncColorIdx(m_color0Idx, dc);
        m_color0 = GetRgb(m_color0Idx);
        IncColorIdx(m_color1Idx, dc);
        m_color1 = GetRgb(m_color1Idx);
        WM_Invalidate(m_hWin);           
    }
}

void GuiBgWnd::IncColorIdx(uint32_t &idx, int dc) {
    int i = (int)idx + dc;
    if (i < 0) {
        i += COLOR_IDX_LIMIT;            
    }
    if (i >= (int)COLOR_IDX_LIMIT) {
        i -= COLOR_IDX_LIMIT;
    }
    idx = (uint32_t)i;     
}

void GuiBgWnd::Paint() {
    FW_ASSERT(m_hWin);
    WM_SelectWindow(m_hWin);
    if (m_dir == GRADIENT_H) {
        GUI_DrawGradientH(0, 0, m_xSize - 1, m_ySize - 1, m_color0, m_color1);
    }
    else {
        GUI_DrawGradientV(0, 0, m_xSize - 1, m_ySize - 1, m_color0, m_color1); 
    }
}

GUI_COLOR GuiBgWnd::GetRgb(uint32_t colorIdx) {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    colorIdx %= 1536;
    if (colorIdx < 512) {
        red   = 255 - colorIdx/2;
        green = (colorIdx+1)/2;
        blue  = 0;
        if (green > 255) green = 255;
    }
    else if (colorIdx < 1024) {
        red   = 0;
        blue  = (colorIdx+1)/2 - 256;        
        green = 255 - (colorIdx/2 - 256);
        if (blue > 255) blue = 255;
    }
    else {
        red   = (colorIdx+1)/2 - 512;
        green = 0;        
        blue  = 255 - (colorIdx/2 - 512);    
        if (red > 255) red = 255;
    }
    return BYTE_TO_LONG(0, blue, green, red);
}

}
