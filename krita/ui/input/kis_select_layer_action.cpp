/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#include "kis_select_layer_action.h"

#include <QDebug>
#include <QMouseEvent>
#include <QApplication>

#include <klocalizedstring.h>

#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_view2.h>
#include <kis_node_manager.h>

#include "kis_input_manager.h"
#include "kis_tool_utils.h"


class KisSelectLayerAction::Private
{
public:
};

KisSelectLayerAction::KisSelectLayerAction()
    : d(new Private)
{
    setName(i18n("Select Layer"));
    setDescription(i18n("Selects a layer under cursor position"));

    QHash<QString, int> shortcuts;
    shortcuts.insert(i18n("Toggle Select Layer Mode"), SelectLayerToggleShortcut);
    setShortcutIndexes(shortcuts);
}

KisSelectLayerAction::~KisSelectLayerAction()
{
    delete d;
}

int KisSelectLayerAction::priority() const
{
    return 5;
}

void KisSelectLayerAction::activate()
{
    QApplication::setOverrideCursor(Qt::PointingHandCursor);
}

void KisSelectLayerAction::deactivate()
{
    QApplication::restoreOverrideCursor();
}

void KisSelectLayerAction::begin(int shortcut, QEvent *event)
{
    KisAbstractInputAction::begin(shortcut, event);

    switch (shortcut) {
        case SelectLayerToggleShortcut:
            inputEvent(event);
            break;
    }
}

void KisSelectLayerAction::inputEvent(QEvent *event)
{
    QMouseEvent *mouseEvent;
    if (event && (mouseEvent = dynamic_cast<QMouseEvent*>(event))) {
        QPoint pos =
            inputManager()->canvas()->
            coordinatesConverter()->widgetToImage(mouseEvent->posF()).toPoint();

        KisNodeSP node = KisToolUtils::findNode(inputManager()->canvas()->image()->root(), pos, false);

        if (node) {
            inputManager()->canvas()->view()->nodeManager()->slotNonUiActivatedNode(node);
        }
    }
}
