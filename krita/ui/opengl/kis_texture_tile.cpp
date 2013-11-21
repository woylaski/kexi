/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#define GL_GLEXT_PROTOTYPES
#include "kis_texture_tile.h"

#ifdef HAVE_OPENGL

#include <kis_config.h>

#ifndef GL_BGRA
#define GL_BGRA 0x814F
#endif

inline QRectF relativeRect(const QRect &br /* baseRect */,
                           const QRect &cr /* childRect */,
                           const KisGLTexturesInfo *texturesInfo)
{
    const qreal x = qreal(cr.x() - br.x()) / texturesInfo->width;
    const qreal y = qreal(cr.y() - br.y()) / texturesInfo->height;
    const qreal w = qreal(cr.width()) / texturesInfo->width;
    const qreal h = qreal(cr.height()) / texturesInfo->height;

    return QRectF(x, y, w, h);
}

void setTextureParameters()
{
    const int numMipmapLevels = KisConfig().numMipmapLevels();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, numMipmapLevels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipmapLevels);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

KisTextureTile::KisTextureTile(QRect imageRect, const KisGLTexturesInfo *texturesInfo,
                               const QByteArray &fillData, FilterMode filter)

    : m_textureId(0)
#ifdef USE_PIXEL_BUFFERS
    , m_glBuffer(0)
#endif
    , m_tileRectInImagePixels(imageRect)
    , m_filter(filter)
    , m_texturesInfo(texturesInfo)
    , m_needsMipmapRegeneration(false)
{
    const GLvoid *fd = fillData.constData();

    m_textureRectInImagePixels =
            stretchRect(m_tileRectInImagePixels, texturesInfo->border);

    m_tileRectInTexturePixels = relativeRect(m_textureRectInImagePixels,
                                             m_tileRectInImagePixels,
                                             m_texturesInfo);

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    setTextureParameters();

#ifdef USE_PIXEL_BUFFERS
    createTextureBuffer(fillData);
    // we set fill data to 0 so the next glTexImage2D call uses our buffer
    fd = 0;
#endif

    glTexImage2D(GL_TEXTURE_2D, 0,
                 m_texturesInfo->internalFormat,
                 m_texturesInfo->width,
                 m_texturesInfo->height, 0,
                 m_texturesInfo->format,
                 m_texturesInfo->type, fd);

#ifdef USE_PIXEL_BUFFERS
    KisConfig cfg;
    if (cfg.useOpenGLTextureBuffer()) {
        m_glBuffer->release();
    }
#endif

    setNeedsMipmapRegeneration();
}

KisTextureTile::~KisTextureTile()
{
#ifdef USE_PIXEL_BUFFERS
    KisConfig cfg;
    if (cfg.useOpenGLTextureBuffer()) {
        delete m_glBuffer;
    }
#endif
    glDeleteTextures(1, &m_textureId);
}

void KisTextureTile::bindToActiveTexture()
{
    glBindTexture(GL_TEXTURE_2D, textureId());

    if (m_needsMipmapRegeneration) {
        glGenerateMipmap(GL_TEXTURE_2D);
        m_needsMipmapRegeneration = false;
    }
}

void KisTextureTile::setNeedsMipmapRegeneration()
{
    if (m_filter == TrilinearFilterMode ||
        m_filter == HighQualityFiltering) {

        m_needsMipmapRegeneration = true;
    }
}

void KisTextureTile::update(const KisTextureTileUpdateInfo &updateInfo)
{
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    setTextureParameters();

    const GLvoid *fd = updateInfo.data();
#ifdef USE_PIXEL_BUFFERS
    KisConfig cfg;
    if (!m_glBuffer) {
        QByteArray ba;
        ba.fromRawData((const char*)updateInfo.data(), updateInfo.patchPixelsLength());
        createTextureBuffer(ba);
    }
#endif

    if (updateInfo.isEntireTileUpdated()) {

#ifdef USE_PIXEL_BUFFERS

        if (cfg.useOpenGLTextureBuffer()) {

            m_glBuffer->bind();
            m_glBuffer->allocate(updateInfo.patchPixelsLength());

            void *vid = m_glBuffer->map(QGLBuffer::WriteOnly);
            memcpy(vid, fd, updateInfo.patchPixelsLength());
            m_glBuffer->unmap();

            // we set fill data to 0 so the next glTexImage2D call uses our buffer
            fd = 0;
        }
#endif

        glTexImage2D(GL_TEXTURE_2D, 0,
                     m_texturesInfo->internalFormat,
                     m_texturesInfo->width,
                     m_texturesInfo->height, 0,
                     m_texturesInfo->format,
                     m_texturesInfo->type,
                     fd);

#ifdef USE_PIXEL_BUFFERS
        if (cfg.useOpenGLTextureBuffer()) {
            m_glBuffer->release();
        }
#endif

    }
    else {
        QPoint patchOffset = updateInfo.patchOffset();
        QSize patchSize = updateInfo.patchSize();

#ifdef USE_PIXEL_BUFFERS
        if (cfg.useOpenGLTextureBuffer()) {
            m_glBuffer->bind();
            quint32 size = patchSize.width() * patchSize.height() * updateInfo.pixelSize();
            m_glBuffer->allocate(size);

            void *vid = m_glBuffer->map(QGLBuffer::WriteOnly);
            memcpy(vid, fd, size);
            m_glBuffer->unmap();

            // we set fill data to 0 so the next glTexImage2D call uses our buffer
            fd = 0;
        }
#endif

        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        patchOffset.x(), patchOffset.y(),
                        patchSize.width(), patchSize.height(),
                        m_texturesInfo->format,
                        m_texturesInfo->type,
                        fd);

#ifdef USE_PIXEL_BUFFERS
        if (cfg.useOpenGLTextureBuffer()) {
            m_glBuffer->release();
        }
#endif

    }

    /**
     * On the boundaries of KisImage, there is a border-effect as well.
     * So we just repeat the bounding pixels of the image to make
     * bilinear interpolator happy.
     */

    /**
     * WARN: This can repeat stripes of 1px width only!
     */

    const int pixelSize = updateInfo.pixelSize();
    const QRect imageRect = updateInfo.imageRect();
    const QPoint patchOffset = updateInfo.patchOffset();
    const QSize patchSize = updateInfo.patchSize();
    const QRect patchRect = QRect(m_textureRectInImagePixels.topLeft() +
                                  patchOffset,
                                  patchSize);

    if(imageRect.top() == patchRect.top()) {
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        patchOffset.x(), patchOffset.y() - 1,
                        patchSize.width(), 1,
                        m_texturesInfo->format,
                        m_texturesInfo->type,
                        updateInfo.data());
    }

    if (imageRect.bottom() == patchRect.bottom()) {
        int shift = patchSize.width() * (patchSize.height() - 1) *
                pixelSize;

        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        patchOffset.x(), patchOffset.y() + patchSize.height(),
                        patchSize.width(), 1,
                        m_texturesInfo->format,
                        m_texturesInfo->type,
                        updateInfo.data() + shift);

    }

    if (imageRect.left() == patchRect.left()) {

        QByteArray columnBuffer(patchSize.height() * pixelSize, 0);

        quint8 *srcPtr = updateInfo.data();
        quint8 *dstPtr = (quint8*) columnBuffer.data();
        for(int i = 0; i < patchSize.height(); i++) {
            memcpy(dstPtr, srcPtr, pixelSize);

            srcPtr += patchSize.width() * pixelSize;
            dstPtr += pixelSize;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        patchOffset.x() - 1, patchOffset.y(),
                        1, patchSize.height(),
                        m_texturesInfo->format,
                        m_texturesInfo->type,
                        columnBuffer.constData());

    }

    if (imageRect.right() == patchRect.right()) {

        QByteArray columnBuffer(patchSize.height() * pixelSize, 0);

        quint8 *srcPtr = updateInfo.data() + (patchSize.width() - 1) * pixelSize;
        quint8 *dstPtr = (quint8*) columnBuffer.data();
        for(int i = 0; i < patchSize.height(); i++) {
            memcpy(dstPtr, srcPtr, pixelSize);

            srcPtr += patchSize.width() * pixelSize;
            dstPtr += pixelSize;
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        patchOffset.x() + patchSize.width(), patchOffset.y(),
                        1, patchSize.height(),
                        m_texturesInfo->format,
                        m_texturesInfo->type,
                        columnBuffer.constData());
    }

    setNeedsMipmapRegeneration();
}

#ifdef USE_PIXEL_BUFFERS
void KisTextureTile::createTextureBuffer(const QByteArray &fillData)
{
    KisConfig cfg;
    if (cfg.useOpenGLTextureBuffer()) {
        m_glBuffer = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
        m_glBuffer->setUsagePattern(QGLBuffer::DynamicDraw);
        m_glBuffer->create();

        m_glBuffer->bind();

        m_glBuffer->allocate(fillData.size());

        void *vid = m_glBuffer->map(QGLBuffer::WriteOnly);
        memcpy(vid, fillData.constData(), fillData.size());
        m_glBuffer->unmap();

    }
    else {
        m_glBuffer = 0;
    }
}
#endif

#endif /* HAVE_OPENGL */

