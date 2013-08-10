/* This file is part of the KDE project
 * Copyright (C) 2001 David Faure <faure@kde.org>
 * Copyright (C) 2005-2007, 2009, 2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010-2011 Boudewijn Rempt <boud@kogmbh.com>
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
 * Boston, MA 02110-1301, USA
 */

// words includes
#include "KWView.h"
#include "KWGui.h"
#include "KWDocument.h"
#include "KWCanvas.h"
#include "KWFactory.h"
#include "KWStatusBar.h"
#include "KWPageManager.h"
#include "frames/KWFrame.h"
#include "frames/KWCopyShape.h"
#include "frames/KWTextFrameSet.h"
#include "dialogs/KWFrameDialog.h"
#include "dialogs/KWPageSettingsDialog.h"
#include "dialogs/KWPrintingDialog.h"
#include "dialogs/KWCreateBookmarkDialog.h"
#include "dialogs/KWSelectBookmarkDialog.h"
#include "dialogs/KWConfigureDialog.h"
#include "commands/KWFrameCreateCommand.h"
#include "commands/KWShapeCreateCommand.h"
#include "ui_KWInsertImage.h"

// calligra libs includes
#include <KoShapeCreateCommand.h>
#include <calligraversion.h>
#include <KoShapeRegistry.h>
#include <KoShapeFactoryBase.h>
#include <KoProperties.h>
#include <KoCopyController.h>
#include <KoTextDocument.h>
#include <KoTextShapeData.h>
#include <KoCanvasResourceManager.h>
#include <KoCutController.h>
#include <KoStandardAction.h>
#include <KoTemplateCreateDia.h>
#include <KoPasteController.h>
#include <KoShape.h>
#include <KoText.h>
#include <KoFind.h>
#include <KoShapeContainer.h>
#include <KoShapeManager.h>
#include <KoSelection.h>
#include <KoToolManager.h>
#include <KoTextRangeManager.h>
#include <KoAnnotationManager.h>
#include <KoTextEditor.h>
#include <KoToolProxy.h>
#include <KoShapeAnchor.h>
#include <KoShapeGroupCommand.h>
#include <KoZoomController.h>
#include <KoInlineTextObjectManager.h>
#include <KoBookmark.h>
#include <KoPathShape.h> // for KoPathShapeId
#include <KoCanvasController.h>
#include <KoDocumentRdfBase.h>
#include <KoDocumentInfo.h>
#include <KoMainWindow.h>
#include <KoCanvasControllerWidget.h>

#ifdef SHOULD_BUILD_RDF
#include <KoDocumentRdf.h>
#include <KoSemanticStylesheetsEditor.h>
#endif

#include <KoFindText.h>
#include <KoFindToolbar.h>
#include <KoTextLayoutRootArea.h>
#include <KoIcon.h>

// KDE + Qt includes
#include <QTimer>
#include <QScrollBar>
#include <QPushButton>
#include <klocale.h>
#include <kdebug.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kxmlguifactory.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kmenubar.h>

#include <limits>

KWView::KWView(KoPart *part, KWDocument *document, QWidget *parent)
        : KoView(part, document, parent)
        , m_canvas(0)
        , m_textMinX(1)
        , m_textMaxX(600)
        , m_minPageNum(1)
        , m_maxPageNum(1)
        , m_isDistractionFreeMode(false)
{
    setAcceptDrops(true);

    m_document = document;
    m_snapToGrid = m_document->gridData().snapToGrid();
    m_gui = new KWGui(QString(), this);
    m_canvas = m_gui->canvas();
    setFocusProxy(m_canvas);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_gui);

    setComponentData(KWFactory::componentData());
    setXMLFile("words.rc");

    m_currentPage = m_document->pageManager()->begin();

    //We need to create associate widget before connect them in actions
    //Perhaps there is a better place for the WordCount widget creates here
    //If you know where to move it in a better place, just do it
    buildAssociatedWidget();

    setupActions();

    connect(m_canvas->shapeManager()->selection(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

    m_find = new KoFindText(this);
    KoFindToolbar *toolbar = new KoFindToolbar(m_find, actionCollection(), this);
    toolbar->setVisible(false);
    connect(m_find, SIGNAL(matchFound(KoFindMatch)), this, SLOT(findMatchFound(KoFindMatch)));
    connect(m_find, SIGNAL(updateCanvas()), m_canvas, SLOT(update()));
    // The text documents to search in will potentially change when we add/remove shapes and after load
    connect(m_document, SIGNAL(shapeAdded(KoShape *, KoShapeManager::Repaint)), this, SLOT(refreshFindTexts()));
    connect(m_document, SIGNAL(shapeRemoved(KoShape *)), this, SLOT(refreshFindTexts()));
    refreshFindTexts();

    layout->addWidget(toolbar);

    m_zoomController = new KoZoomController(m_gui->canvasController(), &m_zoomHandler, actionCollection(), 0, this);
    if (statusBar())
        KWStatusBar::addViewControls(statusBar(), this);

    // the zoom controller needs to be initialized after the status bar gets initialized as
    // that resulted in bug 180759
    m_zoomController->setPageSize(m_currentPage.rect().size());
    m_zoomController->setTextMinMax(m_currentPage.contentRect().left(), m_currentPage.contentRect().right());
    KoZoomMode::Modes modes = KoZoomMode::ZOOM_WIDTH | KoZoomMode::ZOOM_TEXT;
    if (m_canvas->viewMode()->hasPages())
        modes |= KoZoomMode::ZOOM_PAGE;
    m_zoomController->zoomAction()->setZoomModes(modes);
    connect(m_canvas, SIGNAL(documentSize(const QSizeF &)), m_zoomController, SLOT(setDocumentSize(const QSizeF&)));
    m_canvas->updateSize(); // to emit the doc size at least once
    m_zoomController->setZoom(m_document->config().zoomMode(), m_document->config().zoom() / 100.);
    connect(m_zoomController, SIGNAL(zoomChanged(KoZoomMode::Mode, qreal)), this, SLOT(zoomChanged(KoZoomMode::Mode, qreal)));

    //Timer start in Distraction-Free mode view.
    m_hideCursorTimer = new QTimer(this);
    connect(m_hideCursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));

    m_dfmExitButton = new QPushButton(i18n("Exit Distraction-Free Mode"));
    addStatusBarItem(m_dfmExitButton, 0);
    m_dfmExitButton->setVisible(false);
    connect(m_dfmExitButton, SIGNAL(clicked()), this, SLOT(exitDistractioFreeMode()));

#ifdef SHOULD_BUILD_RDF
    if (KoDocumentRdf *rdf = dynamic_cast<KoDocumentRdf*>(m_document->documentRdf())) {
        connect(rdf, SIGNAL(semanticObjectViewSiteUpdated(hKoRdfSemanticItem,QString)),
                this, SLOT(semanticObjectViewSiteUpdated(hKoRdfSemanticItem,QString)));
    }
#endif
}

KWView::~KWView()
{
    m_canvas = 0;
}

KoCanvasBase *KWView::canvasBase() const
{
    return m_canvas;
}

const KWPage KWView::currentPage() const
{
    return m_currentPage;
}

QWidget *KWView::canvas() const
{
    return m_canvas;
}

void KWView::updateReadWrite(bool readWrite)
{
    m_actionFormatFrameSet->setEnabled(readWrite);
    m_actionViewHeader->setEnabled(readWrite);
    m_actionViewFooter->setEnabled(readWrite);
    m_actionViewSnapToGrid->setEnabled(readWrite);
    m_actionAddBookmark->setEnabled(readWrite);
    QAction *action = actionCollection()->action("insert_framebreak");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("insert_variable");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("select_bookmark"); // TODO fix the dialog to honor read-only instead
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("format_page");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("anchor");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("edit_cut");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("edit_copy");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("edit_paste");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("edit_paste_text");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("delete_page");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("edit_delete");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("create_linked_frame");
    if (action) action->setEnabled(readWrite);
    action = actionCollection()->action("create_custom_outline");
    if (action) action->setEnabled(readWrite);
}

void KWView::buildAssociatedWidget() {
    wordCount = new KWStatisticsWidget(this,true);
    wordCount->setLayoutDirection(KWStatisticsWidget::LayoutHorizontal);
    wordCount->setCanvas(dynamic_cast<KWCanvas*>(this->canvas()));
    statusBar()->insertWidget(0,wordCount);
}

void KWView::setupActions()
{
    m_actionFormatFrameSet  = new KAction(i18n("Shape Properties..."), this);
    actionCollection()->addAction("format_frameset", m_actionFormatFrameSet);
    m_actionFormatFrameSet->setToolTip(i18n("Change how the shape behave"));
    m_actionFormatFrameSet->setEnabled(false);
    connect(m_actionFormatFrameSet, SIGNAL(triggered()), this, SLOT(editFrameProperties()));

    m_actionAddBookmark = new KAction(koIcon("bookmark-new"), i18n("Bookmark..."), this);
    m_actionAddBookmark->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_G);
    actionCollection()->addAction("add_bookmark", m_actionAddBookmark);
    connect(m_actionAddBookmark, SIGNAL(triggered()), this, SLOT(addBookmark()));

    KAction *action = new KAction(i18n("Select Bookmark..."), this);
    action->setIconText(i18n("Bookmark"));
    action->setIcon(koIcon("bookmarks"));
    action->setShortcut(Qt::CTRL + Qt::Key_G);
    actionCollection()->addAction("select_bookmark", action);
    connect(action, SIGNAL(triggered()), this, SLOT(selectBookmark()));

    action = actionCollection()->addAction(KStandardAction::Prior,  "page_previous", this, SLOT(goToPreviousPage()));

    action = actionCollection()->addAction(KStandardAction::Next,  "page_next", this, SLOT(goToNextPage()));

    // -------------- File menu
    m_actionCreateTemplate = new KAction(i18n("Create Template From Document..."), this);
    m_actionCreateTemplate->setToolTip(i18n("Save this document and use it later as a template"));
    m_actionCreateTemplate->setWhatsThis(i18n("You can save this document as a template.<br><br>You can use this new template as a starting point for another document."));
    actionCollection()->addAction("extra_template", m_actionCreateTemplate);
    connect(m_actionCreateTemplate, SIGNAL(triggered()), this, SLOT(createTemplate()));

    // -------------- Edit actions
    action = actionCollection()->addAction(KStandardAction::Cut,  "edit_cut", 0, 0);
    new KoCutController(canvasBase(), action);
    action = actionCollection()->addAction(KStandardAction::Copy,  "edit_copy", 0, 0);
    new KoCopyController(canvasBase(), action);
    action = actionCollection()->addAction(KStandardAction::Paste,  "edit_paste", 0, 0);
    new KoPasteController(canvasBase(), action);

    action = new KAction(i18n("Select All Shapes"), this);
    actionCollection()->addAction("edit_selectallframes", action);
    connect(action, SIGNAL(triggered()), this, SLOT(editSelectAllFrames()));

    action = new KAction(koIcon("edit-delete"), i18n("Delete"), this);
    action->setShortcut(QKeySequence("Del"));
    connect(action, SIGNAL(triggered()), this, SLOT(editDeleteSelection()));
    connect(canvasBase()->toolProxy(), SIGNAL(selectionChanged(bool)), action, SLOT(setEnabled(bool)));
    actionCollection()->addAction("edit_delete", action);

    // -------------- View menu
    action = new KAction(i18n("Show Formatting Characters"), this);
    action->setCheckable(true);
    actionCollection()->addAction("view_formattingchars", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setShowFormattingChars(bool)));
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowFormattingCharacters, QVariant(false));
    action->setChecked(m_document->config().showFormattingChars()); // will change resource if true
    action->setToolTip(i18n("Toggle the display of non-printing characters"));
    action->setWhatsThis(i18n("Toggle the display of non-printing characters.<br/><br/>When this is enabled, Words shows you tabs, spaces, carriage returns and other non-printing characters."));

    action = new KAction(i18n("Show Field Shadings"), this);
    action->setCheckable(true);
    actionCollection()->addAction("view_fieldshadings", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setShowInlineObjectVisualization(bool)));
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowInlineObjectVisualization, QVariant(false));
    action->setChecked(m_document->config().showInlineObjectVisualization()); // will change resource if true
    action->setToolTip(i18n("Toggle the shaded background of fields"));
    action->setWhatsThis(i18n("Toggle the visualization of fields (variables etc.) by drawing their background in a contrasting color."));

    action = new KAction(i18n("Show Text Shape Borders"), this);
    action->setToolTip(i18n("Turns the border display on and off"));
    action->setCheckable(true);
    actionCollection()->addAction("view_frameborders", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleViewFrameBorders(bool)));
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowTextShapeOutlines, QVariant(false));
    action->setChecked(m_document->config().viewFrameBorders()); // will change resource if true
    action->setWhatsThis(i18n("Turns the border display on and off.<br/><br/>The borders are never printed. This option is useful to see how the document will appear on the printed page."));

    action = new KAction(i18n("Show Table Borders"), this);
    action->setCheckable(true);
    actionCollection()->addAction("view_tableborders", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(setShowTableBorders(bool)));
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowTableBorders, QVariant(false));
    action->setChecked(m_document->config().showTableBorders()); // will change resource if true
    action->setToolTip(i18n("Toggle the display of table borders"));
    action->setWhatsThis(i18n("Toggle the display of table borders.<br/><br/>When this is enabled, Words shows you any invisible table borders with a thin gray line."));

    action = new KAction(i18n("Show Rulers"), this);
    action->setCheckable(true);
    action->setToolTip(i18n("Shows or hides rulers"));
    action->setWhatsThis(i18n("The rulers are the white measuring spaces top and left of the "
                              "document. The rulers show the position and width of pages and of frames and can "
                              "be used to position tabulators among others.<p>Uncheck this to disable "
                              "the rulers from being displayed.</p>"));
    action->setChecked(m_document->config().viewRulers());
    actionCollection()->addAction("show_ruler", action);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(showRulers(bool)));

    action = m_document->gridData().gridToggleAction(m_canvas);
    actionCollection()->addAction("view_grid", action);

    m_actionViewSnapToGrid = new KToggleAction(i18n("Snap to Grid"), this);
    actionCollection()->addAction("view_snaptogrid", m_actionViewSnapToGrid);
    m_actionViewSnapToGrid->setChecked(m_snapToGrid);
    connect(m_actionViewSnapToGrid, SIGNAL(triggered()), this, SLOT(toggleSnapToGrid()));

    KToggleAction *guides = KoStandardAction::showGuides(this, SLOT(setGuideVisibility(bool)), actionCollection());
    guides->setChecked(m_document->guidesData().showGuideLines());

    KToggleAction *tAction = new KToggleAction(i18n("Show Status Bar"), this);
    tAction->setToolTip(i18n("Shows or hides the status bar"));
    actionCollection()->addAction("showStatusBar", tAction);
    connect(tAction, SIGNAL(toggled(bool)), this, SLOT(showStatusBar(bool)));

    tAction = new KToggleAction(i18n("Distraction Free Mode"), this);
    tAction->setToolTip(i18n("Set view in distraction free mode"));
    tAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    actionCollection()->addAction("view_distractionfreemode", tAction);
    connect(tAction, SIGNAL(toggled(bool)), this, SLOT(setDistractionFreeMode(bool)));

#ifdef SHOULD_BUILD_RDF
    action = new KAction(i18n("Semantic Stylesheets..."), this);
    actionCollection()->addAction("edit_semantic_stylesheets", action);
    action->setToolTip(i18n("Modify and add semantic stylesheets"));
    action->setWhatsThis(i18n("Stylesheets are used to format contact, event, and location information which is stored in Rdf"));
    connect(action, SIGNAL(triggered()), this, SLOT(editSemanticStylesheets()));

    if (KoDocumentRdf* rdf = dynamic_cast<KoDocumentRdf*>(m_document->documentRdf())) {
        KAction* createRef = rdf->createInsertSemanticObjectReferenceAction(canvasBase());
        actionCollection()->addAction("insert_semanticobject_ref", createRef);
        KActionMenu *subMenu = new KActionMenu(i18n("Create"), this);
        actionCollection()->addAction("insert_semanticobject_new", subMenu);
        foreach(KAction *action, rdf->createInsertSemanticObjectNewActions(canvasBase())) {
            subMenu->addAction(action);
        }
    }
#endif

    // -------------- Frame menu
    action  = new KAction(i18n("Create Linked Copy"), this);
    actionCollection()->addAction("create_linked_frame", action);
    action->setToolTip(i18n("Create a copy of the current frame, always showing the same contents"));
    action->setWhatsThis(i18n("Create a copy of the current frame, that remains linked to it. This means they always show the same contents: modifying the contents in such a frame will update all its linked copies."));
    connect(action, SIGNAL(triggered()), this, SLOT(createLinkedFrame()));

    // -------------- Settings menu
    action = new KAction(koIcon("configure"), i18n("Configure..."), this);
    actionCollection()->addAction("configure", action);
    connect(action, SIGNAL(triggered()), this, SLOT(configure()));

    // -------------- Page tool
    action = new KAction(i18n("Page Layout..."), this);
    actionCollection()->addAction("format_page", action);
    action->setToolTip(i18n("Change properties of entire page"));
    action->setWhatsThis(i18n("Change properties of the entire page.<p>Currently you can change paper size, paper orientation, header and footer sizes, and column settings.</p>"));
    connect(action, SIGNAL(triggered()), this, SLOT(formatPage()));

    m_actionViewHeader = new KAction(i18n("Create Header"), this);
    actionCollection()->addAction("insert_header", m_actionViewHeader);
    if (m_currentPage.isValid())
        m_actionViewHeader->setEnabled(m_currentPage.pageStyle().headerPolicy() == Words::HFTypeNone);
    connect(m_actionViewHeader, SIGNAL(triggered()), this, SLOT(enableHeader()));

    m_actionViewFooter = new KAction(i18n("Create Footer"), this);
    actionCollection()->addAction("insert_footer", m_actionViewFooter);
    if (m_currentPage.isValid())
        m_actionViewFooter->setEnabled(m_currentPage.pageStyle().footerPolicy() == Words::HFTypeNone);
    connect(m_actionViewFooter, SIGNAL(triggered()), this, SLOT(enableFooter()));


    // -------- Statistics in the status bar
    KToggleAction *tActionBis = new KToggleAction(i18n("Word Count"), this);
    tActionBis->setToolTip(i18n("Shows or hides word counting in status bar"));
    tActionBis->setChecked(kwdocument()->config().statusBarShowWordCount());
    actionCollection()->addAction("view_wordCount", tActionBis);
    connect(tActionBis, SIGNAL(toggled(bool)), this, SLOT(showWordCountInStatusBar(bool)));
    wordCount->setVisible(kwdocument()->config().statusBarShowWordCount());

    /* ********** From old kwview ****
    We probably want to have each of these again, so just move them when you want to implement it
    This saves problems with finding out which we missed near the end.

    m_actionEditCustomVarsEdit = new KAction(i18n("Custom Variables..."), 0,
            this, SLOT(editCustomVars()), // TODO: new dialog w add etc.
            actionCollection(), "custom_vars");

    m_actionEditCustomVars = new KAction(i18n("Edit Variable..."), 0,
            this, SLOT(editCustomVariable()),
            actionCollection(), "edit_customvars");

    m_actionImportStyle= new KAction(i18n("Import Styles..."), 0,
            this, SLOT(importStyle()),
            actionCollection(), "import_style");
    m_actionAddLinkToBookmak = new KAction(i18n("Add to Bookmark"), 0,
            this, SLOT(addToBookmark()),
            actionCollection(), "add_to_bookmark");

    m_actionAddBookmark= new KAction(i18n("Bookmark..."), 0,
            this, SLOT(addBookmark()),
            actionCollection(), "add_bookmark");
    m_actionSelectBookmark= new KAction(i18n("Select Bookmark..."), 0,
            this, SLOT(selectBookmark()),
            actionCollection(), "select_bookmark");

    m_actionConfigureCompletion = new KAction(i18n("Configure Completion..."), 0,
            this, SLOT(configureCompletion()),
            actionCollection(), "configure_completion");
    m_actionConfigureCompletion->setToolTip(i18n("Change the words and options for autocompletion"));
    m_actionConfigureCompletion->setWhatsThis(i18n("Add words or change the options for autocompletion."));

    new KAction(i18n("Completion"), KStdAccel::shortcut(KStdAccel::TextCompletion), this, SLOT(slotCompletion()), actionCollection(), "completion");
    */
}

QList<KWFrame*> KWView::selectedFrames() const
{
    QList<KWFrame*> frames;
    foreach (KoShape *shape, canvasBase()->shapeManager()->selection()->selectedShapes()) {
        KWFrame *frame = kwdocument()->frameOfShape(shape);
        Q_ASSERT(frame);
        frames.append(frame);
    }
    return frames;
}

KoShape* KWView::selectedShape() const
{
    KoSelection *selection = canvasBase()->shapeManager()->selection();

    foreach (KoShape *s, selection->selectedShapes(KoFlake::TopLevelSelection)) {
        if (s->isGeometryProtected())
            continue;
        return s;
    }

    return 0;
}

// -------------------- Actions -----------------------

void KWView::pasteRequested()
{
    QImage img = QApplication::clipboard()->image();

    if (!img.isNull()) {
        QList<QImage> images;
        images.append(img);
        addImages(images, canvas()->mapFromGlobal(QCursor::pos()));
    }
}

void KWView::showWordCountInStatusBar(bool toggled)
{
    kwdocument()->config().setStatusBarShowWordCount(toggled);
    wordCount->setVisible(toggled);
}

void KWView::editFrameProperties()
{
    QPointer<KWFrameDialog> frameDialog = new KWFrameDialog(selectedFrames(), m_document, m_canvas);
    frameDialog->exec();
    delete frameDialog;
}

KoPrintJob *KWView::createPrintJob()
{
    KWPrintingDialog *dia = new KWPrintingDialog(m_document, m_canvas->shapeManager(), this);
    dia->printer().setResolution(600);
    dia->printer().setCreator(QString("Words %1.%2.%3").arg(Calligra::versionMajor())
                              .arg(Calligra::versionMinor()).arg(Calligra::versionRelease()));
    dia->printer().setFullPage(true); // ignore printer margins
    return dia;
}

void KWView::createTemplate()
{
    KoTemplateCreateDia::createTemplate("words_template", ".ott",
                                        KWFactory::componentData(), m_document, this);
}

void KWView::addBookmark()
{
    QString name, suggestedName;

    KoSelection *selection = canvasBase()->shapeManager()->selection();
    KoShape *shape = 0;
    shape = selection->firstSelectedShape();
    if (shape == 0) return; // no shape selected

    KWFrame *frame = kwdocument()->frameOfShape(shape);
    Q_ASSERT(frame);
    KWTextFrameSet *fs = dynamic_cast<KWTextFrameSet*>(frame->frameSet());
    if (fs == 0) return;

    QString tool = KoToolManager::instance()->preferredToolForSelection(selection->selectedShapes());
    KoToolManager::instance()->switchToolRequested(tool);
    KoTextEditor *editor = KoTextEditor::getTextEditorFromCanvas(canvasBase());
    Q_ASSERT(editor);

    const KoBookmarkManager *manager = m_document->textRangeManager()->bookmarkManager();
    if (editor->hasSelection()) {
        suggestedName = editor->selectedText();
    }

    QPointer<KWCreateBookmarkDialog> dia = new KWCreateBookmarkDialog(manager->bookmarkNameList(), suggestedName, m_canvas->canvasWidget());
    if (dia->exec() == QDialog::Accepted) {
        name = dia->newBookmarkName();
    }
    else {
        delete dia;
        return;
    }
    delete dia;

    editor->addBookmark(name);
}

void KWView::selectBookmark()
{
    QString name;
    const KoBookmarkManager *manager = m_document->textRangeManager()->bookmarkManager();

    QPointer<KWSelectBookmarkDialog> dia = new KWSelectBookmarkDialog(manager->bookmarkNameList(), m_canvas->canvasWidget());
    connect(dia, SIGNAL(nameChanged(const QString &, const QString &)),
            manager, SLOT(rename(const QString &, const QString &)));
    connect(dia, SIGNAL(bookmarkDeleted(const QString &)),
            this, SLOT(deleteBookmark(const QString &)));
    if (dia->exec() == QDialog::Accepted)
        name = dia->selectedBookmarkName();
    else {
        delete dia;
        return;
    }
    delete dia;
    KoBookmark *bookmark = manager->bookmark(name);
#if 0
    KoShape *shape = bookmark->shape();
    KoSelection *selection = canvasBase()->shapeManager()->selection();
    selection->deselectAll();
    selection->select(shape);

    QString tool = KoToolManager::instance()->preferredToolForSelection(selection->selectedShapes());
    KoToolManager::instance()->switchToolRequested(tool);
#else
#ifdef __GNUC__
    #warning FIXME: port to textlayout-rework
#endif
#endif

    KoCanvasResourceManager *rm = m_canvas->resourceManager();
    if ((bookmark->positionOnlyMode() == false) && bookmark->hasRange()) {
        rm->clearResource(KoText::SelectedTextPosition);
        rm->clearResource(KoText::SelectedTextAnchor);
    }
    if (bookmark->positionOnlyMode()) {
        rm->setResource(KoText::CurrentTextPosition, bookmark->rangeStart());
        rm->setResource(KoText::CurrentTextAnchor, bookmark->rangeStart());
    } else {
        rm->setResource(KoText::CurrentTextPosition, bookmark->rangeStart());
        rm->setResource(KoText::CurrentTextAnchor, bookmark->rangeEnd());
    }
}

void KWView::deleteBookmark(const QString &name)
{
    Q_UNUSED(name);
#if 0
    KoInlineTextObjectManager*manager = m_document->inlineTextObjectManager();
    KoBookmark *bookmark = manager->bookmarkManager()->bookmark(name);
    if (!bookmark || !bookmark->shape())
        return;

    KoTextShapeData *data = qobject_cast<KoTextShapeData*>(bookmark->shape()->userData());
    if (!data)
        return;
    QTextCursor cursor(data->document());
    if (bookmark->hasSelection()) {
        cursor.setPosition(bookmark->endBookmark()->position() - 1);
        manager->removeInlineObject(cursor);
    }
    cursor.setPosition(bookmark->position());
    manager->removeInlineObject(cursor);
#else
#ifdef __GNUC__
    #warning FIXME: port to textlayout-rework
#endif
#endif
}

void KWView::enableHeader()
{
    if (!m_currentPage.isValid())
        return;
    Q_ASSERT(m_currentPage.pageStyle().isValid());
    m_currentPage.pageStyle().setHeaderPolicy(Words::HFTypeUniform);
    m_actionViewHeader->setEnabled(false);
    m_document->relayout();
}

void KWView::enableFooter()
{
    if (!m_currentPage.isValid())
        return;
    Q_ASSERT(m_currentPage.pageStyle().isValid());
    m_currentPage.pageStyle().setFooterPolicy(Words::HFTypeUniform);
    m_actionViewFooter->setEnabled(false);
    m_document->relayout();
}

void KWView::toggleSnapToGrid()
{
    m_snapToGrid = !m_snapToGrid;
    m_document->gridData().setSnapToGrid(m_snapToGrid); // for persistency
}


void KWView::toggleViewFrameBorders(bool on)
{
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowTextShapeOutlines, on);
    m_canvas->update();
    m_document->config().setViewFrameBorders(on);
}

void KWView::setShowInlineObjectVisualization(bool on)
{
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowInlineObjectVisualization, QVariant(on));
    m_canvas->update();
    m_document->config().setShowInlineObjectVisualization(on);
}

void KWView::setShowFormattingChars(bool on)
{
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowFormattingCharacters, QVariant(on));
    m_canvas->update();
    m_document->config().setShowFormattingChars(on);
}

void KWView::setShowTableBorders(bool on)
{
    m_canvas->resourceManager()->setResource(KoCanvasResourceManager::ShowTableBorders, QVariant(on));
    m_canvas->update();
    m_document->config().setShowTableBorders(on);
}

void KWView::formatPage()
{
    if (! m_currentPage.isValid())
        return;
    KWPageSettingsDialog *dia = new KWPageSettingsDialog(this, m_document, m_currentPage);
    if (!m_lastPageSettingsTab.isEmpty()) {
        KPageWidgetItem *item = dia->pageItem(m_lastPageSettingsTab);
        if (item)
            dia->setCurrentPage(item);
    }
    connect(dia, SIGNAL(finished()), this, SLOT(pageSettingsDialogFinished()));
    dia->show();
}

void KWView::pageSettingsDialogFinished()
{
    KWPageSettingsDialog *dia = qobject_cast<KWPageSettingsDialog*>(QObject::sender());
    m_lastPageSettingsTab = dia && dia->currentPage() ? dia->currentPage()->name() : QString();
}

void KWView::editSemanticStylesheets()
{
#ifdef SHOULD_BUILD_RDF
    if (KoDocumentRdf *rdf = dynamic_cast<KoDocumentRdf*>(m_document->documentRdf())) {
        KoSemanticStylesheetsEditor *dia = new KoSemanticStylesheetsEditor(this, rdf);
        dia->show();
        // TODO this leaks memory
    }
#endif
}

void KWView::showRulers(bool visible)
{
    m_document->config().setViewRulers(visible);
    m_gui->updateRulers();
}

void KWView::createLinkedFrame()
{
    KoSelection *selection = canvasBase()->shapeManager()->selection();
    QList<KoShape*> oldSelection = selection->selectedShapes(KoFlake::TopLevelSelection);
    if (oldSelection.count() == 0)
        return;
    selection->deselectAll();

    KUndo2Command *cmd = new KUndo2Command(i18nc("(qtundo-format)", "Create Linked Copy"));
    foreach (KoShape *shape, oldSelection) {
        KWFrame *frame = dynamic_cast<KWFrame*>(shape->applicationData());
        Q_ASSERT(frame);
        KWCopyShape *copy = new KWCopyShape(frame->shape(), m_document->pageManager());
        copy->setPosition(frame->shape()->position());
        QPointF offset(40, 40);
        canvasBase()->clipToDocument(copy, offset);
        copy->setPosition(frame->shape()->position() + offset);
        copy->setZIndex(frame->shape()->zIndex() + 1);
        KWFrame *newFrame = new KWFrame(copy, frame->frameSet());
        new KWFrameCreateCommand(m_document, newFrame, cmd);
        selection->select(copy);
    }
    m_document->addCommand(cmd);
}

void KWView::showStatusBar(bool toggled)
{
    if (statusBar()) statusBar()->setVisible(toggled);
}

void KWView::setDistractionFreeMode(bool status)
{
    m_isDistractionFreeMode = status;

    shell()->toggleDockersVisibility(!status);
    shell()->menuBar()->setVisible(!status);

    shell()->viewFullscreen(status);
    foreach(KToolBar* toolbar, shell()->toolBars()) {
        if (toolbar->isVisible() == status) {
            toolbar->setVisible(!status);
        }
    }

    if (status) {
         QTimer::singleShot(2000, this, SLOT(hideUI()));
    } else {
         shell()->statusBar()->setVisible(true);
         static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
         static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    // Exit Distraction-Free mode button.
    m_dfmExitButton->setVisible(status);

    //Hide cursor.
    if(status) {
        m_hideCursorTimer->start(4000);
    }
    else {
        // FIXME: Return back cursor to canvas if cursor is blank cursor.
        m_hideCursorTimer->stop();
    }
}

void KWView::hideUI()
{
    if (m_isDistractionFreeMode) {
        shell()->statusBar()->setVisible(false);
        // Hide vertical  and horizontal scroll bar.
        static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void KWView::hideCursor(){
    m_canvas->setCursor(Qt::BlankCursor);
    m_gui->setCursor(Qt::BlankCursor);
}

void KWView::exitDistractioFreeMode()
{
    if (m_isDistractionFreeMode) {
        QAction *action = actionCollection()->action("view_distractionfreemode");
        action->setChecked(false);
        m_gui->setCursor(Qt::ArrowCursor);
        setDistractionFreeMode(false);
    }
}

void KWView::viewMouseMoveEvent(QMouseEvent *e)
{
    if (!m_isDistractionFreeMode)
        return;

    m_gui->setCursor(Qt::ArrowCursor);

    // Handle stause bar and horizonta scroll bar.
    if (e->y() >= (m_gui->size().height() - statusBar()->size().height())) {
        shell()->statusBar()->setVisible(true);
        static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    else {
       shell()->statusBar()->setVisible(false);
       static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    // Handle vertical scroll bar.
    QScrollBar *vsb = static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->verticalScrollBar();
    if (e->x() >= (m_gui->size().width() - vsb->size().width() - 10)) {
         static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    }
    else {
         static_cast<KoCanvasControllerWidget*>(m_gui->canvasController())->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void KWView::editSelectAllFrames()
{
    KoSelection *selection = canvasBase()->shapeManager()->selection();
    foreach (KWFrameSet *fs, m_document->frameSets()) {
        foreach (KWFrame *frame, fs->frames()) {
            if (frame->shape()->isVisible())
                selection->select(frame->shape());
        }
    }
}

void KWView::editDeleteSelection()
{
    canvasBase()->toolProxy()->deleteSelection();
}

void KWView::setGuideVisibility(bool on)
{
    m_document->guidesData().setShowGuideLines(on);
    m_canvas->update();
}


void KWView::configure()
{
    QPointer<KWConfigureDialog> dialog(new KWConfigureDialog(this));
    dialog->exec();
    delete dialog;
    // TODO update canvas
}

// end of actions

void KWView::popupContextMenu(const QPoint &globalPosition, const QList<QAction*> &actions)
{
    unplugActionList("frameset_type_action");
    plugActionList("frameset_type_action", actions);
    if (factory() == 0) // we are a kpart, the factory is only set on the active component.
        return;
    QMenu *menu = dynamic_cast<QMenu*>(factory()->container("frame_popup", this));
    if (menu)
        menu->exec(globalPosition);
}

void KWView::zoomChanged(KoZoomMode::Mode mode, qreal zoom)
{
    m_document->config().setZoom(qRound(zoom * 100.0));
    m_document->config().setZoomMode(mode);
    m_canvas->update();
}

void KWView::selectionChanged()
{
    KoShape *shape = canvasBase()->shapeManager()->selection()-> firstSelectedShape();

    m_actionFormatFrameSet->setEnabled(shape != 0);
    m_actionAddBookmark->setEnabled(shape != 0);
    if (shape) {
        setCurrentPage(m_document->pageManager()->page(shape));
        KWFrame *frame = kwdocument()->frameOfShape(shape);
        KWTextFrameSet *fs = frame == 0 ? 0 : dynamic_cast<KWTextFrameSet*>(frame->frameSet());
        if (fs)
            m_actionAddBookmark->setEnabled(true);
        else
            m_actionAddBookmark->setEnabled(false);
    }
    // actions that need at least one shape selected
    QAction *action = actionCollection()->action("create_linked_frame");
    if (action) action->setEnabled(shape);
    action = actionCollection()->action("anchor");
    if (action) action->setEnabled(shape && kwdocument()->mainFrameSet());

    foreach (KoShape *shape, canvasBase()->shapeManager()->selection()->selectedShapes(KoFlake::TopLevelSelection)) {
        KWFrame *frame = kwdocument()->frameOfShape(shape);
        Q_ASSERT(frame);
        QVariant variant;
        variant.setValue<void*>(frame);
        m_canvas->resourceManager()->setResource(Words::CurrentFrame, variant);
        variant.setValue<void*>(frame->frameSet());
        m_canvas->resourceManager()->setResource(Words::CurrentFrameSet, variant);
        break;
    }
}

void KWView::setCurrentPage(const KWPage &currentPage)
{
    Q_ASSERT(currentPage.isValid());
    if (currentPage != m_currentPage) {
        m_currentPage = currentPage;
        m_canvas->resourceManager()->setResource(KoCanvasResourceManager::CurrentPage, m_currentPage.pageNumber());

        m_actionViewHeader->setEnabled(m_currentPage.pageStyle().headerPolicy() == Words::HFTypeNone);
        m_actionViewFooter->setEnabled(m_currentPage.pageStyle().footerPolicy() == Words::HFTypeNone);
    }
}

void KWView::goToPreviousPage(Qt::KeyboardModifiers modifiers)
{
    // Scroll display
    qreal moveDistance = m_canvas->canvasController()->visibleHeight() * 0.8;
    m_canvas->canvasController()->pan(QPoint(0, -moveDistance));

    // Find current frameset, FIXME for now assume main
    KWTextFrameSet *currentFrameSet = kwdocument()->mainFrameSet();

    // Since we move _up_ calculate the position where a frame would _start_ if
    // we were scrolled to the _first_ page
    QPointF pos = currentFrameSet->frames().first()->shape()->absoluteTransformation(0).map(QPointF(0, 5));

    pos += m_canvas->viewMode()->viewToDocument(m_canvas->documentOffset(), viewConverter());

    // Find textshape under that position and from current frameset
    QList<KoShape*> possibleTextShapes = m_canvas->shapeManager()->shapesAt(QRectF(pos.x() - 20, pos.y() -20, 40, 40));
    KoTextShapeData *textShapeData = 0;
    foreach (KoShape* shape, possibleTextShapes) {
        KoShapeUserData *userData = shape->userData();
        if ((textShapeData = dynamic_cast<KoTextShapeData*>(userData))) {
            foreach (KWFrame *frame, currentFrameSet->frames()) {
                if (frame->shape() == shape) {
                    pos = shape->absoluteTransformation(0).inverted().map(pos);
                     pos += QPointF(0.0, textShapeData->documentOffset());

                    int cursorPos = textShapeData->rootArea()->hitTest(pos, Qt::FuzzyHit).position;
                    KoTextDocument(textShapeData->document()).textEditor()->setPosition(cursorPos, (modifiers & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return;
                }
            }
        }
    }
}

void KWView::goToNextPage(Qt::KeyboardModifiers modifiers)
{
    // Scroll display
    qreal moveDistance = m_canvas->canvasController()->visibleHeight() * 0.8;
    m_canvas->canvasController()->pan(QPoint(0, moveDistance));

    // Find current frameset, FIXME for now assume main
    KWTextFrameSet *currentFrameSet = kwdocument()->mainFrameSet();

    // Since we move _down_ calculate the position where a frame would _end_ if
    // we were scrolled to the _lasst_ page
    KoShape *shape = currentFrameSet->frames().last()->shape();
    QPointF pos = shape->absoluteTransformation(0).map(QPointF(0, shape->size().height() - 5));
    pos.setY(pos.y() - m_document->pageManager()->page(qreal(pos.y())).rect().bottom());

    pos += m_canvas->viewMode()->viewToDocument(m_canvas->documentOffset() + QPointF(0, m_canvas->canvasController()->visibleHeight()), viewConverter());

    // Find textshape under that position and from current frameset
    QList<KoShape*> possibleTextShapes = m_canvas->shapeManager()->shapesAt(QRectF(pos.x() - 20, pos.y() -20, 40, 40));
    KoTextShapeData *textShapeData = 0;
    foreach (KoShape* shape, possibleTextShapes) {
        KoShapeUserData *userData = shape->userData();
        if ((textShapeData = dynamic_cast<KoTextShapeData*>(userData))) {
            foreach (KWFrame *frame, currentFrameSet->frames()) {
                if (frame->shape() == shape) {
                    pos = shape->absoluteTransformation(0).inverted().map(pos);
                    pos += QPointF(0.0, textShapeData->documentOffset());

                    int cursorPos = textShapeData->rootArea()->hitTest(pos, Qt::FuzzyHit).position;
                    KoTextDocument(textShapeData->document()).textEditor()->setPosition(cursorPos, (modifiers & Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
                    return;
                }
            }
        }
    }
}

void KWView::goToPage(const KWPage &page)
{
    KoCanvasController *controller = m_gui->canvasController();
    QPoint origPos = controller->scrollBarValue();
    QPointF pos = m_canvas->viewMode()->documentToView(QPointF(0,
                            page.offsetInDocument()), m_canvas->viewConverter());
    origPos.setY((int)pos.y());
    controller->setScrollBarValue(origPos);
}

void KWView::showEvent(QShowEvent *e)
{
    KoView::showEvent(e);
    QTimer::singleShot(0, this, SLOT(updateStatusBarAction()));
}

void KWView::updateStatusBarAction()
{
    KToggleAction *action = (KToggleAction*) actionCollection()->action("showStatusBar");
    if (action && statusBar())
        action->setChecked(! statusBar()->isHidden());
}

void KWView::offsetInDocumentMoved(int yOffset)
{
    const qreal offset = -m_zoomHandler.viewToDocumentY(yOffset);
    const qreal height = m_zoomHandler.viewToDocumentY(m_gui->viewportSize().height());
/*    if (m_currentPage.isValid()) { // most of the time the current will not change.
        const qreal pageTop = m_currentPage.offsetInDocument();
        const qreal pageBottom = pageTop + m_currentPage.height();
        const qreal visibleArea = qMin(offset + height, pageBottom) - qMax(pageTop, offset);
        if (visibleArea / height >= 0.45) // current page is just fine if > 45% is shown
            return;

        // using 'next'/'prev' is much cheaper than using a documentOffset, so try that first.
        if (pageTop > offset && pageTop < offset + height) { // check if the page above is a candidate.
            KWPage page = m_currentPage.previous();
            if (page.isValid() && pageTop - offset > visibleArea) {
                firstDrawnPage = page;
                return;
            }
        }
        if (pageBottom > offset && pageBottom < offset + height) { // check if the page above is a candidate.
            KWPage page = m_currentPage.next();
            if (page.isValid() && m_currentPage.height() - height > visibleArea) {
                firstDrawnPage = page;
                return;
            }
        }
    }
*/
    KWPage page = m_document->pageManager()->page(qreal(offset));
    qreal pageTop = page.offsetInDocument();
    QSizeF maxPageSize;
    qreal minTextX = std::numeric_limits<qreal>::max();
    qreal maxTextX = std::numeric_limits<qreal>::min();
    int minPageNum = page.pageNumber();
    int maxPageNum = page.pageNumber();
    while (page.isValid() && pageTop < qreal(offset + height)) {
        pageTop += page.height();
        QSizeF pageSize = page.rect().size();
        maxPageSize = QSize(qMax(maxPageSize.width(), pageSize.width()),
                                     qMax(maxPageSize.height(), pageSize.height()));
        minTextX = qMin(minTextX, page.contentRect().left());
        maxTextX = qMax(maxTextX, page.contentRect().right());
        maxPageNum = page.pageNumber();
        page = page.next();
    }

    if (maxPageSize != m_pageSize) {
        m_pageSize = maxPageSize;
        m_zoomController->setPageSize(m_pageSize);
    }
    if (minTextX != m_textMinX || maxTextX != m_textMaxX) {
        m_textMinX = minTextX;
        m_textMaxX = maxTextX;
        m_zoomController->setTextMinMax(minTextX, maxTextX);
    }
    if (minPageNum != m_minPageNum || maxPageNum != m_maxPageNum) {
        m_minPageNum = minPageNum;
        m_maxPageNum = maxPageNum;
        emit shownPagesChanged();
    }
}

#ifdef SHOULD_BUILD_RDF
void KWView::semanticObjectViewSiteUpdated(hKoRdfSemanticItem item, const QString &xmlid)
{
    kDebug(30015) << "xmlid:" << xmlid << " reflow item:" << item->name();
    KoTextEditor *editor = KoTextEditor::getTextEditorFromCanvas(canvasBase());
    if (!editor) {
        kDebug(30015) << "no editor, not reflowing rdf semantic item.";
        return;
    }
    kDebug(30015) << "reflowing rdf semantic item.";
    KoRdfSemanticItemViewSite vs(item, xmlid);
    vs.reflowUsingCurrentStylesheet(editor);
}
#endif

void KWView::findMatchFound(KoFindMatch match)
{
    if(!match.isValid() || !match.location().canConvert<QTextCursor>() || !match.container().canConvert<QTextDocument*>()) {
        return;
    }

    QTextCursor cursor = match.location().value<QTextCursor>();

    m_canvas->resourceManager()->setResource(KoText::CurrentTextAnchor, cursor.anchor());
    m_canvas->resourceManager()->setResource(KoText::CurrentTextPosition, cursor.position());
}

void KWView::refreshFindTexts()
{
    QList<QTextDocument*> texts;
    foreach (KWFrameSet *fSet, m_document->frameSets()) {
        KWTextFrameSet *tFSet = dynamic_cast<KWTextFrameSet *>(fSet);
        if (tFSet) {
           texts.append(tFSet->document());
        }
    }
    m_find->setDocuments(texts);
}

void KWView::addImages(const QList<QImage> &imageList, const QPoint &insertAt)
{
    if (!m_canvas) {
        // no canvas because we're not on the desktop?
        return;
    }

    QPointF pos = viewConverter()->viewToDocument(m_canvas->documentOffset() + insertAt - KWView::pos());
    pos.setX(qMax(qreal(0), pos.x()));
    pos.setY(qMax(qreal(0), pos.y()));

    // create a factory
    KoShapeFactoryBase *factory = KoShapeRegistry::instance()->value("PictureShape");

    if (!factory) {
        kWarning(30003) << "No picture shape found, cannot drop images.";
        return;
    }

    foreach(const QImage &image, imageList) {
        KoProperties params;
        params.setProperty("qimage", image);

        KoShape *shape = factory->createShape(&params, kwdocument()->resourceManager());

        // resize the shape so it will fit in the document, with some nice
        // hard-coded constants.
        qreal pageWidth = currentPage().width();
        qreal pageHeight = currentPage().height();

        if (shape->size().width() > (pageWidth * 0.8) || shape->size().height() > pageHeight) {
            QSizeF sz = shape->size();
            sz.scale(QSizeF(pageWidth * 0.6, pageHeight *.6), Qt::KeepAspectRatio);
            shape->setSize(sz);
        }

        if (!shape) {
            kWarning(30003) << "Could not create a shape from the image";
            return;
        }

        shape->setTextRunAroundSide(KoShape::BothRunAroundSide);

        KoShapeAnchor *anchor = new KoShapeAnchor(shape);
        anchor->setAnchorType(KoShapeAnchor::AnchorPage);
        anchor->setHorizontalPos(KoShapeAnchor::HFromLeft);
        anchor->setVerticalPos(KoShapeAnchor::VFromTop);
        anchor->setHorizontalRel(KoShapeAnchor::HPage);
        anchor->setVerticalRel(KoShapeAnchor::VPage);
        shape->setAnchor(anchor);
        shape->setPosition(pos);

        pos += QPointF(25,25); // increase the position for each shape we insert so the
                               // user can see them all.

        // create the undo step.
        KWShapeCreateCommand *cmd = new KWShapeCreateCommand(kwdocument(), shape);
        KoSelection *selection = m_canvas->shapeManager()->selection();
        selection->deselectAll();
        selection->select(shape);
        m_canvas->addCommand(cmd);
    }
}

