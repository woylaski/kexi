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
#ifndef KIS_TEXTURE_TILE_H_
#define KIS_TEXTURE_TILE_H_

#include <opengl/kis_opengl.h>

#ifdef HAVE_OPENGL

#include "kis_texture_tile_update_info.h"

#include <QRect>
#include <QRectF>


struct KisGLTexturesInfo {

    // real width and height
    int width;
    int height;

    // width and height minus border padding?
    int effectiveWidth;
    int effectiveHeight;

    // size of the border padding
    int border;

    GLint internalFormat;
    GLint format;
    GLint type;
};

inline QRect stretchRect(const QRect &rc, int delta)
{
    return rc.adjusted(-delta, -delta, delta, delta);
}

class KisTextureTile
{
public:
    enum FilterMode {
        NearestFilterMode,  // nearest
        BilinearFilterMode, // linear, no mipmap
        TrilinearFilterMode, // LINEAR_MIPMAP_LINEAR
        nearest_mipmap_nearest,
        nearest_mipmap_linear,
        linear_mipmap_nearest
    };

    KisTextureTile(QRect imageRect, const KisGLTexturesInfo *texturesInfo,
                   const QByteArray &fillData, FilterMode mode);
    ~KisTextureTile();

    void update(const KisTextureTileUpdateInfo &updateInfo);

    inline QRect tileRectInImagePixels() {
        return m_tileRectInImagePixels;
    }

    inline GLuint textureId() {
        return m_textureId;
    }

    inline QRect textureRectInImagePixels() {
        return m_textureRectInImagePixels;
    }

    inline QRectF tileRectInTexturePixels() {
        return m_tileRectInTexturePixels;
    }

private:

    GLuint m_textureId;

    QRect m_tileRectInImagePixels;
    QRectF m_tileRectInTexturePixels;
    QRect m_textureRectInImagePixels;
    FilterMode m_filter;
    const KisGLTexturesInfo *m_texturesInfo;

    Q_DISABLE_COPY(KisTextureTile)
};


#endif /* HAVE_OPENGL */

#endif /* KIS_TEXTURE_TILE_H_ */

