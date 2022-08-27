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

#ifndef GUIBMP_H
#define GUIBMP_H

#include "fw_active.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"

using namespace QP;
using namespace FW;

#define GUIBMP_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("GuiBmp.h", (int)__LINE__))

namespace APP {

class GuiMgr;

class GuiBmp
{
public:   
    enum {
        IMG_CNT  = 100
    };    
    GuiBmp() : m_hsmn(HSM_UNDEF), m_hWin(0), m_hParent(0), m_imgCnt(0), m_imgIdx(0),
        m_xPos(0), m_yPos(0), m_xSize(0), m_ySize(0), m_vxPos(0), m_vyPos(0) {
        memset(m_bitmap, 0, sizeof(m_bitmap));
    }
    WM_HWIN Create(Active *owner, WM_HWIN hParent, int xPos, int yPos, int xSize, int ySize, WM_CALLBACK *cb);
    WM_HWIN Destroy() {
        WM_HWIN handle = m_hWin;
        if (m_hWin) {
            WM_DeleteWindow(m_hWin);
            m_hWin = 0;
        }
        return handle;
    }
    void Hide() {
        WM_HideWindow(m_hWin);        
    }
    void Show() {
        WM_ShowWindow(m_hWin);        
    }
    void SetBitmap(uint32_t imgIdx, GUI_BITMAP const *bitmap);
    // dx is amount to shift right (+ve for right, -ve for left)
    void Update(int dx, uint32_t& imgIdx, uint32_t& offsetLeft, uint32_t& offsetRight);
    void Paint();

private:
    uint32_t GetNextImgIdx() {
        if ((m_imgIdx + 1) >= m_imgCnt) return 0;
        return (m_imgIdx + 1);
    }
    int GetImgXSize(uint32_t imgIdx) {
        GUIBMP_ASSERT(imgIdx < m_imgCnt);
        return (int)(m_bitmap[imgIdx]->XSize);
    }
    int GetImgYSize(uint32_t imgIdx) {
        GUIBMP_ASSERT(imgIdx < m_imgCnt);
        return (int)(m_bitmap[imgIdx]->YSize);
    }
    
    Hsmn              m_hsmn;         // Hsmn of owner.
    WM_HWIN           m_hWin;
    WM_HWIN           m_hParent;
    uint32_t          m_imgCnt;
    uint32_t          m_imgIdx;
    GUI_BITMAP const *m_bitmap[IMG_CNT];
    int               m_xPos;
    int               m_yPos;
    int               m_xSize;
    int               m_ySize;
    int               m_vxPos;
    int               m_vyPos;
};

}

#endif
        
