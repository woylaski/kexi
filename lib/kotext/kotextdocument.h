/* This file is part of the KDE project
   Copyright (C) 2001 David Faure <faure@kde.org>

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

#ifndef kotextdocument_h
#define kotextdocument_h

#include "qrichtext_p.h"
using namespace Qt3;
class KoZoomHandler;
class KoTextFormatCollection;
class KoParagVisitor;

/**
 * This is our QTextDocument reimplementation, to create KoTextParags instead of QTextParags,
 */
class KoTextDocument : public QTextDocument
{
    Q_OBJECT
public:
    /** don't mind p (we don't use parent documents) */
    KoTextDocument( KoZoomHandler *zoomHandler, QTextDocument *p, KoTextFormatCollection *fc );

    ~KoTextDocument();

    /** Factory method for paragraphs */
    virtual QTextParag * createParag( QTextDocument *d, QTextParag *pr = 0, QTextParag *nx = 0, bool updateIds = TRUE );

    /** Return the zoom handler associated with this document. */
    KoZoomHandler * zoomHandler() const { return m_zoomHandler; }

    /** Visit all the parts of a selection.
     * Returns true, unless canceled. See KoParagVisitor. */
    bool visitSelection( int selectionId, KoParagVisitor *visitor, bool forward = true );

    /** Visit all paragraphs of the document.
     * Returns true, unless canceled. See KoParagVisitor. */
    bool visitDocument( KoParagVisitor *visitor, bool forward = true );

    /** Visit the document between those two point.
     * Returns true, unless canceled. See KoParagVisitor. */
    bool visitFromTo( QTextParag *firstParag, int firstIndex, QTextParag* lastParag, int lastIndex, KoParagVisitor* visitor, bool forw = true );

    /** Used by ~KoTextParag to know if it should die quickly */
    bool isDestroying() const { return m_bDestroying; }

    /** The main drawing method. Equivalent to QTextDocument::draw, but reimplemented
     * for wysiwyg */
    QTextParag *drawWYSIWYG( QPainter *p, int cx, int cy, int cw, int ch, const QColorGroup &cg,
		      bool onlyChanged = FALSE, bool drawCursor = FALSE, QTextCursor *cursor = 0,
		      bool resetChanged = TRUE );

protected:
    void drawWithoutDoubleBuffer( QPainter *p, const QRect &rect, const QColorGroup &cg, const QBrush *paper = 0 );
    void drawParagWYSIWYG( QPainter *p, QTextParag *parag, int cx, int cy, int cw, int ch,
		    QPixmap *&doubleBuffer, const QColorGroup &cg,
		    bool drawCursor, QTextCursor *cursor, bool resetChanged = TRUE );
    KoZoomHandler * m_zoomHandler;

private:
    QPixmap *bufferPixmap( const QSize &s );
    bool m_bDestroying;
    QPixmap *ko_buf_pixmap;
};

/**
 * Base class for "visitors". Visitors are a well-designed way to
 * apply a given operation to all the paragraphs in a selection, or
 * in a document. The visitor needs to inherit KoParagVisitor, and implement visit().
 */
class KoParagVisitor
{
protected:
    /** protected since this is an abstract base class */
    KoParagVisitor() {}
    virtual ~KoParagVisitor() {}
public:
    /** Visit the paragraph @p parag, from index @p start to index @p end */
    virtual bool visit( QTextParag *parag, int start, int end ) = 0;
};

#endif
