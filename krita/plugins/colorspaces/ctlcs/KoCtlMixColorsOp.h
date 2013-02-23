/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KO_CTL_MIX_COLORS_OP_H_
#define _KO_CTL_MIX_COLORS_OP_H_

#include "KoColorSpace.h"

class KoCtlColorSpace;
class KoCtlAccumulator;
class KoCtlColorSpaceInfo;

class KoCtlMixColorsOp : public KoMixColorsOp
{
public:
    KoCtlMixColorsOp(const KoCtlColorSpace* _colorSpace, const KoCtlColorSpaceInfo* _info);
    virtual ~KoCtlMixColorsOp();
    virtual void mixColors(const quint8 * const* colors, const qint16 *weights, quint32 nColors, quint8 *dst) const;
private:
    QList<KoCtlAccumulator*> m_accumulators;
    const KoCtlColorSpace* m_colorSpace;
};


#endif