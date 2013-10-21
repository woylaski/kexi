/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2013
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
#include "SketchDeclarativeView.h"

#include <opengl/kis_opengl.h>
#include <QWidget>
#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QResizeEvent>
#include <QApplication>
#include <QGraphicsItem>

#include "kis_coordinates_converter.h"
#include "kis_config.h"

#include "KisSketchView.h"
#include "gemini/ViewModeSwitchEvent.h"

SketchDeclarativeView::SketchDeclarativeView(QWidget *parent)
    : QDeclarativeView(parent)
    , m_drawCanvas(false)
    , m_canvasWidget(0)
    , m_GLInitialized(false)
    , m_sketchView(0)
{
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    setViewport(new QGLWidget(this, KisOpenGL::sharedContextWidget()));

    setAttribute(Qt::WA_AcceptTouchEvents);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
}

SketchDeclarativeView::SketchDeclarativeView(const QUrl &url, QWidget *parent)
    : QDeclarativeView(url, parent)
    , m_drawCanvas(false)
    , m_canvasWidget(0)
    , m_GLInitialized(false)
    , m_sketchView(0)
{
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    setViewport(new QGLWidget(this, KisOpenGL::sharedContextWidget()));

    setAttribute(Qt::WA_AcceptTouchEvents);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
}

SketchDeclarativeView::~SketchDeclarativeView()
{
    m_canvasWidget = 0;
}

QWidget* SketchDeclarativeView::canvasWidget() const
{
    return m_canvasWidget.data();
}

void SketchDeclarativeView::setCanvasWidget(QWidget *canvasWidget)
{
    m_canvasWidget = qobject_cast<KisOpenGLCanvas2*>(canvasWidget);
    connect(m_canvasWidget, SIGNAL(destroyed(QObject*)), this, SLOT(resetInitialized()));
    emit canvasWidgetChanged();
}

void SketchDeclarativeView::resetInitialized()
{
    m_GLInitialized = false;
}

bool SketchDeclarativeView::drawCanvas() const
{
    return m_drawCanvas;
}

void SketchDeclarativeView::setDrawCanvas(bool drawCanvas)
{
    if (m_drawCanvas != drawCanvas) {
        m_drawCanvas = drawCanvas;
        emit drawCanvasChanged();
    }
}

void SketchDeclarativeView::drawBackground(QPainter *painter, const QRectF &rect)
{

    if (painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("OpenGLScene: drawBackground needs a "
                 "QGLWidget to be set as viewport on the "
                 "graphics view");
        return;
    }

    if (m_drawCanvas && m_canvasWidget) {
        if (!m_GLInitialized) {
            m_canvasWidget->initializeCheckerShader();
            m_canvasWidget->initializeDisplayShader();
            m_GLInitialized = true;
        }
        m_canvasWidget->renderCanvasGL();
        m_canvasWidget->renderDecorations(painter);
    }
    else {
        QDeclarativeView::drawBackground(painter, rect);
    }

}


void SketchDeclarativeView::resizeEvent(QResizeEvent *event)
{
    if (m_canvasWidget) {
        m_canvasWidget->coordinatesConverter()->setCanvasWidgetSize(event->size());
    }

    QDeclarativeView::resizeEvent(event);
}

bool SketchDeclarativeView::event( QEvent* event )
{
    switch(static_cast<int>(event->type())) {
        case ViewModeSwitchEvent::AboutToSwitchViewModeEvent:
        case ViewModeSwitchEvent::SwitchedToSketchModeEvent:
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            // If we don't have a canvas widget yet, we don't really have anywhere to send those events
            // so... let's just not
            if (m_canvasWidget.data())
            {
                //QGraphicsScene is silly and will not forward unknown events to its items, so emulate that
                //functionality.s
                QList<QGraphicsItem*> items = scene()->items();
                Q_FOREACH(QGraphicsItem* item, items) {
                    if (item == m_sketchView || qobject_cast<KisSketchView*>((item))) {
                        if (item != m_sketchView)
                            m_sketchView = item;
                        scene()->sendEvent(item, event);
                        break;
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return QGraphicsView::event( event );
}
