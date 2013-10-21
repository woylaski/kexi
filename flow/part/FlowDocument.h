/* This file is part of the KDE project
   Copyright (C)  2006 Peter Simonsson <peter.simonsson@gmail.com>
   Copyright (C)  2007 Thorsten Zachmann <zachmann@kde.okde.org>
   Copyright (C) 2010 Boudewijn Rempt <boud@valdyas.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef FLOWDOCUMENT_H
#define FLOWDOCUMENT_H

#include <KoPADocument.h>
#include "flow_export.h"

#define FLOW_MIME_TYPE "application/vnd.oasis.opendocument.graphics"

class KoPart;

class FLOW_EXPORT FlowDocument : public KoPADocument
{
    Q_OBJECT

public:
    explicit FlowDocument(KoPart *part);
    ~FlowDocument();

    virtual KoOdf::DocumentType documentType() const;


    /// reimplemented from KoDocument
    virtual QByteArray nativeFormatMimeType() const { return FLOW_MIME_TYPE; }
    /// reimplemented from KoDocument
    virtual QByteArray nativeOasisMimeType() const {return FLOW_MIME_TYPE;}
    /// reimplemented from KoDocument
    virtual QStringList extraNativeMimeTypes() const
    {
        return QStringList() << "application/vnd.oasis.opendocument.graphics-template";
    }

signals:
    /// Emitted when the gui needs to be updated.
    void updateGui();

protected:
    const char *odfTagName( bool withNamespace );
};

#endif
