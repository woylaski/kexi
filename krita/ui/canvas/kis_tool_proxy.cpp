/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_tool_proxy.h"
#include "kis_canvas2.h"

#include <KoToolProxy_p.h>


KisToolProxy::KisToolProxy(KoCanvasBase *canvas, QObject *parent)
    : KoToolProxy(canvas, parent)
{
}

QPointF KisToolProxy::widgetToDocument(const QPointF &widgetPoint) const
{
    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas());
    Q_ASSERT(kritaCanvas);

    return kritaCanvas->coordinatesConverter()->widgetToDocument(widgetPoint);
}

KoPointerEvent KisToolProxy::convertEventToPointerEvent(QEvent *event, const QPointF &docPoint, bool *result)
{
    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::TabletMove:
    {
        *result = true;
        QTabletEvent *tabletEvent = static_cast<QTabletEvent*>(event);
        KoPointerEvent ev(tabletEvent, docPoint);
        ev.setTabletButton(Qt::LeftButton);
        return ev;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        *result = true;
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        return KoPointerEvent(mouseEvent, docPoint);
    }
    default:
        ;
    }

    *result = false;
    QMouseEvent fakeEvent(QEvent::MouseMove, QPoint(),
                          Qt::NoButton, Qt::NoButton,
                          Qt::NoModifier);

    return KoPointerEvent(&fakeEvent, QPointF());
}

QPointF KisToolProxy::tabletToDocument(const QPointF &globalPos, const QPoint &canvasOriginWorkaround)
{
    const QPointF pos = globalPos - canvasOriginWorkaround;
    return widgetToDocument(pos);
}

bool KisToolProxy::forwardEvent(ActionState state, KisTool::ToolAction action, QEvent *event, QEvent *originalEvent, QTabletEvent *lastTabletEvent, const QPoint &canvasOriginWorkaround)
{
    bool retval = true;

    QTabletEvent *tabletEvent = dynamic_cast<QTabletEvent*>(event);
    QTouchEvent *touchEvent = dynamic_cast<QTouchEvent*>(event);
    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);

    if (tabletEvent) {
        QPointF docPoint = tabletToDocument(tabletEvent->hiResGlobalPos(), canvasOriginWorkaround);
        tabletEvent->accept();
        this->tabletEvent(tabletEvent, docPoint);
        forwardToTool(state, action, tabletEvent, docPoint);
        retval = tabletEvent->isAccepted();
    } else if (touchEvent) {
        if (state == END && touchEvent->type() != QEvent::TouchEnd) {
            //Fake a touch end if we are "upgrading" a single-touch gesture to a multi-touch gesture.
            QTouchEvent fakeEvent(QEvent::TouchEnd, touchEvent->deviceType(), touchEvent->modifiers(), touchEvent->touchPointStates(), touchEvent->touchPoints());
            this->touchEvent(&fakeEvent);
        } else {
            this->touchEvent(touchEvent);
        }
    } else if (mouseEvent) {
        if (lastTabletEvent) {
            QPointF docPoint = tabletToDocument(lastTabletEvent->hiResGlobalPos(), canvasOriginWorkaround);
            lastTabletEvent->accept();
            this->tabletEvent(lastTabletEvent, docPoint);
            forwardToTool(state, action, lastTabletEvent, docPoint);
            retval = lastTabletEvent->isAccepted();
        } else {
            QPointF docPoint = widgetToDocument(mouseEvent->posF());
            mouseEvent->accept();
            if (mouseEvent->type() == QEvent::MouseButtonPress) {
                mousePressEvent(mouseEvent, docPoint);
            } else if (mouseEvent->type() == QEvent::MouseButtonDblClick) {
                mouseDoubleClickEvent(mouseEvent, docPoint);
            } else if (mouseEvent->type() == QEvent::MouseButtonRelease) {
                mouseReleaseEvent(mouseEvent, docPoint);
            } else if (mouseEvent->type() == QEvent::MouseMove) {
                mouseMoveEvent(mouseEvent, docPoint);
            }
            forwardToTool(state, action, originalEvent, docPoint);
            retval = mouseEvent->isAccepted();
        }
    } else if(event->type() == QEvent::KeyPress) {
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        keyPressEvent(kevent);
    } else if(event->type() == QEvent::KeyRelease) {
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        keyReleaseEvent(kevent);
    }

    return retval;
}

void KisToolProxy::forwardToTool(ActionState state, KisTool::ToolAction action, QEvent *event, const QPointF &docPoint)
{
    bool eventValid = false;
    KoPointerEvent ev = convertEventToPointerEvent(event, docPoint, &eventValid);
    KisTool *activeTool = dynamic_cast<KisTool*>(priv()->activeTool);

    if (!eventValid || !activeTool) return;

    switch (state) {
    case BEGIN:
        if (action == KisTool::Primary) {
            if (event->type() == QEvent::MouseButtonDblClick) {
                activeTool->beginPrimaryDoubleClickAction(&ev);
            } else {
                activeTool->beginPrimaryAction(&ev);
            }
        } else {
            if (event->type() == QEvent::MouseButtonDblClick) {
                activeTool->beginAlternateDoubleClickAction(&ev, KisTool::actionToAlternateAction(action));
            } else {
                activeTool->beginAlternateAction(&ev, KisTool::actionToAlternateAction(action));
            }
        }
        break;
    case CONTINUE:
        if (action == KisTool::Primary) {
            activeTool->continuePrimaryAction(&ev);
        } else {
            activeTool->continueAlternateAction(&ev, KisTool::actionToAlternateAction(action));
        }
        break;
    case END:
        if (action == KisTool::Primary) {
            activeTool->endPrimaryAction(&ev);
        } else {
            activeTool->endAlternateAction(&ev, KisTool::actionToAlternateAction(action));
        }
        break;
    }
}

bool KisToolProxy::primaryActionSupportsHiResEvents() const
{
    KisTool *activeTool = dynamic_cast<KisTool*>(const_cast<KisToolProxy*>(this)->priv()->activeTool);
    return activeTool && activeTool->primaryActionSupportsHiResEvents();
}
