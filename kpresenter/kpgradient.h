/******************************************************************/
/* KPresenter - (c) by Reginald Stadlbauer 1997-1998              */
/* Version: 0.1.0                                                 */
/* Author: Reginald Stadlbauer                                    */
/* E-Mail: reggie@kde.org                                         */
/* Homepage: http://boch35.kfunigraz.ac.at/~rs                    */
/* needs c++ library Qt (http://www.troll.no)                     */
/* written for KDE (http://www.kde.org)                           */
/* needs mico (http://diamant.vsb.cs.uni-frankfurt.de/~mico/)     */
/* needs OpenParts and Kom (weis@kde.org)                         */
/* License: GNU GPL                                               */
/******************************************************************/
/* Module: Gradients (header)                                     */
/******************************************************************/

#ifndef kpgradient_h
#define kpgradient_h

#include <qcolor.h>
#include <qpixmap.h>
#include <qlist.h>

#include <ksize.h>
#include <krect.h>

#include "global.h"

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

class QPainter;

/******************************************************************/
/* Class: KPGradient                                              */
/******************************************************************/

class KPGradient
{
public:
	KPGradient( QColor _color1, QColor _color2, BCType _bcType, KSize _size );
	virtual ~KPGradient()
    {}

	virtual QColor getColor1()
    { return color1; }
	virtual QColor getColor2()
    { return color2; }
	virtual BCType getBackColorType()
    { return bcType; }

	virtual void setColor1( QColor _color )
    { color1 = _color; paint(); }
	virtual void setColor2( QColor _color )
    { color2 = _color; paint(); }
	virtual void setBackColorType( BCType _type )
    { bcType = _type; paint(); }

	virtual QPixmap* getGradient()
    { return &pixmap; }
	virtual KSize getSize()
    { return pixmap.size(); }

	virtual void setSize( KSize _size )
    { pixmap.resize( _size ); paint(); }

	virtual void addRef();
	virtual bool removeRef();

protected:
	KPGradient()
    {; }
	virtual void paint();
	virtual void makeLineGradient( QPixmap &pmCrop, QColor _color1, QColor _color2, int _ySize );
	virtual void makeColorList( QList<QColor> &_colorList, QColor _color1, QColor _color2, int _ySize );

	QColor color1, color2;
	BCType bcType;

	QPixmap pixmap;
	int refCount;
};

#endif
