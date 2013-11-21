/*
 *  Copyright (c) 2005-2007 Adrian Page <adrian@pagenet.plus.com>
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

#include "opengl/kis_opengl_image_textures.h"

#ifdef HAVE_OPENGL
#include <QGLWidget>

#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>

#include "kis_image.h"
#include "kis_config.h"

#ifdef HAVE_OPENEXR
#include <half.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif


KisOpenGLImageTextures::ImageTexturesMap KisOpenGLImageTextures::imageTexturesMap;

KisOpenGLImageTextures::KisOpenGLImageTextures()
    : m_image(0)
    , m_monitorProfile(0)
    , m_checkerTexture(0)
    , m_allChannelsSelected(true)
{
    KisConfig cfg;
    m_renderingIntent = (KoColorConversionTransformation::Intent)cfg.renderIntent();

    m_conversionFlags = KoColorConversionTransformation::HighQuality;
    if (cfg.useBlackPointCompensation()) m_conversionFlags |= KoColorConversionTransformation::BlackpointCompensation;
    if (!cfg.allowLCMSOptimization()) m_conversionFlags |= KoColorConversionTransformation::NoOptimization;
}

KisOpenGLImageTextures::KisOpenGLImageTextures(KisImageWSP image,
                                               KoColorProfile *monitorProfile,
                                               KoColorConversionTransformation::Intent renderingIntent,
                                               KoColorConversionTransformation::ConversionFlags conversionFlags)
    : m_image(image)
    , m_monitorProfile(monitorProfile)
    , m_renderingIntent(renderingIntent)
    , m_conversionFlags(conversionFlags)
    , m_checkerTexture(0)
{
    Q_ASSERT(renderingIntent < 4);

    KisOpenGL::makeContextCurrent();

    getTextureSize(&m_texturesInfo);

    glGenTextures(1, &m_checkerTexture);
    createImageTextureTiles();

    KisOpenGLUpdateInfoSP info = updateCache(m_image->bounds());
    recalculateCache(info);
}

KisOpenGLImageTextures::~KisOpenGLImageTextures()
{
    ImageTexturesMap::iterator it = imageTexturesMap.find(m_image);
    if (it != imageTexturesMap.end()) {
        KisOpenGLImageTextures *textures = it.value();
        if (textures == this) {
            dbgUI << "Removing shared image context from map";
            imageTexturesMap.erase(it);
        }
    }

    destroyImageTextureTiles();
    glDeleteTextures(1, &m_checkerTexture);
}

bool KisOpenGLImageTextures::imageCanShareTextures()
{
    KisConfig cfg;
    return !cfg.useOcio();
}

KisOpenGLImageTexturesSP KisOpenGLImageTextures::getImageTextures(KisImageWSP image,
                                                                  KoColorProfile *monitorProfile,
                                                                  KoColorConversionTransformation::Intent renderingIntent,
                                                                  KoColorConversionTransformation::ConversionFlags conversionFlags)
{
    KisOpenGL::makeContextCurrent();

    if (imageCanShareTextures()) {
        ImageTexturesMap::iterator it = imageTexturesMap.find(image);

        if (it != imageTexturesMap.end()) {
            KisOpenGLImageTexturesSP textures = it.value();
            textures->setMonitorProfile(monitorProfile, renderingIntent, conversionFlags);

            return textures;
        } else {
            KisOpenGLImageTextures *imageTextures = new KisOpenGLImageTextures(image, monitorProfile, renderingIntent, conversionFlags);
            imageTexturesMap[image] = imageTextures;
            dbgUI << "Added shareable textures to map";

            return imageTextures;
        }
    } else {
        return new KisOpenGLImageTextures(image, monitorProfile, renderingIntent, conversionFlags);
    }
}

QRect KisOpenGLImageTextures::calculateTileRect(int col, int row) const
{
    return m_image->bounds() &
            QRect(col * m_texturesInfo.effectiveWidth,
                  row * m_texturesInfo.effectiveHeight,
                  m_texturesInfo.effectiveWidth,
                  m_texturesInfo.effectiveHeight);
}

void KisOpenGLImageTextures::createImageTextureTiles()
{
    KisOpenGL::makeContextCurrent();

    destroyImageTextureTiles();
    updateTextureFormat();

    m_storedImageBounds = m_image->bounds();
    const int lastCol = xToCol(m_image->width());
    const int lastRow = yToRow(m_image->height());

    m_numCols = lastCol + 1;

    // Default color is transparent black
    const int pixelSize = 4;
    QByteArray emptyTileData((m_texturesInfo.width) * (m_texturesInfo.height) * pixelSize, 0);

    KisConfig config;
    KisTextureTile::FilterMode mode = (KisTextureTile::FilterMode)config.openGLFilteringMode();

    for (int row = 0; row <= lastRow; row++) {
        for (int col = 0; col <= lastCol; col++) {
            QRect tileRect = calculateTileRect(col, row);

            KisTextureTile *tile = new KisTextureTile(tileRect,
                                                      &m_texturesInfo,
                                                      emptyTileData,
                                                      mode);
            m_textureTiles.append(tile);
        }
    }
}

void KisOpenGLImageTextures::destroyImageTextureTiles()
{
    if(m_textureTiles.isEmpty()) return;

    KisOpenGL::makeContextCurrent();
    foreach(KisTextureTile *tile, m_textureTiles) {
        delete tile;
    }
    m_textureTiles.clear();
    m_storedImageBounds = QRect();
}

KisOpenGLUpdateInfoSP KisOpenGLImageTextures::updateCache(const QRect& rect)
{
    KisOpenGLUpdateInfoSP info = new KisOpenGLUpdateInfo();

    QRect updateRect = rect & m_image->bounds();
    if (updateRect.isEmpty()) return info;

    /**
     * Why the rect is artificial? That's easy!
     * It does not represent any real piece of the image. It is
     * intentionally stretched to get through the overlappping
     * stripes of neutrality and poke neighbouring tiles.
     * Thanks to the rect we get the coordinates of all the tiles
     * involved into update process
     */

    QRect artificialRect = stretchRect(updateRect, m_texturesInfo.border);
    artificialRect &= m_image->bounds();

    int firstColumn = xToCol(artificialRect.left());
    int lastColumn = xToCol(artificialRect.right());
    int firstRow = yToRow(artificialRect.top());
    int lastRow = yToRow(artificialRect.bottom());


    KisConfig cfg;
    QBitArray channelFlags; // empty by default

    if (!cfg.useOcio() && !m_allChannelsSelected) {
        channelFlags = m_channelFlags;
    }

    qint32 numItems = (lastColumn - firstColumn + 1) * (lastRow - firstRow + 1);
    info->tileList.reserve(numItems);

    for (int col = firstColumn; col <= lastColumn; col++) {
        for (int row = firstRow; row <= lastRow; row++) {

            QRect tileRect = calculateTileRect(col, row);
            QRect tileTextureRect = stretchRect(tileRect, m_texturesInfo.border);

            KisTextureTileUpdateInfo tileInfo(col, row,
                                              tileTextureRect,
                                              updateRect,
                                              m_image->bounds());
            // Don't update empty tiles
            if (tileInfo.valid()) {
                tileInfo.retrieveData(m_image, channelFlags, m_onlyOneChannelSelected, m_selectedChannelIndex);
                info->tileList.append(tileInfo);
            }
            else {
                kWarning() << "Trying to create an empty tileinfo record" << col << row << tileTextureRect << updateRect << m_image->bounds();
            }
        }
    }
    return info;
}

void KisOpenGLImageTextures::recalculateCache(KisUpdateInfoSP info)
{
    KisOpenGLUpdateInfoSP glInfo = dynamic_cast<KisOpenGLUpdateInfo*>(info.data());
    if(!glInfo) return;

    KisOpenGL::makeContextCurrent();
    KIS_OPENGL_CLEAR_ERROR();

    KisTextureTileUpdateInfo tileInfo;
    foreach(tileInfo, glInfo->tileList) {
        const KoColorSpace *dstCS = 0;

        switch(m_texturesInfo.type) {
        case GL_UNSIGNED_BYTE:
            dstCS = KoColorSpaceRegistry::instance()->rgb8(m_monitorProfile);
            break;
        case GL_UNSIGNED_SHORT:
            dstCS = KoColorSpaceRegistry::instance()->rgb16(m_monitorProfile);
            break;
#if defined(HAVE_OPENEXR)
        case GL_HALF_FLOAT_ARB:
            dstCS = KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F16", 0);
            break;
#endif
        case GL_FLOAT:
            dstCS = KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F32", 0);
            break;
        default:
            qFatal("Unknown m_imageTextureType");
        }

        tileInfo.convertTo(dstCS, m_renderingIntent, m_conversionFlags);
        KisTextureTile *tile = getTextureTileCR(tileInfo.tileCol(), tileInfo.tileRow());
        KIS_ASSERT_RECOVER_RETURN(tile);

        tile->update(tileInfo);
        tileInfo.destroy();

        KIS_OPENGL_PRINT_ERROR();
    }
}

void KisOpenGLImageTextures::generateCheckerTexture(const QImage &checkImage)
{
    KisOpenGL::makeContextCurrent();

    glBindTexture(GL_TEXTURE_2D, m_checkerTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    QImage img = checkImage;
    if (checkImage.width() != BACKGROUND_TEXTURE_SIZE || checkImage.height() != BACKGROUND_TEXTURE_SIZE) {
        img = checkImage.scaled(BACKGROUND_TEXTURE_SIZE, BACKGROUND_TEXTURE_SIZE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BACKGROUND_TEXTURE_SIZE, BACKGROUND_TEXTURE_SIZE,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());

}

GLuint KisOpenGLImageTextures::checkerTexture() const
{
    return m_checkerTexture;
}

void KisOpenGLImageTextures::slotImageSizeChanged(qint32 /*w*/, qint32 /*h*/)
{
    createImageTextureTiles();
}

void KisOpenGLImageTextures::setMonitorProfile(const KoColorProfile *monitorProfile, KoColorConversionTransformation::Intent renderingIntent, KoColorConversionTransformation::ConversionFlags conversionFlags)
{
    Q_ASSERT(renderingIntent < 4);
    if (monitorProfile != m_monitorProfile ||
            renderingIntent != m_renderingIntent ||
            conversionFlags != m_conversionFlags) {

        m_monitorProfile = monitorProfile;
        m_renderingIntent = renderingIntent;
        m_conversionFlags = conversionFlags;
    }
}

void KisOpenGLImageTextures::setChannelFlags(const QBitArray &channelFlags)
{
    m_channelFlags = channelFlags;
    int selectedChannels = 0;
    const KoColorSpace *projectionCs = m_image->projection()->colorSpace();
    QList<KoChannelInfo*> channelInfo = projectionCs->channels();
    for (int i = 0; i < m_channelFlags.size(); ++i) {
        if (m_channelFlags.testBit(i) && channelInfo[i]->channelType() == KoChannelInfo::COLOR) {
            selectedChannels++;
            m_selectedChannelIndex = i;
        }
    }
    m_allChannelsSelected = (selectedChannels == m_channelFlags.size());
    m_onlyOneChannelSelected = (selectedChannels == 1);
}

void KisOpenGLImageTextures::getTextureSize(KisGLTexturesInfo *texturesInfo)
{
    KisConfig cfg;

    const GLint preferredTextureSize = cfg.openGLTextureSize();

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    texturesInfo->width = qMin(preferredTextureSize, maxTextureSize);
    texturesInfo->height = qMin(preferredTextureSize, maxTextureSize);

    texturesInfo->border = cfg.textureOverlapBorder();

    texturesInfo->effectiveWidth = texturesInfo->width - 2 * texturesInfo->border;
    texturesInfo->effectiveHeight = texturesInfo->height - 2 * texturesInfo->border;
}

void KisOpenGLImageTextures::updateTextureFormat()
{
    m_texturesInfo.internalFormat = GL_RGBA8;
    m_texturesInfo.type = GL_UNSIGNED_BYTE;
    m_texturesInfo.format = GL_BGRA;
#ifdef HAVE_GLEW

    KoID colorModelId = m_image->colorSpace()->colorModelId();
    KoID colorDepthId = m_image->colorSpace()->colorDepthId();

    dbgUI << "Choosing texture format:";

    if (colorModelId == RGBAColorModelID) {
        if (colorDepthId == Float16BitsColorDepthID) {

            if (GLEW_ARB_texture_float) {
                m_texturesInfo.internalFormat = GL_RGBA16F_ARB;
                dbgUI << "Using ARB half";
            }
            else if (GLEW_ATI_texture_float){
                m_texturesInfo.internalFormat = GL_RGBA_FLOAT16_ATI;
                dbgUI << "Using ATI half";
            }

            if (GLEW_ARB_half_float_pixel) {
                dbgUI << "Pixel type half";
                m_texturesInfo.type = GL_HALF_FLOAT_ARB;
            } else {
                dbgUI << "Pixel type float";
                m_texturesInfo.type = GL_FLOAT;
            }
            m_texturesInfo.format = GL_RGBA;
        }
        else if (colorDepthId == Float32BitsColorDepthID) {

            if (GLEW_ARB_texture_float) {
                m_texturesInfo.internalFormat = GL_RGBA32F_ARB;
                dbgUI << "Using ARB float";
                m_texturesInfo.type = GL_FLOAT;
            }
            else if (GLEW_ATI_texture_float) {
                m_texturesInfo.internalFormat = GL_RGBA_FLOAT32_ATI;
                dbgUI << "Using ATI float";
                m_texturesInfo.type = GL_FLOAT;
            }
            m_texturesInfo.format = GL_RGBA;
        }
        else if (colorDepthId == Integer16BitsColorDepthID) {
            dbgUI << "Using 16 bits rgba";
            m_texturesInfo.internalFormat = GL_RGBA16;
            m_texturesInfo.type = GL_UNSIGNED_SHORT;
        }
    }
    else {
        // We will convert the colorspace to 16 bits rgba, instead of 8 bits
        if (colorDepthId == Integer16BitsColorDepthID) {
            dbgUI << "Using conversion to 16 bits rgba";
            m_texturesInfo.internalFormat = GL_RGBA16;
            m_texturesInfo.type = GL_UNSIGNED_SHORT;
        }
    }
#endif
}

#include "kis_opengl_image_textures.moc"

#endif // HAVE_OPENGL

