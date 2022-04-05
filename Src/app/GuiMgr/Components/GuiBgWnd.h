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

#ifndef GUIBGWND_H
#define GUIBGWND_H

#include "fw_active.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"
#include "GuiColor.h"

#define GUIBGWND_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("GuiBgWnd.h", (int)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

class GuiMgr;

class GuiBgWnd {
public:    
    enum {
        COLOR_IDX_RED     = 0,
        COLOR_IDX_GREEN   = 512,
        COLOR_IDX_BLUE    = 1024,        
        COLOR_IDX_LIMIT   = 1536,
        INVALID_COLOR_IDX = 0xFFFFFFFF,
        MAX_COLOR_DELTA   = 100
    };

    enum GradientDir {
        GRADIENT_H,
        GRADIENT_V   
    };
    
    GuiBgWnd() : m_hsmn(HSM_UNDEF), m_hWin(0), m_color0Idx(INVALID_COLOR_IDX), m_color1Idx(INVALID_COLOR_IDX),
        m_color0(GUI_CUSTOM_DARK_BLUE), m_color1(GUI_CUSTOM_DARK_BLUE), 
        m_xSize(0), m_ySize(0), m_dir(GRADIENT_H) {}

    WM_HWIN Create(Active *owner, int x0, int y0, int xSize, int ySize, WM_CALLBACK *cb);
    WM_HWIN Destroy() {
        WM_HWIN handle = m_hWin;
        if (m_hWin) {
            WM_DeleteWindow(m_hWin);
            m_hWin = 0;
        }
        return handle;
    }
    WM_HWIN GetHandle() const { return m_hWin; }
    void SetColorIdx(uint32_t colorIdx0, uint32_t colorIdx1, GradientDir dir = GRADIENT_H);
    void SetColor(GUI_COLOR color0, GUI_COLOR color1, GradientDir dir = GRADIENT_H);
    void Update(int dc);
    void Paint();
    static GUI_COLOR GetRgb(uint32_t colorIdx);  
    
private:
    void IncColorIdx(uint32_t &idx, int dc);
    Hsmn        m_hsmn;         // Hsmn of owner.
    WM_HWIN     m_hWin;
    uint32_t    m_color0Idx;
    uint32_t    m_color1Idx;
    GUI_COLOR   m_color0;
    GUI_COLOR   m_color1;
    int         m_xSize;
    int         m_ySize;
    GradientDir m_dir;
}; 
}
#endif
        
