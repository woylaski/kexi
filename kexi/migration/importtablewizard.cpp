/* This file is part of the KDE project
   Copyright (C) 2009 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2014 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "importtablewizard.h"
#include "migratemanager.h"
#include "keximigrate.h"
#include "keximigratedata.h"
#include "AlterSchemaWidget.h"
#include <KexiIcon.h>
#include <core/kexidbconnectionset.h>
#include <core/kexi.h>
#include <core/kexipartmanager.h>
#include <kexiutils/utils.h>
#include <kexidbdrivercombobox.h>
#include <kexitextmsghandler.h>
#include <kexipart.h>
#include <KexiMainWindowIface.h>
#include <kexiproject.h>
#include <widget/kexicharencodingcombobox.h>
#include <widget/kexiprjtypeselector.h>
#include <widget/KexiConnectionSelectorWidget.h>
#include <widget/KexiProjectSelectorWidget.h>
#include <widget/KexiDBTitlePage.h>
#include <widget/KexiFileWidget.h>
#include <widget/KexiNameWidget.h>

#include <KDbDriverManager>
#include <KDbDriver>
#include <KDbConnectionData>
#include <KDbUtils>
#include <KDbDriverManager>

#include <KMessageBox>

#include <QSet>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStringList>
#include <QProgressBar>
#include <QCheckBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QDebug>

using namespace KexiMigration;

#define ROWS_FOR_PREVIEW 3

ImportTableWizard::ImportTableWizard ( KDbConnection* curDB, QWidget* parent, QMap<QString, QString>* args, Qt::WFlags flags)
    : KAssistantDialog ( parent, flags ),
      m_args(args)
{
    m_connection = curDB;
    m_migrateDriver = 0;
    m_prjSet = 0;
    m_migrateManager = new MigrateManager();
    m_importComplete = false;
    m_importWasCanceled = false;

    KexiMainWindowIface::global()->setReasonableDialogSize(this);

    setupIntroPage();
    setupSrcConn();
    setupSrcDB();
    setupTableSelectPage();
    setupAlterTablePage();
    setupImportingPage();
    setupProgressPage();
    setupFinishPage();

    setValid(m_srcConnPageItem, false);

    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)), this, SLOT(slot_currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)));
    //! @todo Change this to message prompt when we move to non-dialog wizard.
    connect(m_srcConnSel, SIGNAL(connectionSelected(bool)), this, SLOT(slotConnPageItemSelected(bool)));
}


ImportTableWizard::~ImportTableWizard() {
  delete m_migrateManager;
  delete m_prjSet;
  delete m_srcConnSel;

}

void ImportTableWizard::back() {
    KAssistantDialog::back();
}

void ImportTableWizard::next() {
    if (currentPage() == m_srcConnPageItem) {
        if (fileBasedSrcSelected()) {
            setAppropriate(m_srcDBPageItem, false);
        } else {
            setAppropriate(m_srcDBPageItem, true);
        }
    } else if (currentPage() == m_alterTablePageItem) {
        if (m_alterSchemaWidget->nameExists(m_alterSchemaWidget->nameWidget()->nameText())) {
            KMessageBox::information(this,
                                     xi18n("<resource>%1</resource> name is already used by an existing table. "
                                          "Enter different table name to continue.", m_alterSchemaWidget->nameWidget()->nameText()),
                                     xi18n("Name Already Used"));
            return;
        }
    }

    KAssistantDialog::next();
}

void ImportTableWizard::accept() {
    if (m_args) {
        if (m_finishCheckBox->isChecked()) {
            m_args->insert("destinationTableName",m_alterSchemaWidget->nameWidget()->nameText());
        } else {
            m_args->remove("destinationTableName");
        }
    }

    QDialog::accept();
}

void ImportTableWizard::reject() {
    QDialog::reject();
}

//===========================================================
//
void ImportTableWizard::setupIntroPage()
{
    m_introPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();

    m_introPageWidget->setLayout(vbox);

    KexiUtils::setStandardMarginsAndSpacing(vbox);

    QLabel *lblIntro = new QLabel(m_introPageWidget);
    lblIntro->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    lblIntro->setWordWrap(true);
    lblIntro->setText(
        xi18n("<para>Table Importing Assistant allows you to import a table from an existing "
             "database into the current Kexi project.</para>"
             "<para>Click <interface>Next</interface> button to continue or "
             "<interface>Cancel</interface> button to exit this assistant.</para>"));
    vbox->addWidget(lblIntro);

    m_introPageItem = new KPageWidgetItem(m_introPageWidget,
                                          xi18n("Welcome to the Table Importing Assistant"));
    addPage(m_introPageItem);
}

void ImportTableWizard::setupSrcConn()
{
    m_srcConnPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(m_srcConnPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    m_srcConnSel = new KexiConnectionSelectorWidget(&Kexi::connset(),
                                           "kfiledialog:///ProjectMigrationSourceDir",
                                           KFileWidget::Opening, m_srcConnPageWidget);

    m_srcConnSel->hideConnectonIcon();
    m_srcConnSel->showSimpleConn();

    QSet<QString> excludedFilters;
    //! @todo remove when support for kexi files as source prj is added in migration
    excludedFilters
        << KDb::defaultFileBasedDriverMimeType()
        << "application/x-kexiproject-shortcut"
        << "application/x-kexi-connectiondata";
    m_srcConnSel->fileWidget->setExcludedFilters(excludedFilters);

    vbox->addWidget(m_srcConnSel);

    m_srcConnPageItem = new KPageWidgetItem(m_srcConnPageWidget, xi18n("Select Location for Source Database"));
    addPage(m_srcConnPageItem);
}

void ImportTableWizard::setupSrcDB()
{
    // arrivesrcdbPage creates widgets on that page
    m_srcDBPageWidget = new QWidget(this);
    m_srcDBName = NULL;

    m_srcDBPageItem = new KPageWidgetItem(m_srcDBPageWidget, xi18n("Select Source Database"));
    addPage(m_srcDBPageItem);
}


void ImportTableWizard::setupTableSelectPage() {
    m_tablesPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(m_tablesPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    m_tableListWidget = new QListWidget(this);
    m_tableListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_tableListWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotTableListWidgetSelectionChanged()));

    vbox->addWidget(m_tableListWidget);

    m_tablesPageItem = new KPageWidgetItem(m_tablesPageWidget, xi18n("Select the Table to Import"));
    addPage(m_tablesPageItem);
}

//===========================================================
//
void ImportTableWizard::setupImportingPage()
{
    m_importingPageWidget = new QWidget(this);
    m_importingPageWidget->hide();
    QVBoxLayout *vbox = new QVBoxLayout(m_importingPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    m_lblImportingTxt = new QLabel(m_importingPageWidget);
    m_lblImportingTxt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_lblImportingTxt->setWordWrap(true);

    m_lblImportingErrTxt = new QLabel(m_importingPageWidget);
    m_lblImportingErrTxt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_lblImportingErrTxt->setWordWrap(true);

    vbox->addWidget(m_lblImportingTxt);
    vbox->addWidget(m_lblImportingErrTxt);
    vbox->addStretch(1);

    QWidget *options_widget = new QWidget(m_importingPageWidget);
    vbox->addWidget(options_widget);
    QVBoxLayout *options_vbox = new QVBoxLayout(options_widget);
    options_vbox->setSpacing(KexiUtils::spacingHint());
    m_importOptionsButton = new QPushButton(koIcon("configure"), xi18n("Advanced Options"), options_widget);
    connect(m_importOptionsButton, SIGNAL(clicked()),this, SLOT(slotOptionsButtonClicked()));
    options_vbox->addWidget(m_importOptionsButton);
    options_vbox->addStretch(1);

    m_importingPageWidget->show();

    m_importingPageItem = new KPageWidgetItem(m_importingPageWidget, xi18n("Importing"));
    addPage(m_importingPageItem);
}

void ImportTableWizard::setupAlterTablePage()
{
    m_alterTablePageWidget = new QWidget(this);
    m_alterTablePageWidget->hide();

    QVBoxLayout *vbox = new QVBoxLayout(m_alterTablePageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    m_alterSchemaWidget = new KexiMigration::AlterSchemaWidget(this);
    vbox->addWidget(m_alterSchemaWidget);
    m_alterTablePageWidget->show();

    m_alterTablePageItem = new KPageWidgetItem(m_alterTablePageWidget, xi18n("Alter the Detected Table Design"));
    addPage(m_alterTablePageItem);
}

void ImportTableWizard::setupProgressPage()
{
    m_progressPageWidget = new QWidget(this);
    m_progressPageWidget->hide();
    QVBoxLayout *vbox = new QVBoxLayout(m_progressPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    m_progressPageWidget->setLayout(vbox);
    m_progressLbl = new QLabel(m_progressPageWidget);
    m_progressLbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_progressLbl->setWordWrap(true);
    m_rowsImportedLbl = new QLabel(m_progressPageWidget);

    m_importingProgressBar = new QProgressBar(m_progressPageWidget);
    m_importingProgressBar->setMinimum(0);
    m_importingProgressBar->setMaximum(0);
    m_importingProgressBar->setValue(0);

    vbox->addWidget(m_progressLbl);
    vbox->addWidget(m_rowsImportedLbl);
    vbox->addWidget(m_importingProgressBar);
    vbox->addStretch(1);

    m_progressPageItem = new KPageWidgetItem(m_progressPageWidget, xi18n("Processing Import"));
    addPage(m_progressPageItem);
}

void ImportTableWizard::setupFinishPage()
{
    m_finishPageWidget = new QWidget(this);
    m_finishPageWidget->hide();
    QVBoxLayout *vbox = new QVBoxLayout(m_finishPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    m_finishLbl = new QLabel(m_finishPageWidget);
    m_finishLbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_finishLbl->setWordWrap(true);

    vbox->addWidget(m_finishLbl);
    m_finishCheckBox = new QCheckBox(xi18n("Open imported table"),
                                     m_finishPageWidget);
    vbox->addSpacing(KexiUtils::spacingHint());
    vbox->addWidget(m_finishCheckBox);
    vbox->addStretch(1);

    m_finishPageItem = new KPageWidgetItem(m_finishPageWidget, xi18n("Success"));
    addPage(m_finishPageItem);
}

void ImportTableWizard::slot_currentPageChanged(KPageWidgetItem* curPage,KPageWidgetItem* prevPage)
{
    Q_UNUSED(prevPage);
    if (curPage == m_introPageItem) {
    }
    else if (curPage == m_srcConnPageItem) {
        arriveSrcConnPage();
    } else if (curPage == m_srcDBPageItem) {
        arriveSrcDBPage();
    } else if (curPage == m_tablesPageItem) {
        arriveTableSelectPage(prevPage);
    } else if (curPage == m_alterTablePageItem) {
        if (prevPage == m_tablesPageItem) {
            arriveAlterTablePage();
        }
    } else if (curPage == m_importingPageItem) {
        arriveImportingPage();
    } else if (curPage == m_progressPageItem) {
        arriveProgressPage();
    } else if (curPage == m_finishPageItem) {
        arriveFinishPage();
    }
}

void ImportTableWizard::arriveSrcConnPage()
{
    qDebug();
}

void ImportTableWizard::arriveSrcDBPage()
{
    if (fileBasedSrcSelected()) {
        //! @todo Back button doesn't work after selecting a file to import
    } else if (!m_srcDBName) {
        m_srcDBPageWidget->hide();
        qDebug() << "Looks like we need a project selector widget!";

        KDbConnectionData* conndata = m_srcConnSel->selectedConnectionData();
        if (conndata) {
            KexiGUIMessageHandler handler;
            m_prjSet = new KexiProjectSet(&handler);
            if (!m_prjSet->setConnectionData(conndata)) {
                handler.showErrorMessage(m_prjSet->result());
                delete m_prjSet;
                m_prjSet = 0;
                return;
            }
            QVBoxLayout *vbox = new QVBoxLayout(m_srcDBPageWidget);
            KexiUtils::setStandardMarginsAndSpacing(vbox);
            m_srcDBName = new KexiProjectSelectorWidget(m_srcDBPageWidget, m_prjSet);
            vbox->addWidget(m_srcDBName);
            m_srcDBName->label()->setText(xi18n("Select source database you wish to import:"));
        }
        m_srcDBPageWidget->show();
    }
}

void ImportTableWizard::arriveTableSelectPage(KPageWidgetItem *prevPage)
{
    if (prevPage == m_alterTablePageItem) {
        if (m_tableListWidget->count() == 1) { //we was skiping it before
            back();
        }
    } else {
        Kexi::ObjectStatus result;
        KexiUtils::WaitCursor wait;
        m_tableListWidget->clear();
        m_migrateDriver = prepareImport(result);

        if (m_migrateDriver) {
            if (!m_migrateDriver->connectSource()) {
                qWarning() << "unable to connect to database";
                return;
            }

            QStringList tableNames;
            if (m_migrateDriver->tableNames(tableNames)) {
                m_tableListWidget->addItems(tableNames);
            }
            if (m_tableListWidget->item(0)) {
                m_tableListWidget->item(0)->setSelected(true);
                if (m_tableListWidget->count() == 1) {
                    KexiUtils::removeWaitCursor();
                    next();
                }
            }
        } else {
            qWarning() << "No driver for selected source";
            QString errMessage =result.message.isEmpty() ? xi18n("Unknown error") : result.message;
            QString errDescription = result.description.isEmpty() ? errMessage : result.description;
            KMessageBox::error(this, errMessage, errDescription);
            setValid(m_tablesPageItem, false);
        }
        KexiUtils::removeWaitCursor();
    }
}

void ImportTableWizard::arriveAlterTablePage()
{
//! @todo handle errors
    if (m_tableListWidget->selectedItems().isEmpty())
        return;
//! @todo (js) support multiple tables?
#if 0
    foreach(QListWidgetItem *table, m_tableListWidget->selectedItems()) {
        m_importTableName = table->text();
    }
#else
    m_importTableName = m_tableListWidget->selectedItems().first()->text();
#endif

    KDbTableSchema *ts = new KDbTableSchema();
    if (!m_migrateDriver->readTableSchema(m_importTableName, *ts)) {
        delete ts;
        return;
    }

    qDebug() << ts->fieldCount();

    setValid(m_alterTablePageItem, ts->fieldCount() > 0);
    if (isValid(m_alterTablePageItem)) {
       connect(m_alterSchemaWidget->nameWidget(), SIGNAL(textChanged()), this, SLOT(slotNameChanged()), Qt::UniqueConnection);
    }

    QString baseName;
    if (fileBasedSrcSelected()) {
        baseName = QFileInfo(m_srcConnSel->selectedFileName()).baseName();
    } else {
        baseName = m_srcDBName->selectedProjectData()->databaseName();
    }

    QString suggestedCaption = xi18nc("<basename-filename> <tablename>", "%1 %2", baseName, m_importTableName);
    m_alterSchemaWidget->setTableSchema(ts, suggestedCaption);

    if (!m_migrateDriver->readFromTable(m_importTableName))
        return;

    if (!m_migrateDriver->moveFirst()) {
        back();
        KMessageBox::information(this,xi18n("No data has been found in table <resource>%1</resource>. Select different table or cancel importing.",
                                           m_importTableName));
    }
    QList<KDbRecordData> data;
    for (int i = 0; i < ROWS_FOR_PREVIEW; ++i) {
        KDbRecordData row;
        row.resize(ts->fieldCount());
        for (int j = 0; j < ts->fieldCount(); ++j) {
            row[j] = m_migrateDriver->value(j);
        }
        data.append(row);
        if (!m_migrateDriver->moveNext()) { // rowCount < 3 (3 is default)
            m_alterSchemaWidget->model()->setRowCount(i+1);
            break;
        }
    }
    m_alterSchemaWidget->setData(data);
}

void ImportTableWizard::arriveImportingPage()
{
    m_importingPageWidget->hide();
#if 0
    if (checkUserInput()) {
        //setNextEnabled(m_importingPageWidget, true);
        user2Button->setEnabled(true);
    } else {
        //setNextEnabled(m_importingPageWidget, false);
        user2Button->setEnabled(false);
    }
#endif

    QString txt;

    txt = xi18nc("@info Table import wizard, final message", "<para>All required information has now been gathered. "
                   "Click <interface>Next</interface> button to start importing table <resource>%1</resource>.</para>"
                   "<note>Depending on size of the table this may take some time.</note>", m_alterSchemaWidget->nameWidget()->nameText());

    m_lblImportingTxt->setText(txt);

    //temp. hack for MS Access driver only
    //! @todo for other databases we will need KexiMigration::Conenction
    //! and KexiMigration::Driver classes
    bool showOptions = false;
    if (fileBasedSrcSelected()) {
        Kexi::ObjectStatus result;
        KexiMigrate* sourceDriver = prepareImport(result);
        if (sourceDriver) {
            showOptions = !result.error()
            && sourceDriver->propertyValue("source_database_has_nonunicode_encoding").toBool();
            KexiMigration::Data *data = sourceDriver->data();
            sourceDriver->setData(0);
            delete data;
        }
    }
    if (showOptions)
        m_importOptionsButton->show();
    else
        m_importOptionsButton->hide();

    m_importingPageWidget->show();
}

void ImportTableWizard::arriveProgressPage()
{
    m_progressLbl->setText(xi18nc("@info", "Please wait while the table is imported."));

    //! @todo KEXI3 user1Button->setEnabled(false);
    //! @todo KEXI3 user2Button->setEnabled(false);
    //! @todo KEXI3 user3Button->setEnabled(false);

    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(slotCancelClicked()));

    QApplication::setOverrideCursor(Qt::BusyCursor);
    m_importComplete = doImport();
    QApplication::restoreOverrideCursor();

    disconnect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
               this, SLOT(slotCancelClicked()));

    next();
}

void ImportTableWizard::arriveFinishPage()
{
    if (m_importComplete) {
        m_finishLbl->setText(xi18n("Table <resource>%1</resource> has been imported.",
                                  m_alterSchemaWidget->nameWidget()->nameText()));
    } else {
        m_finishCheckBox->setEnabled(false);
        m_finishLbl->setText(xi18n("An error occured.",
                                  m_alterSchemaWidget->nameWidget()->nameText()));
    }

    //! @todo KEXI3 user2Button->setEnabled(false);
    //! @todo KEXI3 user3Button->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
}

bool ImportTableWizard::fileBasedSrcSelected() const
{
    return m_srcConnSel->selectedConnectionType() == KexiConnectionSelectorWidget::FileBased;
}

KexiMigrate* ImportTableWizard::prepareImport(Kexi::ObjectStatus& result)
{
    // Find a source (migration) driver name
    QString sourceDriverName;

    sourceDriverName = driverNameForSelectedSource();
    if (sourceDriverName.isEmpty())
        result.setStatus(xi18n("No appropriate migration driver found."),
                            m_migrateManager->possibleProblemsMessage());


    // Get a source (migration) driver
    KexiMigrate* sourceDriver = 0;
    if (!result.error()) {
        sourceDriver = m_migrateManager->driver(sourceDriverName);
        if (!sourceDriver || m_migrateManager->error()) {
            qDebug() << "Import migrate driver error...";
            result.setStatus(m_migrateManager);
        }
    }

    // Set up source (migration) data required for connection
    if (sourceDriver && !result.error()) {
        #if 0
        // Setup progress feedback for the GUI
        if (sourceDriver->progressSupported()) {
            m_progressBar->updateGeometry();
            disconnect(sourceDriver, SIGNAL(progressPercent(int)),
                       this, SLOT(progressUpdated(int)));
                       connect(sourceDriver, SIGNAL(progressPercent(int)),
                               this, SLOT(progressUpdated(int)));
                               progressUpdated(0);
        }
        #endif

        bool keepData = true;

        #if 0
        if (m_importTypeStructureAndDataCheckBox->isChecked()) {
            qDebug() << "Structure and data selected";
            keepData = true;
        } else if (m_importTypeStructureOnlyCheckBox->isChecked()) {
            qDebug() << "structure only selected";
            keepData = false;
        } else {
            qDebug() << "Neither radio button is selected (not possible?) presume keep data";
            keepData = true;
        }
        #endif

        KexiMigration::Data* md = new KexiMigration::Data();

        if (fileBasedSrcSelected()) {
            KDbConnectionData* conn_data = new KDbConnectionData();
            conn_data->setFileName(m_srcConnSel->selectedFileName());
            md->source = conn_data;
            md->sourceName.clear();
        } else {
            md->source = m_srcConnSel->selectedConnectionData();
            md->sourceName = m_srcDBName->selectedProjectData()->databaseName();

        }

        md->keepData = keepData;
        sourceDriver->setData(md);

        return sourceDriver;
    }
    return 0;
}

//===========================================================
//
QString ImportTableWizard::driverNameForSelectedSource()
{
    if (fileBasedSrcSelected()) {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(m_srcConnSel->selectedFileName());
        if (!mime.isValid()
            || mime.name() == "application/octet-stream"
            || mime.name() == "text/plain")
        {
            //try by URL:
            mime = db.mimeTypeForUrl(m_srcConnSel->selectedFileName());
        }
        return mime.isValid() ? m_migrateManager->driverForMimeType(mime.name()) : QString();
    }

    return m_srcConnSel->selectedConnectionData() ? m_srcConnSel->selectedConnectionData()->driverName : QString();
}

bool ImportTableWizard::doImport()
{
    KexiGUIMessageHandler msg;

    KexiProject* project = KexiMainWindowIface::global()->project();
    if (!project) {
        msg.showErrorMessage(xi18n("No project available."));
        return false;
    }

    KexiPart::Part *part = Kexi::partManager().partForPluginId("org.kexi-project.table");
    if (!part) {
        msg.showErrorMessage(&Kexi::partManager());
        return false;
    }

    KDbTableSchema* newSchema = m_alterSchemaWidget->newSchema();
    if (!newSchema) {
        msg.showErrorMessage(xi18n("No table was selected to import."));
        return false;
    }
    newSchema->setName(m_alterSchemaWidget->nameWidget()->nameText());
    newSchema->setCaption(m_alterSchemaWidget->nameWidget()->captionText());

    KexiPart::Item* partItemForSavedTable = project->createPartItem(part->info(), newSchema->name());
    if (!partItemForSavedTable) {
        msg.showErrorMessage(project);
        return false;
    }

    //Create the table
    if (!m_connection->createTable(newSchema, true)) {
        msg.showErrorMessage(xi18n("Unable to create table <resource>%1</resource>.",
                                  newSchema->name()));
        return false;
    }
    m_alterSchemaWidget->takeTableSchema(); //m_connection takes ownership of the KDbTableSchema object

    //Import the data
    QApplication::setOverrideCursor(Qt::BusyCursor);
    QList<QVariant> row;
    unsigned int fieldCount = newSchema->fieldCount();
    m_migrateDriver->moveFirst();
    KDbTransactionGuard tg(m_connection);
    if (m_connection->error()) {
        QApplication::restoreOverrideCursor();
        return false;
    }
    do  {
        for (unsigned int i = 0; i < fieldCount; ++i) {
            if (m_importWasCanceled) {
                return false;
            }
            if (i % 100 == 0) {
                QApplication::processEvents();
            }
            row.append(m_migrateDriver->value(i));
        }
        m_connection->insertRecord(*newSchema, row);
        row.clear();
    } while (m_migrateDriver->moveNext());
    if (!tg.commit()) {
        QApplication::restoreOverrideCursor();
        return false;
    }
    QApplication::restoreOverrideCursor();

    //Done so save part and update gui
    partItemForSavedTable->setIdentifier(newSchema->id());
    project->addStoredItem(part->info(), partItemForSavedTable);

    return true;
}

void ImportTableWizard::slotConnPageItemSelected(bool isSelected)
{
    setValid(m_srcConnPageItem, isSelected);
}

void ImportTableWizard::slotTableListWidgetSelectionChanged()
{
    setValid(m_tablesPageItem, !m_tableListWidget->selectedItems().isEmpty());
}

void ImportTableWizard::slotNameChanged()
{
    setValid(m_alterTablePageItem, !m_alterSchemaWidget->nameWidget()->captionText().isEmpty());
}

void ImportTableWizard::slotCancelClicked()
{
    m_importWasCanceled = true;
}
