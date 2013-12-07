/*
 *  Copyright (c) 1999 Matthias Elter <me@kde.org>
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include "kis_tool_colorpicker.h"
#include <string.h>

#include <QPoint>
#include <QLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QList>
#include <QWidget>

#include <kactioncollection.h>
#include <kaction.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kis_layer.h"
#include "kis_cursor.h"
#include "kis_image.h"
#include "kis_paint_device.h"
#include "kis_properties_configuration.h"

#include "kis_tool_colorpicker.moc"
#include "KoPointerEvent.h"
#include "KoCanvasBase.h"
#include "kis_random_accessor_ng.h"
#include "KoColor.h"
#include "KoResourceServerProvider.h"
#include "KoColorSet.h"
#include <KoChannelInfo.h>
#include <KoMixColorsOp.h>

namespace
{
    // The location of the sample all visible layers in the combobox
    const int SAMPLE_MERGED = 0;
    const QString CONFIG_GROUP_NAME = "tool_color_picker";
}

KisToolColorPicker::Configuration::Configuration()
    : toForegroundColor(true),
      updateColor(true),
      addPalette(false),
      normaliseValues(false),
      sampleMerged(true),
      radius(1)
{
}

inline QString getConfigKey(KisTool::ToolActivation activation) {
    QString configKey;

    switch (activation) {
    case KisTool::TemporaryActivation:
        configKey = "ColorPickerTemporaryActivation";
        break;
    case KisTool::DefaultActivation:
        configKey = "ColorPickerDefaultActivation";
        break;
    };

    return configKey;
}

void KisToolColorPicker::Configuration::save(ToolActivation activation) const
{
    KisPropertiesConfiguration props;
    props.setProperty("toForegroundColor", toForegroundColor);
    props.setProperty("updateColor", updateColor);
    props.setProperty("addPalette", addPalette);
    props.setProperty("normaliseValues", normaliseValues);
    props.setProperty("sampleMerged", sampleMerged);
    props.setProperty("radius", radius);

    KConfigGroup config = KGlobal::config()->group(CONFIG_GROUP_NAME);

    config.writeEntry(getConfigKey(activation), props.toXML());
}

void KisToolColorPicker::Configuration::load(ToolActivation activation)
{
    KisPropertiesConfiguration props;

    KConfigGroup config = KGlobal::config()->group(CONFIG_GROUP_NAME);
    props.fromXML(config.readEntry(getConfigKey(activation)));

    toForegroundColor = props.getBool("toForegroundColor", true);
    updateColor = props.getBool("updateColor", true);
    addPalette = props.getBool("addPalette", false);
    normaliseValues = props.getBool("normaliseValues", false);
    sampleMerged = props.getBool("sampleMerged", activation == KisTool::TemporaryActivation ? false : true);
    radius = props.getInt("radius", 1);
}

KisToolColorPicker::KisToolColorPicker(KoCanvasBase* canvas)
        :  KisTool(canvas, KisCursor::pickerCursor())
{
    setObjectName("tool_colorpicker");
    m_isActivated = false;
    m_optionsWidget = 0;
    m_pickedColor = KoColor();
}

KisToolColorPicker::~KisToolColorPicker()
{
    if (m_isActivated) {
        m_config.save(m_toolActivationSource);
    }
}

void KisToolColorPicker::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(gc);
    Q_UNUSED(converter);
}

void KisToolColorPicker::activate(ToolActivation activation, const QSet<KoShape*> &shapes)
{
    m_isActivated = true;
    m_toolActivationSource = activation;
    m_config.load(m_toolActivationSource);
    updateOptionWidget();

    KisTool::activate(activation, shapes);
}
void KisToolColorPicker::deactivate()
{
    m_config.save(m_toolActivationSource);
    m_isActivated = false;
    KisTool::deactivate();
}

void KisToolColorPicker::pickColor(const QPointF& pos)
{
        if(m_colorPickerDelayTimer.isActive()) {
            return;
        }
        else {
            m_colorPickerDelayTimer.setSingleShot(true);
            m_colorPickerDelayTimer.start(100);
        }

        if (!currentNode())
        {
            return;
        }

        KisPaintDeviceSP dev = currentNode()->paintDevice();
        if (!dev) return;



        if (m_optionsWidget->cmbSources->currentIndex() == SAMPLE_MERGED) {
            currentImage()->lock();
            dev = currentImage()->projection();
        }

        if (m_config.radius == 1) {
            dev->pixel(pos.x(), pos.y(), &m_pickedColor);
        } else {
            // radius 2 ==> 9 pixels, 3 => 9 pixels, etc
            static int counts[] = { 0, 1, 9, 25, 45, 69, 109, 145, 193, 249 };

            const KoColorSpace* cs = dev->colorSpace();
            int pixelSize = cs->pixelSize();

            quint8* data = new quint8[pixelSize];
            quint8** pixels = new quint8*[counts[m_config.radius]];
            qint16* weights = new qint16[counts[m_config.radius]];

            int i = 0;
            KisRandomConstAccessorSP accessor = dev->createRandomConstAccessorNG(0, 0);

            for (int y = -m_config.radius; y <= m_config.radius; y++) {
                for (int x = -m_config.radius; x <= m_config.radius; x++) {
                    if (((x * x) + (y * y)) < m_config.radius * m_config.radius) {

                        accessor->moveTo(pos.x() + x, pos.y() + y);

                        pixels[i] = new quint8[pixelSize];
                        memcpy(pixels[i], accessor->oldRawData(), pixelSize);

                        if (x == 0 && y == 0) {
                            // Because the sum of the weights must be 255,
                            // we cheat a bit, and weigh the center pixel differently in order
                            // to sum to 255 in total
                            // It's -(counts -1), because we'll add the center one implicitly
                            // through that calculation
                            weights[i] = 255 - (counts[m_config.radius] - 1) * (255 / counts[m_config.radius]);
                        } else {
                            weights[i] = 255 / counts[m_config.radius];
                        }
                        i++;
                    }
                }
            }
            // Weird, I can't do that directly :/
            const quint8** cpixels = const_cast<const quint8**>(pixels);
            cs->mixColorsOp()->mixColors(cpixels, weights, counts[m_config.radius], data);
            m_pickedColor = KoColor(data, cs);

            for (i = 0; i < counts[m_config.radius]; i++){
                delete[] pixels[i];
            }
            delete[] pixels;
            delete[] data;
        }

        if (m_config.updateColor) {
            if (m_config.toForegroundColor)
                canvas()->resourceManager()->setResource(KoCanvasResourceManager::ForegroundColor, m_pickedColor);
            else
                canvas()->resourceManager()->setResource(KoCanvasResourceManager::BackgroundColor, m_pickedColor);
        }

        if (m_optionsWidget->cmbSources->currentIndex() == SAMPLE_MERGED) {
            currentImage()->unlock();
        }
}

void KisToolColorPicker::beginPrimaryAction(KoPointerEvent *event)
{
    bool sampleMerged = m_optionsWidget->cmbSources->currentIndex() == SAMPLE_MERGED;
    if (!sampleMerged) {
        if (!currentNode()) {
            KMessageBox::information(0, i18n("Cannot pick a color as no layer is active."));
            event->ignore();
            return;
        }
        if (!currentNode()->visible()) {
            KMessageBox::information(0, i18n("Cannot pick a color as the active layer is not visible."));
            event->ignore();
            return;
        }
    }

    QPoint pos = convertToIntPixelCoord(event);
    // the color picking has to start in the visible part of the layer
    if (!currentImage()->bounds().contains(pos)) {
        event->ignore();
        return;
    }

    setMode(KisTool::PAINT_MODE);
    pickColor(pos);
    displayPickedColor();
}

void KisToolColorPicker::continuePrimaryAction(KoPointerEvent *event)
{
    KIS_ASSERT_RECOVER_RETURN(mode() == KisTool::PAINT_MODE);

    QPoint pos = convertToIntPixelCoord(event);
    pickColor(pos);
    displayPickedColor();
}

void KisToolColorPicker::endPrimaryAction(KoPointerEvent *event)
{
    Q_UNUSED(event);
    KIS_ASSERT_RECOVER_RETURN(mode() == KisTool::PAINT_MODE);

    if (m_config.addPalette) {
        KoColorSetEntry ent;
        ent.color = m_pickedColor;
        // We don't ask for a name, too intrusive here

        KoColorSet* palette = m_palettes.at(m_optionsWidget->cmbPalette->currentIndex());
        palette->add(ent);

        if (!palette->save()) {
            KMessageBox::error(0, i18n("Cannot write to palette file %1. Maybe it is read-only.", palette->filename()), i18n("Palette"));
        }
    }
}

void KisToolColorPicker::displayPickedColor()
{
    if (m_pickedColor.data() && m_optionsWidget) {

        QList<KoChannelInfo *> channels = m_pickedColor.colorSpace()->channels();
        m_optionsWidget->listViewChannels->clear();

        for (int i = 0; i < channels.count(); ++i) {
            QString channelValueText;
            int pos = channels[i]->pos() / channels[i]->size(); // HACK This doesn't work if the channels differ in size

            if (m_config.normaliseValues) {
                channelValueText = m_pickedColor.colorSpace()->normalisedChannelValueText(m_pickedColor.data(), pos);
            } else {
                channelValueText = m_pickedColor.colorSpace()->channelValueText(m_pickedColor.data(), pos);
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(m_optionsWidget->listViewChannels);
            item->setText(0, channels[i]->name());
            item->setText(1, channelValueText);
        }
    }
}

QWidget* KisToolColorPicker::createOptionWidget()
{
    m_optionsWidget = new ColorPickerOptionsWidget(0);
    m_optionsWidget->setObjectName(toolId() + " option widget");
    m_optionsWidget->listViewChannels->setSortingEnabled(false);

    updateOptionWidget();

    connect(m_optionsWidget->cbUpdateCurrentColor, SIGNAL(toggled(bool)), SLOT(slotSetUpdateColor(bool)));
    connect(m_optionsWidget->cbNormaliseValues, SIGNAL(toggled(bool)), SLOT(slotSetNormaliseValues(bool)));
    connect(m_optionsWidget->cbPalette, SIGNAL(toggled(bool)),
            SLOT(slotSetAddPalette(bool)));
    connect(m_optionsWidget->radius, SIGNAL(valueChanged(int)),
            SLOT(slotChangeRadius(int)));
    connect(m_optionsWidget->cmbSources, SIGNAL(currentIndexChanged(int)),
            SLOT(slotSetColorSource(int)));

    KoResourceServer<KoColorSet>* srv = KoResourceServerProvider::instance()->paletteServer();

    if (!srv) {
        return m_optionsWidget;
    }

    QList<KoColorSet*> palettes = srv->resources();

    foreach(KoColorSet *palette, palettes) {
        if (palette) {
            m_optionsWidget->cmbPalette->addSqueezedItem(palette->name());
            m_palettes.append(palette);
        }
    }

    //TODO
    //connect(srv, SIGNAL(resourceAdded(KoResource*)), this, SLOT(slotAddPalette(KoResource*)));
    //m_optionsWidget->setFixedHeight(m_optionsWidget->sizeHint().height());

    return m_optionsWidget;
}

void KisToolColorPicker::updateOptionWidget()
{
    if (!m_optionsWidget) return;

    m_optionsWidget->cbNormaliseValues->setChecked(m_config.normaliseValues);
    m_optionsWidget->cbUpdateCurrentColor->setChecked(m_config.updateColor);
    m_optionsWidget->cmbSources->setCurrentIndex(SAMPLE_MERGED + !m_config.sampleMerged);
    m_optionsWidget->cbPalette->setChecked(m_config.addPalette);
    m_optionsWidget->radius->setValue(m_config.radius);
}

void KisToolColorPicker::setToForeground(bool newValue)
{
    m_config.toForegroundColor = newValue;
    emit toForegroundChanged();
}

bool KisToolColorPicker::toForeground() const
{
    return m_config.toForegroundColor;
}

void KisToolColorPicker::slotSetUpdateColor(bool state)
{
    m_config.updateColor = state;
}


void KisToolColorPicker::slotSetNormaliseValues(bool state)
{
    m_config.normaliseValues = state;
    displayPickedColor();
}

void KisToolColorPicker::slotSetAddPalette(bool state)
{
    m_config.addPalette = state;
}

void KisToolColorPicker::slotChangeRadius(int value)
{
    m_config.radius = value;
}

void KisToolColorPicker::slotSetColorSource(int value)
{
    m_config.sampleMerged = value == SAMPLE_MERGED;
}

void KisToolColorPicker::slotAddPalette(KoResource* resource)
{
    KoColorSet* palette = dynamic_cast<KoColorSet*>(resource);
    if (palette) {
        m_optionsWidget->cmbPalette->addSqueezedItem(palette->name());
        m_palettes.append(palette);
    }
}

