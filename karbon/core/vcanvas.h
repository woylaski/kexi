/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
   Copyright (C) 2002, The Karbon Developers

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __VCANVAS_H__
#define __VCANVAS_H__


#include <qscrollview.h>

class KarbonPartBase;
class KarbonViewBase;
class KoRect;
class KoPoint;
class VPainter;

// The canvas is a QScrollView.

class VCanvas : public QScrollView
{
	Q_OBJECT
public:
	VCanvas( KarbonViewBase* view, KarbonPartBase* part );

	void repaintAll( const KoRect & );
	void repaintAll( bool drawVObjects = true );

	QPixmap *pixmap() { return m_pixmap; }

	/**
	 * Sets mouse position to point p.
	 */
	void setPos( const KoPoint& p );

	KoPoint toContents( const KoPoint & ) const;
	KoRect boundingBox() const;

protected:
	virtual void focusInEvent( QFocusEvent * );
	virtual void viewportPaintEvent( QPaintEvent* );
	virtual void drawContents( QPainter* painter, int clipx, int clipy,
		int clipw, int cliph  );
	void drawDocument( QPainter* painter, const KoRect& rect, bool drawVObjects = true );

	virtual void resizeEvent( QResizeEvent* event );

	virtual bool eventFilter( QObject* object, QEvent* event );

	void setYMirroring( VPainter * );

private slots:
	void slotContentsMoving( int , int );

signals:
	void viewportChanged();

private:
	QPixmap *m_pixmap;
	KarbonPartBase* m_part;
	KarbonViewBase* m_view;

	bool m_bScrolling;
	int	m_scrollX;
	int	m_scrollY;
};

#endif
