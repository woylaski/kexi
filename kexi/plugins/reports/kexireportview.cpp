/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
   Copyright (C) 2014 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kexireportview.h"
#include <KReportView>
#include "kexidbreportdata.h"
#ifndef KEXI_MOBILE
#include <widget/utils/kexirecordnavigator.h>
 //! @todo KEXI3
#if 0
#include "keximigratereportdata.h"
#endif
#endif
#include <core/KexiWindow.h>
#include <core/KexiMainWindowIface.h>
#include <KexiIcon.h>

//! @todo KEXI3 #include "../scripting/kexiscripting/kexiscriptadaptor.h"

#include <KoReportPage>
#include <renderobjects>
#include <KoReportPreRenderer>
#include <krscripthandler>
#include "krscriptfunctions.h"

#include <KIO/NetAccess>
#include <KRun>
#include <KActionMenu>
#include <KMessageBox>

#include <QLayout>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QGraphicsScene>
#include <QDebug>
#include <QFileDialog>
#include <QMimeDatabase>

KexiReportView::KexiReportView(QWidget *parent)
        : KexiView(parent), m_preRenderer(0), m_reportDocument(0) //! @todo KEXI3, m_kexi(0), m_functions(0)
{
    setObjectName("KexiReportDesigner_DataView");

    m_reportView = new KReportView(this);
    layout()->addWidget(m_reportView);

#ifndef KEXI_MOBILE
    m_pageSelector = new KexiRecordNavigator(*m_reportView->scrollArea(), m_reportView);
    m_pageSelector->setInsertingButtonVisible(false);
    m_pageSelector->setInsertingEnabled(false);
    m_pageSelector->setLabelText(xi18nc("Page selector label", "Page:"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonFirst, xi18n("Go to first page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonFirst, xi18n("Goes to first page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonPrevious, xi18n("Go to previous page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonPrevious, xi18n("Goes to previous page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonNext, xi18n("Go to next page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonNext, xi18n("Goes to next page"));
    m_pageSelector->setButtonToolTipText(KexiRecordNavigator::ButtonLast, xi18n("Go to last page"));
    m_pageSelector->setButtonWhatsThisText(KexiRecordNavigator::ButtonLast, xi18n("Goes to last page"));
    m_pageSelector->setNumberFieldToolTips(xi18n("Current page number"), xi18n("Number of pages"));
    m_pageSelector->setRecordHandler(this);
#endif

    // -- setup local actions
    QList<QAction*> viewActions;
    QAction* a;

#ifndef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("document-print"), xi18n("Print"), this));
    a->setObjectName("print_report");
    a->setToolTip(xi18n("Print report"));
    a->setWhatsThis(xi18n("Prints the current report."));
    connect(a, SIGNAL(triggered()), this, SLOT(slotPrintReport()));

    KActionMenu *exportMenu = new KActionMenu(koIcon("document-export"), xi18nc("@title:menu","E&xport As"), this);
    exportMenu->setObjectName("report_export_as");
    exportMenu->setDelayed(false);
#endif

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(xi18n("Export:"), this));
    a->setEnabled(false); //!TODO this is a bit of a dirty way to add what looks like a label to the toolbar!
    // " ", not "", is said to be needed in maemo, the icon didn't display properly without it
    viewActions << (a = new QAction(koIcon("application-vnd.oasis.opendocument.text"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("application-vnd.oasis.opendocument.text"),
                                          xi18nc("open dialog to export as text document", "Text Document..."), this));
#endif
    a->setObjectName("export_as_text_document");
    a->setToolTip(xi18n("Export the report as a text document (in OpenDocument Text format)"));
    a->setWhatsThis(xi18n("Exports the report as a text document (in OpenDocument Text format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsTextDocument()));

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("application-pdf"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("application-pdf"),
                                          xi18nc("Portable Document Format...", "PDF..."), this));
#endif
    a->setObjectName("export_as_pdf");
    a->setToolTip(xi18n("Export as PDF"));
    a->setWhatsThis(xi18n("Exports the current report as PDF."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsPdf()));

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("application-vnd.oasis.opendocument.spreadsheet"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("application-vnd.oasis.opendocument.spreadsheet"),
                                          xi18nc("open dialog to export as spreadsheet", "Spreadsheet..."), this));
#endif
    a->setObjectName("export_as_spreadsheet");
    a->setToolTip(xi18n("Export the report as a spreadsheet (in OpenDocument Spreadsheet format)"));
    a->setWhatsThis(xi18n("Exports the report as a spreadsheet (in OpenDocument Spreadsheet format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsSpreadsheet()));

#ifdef KEXI_MOBILE
    viewActions << (a = new QAction(koIcon("text-html"), QLatin1String(" "), this));
#else
    exportMenu->addAction(a = new QAction(koIcon("text-html"),
                                          xi18nc("open dialog to export as web page", "Web Page..."), this));
#endif
    a->setObjectName("export_as_web_page");
    a->setToolTip(xi18n("Export the report as a web page (in HTML format)"));
    a->setWhatsThis(xi18n("Exports the report as a web page (in HTML format)."));
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(slotExportAsWebPage()));

    setViewActions(viewActions);

#ifndef KEXI_MOBILE
    // setup main menu actions
    QList<QAction*> mainMenuActions;

    mainMenuActions << exportMenu;

    setMainMenuActions(mainMenuActions);

    connect(m_pageSelector, SIGNAL(nextButtonClicked()), this, SLOT(nextPage()));
    connect(m_pageSelector, SIGNAL(prevButtonClicked()), this, SLOT(prevPage()));
    connect(m_pageSelector, SIGNAL(firstButtonClicked()), this, SLOT(firstPage()));
    connect(m_pageSelector, SIGNAL(lastButtonClicked()), this, SLOT(lastPage()));
#endif
}

KexiReportView::~KexiReportView()
{
    qDebug();
    delete m_preRenderer;
    delete m_reportDocument;
}

void KexiReportView::slotPrintReport()
{
    QPrinter printer(QPrinter::HighResolution);
    QPainter painter;
    KoReportRendererBase *renderer;

    renderer = m_factory.createInstance("print");
    QPointer<QPrintDialog> dialog = new QPrintDialog(&printer, this);
    if (dialog->exec() == QDialog::Accepted) {
        KoReportRendererContext cxt;
        cxt.printer = &printer;
        cxt.painter = &painter;

        renderer->render(cxt, m_reportDocument);
    }
    delete dialog;
    delete renderer;
}

void KexiReportView::slotExportAsPdf()
{
    QScopedPointer<KoReportRendererBase> renderer(m_factory.createInstance("print"));
    if (renderer) {
        KoReportRendererContext cxt;

        cxt.destinationUrl = getExportUrl(QLatin1String("application/pdf"),
                                          xi18n("Export Report as PDF"),
                                          "kfiledialog:///LastVisitedPDFExportPath/",
                                          "pdf");
        if (!cxt.destinationUrl.isValid()) {
            return;
        }

        QPrinter printer;
        QPainter painter;

        printer.setOutputFileName(cxt.destinationUrl.path());
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);

        painter.begin(&printer);
        cxt.printer = &printer;
        cxt.painter = &painter;
        if (!renderer->render(cxt, m_reportDocument)) {
            KMessageBox::error(this,
                               xi18n("Exporting the report as PDF to %1 failed.", cxt.destinationUrl.toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.destinationUrl);
        }
   }
}

QUrl KexiReportView::getExportUrl(const QString &mimetype, const QString &caption,
                                  const QString &lastExportPath, const QString &extension)
{
    QUrl result;
    QString defaultSavePath;

    if (lastExportPath.startsWith("kfiledialog:///")) {
        defaultSavePath = lastExportPath + window()->partItem()->captionOrName() + "." + extension;
    }

    // loop until an url has been chosen or the file selection has been cancelled
    const QMimeDatabase db;
    const QString filterString = db.mimeTypeForName(mimetype).filterString();

    while (true) {
        QUrl result = QFileDialog::getSaveFileUrl(this, caption, QUrl::fromLocalFile(defaultSavePath),
                                             filterString);

        // not cancelled?
        if (result.isValid()) {
            if (KIO::NetAccess::exists(result, KIO::NetAccess::DestinationSide, this)) {
                const KMessageBox::ButtonCode answer = KMessageBox::warningContinueCancel(this,
                    xi18n("The file %1 exists.\nDo you want to overwrite it?", result.path()),
                    caption, KGuiItem(xi18nc("@action:button Overwrite File", "Overwrite")));

                // if overwriting not wanted, let select another url
                if (answer == KMessageBox::Cancel) {
                    continue;
                }
            }
        }

        // decision has been made, leave loop
        break;
    }

    return result;
}

void KexiReportView::openExportedDocument(const QUrl &destination)
{
    const int answer =
        KMessageBox::questionYesNo(
            this,
            xi18n("Do you want to open exported document?"),
            QString(),
            KStandardGuiItem::open(),
            KStandardGuiItem::close());

    if (answer == KMessageBox::Yes) {
        (void)new KRun(destination, this->topLevelWidget());
    }
}

void KexiReportView::slotExportAsSpreadsheet()
{
    KoReportRendererBase *renderer;
    KoReportRendererContext cxt;

    renderer = m_factory.createInstance("ods");

    if (renderer) {
        cxt.destinationUrl = getExportUrl(QLatin1String("application/vnd.oasis.opendocument.spreadsheet"),
                                          xi18n("Export Report as Spreadsheet"),
                                          "kfiledialog:///LastVisitedODSExportPath/",
                                          "ods");
        if (!cxt.destinationUrl.isValid()) {
            return;
        }

        if (!renderer->render(cxt, m_reportDocument)) {
            KMessageBox::error(this,
                               xi18n("Failed to export the report as spreadsheet to %1.", cxt.destinationUrl.toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.destinationUrl);
        }
    }
}

void KexiReportView::slotExportAsTextDocument()
{
    KoReportRendererBase *renderer;
    KoReportRendererContext cxt;

    renderer = m_factory.createInstance("odt");

    if (renderer) {
        cxt.destinationUrl = getExportUrl(QLatin1String("application/vnd.oasis.opendocument.text"),
                                          xi18n("Export Report as Text Document"),
                                          "kfiledialog:///LastVisitedODTExportPath/",
                                          "odt");
        if (!cxt.destinationUrl.isValid()) {
            return;
        }

        if (!renderer->render(cxt, m_reportDocument)) {
            KMessageBox::error(this,
                               xi18n("Exporting the report as text document to %1 failed.", cxt.destinationUrl.toDisplayString()),
                               xi18n("Export Failed"));
        } else {
            openExportedDocument(cxt.destinationUrl);
        }
    }
}

void KexiReportView::slotExportAsWebPage()
{
    KoReportRendererContext cxt;
    KoReportRendererBase *renderer;

    const QString dialogTitle = xi18n("Export Report as Web Page");
    cxt.destinationUrl = getExportUrl(QLatin1String("text/html"),
                                      dialogTitle,
                                      "kfiledialog:///LastVisitedHTMLExportPath/",
                                      "html");
    if (!cxt.destinationUrl.isValid()) {
        return;
    }

    const int answer =
        KMessageBox::questionYesNo(
            this,
            xi18n("Would you like to export using a Cascading Style Sheet (CSS), "
                 "which will give an output closer to the original, "
                 "or export using a HTML Table, which outputs a much simpler format?"),
            dialogTitle,
            KGuiItem(xi18nc("@action:button", "Use CSS")),
            KGuiItem(xi18nc("@action:button", "Use Table")));

    if (answer == KMessageBox::Yes) {
        renderer = m_factory.createInstance("htmlcss");
    }
    else {
        renderer = m_factory.createInstance("htmltable");
    }

    if (!renderer->render(cxt, m_reportDocument)) {
        KMessageBox::error(this,
                           xi18n("Exporting the report as web page to %1 failed.", cxt.destinationUrl.toDisplayString()),
                           xi18n("Export Failed"));
    } else {
        openExportedDocument(cxt.destinationUrl);
    }
}

tristate KexiReportView::beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore)
{
    Q_UNUSED(mode);
    Q_UNUSED(dontStore);

    return true;
}

tristate KexiReportView::afterSwitchFrom(Kexi::ViewMode mode)
{
    Q_UNUSED(mode);

    qDebug();
    if (tempData()->reportSchemaChangedInPreviousView) {
        qDebug() << "Schema changed";
        delete m_preRenderer;

        qDebug() << tempData()->reportDefinition.tagName();

        m_preRenderer = new KoReportPreRenderer(tempData()->reportDefinition);
        if (m_preRenderer->isValid()) {
            KoReportData *reportData = 0;
            if (!tempData()->connectionDefinition.isNull())  {
                reportData = sourceData(tempData()->connectionDefinition);
            }
            m_preRenderer->setSourceData(reportData);

            m_preRenderer->setName(window()->partItem()->name());

            //Add a kexi object to provide kexidb and extra functionality
//! @todo KEXI3 if we want this            if(!m_kexi) {
//                m_kexi = new KexiScriptAdaptor();
//            }
//            m_preRenderer->registerScriptObject(m_kexi, "Kexi");
            //If using a kexidb source, add a functions scripting object
            if (tempData()->connectionDefinition.attribute("type") == "internal") {
                m_functions = new KRScriptFunctions(reportData, KexiMainWindowIface::global()->project()->dbConnection());
                m_preRenderer->registerScriptObject(m_functions, "field");
                connect(m_preRenderer, SIGNAL(groupChanged(QMap<QString, QVariant>)),
                        m_functions, SLOT(setGroupData(QMap<QString, QVariant>)));
            }

            if (m_reportDocument) {
                qDebug() << "=======================================Deleting old document";
                delete m_reportDocument;
            }

            m_reportDocument = m_preRenderer->generate();
            if (m_reportDocument) {
                m_reportView->setDocument(m_reportDocument);
#ifndef KEXI_MOBILE
                m_pageSelector->setRecordCount(m_reportView->pageCount());
                m_pageSelector->setCurrentRecordNumber(1);
#endif
            }
        } else {
            KMessageBox::error(this, xi18n("Report schema appears to be invalid or corrupt"), xi18n("Opening failed"));
        }


        tempData()->reportSchemaChangedInPreviousView = false;
    }
    return true;
}

KoReportData* KexiReportView::sourceData(QDomElement e)
{
    KoReportData *kodata = 0;

    if (e.attribute("type") == "internal" && !e.attribute("source").isEmpty()) {
        kodata = new KexiDBReportData(e.attribute("source"), KexiMainWindowIface::global()->project()->dbConnection());
    }
#ifndef KEXI_MOBILE
//! @todo KEXI3
#if 0
    if (e.attribute("type") ==  "external") {
        kodata = new KexiMigrateReportData(e.attribute("source"));
    }
#endif
#endif
    return kodata;
}

KexiReportPart::TempData* KexiReportView::tempData() const
{
    return static_cast<KexiReportPart::TempData*>(window()->data());
}

void KexiReportView::addNewRecordRequested()
{

}

void KexiReportView::moveToFirstRecordRequested()
{

    m_reportView->moveToFirstPage();
    #ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
    #endif

}

void KexiReportView::moveToLastRecordRequested()
{
    m_reportView->moveToLastPage();
    #ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
    #endif
}

void KexiReportView::moveToNextRecordRequested()
{
    m_reportView->moveToNextPage();
    #ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
    #endif
}

void KexiReportView::moveToPreviousRecordRequested()
{
    m_reportView->moveToPreviousPage();
    #ifndef KEXI_MOBILE
    m_pageSelector->setCurrentRecordNumber(m_reportView->currentPage());
    #endif
}

void KexiReportView::moveToRecordRequested(int r)
{
    Q_UNUSED(r);
}

int KexiReportView::currentRecord() const
{
    return m_reportView->currentPage();
}

int KexiReportView::recordCount() const
{
    return m_reportView->pageCount();
}
