/*
 *  Copyright (c) 2004,2007,2009 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
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


#include "kis_auto_brush_widget.h"

#include <math.h>
#include <kis_debug.h>
#include <QSpinBox>
#include <QToolButton>
#include <QImage>
#include <QComboBox>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

#include <kis_fixed_paint_device.h>
#include <kis_mask_generator.h>
#include <kis_slider_spin_box.h>

#define showSlider(input, step) input->setRange(input->minimum(), input->maximum(), step)
#include <kis_cubic_curve.h>

KisAutoBrushWidget::KisAutoBrushWidget(QWidget *parent, const char* name)
    : KisWdgAutobrush(parent, name)
    , m_autoBrush(0)
    , m_linkFade(false)
{

    connect(aspectButton, SIGNAL(keepAspectRatioChanged(bool)), this, SLOT(linkFadeToggled(bool)));
    aspectButton->setKeepAspectRatio(m_linkFade);

    connect((QObject*)comboBoxShape, SIGNAL(activated(int)), this, SLOT(paramChanged()));

    inputRadius->setRange(0.0, 10.0, 2);
    inputRadius->addMultiplier(10.0);
    inputRadius->addMultiplier(100.0);
    inputRadius->setExponentRatio(3.0);
    inputRadius->setValue(5.0);
    connect(inputRadius, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxRadiusChanged(qreal)));

    inputRatio->setRange(0.0, 1.0, 2);
    inputRatio->setSingleStep(0.1);
    inputRatio->setValue(1.0);
    connect(inputRatio, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxRatioChanged(qreal)));

    inputHFade->setRange(0.0, 1.0, 2);
    inputHFade->setSingleStep(0.1);
    inputHFade->setValue(0.5);
    connect(inputHFade, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxHorizontalChanged(qreal)));

    inputVFade->setRange(0.0, 1.0, 2);
    inputVFade->setSingleStep(0.1);
    inputVFade->setValue(0.5);
    connect(inputVFade, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxVerticalChanged(qreal)));

    inputSpikes->setRange(2, 20);
    inputSpikes->setValue(2);
    connect(inputSpikes, SIGNAL(valueChanged(int)), this, SLOT(spinBoxSpikesChanged(int)));

    inputRandomness->setRange(0, 100);
    inputRandomness->setValue(0);
    connect(inputRandomness, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxRandomnessChanged(qreal)));

    inputAngle->setRange(0, 360);
    inputAngle->setSuffix(QChar(Qt::Key_degree));
    inputAngle->setValue(0);
    connect(inputAngle, SIGNAL(valueChanged(int)), this, SLOT(spinBoxAngleChanged(int)));

    inputSpacing->setRange(0.0, 10.0, 2);
    inputSpacing->setSingleStep(0.1);
    inputSpacing->setValue(0.1);
    connect(inputSpacing, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxSpacingChanged(qreal)));

    density->setRange(0, 100, 0);
    density->setSingleStep(1);
    density->setValue(100);
    density->setSuffix("%");
    connect(density, SIGNAL(valueChanged(qreal)), this, SLOT(spinBoxDensityChanged(qreal)));

    KisCubicCurve topLeftBottomRightLinearCurve;
    topLeftBottomRightLinearCurve.setPoint(0, QPointF(0.0, 1.0));
    topLeftBottomRightLinearCurve.setPoint(1, QPointF(1.0, 0.0));
    softnessCurve->setCurve(topLeftBottomRightLinearCurve);
    connect(softnessCurve, SIGNAL(modified()), this, SLOT(paramChanged()));

    m_brush = QImage(1, 1, QImage::Format_RGB32);

    connect(brushPreview, SIGNAL(clicked()), SLOT(paramChanged()));

    QList<KoID> ids = KisMaskGenerator::maskGeneratorIds();
    for (int i = 0; i < ids.size(); i++) {
        comboBoxMaskType->insertItem(i, ids[i].name());
    }

    connect(comboBoxMaskType, SIGNAL(activated(int)), SLOT(paramChanged()));
    connect(comboBoxMaskType, SIGNAL(currentIndexChanged(int)), SLOT(setStackedWidget(int)));

    brushPreview->setIconSize(QSize(100, 100));

    paramChanged();

}

void KisAutoBrushWidget::resizeEvent(QResizeEvent *)
{
    brushPreview->setMinimumHeight(brushPreview->width()); // dirty hack !
    brushPreview->setMaximumHeight(brushPreview->width()); // dirty hack !
}

void KisAutoBrushWidget::activate()
{
    paramChanged();
}

void KisAutoBrushWidget::paramChanged()
{
    KisMaskGenerator* kas;

    if (comboBoxMaskType->currentIndex() == 2) { // gaussian brush
        if (comboBoxShape->currentIndex() == 0) {
            kas = new KisGaussCircleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value());
        }
        else {
            kas = new KisGaussRectangleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value());
        }
    }
    else if (comboBoxMaskType->currentIndex() == 1) { // soft brush
        if (comboBoxShape->currentIndex() == 0) {
            kas = new KisCurveCircleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value(), softnessCurve->curve());
        }
        else {
            kas = new KisCurveRectangleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value(), softnessCurve->curve());
        }
    }
    else {// default == 0 or any other
        if (comboBoxShape->currentIndex() == 0) { // use index compare instead of comparing a translatable string
            kas = new KisCircleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value());
        }
        else {
            kas = new KisRectangleMaskGenerator(inputRadius->value(),  inputRatio->value(), inputHFade->value(), inputVFade->value(), inputSpikes->value());
        }
    }
    Q_CHECK_PTR(kas);

    m_autoBrush = new KisAutoBrush(kas, inputAngle->value() / 180.0 * M_PI, inputRandomness->value() / 100.0, density->value() / 100.0);
    m_autoBrush->setSpacing(inputSpacing->value());
    m_brush = m_autoBrush->image();

    QImage pi(m_brush);
    double coeff = 1.0;
    int bPw = brushPreview->width() - 3;
    if (pi.width() > bPw) {
        coeff =  bPw / (double)pi.width();
    }
    int bPh = brushPreview->height() - 3;
    if (pi.height() > coeff * bPh) {
        coeff = bPh / (double)pi.height();
    }
    if (coeff < 1.0) {
        pi = pi.scaled((int)(coeff * pi.width()) , (int)(coeff * pi.height()),  Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    QPixmap p = QPixmap::fromImage(pi);
    brushPreview->setIcon(QIcon(p));

    emit sigBrushChanged();
}

void KisAutoBrushWidget::setStackedWidget(int index)
{
    if (index == 1) {
        stackedWidget->setCurrentIndex(1);
    }
    else {
        stackedWidget->setCurrentIndex(0);
    }
}

void KisAutoBrushWidget::spinBoxHorizontalChanged(qreal a)
{
    if (m_linkFade) {
        inputVFade->blockSignals(true);
        inputVFade->setValue(a);
        inputVFade->blockSignals(false);
    }

    inputHFade->blockSignals(true);
    inputHFade->setValue(a);
    inputHFade->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxVerticalChanged(qreal a)
{
    if (m_linkFade) {
        inputHFade->blockSignals(true);
        inputHFade->setValue(a);
        inputHFade->blockSignals(false);
    }

    inputVFade->blockSignals(true);
    inputVFade->setValue(a);
    inputVFade->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxRatioChanged(qreal a)
{
    inputRatio->blockSignals(true);
    inputRatio->setValue(a);
    inputRatio->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxRandomnessChanged(qreal a)
{
    inputRandomness->blockSignals(true);
    inputRandomness->setValue(a);
    inputRandomness->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxRadiusChanged(qreal a)
{
    inputRadius->blockSignals(true);
    inputRadius->setValue(a);
    inputRadius->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxSpikesChanged(int a)
{
    inputSpikes->blockSignals(true);
    inputSpikes->setValue(a);
    inputSpikes->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxAngleChanged(int a)
{
    inputAngle->blockSignals(true);
    inputAngle->setValue(a);
    inputAngle->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxSpacingChanged(qreal a)
{
    inputSpacing->blockSignals(true);
    inputSpacing->setValue(a);
    inputSpacing->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::spinBoxDensityChanged(qreal a)
{
    density->blockSignals(true);
    density->setValue(a);
    density->blockSignals(false);
    paramChanged();
}

void KisAutoBrushWidget::linkFadeToggled(bool b)
{
    m_linkFade = b;

    if (m_linkFade)
        inputVFade->setValue(inputHFade->value());
}

KisBrushSP KisAutoBrushWidget::brush()
{
    return m_autoBrush;
}

void KisAutoBrushWidget::setBrush(KisBrushSP brush)
{
    m_autoBrush = brush;
    m_brush = brush->image();
    // XXX: lock, set and unlock the widgets.
    KisAutoBrush* aBrush = dynamic_cast<KisAutoBrush*>(brush.data());
    if (aBrush->maskGenerator()->type() == KisMaskGenerator::CIRCLE) {
        comboBoxShape->setCurrentIndex(0);
    }
    else if (aBrush->maskGenerator()->type() == KisMaskGenerator::RECTANGLE) {
        comboBoxShape->setCurrentIndex(1);
    }
    else {
        comboBoxShape->setCurrentIndex(2);
    }

    comboBoxMaskType->setCurrentIndex(comboBoxMaskType->findText(aBrush->maskGenerator()->name()));


    inputRadius->setValue(aBrush->maskGenerator()->diameter());
    inputRatio->setValue(aBrush->maskGenerator()->ratio());

    inputVFade->blockSignals(true);
    inputHFade->blockSignals(true);
    inputHFade->setValue(aBrush->maskGenerator()->horizontalFade());
    inputVFade->setValue(aBrush->maskGenerator()->verticalFade());
    inputVFade->blockSignals(false);
    inputHFade->blockSignals(false);

    inputAngle->setValue(aBrush->angle() * 180 / M_PI);
    inputSpikes->setValue(aBrush->maskGenerator()->spikes());
    inputSpacing->setValue(aBrush->spacing());
    inputSpacing->setExponentRatio(3.0);
    inputRandomness->setValue(aBrush->randomness() * 100);
    density->setValue(aBrush->density() * 100);

    if (!aBrush->maskGenerator()->curveString().isEmpty()) {
        KisCubicCurve curve;
        curve.fromString(aBrush->maskGenerator()->curveString());
        softnessCurve->setCurve(curve);
    }
}


void KisAutoBrushWidget::setBrushSize(qreal dxPixels, qreal dyPixels)
{
    Q_UNUSED(dyPixels);

    qreal newWidth = inputRadius->value() + dxPixels;
    newWidth = qMax(newWidth, qreal(0.1));

    inputRadius->setValue(newWidth);
}

QSizeF KisAutoBrushWidget::brushSize() const
{
    return QSizeF(inputRadius->value(), inputRadius->value() * inputRatio->value());
}


#include "kis_auto_brush_widget.moc"
