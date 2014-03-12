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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_tga_export.h"

#include <QCheckBox>
#include <QSlider>

#include <kapplication.h>
#include <kdialog.h>
#include <kpluginfactory.h>

#include <KoFilterChain.h>

#include <kis_paint_device.h>
#include <kis_doc2.h>
#include <kis_image.h>
#include <kis_paint_layer.h>

#include "tga.h"

K_PLUGIN_FACTORY(KisTGAExportFactory, registerPlugin<KisTGAExport>();)
K_EXPORT_PLUGIN(KisTGAExportFactory("calligrafilters"))

KisTGAExport::KisTGAExport(QObject *parent, const QVariantList &) : KoFilter(parent)
{
}

KisTGAExport::~KisTGAExport()
{
}

KoFilter::ConversionStatus KisTGAExport::convert(const QByteArray& from, const QByteArray& to)
{
    dbgFile << "TGA export! From:" << from << ", To:" << to << "";

    KisDoc2 *input = dynamic_cast<KisDoc2*>(m_chain->inputDocument());
    QString filename = m_chain->outputFile();

    if (!input)
        return KoFilter::NoDocumentCreated;

    if (filename.isEmpty()) return KoFilter::FileNotFound;

    if (from != "application/x-krita")
        return KoFilter::NotImplemented;

    KUrl url;
    url.setPath(filename);

    qApp->processEvents(); // For vector layers to be updated
    input->image()->waitForDone();

    QRect rc = input->image()->bounds();
    input->image()->refreshGraph();
    input->image()->lock();
    QImage image = input->image()->projection()->convertToQImage(0, 0, 0, rc.width(), rc.height(), KoColorConversionTransformation::InternalRenderingIntent, KoColorConversionTransformation::InternalConversionFlags);
    input->image()->unlock();

    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setByteOrder(QDataStream::LittleEndian);

    const QImage& img = image;
    const bool hasAlpha = (img.format() == QImage::Format_ARGB32);
    for (int i = 0; i < 12; i++)
        s << targaMagic[i];

    // write header
    s << quint16(img.width());   // width
    s << quint16(img.height());   // height
    s << quint8(hasAlpha ? 32 : 24);   // depth (24 bit RGB + 8 bit alpha)
    s << quint8(hasAlpha ? 0x24 : 0x20);   // top left image (0x20) + 8 bit alpha (0x4)

    for (int y = 0; y < img.height(); y++) {
        for (int x = 0; x < img.width(); x++) {
            const QRgb color = img.pixel(x, y);
            s << quint8(qBlue(color));
            s << quint8(qGreen(color));
            s << quint8(qRed(color));
            if (hasAlpha)
                s << quint8(qAlpha(color));
        }
    }

    return KoFilter::OK;
}

#include "kis_tga_export.moc"

