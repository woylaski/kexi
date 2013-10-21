/*
 *  kis_tool_transform.h - part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2010 Marc Pegon <pe.marc@free.fr>
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

#ifndef KIS_TOOL_TRANSFORM_H_
#define KIS_TOOL_TRANSFORM_H_

#include <KoIcon.h>

#include <QPoint>
#include <QPointF>
#include <QVector2D>
#include <QVector3D>
#include <QButtonGroup>

#include <kstandarddirs.h>

#include <KoInteractionTool.h>
#include <KoToolFactoryBase.h>
#include <KoUpdater.h>

#include <kis_shape_selection.h>
#include <kis_undo_adapter.h>
#include <kis_types.h>
#include <flake/kis_node_shape.h>
#include <kis_tool.h>

#include "tool_transform_args.h"
#include "tool_transform_changes_tracker.h"
#include "kis_tool_transform_config_widget.h"
#include "transform_transaction_properties.h"

class KoID;

class KisFilterStrategy;
class KisCanvas2;

class QTouchEvent;


/**
 * Transform tool
 * The tool offers two different modes : Free Transform and Warp
 * - Free Transform mode allows the user to translate, scale, shear, rotate
 * and apply a perspective transformation to a selection or the whole
 * canvas.
 * - Warp mode allows the user to warp the selection of the canvas
 * by grabbing and moving control points placed on the image.
 *   The user can either work with default control points, like a grid
 *   whose density can be modified, or place the control points himself.
 * The modifications made on the selected pixels are applied only when
 * the user clicks the Apply button : the semi-transparent image displayed
 * until the user click that button is only a preview.
 */
class KisToolTransform : public KisTool
{

    Q_OBJECT

    Q_PROPERTY(TransformToolMode transformMode READ transformMode WRITE setTransformMode NOTIFY transformModeChanged)

    Q_PROPERTY(double translateX READ translateX WRITE setTranslateX NOTIFY freeTransformChanged)
    Q_PROPERTY(double translateY READ translateY WRITE setTranslateY NOTIFY freeTransformChanged)

    Q_PROPERTY(double rotateX READ rotateX WRITE setRotateX NOTIFY freeTransformChanged)
    Q_PROPERTY(double rotateY READ rotateY WRITE setRotateY NOTIFY freeTransformChanged)
    Q_PROPERTY(double rotateZ READ rotateZ WRITE setRotateZ NOTIFY freeTransformChanged)

    Q_PROPERTY(double scaleX READ scaleX WRITE setScaleX NOTIFY freeTransformChanged)
    Q_PROPERTY(double scaleY READ scaleY WRITE setScaleY NOTIFY freeTransformChanged)

    Q_PROPERTY(double shearX READ shearX WRITE setShearX NOTIFY freeTransformChanged)
    Q_PROPERTY(double shearY READ shearY WRITE setShearY NOTIFY freeTransformChanged)

    Q_PROPERTY(WarpType warpType READ warpType WRITE setWarpType NOTIFY warpTransformChanged)
    Q_PROPERTY(double warpFlexibility READ warpFlexibility WRITE setWarpFlexibility NOTIFY warpTransformChanged)
    Q_PROPERTY(int warpPointDensity READ warpPointDensity WRITE setWarpPointDensity NOTIFY warpTransformChanged)



public:
    enum TransformToolMode {
        FreeTransformMode,
        WarpTransformMode
    };
    Q_ENUMS(TransformToolMode)

    enum WarpType {
        RigidWarpType,
        AffineWarpType,
        SimilitudeWarpType
    };
    Q_ENUMS(WarpType)

    KisToolTransform(KoCanvasBase * canvas);
    virtual ~KisToolTransform();

    virtual QWidget* createOptionWidget();

    virtual void mousePressEvent(KoPointerEvent *e);
    virtual void mouseMoveEvent(KoPointerEvent *e);
    virtual void mouseReleaseEvent(KoPointerEvent *e);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void touchEvent(QTouchEvent *event);

    void paint(QPainter& gc, const KoViewConverter &converter);

    TransformToolMode transformMode() const;

    double translateX() const;
    double translateY() const;

    double rotateX() const;
    double rotateY() const;
    double rotateZ() const;

    double scaleX() const;
    double scaleY() const;

    double shearX() const;
    double shearY() const;

    WarpType warpType() const;
    double warpFlexibility() const;
    int warpPointDensity() const;

    bool wantsTouch() const { return true; }

public slots:
    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);
    virtual void deactivate();
    // Applies the current transformation to the original paint device and commits it to the undo stack
    void applyTransform();

    void setTransformMode( KisToolTransform::TransformToolMode newMode );

    void setTranslateX(double translateX);
    void setTranslateY(double translateY);

    void setRotateX(double rotation);
    void setRotateY(double rotation);
    void setRotateZ(double rotation);

    void setScaleX(double scaleX);
    void setScaleY(double scaleY);

    void setShearX(double shearX);
    void setShearY(double shearY);

    void setWarpType(WarpType type);
    void setWarpFlexibility(double flexibility);
    void setWarpPointDensity(int density);

Q_SIGNALS:
    void transformModeChanged();
    void freeTransformChanged();
    void warpTransformChanged();

protected:
    void requestUndoDuringStroke();
    void requestStrokeEnd();
    void requestStrokeCancellation();

private:
    void clearDevices(KisNodeSP node, bool recursive);
    void transformDevices(KisNodeSP node, bool recursive);

    void startStroke(ToolTransformArgs::TransformMode mode);
    void endStroke();
    void cancelStroke();

private:
    // Used in dichotomic search (see below)
    typedef enum {NONE, XCOORD, YCOORD} DICHO_DROP;

    // Free-Transform math functions
    QVector3D scale(double x, double y, double z) {
        return QVector3D(x * m_currentArgs.scaleX(), y * m_currentArgs.scaleY(), z);
    }
    QVector3D invscale(double x, double y, double z) {
        return QVector3D(x / m_currentArgs.scaleX(), y / m_currentArgs.scaleY(), z);
    }
    QVector3D shear(double x, double y, double z) {
        double x2 = x + y * m_currentArgs.shearX();
        return QVector3D(x2, y + x2 * m_currentArgs.shearY(), z);
    }
    QVector3D invshear(double x, double y, double z) {
        y -= x * m_currentArgs.shearY();
        x -= y * m_currentArgs.shearX();

        return QVector3D(x, y, z);
    }
    QVector3D rotX(double x, double y, double z) {
        return QVector3D(x, m_cosaX * y - m_sinaX * z, m_cosaX * z + m_sinaX * y);
    }
    QVector3D invrotX(double x, double y, double z) {
        return QVector3D(x, m_cosaX * y + m_sinaX * z, m_cosaX * z - m_sinaX * y);
    }
    QVector3D rotY(double x, double y, double z) {
        return QVector3D(m_cosaY * x + m_sinaY * z, y, - m_sinaY * x + m_cosaY * z);
    }
    QVector3D invrotY(double x, double y, double z) {
        return QVector3D(m_cosaY * x - m_sinaY * z, y, + m_sinaY * x + m_cosaY * z);
    }
    QVector3D rotZ(double x, double y, double z) {
        return QVector3D(m_cosaZ*x - m_sinaZ*y, m_sinaZ*x + m_cosaZ*y, z);
    }
    QVector3D invrotZ(double x, double y, double z) {
        return QVector3D(m_cosaZ*x + m_sinaZ*y, -m_sinaZ*x + m_cosaZ*y, z);
    }
    QPointF perspective(double x, double y, double z) {
        if (!m_currentArgs.aX() && !m_currentArgs.aY())
            return QPointF(x, y);

        QVector3D t = QVector3D(x, y, z) - m_currentArgs.cameraPos();
        if (t.z() == 0.0)
            return QPointF(0,0);

        return QPointF((t.x() - m_currentArgs.eyePos().x()) * m_currentArgs.eyePos().z() / t.z(), (t.y() - m_currentArgs.eyePos().y()) * m_currentArgs.eyePos().z() / t.z());
    }
    // The perspective is only invertible if the plane into which the returned point should be is given
    QVector3D invperspective(double x, double y, QVector3D plan) {
        // The following is the solution of the system :
        // x = (xinv - cx)*ez / (zinv - cz)
        // y = (yinv - cy)*ez / (zinv - cz)
        // a*xinv + b*yinv + c*zinv = 0
        // where :
        //    (cx, cy, cz) = (m_currentArgs.cameraPos().z().x() + m_currentArgs.eyePos().x(), m_currentArgs.cameraPos().z().y() + m_currentArgs.eyePos().y(), m_currentArgs.cameraPos().z().z())
        //    ez = m_currentArgs.eyePos()
        //    (a, b, c) = plan
        if (!m_currentArgs.aX() && !m_currentArgs.aY())
            return QVector3D(x, y, 0);

        double a = plan.x();
        double b = plan.y();
        double c = plan.z();
        double denom = a*x + b*y + c*m_currentArgs.eyePos().z();

        if (!denom)
            return QVector3D(0, 0, 0);

        double cx = (m_currentArgs.cameraPos().x() - m_currentArgs.eyePos().x());
        double cy = (m_currentArgs.cameraPos().y() - m_currentArgs.eyePos().y());
        double cz = m_currentArgs.cameraPos().z();
        double ez = m_currentArgs.eyePos().z();
        double acx = a * cx;
        double bcy = b * cy;
        double ccz = c * cz;
        double xinv, yinv, zinv;
        xinv = (- (ccz + bcy) * x + cx * (b*y + c*ez)) / denom;
        yinv = (cy * (a*x + c*ez) - (ccz + acx) * y) / denom;
        zinv = ((a*x + b*y) * cz - (acx + bcy) * ez) / denom;

        return QVector3D(xinv, yinv, zinv);
    }
    // Convenience method : composition of all the free-transform functions (except perspective)
    QVector3D transformVector(double x, double y, double z) {
        QVector3D t = scale(x, y ,z);
        t = shear(t.x(), t.y(), t.z());
        t = rotZ(t.x(), t.y(), t.z());
        t = rotY(t.x(), t.y(), t.z());
        t = rotX(t.x(), t.y(), t.z());

        return t;
    }
    QVector3D transformVector(QVector3D v) {
        return transformVector(v.x(), v.y(), v.z());
    }
    QVector3D invTransformVector(double x, double y, double z) {
        QVector3D t = invrotX(x, y, z);
        t = invrotY(t.x(), t.y(), t.z());
        t = invrotZ(t.x(), t.y(), t.z());
        t = invshear(t.x(), t.y(), t.z());
        t = invscale(t.x(), t.y(), t.z());

        return t;
    }
    QVector3D invTransformVector(QVector3D v) {
        return invTransformVector(v.x(), v.y(), v.z());
    }
    // Takes resX and resY as scale factors to take view resolution into account (when making QImage previews)
    QVector3D transformVector_preview(double x, double y, double z, double resX, double resY) {
        QVector3D t = scale(x, y ,z);
        t = QVector3D(t.x() * resX, t.y() * resY, t.z());
        t = shear(t.x(), t.y(), t.z());
        t = rotZ(t.x(), t.y(), t.z());
        t = rotY(t.x(), t.y(), t.z());
        t = rotX(t.x(), t.y(), t.z());

        return t;
    }
    QVector3D transformVector_preview(QVector3D v, double resX, double resY) {
        return transformVector_preview(v.x(), v.y(), v.z(), resX, resY);
    }

    // Bounding rectangle of 4 points
    QRectF boundRect(QPointF P0, QPointF P1, QPointF P2, QPointF P3);
    // Returns the minimum and the maximum of the Z component of the 4 given vectors (x being the min, and y the max in the returned point)
    QPointF minMaxZ(QVector3D P0, QVector3D P1, QVector3D P2, QVector3D P3);
    // Determinant math function
    int det(const QPointF & v, const QPointF & w);
    // Square of the euclidian distance
    double distsq(const QPointF & v, const QPointF & w);

    QCursor getScaleCursor(const QPointF &handlePt);
    QCursor getShearCursor(const QPointF &direction);

    // Makes a copy of m_currentArgs into args
    void storeArgs(ToolTransformArgs &args);
    // Makes a copy of args into m_currentArgs
    void restoreArgs(const ToolTransformArgs &args);
    // Returns the bounding rect of the transformed preview when the tool is in Warp mode
	QRectF calcWarpBoundRect();
    // Recalcs the outline, current QImage(s)..
    void recalcOutline();
    // Recalcs the outline and update the corresponding areas of the canvas (union of the outline boundrect before and after recalc)
    void outlineChanged();
    // Sets the cursor according to mouse position (doesn't take shearing into account well yet)
    void setFunctionalCursor();
    inline void switchPoints(QPointF *p1, QPointF *p2);
    // Sets m_function according to mouse position and modifier
    void setTransformFunction(QPointF mousePos, Qt::KeyboardModifiers modifiers);

    double gradientDescent_f(QVector3D v1, QVector3D v2, QVector3D desired, double scaleX, double scaleY);
    double gradientDescent_partialDeriv1_f(QVector3D v1, QVector3D v2, QVector3D desired, double scaleX, double scaleY, double epsilon);
    double gradientDescent_partialDeriv2_f(QVector3D v1, QVector3D v2, QVector3D desired, double scaleX, double scaleY, double epsilon);
    int gradientDescent(QVector3D v1, QVector3D v2, QVector3D desired, double x0, double y0, double epsilon, double gradStep, int nbIt1, int nbIt2, double epsilon_deriv, double *x_min, double *y_min);

    // Dichotomic search of scaleX when there is perspective
    double dichotomyScaleX(QVector3D v1, QVector3D v2, DICHO_DROP flag, double desired, double b, double precision, double maxIterations1, double maxIterations2);
    double dichotomyScaleY(QVector3D v1, QVector3D v2, DICHO_DROP flag, double desired, double b, double precision, double maxIterations1, double maxIterations2);
    // If p is inside r, p is returned, otherwise the returned point is the intersection of the line given by vector p, and the rectangle
    inline QPointF clipInRect(QPointF p, QRectF r);

    void commitChanges();

    // Updated the widget according to m_currentArgs
    void updateOptionWidget();

    void initTransformMode(ToolTransformArgs::TransformMode mode);

    void initThumbnailImage(KisPaintDeviceSP previewDevice);
    void updateSelectionPath();
    void updateApplyResetAvailability();

private:
    enum function {ROTATE = 0, MOVE, RIGHTSCALE, TOPRIGHTSCALE, TOPSCALE, TOPLEFTSCALE,
                   LEFTSCALE, BOTTOMLEFTSCALE, BOTTOMSCALE, BOTTOMRIGHTSCALE,
                   BOTTOMSHEAR, RIGHTSHEAR, TOPSHEAR, LEFTSHEAR,
                   MOVECENTER, PERSPECTIVE
                  };

    function m_function; // current transformation function

    QCursor m_scaleCursors[8]; // cursors for the 8 directions
    QPixmap m_shearCursorPixmap;

    ToolTransformArgs m_currentArgs;
    ToolTransformArgs m_clickArgs;
    int m_handleRadius;
    int m_rotationCenterRadius;
    int m_maxRadius;

    bool m_actuallyMoveWhileSelected; // true <=> selection has been moved while clicked
    bool m_imageTooBig;

    QImage m_origImg; // image of the pixels in selection bound rect
    QTransform m_transform; // transformation to apply on origImg
    QTransform m_thumbToImageTransform;
    QTransform m_handlesTransform;

    QTransform m_paintingTransform;
    QPointF m_paintingOffset;

    QImage m_currImg; // origImg transformed using m_transform
    KisPaintDeviceSP m_selectedPortionCache;
    KisStrokeId m_strokeId;
    bool m_workRecursively;

    QPainterPath m_selectionPath; // original (unscaled) selection outline, used for painting decorations

    QSizeF m_refSize; // used in paint() to check if the view has changed (need to update m_currSelectionImg)

    KisToolTransformConfigWidget *m_optWidget;
    KisPaintDeviceSP m_target;
    // we don't need this origDevice for now
    // but I keep it here because I might use it when adding one of enkithan's suggestion (cut the seleted pixels instead of keeping them darkened)
    KisPaintDeviceSP m_origDevice;
    KisSelectionSP m_origSelection;
    //KisShapeSelection *m_previousShapeSelection;
    KisCanvas2 *m_canvas;

    // center used for rotation (calculated from rotationCenterOffset (in m_currentArgs))
    QVector3D m_rotationCenter;
    QPointF m_clickRotationCenterProj; // the rotation center projection at click
    QPointF m_rotationCenterProj;

    // information on the current selection
    QVector3D m_topLeft; //in image coords
    QVector3D m_topRight;
    QVector3D m_bottomLeft;
    QVector3D m_bottomRight;
    QVector3D m_middleLeft;
    QVector3D m_middleRight;
    QVector3D m_middleTop;
    QVector3D m_middleBottom;

    QPointF m_topLeftProj; // perspective projection of m_topLeft
    QPointF m_topRightProj;
    QPointF m_bottomLeftProj;
    QPointF m_bottomRightProj;
    QPointF m_middleLeftProj;
    QPointF m_middleRightProj;
    QPointF m_middleTopProj;
    QPointF m_middleBottomProj;

    QPointF m_clickTopLeftProj; // perspective projection of m_topLeft at click
    QPointF m_clickTopRightProj;
    QPointF m_clickBottomLeftProj;
    QPointF m_clickBottomRightProj;
    QPointF m_clickMiddleLeftProj;
    QPointF m_clickMiddleRightProj;
    QPointF m_clickMiddleTopProj;
    QPointF m_clickMiddleBottomProj;

    QPointF m_prevMousePos;
    QPointF m_clickPoint; //position of the mouse when click occurred

    // 'Free-transform'-related :
    QVector3D m_currentPlane, m_clickPlane; // vector (a, b, c) represents the vect plane a*x + b*y + c*z = 0
    double m_cosaZ; // cos of currentArgs.aZ()
    double m_sinaZ;
    double m_cosaX;
    double m_sinaX;
    double m_cosaY;
    double m_sinaY;
    double m_clickangle; // angle made from the rotationCenter at click
    // current scale factors
    // wOutModifiers don't take shift modifier into account
    double m_scaleX_wOutModifier;
    double m_scaleY_wOutModifier;

	// Warp-related :
    int m_defaultPointsPerLine;
    QVector<QPointF> m_viewTransfPoints;
    QVector<QPointF> m_viewOrigPoints;
	bool m_cursorOverPoint;
	int m_pointUnderCursor; // the id of the point in the vector

    bool m_isActive;

    TransformTransactionProperties m_transaction;
    TransformChangesTracker m_changesTracker;


private:
    QPointF imageToFlake(const QPointF &pt);
    QPointF flakeToImage(const QPointF &pt);

    QRectF imageToFlake(const QRectF &pt);
    QRectF flakeToImage(const QRectF &pt);

    QPointF imageToThumb(const QPointF &pt, bool useFlakeOptimization);

private slots:
    void slotTrackerChangedConfig();
    void slotUiChangedConfig();
    void slotApplyTransform();
    void slotResetTransform();
    void slotRestartTransform();
    void slotEditingFinished();
};

class KisToolTransformFactory : public KoToolFactoryBase
{
public:

    KisToolTransformFactory(const QStringList&)
            : KoToolFactoryBase("KisToolTransform") {
        setToolTip(i18n("Transform a layer or a selection"));
        setToolType(TOOL_TYPE_TRANSFORM);
        setIconName(koIconNameCStr("krita_tool_transform"));
        setShortcut(KShortcut( QKeySequence(Qt::CTRL + Qt::Key_T) ));
        setPriority(11);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    virtual ~KisToolTransformFactory() {}

    virtual KoToolBase * createTool(KoCanvasBase *canvas) {
        return new KisToolTransform(canvas);
    }

};



#endif // KIS_TOOL_TRANSFORM_H_

