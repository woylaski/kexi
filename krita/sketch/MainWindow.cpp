/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 * Copyright (C) 2012 KO GmbH. Contact: Boudewijn Rempt <boud@kogmbh.com>
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

#include "MainWindow.h"

#include "opengl/kis_opengl.h"

#include <QApplication>
#include <QResizeEvent>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QGLWidget>
#include <QTimer>

#include <kcmdlineargs.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include "filter/kis_filter.h"
#include "filter/kis_filter_registry.h"
#include "kis_paintop.h"
#include "kis_paintop_registry.h"
#include <KoZoomController.h>

#include "kis_view2.h"
#include <kis_canvas_controller.h>
#include "kis_config.h"

#include "SketchDeclarativeView.h"
#include "RecentFileManager.h"
#include "DocumentManager.h"

class MainWindow::Private
{
public:
    Private(MainWindow* qq)
        : q(qq)
        , allowClose(true)
        , sketchKisView(0)
	{
        centerer = new QTimer(q);
        centerer->setInterval(10);
        centerer->setSingleShot(true);
        connect(centerer, SIGNAL(timeout()), q, SLOT(adjustZoomOnDocumentChangedAndStuff()));
	}
	MainWindow* q;
    bool allowClose;
    KisView2* sketchKisView;
    QString currentSketchPage;
	QTimer *centerer;
};

MainWindow::MainWindow(QStringList fileNames, QWidget* parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags ), d( new Private(this) )
{
    qApp->setActiveWindow( this );

    setWindowTitle(i18n("Krita Sketch"));

    // Load filters and other plugins in the gui thread
    Q_UNUSED(KisFilterRegistry::instance());
    Q_UNUSED(KisPaintOpRegistry::instance());

    KisConfig cfg;
    cfg.setCursorStyle(CURSOR_STYLE_NO_CURSOR);
    cfg.setUseOpenGL(true);

    foreach(QString fileName, fileNames) {
        DocumentManager::instance()->recentFileManager()->addRecent(fileName);
    }


    QDeclarativeView* view = new SketchDeclarativeView();
    view->engine()->rootContext()->setContextProperty("mainWindow", this);

#ifdef Q_OS_WIN
    QDir appdir(qApp->applicationDirPath());

    // Corrects for mismatched case errors in path (qtdeclarative fails to load)
    wchar_t buffer[1024];
    QString absolute = appdir.absolutePath();
    DWORD rv = ::GetShortPathName((wchar_t*)absolute.utf16(), buffer, 1024);
    rv = ::GetLongPathName(buffer, buffer, 1024);
    QString correctedPath((QChar *)buffer);
    appdir.setPath(correctedPath);

    // for now, the app in bin/ and we still use the env.bat script
    appdir.cdUp();

    view->engine()->addImportPath(appdir.canonicalPath() + "/lib/calligra/imports");
    view->engine()->addImportPath(appdir.canonicalPath() + "/lib64/calligra/imports");
    QString mainqml = appdir.canonicalPath() + "/share/apps/kritasketch/kritasketch.qml";
#else
    view->engine()->addImportPath(KGlobal::dirs()->findDirs("lib", "calligra/imports").value(0));
    QString mainqml = KGlobal::dirs()->findResource("appdata", "kritasketch.qml");
#endif

    Q_ASSERT(QFile::exists(mainqml));
    if (!QFile::exists(mainqml)) {
        QMessageBox::warning(0, "No QML found", mainqml + " doesn't exist.");
    }
    QFileInfo fi(mainqml);

    view->setSource(QUrl::fromLocalFile(fi.canonicalFilePath()));
    view->setResizeMode( QDeclarativeView::SizeRootObjectToView );

    if (view->errors().count() > 0) {
        foreach(const QDeclarativeError &error, view->errors()) {
            kDebug() << error.toString();
        }
    }

    setCentralWidget(view);
}

bool MainWindow::allowClose() const
{
    return d->allowClose;
}

void MainWindow::setAllowClose(bool allow)
{
    d->allowClose = allow;
}

QString MainWindow::currentSketchPage() const
{
    return d->currentSketchPage;
}

void MainWindow::setCurrentSketchPage(QString newPage)
{
    d->currentSketchPage = newPage;
    emit currentSketchPageChanged();
}
void MainWindow::adjustZoomOnDocumentChangedAndStuff()
{
	if (d->sketchKisView) {
        qApp->processEvents();
        d->sketchKisView->zoomController()->setZoom(KoZoomMode::ZOOM_PAGE, 1.0);
        qApp->processEvents();
        QPoint center = d->sketchKisView->rect().center();
        d->sketchKisView->canvasControllerWidget()->zoomRelativeToPoint(center, 0.9);
        qApp->processEvents();
    }
}

QObject* MainWindow::sketchKisView() const
{
    return d->sketchKisView;
}

void MainWindow::setSketchKisView(QObject* newView)
{
    if (d->sketchKisView)
        d->sketchKisView->disconnect(this);
    if (d->sketchKisView != newView)
    {
        d->sketchKisView = qobject_cast<KisView2*>(newView);
        connect(d->sketchKisView, SIGNAL(sigLoadingFinished()), d->centerer, SLOT(start()));
        d->centerer->start();
        emit sketchKisViewChanged();
    }
}

void MainWindow::minimize()
{
    setWindowState(windowState() ^ Qt::WindowMinimized);
}

void MainWindow::closeWindow()
{
    //For some reason, close() does not work even if setAllowClose(true) was called just before this method.
    //So instead just completely quit the application, since we are using a single window anyway.
    QApplication::exit();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // TODO this needs setting somewhere...
//     d->constants->setGridWidth( event->size().width() / d->constants->gridColumns() );
//     d->constants->setGridHeight( event->size().height() / d->constants->gridRows() );
    QWidget::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!d->allowClose) {
        event->ignore();
        emit closeRequested();
    } else {
        event->accept();
    }
}

MainWindow::~MainWindow()
{
    delete d;
}

#include "MainWindow.moc"
