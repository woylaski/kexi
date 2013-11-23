/* This file is part of KimageShop^WKrayon^WKrita
 *
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_statusbar.h"


#include <QLabel>
#include <QFontMetrics>
#include <QToolButton>

#include <KoProgressBar.h>
#include <ksqueezedtextlabel.h>
#include <kstatusbar.h>
#include <klocale.h>

#include <KoIcon.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>

#include <kis_types.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_paint_device.h>

#include "kis_view2.h"
#include "canvas/kis_canvas2.h"
#include "kis_progress_widget.h"

#include "KoViewConverter.h"

enum {
    IMAGE_SIZE_ID,
    POINTER_POSITION_ID
};

KisStatusBar::KisStatusBar(KisView2 * view)
        : m_view(view)
{
    m_selectionStatusLabel = new QLabel(view);
    m_selectionStatusLabel->setPixmap(koIcon("tool_rect_selection").pixmap(22));
    m_selectionStatusLabel->setEnabled(false);
    view->addStatusBarItem(m_selectionStatusLabel);

    // XXX: Use the KStatusbar fixed size labels!
    m_statusBarStatusLabel = new KSqueezedTextLabel(view);
    m_statusBarStatusLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    connect(KoToolManager::instance(), SIGNAL(changedStatusText(const QString &)),
            m_statusBarStatusLabel, SLOT(setText(const QString &)));
    view->addStatusBarItem(m_statusBarStatusLabel, 2);

    m_statusBarProfileLabel = new KSqueezedTextLabel(view);
    m_statusBarProfileLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    view->addStatusBarItem(m_statusBarProfileLabel, 3);

    m_progress = new KisProgressWidget(view);
    view->addStatusBarItem(m_progress);

    m_imageSizeLabel = new QLabel(QString(), view);
    m_imageSizeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_imageSizeLabel->setMinimumWidth(100);
    view->addStatusBarItem(m_imageSizeLabel);

    m_pointerPositionLabel = new QLabel(QString(), view);
    m_pointerPositionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_pointerPositionLabel->setMinimumWidth(100);
    view->addStatusBarItem(m_pointerPositionLabel);
    m_pointerPositionLabel->setVisible(false);
}

KisStatusBar::~KisStatusBar()
{
    m_view->removeStatusBarItem(m_selectionStatusLabel);
    m_view->removeStatusBarItem(m_statusBarStatusLabel);
    m_view->removeStatusBarItem(m_statusBarProfileLabel);
    m_view->removeStatusBarItem(m_imageSizeLabel);
    m_view->removeStatusBarItem(m_pointerPositionLabel);
    m_view->removeStatusBarItem(m_progress);
}

void KisStatusBar::documentMousePositionChanged(const QPointF &pos)
{
    QPoint pixelPos = m_view->image()->documentToIntPixel(pos);

    pixelPos.setX(qBound(0, pixelPos.x(), m_view->image()->width() - 1));
    pixelPos.setY(qBound(0, pixelPos.y(), m_view->image()->height() - 1));
    m_pointerPositionLabel->setText(QString("%1, %2").arg(pixelPos.x()).arg(pixelPos.y()));
}

void KisStatusBar::imageSizeChanged()
{
    KisImageWSP image = m_view->image();
    qint32 w = image->width();
    qint32 h = image->height();

    m_imageSizeLabel->setText(QString("%1 x %2").arg(w).arg(h));
}

void KisStatusBar::setSelection(KisImageWSP image)
{
    Q_UNUSED(image);

    KisSelectionSP selection = m_view->selection();
    if (selection) {
        m_selectionStatusLabel->setEnabled(true);

        QRect r = selection->selectedExactRect();
        m_selectionStatusLabel->setToolTip(i18n("Selection Active: x = %1 y = %2 width = %3 height = %4", r.x(), r.y(), r.width(), r.height()));
        return;
    }
    m_selectionStatusLabel->setEnabled(false);
    m_selectionStatusLabel->setToolTip(i18n("No Selection"));
}

void KisStatusBar::setProfile(KisImageWSP image)
{
    if (m_statusBarProfileLabel == 0) {
        return;
    }

    if (!image) return;

    if (image->profile() == 0) {
        m_statusBarProfileLabel->setText(i18n("No profile"));
    } else {
        m_statusBarProfileLabel->setText(image->colorSpace()->name() + "  " + image->profile()->name());
    }

}

void KisStatusBar::setHelp(const QString &t)
{
    Q_UNUSED(t);
}

void KisStatusBar::updateStatusBarProfileLabel()
{
    setProfile(m_view->image());
}


KisProgressWidget* KisStatusBar::progress()
{
    return m_progress;
}

#include "kis_statusbar.moc"
