/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2009 Sven Langkamp <sven.langkamp@gmail.com>
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
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

#include "kis_brush_chooser.h"

#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QAbstractItemDelegate>
#include <klocale.h>

#include <KoResourceItemChooser.h>
#include <KoResourceServer.h>
#include <KoResourceServerAdapter.h>
#include <KoResourceServerProvider.h>

#include "kis_brush_registry.h"
#include "kis_brush_server.h"
#include "widgets/kis_slider_spin_box.h"
#include "widgets/kis_multipliers_double_slider_spinbox.h"

#include "kis_global.h"
#include "kis_gbr_brush.h"
#include "kis_debug.h"

/// The resource item delegate for rendering the resource preview
class KisBrushDelegate : public QAbstractItemDelegate
{
public:
    KisBrushDelegate(QObject * parent = 0) : QAbstractItemDelegate(parent) {}
    virtual ~KisBrushDelegate() {}
    /// reimplemented
    virtual void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;
    /// reimplemented
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const {
        return option.decorationSize;
    }
};

void KisBrushDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (! index.isValid())
        return;

    KisBrush *brush = static_cast<KisBrush*>(index.internalPointer());

    QRect itemRect = option.rect;
    QImage thumbnail = brush->image();

    if (thumbnail.height() > itemRect.height() || thumbnail.width() > itemRect.width()) {
        thumbnail = thumbnail.scaled(itemRect.size() , Qt::KeepAspectRatio);
    }

    painter->save();
    int dx = (itemRect.width() - thumbnail.width()) / 2;
    int dy = (itemRect.height() - thumbnail.height()) / 2;
    painter->drawImage(itemRect.x() + dx, itemRect.y() + dy, thumbnail);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlight(), 2.0));
        painter->drawRect(option.rect);
    }

    painter->restore();
}


KisBrushChooser::KisBrushChooser(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setObjectName(name);

    m_lbSize = new QLabel(i18n("Size:"), this);
    m_slSize = new KisMultipliersDoubleSliderSpinBox(this);
    m_slSize->setRange(0.0, 10.0, 2);
    m_slSize->setValue(5.0);
    m_slSize->addMultiplier(10);
    m_slSize->addMultiplier(100);
    QObject::connect(m_slSize, SIGNAL(valueChanged(qreal)), this, SLOT(slotSetItemSize(qreal)));


    m_lbRotation = new QLabel(i18n("Rotation:"), this);
    m_slRotation = new KisDoubleSliderSpinBox(this);
    m_slRotation->setRange(0.0, 360, 2);
    m_slRotation->setValue(0.0);
    QObject::connect(m_slRotation, SIGNAL(valueChanged(qreal)), this, SLOT(slotSetItemRotation(qreal)));

    m_lbSpacing = new QLabel(i18n("Spacing:"), this);
    m_slSpacing = new KisDoubleSliderSpinBox(this);
    m_slSpacing->setRange(0.02, 10, 2);
    m_slSpacing->setValue(0.1);
    m_slSpacing->setExponentRatio(3.0);
    QObject::connect(m_slSpacing, SIGNAL(valueChanged(qreal)), this, SLOT(slotSetItemSpacing(qreal)));

    m_chkColorMask = new QCheckBox(i18n("Use color as mask"), this);
    QObject::connect(m_chkColorMask, SIGNAL(toggled(bool)), this, SLOT(slotSetItemUseColorAsMask(bool)));

    m_lbName = new QLabel(this);

    KoResourceServer<KisBrush>* rServer = KisBrushServer::instance()->brushServer();
    KoResourceServerAdapter<KisBrush>* adapter = new KoResourceServerAdapter<KisBrush>(rServer);
    m_itemChooser = new KoResourceItemChooser(adapter, this);
    QString knsrcFile = "kritabrushes.knsrc";
    m_itemChooser->setKnsrcFile(knsrcFile);
    m_itemChooser->showGetHotNewStuff(true, true);
    m_itemChooser->showTaggingBar(true, true);
    m_itemChooser->setColumnCount(10);
    m_itemChooser->setRowHeight(30);
    m_itemChooser->setItemDelegate(new KisBrushDelegate(this));
    m_itemChooser->setCurrentItem(0, 0);

    connect(m_itemChooser, SIGNAL(resourceSelected(KoResource *)), this, SLOT(update(KoResource *)));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setObjectName("main layout");

    mainLayout->addWidget(m_lbName);
    mainLayout->addWidget(m_itemChooser, 10);

    QGridLayout *spacingLayout = new QGridLayout();

    mainLayout->addLayout(spacingLayout, 1);

    spacingLayout->addWidget(m_lbSize, 1, 0);
    spacingLayout->addWidget(m_slSize, 1, 1);
    spacingLayout->addWidget(m_lbRotation, 2, 0);
    spacingLayout->addWidget(m_slRotation, 2, 1);
    spacingLayout->addWidget(m_lbSpacing, 3, 0);
    spacingLayout->addWidget(m_slSpacing, 3, 1);
    spacingLayout->setColumnStretch(1, 3);

    QPushButton *resetBrushButton = new QPushButton(i18n("Reset Brush"), this);
    resetBrushButton->setToolTip(i18n("Reloads Spacing from file\nSets Scale to 1.0\nSets Rotation to 0.0"));
    connect(resetBrushButton, SIGNAL(clicked()), SLOT(slotResetBrush()));

    QHBoxLayout *resetHLayout = new QHBoxLayout();
    resetHLayout->addWidget(m_chkColorMask, 0);
    resetHLayout->addWidget(resetBrushButton, 0, Qt::AlignRight);

    spacingLayout->addLayout(resetHLayout, 4, 0, 1, 2);

    slotActivatedBrush(m_itemChooser->currentResource());
    update(m_itemChooser->currentResource());
}

KisBrushChooser::~KisBrushChooser()
{
}

void KisBrushChooser::setBrush(KisBrushSP _brush)
{
    m_itemChooser->setCurrentResource(_brush.data());
    update(_brush.data());
}

void KisBrushChooser::slotResetBrush()
{
    KisBrush *brush = dynamic_cast<KisBrush *>(m_itemChooser->currentResource());
    if (brush) {
        brush->load();
        brush->setScale(1.0);
        brush->setAngle(0.0);
        slotActivatedBrush(brush);
        update(brush);
        emit sigBrushChanged();
    }
}

void KisBrushChooser::slotSetItemSize(qreal sizeValue)
{
    KisBrush *brush = dynamic_cast<KisBrush *>(m_itemChooser->currentResource());

    if (brush) {
        int brushWidth = brush->width();

        brush->setScale(sizeValue / qreal(brushWidth));
        slotActivatedBrush(brush);
        emit sigBrushChanged();
    }
}

void KisBrushChooser::slotSetItemRotation(qreal rotationValue)
{
    KisBrush *brush = dynamic_cast<KisBrush *>(m_itemChooser->currentResource());
    if (brush) {
        brush->setAngle(rotationValue / 180.0 * M_PI);
        slotActivatedBrush(brush);

        emit sigBrushChanged();
    }
}

void KisBrushChooser::slotSetItemSpacing(qreal spacingValue)
{
    KisBrush *brush = dynamic_cast<KisBrush *>(m_itemChooser->currentResource());
    if (brush) {
        brush->setSpacing(spacingValue);
        slotActivatedBrush(brush);

        emit sigBrushChanged();
    }
}

void KisBrushChooser::slotSetItemUseColorAsMask(bool useColorAsMask)
{
    KisGbrBrush *brush = dynamic_cast<KisGbrBrush *>(m_itemChooser->currentResource());
    if (brush) {
        brush->setUseColorAsMask(useColorAsMask);
        slotActivatedBrush(brush);

        emit sigBrushChanged();
    }
}

void KisBrushChooser::update(KoResource * resource)
{
    KisBrush* brush = dynamic_cast<KisBrush*>(resource);

    if (brush) {
        blockSignals(true);
        QString text = QString("%1 (%2 x %3)")
                       .arg(i18n(brush->name().toUtf8().data()))
                       .arg(brush->width())
                       .arg(brush->height());

        m_lbName->setText(text);
        m_slSpacing->blockSignals(true);
        m_slSpacing->setValue(brush->spacing());
        m_slSpacing->blockSignals(false);

        m_slRotation->blockSignals(true);
        m_slRotation->setValue(brush->angle() * 180 / M_PI);
        m_slRotation->blockSignals(false);

        m_slSize->blockSignals(true);
        m_slSize->setValue(brush->width() * brush->scale());
        m_slSize->blockSignals(false);

        // useColorAsMask support is only in gimp brush so far
        KisGbrBrush *gimpBrush = dynamic_cast<KisGbrBrush*>(resource);
        if (gimpBrush) {
            m_chkColorMask->setChecked(gimpBrush->useColorAsMask());
        }
        m_chkColorMask->setEnabled(brush->hasColor() && gimpBrush);
        blockSignals(false);

        slotActivatedBrush(brush);
        emit sigBrushChanged();
    }
}

void KisBrushChooser::slotActivatedBrush(KoResource * resource)
{
    if (m_brush) {
        m_brush->clearBrushPyramid();
    }
    KisBrush* brush = dynamic_cast<KisBrush*>(resource);
    if (brush) {
        m_brush = brush;
        m_brush->prepareBrushPyramid();
    }
}

void KisBrushChooser::setBrushSize(qreal xPixels, qreal yPixels)
{
    Q_UNUSED(yPixels);
    qreal oldWidth = m_brush->width() * m_brush->scale();
    qreal newWidth = oldWidth + xPixels;

    newWidth = qMax(newWidth, qreal(0.1));

    m_slSize->setValue(newWidth);
}


#include "kis_brush_chooser.moc"
