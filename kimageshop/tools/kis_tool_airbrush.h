/*
 *  kis_tool_airbrush.h - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter
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

#ifndef __airbrushtool_h__
#define __airbrushtool_h__

#include <qpoint.h>
#include <qpointarray.h>

#include "kis_tool.h"

class KisBrush;

class AirBrushTool : public KisTool
{
    Q_OBJECT
    
public:

    AirBrushTool(KisDoc *doc, KisView *view, KisBrush *brush);
    ~AirBrushTool();
  
    QString toolName() { return QString("AirBrushTool"); }
    void setBrush(KisBrush *_brush);
    bool paint(QPoint pos, bool timeout);
      
public slots:

    virtual void mousePress(QMouseEvent*); 
    virtual void mouseMove(QMouseEvent*);
    virtual void mouseRelease(QMouseEvent*);
    virtual void optionsDialog();
    
    void timeoutPaint();  

protected:

    KisBrush *m_pBrush;
    QTimer *timer;    
    
    QArray <int> brushArray; // array of points in brush
    int nPoints;  // number of points marked in array
    
    QPoint  pos; 
    QPoint 	m_dragStart;
    bool   	m_dragging;
    float   m_dragdist;
    int     density; 
         
    unsigned int brushWidth;
    unsigned int brushHeight;
    
    // options
    int opacity;
    bool usePattern;
    bool useGradient;
};

#endif //__airbrushtool_h__
