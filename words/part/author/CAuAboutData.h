/* This file is part of the KDE project
   Copyright (C) 1998 - 2001 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2012        Inge Wallin <inge@lysator.liu.se>

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

#ifndef CAUABOUTDATA_H
#define CAUABOUTDATA_H

#include <kaboutdata.h>
#include <klocale.h>
#include <calligraversion.h>

static const char AUTHOR_DESCRIPTION[] = I18N_NOOP("Author tool");
static const char AUTHOR_VERSION[] = CALLIGRA_VERSION_STRING;

KAboutData * newAuthorAboutData()
{
    // The second argument, "words", apparently enables translations.
    // FIXME: We will probably have to change this when we move into
    //        our own top level directory.
    KAboutData * aboutData = new KAboutData("author", "words", ki18nc("application name", "Calligra Author"),
                                            AUTHOR_VERSION, ki18n(AUTHOR_DESCRIPTION), KAboutData::License_LGPL,
                                            ki18n("© 2012-2013, The Author Team"), KLocalizedString(),
                                            "http://www.calligra.org/author/");
    aboutData->setProductName("calligraauthor"); // for bugs.kde.org
    aboutData->setProgramIconName(QLatin1String("calligraauthor"));
    //                          Name             Function               email (if any)
    aboutData->addAuthor(ki18n("Inge Wallin"), ki18n("Co-maintainer"), "");
    aboutData->addAuthor(ki18n("Gopalakrishna Bhat"), ki18n("Co-maintainer"), "");
    aboutData->addAuthor(ki18n("Mojtaba Shahi Senobari"), ki18n("EPUB export"), "mojtaba.shahi3000@gmail.com");
    return aboutData;
}

#endif
