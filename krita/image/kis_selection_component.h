/*
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_SELECTION_COMPONENT_H
#define KIS_SELECTION_COMPONENT_H

#include <krita_export.h>

class QRect;
class QUndoCommand;
class KisSelection;
class KisPixelSelection;

class KRITAIMAGE_EXPORT KisSelectionComponent
{
public:
    KisSelectionComponent() {}
    virtual ~KisSelectionComponent() {}

    virtual KisSelectionComponent* clone(KisSelection* selection) = 0;

    virtual void renderToProjection(KisPixelSelection* projection) = 0;
    virtual void renderToProjection(KisPixelSelection* projection, const QRect& r) = 0;

    virtual void moveX(qint32 x) { Q_UNUSED(x); }
    virtual void moveY(qint32 y) { Q_UNUSED(y); }
    
    virtual QUndoCommand* transform(double  xscale, double  yscale, double  xshear, double  yshear, double angle, qint32  translatex, qint32  translatey)
    {
        Q_UNUSED(xscale);
        Q_UNUSED(yscale);
        Q_UNUSED(xshear);
        Q_UNUSED(yshear);
        Q_UNUSED(angle);
        Q_UNUSED(translatex);
        Q_UNUSED(translatey);
        return 0;
    }
};

#endif
