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

#include "kis_oiio_import.h"

#include <QCheckBox>
#include <QSlider>

#include <kio/netaccess.h>

#include <kpluginfactory.h>

#include <KoColorSpace.h>
#include <KisFilterChain.h>
#include <KoColorSpaceRegistry.h>
#include <KoCompositeOp.h>
#include <KoColorSpaceTraits.h>
#include <KoColorModelStandardIds.h>

#include <kis_transaction.h>
#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_node.h>
#include <kis_group_layer.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>

OIIO_NAMESPACE_USING

K_PLUGIN_FACTORY(KisOiioImportFactory, registerPlugin<KisOiioImport>();)
K_EXPORT_PLUGIN(KisOiioImportFactory("calligrafilters"))

KisOiioImport::KisOiioImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisOiioImport::~KisOiioImport()
{
}

KisImportExportFilter::ConversionStatus KisOiioImport::convert(const QByteArray& from, const QByteArray& to)
{
    dbgFile << "Oiio import! From:" << from << ", To:" << to << 0;

    if (!(from == "image/oiio" || from == "image/x-xpixmap" || from == "image/gif" || from == "image/x-xbitmap"))
        return KisImportExportFilter::NotImplemented;

    if (to != "application/x-krita")
        return KisImportExportFilter::BadMimeType;

        KisDocument * doc = dynamic_cast<KisDocument*>(m_chain -> outputDocument());

    if (!doc)
        return KisImportExportFilter::NoDocumentCreated;

    QString filename = m_chain -> inputFile();

    doc->prepareForImport();

    if (!filename.isEmpty()) {
        QUrl url(filename);

        if (url.isEmpty())
            return KisImportExportFilter::FileNotFound;

        if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, qApp->activeWindow())) {
            return KisImportExportFilter::FileNotFound;
        }


        QString localFile = url.toLocalFile();

        ImageBuf buf(localFile::toStdString());
        int nSubImages = buf.nsubimages();


//        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(),
//                                                                                      Float32BitsColorDepthID.id(),
//                                                                                      "");


//        KisImageSP image = new KisImage(doc->createUndoStore(), img.width(), img.height(), colorSpace, localFile);

//        KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);
//        layer->paintDevice()->convertFromQImage(img, 0, 0, 0);
//        image->addNode(layer.data(), image->rootLayer().data());

//        doc->setCurrentImage(image);
        return KisImportExportFilter::OK;
    }
    return KisImportExportFilter::StorageCreationError;

}

#include "kis_oiio_import.moc"

