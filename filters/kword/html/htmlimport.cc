/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2001 Nicolas GOUTTE <nicog@snafu.de>

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

#include <htmlimport.h>
#include <htmlimport.moc>
#include <kdebug.h>

#include "ImportListener.h"

HTMLImport::HTMLImport(KoFilter *parent, const char*name) :
                     KoFilter(parent, name) {
}

bool HTMLImport::filter(const QString &fileIn, const QString &fileOut,
                        const QString& from, const QString& to,
                        const QString &)
{
    if(to!="application/x-kword" || from!="text/html")
        return false;

    kdDebug(30503) << "Calling HTML filter" << endl;
    return HtmlFilter(fileIn,fileOut);
}
