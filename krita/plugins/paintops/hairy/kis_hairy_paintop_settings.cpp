/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2008-2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include <QPainter>

#include "kis_image.h"

#include "kis_hairy_paintop_settings.h"
#include "kis_hairy_bristle_option.h"
#include "kis_hairy_shape_option.h"
#include "kis_brush_based_paintop_options_widget.h"
#include "kis_boundary.h"

const QString HAIRY_VERSION = "Hairy/Version";

KisHairyPaintOpSettings::KisHairyPaintOpSettings()
{
    setProperty(HAIRY_VERSION, "2");
}

QPainterPath KisHairyPaintOpSettings::brushOutline(const KisPaintInformation &info, OutlineMode mode) const
{
    QPainterPath path;
    if (mode == CursorIsOutline) {
        KisBrushBasedPaintopOptionWidget *widget = dynamic_cast<KisBrushBasedPaintopOptionWidget*>(optionsWidget());

        if (!widget) {
            return KisPaintOpSettings::brushOutline(info, mode);
        }

        KisBrushSP brush = widget->brush();

        qreal additionalScale = brush->scale() * getDouble(HAIRY_BRISTLE_SCALE);

        return outlineFetcher()->fetchOutline(info, this, brush->outline(), additionalScale, brush->angle());
    }
    return path;
}

void KisHairyPaintOpSettings::fromXML(const QDomElement& elt)
{
    setProperty(HAIRY_VERSION, "1"); // This make sure that fromXML will override HAIRY_VERSION with 2, or will default to 1
    KisBrushBasedPaintOpSettings::fromXML(elt);
    QVariant v;
    if (!getProperty(HAIRY_VERSION, v) || v == "1") {
        setProperty(HAIRY_BRISTLE_SCALE, 2.0 * getDouble(HAIRY_BRISTLE_SCALE));
    }
}
