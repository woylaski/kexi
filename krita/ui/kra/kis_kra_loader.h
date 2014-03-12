/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2007
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KIS_KRA_LOADER_H
#define KIS_KRA_LOADER_H

class QString;

#include "KoXmlReaderForward.h"
class KoStore;

class KisDoc2;
class KisNode;
class KoColorSpace;
class KisPaintingAssistant;

#include <kis_types.h>
#include <krita_export.h>
/**
 * Load old-style 1.x .kra files. Updated for 2.0, let's try to stay
 * compatible. But 2.0 won't be able to save 1.x .kra files unless we
 * implement an export filter.
 */
class KRITAUI_EXPORT KisKraLoader
{

public:

    KisKraLoader(KisDoc2 * document, int syntaxVersion);

    ~KisKraLoader();

    /**
     * Loading is done in two steps: first all xml is loaded, then, in finishLoading,
     * the actual layer data is loaded.
     */
    KisImageWSP loadXML(const KoXmlElement& elem);

    void loadBinaryData(KoStore* store, KisImageWSP image, const QString & uri, bool external);

    vKisNodeSP selectedNodes() const;

    // it's neater to follow the same design as with selectedNodes, so let's have a getter here
    QList<KisPaintingAssistant*> assistants() const;

private:

    // this needs to be private, for neatness sake
    void loadAssistants(KoStore* store, const QString & uri, bool external);

    KisNodeSP loadNodes(const KoXmlElement& element, KisImageWSP image, KisNodeSP parent);

    KisNodeSP loadNode(const KoXmlElement& elem, KisImageWSP image, KisNodeSP parent);

    KisNodeSP loadPaintLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadGroupLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadAdjustmentLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadShapeLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadGeneratorLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadCloneLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, const KoColorSpace* cs, quint32 opacity);

    KisNodeSP loadFilterMask(const KoXmlElement& elem, KisNodeSP parent);

    KisNodeSP loadTransparencyMask(const KoXmlElement& elem, KisNodeSP parent);

    KisNodeSP loadSelectionMask(KisImageWSP image, const KoXmlElement& elem, KisNodeSP parent);

    KisNodeSP loadFileLayer(const KoXmlElement& elem, KisImageWSP image, const QString& name, quint32 opacity);

    void loadCompositions(const KoXmlElement& elem, KisImageWSP image);

    void loadAssistantsList(const KoXmlElement& elem);
private:

    struct Private;
    Private * const m_d;

};

#endif
