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

#include "kptpart.h"

#include "kptview.h"
#include "kptmaindocument.h"
#include "kptfactory.h"

#include <kglobal.h>

Part::Part(QObject *parent)
    : KoPart(parent)
{
    setComponentData( Factory::global()); // Do not load plugins now (the view will load them)
    setTemplateType( "plan_template" );
}

Part::~Part()
{
}

void Part::setDocument(KPlato::MainDocument *document)
{
    KoPart::setDocument(document);
    m_document = document;
}

KoView *Part::createViewInstance(KoDocument *document, QWidget *parent)
{
    // synchronize view selector
    View *view = dynamic_cast<View*>(views().value(0));
    /*FIXME
    if (view && m_context) {
        QDomDocument doc = m_context->save(view);
        m_context->setContent(doc.toString());
    }*/
    view = new View(this, qobject_cast<MainDocument*>(document), parent);
//    connect(view, SIGNAL(destroyed()), this, SLOT(slotViewDestroyed()));
//    connect(document, SIGNAL(viewListItemAdded(const ViewListItem*,const ViewListItem*,int)), view, SLOT(addViewListItem(const ViewListItem*,const ViewListItem*,int)));
//    connect(document, SIGNAL(viewListItemRemoved(const ViewListItem*)), view, SLOT(removeViewListItem(const ViewListItem*)));
    return view;
}

KoMainWindow *Part::createMainWindow()
{
    return new KoMainWindow(PLAN_MIME_TYPE, componentData());
}

void Part::openTemplate(const KUrl &url)
{
    m_document->setLoadingTemplate(true);
    KoPart::openTemplate(url);
    m_document->setLoadingTemplate(false);
}
