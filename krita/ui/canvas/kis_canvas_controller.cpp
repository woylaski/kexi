/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_canvas_controller.h"

#include <QMouseEvent>
#include <QTabletEvent>

#include <klocale.h>

#include "kis_paintop_transformation_connector.h"
#include "kis_coordinates_converter.h"
#include "kis_canvas2.h"
#include "kis_image.h"
#include "kis_view2.h"
#include "input/kis_input_manager.h"
#include "input/kis_tablet_event.h"

struct KisCanvasController::Private {
    Private(KisCanvasController *qq)
        : q(qq),
          paintOpTransformationConnector(0)
    {
    }

    KisView2 *view;
    KisCoordinatesConverter *coordinatesConverter;
    KisCanvasController *q;
    KisPaintopTransformationConnector *paintOpTransformationConnector;

    KisInputManager *globalEventFilter;

    void emitPointerPositionChangedSignals(QEvent *event);
    void updateDocumentSizeAfterTransform();
};

void KisCanvasController::Private::emitPointerPositionChangedSignals(QEvent *event)
{
    if (!coordinatesConverter) return;

    QPoint pointerPos;
    if (QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event)) {
        pointerPos = mouseEvent->pos();
    } else if (QTabletEvent *tabletEvent = dynamic_cast<QTabletEvent*>(event)) {
        pointerPos = tabletEvent->pos();
    } else if (KisTabletEvent *kisTabletEvent = dynamic_cast<KisTabletEvent*>(event)) {
        pointerPos = kisTabletEvent->pos();
    }

    QPointF documentPos = coordinatesConverter->widgetToDocument(pointerPos);

    q->proxyObject->emitDocumentMousePositionChanged(documentPos);
    q->proxyObject->emitCanvasMousePositionChanged(pointerPos);
}

void KisCanvasController::Private::updateDocumentSizeAfterTransform()
{
    // round the size of the area to the nearest integer instead of getting aligned rect
    QSize widgetSize = coordinatesConverter->imageRectInWidgetPixels().toRect().size();
    q->updateDocumentSize(widgetSize, true);

    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(q->canvas());
    Q_ASSERT(kritaCanvas);

    kritaCanvas->notifyZoomChanged();
}


KisCanvasController::KisCanvasController(KisView2 *parent, KActionCollection * actionCollection)
    : KoCanvasControllerWidget(actionCollection, parent),
      m_d(new Private(this))
{
    m_d->view = parent;
}

KisCanvasController::~KisCanvasController()
{
    if (m_d->globalEventFilter) {
        m_d->globalEventFilter->setupAsEventFilter(0);
    }

    delete m_d;
}

void KisCanvasController::setCanvas(KoCanvasBase *canvas)
{
    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas);
    Q_ASSERT(kritaCanvas);

    m_d->globalEventFilter = kritaCanvas->inputManager();

    m_d->coordinatesConverter =
        const_cast<KisCoordinatesConverter*>(kritaCanvas->coordinatesConverter());
    KoCanvasControllerWidget::setCanvas(canvas);

    m_d->paintOpTransformationConnector =
        new KisPaintopTransformationConnector(m_d->view, this);
}

void KisCanvasController::changeCanvasWidget(QWidget *widget)
{
    KIS_ASSERT_RECOVER_RETURN(m_d->globalEventFilter);

    m_d->globalEventFilter->setupAsEventFilter(widget);
    KoCanvasControllerWidget::changeCanvasWidget(widget);
}

void KisCanvasController::keyPressEvent(QKeyEvent *event)
{
    /**
     * Dirty Hack Alert:
     * Do not call the KoCanvasControllerWidget::keyPressEvent()
     * to avoid activation of Pan and Default tool activation shortcuts
     */
    Q_UNUSED(event);
}

bool KisCanvasController::eventFilter(QObject *watched, QEvent *event)
{
    KoCanvasBase *canvas = this->canvas();
    if (canvas && canvas->canvasWidget() && (watched == canvas->canvasWidget())) {
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::TabletMove || event->type() == (QEvent::Type)KisTabletEvent::TabletMoveEx) {
            m_d->emitPointerPositionChangedSignals(event);
            return false;
        }
    }

    return KoCanvasControllerWidget::eventFilter(watched, event);
}

void KisCanvasController::updateDocumentSize(const QSize &sz, bool recalculateCenter)
{
    KoCanvasControllerWidget::updateDocumentSize(sz, recalculateCenter);

    emit documentSizeChanged();
}

void KisCanvasController::mirrorCanvas(bool enable)
{
    QPoint newOffset = m_d->coordinatesConverter->mirror(m_d->coordinatesConverter->widgetCenterPoint(), enable, false);
    m_d->updateDocumentSizeAfterTransform();
    setScrollBarValue(newOffset);
    m_d->paintOpTransformationConnector->notifyTransformationChanged();
}

void KisCanvasController::rotateCanvas(qreal angle)
{
    QPoint newOffset = m_d->coordinatesConverter->rotate(m_d->coordinatesConverter->widgetCenterPoint(), angle);
    m_d->updateDocumentSizeAfterTransform();
    setScrollBarValue(newOffset);
    m_d->paintOpTransformationConnector->notifyTransformationChanged();
}

void KisCanvasController::rotateCanvasRight15()
{
    rotateCanvas(15.0);
}

void KisCanvasController::rotateCanvasLeft15()
{
    rotateCanvas(-15.0);
}

void KisCanvasController::resetCanvasTransformations()
{
    QPoint newOffset = m_d->coordinatesConverter->resetRotation(m_d->coordinatesConverter->widgetCenterPoint());
    m_d->updateDocumentSizeAfterTransform();
    setScrollBarValue(newOffset);
    m_d->paintOpTransformationConnector->notifyTransformationChanged();
}

void KisCanvasController::slotToggleWrapAroundMode(bool value)
{
    KisCanvas2 *kritaCanvas = dynamic_cast<KisCanvas2*>(canvas());
    Q_ASSERT(kritaCanvas);

    if (!canvas()->canvasIsOpenGL() && value) {
        m_d->view->showFloatingMessage(i18n("You are activating wrap-around mode, but have not enabled OpenGL.\n"
                                            "To visualize wrap-around mode, enable OpenGL."), QIcon());
    }

    kritaCanvas->setWrapAroundViewingMode(value);
    kritaCanvas->image()->setWrapAroundModePermitted(value);
}
