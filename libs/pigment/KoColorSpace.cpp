/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KoColorSpace.h"
#include "KoColorSpace_p.h"

#include "KoChannelInfo.h"
#include "DebugPigment.h"
#include "KoCompositeOp.h"
#include "KoColorTransformation.h"
#include "KoColorTransformationFactory.h"
#include "KoColorTransformationFactoryRegistry.h"
#include "KoColorConversionCache.h"
#include "KoColorConversionSystem.h"
#include "KoColorSpaceRegistry.h"
#include "KoColorProfile.h"
#include "KoCopyColorConversionTransformation.h"
#include "KoFallBackColorTransformation.h"
#include "KoUniqueNumberForIdServer.h"
#include "KoMixColorsOp.h"
#include "KoConvolutionOp.h"
#include "KoCompositeOpRegistry.h"

#include <QThreadStorage>
#include <QByteArray>
#include <QBitArray>


KoColorSpace::KoColorSpace()
        : d(new Private())
{
}

KoColorSpace::KoColorSpace(const QString &id, const QString &name, KoMixColorsOp* mixColorsOp, KoConvolutionOp* convolutionOp)
        : d(new Private())
{
    d->id = id;
    d->idNumber = KoUniqueNumberForIdServer::instance()->numberForId(d->id);
    d->name = name;
    d->mixColorsOp = mixColorsOp;
    d->convolutionOp = convolutionOp;
    d->transfoToRGBA16 = 0;
    d->transfoFromRGBA16 = 0;
    d->transfoToLABA16 = 0;
    d->transfoFromLABA16 = 0;
    d->deletability = NotOwnedByRegistry;
}

KoColorSpace::~KoColorSpace()
{
    Q_ASSERT(d->deletability != OwnedByRegistryDoNotDelete);

    qDeleteAll(d->compositeOps);
    foreach(KoChannelInfo * channel, d->channels) {
        delete channel;
    }
    if (d->deletability == NotOwnedByRegistry) {
        KoColorConversionCache* cache = KoColorSpaceRegistry::instance()->colorConversionCache();
        if (cache) {
            cache->colorSpaceIsDestroyed(this);
        }
    }
    delete d->mixColorsOp;
    delete d->convolutionOp;
    delete d->transfoToRGBA16;
    delete d->transfoFromRGBA16;
    delete d->transfoToLABA16;
    delete d->transfoFromLABA16;
    delete d;
}

bool KoColorSpace::operator==(const KoColorSpace& rhs) const
{
    const KoColorProfile* p1 = rhs.profile();
    const KoColorProfile* p2 = profile();
    return d->idNumber == rhs.d->idNumber && ((p1 == p2) || (*p1 == *p2));
}

QString KoColorSpace::id() const
{
    return d->id;
}

QString KoColorSpace::name() const
{
    return d->name;
}

QList<KoChannelInfo *> KoColorSpace::channels() const
{
    return d->channels;
}

QBitArray KoColorSpace::channelFlags(bool color, bool alpha) const
{
    QBitArray ba(d->channels.size());
    if (!color && !alpha) return ba;

    for (int i = 0; i < d->channels.size(); ++i) {
        KoChannelInfo * channel = d->channels.at(i);
        if ((color && channel->channelType() == KoChannelInfo::COLOR) ||
                (alpha && channel->channelType() == KoChannelInfo::ALPHA))
            ba.setBit(i, true);
    }
    return ba;
}

void KoColorSpace::addChannel(KoChannelInfo * ci)
{
    d->channels.push_back(ci);
}

quint8 *KoColorSpace::allocPixelBuffer(quint32 numPixels, bool clear, quint8 defaultvalue) const
{
    if (numPixels == 0) {
        return 0;
    }
    quint8* buf = new quint8[pixelSize()*numPixels];
    if (clear) {
        memset(buf, defaultvalue, pixelSize() * numPixels);
    }
    return buf;
}

bool KoColorSpace::hasCompositeOp(const QString& id) const
{
    return d->compositeOps.contains(id);
}

QList<KoCompositeOp*> KoColorSpace::compositeOps() const
{
    return d->compositeOps.values();
}


KoMixColorsOp* KoColorSpace::mixColorsOp() const
{
    return d->mixColorsOp;
}


KoConvolutionOp* KoColorSpace::convolutionOp() const
{
    return d->convolutionOp;
}

const KoCompositeOp * KoColorSpace::compositeOp(const QString & id) const
{
    if (d->compositeOps.contains(id))
        return d->compositeOps.value(id);
    else {
        warnPigment << "Asking for non-existent composite operation " << id << ", returning " << COMPOSITE_OVER;
        return d->compositeOps.value(COMPOSITE_OVER);
    }
}

void KoColorSpace::addCompositeOp(const KoCompositeOp * op)
{
    if (op->colorSpace()->id() == id()) {
        d->compositeOps.insert(op->id(), const_cast<KoCompositeOp*>(op));
    }
}

const KoColorConversionTransformation* KoColorSpace::toLabA16Converter() const
{
    if (!d->transfoToLABA16) {
        d->transfoToLABA16 = KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverter(this, KoColorSpaceRegistry::instance()->lab16(""), KoColorConversionTransformation::InternalRenderingIntent, KoColorConversionTransformation::InternalConversionFlags) ;
    }
    return d->transfoToLABA16;
}

const KoColorConversionTransformation* KoColorSpace::fromLabA16Converter() const
{
    if (!d->transfoFromLABA16) {
        d->transfoFromLABA16 = KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverter(KoColorSpaceRegistry::instance()->lab16(""), this, KoColorConversionTransformation::InternalRenderingIntent, KoColorConversionTransformation::InternalConversionFlags) ;
    }
    return d->transfoFromLABA16;
}
const KoColorConversionTransformation* KoColorSpace::toRgbA16Converter() const
{
    if (!d->transfoToRGBA16) {
        d->transfoToRGBA16 = KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverter(this, KoColorSpaceRegistry::instance()->rgb16(""), KoColorConversionTransformation::InternalRenderingIntent, KoColorConversionTransformation::InternalConversionFlags) ;
    }
    return d->transfoToRGBA16;
}
const KoColorConversionTransformation* KoColorSpace::fromRgbA16Converter() const
{
    if (!d->transfoFromRGBA16) {
        d->transfoFromRGBA16 = KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverter(KoColorSpaceRegistry::instance()->rgb16("") , this, KoColorConversionTransformation::InternalRenderingIntent, KoColorConversionTransformation::InternalConversionFlags) ;
    }
    return d->transfoFromRGBA16;
}

void KoColorSpace::toLabA16(const quint8 * src, quint8 * dst, quint32 nPixels) const
{
    toLabA16Converter()->transform(src, dst, nPixels);
}

void KoColorSpace::fromLabA16(const quint8 * src, quint8 * dst, quint32 nPixels) const
{
    fromLabA16Converter()->transform(src, dst, nPixels);
}

void KoColorSpace::toRgbA16(const quint8 * src, quint8 * dst, quint32 nPixels) const
{
    toRgbA16Converter()->transform(src, dst, nPixels);
}

void KoColorSpace::fromRgbA16(const quint8 * src, quint8 * dst, quint32 nPixels) const
{
    fromRgbA16Converter()->transform(src, dst, nPixels);
}

KoColorConversionTransformation* KoColorSpace::createColorConverter(const KoColorSpace * dstColorSpace, KoColorConversionTransformation::Intent renderingIntent, KoColorConversionTransformation::ConversionFlags conversionFlags) const
{
    if (*this == *dstColorSpace) {
        return new KoCopyColorConversionTransformation(this);
    } else {
        return KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverter(this, dstColorSpace, renderingIntent, conversionFlags);
    }
}

bool KoColorSpace::convertPixelsTo(const quint8 * src,
                                   quint8 * dst,
                                   const KoColorSpace * dstColorSpace,
                                   quint32 numPixels,
                                   KoColorConversionTransformation::Intent renderingIntent,
                                   KoColorConversionTransformation::ConversionFlags conversionFlags) const
{
    if (*this == *dstColorSpace) {
        if (src != dst) {
            memcpy(dst, src, numPixels * sizeof(quint8) * pixelSize());
        }
    } else {
        KoCachedColorConversionTransformation cct = KoColorSpaceRegistry::instance()->colorConversionCache()->cachedConverter(this, dstColorSpace, renderingIntent, conversionFlags);
        cct.transformation()->transform(src, dst, numPixels);
    }
    return true;
}


void KoColorSpace::bitBlt(const KoColorSpace* srcSpace, const KoCompositeOp::ParameterInfo& params, const KoCompositeOp* op,
                          KoColorConversionTransformation::Intent renderingIntent,
                          KoColorConversionTransformation::ConversionFlags conversionFlags) const
{
    Q_ASSERT_X(*op->colorSpace() == *this, "KoColorSpace::bitBlt", QString("Composite op is for color space %1 (%2) while this is %3 (%4)").arg(op->colorSpace()->id()).arg(op->colorSpace()->profile()->name()).arg(id()).arg(profile()->name()).toLatin1());

    if(params.rows <= 0 || params.cols <= 0)
        return;

    if(!(*this == *srcSpace)) {
         if (preferCompositionInSourceColorSpace() &&
             srcSpace->hasCompositeOp(op->id())) {

             quint32           conversionDstBufferStride = params.cols * srcSpace->pixelSize();
             QVector<quint8> * conversionDstCache        = threadLocalConversionCache(params.rows * conversionDstBufferStride);
             quint8*           conversionDstData         = conversionDstCache->data();

             for(qint32 row=0; row<params.rows; row++) {
                 convertPixelsTo(params.dstRowStart + row * params.dstRowStride,
                                 conversionDstData  + row * conversionDstBufferStride, srcSpace, params.cols,
                                 renderingIntent, conversionFlags);
             }

             // FIXME: do not calculate the otherOp every time
             const KoCompositeOp *otherOp = srcSpace->compositeOp(op->id());

             KoCompositeOp::ParameterInfo paramInfo(params);
             paramInfo.dstRowStart  = conversionDstData;
             paramInfo.dstRowStride = conversionDstBufferStride;
             otherOp->composite(paramInfo);

             for(qint32 row=0; row<params.rows; row++) {
                 srcSpace->convertPixelsTo(conversionDstData  + row * conversionDstBufferStride,
                                           params.dstRowStart + row * params.dstRowStride, this, params.cols,
                                           renderingIntent, conversionFlags);
             }

        } else {
            quint32           conversionBufferStride = params.cols * pixelSize();
            QVector<quint8> * conversionCache        = threadLocalConversionCache(params.rows * conversionBufferStride);
            quint8*           conversionData         = conversionCache->data();

            for(qint32 row=0; row<params.rows; row++) {
                srcSpace->convertPixelsTo(params.srcRowStart + row * params.srcRowStride,
                                          conversionData     + row * conversionBufferStride, this, params.cols,
                                          renderingIntent, conversionFlags);
            }

            KoCompositeOp::ParameterInfo paramInfo(params);
            paramInfo.srcRowStart  = conversionData;
            paramInfo.srcRowStride = conversionBufferStride;
            op->composite(paramInfo);
        }
    }
    else {
        op->composite(params);
    }
}


QVector<quint8> * KoColorSpace::threadLocalConversionCache(quint32 size) const
{
    QVector<quint8> * ba = 0;
    if (!d->conversionCache.hasLocalData()) {
        ba = new QVector<quint8>(size, '0');
        d->conversionCache.setLocalData(ba);
    } else {
        ba = d->conversionCache.localData();
        if ((quint8)ba->size() < size)
            ba->resize(size);
    }
    return ba;
}

KoColorTransformation* KoColorSpace::createColorTransformation(const QString & id, const QHash<QString, QVariant> & parameters) const
{
    KoColorTransformationFactory* factory = KoColorTransformationFactoryRegistry::instance()->get(id);
    if (!factory) return 0;
    QPair<KoID, KoID> model(colorModelId(), colorDepthId());
    QList< QPair<KoID, KoID> > models = factory->supportedModels();
    if (models.isEmpty() || models.contains(model)) {
        return factory->createTransformation(this, parameters);
    } else {
        // Find the best solution
        // TODO use the color conversion cache
        KoColorConversionTransformation* csToFallBack = 0;
        KoColorConversionTransformation* fallBackToCs = 0;
        KoColorSpaceRegistry::instance()->colorConversionSystem()->createColorConverters(this, models, csToFallBack, fallBackToCs);
        Q_ASSERT(csToFallBack);
        Q_ASSERT(fallBackToCs);
        KoColorTransformation* transfo = factory->createTransformation(fallBackToCs->srcColorSpace(), parameters);
        return new KoFallBackColorTransformation(csToFallBack, fallBackToCs, transfo);
    }
}

QImage KoColorSpace::convertToQImage(const quint8 *data, qint32 width, qint32 height,
                                     const KoColorProfile *dstProfile,
                                     KoColorConversionTransformation::Intent renderingIntent,
                                     KoColorConversionTransformation::ConversionFlags conversionFlags) const

{
    QImage img = QImage(width, height, QImage::Format_ARGB32);

    const KoColorSpace * dstCS = KoColorSpaceRegistry::instance()->rgb8(dstProfile);

    if (data)
        this->convertPixelsTo(const_cast<quint8 *>(data), img.bits(), dstCS, width * height, renderingIntent, conversionFlags);

    return img;
}

bool KoColorSpace::preferCompositionInSourceColorSpace() const
{
    return false;
}
