/*
 *  kis_doc.cc - part of KImageShop
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *  Copyright (c) 2000 John Califf  <jcaliff@compuzone.net>
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


#include <string.h>
#include <stdio.h>

#include <iostream.h>
#include <istream.h>
#include <ostream.h>

#include <sys/time.h>
#include <unistd.h>

#include <qpainter.h>
#include <qwidget.h>
#include <qregexp.h>
#include <qfileinfo.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qbuffer.h>
#include <qimage.h>

#include <kstddirs.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <kimageio.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <koStream.h>
#include <komlParser.h>
#include <komlWriter.h>
#include <koFilterManager.h>
#include <koTemplateChooseDia.h>
#include <koStore.h>
#include <koStoreDevice.h>
#include <koMainWindow.h>
#include <koQueryTrader.h> 

#include "kis_doc.h"
#include "kis_view.h"
#include "kis_shell.h"
#include "kis_factory.h"
#include "kis_dlg_new.h"
#include "kis_image.h"
#include "kis_layer.h"
#include "kis_channel.h"
#include "kis_selection.h"
#include "koUndo.h"

#define KIS_DEBUG(AREA, CMD)

/*
    KisDoc - ko virtual method implemented
*/

KisDoc::KisDoc( QWidget *parentWidget, const char *widgetName, QObject* parent, const char* name, bool singleViewMode )
    : KoDocument( parentWidget, widgetName, parent, name, singleViewMode ) 
    , m_commands()
{
kdDebug(0) << "KisDoc::KisDoc() entering" << endl;

    bool loadPlugins = true;    
    setInstance( KisFactory::global(), loadPlugins );
    m_pCurrent = 0L;
    m_pNewDialog = 0L;
    m_pClipImage = 0L;
    m_pSelection = new KisSelection(this);    
    m_Images.setAutoDelete(false);
    
kdDebug(0) << "QPixmap::defaultDepth(): " << QPixmap::defaultDepth() << endl; 
kdDebug(0) << "KisDoc::KisDoc() leaving" << endl;    
}

/*
    Init doc - ko virtual method implemented
*/

bool KisDoc::initDoc()
{
    kdDebug(0) << "KisDoc::initDoc() entering" << endl;
    
    bool ok = false;
    QString name;
    name.sprintf("image%d", m_Images.count() + 1);
    
    // choose dialog for open mode  

    QString templ;
    KoTemplateChooseDia::ReturnType ret;

    ret = KoTemplateChooseDia::choose (KisFactory::global(),
              templ,
              "application/x-krayon", "*.kra",
              i18n("Krayon"),
              KoTemplateChooseDia::Everything,
              "krayon_template");

    // create document from template - use default 
    // 512x512 RGBA image
    
    if (ret == KoTemplateChooseDia::Template) 
    {
        KisImage *img = newImage(name, 512, 512, cm_RGBA, 8);
        if (!img) return false;

        // add background layer
        img->addLayer(QRect(0, 0, 512, 512), KisColor::white(), 
            false, "background");
        img->markDirty(QRect(0, 0, 512, 512));
 
        // list of images - mdi document 
        setCurrentImage(img);
        emit imageListUpdated();
        
        setModified (true);
        ok = true;
    } 
    // open an existing document
    else if ( ret == KoTemplateChooseDia::File ) 
    {
        KURL url;
        url.setPath (templ);
        ok = openURL (url);
    } 
    // create a new document from scratch
    else if ( ret == KoTemplateChooseDia::Empty )
    {
        // NewDialog for entering parameters
        ok = slotNewImage();        
        if(ok) emit imageListUpdated();
    } 

    return ok;    
}


/*
    Save current document (image) to xml format using QDomDocument
    ko virtual method implemented
*/

QDomDocument KisDoc::saveXML( )
{
    cout << " --- KisDoc::saveXML --- " << endl;
    KisImage *img = m_pCurrent;
    
    // FIXME: implement saving of non-RGB modes.

    QDomDocument doc( "image" );
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );

    // image element - only one per document
    QDomElement image = doc.createElement( "image" );
  
    image.setAttribute( "name", img->name() );
    image.setAttribute( "author", img->author() );
    image.setAttribute( "email", img->email() );
    image.setAttribute( "editor", "Krayon" );
    image.setAttribute( "mime", "application/x-krayon" );
    image.setAttribute( "width", img->width() );
    image.setAttribute( "height", img->height() );
    image.setAttribute( "bitDepth", static_cast<int>(img->bitDepth()) );
    image.setAttribute( "cMode", static_cast<int>(img->colorMode()) );
  
kdDebug(0) << "name: " <<  img->name() << endl;
kdDebug(0) << "author: " <<  img->author() << endl;          
kdDebug(0) << "email: " <<  img->email() << endl;
kdDebug(0) << "editor: " <<  "Krayon" << endl;
kdDebug(0) << "mime: " << "application/x-krayon"  << endl;
kdDebug(0) << "img->width(): " <<  img->width() << endl;  
kdDebug(0) << "img->height(): " <<  img->height() << endl;    
kdDebug(0) << "bitDepth " <<  static_cast<int>(img->bitDepth()) << endl;  
kdDebug(0) << "cMode: " <<  static_cast<int>(img->colorMode()) << endl;  

    doc.appendChild( image );

    // layers element - variable
    QDomElement layers = doc.createElement( "layers" );
    image.appendChild( layers );

    // layer elements
kdDebug(0) << "layer elements" << endl;

    QList<KisLayer> l_lst = img->layerList();
    for (KisLayer *lay = l_lst.first(); lay != 0; lay = l_lst.next())
    {
        QDomElement layer = doc.createElement( "layer" );

	    layer.setAttribute( "name", lay->name() );
	    layer.setAttribute( "x", lay->imageExtents().x() );
	    layer.setAttribute( "y", lay->imageExtents().y() );
	    layer.setAttribute( "width", lay->imageExtents().width() );
        layer.setAttribute( "height", lay->imageExtents().height() );
	    layer.setAttribute( "opacity", static_cast<int>(lay->opacity()) );

kdDebug(0) << "name: " <<  lay->name() << endl;          
kdDebug(0) << "x: " << lay->imageExtents().x()   << endl;                  
kdDebug(0) << "y: " << lay->imageExtents().y()   << endl;
kdDebug(0) << "width: " << lay->imageExtents().width()   << endl;
kdDebug(0) << "height: " << lay->imageExtents().height()   << endl;        
kdDebug(0) << "opacity: " <<  static_cast<int>(lay->opacity())  << endl;        

	    if (lay->visible())
		    layer.setAttribute( "visible", "true" );
	    else
		    layer.setAttribute( "visible", "false" );

	    if (lay->linked())
		    layer.setAttribute( "linked", "true" );
	    else
		    layer.setAttribute( "linked", "false" );

	    layer.setAttribute( "bitDepth", static_cast<int>(lay->bitDepth()) );
	    layer.setAttribute( "cMode", static_cast<int>(lay->colorMode()) );

kdDebug(0) << "bitDepth: " <<  static_cast<int>(lay->bitDepth())  << endl;
kdDebug(0) << "colorMode: " <<  static_cast<int>(lay->colorMode())  << endl;        

	    layers.appendChild( layer );

	    // channels element - variable, normally maximum of 4 channels
	    QDomElement channels = doc.createElement( "channels" );
	    layer.appendChild( channels );

kdDebug(0) << "channel elements" << endl;

	    // channel elements
	    for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel())
	    {
	        QDomElement channel = doc.createElement( "channel" );

	        channel.setAttribute( "cId", static_cast<int>(ch->channelId()) );
	        channel.setAttribute( "bitDepth", static_cast<int>(ch->bitDepth()) );

kdDebug(0) << "cId: " <<  static_cast<int>(ch->channelId())  << endl;
kdDebug(0) << "bitDepth: " <<  static_cast<int>(ch->bitDepth())  << endl;            

	        channels.appendChild( channel );

	    } // end of channels loop
    } // end of layers loop

    setModified( false );
    return doc;
}

/*
    Save extra, document-specific data outside xml format as defined by
    DTD for this document type.  It is appended to the saved
    xml document in gzipped format using the store methods of koffice
    common code koffice/lib/store/ as an internal file, not a real, 
    separate file in the filesystem.
    
    In this case it's the binary image data
    Krayon can only handle rgb and rgba formats for now, 
    by channel for each layer of the image saved in binary format
    
    ko virtual method implemented
*/

bool KisDoc::completeSaving( KoStore* store )
{
kdDebug(0) << "KisDoc::completeSaving() entering" << endl;

    if (!store)         return false;
    if (!m_pCurrent)    return false;

    QList<KisLayer> layers = m_pCurrent->layerList();

    for ( KisLayer *lay = layers.first(); lay != 0; lay = layers.next())
    {
        for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel())
	    {
	        QString url = QString( "layers/%1/channels/ch%2.bin" ).arg( lay->name() )
			    .arg( static_cast<int>(ch->channelId()) );

            if ( store->open( url ) )
	        {
                ch->writeToStore(store);
		        store->close();
	        }
	    }
    }

kdDebug(0) << "KisDoc::completeSaving() leaving" << endl;    

    return true;
}


/*
    loadXML - reimplements ko method    
*/

bool KisDoc::loadXML( QIODevice *, const QDomDocument& doc )
{
kdDebug(0) << "KisDoc::loadXML() entering" << endl;

    if ( doc.doctype().name() != "image" )
    {
      kdDebug(0) << "KisDoc::loadXML() no doctype name error" << endl;    
	  return false;
    }

    QDomElement image = doc.documentElement();

    if (image.attribute( "mime" ) != "application/x-krayon") 
    {
      kdDebug(0) << "KisDoc::loadXML() no mime name error" << endl;        
      return false;
    }
    
    // this assumes that we are loading an existing image
    // with certain attributes set 
    
    QString name = image.attribute( "name" );
    int w = image.attribute( "width" ).toInt();
    int h = image.attribute( "height" ).toInt();
    int cm = image.attribute( "cMode" ).toInt();
    int bd = image.attribute( "bitDepth" ).toInt();

kdDebug(0) << "name: " << name << endl;            
kdDebug(0) << "width: " << w << endl; 
kdDebug(0) << "height: " << w << endl;           
kdDebug(0) << "cMode: " << cm << endl;            
kdDebug(0) << "bitDepth: " << bd << endl;        

    cMode colorMode;

    switch (cm)
    {
      case 0:
	    colorMode = cm_Indexed;
	    break;
	  case 1:
	    colorMode = cm_Greyscale;
	    break;
	  case 2:
	    colorMode = cm_RGB;
	    break;
	  case 3:
	    colorMode = cm_RGBA;
	    break;
	  case 4:
	    colorMode = cm_CMYK;
	    break;
	  case 5:
	    colorMode = cm_CMYKA;
	    break;
	  case 6:
	    colorMode = cm_Lab;
	    break;
	  case 7:
	    colorMode = cm_LabA;
	    break;
	  default:
	    return false;
    }

    KisImage *img = newImage(name, w, h, colorMode, bd);  
    if (!img) return false;

    img->setAuthor( image.attribute( "author" ));
    img->setEmail( image.attribute( "email" ));

    // layers element
    QDomElement layers = image.namedItem( "layers" ).toElement();
    if (layers.isNull())
    { 
        kdDebug(0) << "KisDoc::loadXML(): layers.isNull() error!" << endl;
        return false;
    }
    
    // layer elements
    QDomNode l = layers.firstChild();

    // FIXME - need to load layers and channels here, 
    // as read from elements
    while (!l.isNull())
    {
        QDomElement layer = l.toElement();
        if (layer.tagName() != "layer" ) continue;

	    cout << "--- layer ---" << endl;

	    // channels element
	    QDomElement channels = layer.namedItem( "channels" ).toElement();
	    if (channels.isNull()) continue;

	    // channel elements
	    QDomNode c = channels.firstChild();
	
	    while (!c.isNull())
	    {
	      QDomElement channel = c.toElement();
	      if (channel.tagName() != "channel" ) continue;

          cout << "--- channel ---" << endl;
          
	      c = c.nextSibling();
	    }
	    l = l.nextSibling();
    }

    /* add background layer : should only do this for a new document, not 
    when loading an existing image - for existing document,
    but will do for now */
    
    img->addLayer( QRect(0, 0, w, h), KisColor::white(), false, "background");
    img->markDirty( QRect(0, 0, w, h) );
    setCurrentImage(img);
  
kdDebug(0) << "KisDoc::loadXML() leaving succesfully" << endl;    
    
    return true;
}



bool KisDoc::completeLoading( KoStore* store)
{

kdDebug(0) << "KisDoc::completeLoading() entering" << endl;

    if ( !store )  return false;
    if ( !m_pCurrent) return false;
    
    QList<KisLayer> layers = m_pCurrent->layerList();

    for (KisLayer *lay = layers.first(); lay != 0; lay = layers.next())
    {
      for ( KisChannel* ch = lay->firstChannel(); ch != 0; ch = lay->nextChannel())
	  {
	    QString url = QString( "layers/%1/channels/ch%2.bin" ).arg( lay->name() )
			.arg( static_cast<int>(ch->channelId()) );

          if ( store->open( url ) )
	      {
            kdDebug(0) << "KisDoc::completeLoading() ch->loadFromStore()" << endl;            
            ch->loadFromStore(store);
		    store->close();
	    }
	  }
    }

    // need this to force redraw of image data just loaded
    current()->markDirty(QRect(0, 0, current()->width(), current()->height()));
    setCurrentImage(current());
    
kdDebug(0) << "KisDoc::completeLoading() leaving" << endl;        

    return true;
}


/*
    Set current image - generic
*/

void KisDoc::setCurrentImage(KisImage *img)
{
kdDebug() << "KisDoc::setCurrentImage entering" << endl; 

    if (m_pCurrent)
    {
      // disconnect old current image
      QObject::disconnect( m_pCurrent, SIGNAL( updated() ),
	            this, SLOT( slotImageUpdated() ) );
      QObject::disconnect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
	            this, SLOT( slotImageUpdated( const QRect& ) ) );
      QObject::disconnect( m_pCurrent, SIGNAL( layersUpdated() ),
                    this, SLOT( slotLayersUpdated() ) );
    }

    m_pCurrent = img;

    if(m_pCurrent)
    {
        // connect new current image
        QObject::connect( m_pCurrent, SIGNAL( updated() ),
		    this, SLOT( slotImageUpdated() ) );
        QObject::connect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
		    this, SLOT( slotImageUpdated( const QRect& ) ) );
        QObject::connect( m_pCurrent, SIGNAL( layersUpdated() ),
		    this, SLOT( slotLayersUpdated() ) );
    }
    
    emit imageListUpdated();
    emit layersUpdated();
    emit docUpdated();
    
kdDebug() << "KisDoc::setCurrentImage leaving" << endl; 
}


/*
    Set current image by name
*/

void KisDoc::setCurrentImage(const QString& _name)
{

kdDebug() << "KisDoc::setCurrentImage entering" << endl;

    KisImage *img = m_Images.first();

    while (img)
    {
        if (img->name() == _name)
	    {
	        setCurrentImage(img);
	        return;
	    }
        img = m_Images.next();
    }
   
kdDebug() << "KisDoc::setCurrentImage leaving" << endl;   

}


/*
    rename an image - from menu or click on image tab
*/

void KisDoc::renameImage(QString & oldName, QString & newName)
{
    KisImage *img = m_Images.first();

    while (img)
    {
        if(img->name() == oldName)
        {
            img->setName(newName);
            break;
        }
        img = m_Images.next();
    }
    
    emit imageListUpdated();
}


/*
    build list of images by name
*/

QStringList KisDoc::images()
{
    QStringList lst;
    KisImage *img = m_Images.first();

    while (img)
    {
        lst.append(img->name());
        img = m_Images.next();
    }
    
    return lst;
}


bool KisDoc::isEmpty()
{
    if (m_pCurrent) return false;
    return true;
}


QString KisDoc::currentImage()
{
    if (m_pCurrent) return m_pCurrent->name();
    return QString("");
}


KisImage* KisDoc::current()
{
    return m_pCurrent;
}


/*
    KisDoc destructor - Note that since this is MDI, each image
    in the list must be deleted to free up all memory.  While
    there is only ONE Koffice "document", there are multiple 
    images you can load and work with in any session
*/

KisDoc::~KisDoc()
{
    KisImage *img = m_Images.first();

    while (img)
    {
        // disconnect old current image
        if(img == m_pCurrent)
        {
            QObject::disconnect( m_pCurrent, SIGNAL( updated() ),
	            this, SLOT( slotImageUpdated() ) );
            QObject::disconnect( m_pCurrent, SIGNAL( updated( const QRect& ) ),
	            this, SLOT( slotImageUpdated( const QRect& ) ) );
            QObject::disconnect( m_pCurrent, SIGNAL( layersUpdated() ),
                this, SLOT( slotLayersUpdated() ) );
        }
        
        delete img;
        img = m_Images.next();
    }

    if(m_pClipImage != 0L)
    {
        delete m_pClipImage;
        m_pClipImage = 0L;
    }   
    
    if(m_pSelection != 0L)
    {
        delete m_pSelection;
        m_pSelection = 0L;
    }    
}


/*
    Save current document (image) in a standard image format
    Note that only the current visible layer(s) will be saved
    usually one needs to merge all layers first, as with Gimp
    The format the image is saved in is determined solely by 
    the file extension used.
*/

bool KisDoc::saveAsQtImage( QString file)
{
    unsigned int w, h;
    
    w = (unsigned int) current()->width();
    h = (unsigned int) current()->height();
    
    // prepare a pixmap for drawing
    QPixmap *buffer = new QPixmap (w, h);
    if (buffer == 0L)
    {  
        kdDebug(0) << "KisDoc::saveAsQtImage: can't create QPixmap" << endl;    
        return false;
    }

    buffer->fill (Qt::white);
    QPainter p;
    p.begin (buffer);
    p.setBackgroundColor (Qt::white);
    p.eraseRect (0, 0, w, h);

    /* draw the contents of the document into the painter
    Note that later this can be used to trim the image to a
    selected rectangular or other area */
    
    QRect wholeImage = QRect(0, 0, w, h);
    paintContent(p, wholeImage);
    p.end ();

    // copy the affected area (default is entire image) to the new pixmap
    QPixmap *pixmap = new QPixmap (w, h);
    if (pixmap == 0L)
    {
        kdDebug(0) << "KisDoc::saveAsQtImage: Can't create QPixmap" << endl;
        return false;
    }    
  
    // copy from one pixmap to the other, unnecessary, but
    // useful later for selection of area within image
    bitBlt (pixmap, 0, 0, buffer, 0, 0, w, h);
    
    // clean up source pixmap
    delete buffer;

    // now create an image
    QImage img  = pixmap->convertToImage ();
    
    // need to pass in bool flag for whether or not to set
    // alpha buffer depending on image and savefile type
    // img.setAlphaBuffer (true);

    // clean up destiation pixmap    
    delete pixmap;
    
    // and save the image in requested format.
    // file extension determines image format - best way    
    return img.save( file, KImageIO::type(file).ascii() );
}


void KisDoc::CopyToLayer(KisView *pView)
{
    unsigned int w = (unsigned int) current()->width();
    unsigned int h = (unsigned int) current()->height();
    
    /* prepare a pixmap for drawing - same size as current image */
    
    QPixmap *buffer = new QPixmap (w, h);
    if (buffer == 0L)
    {  
        kdDebug(0) << "KisDoc::CopyToLayer: can't create QPixmap" << endl;    
        return;
    }

    buffer->fill (Qt::white);
    QPainter p;
    p.begin (buffer);
    p.setBackgroundColor (Qt::white);
    p.eraseRect (0, 0, w, h);

    /* draw the contents of the document into the painter
    Note that later this can be used to trim the image to a
    selected rectangular or other area */
    
    QRect wholeImage = QRect(0, 0, w, h);
    paintContent(p, wholeImage);
    p.end ();

    /* copy the affected area (default is entire image) to the 
    new pixmap */
    
    QPixmap *pixmap = new QPixmap (w, h);
    if (pixmap == 0L)
    {
        kdDebug(0) << "KisDoc::CanvasToLayer: Can't create QPixmap" << endl;
        return;
    }    
  
    /* copy from one pixmap to the other, unnecessary, but
    useful later for selection of area within image and applying
    Qt raster operations prior to using image - easy to do with 
    QPixmaps */
    
    bitBlt (pixmap, 0, 0, buffer, 0, 0, w, h);
    
    // clean up source pixmap
    delete buffer;

    // now create an image
    QImage img  = pixmap->convertToImage ();
    // img.setAlphaBuffer (true);
    
    // Copy the image data into the current layer */
    QtImageToLayer(&img, pView);
    
    // clean up destiation pixmap    
    delete pixmap;
}

/*
    Copy a QImage exactly into the current image's active layer, 
    pixel by pixel using scanlines,  fully 32 bit even if the alpha 
    channel isn't used. This provides a  basis for a clipboard buffer 
    and a Krayon blit routine, with custom modifiers to blend, apply 
    various filters and raster operations,  and many other neat 
    effects.  -jwc-
*/

bool KisDoc::QtImageToLayer(QImage *qimage, KisView *pView)
{
    KisImage *img = current();
    KisLayer *lay = img->getCurrentLayer();
    QImage   *qimg = qimage;

    if (!img)	        return false;
    if (!lay)           return false;

    // FIXME: Implement this for non-RGB modes.
    if (!img->colorMode() == cm_RGB && !img->colorMode() == cm_RGBA)
	  return false;

    /* if dealing with 1 or 8 bit images, convert to 16 bit */
    if(qimage->depth() < 16)
    {
        QImage Converted 
            = qimage->smoothScale(qimage->width(), qimage->height());
        qimg = &Converted;
    }
    
    bool grayScale = false;
    int startx = 0;
    int starty = 0;

    QRect clipRect(startx, starty, qimg->width(), qimg->height());

    if (!clipRect.intersects(img->getCurrentLayer()->imageExtents()))
        return false;
  
    clipRect = clipRect.intersect(img->getCurrentLayer()->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar bv = 255;
    uchar invbv = 0;
    uchar r, g, b, a;
    int   v = 255;

    int red     = pView->fgColor().R();
    int green   = pView->fgColor().G();
    int blue    = pView->fgColor().B();

    bool alpha = (img->colorMode() == cm_RGBA);
  
    for (int y = sy; y <= ey; y++)
    {
      sl = qimg->scanLine(y);

      for (int x = sx; x <= ex; x++)
	  {
        // destination binary values by channel
	    r = lay->pixel(0, startx + x, starty + y);
	    g = lay->pixel(1, startx + x, starty + y);
	    b = lay->pixel(2, startx + x, starty + y);

        if(grayScale)
        {
          // source binary value for grayscale - not used here
	      bv = *(sl + x);
            
          // skip black pixels in source - only for gray scale 
	      if (bv == 0) continue;

          // inverse of gray scale binary value - the darker the higher
          // the value - only used forgray scale images
	      invbv = 255 - bv;

	      r = ((red * bv) + (r * invbv))/255;
	      g = ((green * bv) + (g * invbv))/255;
          b = ((blue * bv) + (b * invbv))/255;
        }

        uint *p = (uint *)qimg->scanLine(y) + x;
            
	    lay->setPixel(0, startx + x, starty + y, qRed(*p));
	    lay->setPixel(1, startx + x, starty + y, qGreen(*p));
	    lay->setPixel(2, startx + x, starty + y, qBlue(*p));
                       	  
        if (alpha)
	    {
	      a = lay->pixel(3, startx + x, starty + y);
          
          if(grayScale)
          {
		    v = a + bv;
		    if (v < 0 ) v = 0;
		    if (v > 255 ) v = 255;
		    a = (uchar) v;
          }

		  lay->setPixel(3, startx + x, starty + y, a);
	    }
	  } 
    }
    
    return true;
}


/*
    Copy a rectangular area of a layer into a QImage, pixel by pixel 
    using scanlines,  fully 32 bit even if the alpha channel isn't used. 
    This provides a  basis for a clipboard buffer and a Krayon blit 
    routine, with custom modifiers to blend, apply various filters and 
    raster operations,  and many other neat effects.  -jwc-
*/

// this belongs in either the image class or selection class, 
// and is not used yet - not finished.  doesn't really belong in document - jwc

bool KisDoc::LayerToQtImage(QImage *qimage, KisView *pView, QRect & clipRect)
{
    KisImage *img = current();
    KisLayer *lay = img->getCurrentLayer();
    QImage   *qimg = qimage;

    if (!img)  return false;
    if (!lay)  return false;

    // FIXME: Implement this for non-RGB modes.
    if (!img->colorMode() == cm_RGB && !img->colorMode() == cm_RGBA)
	    return false;

    bool grayScale = false;
    
    /* if dealing with 1 or 8 bit images, convert to 16 bit */
    if(qimage->depth() < 16 && !grayScale)
    {
        QImage Converted 
            = qimage->smoothScale(qimage->width(), qimage->height());
        qimg = &Converted;
    }
    
    int startx = 0;
    int starty = 0;

    //QRect clipRect(startx, starty, qimg->width(), qimg->height());

    if (!clipRect.intersects(img->getCurrentLayer()->imageExtents()))
        return false;
  
    clipRect = clipRect.intersect(img->getCurrentLayer()->imageExtents());

    int sx = clipRect.left() - startx;
    int sy = clipRect.top() - starty;
    int ex = clipRect.right() - startx;
    int ey = clipRect.bottom() - starty;

    uchar *sl;
    uchar bv, invbv;
    uchar r, g, b;
    uchar a = 255;
    int v = 255;

    int red     = pView->fgColor().R();
    int green   = pView->fgColor().G();
    int blue    = pView->fgColor().B();

    bool alpha = (img->colorMode() == cm_RGBA);
  
    for (int y = sy; y <= ey; y++)
    {
        sl = qimg->scanLine(y);

        for (int x = sx; x <= ex; x++)
	    {
            // destination binary values by channel
	        r = lay->pixel(0, startx + x, starty + y);
	        g = lay->pixel(1, startx + x, starty + y);
	        b = lay->pixel(2, startx + x, starty + y);
            if(alpha) a = lay->pixel(3, startx + x, starty + y);        

            if(grayScale)
            {
                // source binary value for grayscale - not used here
	            bv = *(sl + x);
            
                // skip black pixels in source - only for gray scale 
	            if (bv == 0) continue;

                // inverse of gray scale binary value  
                // the darker the higher the value
	            invbv = 255 - bv;

	            r = ((red * bv) + (r * invbv))/255;
	            g = ((green * bv) + (g * invbv))/255;
                b = ((blue * bv) + (b * invbv))/255;
                
                if(alpha)
                {
		            v = a + bv;
		            if (v < 0 ) v = 0;
		            if (v > 255 ) v = 255;
		            a = (uchar) v;
                }              
            }
        
            uint *p = (uint *)qimg->scanLine(y) + x;
            *p = alpha ? qRgba(r, g, b, a) : qRgb(r, g, b);            
	    } 
    }
    
    return true;
}

/*
    set current selection rectangle for the document
*/

void KisDoc::setSelectRect(QRect & rect)
{
    selectRect.setLeft(rect.left());
    selectRect.setTop(rect.top());
    selectRect.setRight(rect.right());
    selectRect.setBottom(rect.bottom());    
}


/*
    set current selection or clip image for the document
*/

bool KisDoc::setClipImage( )
{
    KisImage *img = current();
    KisLayer *lay = img->getCurrentLayer();

    if(m_pClipImage != 0L)
    { 
        delete m_pClipImage;
        m_pClipImage = 0L;
    }   
    
    selectRect = getSelection()->getRect();
     
    m_pClipImage = new QImage(selectRect.width(), selectRect.height(), 32);
    if(!m_pClipImage) return false;
    
    // destination rect - the clip image
    int dx = selectRect.width();
    kdDebug(0) << "selection width: " << dx << endl;
    int dy = selectRect.height();
    kdDebug(0) << "selection height: " << dy << endl;    

    // source rect - the layer data
    int sx = selectRect.left();
    int sy = selectRect.top();
    
    // channel values of pixel in layer
    int r, g, b; 
    
    // need to determine if image is rgb or rgba
    bool alpha = (img->colorMode() == cm_RGBA);
    
    // alpha channel value in layer 
    // 255 is opaque, 0 is transparent
    int a = 255; 
    
    // ptr to data at scanline in QImage
    uint *p;
    
    // fill in the clip image with data from selection rectange
    // in image layer, using channel data at x+dx, y+dy offset into layer 
    for (int y = 0; y < dy; y++)
    {
        for (int x = 0; x < dx; x++)
	    {
            // source binary values by channel
	        r = lay->pixel(0, sx + x, sy + y);
	        g = lay->pixel(1, sx + x, sy + y);
	        b = lay->pixel(2, sx + x, sy + y);
            if(alpha) a = lay->pixel(3, sx + x, sy + y);
            
            // dest binary data in scanline at x offset
            // is set at this point - uint (32 bit)
            p = (uint *)m_pClipImage->scanLine(y) + x;
            *p = alpha ? qRgba(r, g, b, a) : qRgb(r, g, b);
        }
    }
    
    return true;    
}


KisImage* KisDoc::newImage(const QString& n, int width, int height, 
    cMode cm , uchar bitDepth )
{
    kdDebug() << "KisDoc::newImage(): entering" << endl; 

    KisImage *img = new KisImage( n, width, height, cm, bitDepth );
    kdDebug() << "KisDoc::newImage(): returned from KisImage constuctor" << endl;

    m_Images.append(img);
    return img;
}


void KisDoc::removeImage( KisImage *img )
{
    m_Images.remove(img);
    delete img;

    if(m_Images.count() > 1)
    {
        setCurrentImage(m_Images.first());
    }
    else 
    {
        setCurrentImage(0L);
    }
}


void KisDoc::slotRemoveImage( const QString& _name )
{
    KisImage *img = m_Images.first();

    while (img)
    {
        if (img->name() == _name)
	    {
	        removeImage(img);
	        return;
	    }
        img = m_Images.next();
    }
}

/*
    Create a new image for this document and set the current image to it
    There can be more than one image for each doc
*/

bool KisDoc::slotNewImage()
{
    kdDebug() << "KisDoc::slotNewImage" << endl; 

    kdDebug() << "KisDoc::slotNewImage:new NewDialog()" << endl; 
    if (!m_pNewDialog) 
        m_pNewDialog = new NewDialog();

    kdDebug() << "KisDoc::slotNewImage:NewDialog()->exec()" << endl; 
    // jwc - this causes bad drawable, probably qt spin buttons,
    // seems harmless, though
    m_pNewDialog->exec();
    
    if(!m_pNewDialog->result() == QDialog::Accepted)
        return false;

    kdDebug() << "KisDoc::slotNewImage:NewDialog()->newwidth()" << endl; 
    int w = m_pNewDialog->newwidth();
    kdDebug() << "KisDoc::slotNewImage:NewDialog()->newheight()" << endl;     
    int h = m_pNewDialog->newheight();
    kdDebug() << "KisDoc::slotNewImage:NewDialog()->backgroundMode()" << endl;     
    bgMode bg = m_pNewDialog->backgroundMode();
    kdDebug() << "KisDoc::slotNewImage:NewDialog()->colorMode()" << endl;     
    cMode cm = m_pNewDialog->colorMode();

    kdDebug() << "KisDoc::slotNewImage: w: "<< w << "h: " << h << endl; 

    QString name, desiredName;
    int numero = 1;
    unsigned int runs = 0;
    
    /* don't allow duplicate image names if some images have
    been removed leaving "holes" in name sequence */
    
    do {
        desiredName.sprintf("image %d", numero);
        KisImage *currentImg = m_Images.first();

        while (currentImg)
        {
            if (currentImg->name() == desiredName)
            {
                numero++; 
            }
            currentImg = m_Images.next();
        }
        runs++;
                    
    } while(runs < m_Images.count());
    
    name.sprintf("image %d", numero);
            
    KisImage *img = newImage(name, w, h, cm, 8);
    if (!img) return false;

    kdDebug() << "KisDoc::slotNewImage: returned from newImage()" << endl; 

    // add background layer

    if (bg == bm_White)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, "background");

    else if (bg == bm_Transparent)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), true, "background");

    else if (bg == bm_ForegroundColor)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, "background");

    else if (bg == bm_BackgroundColor)
	    img->addLayer(QRect(0, 0, w, h), KisColor::white(), false, "background");

    kdDebug() << "KisDoc::slotNewImage: returned from addLayer()" << endl;

    img->markDirty(QRect(0, 0, w, h));
    setCurrentImage(img);

    return true;
}

/*
    Mime type for this app - not same as file type, but file types
    can be associated with a mime type and are opened with applications
    associated with the same mime type
*/

QCString KisDoc::mimeType() const
{
    return "application/x-krayon";
}


/*
    Create view instance for this document - there can be more than
    one view of a document open at any time - a list of the views for 
    this document can be obtained somehow - perhaps from the factory
    or some kind of view manager - needed 
*/

KoView* KisDoc::createViewInstance( QWidget* parent, const char* name )
{
    if(name == 0) name = "View";    
    KisView* view = new KisView( this, parent, name );
    
    // undo-redo   

    QObject::connect( &m_commands, SIGNAL( undoRedoChanged( QString, QString ) ),
                    view, SLOT( slotUndoRedoChanged( QString, QString ) ) );
    QObject::connect( &m_commands, SIGNAL( undoRedoChanged( QStringList, QStringList ) ),
                    view, SLOT( slotUndoRedoChanged( QStringList, QStringList ) ) );
    
    return (view);
}


/*
    Create view shell or top level window for document
*/

KoMainWindow* KisDoc::createShell()
{
    KoMainWindow* shell = new KisShell;
    shell->show();

    return shell;
}

/*
    Draw current image on canvas - indirect approach
*/

void KisDoc::paintContent( QPainter& painter, 
        const QRect& rect, bool /*transparent*/)
{
    if (m_pCurrent)
    {
        m_pCurrent->paintPixmap( &painter, rect );
    }
    else
    {
        kdDebug(0) <<  "Error KisDoc::paintContent() - no m_pCurrent" << endl; 
    }    
}

/*
    Draw current image on canvas - direct apprach
*/

void KisDoc::paintPixmap(QPainter *p, QRect area)
{
    if (m_pCurrent)
    {
        m_pCurrent->paintPixmap(p, area);
    }
    else
    {
        kdDebug(0) <<  "Error KisDoc::paintPixmap() - no m_pCurrent" << endl;     
    }    
}

/* 
    let document update view when image is changed
*/
void KisDoc::slotImageUpdated()
{
    emit docUpdated();
}

/*
    let document update specific area of view when image is changed
*/

void KisDoc::slotImageUpdated( const QRect& rect )
{
    emit docUpdated(rect);
}


void KisDoc::slotLayersUpdated()
{
    emit layersUpdated();
}

#include "kis_doc.moc"
