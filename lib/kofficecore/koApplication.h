/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __ko_app_h__
#define __ko_app_h__

#include <kapp.h>

#define KOAPP ((KoApplication *)KApplication::kApplication())

/**
 *  Base class for all KOffice apps
 *
 *  This class handles given arguments giving on the command line and
 *  shows a generic about dialog for all KOffice apps.
 *
 * In addition it adds the standard directories where KOffice applications
 * can find their images etc.
 *
 * If the last mainwindow becomes closed, KoApplication automatically
 * calls @ref QApplication::quit.
 *
 *  @short Base class for all KOffice apps.
 */
class KoApplication : public KApplication
{
    Q_OBJECT

public:

    /**
     * Creates an application object, adds some standard directories and
     * initializes kimgio.
     */
    KoApplication();


    /**
     *  Destructor.
     */
    virtual ~KoApplication();

    // ######### Bad name
    /**
     * Call this to start the application.
     *
     * Parses command line arguments and creates the initial shells and docs
     * from them (or an empty doc if no cmd-line argument is specified ).
     *
     * You must call this method directly before calling @ref QApplication::exec.
     *
     * It is valid behaviour not to call this method at all. In this case you
     * have to process your command line parameters by yourself.
     */
    virtual bool start();
};

#endif
