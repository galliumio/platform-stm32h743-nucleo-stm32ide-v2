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

#include "fw_log.h"
#include "fw_assert.h"
#include "WM.h"
#include "LCDConf_Lin_Template.h"
#include "GuiBmp.h"

FW_DEFINE_THIS_FILE("GuiBmp.cpp")

namespace APP {

WM_HWIN GuiBmp::Create(Active *owner, WM_HWIN hParent, int xPos, int yPos, int xSize, int ySize, WM_CALLBACK *cb) {
    FW_ASSERT(hParent && xSize && ySize && cb);
    FW_ASSERT(owner);
    FW_ASSERT(m_hWin == 0);
    
    m_hsmn = owner->GetHsmn();       // For logging.
    m_xPos = xPos;
    m_yPos = yPos;
    m_xSize = xSize;
    m_ySize = ySize;
    m_hParent = hParent;
    m_hWin = WM_CreateWindowAsChild(xPos, yPos, xSize, ySize, hParent, WM_CF_SHOW | WM_CF_MEMDEV, cb, sizeof(owner));
    FW_ASSERT(m_hWin);
    WM_SetUserData(m_hWin, &owner, sizeof(owner));   
    
    m_imgCnt = 0;
    m_imgIdx = 0;
    memset(m_bitmap, 0, sizeof(m_bitmap));
    m_vxPos = 0;
    m_vyPos = 0;        
    return m_hWin;
}

void GuiBmp::SetBitmap(uint32_t imgIdx, GUI_BITMAP const *bitmap) {
    FW_ASSERT(bitmap);
    FW_ASSERT(m_hWin);
    FW_ASSERT(imgIdx < IMG_CNT);
    
    if (imgIdx > m_imgCnt) {
        // New index must start from last used one
        Log::Debug(Log::TYPE_ERROR, m_hsmn, "GuiBmp::SetBitmap - Invalid imgIdx %d (> m_imgCnt %d)", imgIdx, m_imgCnt);
        return;
    }
    if (imgIdx == m_imgCnt) {
        // A new entry added just past last used one
        m_imgCnt++;
    }
    m_bitmap[imgIdx] = bitmap;
    WM_Invalidate(m_hWin);            
}

void GuiBmp::Update(int dx, uint32_t& imgIdx, uint32_t& offsetLeft, uint32_t& offsetRight)
{
    FW_ASSERT(m_hWin);
    if (m_imgCnt == 0) {
        return;
    }
    if ((m_imgCnt == 1) && (GetImgXSize(0) <= m_xSize)) {
        // no need to scroll, reset
        m_vxPos = 0;
    }
    else {
        m_vxPos += dx;
        if ((m_vxPos + GetImgXSize(m_imgIdx)) <= 0) {
            m_vxPos = 0;
            m_imgIdx = GetNextImgIdx();
        }
    }
    imgIdx = m_imgIdx;
    offsetLeft = 0 - m_vxPos;
    offsetRight = m_vxPos + GetImgXSize(m_imgIdx) - 0;
    WM_Invalidate(m_hWin);    
    return;
}

void GuiBmp::Paint() {
    FW_ASSERT(m_hWin);
    WM_SelectWindow(m_hWin);
    GUI_SetBkColor(0);
    GUI_Clear();
    if (m_imgCnt > 0) {
        FW_ASSERT(m_imgIdx < m_imgCnt);
        GUI_DrawBitmap(m_bitmap[m_imgIdx], m_vxPos, m_vyPos);
        uint32_t nextIdx = GetNextImgIdx();
        FW_ASSERT(nextIdx < m_imgCnt);
        GUI_DrawBitmap(m_bitmap[nextIdx], m_vxPos + GetImgXSize(m_imgIdx), m_vyPos);
    }
}

}
