/*
 *  Copyright (c) 2005 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KIS_WETOP_H_
#define KIS_WETOP_H_

#include "kis_paintop.h"
#include "kis_types.h"

class KisPoint;
class KisPainter;


class KisWetOpFactory : public KisPaintOpFactory  {

public:
	KisWetOpFactory() {}
	virtual ~KisWetOpFactory() {}

	virtual KisPaintOp * createOp(KisPainter * painter);
	virtual KisID id() { return KisID("wetbrush", i18n("watercolor brush")); }
};

class KisWetOp : public KisPaintOp {

	typedef KisPaintOp super;

public:

	KisWetOp(KisPainter * painter);
	virtual ~KisWetOp();

	void paintAt(const KisPoint &pos,
		     const double pressure,
		     const double /*xTilt*/,
		     const double /*yTilt*/);

};

#endif // KIS_WETOP_H_
