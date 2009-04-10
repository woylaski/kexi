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

#include "KoCtlColorSpaceInfoTest.h"

#include <qtest_kde.h>
#include "../KoCtlColorSpaceInfo.h"

void KoCtlColorSpaceInfoTest::testCreation()
{
    KoCtlColorSpaceInfo info(FILES_DATA_DIR + QString("rgba32f.ctlcs"));
    QVERIFY(info.load());
    QCOMPARE(info.colorDepthId(), QString( "F32"));
    QCOMPARE(info.colorModelId(), QString( "RGBA"));
    QCOMPARE(info.colorSpaceId(), QString( "RgbAF32"));
    QCOMPARE(info.name(), QString( "RGB (32-bit float/channel) for High Dynamic Range imaging"));
    QCOMPARE(info.defaultProfile(), QString( "Standard Linear RGB (scRGB/sRGB64)"));
    QVERIFY(info.isHdr());
    
    QCOMPARE(info.channels().size(), 4);
    const KoCtlColorSpaceInfo::ChannelInfo* redChannel = info.channels()[0];
    QCOMPARE(redChannel->name(), QString("Red"));
    QCOMPARE(redChannel->shortName(), QString("r"));
    QCOMPARE(redChannel->index(), 2 );
    QCOMPARE(redChannel->position(), 8 );
    QCOMPARE(redChannel->channelType(), KoChannelInfo::COLOR);
    QCOMPARE(redChannel->valueType(), KoChannelInfo::FLOAT32);
    QCOMPARE(redChannel->size(), 4);
    QCOMPARE(redChannel->color(), QColor(255,0,0));
    
    const KoCtlColorSpaceInfo::ChannelInfo* alphaChannel = info.channels()[3];
    QCOMPARE(alphaChannel->name(), QString("Alpha"));
    QCOMPARE(alphaChannel->shortName(), QString("a"));
    QCOMPARE(alphaChannel->index(), 3 );
    QCOMPARE(alphaChannel->position(), 12 );
    QCOMPARE(alphaChannel->channelType(), KoChannelInfo::ALPHA);
    QCOMPARE(alphaChannel->valueType(), KoChannelInfo::FLOAT32);
    QCOMPARE(alphaChannel->size(), 4);
    QCOMPARE(alphaChannel->color(), QColor(0,0,0));
}



QTEST_KDEMAIN(KoCtlColorSpaceInfoTest, NoGUI)
#include "KoCtlColorSpaceInfoTest.moc"
