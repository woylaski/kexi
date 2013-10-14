/*
 *  kis_tool_freehand.cc - part of Krita
 *
 *  Copyright (c) 2003-2007 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Bart Coppens <kde@bartcoppens.be>
 *  Copyright (c) 2007,2008,2010 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2009 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "kis_tool_freehand.h"

#include <QPainter>
#include <QRect>
#include <QThreadPool>

#include <kaction.h>
#include <kactioncollection.h>

#include <KoIcon.h>
#include <KoPointerEvent.h>
#include <KoViewConverter.h>
#include <KoCanvasController.h>

//pop up palette
#include <kis_canvas_resource_provider.h>

// Krita/image
#include <kis_layer.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_paintop.h>
#include <kis_selection.h>
#include <kis_paintop_preset.h>


// Krita/ui
#include "kis_abstract_perspective_grid.h"
#include "kis_config.h"
#include "canvas/kis_canvas2.h"
#include "kis_cursor.h"
#include <kis_view2.h>
#include <kis_painting_assistants_manager.h>
#include "kis_painting_information_builder.h"
#include "kis_tool_freehand_helper.h"
#include "kis_recording_adapter.h"
#include "strokes/freehand_stroke.h"


static const int HIDE_OUTLINE_TIMEOUT = 800; // ms

KisToolFreehand::KisToolFreehand(KoCanvasBase * canvas, const QCursor & cursor, const QString & /*transactionText*/)
    : KisToolPaint(canvas, cursor)
{
    m_explicitShowOutline = false;

    m_assistant = false;
    m_magnetism = 1.0;

    setSupportOutline(true);

    m_outlineTimer.setSingleShot(true);
    connect(&m_outlineTimer, SIGNAL(timeout()), this, SLOT(hideOutline()));

    m_infoBuilder = new KisToolPaintingInformationBuilder(this);
    m_recordingAdapter = new KisRecordingAdapter();
    m_helper = new KisToolFreehandHelper(m_infoBuilder, m_recordingAdapter);
}

KisToolFreehand::~KisToolFreehand()
{
    delete m_helper;
    delete m_recordingAdapter;
    delete m_infoBuilder;
}

KisPaintingInformationBuilder* KisToolFreehand::paintingInformationBuilder() const
{
    return m_infoBuilder;
}

KisRecordingAdapter* KisToolFreehand::recordingAdapter() const
{
    return m_recordingAdapter;
}

void KisToolFreehand::resetHelper(KisToolFreehandHelper *helper)
{
    delete m_helper;
    m_helper = helper;
}

int KisToolFreehand::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP|KisTool::FLAG_USES_CUSTOM_PRESET;
}

void KisToolFreehand::activate(ToolActivation activation, const QSet<KoShape*> &shapes)
{
    KisToolPaint::activate(activation, shapes);
}

void KisToolFreehand::deactivate()
{
    if (mode() == PAINT_MODE) {
        endStroke();
        setMode(KisTool::HOVER_MODE);
    }
    KisToolPaint::deactivate();
}

void KisToolFreehand::initStroke(KoPointerEvent *event)
{
    setCurrentNodeLocked(true);

    m_helper->setSmoothness(m_smoothingOptions);
    m_helper->initPaint(event, canvas()->resourceManager(),
                        image(),
                        image().data(),
                        image()->postExecutionUndoAdapter());
}

void KisToolFreehand::doStroke(KoPointerEvent *event)
{
    m_helper->paint(event);
}

void KisToolFreehand::endStroke()
{
    m_helper->endPaint();
    setCurrentNodeLocked(false);
}

void KisToolFreehand::mousePressEvent(KoPointerEvent *e)
{
    if (mode() == KisTool::PAINT_MODE)
        return;

    /**
     * FIXME: we need some better way to implement modifiers
     * for a paintop level
     */
    QPointF pos = adjustPosition(e->point, e->point);
    qreal perspective = 1.0;
    foreach (const KisAbstractPerspectiveGrid* grid, static_cast<KisCanvas2*>(canvas())->view()->resourceProvider()->perspectiveGrids()) {
        if (grid->contains(pos)) {
            perspective = grid->distance(pos);
            break;
        }
    }
    bool eventIgnored = currentPaintOpPreset()->settings()->mousePressEvent(KisPaintInformation(convertToPixelCoord(e->point),
                                                                                                pressureToCurve(e->pressure()), e->xTilt(), e->yTilt(),
                                                                                                e->rotation(), e->tangentialPressure(), perspective, 0),e->modifiers());
    if (!eventIgnored){
        e->accept();
        return;
    }else{
        e->ignore();
    }


    if (mode() == KisTool::HOVER_MODE &&
            e->button() == Qt::LeftButton &&
            e->modifiers() == Qt::NoModifier &&
            !specialModifierActive()) {

        requestUpdateOutline(e->point);

        if (currentNode() && currentNode()->inherits("KisShapeLayer")) {
            KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
            canvas2->view()->showFloatingMessage(i18n("Can't paint on vector layer."), koIcon("draw-brush"));
        }

        if (nodePaintAbility() != PAINT)
            return;

        if (!nodeEditable()) {
            return;
        }

        setMode(KisTool::PAINT_MODE);

        KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
        if (canvas2)
            canvas2->view()->disableControls();


        const KisCoordinatesConverter *converter = static_cast<KisCanvas2*>(canvas())->coordinatesConverter();
        currentPaintOpPreset()->settings()->setCanvasRotation(converter->rotationAngle());
        currentPaintOpPreset()->settings()->setCanvasMirroring(converter->xAxisMirrored(),
                                                               converter->yAxisMirrored());
        initStroke(e);

        e->accept();
    }
    else {
        KisToolPaint::mousePressEvent(e);
    }
}

void KisToolFreehand::mouseMoveEvent(KoPointerEvent *e)
{
    /**
     * Update outline
     */
    if (mode() == KisTool::PAINT_MODE) {
        requestUpdateOutline(e->point);

        /**
         * Actual painting
         */
        doStroke(e);
    } else {
        KisToolPaint::mouseMoveEvent(e);
    }
}

void KisToolFreehand::mouseReleaseEvent(KoPointerEvent* e)
{
    if (mode() == KisTool::PAINT_MODE &&
            e->button() == Qt::LeftButton) {
        endStroke();

        if (m_assistant) {
            static_cast<KisCanvas2*>(canvas())->view()->paintingAssistantManager()->endStroke();
        }

        notifyModified();
        KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2 *>(canvas());
        if (canvas2) {
            canvas2->view()->enableControls();
        }

        setMode(KisTool::HOVER_MODE);
        e->accept();
    }
    else {
        KisToolPaint::mouseReleaseEvent(e);
        requestUpdateOutline(e->point);
    }
}

void KisToolFreehand::keyPressEvent(QKeyEvent *event)
{
    if (mode() != KisTool::PAINT_MODE) {
        KisToolPaint::keyPressEvent(event);
        requestUpdateOutline(m_outlineDocPoint);
        return;
    }

    event->accept();
}

void KisToolFreehand::keyReleaseEvent(QKeyEvent* event)
{
    if (mode() != KisTool::PAINT_MODE) {
        KisToolPaint::keyReleaseEvent(event);
        requestUpdateOutline(m_outlineDocPoint);
        return;
    }

    event->accept();
}

bool KisToolFreehand::isGestureSupported() const
{
    return true;
}

void KisToolFreehand::gesture(const QPointF &offsetInDocPixels, const QPointF &initialDocPoint)
{
    currentPaintOpPreset()->settings()->changePaintOpSize(offsetInDocPixels.x(), offsetInDocPixels.y());
    requestUpdateOutline(initialDocPoint);
}

bool KisToolFreehand::wantsAutoScroll() const
{
    return false;
}

void KisToolFreehand::setAssistant(bool assistant)
{
    m_assistant = assistant;
}

void KisToolFreehand::paint(QPainter& gc, const KoViewConverter &/*converter*/)
{
    paintToolOutline(&gc,pixelToView(m_currentOutline));
}

QPointF KisToolFreehand::adjustPosition(const QPointF& point, const QPointF& strokeBegin)
{
    if (m_assistant) {
        QPointF ap = static_cast<KisCanvas2*>(canvas())->view()->paintingAssistantManager()->adjustPosition(point, strokeBegin);
        return (1.0 - m_magnetism) * point + m_magnetism * ap;
    }
    return point;
}

qreal KisToolFreehand::calculatePerspective(const QPointF &documentPoint)
{
    qreal perspective = 1.0;
    foreach (const KisAbstractPerspectiveGrid* grid, static_cast<KisCanvas2*>(canvas())->view()->resourceProvider()->perspectiveGrids()) {
        if (grid->contains(documentPoint)) {
            perspective = grid->distance(documentPoint);
            break;
        }
    }
    return perspective;
}

void KisToolFreehand::showOutlineTemporary()
{
    m_explicitShowOutline = true;
    m_outlineTimer.start(HIDE_OUTLINE_TIMEOUT);
    requestUpdateOutline(m_outlineDocPoint);
}

void KisToolFreehand::hideOutline()
{
    m_explicitShowOutline = false;
    requestUpdateOutline(m_outlineDocPoint);
}

QPainterPath KisToolFreehand::getOutlinePath(const QPointF &documentPos,
                                          KisPaintOpSettings::OutlineMode outlineMode)
{
    qreal scale = 1.0;
    qreal rotation = 0;

    if (mode() == KisTool::HOVER_MODE) {
        rotation += static_cast<KisCanvas2*>(canvas())->rotationAngle() * M_PI / 180.0;
    }

    const KisPaintOp *paintOp = m_helper->currentPaintOp();
    if (paintOp){
        scale = paintOp->currentScale();
        rotation = paintOp->currentRotation();
    }

    QPointF imagePos = currentImage()->documentToPixel(documentPos);
    QPainterPath path = currentPaintOpPreset()->settings()->
            brushOutline(imagePos, outlineMode, scale, rotation);

    return path;
}

#include "kis_tool_freehand.moc"

