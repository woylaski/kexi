/* This file is part of the Calligra project
 * Copyright (C) 2005 Thomas Zander <zander@kde.org>
 * Copyright (C) 2005 C. Boemann <cbo@boemann.dk>
 * Copyright (C) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "widgets/kis_custom_image_widget.h"

#include <QMimeData>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QRect>
#include <QApplication>
#include <QClipboard>
#include <QDesktopWidget>
#include <kundo2command.h>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>


#include <kcolorcombo.h>
#include <kcomponentdata.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include <kis_debug.h>

#include <KoIcon.h>
#include <KoCompositeOp.h>
#include <KoUnitDoubleSpinBox.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoID.h>
#include <KoColor.h>
#include <KoUnit.h>
#include <KoColorModelStandardIds.h>

#include <kis_fill_painter.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_painter.h>

#include "kis_clipboard.h"
#include "kis_doc2.h"
#include "widgets/kis_cmb_idlist.h"
#include "widgets/squeezedcombobox.h"


KisCustomImageWidget::KisCustomImageWidget(QWidget* parent, KisDoc2* doc, qint32 defWidth, qint32 defHeight, double resolution, const QString& defColorModel, const QString& defColorDepth, const QString& defColorProfile, const QString& imageName)
    : WdgNewImage(parent)
{
    setObjectName("KisCustomImageWidget");
    m_doc = doc;

    txtName->setText(imageName);
    m_widthUnit = KoUnit(KoUnit::Pixel, resolution);
    doubleWidth->setValue(defWidth);
    doubleWidth->setDecimals(0);
    m_width = m_widthUnit.fromUserValue(defWidth);
    cmbWidthUnit->addItems(KoUnit::listOfUnitNameForUi(KoUnit::ListAll));
    cmbWidthUnit->setCurrentIndex(m_widthUnit.indexInListForUi(KoUnit::ListAll));

    m_heightUnit = KoUnit(KoUnit::Pixel, resolution);
    doubleHeight->setValue(defHeight);
    doubleHeight->setDecimals(0);
    m_height = m_heightUnit.fromUserValue(defHeight);
    cmbHeightUnit->addItems(KoUnit::listOfUnitNameForUi(KoUnit::ListAll));
    cmbHeightUnit->setCurrentIndex(m_heightUnit.indexInListForUi(KoUnit::ListAll));

    doubleResolution->setValue(72.0 * resolution);
    doubleResolution->setDecimals(0);
    
    grpClipboard->hide();

    connect(cmbPredefined, SIGNAL(activated(int)), SLOT(predefinedClicked(int)));
    connect(doubleResolution, SIGNAL(valueChanged(double)),
            this, SLOT(resolutionChanged(double)));
    connect(cmbWidthUnit, SIGNAL(activated(int)),
            this, SLOT(widthUnitChanged(int)));
    connect(doubleWidth, SIGNAL(valueChanged(double)),
            this, SLOT(widthChanged(double)));
    connect(cmbHeightUnit, SIGNAL(activated(int)),
            this, SLOT(heightUnitChanged(int)));
    connect(doubleHeight, SIGNAL(valueChanged(double)),
            this, SLOT(heightChanged(double)));
    connect(createButton, SIGNAL(clicked()), this, SLOT(createImage()));
    createButton->setDefault(true);

    bnPortrait->setIcon(koIcon("portrait"));
    connect(bnPortrait, SIGNAL(clicked()), SLOT(switchWidthHeight()));
    connect(bnLandscape, SIGNAL(clicked()), SLOT(switchWidthHeight()));
    bnLandscape->setIcon(koIcon("landscape"));

    connect(doubleWidth, SIGNAL(valueChanged(double)), this, SLOT(switchPortraitLandscape()));
    connect(doubleHeight, SIGNAL(valueChanged(double)), this, SLOT(switchPortraitLandscape()));
    connect(bnSaveAsPredefined, SIGNAL(clicked()), this, SLOT(saveAsPredefined()));

    colorSpaceSelector->setCurrentColorModel(KoID(defColorModel));
    colorSpaceSelector->setCurrentColorDepth(KoID(defColorDepth));
    colorSpaceSelector->setCurrentProfile(defColorProfile);

    //connect(chkFromClipboard,SIGNAL(stateChanged(int)),this,SLOT(clipboardDataChanged()));
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));
    connect(QApplication::clipboard(), SIGNAL(selectionChanged()), this, SLOT(clipboardDataChanged()));
    connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), this, SLOT(clipboardDataChanged()));

    connect(bnScreenSize, SIGNAL(clicked()), this, SLOT(screenSizeClicked()));
    connect(colorSpaceSelector, SIGNAL(selectionChanged(bool)), createButton, SLOT(setEnabled(bool)));

    fillPredefined();
    switchPortraitLandscape();
}

void KisCustomImageWidget::showEvent(QShowEvent *){
    this->createButton->setFocus(); 
}

KisCustomImageWidget::~KisCustomImageWidget()
{
    qDeleteAll(m_predefined);
    m_predefined.clear();
}

void KisCustomImageWidget::resolutionChanged(double res)
{
    if (m_widthUnit.type() == KoUnit::Pixel) {
        m_widthUnit.setFactor(res / 72.0);
        m_width = m_widthUnit.fromUserValue(doubleWidth->value());
    }

    if (m_heightUnit.type() == KoUnit::Pixel) {
        m_heightUnit.setFactor(res / 72.0);
        m_height = m_heightUnit.fromUserValue(doubleHeight->value());
    }
}


void KisCustomImageWidget::widthUnitChanged(int index)
{
    doubleWidth->blockSignals(true);

    m_widthUnit = KoUnit::fromListForUi(index, KoUnit::ListAll);
    if (m_widthUnit.type() == KoUnit::Pixel) {
        doubleWidth->setDecimals(0);
        m_widthUnit.setFactor(doubleResolution->value() / 72.0);
    } else {
        doubleWidth->setDecimals(2);
    }

    doubleWidth->setValue(KoUnit::ptToUnit(m_width, m_widthUnit));

    doubleWidth->blockSignals(false);
}

void KisCustomImageWidget::widthChanged(double value)
{
    m_width = m_widthUnit.fromUserValue(value);
}

void KisCustomImageWidget::heightUnitChanged(int index)
{
    doubleHeight->blockSignals(true);

    m_heightUnit = KoUnit::fromListForUi(index, KoUnit::ListAll);
    if (m_heightUnit.type() == KoUnit::Pixel) {
        doubleHeight->setDecimals(0);
        m_heightUnit.setFactor(doubleResolution->value() / 72.0);
    } else {
        doubleHeight->setDecimals(2);
    }

    doubleHeight->setValue(KoUnit::ptToUnit(m_height, m_heightUnit));

    doubleHeight->blockSignals(false);
}

void KisCustomImageWidget::heightChanged(double value)
{
    m_height = m_heightUnit.fromUserValue(value);
}

void KisCustomImageWidget::createImage()
{
    createNewImage();

    emit documentSelected();
}

void KisCustomImageWidget::createNewImage()
{
    const KoColorSpace * cs = colorSpaceSelector->currentColorSpace();

    QColor qc = cmbColor->color();

    qint32 width, height;
    double resolution;
    resolution =  doubleResolution->value() / 72.0;  // internal resolution is in pixels per pt

    width = static_cast<qint32>(0.5  + KoUnit::ptToUnit(m_width, KoUnit(KoUnit::Pixel, resolution)));
    height = static_cast<qint32>(0.5 + KoUnit::ptToUnit(m_height, KoUnit(KoUnit::Pixel, resolution)));

    qc.setAlpha(backgroundOpacity());
    KoColor bgColor(qc, cs);
    m_doc->newImage(txtName->text(), width, height, cs, bgColor, txtDescription->toPlainText(), resolution);

    KisImageWSP image = m_doc->image();
    if (image && image->root() && image->root()->firstChild()) {
        KisLayer * layer = dynamic_cast<KisLayer*>(image->root()->firstChild().data());
        if (layer) {
            layer->setOpacity(OPACITY_OPAQUE_U8);
        }
        // Hack: with a semi-transparent background color, the projection isn't composited right if we just set the default pixel
        if (layer && backgroundOpacity() < OPACITY_OPAQUE_U8) {
            KisFillPainter painter;
            painter.begin(layer->paintDevice());
            painter.fillRect(0, 0, width, height, bgColor, backgroundOpacity());

        }
       
        layer->setDirty(QRect(0, 0, width, height));
    }
}

quint8 KisCustomImageWidget::backgroundOpacity() const
{
    qint32 opacity = sliderOpacity->value();

    if (!opacity)
        return 0;

    return (opacity * 255) / 100;
}

void KisCustomImageWidget::clipboardDataChanged()
{
}

void KisCustomImageWidget::screenSizeClicked()
{
    QSize sz = QApplication::desktop()->screenGeometry(this).size();

    const int index = KoUnit(KoUnit::Pixel).indexInListForUi(KoUnit::ListAll);
    cmbWidthUnit->setCurrentIndex(index);
    cmbHeightUnit->setCurrentIndex(index);
    widthUnitChanged(cmbWidthUnit->currentIndex());
    heightUnitChanged(cmbHeightUnit->currentIndex());

    doubleWidth->setValue(sz.width());
    doubleHeight->setValue(sz.height());
}

void KisCustomImageWidget::fillPredefined()
{
    cmbPredefined->addItem("");

    QString appName = KGlobal::mainComponent().componentName();
    QStringList definitions = KGlobal::dirs()->findAllResources("data", appName + "/predefined_image_sizes/*", KStandardDirs::Recursive);

    if (!definitions.empty()) {

        foreach(const QString &definition, definitions) {
            QFile f(definition);
            f.open(QIODevice::ReadOnly);
            if (f.exists()) {
                QString xml = QString::fromUtf8(f.readAll());
                KisPropertiesConfiguration *predefined = new KisPropertiesConfiguration;
                predefined->fromXML(xml);
                if (predefined->hasProperty("name")
                        && predefined->hasProperty("width")
                        && predefined->hasProperty("height")
                        && predefined->hasProperty("resolution")
                        && predefined->hasProperty("x-unit")
                        && predefined->hasProperty("y-unit")) {
                    m_predefined << predefined;
                    cmbPredefined->addItem(predefined->getString("name"));
                }
            }
        }
    }

    cmbPredefined->setCurrentIndex(0);

}


void KisCustomImageWidget::predefinedClicked(int index)
{
    if (index < 1 || index > m_predefined.size()) return;

    KisPropertiesConfiguration *predefined = m_predefined[index - 1];
    txtPredefinedName->setText(predefined->getString("name"));
    doubleWidth->setValue(predefined->getDouble("width"));
    doubleHeight->setValue(predefined->getDouble("height"));
    doubleResolution->setValue(predefined->getDouble("resolution"));
    cmbWidthUnit->setCurrentIndex(predefined->getInt("x-unit"));
    cmbHeightUnit->setCurrentIndex(predefined->getInt("y-unit"));

}

void KisCustomImageWidget::saveAsPredefined()
{
    QString fileName = txtPredefinedName->text();
    if (fileName.isEmpty()) {
        return;
    }
    QString saveLocation = KGlobal::mainComponent().dirs()->saveLocation("data");
    QString appName = KGlobal::mainComponent().componentName();

    QDir d;
    d.mkpath(saveLocation + appName + "/predefined_image_sizes/");

    QFile f(saveLocation + appName + "/predefined_image_sizes/" + fileName.replace(' ', '_').replace('(', '_').replace(')', '_') + ".predefinedimage");

    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    KisPropertiesConfiguration *predefined = new KisPropertiesConfiguration();
    predefined->setProperty("name", txtPredefinedName->text());
    predefined->setProperty("width", doubleWidth->value());
    predefined->setProperty("height", doubleHeight->value());
    predefined->setProperty("resolution", doubleResolution->value());
    predefined->setProperty("x-unit", cmbWidthUnit->currentIndex());
    predefined->setProperty("y-unit", cmbHeightUnit->currentIndex());

    QString xml = predefined->toXML();

    f.write(xml.toUtf8());
    f.flush();
    f.close();

    int i = 0;
    bool found = false;
    foreach(KisPropertiesConfiguration *pr, m_predefined) {
        if (pr->getString("name") == txtPredefinedName->text()) {
            found = true;
            break;
        }
        ++i;
    }
    if (found) {
        m_predefined[i] = predefined;
    }
    else {
        m_predefined.append(predefined);
        cmbPredefined->addItem(txtPredefinedName->text());
    }

}

void KisCustomImageWidget::switchWidthHeight()
{
    double width = doubleWidth->value();
    doubleWidth->setValue(doubleHeight->value());
    doubleHeight->setValue(width);
}

void KisCustomImageWidget::switchPortraitLandscape()
{
    if(doubleWidth->value() > doubleHeight->value())
        bnLandscape->setChecked(true);
    else
        bnPortrait->setChecked(true);
}

#include "kis_custom_image_widget.moc"
