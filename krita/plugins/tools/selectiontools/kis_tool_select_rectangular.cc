/*
 *  kis_tool_select_rectangular.cc -- part of Krita
 *
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *                2001 John Califf <jcaliff@compuzone.net>
 *                2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "kis_tool_select_rectangular.h"

#include "kis_painter.h"
#include "kis_paintop_registry.h"
#include "kis_selection_options.h"
#include "kis_canvas2.h"
#include "kis_pixel_selection.h"
#include "kis_selection_tool_helper.h"
#include "kis_shape_tool_helper.h"

#include "kis_system_locker.h"
#include "kis_view2.h"
#include "kis_selection_manager.h"


KisToolSelectRectangular::KisToolSelectRectangular(KoCanvasBase * canvas)
    : KisToolRectangleBase(canvas, KisToolRectangleBase::SELECT,
                           KisCursor::load("tool_rectangular_selection_cursor.png", 6, 6)),
      m_widgetHelper(i18n("Rectangular Selection"))
{
    connect(&m_widgetHelper, SIGNAL(selectionActionChanged(int)), this, SLOT(setSelectionAction(int)));
}

SelectionAction KisToolSelectRectangular::selectionAction() const
{
    return m_selectionAction;
}

void KisToolSelectRectangular::setSelectionAction(int newSelectionAction)
{
    if(newSelectionAction >= SELECTION_REPLACE && newSelectionAction <= SELECTION_INTERSECT && m_selectionAction != newSelectionAction)
    {
        if(m_widgetHelper.optionWidget())
        {
            m_widgetHelper.slotSetAction(newSelectionAction);
        }
        m_selectionAction = (SelectionAction)newSelectionAction;
        emit selectionActionChanged();
    }
}

QWidget* KisToolSelectRectangular::createOptionWidget()
{
    KisCanvas2* canvas = dynamic_cast<KisCanvas2*>(this->canvas());
    Q_ASSERT(canvas);

    m_widgetHelper.createOptionWidget(canvas, this->toolId());
    m_widgetHelper.optionWidget()->disableAntiAliasSelectionOption();
    return m_widgetHelper.optionWidget();
}

void KisToolSelectRectangular::keyPressEvent(QKeyEvent *event)
{
    if (!m_widgetHelper.processKeyPressEvent(event)) {
        KisTool::keyPressEvent(event);
    }
}

void KisToolSelectRectangular::finishRect(const QRectF& rect)
{
    KisCanvas2 * kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    if (!kisCanvas)
        return;

    KisSelectionToolHelper helper(kisCanvas, i18n("Rectangular Selection"));

    QRect rc(rect.normalized().toRect());
    helper.cropRectIfNeeded(&rc);

    // If the user just clicks on the canvas deselect
    if (rc.isEmpty()) {
        // Queueing this action to ensure we avoid a race condition when unlocking the node system
        QTimer::singleShot(0, kisCanvas->view()->selectionManager(), SLOT(deselect()));
        return;
    }

    if (m_widgetHelper.selectionMode() == PIXEL_SELECTION) {
        if (rc.isValid()) {
            KisPixelSelectionSP tmpSel = KisPixelSelectionSP(new KisPixelSelection());
            tmpSel->select(rc);

            QPainterPath cache;
            cache.addRect(rc);
            tmpSel->setOutlineCache(cache);

            helper.selectPixelSelection(tmpSel, m_widgetHelper.selectionAction());
        }
    } else {
        QRectF documentRect = convertToPt(rc);
        helper.addSelectionShape(KisShapeToolHelper::createRectangleShape(documentRect));
    }
}
