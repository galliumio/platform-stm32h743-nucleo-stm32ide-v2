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

#ifndef GUITEXT_H
#define GUITEXT_H

#include "fw_active.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"


#define GUITEXT_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("GuiText.h", (int)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

class GuiMgr;

/**
 *****************************************************************************
 * @class GuiText
 *****************************************************************************
 */
class GuiText
{
public:   
    enum
    {
        BUF_CNT  = 3,
        MAX_TEXT_LEN = 1024 
    };

    GuiText()
        : m_hsmn(HSM_UNDEF), m_hWin(0), m_hParent(0), m_bufCnt(BUF_CNT), m_bufIdx(0), m_borColor(GUI_BLACK),
          m_xPos(0), m_yPos(0),
          m_xSize(0), m_ySize(0), m_vxPos(0), m_vyPos(0)
    {
        memset(m_textBuf, 0, sizeof(m_textBuf));
        memset(m_ySizeText, 0, sizeof(m_ySizeText));
        memset(m_pFont, 0, sizeof(m_pFont));
        memset(m_textColor, 0, sizeof(m_textColor));
        memset(m_bgColor, 0, sizeof(m_bgColor));              
    }

    WM_HWIN Create(Active *owner, WM_HWIN hParent, int xPos, int yPos,
                   int xSize, int ySize, GUI_COLOR borColor, WM_CALLBACK *cb);
    WM_HWIN Destroy() {
        WM_HWIN handle = m_hWin;
        if (m_hWin) {
            WM_DeleteWindow(m_hWin);
            m_hWin = 0;
        }
        return handle;
    }
    void Hide()
    {
        WM_HideWindow(m_hWin);        
    }
    void Show()
    {
        WM_ShowWindow(m_hWin);        
    }
    void SetText(uint32_t bufIdx, const char *pText, const GUI_FONT *pFont, 
                 GUI_COLOR textColor, GUI_COLOR bgColor);
    void SetBorder(GUI_COLOR borColor)
    {
        m_borColor = borColor;
        WM_Invalidate(m_hWin);          
    }       
    // dy is amount to shift down (+ve for down, -ve for up)
    // only up is supported
    void Update(int dy, uint32_t& bufIdx, uint32_t& offsetLeft, uint32_t& offsetRight);
    void Paint();
   
private:
    uint32_t GetNextBufIdx() {
        uint32_t newBufIdx = m_bufIdx;
        do {
            newBufIdx = (newBufIdx + 1) >= m_bufCnt ? 0 : newBufIdx + 1;
        } while((newBufIdx != m_bufIdx) && !m_textBuf[newBufIdx][0]);
        return newBufIdx;
    }
    Hsmn              m_hsmn;         // Hsmn of owner.
    WM_HWIN           m_hWin;
    WM_HWIN           m_hParent;
    uint32_t          m_bufCnt;    
    const GUI_FONT  * m_pFont[BUF_CNT];
    char              m_textBuf[BUF_CNT][MAX_TEXT_LEN];
    uint32_t          m_bufIdx;
    GUI_COLOR         m_textColor[BUF_CNT];
    GUI_COLOR         m_bgColor[BUF_CNT];
    GUI_COLOR         m_borColor;
    int               m_ySizeText[BUF_CNT];
    int               m_xPos;
    int               m_yPos;
    int               m_xSize;
    int               m_ySize;
    int               m_vxPos;
    int               m_vyPos;

}; 

}
#endif
        
