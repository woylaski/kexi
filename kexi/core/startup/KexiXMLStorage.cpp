/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>

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

#include "KexiXMLStorage.h"

#include <qdom.h>
#include <qdir.h>
#include <qfile.h>
#include <qregexp.h>

#include <kglobal.h>
#include <kdebug.h>

KexiProjectData* loadKexiConnectionDataXML(QIODevice *dev, QString &error)
{
	//TODO
	return 0;
}

bool saveKexiConnectionDataXML(QIODevice *dev, const KexiDB::ConnectionData &data, QString &error)
{
	//TODO
	return false;
}


KexiProjectData* loadKexiProjectDataXML(QIODevice *dev, QString &error)
{
	//TODO
	return 0;
}

bool saveKexiProjectDataXML(QIODevice *dev, const KexiProjectData &data, QString &error)
{
	//TODO
	return false;
}


KexiProjectData* loadKexiProjectSetXML(QIODevice *dev, QString &error)
{
	//TODO
	return 0;
}

bool saveKexiProjectSetXML(QIODevice *dev, const KexiProjectData &pset, QString &error)
{
	//TODO
	return false;
}
