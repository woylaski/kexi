// -*- Mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2002-2005 Thorsten Zachmann <zachmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/


#ifndef __KPRCANVAS__
#define __KPRCANVAS__

#include <qwidget.h>
#include <qptrlist.h>
#include <qpicture.h>
#include <qvaluelist.h>
#include <qpixmap.h>
#include <qpointarray.h>
#include <qvaluevector.h>
#include <qtimer.h>

#include <koGuides.h>
#include <koRuler.h>
#include <koQueryTrader.h>
#include "koPointArray.h"
#include "global.h"
#include <koRect.h>
#include <kotextformat.h>
#include "presstep.h"

class KURL;
class KoTextFormatInterface;
class KPresenterView;
class KPresenterDoc;
class QPainter;
class KTextEdit;
class KPTextObject;
class KPObject;
class KPopupMenu;
class QResizeEvent;
class QPaintEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QFocusEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class KPPixmapObject;
class KPBackGround;
class QRect;
class QSize;
class QPoint;
class KPresenterSoundPlayer;
class KoTextObject;
class KoParagCounter;
class KPrPage;
class KPrinter;
class KPTextView;
class KPPartObject;
class KCommand;
class EffectHandler;
class KPPageEffects;
/**
 * Class KPCanvas - There is a single instance of this class for a given view.
 *
 * It manages the graphical representation of all the objects.
 */
class KPrCanvas : public QWidget
{
    Q_OBJECT

public:

    /// constructor
    KPrCanvas( QWidget *parent=0,const char *name=0,KPresenterView *_view=0 );
    /// destructor
    ~KPrCanvas();

    KPresenterView * getView()const { return m_view; }

    // The equivalent of contentsX() and contentsY() for a QScrollView
    // The Canvas is a widget, not a scrollview (because we handle the scrollbars ourselves,
    // to have the "goto page" buttons at the bottom of the scrollbars etc.)
    int diffx() const { return m_xOffset; }
    int diffy() const { return m_yOffset; }
    void setDiffX( int _diffX ) { m_xOffset = _diffX; }
    void setDiffY( int _diffY ) { m_yOffset = _diffY; }
    void scrollX( int x );
    void scrollY( int y );

    // public functions
    void selectAllObj();
    void deSelectAllObj();
    void selectObj( KPObject* );
    void deSelectObj( KPObject* );
    void setTextFormat(const KoTextFormat &format, int flags);

    void setTextColor( const QColor & );
    void setTextBackgroundColor( const QColor & );
    void setTextAlign( int );
    void setTextDepthPlus();
    void setTextDepthMinus();
    void setNewRightIndent(double _rightIndent);
    void setNewLeftIndent(double _leftIndent);
    void setNewFirstIndent(double _firstIndent);
    //void setTextCounter(KoParagCounter counter);
    void setTabList( const KoTabulatorList & tabList );
    void setTextBold( bool b );
    void setTextItalic( bool b );
    void setTextUnderline( bool b );
    void setTextStrikeOut( bool b );
    void setTextFamily( const QString &f );
    void setTextPointSize( int s );
    void setTextSubScript( bool b );
    void setTextSuperScript( bool b );
    void setTextDefaultFormat( );
    void setIncreaseFontSize();
    void setDecreaseFontSize();
    KPTextObject* kpTxtObj() const;

    /**
     * Start a screen presentation
     * All pages if @p curPgNum is -1, otherwise just @p curPgNum (1-based)
     */
    void startScreenPresentation( double zoomX, double zoomY, int curPgNum = -1);
    void stopScreenPresentation();

    /**
     * Go to the next step in the presentation.
     * gotoNextPage: if is set to true go to beginning of the
     *               next page.
     */
    bool pNext( bool gotoNextPage = false );

    /**
     * Go to previous step of the presentation.
     * gotoPreviousPage: if is set to true go to beginning of the
     *                   previous page.
     */
    bool pPrev( bool gotoPreviousPage = false );

    /// returns the current page of the presentation 1 based
    unsigned int presPage() const { return m_step.m_pageNumber + 1; }
    /// returns the current step of the presentation
    int presStep() const { return m_step.m_step; }
    int numPresSteps() const { return m_pageEffectSteps.count(); }
    int numPresPages() const { return m_presentationSlides.count(); }

    bool canAssignEffect( QPtrList<KPObject> &objs ) const;

    void print( QPainter*, KPrinter*, float, float );


    void setPartEntry( KoDocumentEntry& _e )
        { partEntry = _e; }
    void setToolEditMode( ToolEditMode _m, bool updateView = true );
    void setAutoForm( const QString &_autoform )
        { autoform = _autoform; }

    /**
    \brief Draw page into QPixmap.

    Draws page pgnum of the currently open presentation into a QPixmap
    using the specified zoom factor (or fixed width/height dimensions, resp.)

    Set forceWidth and/or forceHeight to override the zoom factor
    and obtain a pixmap of the specified width and/or height.
    By omitting one of them you make sure that the aspect ratio
    of your page is used for the resulting image.

    \sa exportPage
    */
    void drawPageInPix( QPixmap&, int pgnum, int zoom,
                        bool forceRealVariableValue = false,
                        int forceWidth  = 0,
                        int forceHeight = 0 );

    /**
     \brief Save page to bitmap file.

     Export a page of the currently open presentation to disk
     using a bitmap format like e.g. PNG.
     This method uses a QPixmap::save() call.

     \param nPage the internally used <b>0-based</b> page number
     \param nWidth the desired image width in px
     \param nHeight the desired image height in px
     \param fileURL the URL of the image file to be created: if this
       does not point to a local file a KTempFile is created by QPixmap::save()
       which is then copied over to the desired location.
     \param format the format of the image file (see QPixmap::save())
      \param quality the quality of the image (see QPixmap::save())

     example:
\verbatim
exportPage( 0, s, 800, 600, "/home/khz/page0.png", "PNG", 100 );
\endverbatim
     \returns True if the file was written successfully.

     \sa drawPageInPix, KPresenterViewIface::exportPage
     */
    bool exportPage( int nPage, int nWidth, int nHeight,
                     const KURL& fileURL,
                     const char* format,
                     int quality = -1 );

    void gotoPage( int pg );

    /**
     * Go to first slide of presentation.
     * Used in presentation mode.
     */
    void presGotoFirstPage();

    KPrPage* activePage() const;

    bool oneObjectTextExist() const;
    bool oneObjectTextSelected() const;
    bool isOneObjectSelected() const;
    /** Returns the list of selected text objects */
    QPtrList<KPTextObject> selectedTextObjs() const;
    /**
     * Returns the list of interfaces to use for changing the text format.
     * This can be either the currently edited text object,
     * or the list of text objects currently selected
     */
    QPtrList<KoTextFormatInterface> applicableTextInterfaces() const;
    /**
     * Returns the list of text objects, either selected or activated.
     */
    QPtrList<KPTextObject> applicableTextObjects() const;

    void setMouseSelectedObject(bool b);

    bool haveASelectedPartObj() const;
    bool haveASelectedGroupObj() const;
    bool haveASelectedPixmapObj() const;

    KPTextView *currentTextObjectView() const { return m_currentTextObjectView; }

    QPtrList<KPObject> objectList() const;

    // get - set data
    const QPtrList<KPObject> & getObjectList() const;

    void playSound( const QString &soundFileName );
    void stopSound();

    ///for KPTextView
    void dragStarted() { mousePressed = false; }

    void setXimPosition( int x, int y, int w, int h, QFont *f );
    void createEditing( KPTextObject *textObj );


    /**
     * @brief Add a line
     *
     * @param startPoint the start point of the line
     * @param endPoint the end point of the line
     */
    void insertLine( const KoPoint &startPoint, const KoPoint &endPoint );
    
    /**
     * @brief Add a rectangle
     *
     * @param rect the size and position of the rectangle
     */
    void insertRect( const KoRect &rect);

    /**
     * @brief Add a ellipse
     *
     * @param rect the size and position of the ellipse
     */
    void insertEllipse( const KoRect &rect );

    /**
     * @brief Add a pie/arc/chord
     *
     * @param rect the size and position of the pie/arc/chord
     */
    void insertPie( const KoRect &rect );

    /**
     * @brief Add a text object
     *
     * @param rect the size and position of the text object
     */
    KPTextObject* insertTextObject( const KoRect &rect );

    /**
     * @brief Add a picture
     *
     * @param rect the size and position of the picture
     */
    void insertPicture( const KoRect &rect );
    void changePicture( const KURL & url, QWidget *window );

    int numberOfObjectSelected() const;
    KPObject *getSelectedObj() const;
    KPPixmapObject *getSelectedImage() const;

    bool getEditMode() const { return editMode; }

    void ungroupObjects();
    void groupObjects();

    unsigned int objNums() const;

    void ensureVisible( int x, int y, int xmargin, int ymargin );

    /**
     * @brief get the rect for the objects
     *
     * @param all true if the rect for all objects shoud be returned
     *        false if only the rect for selected objects sould be returned
     *
     * @return rect of the objects
     */
    KoRect objectRect( bool all ) const;

    void scrollTopLeftPoint( const QPoint & pos );

    void textContentsToHeight();
    void textObjectToContents();

    void flipObject( bool _horizontal );

    void lowerObjs( bool backward );
    void raiseObjs( bool forward );

    /**
     * Align all selected and not protected objects on the
     * active and on the sticky page according to the AlignType
     */
    void alignObjects( AlignType at );

    bool canMoveOneObject()const;
    QPtrList<KPTextObject> listOfTextObjs() const;

    QPixmap getPicturePixmap() const;
    void closeObject(bool _close);
    void layout();
    void alignVertical( VerticalAlignmentType _type );
    void savePicture();

    void dropImage( QMimeSource * data, bool resizeImageToOriginalSize = false , int posX = 10, int posX=10 );
    KoGuides & guideLines() { return m_gl; }

public slots:
    void exitEditMode();

    void clipCut();
    void clipCopy();
    void clipPaste();
    void deleteObjs();
    void copyObjs();
    void copyOasisObjs();

    void rotateObjs();
    void shadowObjs();

    void chPic();
    void picViewOriginalSize();
    void picViewOrig640x480();
    void picViewOrig800x600();
    void picViewOrig1024x768();
    void picViewOrig1280x1024();
    void picViewOrig1600x1200();
    void picViewOrigFactor();
    void setActivePage( KPrPage* active );

    /**
     * @brief Set the guide move state
     *
     * @param state true set guide move state
     *              false unset guide move state
     */
    void setMoveGuides( bool state );

signals:

    // signals to notify of changes
    void fontChanged( const QFont & );
    void colorChanged( const QColor & );
    void alignChanged( int );
    /**
     * Emmitted when presentation should be restarted
     * This is used in automatic screen and infinite loop mode.
     */
    void stopAutomaticPresentation();
    /**
     * Emmitted when presentation should be restarted
     * This is used in automatic screen and infinite loop mode.
     */
    void restartPresentation();
    void objectSelectedChanged();
    void objectSizeChanged();
    /// Emitted when the current frameset edit changes
    void currentObjectEditChanged();

    void selectionChanged( bool hasSelection );
    void sigMouseWheelEvent( QWheelEvent * );

protected:
    struct PicCache
    {
        QPicture pic;
        int num;
        int subPresStep;
    };

    // functions for displaying
    /**
     * This method is used for repainting the canvas.
     */
    virtual void paintEvent( QPaintEvent* );

    /**
     * Draws the background of page to painter.
     * Set edit to true if in edit mode, false in presentation mode.
     */
    void drawBackground( QPainter* painter, const QRect& rect, KPrPage * page, bool edit = false ) const;

    /**
     * Draw obj to painter.
     */
    void drawAllObjectsInPage( QPainter *painter, const QPtrList<KPObject> & obj, int pageNum ) const;

    /**
     * Draw _objects shown at step to painter.
     * This method is used for presentation mode, printing.
     */
    void drawObjectsPres( QPainter *painter, const QPtrList<KPObject> &_objects, PresStep step ) const;

    /**
     * Draw _objects to painter.
     * Only rect is painted.
     * This method is used for edit mode.
     */
    void drawObjectsEdit( QPainter *painter, const KoRect &rect, const QPtrList<KPObject> &_objects,
                          SelectionMode selectionMode, int page ) const;

    /**
     * Draw _objects to painter.
     * contour if true only countor of selected objects is drawn
     * selectionMode selected objects use this selection mode
     * textView if set print editable textobject (used by drawObjectsEdit)
     * This method is used by drawObjectsPres and drawObjectsEdit.
     */
    void drawObjects( QPainter *painter, const QPtrList<KPObject> &objects, SelectionMode selectionMode,
                      bool contour, KPTextView * textView, int pageNum ) const;

    /**
     * Draw _objects of page to painter.
     * This method is used for edit mode.
     */
    void drawEditPage( QPainter *painter, const QRect &_rect,
                       KPrPage *page, SelectionMode selectionMode ) const;

    /**
     * Draw _objects of page shown at step to painter.
     * This method is used for presentation mode, printing.
     */
    void drawPresPage( QPainter *painter, const QRect &_rect, PresStep step ) const;

    /// draw grid
    void drawGrid(QPainter *painter, const QRect &rect2) const;

    /**
     * Finish the object effects.
     * This shown the last step of the effect. It stops the effect timer and
     * disconnect it and the effect handler deleted.
     */
    bool finishObjectEffects();

    bool finishPageEffect( bool cancel = false );

    void drawCurrentPageInPix( QPixmap& ) const;
    virtual void mousePressEvent( QMouseEvent *e );
    virtual void mouseReleaseEvent( QMouseEvent *e );
    virtual void mouseMoveEvent( QMouseEvent *e );
    virtual void mouseDoubleClickEvent( QMouseEvent *e );
    virtual void wheelEvent( QWheelEvent *e );
    virtual void resizeEvent( QResizeEvent *e );
    /**
     * Returns the object at position pos
     * @param pos the position in the document on which to look
     * @param withoutProtected if set to true only unprotecred objects are returned
     * @return the object at position pos
     */
    KPObject *getObjectAt( const KoPoint &pos, bool withoutProtected = false );
    virtual void focusInEvent( QFocusEvent* ) {}
    virtual void focusOutEvent( QFocusEvent* ) {}
    virtual void enterEvent( QEvent *e );
    virtual void leaveEvent( QEvent *e );
    virtual void dragEnterEvent( QDragEnterEvent *e );
    virtual void dragMoveEvent( QDragMoveEvent *e );
    virtual void dropEvent( QDropEvent *e );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void keyReleaseEvent( QKeyEvent *e );
    virtual void imStartEvent( QIMEvent * e );
    virtual void imComposeEvent( QIMEvent * e );
    virtual void imEndEvent( QIMEvent * e );

    void eraseEmptySpace( QPainter * painter, const QRegion & emptySpaceRegion, const QBrush & brush ) const;

    /// setup popupmenus
    void setupMenus();

    unsigned int currPgNum() const;
    QColor txtBackCol() const;
    bool spInfiniteLoop() const;
    bool spManualSwitch() const;
    QRect getPageRect( bool decBorders = true ) const;
    unsigned int pageNums() const;
    void calcRatio( double &dx, double &dy, ModifyType _modType, double ratio ) const;

    void _repaint( bool erase=true );
    void _repaint( const QRect &r );
    void _repaint( KPObject *o );

    /**
     * Draw page defined in step to painter.
     * This method is used for printing a presentation.
     */
    void printPage( QPainter*, PresStep step, KPrinter *printer, int rows, int cols );

    /**
     * This method animates the objects in the presentation.
     */
    void doObjEffects( bool isAllreadyPainted = false );

    /**
     * @brief Add a object
     *
     * @param rect the size and position of the object
     */
    KPPartObject *insertObject( const KoRect &rect );

    /**
     * @brief Add a autoform object
     *
     * @param rect the size and position of the autoform object
     */
    void insertAutoform( const KoRect &rect );
    void insertFreehand( const KoPointArray &_pointArray );
    void insertPolyline( const KoPointArray &_pointArray );
    void insertCubicBezierCurve( const KoPointArray &_pointArray );
    void insertPolygon( const KoPointArray &_pointArray );
    void insertClosedLine( const KoPointArray &_pointArray );

    void selectNext();
    void selectPrev();

    void scalePixmapToBeOrigIn( const KoSize &currentSize, const KoSize &pgSize,
                                const QSize &presSize, KPPixmapObject *obj );
    bool getPixmapOrigAndCurrentSize( KPPixmapObject *&obj, KoSize *origSize, KoSize *currentSize );
    void setTextBackground( KPTextObject *obj );
    virtual bool eventFilter( QObject *o, QEvent *e );
    virtual bool focusNextPrevChild( bool );

    void endDrawPolyline();

    void drawCubicBezierCurve( int _dx, int _dy );
    void endDrawCubicBezierCurve();


#ifndef NDEBUG
    void printRTDebug( int info );
#endif

    /** 
     * @brief Scroll the canvas by diff
     *
     * This causes a processing of all posted events before the scrolling
     * happens.
     *
     * @param diff the distance the canvas should be scrolled
     */
    void scrollCanvas( const KoPoint & diff );

    KoRect getAlignBoundingRect() const;

private:
    void picViewOrigHelper(int x, int y);

    /**
     * @brief Move object by key
     *
     * @param x The distance to move the objects in x direction
     * @param y The distance to move the objects in y direction
     */
    void moveObjectsByKey( int x, int y );

    /**
     * @brief Move object by mouse
     *
     * @param pos The position of the mouse
     */
    void moveObjectsByMouse( KoPoint &pos );

    //---- stuff needed for resizing ----
    /// resize the m_resizeObject
    void resizeObject( ModifyType _modType, int _dx, int _dy, bool doApplyGrid );
    /// create ResizeCmd
    void finishResizeObject( const QString &name, int mx, int my, bool layout = true );

    void raiseObject( KPObject *_kpobject );
    void lowerObject();
    int selectedObjectPosition;

    /**
     * This is used in automatic presentation mode.
     * If it is set to true and we are in automatic presentation
     * mode the timer of the page will be activated after the last
     * effect has been shown.
     */
    bool m_setPageTimer;

    /// Helper function to draw a polygon
    void drawPolygon( const KoRect &rect );

    /// Helper function to draw a pie/arc/chord
    void drawPieObject( QPainter *p,  const KoRect &rect );
    void limitSizeOfObject();

    QPoint limitOfPoint(const QPoint& _point) const;

    /**
     * Return true if object is a header/footer which is hidden.
     */
    bool objectIsAHeaderFooterHidden(KPObject *obj) const;

    KPTextObject* textUnderMouse( const QPoint & point );
    bool checkCurrentTextEdit( KPTextObject * textObj );

    void rectSymetricalObjet();
    /**
     * @brief Snap @p pos to grid and guide lines.
     *
     * This method looks if the @p pos has to be snapped to a guide or grid or both. 
     * If so it returns the position it has snapped to.
     *
     * @param pos the point which should be snapped
     *
     * @return the position of the snapped point
     */
    KoPoint snapPoint( KoPoint &pos );
    QPoint applyGrid( const QPoint &pos,bool offset);
    KoPoint applyGrid( const KoPoint &pos);
    double applyGridX( double x );
    double applyGridY( double y );
    int applyGridOnPosX( int pos ) const;
    int applyGridOnPosY( int pos ) const;

    /**
     * @brief Find the next grid in distance
     *
     * Allways the top left corner is snapped to the grid.
     *
     * @param rect The rect which should be snapped to the grid.
     * @param diffx The range in x distance in which the grid has to be.
     * @param diffy The range in y distance in which the grid has to be.
     *
     * @return the distance to the guide or (  0, 0 ) if there is no guide to snap to.
     */
    KoPoint diffGrid( KoRect &rect, double diffx, double diffy );

    void recalcAutoGuides( );

    /**
     * @brief Calculate the points in distance to the top left corner
     *
     * @param pointArray the points in coordinate position
     */
    KoPointArray getObjectPoints( const KoPointArray &pointArray );

private slots:
    void toFontChanged( const QFont &font ) { emit fontChanged( font ); }
    void toColorChanged( const QColor &color ) { emit colorChanged( color ); }
    void toAlignChanged( int a ) { emit alignChanged( a ); }
    /**
     * Switch to drawing mode. In drawing mode it is possible to draw on the
     * current slide.
     * Only to be used in presentation mode.
     */
    void setDrawingMode();
    /**
     * Set switching mode. This ends drawing mode and goes back to
     * stepping between the pages.
     * If continueTimer is true the autoPres timer is continued
     * Only to be used in presentation mode.
     */
    void setSwitchingMode( bool continueTimer = true );
    void slotGotoPage();
    void slotExitPres();
    void terminateEditing( KPTextObject * );

    /**
     * Do the next step of the object effect.
     * This restarts the effect tmer. If the effects are
     * completed the timer is disconnected and the effect handler
     * deleted.
     */
    void slotDoEffect();

    void slotDoPageEffect();

private:
    // variables
    /// Popup menu used in presentation mode.
    KPopupMenu *m_presMenu;
    bool showingLastSlide;
    bool mousePressed;
    bool drawContour;
    ModifyType modType;
    /**
     * Saves the last mouse position during mouse move events.
     */
    QPoint m_savedMousePos;

    KPObject *editNum;

    bool fillBlack;
    KPresenterView *m_view;
    bool editMode, goingBack;
    /**
     * True when we are in drawing mode.
     * False when we are in switching mode.
     */
    bool m_drawMode;
    /**
     * True if the a line should be drawn.
     * Used in drawing mode.
     */
    bool m_drawLineInDrawMode;

    /**
     * Save the lines drawed in drawMode.
     * This is used for paint events.
     * Used in drawing mode.
     */
    QValueVector<QPointArray> m_drawModeLines;

    /**
     * Index into the QPointArray for the next point of the line in draw mode.
     * Used in drawing mode.
     */
    int m_drawModeLineIndex;

    bool mouseSelectedObject;
    /// information about current step of the presentation
    PresStep m_step;
    float _presFakt;
    int m_showOnlyPage; // 1-based (-1 = all)
    /// list of all effect steps occuring on the active page
    QValueList<int> m_pageEffectSteps;
    /// List of the slides used in the presentation
    QValueList<int> m_presentationSlides;
    /// Iterator over the slides of a presentation
    QValueList<int>::Iterator m_presentationSlidesIterator;
    /// EffectHandler for object effects
    EffectHandler *m_effectHandler;

    KPPageEffects *m_pageEffect;

    /// EffectTimer
    QTimer m_effectTimer;

    QTimer m_pageEffectTimer;

    /// menu identifier for draw mode
    int PM_DM;
    int firstX, firstY;
    int delPageId;
    bool drawRubber;
    QRect rubber;

    //---- stuff needed for resizing ----
    /// object which gets resized
    KPObject *m_resizeObject;
    /// size of the object at when resizing is started
    KoRect m_rectBeforeResize;
    /// should the ratio of the object kept during resize
    bool m_keepRatio;
    /// ratio of the object ( width / height )
    double m_ratio;
    bool m_isResizing;

    //---- stuff needed for moving ----
    bool m_isMoving;
    KoPoint m_moveStartPoint;

    //---- stuff needed for rotating ----
    /// object which gets rotated
    KPObject *m_rotateObject;
    /// center of the rotated object
    KoPoint m_rotateCenter;
    /// angle of the object at start of rotate
    double m_angleBeforeRotate;

    ToolEditMode toolEditMode;
    QRect insRect;
    /// The rect of the object during insert.
    KoRect m_insertRect;
    KoDocumentEntry partEntry;
    QString autoform;
    QPixmap buffer;

    KPTextView *m_currentTextObjectView;

    KPresenterSoundPlayer *soundPlayer;

    KoPointArray m_pointArray, m_oldCubicBezierPointArray;
    QPoint m_dragStartPoint, m_dragEndPoint, m_dragSymmetricEndPoint;
    /// The start position of an insert for line objects
    KoPoint m_startPoint;
    /// The end position of an insert for line objects
    KoPoint m_endPoint;
    KoPoint m_CubicBezierSecondPoint, m_CubicBezierThirdPoint;
    unsigned int m_indexPointArray;
    bool m_drawPolyline;
    bool m_drawCubicBezierCurve;
    bool m_drawLineWithCubicBezierCurve;
    bool m_zoomRubberDraw;

    KPrPage *m_activePage;
    int m_xOffset, m_yOffset;
    int m_xOffsetSaved, m_yOffsetSaved; // saved when going fullscreen

    /// Start position for move with mouse
    KoPoint m_moveStartPosMouse;
    /// This holds the distance it an object was snapped to a guide
    KoPoint m_moveSnapDiff;
    /// This is set to true if the snap logic has to be reverted (by pressing shift during move)
    bool m_changeSnap;
    /// The last position of the mouse during moving
    KoPoint m_origMousePos;
    /// start position for move with key
    KoPoint m_moveStartPosKey; 

    KPrPage * m_activePageBeforePresentation;
    int m_zoomBeforePresentation;

    bool m_keyPressEvent;
    bool m_drawSymetricObject;

    /// guides
    KoGuides m_gl;
    /// guides - the auto style ones
    KoGuides m_autoGl;
    /// true if we are in guide move state, false otherwise
    bool m_moveGuides;
};

#endif // __KPRCANVAS__
