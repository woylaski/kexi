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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef __ko_app_h__
#define __ko_app_h__

#include <kapplication.h>
#include "komain_export.h"

class KoPart;

class KoApplicationPrivate;

class QSplashScreen;
class QStringList;

#include <KoFilterManager.h>

#define koApp KoApplication::koApplication()

/**
 *  @brief Base class for all %Calligra apps
 *
 *  This class handles arguments given on the command line and
 *  shows a generic about dialog for all Calligra apps.
 *
 *  In addition it adds the standard directories where Calligra applications
 *  can find their images etc.
 *
 *  If the last mainwindow becomes closed, KoApplication automatically
 *  calls QApplication::quit.
 */
class KOMAIN_EXPORT KoApplication : public KApplication
{
    Q_OBJECT

public:
    /**
     * Creates an application object, adds some standard directories and
     * initializes kimgio.
     *
     * @param nativeMimeType: the nativeMimeType of the calligra application
     */
    explicit KoApplication(const QByteArray &nativeMimeType);

    /**
     *  Destructor.
     */
    virtual ~KoApplication();

    /**
     * Call this to start the application.
     *
     * Parses command line arguments and creates the initial main windowss and docs
     * from them (or an empty doc if no cmd-line argument is specified ).
     *
     * You must call this method directly before calling QApplication::exec.
     *
     * It is valid behaviour not to call this method at all. In this case you
     * have to process your command line parameters by yourself.
     */
    virtual bool start();

    /**
     * Tell KoApplication to show this splashscreen when you call start();
     * when start returns, the splashscreen is hidden. Use KSplashScreen
     * to have the splash show correctly on Xinerama displays. 
     */
    void setSplashScreen(QSplashScreen *splash);


    QList<KoPart*> partList() const;

    /**
     * return a list of mimetypes this application supports.
     */
    QStringList mimeFilter(KoFilterManager::Direction direction) const;

    // Overridden to handle exceptions from event handlers.
    bool notify(QObject *receiver, QEvent *event);

    /**
     * Returns the current application object.
     *
     * This is similar to the global QApplication pointer qApp. It
     * allows access to the single global KoApplication object, since
     * more than one cannot be created in the same application. It
     * saves you the trouble of having to pass the pointer explicitly
     * to every function that may require it.
     * @return the current application object
     */
    static KoApplication* koApplication();

signals:

    /// KoPart needs to be able to emit document signals from here. These
    /// signals are used for the dbus interface of stage, see commit
    /// d102d9beef80cc93fc9c130b0ad5fe1caf238267
    friend class KoPart;

    /**
     * emitted when a new document is opened.
     */
    void documentOpened(const QString &ref);

    /**
     * emitted when an old document is closed.
     */
    void documentClosed(const QString &ref);

protected:

    // Current application object.
    static KoApplication *KoApp;

private:
    bool initHack();
    KoApplicationPrivate * const d;
    class ResetStarting;
    friend class ResetStarting;
};

#endif
