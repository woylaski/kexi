/*
 *  kis_layerview.cc - part of Krayon
 *
 *  Copyright (c) 1999 Andrew Richards <A.Richards@phys.canterbury.ac.nz>
 *                1999 Michael Koch    <koch@kde.org>
 *                2000 Matthias Elter  <elter@kde.org>
 *                2001 John Califf
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>
#include <qslider.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qlineedit.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qstyle.h>

#include <kstandarddirs.h>
#include <iostream>
#include <klocale.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#include "kis_doc.h"
#include "kis_view.h"
#include "kis_util.h"
#include "kis_layerview.h"
#include "kis_factory.h"
#include "kis_framebutton.h"
#include "integerwidget.h"

//#define KISBarIcon( x ) BarIcon( x, KisFactory::global() )

using namespace std;
const int iheight = 32;

KisLayerView::KisLayerView( KisDoc *doc, QWidget *parent, const char *name )
  : QWidget( parent, name )
{
    buttons = new QHBox( this );
    buttons->setMaximumHeight(12);

    pbAddLayer = new KisFrameButton( buttons );
    pbAddLayer->setPixmap( BarIcon( "newlayer" ) );

    pbRemoveLayer = new KisFrameButton( buttons );
    pbRemoveLayer->setPixmap( BarIcon( "deletelayer" ) );

    pbUp = new KisFrameButton( buttons );
    pbUp->setPixmap( BarIcon( "raiselayer" ) );

    pbDown = new KisFrameButton( buttons );
    pbDown->setPixmap( BarIcon( "lowerlayer" ) );

    // only serves as beautifier for the widget
    frame = new QHBox( this );
    frame->setFrameStyle( QFrame::Panel | QFrame::Sunken );

    layertable = new LayerTable( doc, frame, this, "layerlist" );

    connect( pbAddLayer, SIGNAL( clicked() ),
        layertable, SLOT( slotAddLayer() ) );
    connect( pbRemoveLayer, SIGNAL( clicked() ),
        layertable, SLOT( slotRemoveLayer() ) );
    connect( pbUp, SIGNAL( clicked() ),
        layertable, SLOT( slotRaiseLayer() ) );
    connect( pbDown, SIGNAL( clicked() ),
        layertable, SLOT( slotLowerLayer() ) );

    QToolTip::add( pbAddLayer, i18n( "Create New Layer" ) );
    QToolTip::add( pbRemoveLayer, i18n( "Remove Current Layer" ) );
    QToolTip::add( pbUp, i18n( "Upper Current Layer" ) );
    QToolTip::add( pbDown, i18n( "Lower Current Layer" ) );

    initGUI();
}

void KisLayerView::initGUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this, 2);
    QHBoxLayout *buttonsLayout = new QHBoxLayout( buttons, 4 );

    buttonsLayout->addWidget(pbAddLayer);
    buttonsLayout->addWidget(pbRemoveLayer);
    buttonsLayout->addWidget(pbUp);
    buttonsLayout->addWidget(pbDown);

    mainLayout->addWidget( frame);
    mainLayout->addWidget( buttons);
}

KisLayerView::~KisLayerView()
{
    delete pbAddLayer;
    delete pbRemoveLayer;
    delete pbUp;
    delete pbDown;
    delete buttons;
    delete layertable;
    delete frame;
}

void KisLayerView::showScrollBars( )
{
    resizeEvent(0L);
}

LayerTable::LayerTable( QWidget* parent, const char* name )
  : QtTableView( parent, name )
{
    pLayerView = 0L;
    init( 0 );
}


LayerTable::LayerTable( KisDoc* doc, QWidget* parent, const char* name )
  : QtTableView( parent, name )
{
    pLayerView = 0L;
    init( doc );
}

LayerTable::LayerTable( KisDoc* doc, QWidget* parent,
KisLayerView *layerview, const char* name )
  : QtTableView(parent, name )
{
    pLayerView = layerview;
    init( doc );
}

void LayerTable::init( KisDoc* doc)
{
    setTableFlags(Tbl_autoVScrollBar | Tbl_autoHScrollBar);

    m_doc = doc;
    setBackgroundColor( white );

    // load icon pixmaps
    QString icon = locate( "kis_pics", "visible.png", KisFactory::global());
    mVisibleIcon = new QPixmap;
    if( !mVisibleIcon->load( icon ) )
	    KMessageBox::error( this, "Can't find visible.png", "Canvas" );
    mVisibleRect = QRect( QPoint( 3, (iheight - 24)/2), QSize(24,24));

    icon = locate( "kis_pics", "novisible.png",
        KisFactory::global() );
    mNovisibleIcon = new QPixmap;
    if( !mNovisibleIcon->load( icon ) )
	    KMessageBox::error( this, "Can't find novisible.png", "Canvas" );

    icon = locate( "kis_pics", "linked.png", KisFactory::global() );
    mLinkedIcon = new QPixmap;
    if( !mLinkedIcon->load( icon ) )
	    KMessageBox::error( this, "Can't find linked.png", "Canvas" );
    mLinkedRect = QRect(QPoint(30, (iheight - 24)/2), QSize(24,24));

    icon = locate( "kis_pics", "unlinked.png", KisFactory::global() );
    mUnlinkedIcon = new QPixmap;
    if( !mUnlinkedIcon->load( icon ) )
	    KMessageBox::error( this, "Can't find unlinked.png", "Canvas" );
    mPreviewRect = QRect(QPoint(57, (iheight - 24)/2), QSize(24,24));

    updateTable();

    setCellWidth( CELLWIDTH );
    setCellHeight( iheight );
    m_selected = m_doc->current()->layerList().count() - 1;

    QPopupMenu *submenu = new QPopupMenu();

    submenu->insertItem( i18n( "Upper" ), UPPERLAYER );
    submenu->insertItem( i18n( "Lower" ), LOWERLAYER );
    submenu->insertItem( i18n( "Foremost" ), FRONTLAYER );
    submenu->insertItem( i18n( "Hindmost" ), BACKLAYER );

    m_contextmenu = new QPopupMenu();

    m_contextmenu->setCheckable(TRUE);

    m_contextmenu->insertItem( i18n( "Visible" ), VISIBLE );
    m_contextmenu->insertItem( i18n( "Selection"), SELECTION );
    m_contextmenu->insertItem( i18n( "Level" ), submenu );
    m_contextmenu->insertItem( i18n( "Linked"), LINKING );
    m_contextmenu->insertItem( i18n( "Properties"), PROPERTIES );

    m_contextmenu->insertSeparator();

    m_contextmenu->insertItem( i18n( "Add Layer" ), ADDLAYER );
    m_contextmenu->insertItem( i18n( "Remove Layer"), REMOVELAYER );
    m_contextmenu->insertItem( i18n( "Add Mask" ), ADDMASK );
    m_contextmenu->insertItem( i18n( "Remove Mask"), REMOVEMASK );

    connect( m_contextmenu, SIGNAL( activated( int ) ),
        SLOT( slotMenuAction( int ) ) );
    connect( submenu, SIGNAL( activated( int ) ),
        SLOT( slotMenuAction( int ) ) );
    connect( doc, SIGNAL( layersUpdated()),
        this, SLOT( slotDocUpdated () ) );

    setAutoUpdate(true);
}


void LayerTable::slotDocUpdated()
{
    updateTable();
    updateAllCells();
    if(pLayerView) pLayerView->showScrollBars();
}


void LayerTable::paintCell( QPainter* painter, int row, int )
{
    if( row == m_selected )
    {
        painter->fillRect( 0, 0,
            cellWidth(0) - 1, cellHeight() - 1, gray);
    }
    else
    {
        painter->fillRect( 0, 0,
            cellWidth(0) - 1, cellHeight() - 1, lightGray);
    }

    style().drawPrimitive( QStyle::PE_Panel, painter,
                           QRect( mVisibleRect.x(), mVisibleRect.y(),
                                  mVisibleRect.width(), mVisibleRect.height() ),
                           colorGroup() ); //, true );

    QPoint pt = QPoint(mVisibleRect.left() + 2, mVisibleRect.top() + 2);
    if( m_doc->current()->layerList().at(row)->visible() )
    {
        painter->drawPixmap( pt, *mVisibleIcon );
    }
    else
    {
        painter->drawPixmap( pt, *mNovisibleIcon );
    }

    style().drawPrimitive( QStyle::PE_Panel, painter,
                           QRect( mLinkedRect.x(), mLinkedRect.y(),
                                  mLinkedRect.width(), mLinkedRect.height() ),
                           colorGroup() ); // , true );

    pt = QPoint(mLinkedRect.left() + 2, mLinkedRect.top() + 2);
    if( m_doc->current()->layerList().at(row)->linked() )
    {
        painter->drawPixmap( pt, *mLinkedIcon );
    }
    else
    {
        painter->drawPixmap( pt, *mUnlinkedIcon );
    }

    style().drawPrimitive( QStyle::PE_Panel, painter,
                           QRect( mPreviewRect.x(), mPreviewRect.y(),
                                  mPreviewRect.width(), mPreviewRect.height() ),
                           colorGroup() ); //, true );

    painter->drawRect(0, 0, cellWidth(0) - 1, cellHeight() - 1);
    painter->drawText(iheight * 3 + 3*3, 20,
        m_doc->current()->layerList().at(row)->name());
}



void LayerTable::updateTable()
{
    if( m_doc->current() )
    {
        m_items = m_doc->current()->layerList().count();
        setNumRows( m_items );
        setNumCols( 1 );
    }
    else
    {
        m_items = 0;
        setNumRows( 0 );
        setNumCols( 0 );
    }

    resize( sizeHint() );
    if(pLayerView) pLayerView->showScrollBars();
    repaint();
}


void LayerTable::update_contextmenu( int indx )
{
    m_contextmenu->setItemChecked( VISIBLE,
        m_doc->current()->layerList().at(indx)->visible() );
    m_contextmenu->setItemChecked( LINKING,
        m_doc->current()->layerList().at(indx)->linked() );
}

/*
    makes this the current layer and highlites in gray
*/
void LayerTable::selectLayer( int indx )
{
    int currentSel = m_selected;
    m_selected = -1;

    updateCell( currentSel, 0 );
    m_selected = indx;
    m_doc->current()->setCurrentLayer( m_selected );
    updateCell( m_selected, 0 );
}


void LayerTable::slotInverseVisibility( int indx )
{
  KisImage *img = m_doc->current();
  img->layerList().at(indx)->setVisible(!img->layerList().at(indx)->visible());
  updateCell( indx, 0 );

  img->markDirty(img->layerList().at( indx )->imageExtents() );

  m_doc->setModified( true );
}


void LayerTable::slotInverseLinking( int indx )
{
    KisImage *img = m_doc->current();
    img->layerList().at(indx)->setLinked(!img->layerList().at(indx)->linked());
    updateCell( indx, 0 );

    m_doc->setModified( true );
}


void LayerTable::slotMenuAction( int id )
{
    switch( id )
    {
        case VISIBLE:
            slotInverseVisibility( m_selected );
            break;
        case LINKING:
            slotInverseLinking( m_selected );
            break;
        case PROPERTIES:
            slotProperties();
            break;
        case ADDLAYER:
            slotAddLayer();
            break;
        case REMOVELAYER:
            slotRemoveLayer();
            break;
        case UPPERLAYER:
            slotRaiseLayer();
            break;
        case LOWERLAYER:
            slotLowerLayer();
            break;
        case FRONTLAYER:
            slotFrontLayer();
            break;
        case BACKLAYER:
            slotBackgroundLayer();
            break;
        default:
            break;
    }
}


QSize LayerTable::sizeHint() const
{
    if(pLayerView)
        return QSize( CELLWIDTH, pLayerView->getFrame()->height());
    else
        return QSize( CELLWIDTH, iheight * 5 );
}


void LayerTable::mousePressEvent( QMouseEvent *ev)
{
    int row = findRow( ev->pos().y() );
    QPoint localPoint( ev->pos().x() % cellWidth(),
        ev->pos().y() % cellHeight() );

    if( ev->button() & LeftButton )
    {
        if( mVisibleRect.contains( localPoint ) )
        {
            slotInverseVisibility( row );
        }
        else if( mLinkedRect.contains( localPoint ) )
        {
            slotInverseLinking( row );
        }
        else if( row != -1 )
        {
            selectLayer( row );
        }
    }
    else if( ev->button() & RightButton )
    {
        if( row != -1)
        {
            selectLayer( row );
            update_contextmenu( row );
            m_contextmenu->popup( mapToGlobal( ev->pos() ) );
        }
    }
}


void LayerTable::mouseDoubleClickEvent( QMouseEvent *ev )
{
    if( ev->button() & LeftButton )
    {
        slotProperties();
    }
}


void LayerTable::slotAddLayer()
{
    KisImage *img = m_doc->current();

    QString name = i18n( "layer %1" ).arg( img->layerList().count() );

    img->addLayer( img->imageExtents(), KisColor::white(), true, name );

    QRect uR = img->layerList().at(img->layerList().count() - 1)->imageExtents();
    img->markDirty(uR);

    selectLayer( img->layerList().count() - 1 );

    updateTable();
    updateAllCells();

    m_doc->setModified( true );
}


void LayerTable::slotRemoveLayer()
{
  if( m_doc->current()->layerList().count() != 0 )
  {
    QRect uR = m_doc->current()->layerList().at(m_selected)->imageExtents();
    m_doc->current()->removeLayer( m_selected );
    m_doc->current()->markDirty(uR);

    if( m_selected == (int)m_doc->current()->layerList().count() )
      m_selected--;

    updateTable();
    updateAllCells();

    m_doc->setModified( true );
  }
}

void LayerTable::swapLayers( int a, int b )
{
    if( ( m_doc->current()->layerList().at( a )->visible() ) &&
      ( m_doc->current()->layerList().at( b )->visible() ) )
    {
        QRect l1 = m_doc->current()->layerList().at( a )->imageExtents();
        QRect l2 = m_doc->current()->layerList().at( b )->imageExtents();

        if( l1.intersects( l2 ) )
        {
            QRect rect = l1.intersect( l2 );
            m_doc->current()->markDirty( rect );
        }
    }
}


void LayerTable::slotRaiseLayer()
{
    int newpos = m_selected > 0 ? m_selected - 1 : 0;

    if( m_selected != newpos )
    {
        m_doc->current()->upperLayer( m_selected );
        repaint();
        swapLayers( m_selected, newpos );
        selectLayer( newpos );

        m_doc->setModified( true );
    }
}


void LayerTable::slotLowerLayer()
{
    int npos = (m_selected + 1) < (int)m_doc->current()->layerList().count() ?
        m_selected + 1 : m_selected;

    if( m_selected != npos )
    {
        m_doc->current()->lowerLayer( m_selected );
        repaint();
        swapLayers( m_selected, npos );
        selectLayer( npos );

        m_doc->setModified( true );
    }
}


void LayerTable::slotFrontLayer()
{
  if( m_selected != (int)(m_doc->current()->layerList().count() - 1))
  {
    m_doc->current()->setFrontLayer( m_selected );
    selectLayer( m_doc->current()->layerList().count() - 1 );

    QRect uR = m_doc->current()->layerList().at( m_selected )->imageExtents();
    m_doc->current()->markDirty( uR );

    updateAllCells();

    m_doc->setModified( true );
  }
}


void LayerTable::slotBackgroundLayer()
{
    if( m_selected != 0 )
    {
        m_doc->current()->setBackgroundLayer( m_selected );
        selectLayer( 0 );

        QRect uR = m_doc->current()->layerList().at(m_selected)->imageExtents();
        m_doc->current()->markDirty(uR);

        updateAllCells();

        m_doc->setModified( true );
    }
}


void LayerTable::updateAllCells()
{
    if(m_doc->current())
        for( int i = 0; i < (int)m_doc->current()->layerList().count(); i++ )
            updateCell( i, 0 );
}


void LayerTable::slotProperties()
{
    if( LayerPropertyDialog::editProperties(
        *( m_doc->current()->layerList().at(m_selected))))
    {
        QRect uR = m_doc->current()->layerList().at( m_selected )->imageExtents();
        updateCell( m_selected, 0 );
        m_doc->current()->markDirty( uR );

        m_doc->setModified( true );
    }
}

LayerPropertyDialog::LayerPropertyDialog( QString layername,
    uchar opacity,  QWidget *parent, const char *name )
    : QDialog( parent, name, true )
{
    QGridLayout *layout = new QGridLayout( this, 4, 2, 15, 7 );

    m_name = new QLineEdit( layername, this );
    layout->addWidget( m_name, 0, 1 );

    QLabel *lblName = new QLabel( m_name, i18n( "Name" ), this );
    layout->addWidget( lblName, 0, 0 );

    m_opacity = new IntegerWidget( 0, 255, this );
    m_opacity->setValue( opacity );
    m_opacity->setTickmarks( QSlider::Below );
    m_opacity->setTickInterval( 32 );
    layout->addWidget( m_opacity, 1, 1 );

    QLabel *lblOpacity = new QLabel( m_opacity, i18n( "Opacity" ), this );
    layout->addWidget( lblOpacity, 1, 0 );

    layout->setRowStretch( 2, 1 );

    QHBox *buttons = new QHBox( this );
    layout->addMultiCellWidget( buttons, 3, 4, 0, 1 );

    (void) new QWidget( buttons );

    QPushButton *pbOk = new QPushButton( i18n("OK"), buttons);
    pbOk->setDefault( true );
    QObject::connect( pbOk, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *pbCancel = new QPushButton( i18n( "Cancel" ), buttons);
    QObject::connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
}


bool LayerPropertyDialog::editProperties( KisLayer &layer )
{
    LayerPropertyDialog *dialog;

    dialog = new LayerPropertyDialog( layer.name(), layer.opacity(),
        NULL, "Layer Properties" );

    if( dialog->exec() == Accepted )
    {
        layer.setName( dialog->m_name->text() );
        layer.setOpacity( dialog->m_opacity->value() );

        return true;
    }

    return false;
}


#include "kis_layerview.moc"

