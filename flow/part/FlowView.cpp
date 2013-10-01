/* This file is part of the KDE project
   Copyright (C)  2006 Peter Simonsson <peter.simonsson@gmail.com>

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

#include "FlowView.h"

#include "FlowDocument.h"
#include "FlowPart.h"

#include <KoShapeManager.h>
#include <KoSelection.h>

#include <klocale.h>
#include <kactioncollection.h>
#include <kactionmenu.h>

FlowView::FlowView(FlowPart *part, FlowDocument* document, QWidget* parent)
  : KoPAView(part, document, KoPAView::NormalMode, parent), m_document(document)
{
    Q_ASSERT(m_document);

    setXMLFile("flow.rc");

    initializeActions();
    initializeGUI();

    connect(m_document, SIGNAL(updateGui()), this, SLOT(updateGui()));

    setAcceptDrops(true);
}

FlowView::~FlowView()
{
}

FlowDocument* FlowView::document() const
{
    return m_document;
}

void FlowView::initializeGUI()
{
}

void FlowView::initializeActions()
{
    actionCollection()->action("configure")->setText(i18n("Configure Flow..."));
}

void FlowView::updateGui()
{
    selectionChanged();
}

#include "FlowView.moc"
