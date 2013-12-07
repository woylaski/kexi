/*
 *  Copyright (c) 2011 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#include "kis_tool_multihand.h"

#include <QTransform>

#include <QPushButton>
#include <QComboBox>
#include <QFormLayout>
#include <QStackedWidget>
#include <kis_slider_spin_box.h>

#include "kis_canvas2.h"
#include "kis_cursor.h"

#include "kis_tool_multihand_helper.h"

static const int MAXIMUM_BRUSHES = 50;

#include <QtGlobal>
#ifdef Q_OS_WIN
// quoting DRAND48(3) man-page:
// These functions are declared obsolete by  SVID  3,
// which  states  that rand(3) should be used instead.
#define drand48() (static_cast<double>(qrand()) / static_cast<double>(RAND_MAX))
#endif


KisToolMultihand::KisToolMultihand(KoCanvasBase *canvas)
    : KisToolBrush(canvas),
      m_transformMode(SYMMETRY),
      m_angle(0),
      m_handsCount(6),
      m_mirrorVertically(true),
      m_mirrorHorizontally(true),
      m_showAxis(false),
      m_translateRadius(100),
      m_setupAxisFlag(false)
{
    m_helper =
        new KisToolMultihandHelper(paintingInformationBuilder(),
                                   recordingAdapter());
    resetHelper(m_helper);

    m_axisPoint = QPointF(0.5 * image()->width(), 0.5 * image()->height());
}

KisToolMultihand::~KisToolMultihand()
{
}

void KisToolMultihand::beginPrimaryAction(KoPointerEvent *event)
{
    if(m_setupAxisFlag) {
        setMode(KisTool::OTHER);
        m_axisPoint = convertToPixelCoord(event->point);
        requestUpdateOutline(event->point);
        updateCanvas();
    }
    else {
        initTransformations();
        KisToolFreehand::beginPrimaryAction(event);
    }
}

void KisToolMultihand::continuePrimaryAction(KoPointerEvent *event)
{
    if(mode() == KisTool::OTHER) {
        m_axisPoint = convertToPixelCoord(event->point);
        requestUpdateOutline(event->point);
        updateCanvas();
    }
    else {
        KisToolFreehand::continuePrimaryAction(event);
    }
}

void KisToolMultihand::endPrimaryAction(KoPointerEvent *event)
{
    if(mode() == KisTool::OTHER) {
        setMode(KisTool::HOVER_MODE);
        requestUpdateOutline(event->point);
        finishAxisSetup();
    }
    else {
        KisToolFreehand::endPrimaryAction(event);
    }
}

void KisToolMultihand::paint(QPainter& gc, const KoViewConverter &converter)
{
    if(m_setupAxisFlag) {
        int diagonal = (currentImage()->height() + currentImage()->width());

        QPainterPath path;
        path.moveTo(m_axisPoint.x()-diagonal*cos(m_angle), m_axisPoint.y()-diagonal*sin(m_angle));
        path.lineTo(m_axisPoint.x()+diagonal*cos(m_angle), m_axisPoint.y()+diagonal*sin(m_angle));
        path.moveTo(m_axisPoint.x()-diagonal*cos(m_angle+M_PI_2), m_axisPoint.y()-diagonal*sin(m_angle+M_PI_2));
        path.lineTo(m_axisPoint.x()+diagonal*cos(m_angle+M_PI_2), m_axisPoint.y()+diagonal*sin(m_angle+M_PI_2));
        paintToolOutline(&gc, pixelToView(path));
    }
    else {
        KisToolFreehand::paint(gc, converter);
        if(m_showAxis){
            int diagonal = (currentImage()->height() + currentImage()->width());

            QPainterPath path;
            path.moveTo(m_axisPoint.x()-diagonal*cos(m_angle), m_axisPoint.y()-diagonal*sin(m_angle));
            path.lineTo(m_axisPoint.x()+diagonal*cos(m_angle), m_axisPoint.y()+diagonal*sin(m_angle));
            path.moveTo(m_axisPoint.x()-diagonal*cos(m_angle+M_PI_2), m_axisPoint.y()-diagonal*sin(m_angle+M_PI_2));
            path.lineTo(m_axisPoint.x()+diagonal*cos(m_angle+M_PI_2), m_axisPoint.y()+diagonal*sin(m_angle+M_PI_2));
            paintToolOutline(&gc, pixelToView(path));
        }
    }
}

void KisToolMultihand::initTransformations()
{
    QVector<QTransform> transformations;
    QTransform m;

    if(m_transformMode == SYMMETRY) {
        qreal angle = 0;
        qreal angleStep = (2 * M_PI) / m_handsCount;

        for(int i = 0; i < m_handsCount; i++) {
            m.translate(m_axisPoint.x(), m_axisPoint.y());
            m.rotateRadians(angle);
            m.translate(-m_axisPoint.x(), -m_axisPoint.y());

            transformations << m;
            m.reset();
            angle += angleStep;
        }
    }
    else if(m_transformMode == MIRROR) {
        transformations << m;

        if (m_mirrorHorizontally) {
            m.translate(m_axisPoint.x(),m_axisPoint.y());
            m.rotateRadians(m_angle);
            m.scale(-1,1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axisPoint.x(), -m_axisPoint.y());
            transformations << m;
            m.reset();
        }

        if (m_mirrorVertically) {
            m.translate(m_axisPoint.x(),m_axisPoint.y());
            m.rotateRadians(m_angle);
            m.scale(1,-1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axisPoint.x(), -m_axisPoint.y());
            transformations << m;
            m.reset();
        }

        if (m_mirrorVertically && m_mirrorHorizontally){
            m.translate(m_axisPoint.x(),m_axisPoint.y());
            m.rotateRadians(m_angle);
            m.scale(-1,-1);
            m.rotateRadians(-m_angle);
            m.translate(-m_axisPoint.x(), -m_axisPoint.y());
            transformations << m;
            m.reset();
        }
    }
    else /* if(m_transformationNode == TRANSLATE) */ {
        /**
         * TODO: currently, the seed is the same for all the
         * strokes
         */
#ifdef Q_WS_WIN
        srand(0);
#else
        srand48(0);
#endif

        for (int i = 0; i < m_handsCount; i++){
            qreal angle = drand48() * M_PI * 2;
            qreal length = drand48();

            // convert the Polar coordinates to Cartesian coordinates
            qreal nx = (m_translateRadius * cos(angle) * length);
            qreal ny = (m_translateRadius * sin(angle) * length);

            m.translate(m_axisPoint.x(),m_axisPoint.y());
            m.rotateRadians(m_angle);
            m.translate(nx,ny);
            m.rotateRadians(-m_angle);
            m.translate(-m_axisPoint.x(), -m_axisPoint.y());

            transformations << m;
            m.reset();
        }
    }

    m_helper->setupTransformations(transformations);
}

QWidget* KisToolMultihand::createOptionWidget()
{
    QWidget *widget = KisToolBrush::createOptionWidget();

    m_axisChCkBox = new QCheckBox(i18n("Show Axis"));
    m_axisChCkBox->setChecked(m_showAxis);
    connect(m_axisChCkBox,SIGNAL(toggled(bool)),this, SLOT(slotSetAxisVisable(bool)));

    m_axisPointBtn = new QPushButton(i18n("Axis point"), widget);
    m_axisPointBtn->setCheckable(true);
    connect(m_axisPointBtn, SIGNAL(clicked(bool)),this, SLOT(activateAxisPointModeSetup()));
    addOptionWidgetOption(m_axisPointBtn, m_axisChCkBox);

    m_axisAngleSlider = new KisDoubleSliderSpinBox(widget);
    m_axisAngleSlider->setToolTip(i18n("Set axis angle (degrees)"));
    m_axisAngleSlider->setRange(0.0, 90.0,1);
    m_axisAngleSlider->setValue(0.0);
    m_axisAngleSlider->setEnabled(true);
    connect(m_axisAngleSlider, SIGNAL(valueChanged(qreal)),this, SLOT(slotSetAxisAngle(qreal)));
    addOptionWidgetOption(m_axisAngleSlider, new QLabel(i18n("Axis Angle:")));

    m_transformModesComboBox = new QComboBox(widget);
    m_transformModesComboBox->addItem(i18n("Symmetry"),int(SYMMETRY));
    m_transformModesComboBox->addItem(i18n("Mirror"),int(MIRROR));
    m_transformModesComboBox->addItem(i18n("Translate"),int(TRANSLATE));
    m_transformModesComboBox->setCurrentIndex(m_transformModesComboBox->findData(int(m_transformMode)));
    connect(m_transformModesComboBox,SIGNAL(currentIndexChanged(int)),SLOT(slotSetTransformMode(int)));
    addOptionWidgetOption(m_transformModesComboBox);

    m_handsCountSlider = new KisSliderSpinBox(widget);
    m_handsCountSlider->setToolTip(i18n("Brush count"));
    m_handsCountSlider->setRange(1, MAXIMUM_BRUSHES);
    m_handsCountSlider->setValue(m_handsCount);
    m_handsCountSlider->setEnabled(true);
    connect(m_handsCountSlider, SIGNAL(valueChanged(int)),this, SLOT(slotSetHandsCount(int)));
    addOptionWidgetOption(m_handsCountSlider);

    m_modeCustomOption = new QStackedWidget(widget);

    QWidget * symmetryWidget = new QWidget(m_modeCustomOption);
    m_modeCustomOption->addWidget(symmetryWidget);

    QWidget * mirrorWidget = new QWidget(m_modeCustomOption);
    m_mirrorHorizontallyChCkBox = new QCheckBox(i18n("Horizontally"));
    m_mirrorHorizontallyChCkBox->setChecked(m_mirrorHorizontally);
    m_mirrorVerticallyChCkBox = new QCheckBox(i18n("Vertically"));
    m_mirrorVerticallyChCkBox->setChecked(m_mirrorVertically);
    connect(m_mirrorHorizontallyChCkBox,SIGNAL(toggled(bool)),this, SLOT(slotSetMirrorHorizontally(bool)));
    connect(m_mirrorVerticallyChCkBox,SIGNAL(toggled(bool)),this, SLOT(slotSetMirrorVertically(bool)));

    QGridLayout * mirrorLayout = new QGridLayout(mirrorWidget);
    mirrorLayout->addWidget(m_mirrorHorizontallyChCkBox,0,0);
    mirrorLayout->addWidget(m_mirrorVerticallyChCkBox,0,1);
    mirrorWidget->setLayout(mirrorLayout);
    m_modeCustomOption->addWidget(mirrorWidget);

    QWidget * translateWidget = new QWidget(m_modeCustomOption);
    m_translateRadiusSlider = new KisSliderSpinBox(translateWidget);
    m_translateRadiusSlider->setRange(0, 200);
    m_translateRadiusSlider->setValue(m_translateRadius);
    m_translateRadiusSlider->setSuffix(" px");
    connect(m_translateRadiusSlider,SIGNAL(valueChanged(int)),this,SLOT(slotSetTranslateRadius(int)));

    QFormLayout *radiusLayout = new QFormLayout(translateWidget);
    radiusLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    radiusLayout->addRow(i18n("Radius"), m_translateRadiusSlider);
    translateWidget->setLayout(radiusLayout);

    m_modeCustomOption->addWidget(translateWidget);
    m_modeCustomOption->setCurrentIndex(m_transformModesComboBox->currentIndex());

    addOptionWidgetOption(m_modeCustomOption);

    return widget;
}

void KisToolMultihand::activateAxisPointModeSetup()
{
    if (m_axisPointBtn->isChecked()){
        m_setupAxisFlag = true;
        useCursor(KisCursor::crossCursor());
        updateCanvas();
    } else {
        finishAxisSetup();
    }
}

void KisToolMultihand::finishAxisSetup()
{
    m_setupAxisFlag = false;
    m_axisPointBtn->setChecked(false);
    resetCursorStyle();
    updateCanvas();
}

void KisToolMultihand::updateCanvas()
{
    KisCanvas2 *kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    Q_ASSERT(kisCanvas);
    kisCanvas->updateCanvas();
}

void KisToolMultihand::slotSetHandsCount(int count)
{
    m_handsCount = count;
}

void KisToolMultihand::slotSetAxisAngle(qreal angle)
{
    //negative so axis rotates counter clockwise
    m_angle = -angle*M_PI/180;
    updateCanvas();
}

void KisToolMultihand::slotSetTransformMode(int index)
{
    m_transformMode = enumTransforModes(m_transformModesComboBox->itemData(index).toInt());
    m_modeCustomOption->setCurrentIndex(index);
    m_handsCountSlider->setVisible(m_transformMode != MIRROR);
}

void KisToolMultihand::slotSetAxisVisable(bool vis)
{
    m_showAxis = vis;
    updateCanvas();
}


void KisToolMultihand::slotSetMirrorVertically(bool mirror)
{
    m_mirrorVertically = mirror;
}

void KisToolMultihand::slotSetMirrorHorizontally(bool mirror)
{
    m_mirrorHorizontally = mirror;
}

void KisToolMultihand::slotSetTranslateRadius(int radius)
{
    m_translateRadius = radius;
}

