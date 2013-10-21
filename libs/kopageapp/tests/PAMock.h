/* This file is part of the KDE project
 * Copyright ( C ) 2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ( at your option ) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PAMOCK_H
#define PAMOCK_H

#include "KoPADocument.h"

#include <KoPart.h>
#include <KoOdf.h> 
#include <QGraphicsItem>

class KoView;

class MockDocument : public KoPADocument
{
public:
    MockDocument()
    : KoPADocument( new MockPart )
    {}
    const char *odfTagName( bool b ) { return KoOdf::bodyContentElement( KoOdf::Presentation, b ); }
    virtual KoOdf::DocumentType documentType() const { return KoOdf::Presentation; }

    /// reimplemented from KoDocument
    virtual QByteArray nativeFormatMimeType() const { return ""; }
    /// reimplemented from KoDocument
    virtual QByteArray nativeOasisMimeType() const {return "";}
    /// reimplemented from KoDocument
    virtual QStringList extraNativeMimeTypes() const
    {
        return QStringList();
    }

};


#endif // PAMOCK_H
