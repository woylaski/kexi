/*
 *  kis_tool_select_contiguous - part of Krayon^WKrita
 *
 *  Copyright (c) 1999 Michael Koch <koch@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2012 José Luis Vergara <pentalis@gmail.com>
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

#include "kis_tool_select_contiguous.h"
#include <QPainter>
#include <QLayout>
#include <QLabel>
#include <QApplication>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kis_debug.h>
#include <klocale.h>
#include <kcolorbutton.h>

#include "KoPointerEvent.h"
#include "KoViewConverter.h"

#include "kis_cursor.h"
#include "kis_selection_manager.h"
#include "kis_image.h"
#include "canvas/kis_canvas2.h"
#include "kis_layer.h"
#include "kis_selection_options.h"
#include "kis_paint_device.h"
#include "kis_fill_painter.h"
#include "kis_pixel_selection.h"
#include "kis_selection_tool_helper.h"
#include "kis_slider_spin_box.h"

KisToolSelectContiguous::KisToolSelectContiguous(KoCanvasBase *canvas)
        : KisToolSelectBase(canvas,
                            KisCursor::load("tool_contiguous_selection_cursor.png", 6, 6),
                            i18n("Contiguous Area Selection"))
{
    setObjectName("tool_select_contiguous");
    m_fuzziness = 20;
    m_sizemod = 0;
    m_feather = 0;
    m_limitToCurrentLayer = false;
}

KisToolSelectContiguous::~KisToolSelectContiguous()
{
}

void KisToolSelectContiguous::mousePressEvent(KoPointerEvent *event)
{
    if(PRESS_CONDITION(event, KisTool::HOVER_MODE,
                       Qt::LeftButton, Qt::NoModifier)) {

        if (!currentNode())
            return;
        KisPaintDeviceSP dev = currentNode()->projection();

        if (!dev || !currentNode()->visible())
            return;

        if (!selectionEditable()) {
            return;
        }

        QApplication::setOverrideCursor(KisCursor::waitCursor());

        QPoint pos = convertToIntPixelCoord(event);
        QRect rc = currentImage()->bounds();
        KisFillPainter fillpainter(dev);
        fillpainter.setHeight(rc.height());
        fillpainter.setWidth(rc.width());
        fillpainter.setFillThreshold(m_fuzziness);
        fillpainter.setSampleMerged(!m_limitToCurrentLayer);

        KisImageWSP image = currentImage();
        image->lock();
        fillpainter.setFeather(m_feather);
        fillpainter.setSizemod(m_sizemod);
        KisSelectionSP selection = fillpainter.createFloodSelection(pos.x(), pos.y(), image->projection());
        image->unlock();

        KisCanvas2 * kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
        if (!kisCanvas || !selection->pixelSelection()) {
            QApplication::restoreOverrideCursor();
            return;
        }

        selection->pixelSelection()->invalidateOutlineCache();
        KisSelectionToolHelper helper(kisCanvas, i18n("Contiguous Area Selection"));
        helper.selectPixelSelection(selection->pixelSelection(), selectionAction());

        QApplication::restoreOverrideCursor();
    }
    else {
        KisTool::mousePressEvent(event);
    }
}

void KisToolSelectContiguous::paint(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(painter);
    Q_UNUSED(converter);
}

void KisToolSelectContiguous::slotSetFuzziness(int fuzziness)
{
    m_fuzziness = fuzziness;
}

void KisToolSelectContiguous::slotSetSizemod(int sizemod)
{
    m_sizemod = sizemod;
}

void KisToolSelectContiguous::slotSetFeather(int feather)
{
    m_feather = feather;
}

QWidget* KisToolSelectContiguous::createOptionWidget()
{
    KisToolSelectBase::createOptionWidget();
    KisSelectionOptions *selectionWidget = selectionOptionWidget();

    selectionWidget->disableAntiAliasSelectionOption();
    selectionWidget->disableSelectionModeOption();

    QVBoxLayout * l = dynamic_cast<QVBoxLayout*>(selectionWidget->layout());
    Q_ASSERT(l);
    if (l) {
        QHBoxLayout * hbox = new QHBoxLayout();
        Q_CHECK_PTR(hbox);
        l->insertLayout(1, hbox);

        QLabel * lbl = new QLabel(i18n("Fuzziness: "), selectionWidget);
        hbox->addWidget(lbl);

        KisSliderSpinBox *input = new KisSliderSpinBox(selectionWidget);
        Q_CHECK_PTR(input);
        input->setObjectName("fuzziness");
        input->setRange(0, 200);
        input->setSingleStep(10);
        input->setValue(20);
        hbox->addWidget(input);
        
        hbox = new QHBoxLayout();
        Q_CHECK_PTR(hbox);
        l->insertLayout(2, hbox);
        
        lbl = new QLabel(i18n("Grow/shrink selection: "), selectionWidget);
        hbox->addWidget(lbl);
        
        KisSliderSpinBox *sizemod = new KisSliderSpinBox(selectionWidget);
        Q_CHECK_PTR(sizemod);
        sizemod->setObjectName("sizemod");
        sizemod->setRange(-40, 40);
        sizemod->setSingleStep(1);
        sizemod->setValue(0);
        hbox->addWidget(sizemod);
        
        hbox = new QHBoxLayout();
        Q_CHECK_PTR(hbox);
        l->insertLayout(3, hbox);
        
        hbox->addWidget(new QLabel(i18n("Feathering radius: "), selectionWidget));
        
        KisSliderSpinBox *feather = new KisSliderSpinBox(selectionWidget);
        Q_CHECK_PTR(feather);
        feather->setObjectName("feathering");
        feather->setRange(0, 40);
        feather->setSingleStep(1);
        feather->setValue(0);
        hbox->addWidget(feather);
        
        connect (input  , SIGNAL(valueChanged(int)), this, SLOT(slotSetFuzziness(int) ));
        connect (sizemod, SIGNAL(valueChanged(int)), this, SLOT(slotSetSizemod(int)   ));
        connect (feather, SIGNAL(valueChanged(int)), this, SLOT(slotSetFeather(int)   ));

        QCheckBox* limitToCurrentLayer = new QCheckBox(i18n("Limit to current layer"), selectionWidget);
        l->insertWidget(4, limitToCurrentLayer);
        limitToCurrentLayer->setChecked(m_limitToCurrentLayer);
        connect (limitToCurrentLayer, SIGNAL(stateChanged(int)), this, SLOT(slotLimitToCurrentLayer(int)));

    }
    return selectionWidget;
}

void KisToolSelectContiguous::slotLimitToCurrentLayer(int state)
{
    if (state == Qt::PartiallyChecked)
        return;
    m_limitToCurrentLayer = (state == Qt::Checked);
}

#include "kis_tool_select_contiguous.moc"
