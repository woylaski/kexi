/* This file is part of the KDE project
 * Copyright (C) 2012 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

#include "ColorImageProvider.h"

ColorImageProvider::ColorImageProvider()
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap)
{
}

QPixmap ColorImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    int width = 100;
    int height = 50;

    if ( size )
        *size = QSize(width, height);
    QPixmap pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                   requestedSize.height() > 0 ? requestedSize.height() : height);
    if (QColor::isValidColor(id))
    {
        pixmap.fill(QColor(id).rgba());
    }
    else
    {
        QList<QString> elements = id.split(",");
        if (elements.count() == 4)
            pixmap.fill(QColor::fromRgbF(elements.at(0).toFloat(), elements.at(1).toFloat(), elements.at(2).toFloat(), elements.at(3).toFloat()));
        if (elements.count() == 3)
            pixmap.fill(QColor::fromRgbF(elements.at(0).toFloat(), elements.at(1).toFloat(), elements.at(2).toFloat()));
    }
    return pixmap;
}
