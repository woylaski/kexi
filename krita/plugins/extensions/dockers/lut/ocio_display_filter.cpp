/*
 *  Copyright (c) 2012 Boudewijn Rempt <boud@valdyas.org>
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
#include "ocio_display_filter.h"
#include <math.h>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include <kis_config.h>

#ifdef HAVE_OPENGL
#include <opengl/kis_opengl.h>

static const int LUT3D_EDGE_SIZE = 32;

const char * m_fragShaderText = ""
        "\n"
        "uniform sampler2D texture0;\n"
        "uniform sampler3D texture1;\n"
        "varying mediump vec4 v_textureCoordinate;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec4 col = texture2D(texture0, v_textureCoordinate.st);\n"
        "    gl_FragColor = OCIODisplay(col, texture1);\n"
        "}\n";

#endif

OcioDisplayFilter::OcioDisplayFilter(QObject *parent)
    : KisDisplayFilter(parent)
    , inputColorSpaceName(0)
    , displayDevice(0)
    , view(0)
    , swizzle(RGBA)
#ifdef HAVE_OPENGL
    , m_lut3dTexID(0)
#endif
{
}


void OcioDisplayFilter::filter(quint8 *src, quint8 */*dst*/, quint32 numPixels)
{
    // processes that data _in_ place
    if (m_processor) {
        OCIO::PackedImageDesc img(reinterpret_cast<float*>(src), numPixels, 1, 4);
        m_processor->apply(img);
    }
}

#ifdef HAVE_OPENGL

QString OcioDisplayFilter::program() const
{
    return m_program;
}

GLuint OcioDisplayFilter::lutTexture() const
{
    return m_lut3dTexID;
}
#endif

void OcioDisplayFilter::updateProcessor()
{
    if (!config) {
        return;
    }

    if (!displayDevice) {
        displayDevice = config->getDefaultDisplay();
    }

    if (!view) {
        view = config->getDefaultView(displayDevice);
    }

    if (!inputColorSpaceName) {
        inputColorSpaceName = config->getColorSpaceNameByIndex(0);
    }

    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpaceName(inputColorSpaceName);
    transform->setDisplay(displayDevice);
    transform->setView(view);

    // fstop exposure control -- not sure how that translates to our exposure
    {
        float gain = powf(2.0f, exposure);
        const float slope4f[] = { gain, gain, gain, 1.0f };
        float m44[16];
        float offset4[4];
        OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
        OCIO::MatrixTransformRcPtr mtx =  OCIO::MatrixTransform::Create();
        mtx->setValue(m44, offset4);
        transform->setLinearCC(mtx);
    }

    // channel swizzle
    {
        int channelHot[4];
        switch (swizzle) {
        case LUMINANCE:
            channelHot[0] = 1;
            channelHot[1] = 1;
            channelHot[2] = 1;
            channelHot[3] = 0;
            break;
        case RGBA:
            channelHot[0] = 1;
            channelHot[1] = 1;
            channelHot[2] = 1;
            channelHot[3] = 1;
            break;
        case R:
            channelHot[0] = 1;
            channelHot[1] = 0;
            channelHot[2] = 0;
            channelHot[3] = 0;
            break;
        case G:
            channelHot[0] = 0;
            channelHot[1] = 1;
            channelHot[2] = 0;
            channelHot[3] = 0;
            break;
        case B:
            channelHot[0] = 0;
            channelHot[1] = 0;
            channelHot[2] = 1;
            channelHot[3] = 0;
            break;
        case A:
            channelHot[0] = 0;
            channelHot[1] = 0;
            channelHot[2] = 0;
            channelHot[3] = 1;
        default:
            ;
        }
        float lumacoef[3];
        config->getDefaultLumaCoefs(lumacoef);
        float m44[16];
        float offset[4];
        OCIO::MatrixTransform::View(m44, offset, channelHot, lumacoef);
        OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
        swizzle->setValue(m44, offset);
        transform->setChannelView(swizzle);
    }

    // Post-display transform gamma
    {
        float exponent = 1.0f/std::max(1e-6f, static_cast<float>(gamma));
        const float exponent4f[] = { exponent, exponent, exponent, exponent };
        OCIO::ExponentTransformRcPtr expTransform =  OCIO::ExponentTransform::Create();
        expTransform->setValue(exponent4f);
        transform->setDisplayCC(expTransform);
    }

    m_processor = config->getProcessor(transform);

#ifdef HAVE_OPENGL

    // check whether we are allowed to use shaders -- though that should
    // work for everyone these days
    KisConfig cfg;
    if (!cfg.useOpenGL()) return;

    if (m_lut3d.size() == 0) {
        //qDebug() << "generating lut";
        glGenTextures(1, &m_lut3dTexID);

        int num3Dentries = 3 * LUT3D_EDGE_SIZE * LUT3D_EDGE_SIZE * LUT3D_EDGE_SIZE;
        m_lut3d.fill(0.0, num3Dentries);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, m_lut3dTexID);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F_ARB,
                     LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                     0, GL_RGB, GL_FLOAT, &m_lut3d.constData()[0]);
    }

    // Step 1: Create a GPU Shader Description
    OCIO::GpuShaderDesc shaderDesc;
    shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
    shaderDesc.setFunctionName("OCIODisplay");
    shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);


    // Step 2: Compute the 3D LUT
    QString lut3dCacheID = QString::fromLatin1(m_processor->getGpuLut3DCacheID(shaderDesc));
    if(lut3dCacheID != m_lut3dcacheid)
    {
        //qDebug() << "Computing 3DLut " << m_lut3dcacheid;
        m_lut3dcacheid = lut3dCacheID;
        m_processor->getGpuLut3D(&m_lut3d[0], shaderDesc);

        glBindTexture(GL_TEXTURE_3D, m_lut3dTexID);
        glTexSubImage3D(GL_TEXTURE_3D, 0,
                        0, 0, 0,
                        LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                        GL_RGB, GL_FLOAT, &m_lut3d[0]);
    }


    // Step 3: Generate the shader text
    QString shaderCacheID = QString::fromLatin1(m_processor->getGpuShaderTextCacheID(shaderDesc));
    if (m_program.isEmpty() || shaderCacheID != m_shadercacheid) {
        qDebug() << "Computing Shader " << m_shadercacheid;

        m_shadercacheid = shaderCacheID;

        std::ostringstream os;
        os << m_processor->getGpuShaderText(shaderDesc) << "\n";
        os << m_fragShaderText;

        m_program = QString::fromLatin1(os.str().c_str());
    }


#endif

}
