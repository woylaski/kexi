/*
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_transaction_test.h"
#include <qtest_kde.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>

#include <kundo2qstack.h>

#include "kis_types.h"
#include "kis_transform_worker.h"
#include "kis_paint_device.h"
#include "kis_transaction.h"
#include "kis_surrogate_undo_adapter.h"
#include "kis_image.h"

void KisTransactionTest::testUndo()
{
    KisSurrogateUndoAdapter undoAdapter;
    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP dev = new KisPaintDevice(cs);

    quint8* pixel = new quint8[cs->pixelSize()];
    cs->fromQColor(Qt::white, pixel);
    dev->fill(0, 0, 512, 512, pixel);

    cs->fromQColor(Qt::black, pixel);
    dev->fill(512, 0, 512, 512, pixel);

    QColor c1, c2;
    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::white);
    QVERIFY(c2 == Qt::black);

    KisTransaction transaction("mirror", dev, 0);
    KisTransformWorker::mirrorX(dev);
    transaction.commit(&undoAdapter);

    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::black);
    QVERIFY(c2 == Qt::white);

    undoAdapter.undo();

    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::white);
    QVERIFY(c2 == Qt::black);

}

void KisTransactionTest::testRedo()
{
    KisSurrogateUndoAdapter undoAdapter;

    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP dev = new KisPaintDevice(cs);

    quint8* pixel = new quint8[cs->pixelSize()];
    cs->fromQColor(Qt::white, pixel);
    dev->fill(0, 0, 512, 512, pixel);

    cs->fromQColor(Qt::black, pixel);
    dev->fill(512, 0, 512, 512, pixel);

    QColor c1, c2;
    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::white);
    QVERIFY(c2 == Qt::black);

    KisTransaction transaction("mirror", dev, 0);
    KisTransformWorker::mirrorX(dev);
    transaction.commit(&undoAdapter);

    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::black);
    QVERIFY(c2 == Qt::white);


    undoAdapter.undo();

    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::white);
    QVERIFY(c2 == Qt::black);

    undoAdapter.redo();

    dev->pixel(5, 5, &c1);
    dev->pixel(517, 5, &c2);

    QVERIFY(c1 == Qt::black);
    QVERIFY(c2 == Qt::white);
}

void KisTransactionTest::testDeviceMove()
{
    KisSurrogateUndoAdapter undoAdapter;

    const KoColorSpace * cs = KoColorSpaceRegistry::instance()->rgb8();
    KisPaintDeviceSP dev = new KisPaintDevice(cs);

    QCOMPARE(dev->x(), 0);
    QCOMPARE(dev->y(), 0);

    KisTransaction t1("move1", dev, 0);
    dev->move(10,20);
    t1.commit(&undoAdapter);

    QCOMPARE(dev->x(), 10);
    QCOMPARE(dev->y(), 20);

    KisTransaction t2("move2", dev, 0);
    dev->move(7,11);
    t2.commit(&undoAdapter);

    QCOMPARE(dev->x(),  7);
    QCOMPARE(dev->y(), 11);

    undoAdapter.undo();

    QCOMPARE(dev->x(), 10);
    QCOMPARE(dev->y(), 20);

    undoAdapter.undo();

    QCOMPARE(dev->x(), 0);
    QCOMPARE(dev->y(), 0);

    undoAdapter.redo();

    QCOMPARE(dev->x(), 10);
    QCOMPARE(dev->y(), 20);

    undoAdapter.redo();

    QCOMPARE(dev->x(),  7);
    QCOMPARE(dev->y(), 11);
}

QTEST_KDEMAIN(KisTransactionTest, GUI)
#include "kis_transaction_test.moc"


