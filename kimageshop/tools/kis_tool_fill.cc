/*
 *  kis_tool_fill.cc - part of Krayon
 *
 *  Copyright (c) 2000 John Califf <jcaliff@compuzone.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <kdebug.h>

#include "kis_doc.h"
#include "kis_view.h"
#include "kis_framebuffer.h"
#include "kis_cursor.h"
#include "kis_tool_fill.h"
#include "kis_dlg_toolopts.h"


FillTool::FillTool(KisDoc *doc, KisView *view)
  : KisTool(doc, view)
{
    m_Cursor = KisCursor::pickerCursor();
    
    fillOpacity = 255;
    usePattern  = false;
    useGradient = false;
        
    toleranceRed = 0;
    toleranceGreen = 0;
    toleranceBlue = 0;

    layerAlpha = true;
}

FillTool::~FillTool() {}

// floodfill based on GPL code in gpaint by Li-Cheng (Andy) Tai

int FillTool::is_old_pixel_value(struct fillinfo *info, int x, int y)
{
   unsigned char o_r = fLayer->pixel(0, x, y);
   unsigned char o_g = fLayer->pixel(1, x, y);   
   unsigned char o_b = fLayer->pixel(2, x, y);   

   if ((o_r == info->o_r) 
   && (o_g == info->o_g) 
   && (o_b == info->o_b))
      return 1;
   
   return 0;
}


void FillTool::set_new_pixel_value(struct fillinfo *info, int x, int y)
{
    // fill with pattern
    if(useGradient)
    {
        m_pDoc->frameBuffer()->setGradientToPixel(fLayer, x, y);
    }

    // fill with pattern
    else if(usePattern)
    {
        m_pDoc->frameBuffer()->setPatternToPixel(fLayer, x, y, 0);
    }

    // fill with color
    else
    {
        fLayer->setPixel(0, x, y, info->r);
        fLayer->setPixel(1, x, y, info->g);   
        fLayer->setPixel(2, x, y, info->b);
    }
    
    // alpha adjustment with either fill method
    if(layerAlpha)
    {
        fLayer->setPixel(3, x, y, fillOpacity);
    }    
}


#define STACKSIZE 10000

/* algorithm based on SeedFill.c from GraphicsGems 1 */

void FillTool::flood_fill(struct fillinfo *info, int x, int y)
{
   struct fillpixelinfo stack[STACKSIZE];
   struct fillpixelinfo * sp = stack;
   int l, x1, x2, dy;
   
#define PUSH(py, pxl, pxr, pdy) \
{  struct fillpixelinfo *p = sp;\
   if (((py) + (pdy) >= info->top) && ((py) + (pdy) < info->bottom))\
   {\
      p->y = (py);\
      p->xl = (pxl);\
      p->xr = (pxr);\
      p->dy = (pdy);\
      sp++; \
   }\
}
   
#define POP(py, pxl, pxr, pdy) \
{\
   sp--;\
   (py) = sp->y + sp->dy;\
   (pxl) = sp->xl;\
   (pxr) = sp->xr;\
   (pdy) = sp->dy;\
}

   if ((x >= info->left) && (x <= info->right) 
   && (y >= info->top) && (y <= info->bottom))
   {
        if((fLayer->pixel(0, x, y) == info->r)
        && (fLayer->pixel(1, x, y) == info->g)
        && (fLayer->pixel(2, x, y) == info->b))
            return;
      
        PUSH(y, x, x, 1);
        PUSH(y + 1, x, x, -1);
      	 
        while (sp > stack)	
        {
            POP(y, x1, x2, dy);
	        for (x = x1; (x >= info->left) && is_old_pixel_value(info, x, y); x--)
	            set_new_pixel_value(info, x, y);
	        if (x >= x1) goto skip;
	        l = x + 1;
	        if (l < x1)
	            PUSH(y, l, x1 - 1, -dy);
	        x = x1 + 1;
	        do
	        {
	            for (; (x <= info->right) && is_old_pixel_value(info, x, y); x++)
	                set_new_pixel_value(info, x, y);
	    
	            PUSH(y, l, x - 1, dy);
	            if (x > x2 + 1)
	                PUSH(y, x2 + 1, x - 1, -dy);
skip:
                for (x++; x <= x2 && !is_old_pixel_value(info, x, y); x++);
	            l = x;
	        } while (x <= x2);
        }
   }

#undef POP
#undef PUSH
   	 
}   
   

void FillTool::seed_flood_fill( int x, int y, QRect & frect )
{
    struct fillinfo fillinfo;
   
    fillinfo.left   = frect.left();
    fillinfo.top    = frect.top();
    fillinfo.right  = frect.right();
    fillinfo.bottom = frect.bottom();
    
    // r,g,b are color to set to   
    fillinfo.r = nRed; 
    fillinfo.g = nGreen; 
    fillinfo.b = nBlue; 

    // original color at mouse click position 
    fillinfo.o_r =  sRed; 
    fillinfo.o_g =  sGreen; 
    fillinfo.o_b =  sBlue; 
   
    flood_fill(&fillinfo, x, y);
}


bool FillTool::flood(int startX, int startY)
{
    int startx = startX;
    int starty = startY;
    
    KisImage *img = m_pDoc->current();
    if (!img) return false;    

    KisLayer *lay = img->getCurrentLayer();
    if (!lay) return false;

    if (!img->colorMode() == cm_RGB && !img->colorMode() == cm_RGBA)
	    return false;
    
    layerAlpha = (img->colorMode() == cm_RGBA);
    fLayer = lay;
    
    // source color values of selected pixed
    sRed    = lay->pixel(0, startx, starty);
    sGreen  = lay->pixel(1, startx, starty);
    sBlue   = lay->pixel(2, startx, starty);

    // new color values from color selector 

    nRed     = m_pView->fgColor().R();
    nGreen   = m_pView->fgColor().G();
    nBlue    = m_pView->fgColor().B();
    
    int left    = lay->imageExtents().left(); 
    int top     = lay->imageExtents().top();    
    int width   = lay->imageExtents().width();    
    int height  = lay->imageExtents().height();    

    QRect floodRect(left, top, width, height);
    
    kdDebug() << "floodRect.left() " << floodRect.left() 
              << "floodRect.top() "  << floodRect.top() << endl;

    /* set up gradient - if any.  this should only be done when the
    current layer is changed or when the fgColor or bgColor are changed,
    or when the gradient is changed with the gradient settings dialog
    or by selecting a prexisting gradient from the chooser.
    Otherwise, it can get slow calculating gradients for every fill
    operation when this calculation is not needed - when the gradient
    array is already filled with current values */
    
    if(useGradient)
    {
        KisColor startColor(m_pView->fgColor().R(),
            m_pView->fgColor().G(), m_pView->fgColor().B());
        KisColor endColor(m_pView->bgColor().R(),
            m_pView->bgColor().G(), m_pView->bgColor().B());        
            
        m_pDoc->frameBuffer()->setGradientPaint(true, startColor, endColor);        
    }

    seed_flood_fill( startx, starty, floodRect);
      
    /* refresh canvas so changes show up */
    QRect updateRect(0, 0, img->width(), img->height());
    img->markDirty(updateRect);
  
    return true;
}


void FillTool::mousePress(QMouseEvent *e)
{
    KisImage * img = m_pDoc->current();
    if (!img) return;

    if (e->button() != QMouseEvent::LeftButton
    && e->button() != QMouseEvent::RightButton)
        return;

    QPoint pos = e->pos();
    pos = zoomed(pos);
        
    if( !img->getCurrentLayer()->visible() )
        return;
  
    if( !img->getCurrentLayer()->imageExtents().contains(pos))
        return;
  
    /*  need to fill with foreground color on left click,
    transparent on middle click, and background color on right click,
    need another paramater or nned to set color here and pass in */
    
    if (e->button() == QMouseEvent::LeftButton)
        flood(pos.x(), pos.y());
    else if (e->button() == QMouseEvent::RightButton)
        flood(pos.x(), pos.y());
}

void FillTool::optionsDialog()
{
    ToolOptsStruct ts;    
    
    ts.usePattern       = usePattern;
    ts.useGradient      = useGradient;

    ToolOptionsDialog *pOptsDialog 
        = new ToolOptionsDialog(tt_filltool, ts);

    pOptsDialog->exec();
    
    if(!pOptsDialog->result() == QDialog::Accepted)
        return;

    fillOpacity     = pOptsDialog->fillToolTab()->opacity();
    usePattern      = pOptsDialog->fillToolTab()->usePattern();
    useGradient     = pOptsDialog->fillToolTab()->useGradient();

    //toleranceRed    = pOptsDialog->ToleranceRed();
    //toleranceGreen  = pOptsDialog->ToleranceGreen();    
    //toleranceBlue   = pOptsDialog->ToleranceBlue();    
}
