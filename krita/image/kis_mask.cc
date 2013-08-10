/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *            (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_mask.h"


#include <kis_debug.h>

// to prevent incomplete class types on "delete selection->flatten();"
#include <kundo2command.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOp.h>

#include "kis_paint_device.h"
#include "kis_selection.h"
#include "kis_pixel_selection.h"
#include "kis_painter.h"

#include "kis_image.h"
#include "kis_layer.h"

#include "tiles3/kis_lockless_stack.h"

struct KisMask::Private {
    class CachedPaintDevice {
    public:
        KisPaintDeviceSP getDevice(KisPaintDeviceSP prototype) {
            KisPaintDeviceSP device;

            if(!m_stack.pop(device)) {
                device = new KisPaintDevice(prototype->colorSpace());
            }

            device->prepareClone(prototype);
            return device;
        }

        void putDevice(KisPaintDeviceSP device) {
            device->clear();
            m_stack.push(device);
        }

    private:
        KisLocklessStack<KisPaintDeviceSP> m_stack;
    };

    Private(KisMask *_q) : q(_q) {}

    mutable KisSelectionSP selection;
    CachedPaintDevice paintDeviceCache;
    KisMask *q;

    void initSelectionImpl(KisSelectionSP copyFrom, KisLayerSP parentLayer, KisPaintDeviceSP copyFromDevice);
};

KisMask::KisMask(const QString & name)
        : KisNode()
        , m_d(new Private(this))
{
    setName(name);
}

KisMask::KisMask(const KisMask& rhs)
        : KisNode(rhs)
        , m_d(new Private(this))
{
    setName(rhs.name());

    if (rhs.m_d->selection) {
        m_d->selection = new KisSelection(*rhs.m_d->selection.data());
        m_d->selection->setParentNode(this);
    }
}

KisMask::~KisMask()
{
    delete m_d;
}

const KoColorSpace * KisMask::colorSpace() const
{
    KisNodeSP parentNode = parent();
    return parentNode ? parentNode->colorSpace() : 0;
}

const KoCompositeOp * KisMask::compositeOp() const
{
    /**
     * FIXME: This function duplicates the same function from
     * KisLayer. We can't move it to KisBaseNode as it doesn't
     * know anything about parent() method of KisNode
     * Please think it over...
     */

    KisNodeSP parentNode = parent();
    if (!parentNode) return 0;

    if (!parentNode->colorSpace()) return 0;
    const KoCompositeOp* op = parentNode->colorSpace()->compositeOp(compositeOpId());
    return op ? op : parentNode->colorSpace()->compositeOp(COMPOSITE_OVER);
}

void KisMask::initSelection(KisSelectionSP copyFrom, KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(copyFrom, parentLayer, 0);
}

void KisMask::initSelection(KisPaintDeviceSP copyFromDevice, KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(0, parentLayer, copyFromDevice);
}

void KisMask::initSelection(KisLayerSP parentLayer)
{
    m_d->initSelectionImpl(0, parentLayer, 0);
}

void KisMask::Private::initSelectionImpl(KisSelectionSP copyFrom, KisLayerSP parentLayer, KisPaintDeviceSP copyFromDevice)
{
    Q_ASSERT(parentLayer);

    KisPaintDeviceSP parentPaintDevice = parentLayer->original();

    if (copyFrom) {
        /**
         * We can't use setSelection as we may not have parent() yet
         */
        selection = new KisSelection(*copyFrom);
        selection->setDefaultBounds(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));
        if (copyFrom->hasShapeSelection()) {
            delete selection->flatten();
        }
    } else if (copyFromDevice) {
        selection = new KisSelection(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));

        KisPainter gc(selection->pixelSelection());
        gc.setCompositeOp(COMPOSITE_COPY);
        QRect rc(copyFromDevice->extent());
        gc.bitBlt(rc.topLeft(), copyFromDevice, rc);

    } else {
        selection = new KisSelection(new KisSelectionDefaultBounds(parentPaintDevice, parentLayer->image()));

        quint8 newDefaultPixel = MAX_SELECTED;
        selection->pixelSelection()->setDefaultPixel(&newDefaultPixel);
    }
    selection->setParentNode(q);
    selection->updateProjection();
}

KisSelectionSP KisMask::selection() const
{
    /**
     * The mask is created without any selection present.
     * You must always init the selection with initSelection() method.
     */
    Q_ASSERT(m_d->selection);

    return m_d->selection;
}

KisPaintDeviceSP KisMask::paintDevice() const
{
    return selection()->pixelSelection();
}

KisPaintDeviceSP KisMask::original() const
{
    return paintDevice();
}

KisPaintDeviceSP KisMask::projection() const
{
    return paintDevice();
}

void KisMask::setSelection(KisSelectionSP selection)
{
    m_d->selection = selection;
    if (parent()) {
        const KisLayer *parentLayer = qobject_cast<const KisLayer*>(parent());
        m_d->selection->setDefaultBounds(new KisDefaultBounds(parentLayer->image()));
    }
    m_d->selection->setParentNode(this);
}

void KisMask::select(const QRect & rc, quint8 selectedness)
{
    KisSelectionSP sel = selection();
    KisPixelSelectionSP psel = sel->pixelSelection();
    psel->select(rc, selectedness);
    sel->updateProjection(rc);
}


QRect KisMask::decorateRect(KisPaintDeviceSP &src,
                            KisPaintDeviceSP &dst,
                            const QRect & rc) const
{
    Q_UNUSED(src);
    Q_UNUSED(dst);
    Q_ASSERT_X(0, "KisMask::decorateRect", "Should be overridden by successors");
    return rc;
}

void KisMask::apply(KisPaintDeviceSP projection, const QRect & rc) const
{
    if (selection()) {

        m_d->selection->updateProjection(rc);

        if(!extent().intersects(rc))
            return;

        KisPaintDeviceSP cacheDevice = m_d->paintDeviceCache.getDevice(projection);

        QRect updatedRect = decorateRect(projection, cacheDevice, rc);

        KisPainter gc(projection);
        gc.setCompositeOp(compositeOp());
        gc.setOpacity(opacity());
        gc.setSelection(m_d->selection);
        gc.bitBlt(updatedRect.topLeft(), cacheDevice, updatedRect);

        m_d->paintDeviceCache.putDevice(cacheDevice);

    } else {
        KisPaintDeviceSP cacheDevice = m_d->paintDeviceCache.getDevice(projection);
        cacheDevice->makeCloneFromRough(projection, rc);
        projection->clear(rc);

        // FIXME: how about opacity and compositeOp?
        decorateRect(cacheDevice, projection, rc);

        m_d->paintDeviceCache.putDevice(cacheDevice);
    }
}

QRect KisMask::needRect(const QRect &rect,  PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    QRect resultRect = rect;
    if (m_d->selection)
        resultRect &= m_d->selection->selectedRect();

    return resultRect;
}

QRect KisMask::changeRect(const QRect &rect, PositionToFilthy pos) const
{
    Q_UNUSED(pos);
    QRect resultRect = rect;
    if (m_d->selection)
        resultRect &= m_d->selection->selectedRect();

    return resultRect;
}

QRect KisMask::extent() const
{
    return m_d->selection ? m_d->selection->selectedRect() :
           parent() ? parent()->extent() : QRect();
}

QRect KisMask::exactBounds() const
{
    return m_d->selection ? m_d->selection->selectedExactRect() :
           parent() ? parent()->exactBounds() : QRect();
}

qint32 KisMask::x() const
{
    return m_d->selection ? m_d->selection->x() :
           parent() ? parent()->x() : 0;
}

qint32 KisMask::y() const
{
    return m_d->selection ? m_d->selection->y() :
           parent() ? parent()->y() : 0;
}

void KisMask::setX(qint32 x)
{
    if (m_d->selection)
        m_d->selection->setX(x);
}

void KisMask::setY(qint32 y)
{
    if (m_d->selection)
        m_d->selection->setY(y);
}

QImage KisMask::createThumbnail(qint32 w, qint32 h)
{
    KisPaintDeviceSP originalDevice =
        selection() ? selection()->projection() : 0;

    return originalDevice ?
           originalDevice->createThumbnail(w, h,
                                           KoColorConversionTransformation::InternalRenderingIntent,
                                           KoColorConversionTransformation::InternalConversionFlags) : QImage();
}

void KisMask::testingInitSelection(const QRect &rect)
{
    m_d->selection = new KisSelection();
    m_d->selection->pixelSelection()->select(rect, OPACITY_OPAQUE_U8);
    m_d->selection->updateProjection(rect);
    m_d->selection->setParentNode(this);
}

#include "kis_mask.moc"
