/*
 *  Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KROSS_KRITACOREKRS_PAINTER_H
#define KROSS_KRITACOREKRS_PAINTER_H

#include <api/class.h>

#include <kis_types.h>

class KisPainter;

namespace Kross {

namespace KritaCore {

class Painter : public Kross::Api::Class<Painter>
{
    public:
        explicit Painter(KisPaintLayerSP layer);
        ~Painter();
        Kross::Api::Object::Ptr paintAt(Kross::Api::List::Ptr args);
        Kross::Api::Object::Ptr setPaintColor(Kross::Api::List::Ptr args);
        Kross::Api::Object::Ptr setBrush(Kross::Api::List::Ptr args);
        Kross::Api::Object::Ptr setPaintOp(Kross::Api::List::Ptr args);
    protected:
        inline KisPaintLayerSP paintLayer() { return m_layer; }
    private:
        KisPaintLayerSP m_layer;
        KisPainter* m_painter;
};

}

}

#endif
