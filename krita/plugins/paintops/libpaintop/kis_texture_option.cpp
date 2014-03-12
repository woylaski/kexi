/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2012
 * Copyright (C) Mohit Goyal <mohit.bits2011@gmail.com>, (C) 2014
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "kis_texture_option.h"

#include <QWidget>
#include <QString>
#include <QByteArray>
#include <QCheckBox>
#include <QBuffer>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QTransform>
#include <QPainter>
#include <QBoxLayout>

#include <klocale.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <kis_resource_server_provider.h>
#include <kis_pattern_chooser.h>
#include <kis_slider_spin_box.h>
#include <kis_multipliers_double_slider_spinbox.h>
#include <KoPattern.h>
#include <kis_paint_device.h>
#include <kis_fill_painter.h>
#include <kis_painter.h>
#include <kis_iterator_ng.h>
#include <kis_fixed_paint_device.h>
#include <kis_gradient_slider.h>
#include "kis_embedded_pattern_manager.h"
#include <time.h>

class KisTextureOptionWidget : public QWidget
{
public:

    KisTextureOptionWidget(QWidget *parent = 0)
        : QWidget(parent) {
        QFormLayout *formLayout = new QFormLayout(this);
        formLayout->setMargin(0);

        chooser = new KoPatternChooser(this);
        chooser->setGrayscalePreview(true);
        chooser->setMaximumHeight(250);
        chooser->setCurrentItem(0, 0);
        formLayout->addRow(chooser);

        scaleSlider = new KisMultipliersDoubleSliderSpinBox(this);
        scaleSlider->setRange(0.0, 2.0, 2);
        scaleSlider->setValue(1.0);
        scaleSlider->addMultiplier(0.1);
        scaleSlider->addMultiplier(2);
        scaleSlider->addMultiplier(10);

        formLayout->addRow(i18n("Scale:"), scaleSlider);


        QBoxLayout *offsetLayoutX = new QBoxLayout(QBoxLayout::LeftToRight,this);
        offsetSliderX = new KisSliderSpinBox(this);
        offsetSliderX->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        randomOffsetX = new QCheckBox(i18n("Random Offset"),this);
        offsetLayoutX->addWidget(offsetSliderX,1,0);
        offsetLayoutX->addWidget(randomOffsetX,0,0);
        formLayout->addRow(i18n("Horizontal Offset:"), offsetLayoutX);

        QBoxLayout *offsetLayoutY = new QBoxLayout(QBoxLayout::LeftToRight,this);
        offsetSliderY = new KisSliderSpinBox(this);
        offsetSliderY->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        randomOffsetY = new QCheckBox(i18n("Random Offset"),this);
        offsetLayoutY->addWidget(offsetSliderY,1,0);
        offsetLayoutY->addWidget(randomOffsetY,0,0);
        formLayout->addRow(i18n("Vertical Offset:"), offsetLayoutY);

        cmbTexturingMode = new QComboBox(this);
        cmbTexturingMode->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QStringList texturingModes;
        texturingModes << i18n("Multiply") << i18n("Subtract");
        cmbTexturingMode->addItems(texturingModes);
        formLayout->addRow(i18n("Texturing Mode:"), cmbTexturingMode);
        cmbTexturingMode->setCurrentIndex(KisTextureProperties::SUBTRACT);

        cmbCutoffPolicy = new QComboBox(this);
        cmbCutoffPolicy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QStringList cutOffPolicies;
        cutOffPolicies << i18n("Cut Off Disabled") << i18n("Cut Off Brush") << i18n("Cut Off Pattern");
        cmbCutoffPolicy->addItems(cutOffPolicies);
        formLayout->addRow(i18n("Cutoff Policy:"), cmbCutoffPolicy);

        cutoffSlider = new KisGradientSlider(this);
        cutoffSlider->setMinimumSize(256, 30);
        cutoffSlider->enableGamma(false);
        cutoffSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        cutoffSlider->setToolTip(i18n("When pattern texture values are outside the range specified"
                                      " by the slider, the cut-off policy will be applied."));
        formLayout->addRow(i18n("Cutoff:"), cutoffSlider);

        chkInvert = new QCheckBox(this);
        chkInvert->setChecked(false);
        formLayout->addRow(i18n("Invert Texture:"), chkInvert);

        setLayout(formLayout);
    }
    KoPatternChooser *chooser;
    KisMultipliersDoubleSliderSpinBox *scaleSlider;
    KisSliderSpinBox *offsetSliderX;
    QCheckBox *randomOffsetX;
    KisSliderSpinBox *offsetSliderY;
    QCheckBox *randomOffsetY;
    QComboBox *cmbTexturingMode;
    KisGradientSlider *cutoffSlider;
    QComboBox *cmbCutoffPolicy;
    QCheckBox *chkInvert;
};

KisTextureOption::KisTextureOption(QObject *)
    : KisPaintOpOption(i18n("Pattern"), KisPaintOpOption::textureCategory(), true)
{
    setChecked(false);
    m_optionWidget = new KisTextureOptionWidget;
    m_optionWidget->hide();
    setConfigurationPage(m_optionWidget);

    connect(m_optionWidget->chooser, SIGNAL(resourceSelected(KoResource*)), SLOT(resetGUI(KoResource*)));
    connect(m_optionWidget->chooser, SIGNAL(resourceSelected(KoResource*)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->scaleSlider, SIGNAL(valueChanged(qreal)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->offsetSliderX, SIGNAL(valueChanged(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->randomOffsetX, SIGNAL(toggled(bool)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->randomOffsetY, SIGNAL(toggled(bool)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->offsetSliderY, SIGNAL(valueChanged(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->cmbTexturingMode, SIGNAL(currentIndexChanged(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->cmbCutoffPolicy, SIGNAL(currentIndexChanged(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->cutoffSlider, SIGNAL(sigModifiedBlack(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->cutoffSlider, SIGNAL(sigModifiedWhite(int)), SIGNAL(sigSettingChanged()));
    connect(m_optionWidget->chkInvert, SIGNAL(toggled(bool)), SIGNAL(sigSettingChanged()));
    resetGUI(m_optionWidget->chooser->currentResource());
}

KisTextureOption::~KisTextureOption()
{
    delete m_optionWidget;
}

void KisTextureOption::writeOptionSetting(KisPropertiesConfiguration* setting) const
{
    if (!m_optionWidget->chooser->currentResource()) return;
    KoPattern *pattern = static_cast<KoPattern*>(m_optionWidget->chooser->currentResource());
    if (!pattern) return;

    qreal scale = m_optionWidget->scaleSlider->value();

    int offsetX = m_optionWidget->offsetSliderX->value();
    if (m_optionWidget ->randomOffsetX->isChecked()) {

        m_optionWidget -> offsetSliderX ->setEnabled(false);
        m_optionWidget -> offsetSliderX ->blockSignals(true);
        m_optionWidget -> offsetSliderX ->setValue(offsetX);
        m_optionWidget -> offsetSliderX ->blockSignals(false);
        srand(time(0));
    }
    else {
        m_optionWidget -> offsetSliderX ->setEnabled(true);
    }

    int offsetY = m_optionWidget->offsetSliderY->value();
    if (m_optionWidget ->randomOffsetY->isChecked()) {

        m_optionWidget -> offsetSliderY ->setEnabled(false);
        m_optionWidget -> offsetSliderY ->blockSignals(true);
        m_optionWidget -> offsetSliderY ->setValue(offsetY);
        m_optionWidget -> offsetSliderY ->blockSignals(false);
        srand(time(0));
    }
    else {
        m_optionWidget -> offsetSliderY ->setEnabled(true);
    }
    
    int texturingMode = m_optionWidget->cmbTexturingMode->currentIndex();
    bool invert = (m_optionWidget->chkInvert->checkState() == Qt::Checked);

    setting->setProperty("Texture/Pattern/Scale", scale);
    setting->setProperty("Texture/Pattern/OffsetX", offsetX);
    setting->setProperty("Texture/Pattern/OffsetY", offsetY);
    setting->setProperty("Texture/Pattern/TexturingMode", texturingMode);
    setting->setProperty("Texture/Pattern/CutoffLeft", m_optionWidget->cutoffSlider->black());
    setting->setProperty("Texture/Pattern/CutoffRight", m_optionWidget->cutoffSlider->white());
    setting->setProperty("Texture/Pattern/CutoffPolicy", m_optionWidget->cmbCutoffPolicy->currentIndex());
    setting->setProperty("Texture/Pattern/Invert", invert);
    setting->setProperty("Texture/Pattern/Enabled", isChecked());
    setting->setProperty("Texture/Pattern/MaximumOffsetX",m_optionWidget -> offsetSliderX ->maximum());
    setting->setProperty("Texture/Pattern/MaximumOffsetY",m_optionWidget -> offsetSliderY ->maximum());
    setting->setProperty("Texture/Pattern/isRandomOffsetX",m_optionWidget ->randomOffsetX ->isChecked());
    setting->setProperty("Texture/Pattern/isRandomOffsetY",m_optionWidget ->randomOffsetY ->isChecked());

    KisEmbeddedPatternManager::saveEmbeddedPattern(setting, pattern);
}



void KisTextureOption::readOptionSetting(const KisPropertiesConfiguration* setting)
{
    KoPattern *pattern = KisEmbeddedPatternManager::loadEmbeddedPattern(setting);

    if (!pattern) {
        pattern = static_cast<KoPattern*>(m_optionWidget->chooser->currentResource());
    }

    m_optionWidget->chooser->setCurrentPattern(pattern);

    m_optionWidget->scaleSlider->setValue(setting->getDouble("Texture/Pattern/Scale", 1.0));
    m_optionWidget->offsetSliderX->setValue(setting->getInt("Texture/Pattern/OffsetX"));
    m_optionWidget->offsetSliderY->setValue(setting->getInt("Texture/Pattern/OffsetY"));
    m_optionWidget->cmbTexturingMode->setCurrentIndex(setting->getInt("Texture/Pattern/TexturingMode", KisTextureProperties::MULTIPLY));
    m_optionWidget->cmbCutoffPolicy->setCurrentIndex(setting->getInt("Texture/Pattern/CutoffPolicy"));
    m_optionWidget->cutoffSlider->slotModifyBlack(setting->getInt("Texture/Pattern/CutoffLeft", 0));
    m_optionWidget->cutoffSlider->slotModifyWhite(setting->getInt("Texture/Pattern/CutoffRight", 255));
    m_optionWidget->chkInvert->setChecked(setting->getBool("Texture/Pattern/Invert"));

    setChecked(setting->getBool("Texture/Pattern/Enabled"));
}

void KisTextureOption::resetGUI(KoResource* res)
{
    KoPattern *pattern = static_cast<KoPattern *>(res);
    if (!pattern) return;

    m_optionWidget->offsetSliderX->setRange(0, pattern->image().width() / 2);
    m_optionWidget->offsetSliderY->setRange(0, pattern->image().height() / 2);
}

void KisTextureProperties::recalculateMask()
{
    if (!pattern) return;

    m_mask = 0;

    QImage mask = pattern->image();

    if (mask.format() != QImage::Format_RGB32 ||
            mask.format() != QImage::Format_ARGB32) {

        mask = mask.convertToFormat(QImage::Format_ARGB32);
    }

    if (!qFuzzyCompare(scale, 0.0)) {
        QTransform tf;
        tf.scale(scale, scale);
        QRect rc = tf.mapRect(mask.rect());
        mask = mask.scaled(rc.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
#if QT_VERSION >= 0x040700
    const QRgb* pixel = reinterpret_cast<const QRgb*>(mask.constBits());
#else
    const QRgb* pixel = reinterpret_cast<const QRgb*>(mask.bits());
#endif
    int width = mask.width();
    int height = mask.height();

    const KoColorSpace *cs = KoColorSpaceRegistry::instance()->alpha8();
    m_mask = new KisPaintDevice(cs);

    KisHLineIteratorSP iter = m_mask->createHLineIteratorNG(0, 0, width);

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const QRgb currentPixel = pixel[row * width + col];

            const int red = qRed(currentPixel);
            const int green = qGreen(currentPixel);
            const int blue = qBlue(currentPixel);
            float alpha = qAlpha(currentPixel) / 255.0;

            const int grayValue = (red * 11 + green * 16 + blue * 5) / 32;
            float maskValue = (grayValue / 255.0) * alpha + (1 - alpha);

            if (invert) {
                maskValue = 1 - maskValue;
            }

            if (cutoffPolicy == 1 && (maskValue < (cutoffLeft / 255.0) || maskValue > (cutoffRight / 255.0))) {
                // mask out the dab if it's outside the pattern's cuttoff points
                maskValue = OPACITY_TRANSPARENT_F;
            }
            else if (cutoffPolicy == 2 && (maskValue < (cutoffLeft / 255.0) || maskValue > (cutoffRight / 255.0))) {
                maskValue = OPACITY_OPAQUE_F;
            }

            cs->setOpacity(iter->rawData(), maskValue, 1);
            iter->nextPixel();
        }
        iter->nextRow();
    }

    m_maskBounds = QRect(0, 0, width, height);
}


void KisTextureProperties::fillProperties(const KisPropertiesConfiguration *setting)
{
    pattern = KisEmbeddedPatternManager::loadEmbeddedPattern(setting);

    if (!pattern) {
        qWarning() << "WARNING: Couldn't load the pattern for a stroke";
        enabled = false;
        return;
    }

    enabled = setting->getBool("Texture/Pattern/Enabled", false);
    scale = setting->getDouble("Texture/Pattern/Scale", 1.0);
    offsetX = setting->getInt("Texture/Pattern/OffsetX");
    offsetY = setting->getInt("Texture/Pattern/OffsetY");
    texturingMode = (TexturingMode) setting->getInt("Texture/Pattern/TexturingMode", MULTIPLY);
    invert = setting->getBool("Texture/Pattern/Invert");
    cutoffLeft = setting->getInt("Texture/Pattern/CutoffLeft", 0);
    cutoffRight = setting->getInt("Texture/Pattern/CutoffRight", 255);
    cutoffPolicy = setting->getInt("Texture/Pattern/CutoffPolicy", 0);

    m_strengthOption.readOptionSetting(setting);
    m_strengthOption.resetAllSensors();

    recalculateMask();
}

void KisTextureProperties::apply(KisFixedPaintDeviceSP dab, const QPoint &offset, const KisPaintInformation & info)
{
    if (!enabled) return;

    KisPaintDeviceSP fillDevice = new KisPaintDevice(KoColorSpaceRegistry::instance()->alpha8());
    QRect rect = dab->bounds();

    int x = offset.x() % m_maskBounds.width() - offsetX;
    int y = offset.y() % m_maskBounds.height() - offsetY;

    KisFillPainter fillPainter(fillDevice);
    fillPainter.fillRect(x - 1, y - 1, rect.width() + 2, rect.height() + 2, m_mask, m_maskBounds);
    fillPainter.end();

    qreal pressure = m_strengthOption.apply(info);
    quint8 *dabData = dab->data();

    KisHLineIteratorSP iter = fillDevice->createHLineIteratorNG(x, y, rect.width());
    for (int row = 0; row < rect.height(); ++row) {
        for (int col = 0; col < rect.width(); ++col) {
            if (texturingMode == MULTIPLY) {
                dab->colorSpace()->multiplyAlpha(dabData, quint8(*iter->oldRawData() * pressure), 1);
            }
            else {
                int pressureOffset = (1.0 - pressure) * 255;

                qint16 maskA = *iter->oldRawData() + pressureOffset;
                quint8 dabA = dab->colorSpace()->opacityU8(dabData);

                dabA = qMax(0, (qint16)dabA - maskA);
                dab->colorSpace()->setOpacity(dabData, dabA, 1);
            }

            iter->nextPixel();
            dabData += dab->pixelSize();
        }
        iter->nextRow();
    }
}

