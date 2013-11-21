/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QDir>
#include <QMessageBox>

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include "MainWindow.h"

#include "DocumentListModel.h"
#include "KisSketchView.h"
#include "SketchInputContext.h"
#include "stdlib.h"


int main( int argc, char** argv )
{
    KAboutData aboutData("kritasketch",
                         0,
                         ki18n("Krita Sketch"),
                         "0.1",
                         ki18n("Krita Sketch: Painting on the Go for Artists"),
                         KAboutData::License_GPL,
                         ki18n("(c) 1999-2012 The Krita team and KO GmbH.\n"),
                         KLocalizedString(),
                         "http://www.krita.org",
                         "submit@bugs.kde.org");

    KCmdLineArgs::init (argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add( "+[files]", ki18n( "Images to open" ) );
    options.add( "novkb", ki18n( "Don't use the virtual keyboard" ) );
    KCmdLineArgs::addCmdLineOptions( options );

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    QStringList fileNames;
    if (args->count() > 0) {
        for (int i = 0; i < args->count(); ++i) {
            QString fileName = args->arg(i);
            if (QFile::exists(fileName)) {
                fileNames << fileName;
            }
        }
    }

    KApplication app;
    QDir appdir(app.applicationDirPath());
    appdir.cdUp();

#ifdef Q_OS_WIN
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // If there's no kdehome, set it and restart the process.
    //QMessageBox::information(0, "krita sketch", "KDEHOME: " + env.value("KDEHOME"));
    if (!env.contains("KDEHOME") ) {
        _putenv_s("KDEHOME", QDesktopServices::storageLocation(QDesktopServices::DataLocation).toLocal8Bit());
    }
    if (!env.contains("KDESYCOCA")) {
        _putenv_s("KDESYCOCA", QString(appdir.absolutePath() + "/sycoca").toLocal8Bit());
    }
    if (!env.contains("XDG_DATA_DIRS")) {
        _putenv_s("XDG_DATA_DIRS", QString(appdir.absolutePath() + "/share").toLocal8Bit());
    }
    if (!env.contains("KDEDIR")) {
        _putenv_s("KDEDIR", appdir.absolutePath().toLocal8Bit());
    }
    if (!env.contains("KDEDIRS")) {
        _putenv_s("KDEDIRS", appdir.absolutePath().toLocal8Bit());
    }
    _putenv_s("PATH", QString(appdir.absolutePath() + "/bin" + ";"
              + appdir.absolutePath() + "/lib" + ";"
              + appdir.absolutePath() + "/lib"  +  "/kde4" + ";"
              + appdir.absolutePath()).toLocal8Bit());

    app.addLibraryPath(appdir.absolutePath());
    app.addLibraryPath(appdir.absolutePath() + "/bin");
    app.addLibraryPath(appdir.absolutePath() + "/lib");
    app.addLibraryPath(appdir.absolutePath() + "/lib/kde4");
#endif

#if defined Q_WS_X11 && QT_VERSION >= 0x040800
    QApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

    QStringList fonts = KGlobal::dirs()->findAllResources( "appdata", "fonts/*.otf" );
    foreach( const QString &font, fonts ) {
        QFontDatabase::addApplicationFont( font );
    }

    QFontDatabase db;
    QApplication::setFont( db.font( "Source Sans Pro", "Regular", 12 ) );

    MainWindow window(fileNames);

    if (args->isSet("vkb")) {
        app.setInputContext(new SketchInputContext(&app));
    }

#ifdef Q_OS_WIN
    window.showFullScreen();
#else
    window.show();
#endif

    return app.exec();
}
