/* This file is part of the KDE project
   Copyright (C) 2012 C. Boemann <cbo@kogmbh.com>

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
 * Boston, MA 02110-1301, USA.
*/

#ifndef FLOWPART_H
#define FLOWPART_H

#include <KoPart.h>

#include "flow_export.h"

class FlowDocument;
class QGraphicsItem;
class KoView;

class FLOW_EXPORT FlowPart : public KoPart
{
    Q_OBJECT

public:
    explicit FlowPart(QObject *parent);

    virtual ~FlowPart();

    void setDocument(FlowDocument *document);

    /**
     * Creates and shows the start up widget. Reimplemented from KoDocument.
     *
     * @param parent the KoMainWindow used as parent for the widget.
     * @param alwaysShow always show the widget even if the user has configured it to not show.
     */
    void showStartUpWidget(KoMainWindow *parent, bool alwaysShow);

    /// reimplemented
    virtual KoView *createViewInstance(KoDocument *document, QWidget *parent);
    /// reimplemented
    virtual QGraphicsItem *createCanvasItem(KoDocument *document);
    /// reimplemented
    virtual KoMainWindow *createMainWindow();
protected slots:
    /// Quits Stage with error message from m_errorMessage.
    void showErrorAndDie();

protected:
    QString m_errorMessage;
    FlowDocument *m_document;
};

#endif // FLOWPART_H
