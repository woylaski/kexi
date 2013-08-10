/*
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
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

#include "kis_svg_brush_factory.h"

#include <QDomDocument>

#include <KoResourceServer.h>
#include <KoResourceServerAdapter.h>

#include "kis_brush_server.h"


KisSvgBrushFactory::KisSvgBrushFactory()
{
}

KisBrushSP KisSvgBrushFactory::getOrCreateBrush(const QDomElement& brushDefinition)
{
    KoResourceServer<KisBrush>* rServer = KisBrushServer::instance()->brushServer();
    QString brushFileName = brushDefinition.attribute("filename", "");
    KisBrushSP brush = rServer->resourceByFilename(brushFileName);
    if(!brush)
        return 0;
    
    double spacing = brushDefinition.attribute("spacing", "0.25").toDouble();
    brush->setSpacing(spacing);

    return brush;
}
