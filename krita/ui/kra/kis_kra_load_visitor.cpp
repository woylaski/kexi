/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
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

#include "kra/kis_kra_load_visitor.h"
#include "kis_kra_tags.h"
#include "flake/kis_shape_layer.h"

#include <QRect>
#include <QBuffer>
#include <QByteArray>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoStore.h>
#include <KoColorSpace.h>

// kritaimage
#include <metadata/kis_meta_data_io_backend.h>
#include <metadata/kis_meta_data_store.h>
#include <kis_types.h>
#include <kis_node_visitor.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_layer.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_adjustment_layer.h>
#include <filter/kis_filter_configuration.h>
#include <kis_datamanager.h>
#include <generator/kis_generator_layer.h>
#include <kis_pixel_selection.h>
#include <kis_clone_layer.h>
#include <kis_filter_mask.h>
#include <kis_transparency_mask.h>
#include <kis_selection_mask.h>
#include "kis_shape_selection.h"



using namespace KRA;

KisKraLoadVisitor::KisKraLoadVisitor(KisImageWSP image,
                                     KoStore *store,
                                     QMap<KisNode *, QString> &layerFilenames,
                                     const QString & name,
                                     int syntaxVersion) :
        KisNodeVisitor(),
        m_layerFilenames(layerFilenames)
{
    m_external = false;
    m_image = image;
    m_store = store;
    m_name = name;
    m_syntaxVersion = syntaxVersion;
}

void KisKraLoadVisitor::setExternalUri(const QString &uri)
{
    m_external = true;
    m_uri = uri;
}

bool KisKraLoadVisitor::visit(KisExternalLayer * layer)
{
    bool result = false;

    if (KisShapeLayer* shapeLayer = dynamic_cast<KisShapeLayer*>(layer)) {

        if (!loadMetaData(layer)) {
            return false;
        }

        m_store->pushDirectory();
        m_store->enterDirectory(getLocation(layer, DOT_SHAPE_LAYER)) ;
        result =  shapeLayer->loadLayer(m_store);
        m_store->popDirectory();

    }

    result = visitAll(layer) && result;
    return result;
}

bool KisKraLoadVisitor::visit(KisPaintLayer *layer)
{
    dbgFile << "Visit: " << layer->name() << " colorSpace: " << layer->colorSpace()->id();
    if (!loadPaintDevice(layer->paintDevice(), getLocation(layer))) {
        return false;
    }
    if (!loadProfile(layer->paintDevice(), getLocation(layer, DOT_ICC))) {
        return false;
    }
    if (!loadMetaData(layer)) {
        return false;
    }

    if (m_syntaxVersion == 1) {
        // Check whether there is a file with a .mask extension in the
        // layer directory, if so, it's an old-style transparency mask
        // that should be converted.
        QString location = getLocation(layer, ".mask");

        if (m_store->open(location)) {

            KisSelectionSP selection = KisSelectionSP(new KisSelection());
            KisPixelSelectionSP pixelSelection = selection->pixelSelection();
            if (!pixelSelection->read(m_store->device())) {
                pixelSelection->disconnect();
            } else {
                KisTransparencyMask* mask = new KisTransparencyMask();
                mask->setSelection(selection);
                m_image->addNode(mask, layer, layer->firstChild());
            }
            m_store->close();
        }
    }
    bool result = visitAll(layer);
    return result;
}

bool KisKraLoadVisitor::visit(KisGroupLayer *layer)
{
    if (!loadMetaData(layer)) {
        return false;
    }

    bool result = visitAll(layer);
    return result;
}

bool KisKraLoadVisitor::visit(KisAdjustmentLayer* layer)
{
    // Adjustmentlayers are tricky: there's the 1.x style and the 2.x
    // style, which has selections with selection components

    if (m_syntaxVersion == 1) {
        QString location = getLocation(layer, ".selection");
        KisSelectionSP selection = new KisSelection();
        KisPixelSelectionSP pixelSelection = selection->pixelSelection();
        loadPaintDevice(pixelSelection, getLocation(layer, ".selection"));
        layer->setInternalSelection(selection);
    } else if (m_syntaxVersion == 2) {
        loadSelection(getLocation(layer), layer->internalSelection());

    } else {
        // We use the default, empty selection
    }

    if (!loadMetaData(layer)) {
        return false;
    }

    loadFilterConfiguration(layer->filter().data(), getLocation(layer, DOT_FILTERCONFIG));

    bool result = visitAll(layer);
    return result;
}

bool KisKraLoadVisitor::visit(KisGeneratorLayer* layer)
{
//     if (!loadPaintDevice(layer->paintDevice(), getLocation(layer))) {
//         return false;
//     }
//
//     if (!loadProfile(layer->paintDevice(), getLocation(layer, DOT_ICC))) {
//         return false;
//     }

    if (!loadMetaData(layer)) {
        return false;
    }

    loadSelection(getLocation(layer), layer->internalSelection());

    loadFilterConfiguration(layer->filter().data(), getLocation(layer, DOT_FILTERCONFIG));

    layer->update();

    bool result = visitAll(layer);
    return result;
}

bool KisKraLoadVisitor::visit(KisCloneLayer *layer)
{
    if (!loadMetaData(layer)) {
        return false;
    }

    // the layer might have already been lazily initialized
    // from the mask loading code
    if (layer->copyFrom()) {
        return true;
    }

    KisNodeSP srcNode = layer->copyFromInfo().findNode(m_image->rootLayer());
    KisLayerSP srcLayer = dynamic_cast<KisLayer*>(srcNode.data());
    Q_ASSERT(srcLayer);

    layer->setCopyFrom(srcLayer);

    // Clone layers have no data except for their masks
    bool result = visitAll(layer);
    return result;
}

void KisKraLoadVisitor::initSelectionForMask(KisMask *mask)
{
    KisLayer *cloneLayer = dynamic_cast<KisCloneLayer*>(mask->parent().data());
    if (cloneLayer) {
        // the clone layers should be initialized out of order
        // and lazily, because their original() is still not
        // initialized
        cloneLayer->accept(*this);
    }

    KisLayer *parentLayer = dynamic_cast<KisLayer*>(mask->parent().data());
    // the KisKraLoader must have already set the parent for us
    Q_ASSERT(parentLayer);
    mask->initSelection(parentLayer);
}

bool KisKraLoadVisitor::visit(KisFilterMask *mask)
{
    initSelectionForMask(mask);
    loadSelection(getLocation(mask), mask->selection());
    loadFilterConfiguration(mask->filter().data(), getLocation(mask, DOT_FILTERCONFIG));
    return true;
}

bool KisKraLoadVisitor::visit(KisTransparencyMask *mask)
{
    initSelectionForMask(mask);
    loadSelection(getLocation(mask), mask->selection());
    return true;
}

bool KisKraLoadVisitor::visit(KisSelectionMask *mask)
{
    initSelectionForMask(mask);
    loadSelection(getLocation(mask), mask->selection());
    return true;
}

bool KisKraLoadVisitor::loadPaintDevice(KisPaintDeviceSP device, const QString& location)
{
    // Layer data
    if (m_store->open(location)) {
        if (!device->read(m_store->device())) {
            device->disconnect();
            m_store->close();
            return false;
        }

        m_store->close();
    } else {
        kError() << "No image data: that's an error! " << device << ", " << location;
        return false;
    }
    if (m_store->open(location + ".defaultpixel")) {
        int pixelSize = device->colorSpace()->pixelSize();
        if (m_store->size() == pixelSize) {
            quint8 *defPixel = new quint8[pixelSize];
            m_store->read((char*)defPixel, pixelSize);
            device->setDefaultPixel(defPixel);
            delete[] defPixel;
        }
        m_store->close();
    }
    return true;
}


bool KisKraLoadVisitor::loadProfile(KisPaintDeviceSP device, const QString& location)
{

    if (m_store->hasFile(location)) {
        m_store->open(location);
        QByteArray data; data.resize(m_store->size());
        dbgFile << "Data to load: " << m_store->size() << " from " << location << " with color space " << device->colorSpace()->id();
        int read = m_store->read(data.data(), m_store->size());
        dbgFile << "Profile size: " << data.size() << " " << m_store->atEnd() << " " << m_store->device()->bytesAvailable() << " " << read;
        m_store->close();
        // Create a colorspace with the embedded profile
        const KoColorProfile *profile = KoColorSpaceRegistry::instance()->createColorProfile(device->colorSpace()->colorModelId().id(), device->colorSpace()->colorDepthId().id(), data);
        const KoColorSpace *cs =
            KoColorSpaceRegistry::instance()->colorSpace(device->colorSpace()->colorModelId().id(), device->colorSpace()->colorDepthId().id(), profile);
        // replace the old colorspace
        if (cs) {
            device->setDataManager(device->dataManager(), cs);
            return true;
        }
    }
    return false;
}

bool KisKraLoadVisitor::loadFilterConfiguration(KisFilterConfiguration* kfc, const QString& location)
{
    if (m_store->hasFile(location)) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        if (!data.isEmpty()) {
            QString xml(data);
            QDomDocument doc;
            doc.setContent(data);
            QDomElement e = doc.documentElement();
            if (e.tagName() == "filterconfig") {
                kfc->fromLegacyXML(e);
            } else {
                kfc->fromXML(e);
            }
            return true;
        }
    }
    return false;
}

bool KisKraLoadVisitor::loadMetaData(KisNode* node)
{
    dbgFile << "Load metadata for " << node->name();
    KisLayer* layer = qobject_cast<KisLayer*>(node);
    if (!layer) return true;

    bool result = true;

    KisMetaData::IOBackend* backend = KisMetaData::IOBackendRegistry::instance()->get("xmp");

    if (!backend->supportLoading()) {
        dbgFile << "Backend " << backend->id() << " does not support loading.";
        return false;
    }

    QString location = getLocation(node, QString(".") + backend->id() +  DOT_METADATA);
    dbgFile << "going to load " << backend->id() << ", " << backend->name() << " from " << location;

    if (m_store->hasFile(location)) {
        QByteArray data;
        m_store->open(location);
        data = m_store->read(m_store->size());
        m_store->close();
        QBuffer buffer(&data);
        if (!backend->loadFrom(layer->metaData(), &buffer)) {
            dbgFile << "\terror while loading metadata";
            result = false;
        }

    }

    return result;
}

void KisKraLoadVisitor::loadSelection(const QString& location, KisSelectionSP dstSelection)
{
    // Pixel selection
    QString pixelSelectionLocation = location + DOT_PIXEL_SELECTION;
    if (m_store->hasFile(pixelSelectionLocation)) {
        KisPixelSelectionSP pixelSelection = dstSelection->pixelSelection();
        loadPaintDevice(pixelSelection, pixelSelectionLocation);
    }

    // Shape selection
    QString shapeSelectionLocation = location + DOT_SHAPE_SELECTION;
    if (m_store->hasFile(shapeSelectionLocation + "/content.xml")) {
        m_store->pushDirectory();
        m_store->enterDirectory(shapeSelectionLocation) ;

        KisShapeSelection* shapeSelection = new KisShapeSelection(m_image, dstSelection);
        dstSelection->setShapeSelection(shapeSelection);
        shapeSelection->loadSelection(m_store);
        m_store->popDirectory();
    }
}

QString KisKraLoadVisitor::getLocation(KisNode* node, const QString& suffix)
{
    QString location = m_external ? QString() : m_uri;
    location += m_name + LAYER_PATH + m_layerFilenames[node] + suffix;
    return location;
}
