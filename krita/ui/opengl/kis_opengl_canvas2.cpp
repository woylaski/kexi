/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006-2013
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

#include "opengl/kis_opengl_canvas2.h"

#ifdef HAVE_OPENGL

#include <QMenu>
#include <QWidget>
#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QPainter>
#include <QMatrix>
#include <QDebug>
#include <QThread>
#include <QMessageBox>

#include <QGLShaderProgram>
#include <QGLFramebufferObject>
#include <QGLContext>
#include <QTransform>

#include <kstandarddirs.h>

#include "KoToolProxy.h"
#include "KoToolManager.h"
#include "KoShapeManager.h"

#include "kis_types.h"
#include <ko_favorite_resource_manager.h>
#include "canvas/kis_canvas2.h"
#include "kis_coordinates_converter.h"
#include "kis_image.h"
#include "opengl/kis_opengl_image_textures.h"
#include "kis_canvas_resource_provider.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_debug.h"
#include "opengl/kis_opengl_canvas2_p.h"
#include "kis_coordinates_converter.h"
#include "canvas/kis_display_filter.h"

#define NEAR_VAL -1000.0
#define FAR_VAL 1000.0

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

namespace
{
const GLuint NO_PROGRAM = 0;
}

struct KisOpenGLCanvas2::Private
{
public:
    Private()
        : displayShader(0)
        , checkerShader(0)
        , displayFilter(0)
        , glSyncObject(0)
        , wrapAroundMode(false)
    {
    }

    ~Private() {
        delete displayShader;
        delete checkerShader;
    }

    KisOpenGLImageTexturesSP openGLImageTextures;

    QGLShaderProgram *displayShader;
    QGLShaderProgram *checkerShader;

    KisDisplayFilter *displayFilter;
    KisTextureTile::FilterMode filterMode;

    GLsync glSyncObject;

    bool wrapAroundMode;

    int xToColWithWrapCompensation(int x, const QRect &imageRect) {
        int firstImageColumn = openGLImageTextures->xToCol(imageRect.left());
        int lastImageColumn = openGLImageTextures->xToCol(imageRect.right());

        int colsPerImage = lastImageColumn - firstImageColumn + 1;
        int numWraps = floor(qreal(x) / imageRect.width());
        int remainder = x - imageRect.width() * numWraps;

        return colsPerImage * numWraps + openGLImageTextures->xToCol(remainder);
    }

    int yToRowWithWrapCompensation(int y, const QRect &imageRect) {
        int firstImageRow = openGLImageTextures->yToRow(imageRect.top());
        int lastImageRow = openGLImageTextures->yToRow(imageRect.bottom());

        int rowsPerImage = lastImageRow - firstImageRow + 1;
        int numWraps = floor(qreal(y) / imageRect.height());
        int remainder = y - imageRect.height() * numWraps;

        return rowsPerImage * numWraps + openGLImageTextures->yToRow(remainder);
    }

    void activateFilteringMode(qreal scaleX, qreal scaleY);
};

KisOpenGLCanvas2::KisOpenGLCanvas2(KisCanvas2 *canvas, KisCoordinatesConverter *coordinatesConverter, QWidget *parent, KisOpenGLImageTexturesSP imageTextures)
    : QGLWidget(KisOpenGL::sharedContextWidget()->format(), parent, KisOpenGL::sharedContextWidget())
    , KisCanvasWidgetBase(canvas, coordinatesConverter)
    , d(new Private())
{
    d->openGLImageTextures = imageTextures;

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAutoFillBackground(false);

    setAttribute(Qt::WA_InputMethodEnabled, true);

    if (isSharing()) {
        dbgUI << "Created QGLWidget with sharing";
    } else {
        dbgUI << "Created QGLWidget with no sharing";
    }

    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    slotConfigChanged();

    KisConfig cfg;
    d->openGLImageTextures->generateCheckerTexture(createCheckersImage(cfg.checkSize()));

}

KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
    delete d;
}

void KisOpenGLCanvas2::setDisplayFilter(KisDisplayFilter *displayFilter)
{
    d->displayFilter = displayFilter;
    initializeDisplayShader();
}

void KisOpenGLCanvas2::setWrapAroundViewingMode(bool value)
{
    d->wrapAroundMode = value;
    update();
}

void KisOpenGLCanvas2::initializeGL()
{
    if (!VSyncWorkaround::tryDisableVSync(this)) {
        qWarning();
        qWarning() << "WARNING: We didn't manage to switch off VSync on your graphics adapter.";
        qWarning() << "WARNING: It means either your hardware or driver doesn't support it,";
        qWarning() << "WARNING: or we just don't know about this hardware. Please report us a bug";
        qWarning() << "WARNING: with the output of \'glxinfo\' for your card.";
        qWarning();
        qWarning() << "WARNING: Trying to workaround it by disabling Double Buffering.";
        qWarning() << "WARNING: You may see some flickering when painting with some tools. It doesn't";
        qWarning() << "WARNING: affect the quality of the final image, though.";
        qWarning();

        if (doubleBuffer()) {
            qCritical() << "CRITICAL: Failed to disable Double Buffering. Lines may look \"bended\" on your image.";
            qCritical() << "CRITICAL: Your graphics card or driver does not fully support Krita's OpenGL canvas.";
            qCritical() << "CRITICAL: For an optimal experience, please disable OpenGL";
            qCritical();
        }
    }

    initializeCheckerShader();
    initializeDisplayShader();

    Sync::init();
}

void KisOpenGLCanvas2::resizeGL(int width, int height)
{
    glViewport(0, 0, (GLint)width, (GLint)height);
    coordinatesConverter()->setCanvasWidgetSize(QSize(width, height));
}

void KisOpenGLCanvas2::paintGL()
{
    renderCanvasGL();

    QPainter gc(this);
    renderDecorations(&gc);
    gc.end();

    if (d->glSyncObject) {
        Sync::deleteSync(d->glSyncObject);
    }

    d->glSyncObject = Sync::getSync();
}

bool KisOpenGLCanvas2::isBusy() const
{
    return Sync::syncStatus(d->glSyncObject) == Sync::Unsignaled;
}

inline QVector<QVector3D> rectToVertices(const QRectF &rc)
{
    QVector<QVector3D> vertices;
    vertices << QVector3D(rc.left(),  rc.bottom(), 0.f)
             << QVector3D(rc.left(),  rc.top(),    0.f)
             << QVector3D(rc.right(), rc.bottom(), 0.f)
             << QVector3D(rc.left(),  rc.top(), 0.f)
             << QVector3D(rc.right(), rc.top(), 0.f)
             << QVector3D(rc.right(), rc.bottom(),    0.f);

    return vertices;
}

inline QVector<QVector2D> rectToTexCoords(const QRectF &rc)
{
    QVector<QVector2D> texCoords;
    texCoords << QVector2D(rc.left(), rc.bottom())
              << QVector2D(rc.left(), rc.top())
              << QVector2D(rc.right(), rc.bottom())
              << QVector2D(rc.left(), rc.top())
              << QVector2D(rc.right(), rc.top())
              << QVector2D(rc.right(), rc.bottom());

    return texCoords;
}

void KisOpenGLCanvas2::drawCheckers() const
{
    if(!d->checkerShader)
        return;

    KisCoordinatesConverter *converter = coordinatesConverter();
    QTransform textureTransform;
    QTransform modelTransform;
    QRectF textureRect;
    QRectF modelRect;

    QRectF viewportRect = !d->wrapAroundMode ?
        converter->imageRectInViewportPixels() :
        converter->widgetToViewport(this->rect());

    converter->getOpenGLCheckersInfo(viewportRect,
                                     &textureTransform, &modelTransform, &textureRect, &modelRect);

    // XXX: getting a config object every time we draw the checkers is bad for performance!
    KisConfig cfg;
    GLfloat checkSizeScale = KisOpenGLImageTextures::BACKGROUND_TEXTURE_CHECK_SIZE / static_cast<GLfloat>(cfg.checkSize());

    textureTransform *= QTransform::fromScale(checkSizeScale / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE,
                                              checkSizeScale / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE);

    d->checkerShader->bind();

    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(0, width(), height(), 0, NEAR_VAL, FAR_VAL);

    // Set view/projection matrices
    QMatrix4x4 modelMatrix(modelTransform);
    modelMatrix.optimize();
    modelMatrix = projectionMatrix * modelMatrix;
    d->checkerShader->setUniformValue("modelViewProjection", modelMatrix);

    QMatrix4x4 textureMatrix(textureTransform);
    d->checkerShader->setUniformValue("textureMatrix", textureMatrix);

    //Setup the geometry for rendering
    QVector<QVector3D> vertices = rectToVertices(modelRect);
    d->checkerShader->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d->checkerShader->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());

    QVector<QVector2D> texCoords = rectToTexCoords(textureRect);
    d->checkerShader->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d->checkerShader->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());

     // render checkers
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->openGLImageTextures->checkerTexture());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    d->checkerShader->release();
}

void KisOpenGLCanvas2::drawImage() const
{
    if(!d->displayShader)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    KisCoordinatesConverter *converter = coordinatesConverter();

    d->displayShader->bind();

    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(0, width(), height(), 0, NEAR_VAL, FAR_VAL);

    // Set view/projection matrices
    QMatrix4x4 modelMatrix(coordinatesConverter()->imageToWidgetTransform());
    modelMatrix.optimize();
    modelMatrix = projectionMatrix * modelMatrix;
    d->displayShader->setUniformValue("modelViewProjection", modelMatrix);

    QMatrix4x4 textureMatrix;
    textureMatrix.setToIdentity();
    d->displayShader->setUniformValue("textureMatrix", textureMatrix);

    QRectF widgetRect(0,0, width(), height());
    QRectF widgetRectInImagePixels = converter->documentToImage(converter->widgetToDocument(widgetRect));

    qreal scaleX, scaleY;
    converter->imageScale(&scaleX, &scaleY);
    d->displayShader->setUniformValue("viewportScale", (GLfloat) scaleX);
    d->displayShader->setUniformValue("texelSize", (GLfloat) d->openGLImageTextures->texelSize());

    QRect ir = d->openGLImageTextures->storedImageBounds();
    QRect wr = widgetRectInImagePixels.toAlignedRect();

    if (!d->wrapAroundMode) {
        // if we don't want to paint wrapping images, just limit the
        // processing area, and the code will handle all the rest
        wr &= ir;
    }

    int firstColumn = d->xToColWithWrapCompensation(wr.left(), ir);
    int lastColumn = d->xToColWithWrapCompensation(wr.right(), ir);
    int firstRow = d->yToRowWithWrapCompensation(wr.top(), ir);
    int lastRow = d->yToRowWithWrapCompensation(wr.bottom(), ir);

    int minColumn = d->openGLImageTextures->xToCol(ir.left());
    int maxColumn = d->openGLImageTextures->xToCol(ir.right());
    int minRow = d->openGLImageTextures->yToRow(ir.top());
    int maxRow = d->openGLImageTextures->yToRow(ir.bottom());

    int imageColumns = maxColumn - minColumn + 1;
    int imageRows = maxRow - minRow + 1;

    for (int col = firstColumn; col <= lastColumn; col++) {
        for (int row = firstRow; row <= lastRow; row++) {

            int effectiveCol = col;
            int effectiveRow = row;
            QPointF tileWrappingTranslation;

            if (effectiveCol > maxColumn || effectiveCol < minColumn) {
                int translationStep = floor(qreal(col) / imageColumns);
                int originCol = translationStep * imageColumns;
                effectiveCol = col - originCol;
                tileWrappingTranslation.rx() = translationStep * ir.width();
            }

            if (effectiveRow > maxRow || effectiveRow < minRow) {
                int translationStep = floor(qreal(row) / imageRows);
                int originRow = translationStep * imageRows;
                effectiveRow = row - originRow;
                tileWrappingTranslation.ry() = translationStep * ir.height();
            }

            KisTextureTile *tile =
                    d->openGLImageTextures->getTextureTileCR(effectiveCol, effectiveRow);

            KIS_ASSERT_RECOVER_BREAK(tile);

            /*
             * We create a float rect here to workaround Qt's
             * "history reasons" in calculation of right()
             * and bottom() coordinates of integer rects.
             */
            QRectF textureRect(tile->tileRectInTexturePixels());
            QRectF modelRect(tile->tileRectInImagePixels().translated(tileWrappingTranslation.x(), tileWrappingTranslation.y()));

            //Setup the geometry for rendering
            QVector<QVector3D> vertices = rectToVertices(modelRect);
            d->displayShader->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
            d->displayShader->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());

            QVector<QVector2D> texCoords  = rectToTexCoords(textureRect);
            d->displayShader->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
            d->displayShader->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, texCoords.constData());

            glActiveTexture(GL_TEXTURE0);
            tile->bindToActiveTexture();
            d->displayShader->setUniformValue("texture0", 0);

            if (d->displayFilter) {
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_3D, d->displayFilter->lutTexture());
                d->displayShader->setUniformValue("texture1", 1);
            }

            d->activateFilteringMode(scaleX, scaleY);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    d->displayShader->release();
}

void KisOpenGLCanvas2::Private::activateFilteringMode(qreal scaleX, qreal scaleY)
{
    if (SCALE_MORE_OR_EQUAL_TO(scaleX, scaleY, 2.0)) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        switch(filterMode) {
        case KisTextureTile::NearestFilterMode:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
        case KisTextureTile::BilinearFilterMode:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        case KisTextureTile::TrilinearFilterMode:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            break;
        case KisTextureTile::HighQualityFiltering:
            if (SCALE_LESS_THAN(scaleX, scaleY, 0.5)) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
            break;
        }
    }
}

void KisOpenGLCanvas2::initializeCheckerShader()
{
    delete d->checkerShader;
    d->checkerShader = new QGLShaderProgram();

    QString vertexShaderName;
    QString fragmentShaderName;

    if (KisOpenGL::supportsGLSL13()) {
        vertexShaderName = KGlobal::dirs()->findResource("data", "krita/shaders/matrix_transform.vert");
        fragmentShaderName = KGlobal::dirs()->findResource("data", "krita/shaders/simple_texture.frag");
    } else {
        vertexShaderName = KGlobal::dirs()->findResource("data", "krita/shaders/matrix_transform_legacy.vert");
        fragmentShaderName = KGlobal::dirs()->findResource("data", "krita/shaders/simple_texture_legacy.frag");
    }

    d->checkerShader->addShaderFromSourceFile(QGLShader::Vertex, vertexShaderName);
    d->checkerShader->addShaderFromSourceFile(QGLShader::Fragment, fragmentShaderName);

    d->checkerShader->bindAttributeLocation("a_vertexPosition", PROGRAM_VERTEX_ATTRIBUTE);
    d->checkerShader->bindAttributeLocation("a_textureCoordinate", PROGRAM_TEXCOORD_ATTRIBUTE);

    if (!d->checkerShader->link()) {
        QMessageBox::critical(this, "Fatal Error", QString(i18n("Krita could not initialize the OpenGL canvas:\n %1.\n Krita will disable OpenGL and close now.")).arg(d->checkerShader->log()), QMessageBox::Close);
        KisConfig cfg;
        cfg.setUseOpenGL(false);
        cfg.setCanvasState("OPENGL_FAILED");
        exit(1);
    }
    Q_ASSERT(d->checkerShader->isLinked());
}

void KisOpenGLCanvas2::initializeDisplayShader()
{
    delete d->displayShader;
    d->displayShader = new QGLShaderProgram();

    if (d->displayFilter && !d->displayFilter->program().isEmpty()) {
        d->displayShader->addShaderFromSourceFile(QGLShader::Vertex, KGlobal::dirs()->findResource("data", "krita/shaders/matrix_transform_legacy.vert"));
        d->displayShader->addShaderFromSourceCode(QGLShader::Fragment, d->displayFilter->program().toLatin1());
    } else if (KisOpenGL::supportsGLSL13()) {
        d->displayShader->addShaderFromSourceFile(QGLShader::Vertex, KGlobal::dirs()->findResource("data", "krita/shaders/matrix_transform.vert"));
        if (d->filterMode == KisTextureTile::HighQualityFiltering) {
            d->displayShader->addShaderFromSourceFile(QGLShader::Fragment, KGlobal::dirs()->findResource("data", "krita/shaders/highq_downscale.frag"));
        } else {
            d->displayShader->addShaderFromSourceFile(QGLShader::Fragment, KGlobal::dirs()->findResource("data", "krita/shaders/simple_texture.frag"));
        }
    } else {
        d->displayShader->addShaderFromSourceFile(QGLShader::Vertex, KGlobal::dirs()->findResource("data", "krita/shaders/matrix_transform_legacy.vert"));
        d->displayShader->addShaderFromSourceFile(QGLShader::Fragment, KGlobal::dirs()->findResource("data", "krita/shaders/simple_texture_legacy.frag"));
    }

    d->displayShader->bindAttributeLocation("a_vertexPosition", PROGRAM_VERTEX_ATTRIBUTE);
    d->displayShader->bindAttributeLocation("a_textureCoordinate", PROGRAM_TEXCOORD_ATTRIBUTE);

    if (!d->displayShader->link()) {
        QMessageBox::critical(this, "Fatal Error", QString(i18n("Krita could not initialize the OpenGL canvas:\n %1.\n Krita will disable OpenGL and close now.")).arg(d->displayShader->log()), QMessageBox::Close);
        KisConfig cfg;
        cfg.setUseOpenGL(false);
        cfg.setCanvasState("OPENGL_FAILED");
        exit(1);
    }

    Q_ASSERT(d->displayShader->isLinked());
}

void KisOpenGLCanvas2::slotConfigChanged()
{
    KisConfig cfg;
    d->openGLImageTextures->generateCheckerTexture(createCheckersImage(cfg.checkSize()));
    d->filterMode = (KisTextureTile::FilterMode) cfg.openGLFilteringMode();

    notifyConfigChanged();
}

QVariant KisOpenGLCanvas2::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return processInputMethodQuery(query);
}

void KisOpenGLCanvas2::inputMethodEvent(QInputMethodEvent *event)
{
    processInputMethodEvent(event);
}

void KisOpenGLCanvas2::renderCanvasGL() const
{
    // Draw the border (that is, clear the whole widget to the border color)
    QColor widgetBackgroundColor = borderColor();
    glClearColor(widgetBackgroundColor.redF(), widgetBackgroundColor.greenF(), widgetBackgroundColor.blueF(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    drawCheckers();
    drawImage();
}

void KisOpenGLCanvas2::renderDecorations(QPainter *painter)
{
    QRect boundingRect = coordinatesConverter()->imageRectInWidgetPixels().toAlignedRect();
    drawDecorations(*painter, boundingRect);
}

bool KisOpenGLCanvas2::callFocusNextPrevChild(bool next)
{
    return focusNextPrevChild(next);
}

void KisOpenGLCanvas2::paintEvent(QPaintEvent* event)
{
    // Workaround for bug 322808, paint events with only a partial rect cause flickering
    // Drop those event and trigger a new full update
    if (event->rect().width() == width() && event->rect().height() == height()) {
        QGLWidget::paintEvent(event);
    } else {
        update();
    }
}


#include "kis_opengl_canvas2.moc"
#endif // HAVE_OPENGL
