/* This file is part of the KDE project
   Copyright 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
   Copyright 2002-2003 Norbert Andres <nandres@web.de>
   Copyright 2002 Ariya Hidayat <ariya@kde.org>
   Copyright 2002 Harri Porten <porten@kde.org>
   Copyright 2002 John Dailey <dailey@vt.edu>
   Copyright 1999-2002 Laurent Montel <montel@kde.org>
   Copyright 2001-2002 Philipp Mueller <philipp.mueller@gmx.de>
   Copyright 1998-2000 Torben Weis <weis@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

// Local
#include "NamedAreaDialog.h"

// Qt
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QHBoxLayout>
#include <QVBoxLayout>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klistwidget.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>
#include <kpushbutton.h>

#include <KoCanvasBase.h>

// KSpread
#include "Localization.h"
#include "Map.h"
#include "NamedAreaManager.h"
#include "ui/Selection.h"
#include "Sheet.h"
#include "Util.h"

#include "commands/NamedAreaCommand.h"

using namespace Calligra::Sheets;

NamedAreaDialog::NamedAreaDialog(QWidget* parent, Selection* selection)
        : KDialog(parent)
        , m_selection(selection)
{
    setButtons(KDialog::Ok | KDialog::Close);
    setButtonText(KDialog::Ok, i18n("&Select"));
    setCaption(i18n("Named Areas"));
    setModal(true);
    setObjectName(QLatin1String("NamedAreaDialog"));

    QWidget* widget = new QWidget(this);
    setMainWidget(widget);

    QHBoxLayout *hboxLayout = new QHBoxLayout(widget);
    hboxLayout->setMargin(0);

    QVBoxLayout *vboxLayout = new QVBoxLayout();

    m_list = new KListWidget(this);
    m_list->setSortingEnabled(true);
    vboxLayout->addWidget(m_list);

    m_rangeName = new QLabel(this);
    m_rangeName->setText(i18n("Area: %1", QString()));
    vboxLayout->addWidget(m_rangeName);

    hboxLayout->addLayout(vboxLayout);

    // list buttons
    QVBoxLayout *listButtonLayout = new QVBoxLayout();

    m_newButton = new KPushButton(i18n("&New..."), widget);
    listButtonLayout->addWidget(m_newButton);
    m_editButton = new KPushButton(i18n("&Edit..."), widget);
    listButtonLayout->addWidget(m_editButton);
    m_removeButton = new KPushButton(i18n("&Remove"), widget);
    listButtonLayout->addWidget(m_removeButton);
    listButtonLayout->addStretch(1);

    hboxLayout->addLayout(listButtonLayout);

    const QList<QString> namedAreas = m_selection->activeSheet()->map()->namedAreaManager()->areaNames();
    for (int i = 0; i < namedAreas.count(); ++i)
        m_list->addItem(namedAreas[i]);

    if (m_list->count() == 0) {
        enableButtonOk(false);
        m_removeButton->setEnabled(false);
        m_editButton->setEnabled(false);
        m_list->setCurrentRow(0);
    }

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(cancelClicked()), this, SLOT(slotClose()));
    connect(m_newButton, SIGNAL(clicked(bool)), this, SLOT(slotNew()));
    connect(m_editButton, SIGNAL(clicked(bool)), this, SLOT(slotEdit()));
    connect(m_removeButton, SIGNAL(clicked(bool)), this, SLOT(slotRemove()));
    connect(m_list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(slotOk()));
    connect(m_list, SIGNAL(currentTextChanged(QString)),
            this, SLOT(displayAreaValues(QString)));

    if (m_list->count() > 0)
        m_list->setCurrentItem(m_list->item(0));

    m_list->setFocus();
}

void NamedAreaDialog::displayAreaValues(QString const & areaName)
{
    const QString regionName = m_selection->activeSheet()->map()->namedAreaManager()->namedArea(areaName).name();
    m_rangeName->setText(i18n("Area: %1", regionName));
}

void NamedAreaDialog::slotOk()
{
    if (m_list->count() > 0) {
        QListWidgetItem* item = m_list->currentItem();
        Region region = m_selection->activeSheet()->map()->namedAreaManager()->namedArea(item->text());
        Sheet* sheet = m_selection->activeSheet()->map()->namedAreaManager()->sheet(item->text());
        if (!sheet || !region.isValid()) {
            return;
        }

        if (sheet && sheet != m_selection->activeSheet())
            m_selection->emitVisibleSheetRequested(sheet);
        m_selection->initialize(region);
    }

    m_selection->emitModified();
    accept();
}

void NamedAreaDialog::slotClose()
{
    reject();
}

void NamedAreaDialog::slotNew()
{
    QPointer<EditNamedAreaDialog> dialog = new EditNamedAreaDialog(this, m_selection);
    dialog->setCaption(i18n("New Named Area"));
    dialog->setRegion(*m_selection);
    dialog->exec();
    if (dialog->result() == Rejected)
        return;
    if (dialog->areaName().isEmpty())
        return;

    m_list->addItem(dialog->areaName());
    QList<QListWidgetItem*> items = m_list->findItems(dialog->areaName(),
                                    Qt::MatchExactly | Qt::MatchCaseSensitive);
    m_list->setCurrentItem(items.first());
    displayAreaValues(dialog->areaName());
    delete dialog;

    enableButtonOk(true);
    m_removeButton->setEnabled(true);
    m_editButton->setEnabled(true);
}

void NamedAreaDialog::slotEdit()
{
    QListWidgetItem* item = m_list->currentItem();
    if (item->text().isEmpty())
        return;

    QPointer<EditNamedAreaDialog> dialog = new EditNamedAreaDialog(this, m_selection);
    dialog->setCaption(i18n("Edit Named Area"));
    dialog->setAreaName(item->text());
    dialog->exec();
    if (dialog->result() == Rejected)
        return;

    item->setText(dialog->areaName());
    displayAreaValues(dialog->areaName());
    delete dialog;
}

void NamedAreaDialog::slotRemove()
{
    const QString question = i18n("Do you really want to remove this named area?");
    int result = KMessageBox::warningContinueCancel(this, question, i18n("Remove Named Area"),
                 KStandardGuiItem::del());
    if (result == KMessageBox::Cancel)
        return;

    QListWidgetItem* item = m_list->currentItem();

    NamedAreaCommand* command = new NamedAreaCommand();
    command->setAreaName(item->text());
    command->setReverse(true);
    command->setSheet(m_selection->activeSheet());
    if (!command->execute(m_selection->canvas())) {
        delete command;
        return;
    }
    m_list->takeItem(m_list->row(item));

    if (m_list->count() == 0) {
        enableButtonOk(false);
        m_removeButton->setEnabled(false);
        m_editButton->setEnabled(false);
        displayAreaValues(QString());
    } else
        displayAreaValues(m_list->currentItem()->text());
}



EditNamedAreaDialog::EditNamedAreaDialog(QWidget* parent, Selection* selection)
        : KDialog(parent)
        , m_selection(selection)
{
    setButtons(Ok | Cancel);
    setModal(true);
    setObjectName(QLatin1String("EditNamedAreaDialog"));
    enableButtonOk(false);

    QWidget *page = new QWidget();
    setMainWidget(page);

    QGridLayout * gridLayout = new QGridLayout(page);

    QLabel * textLabel4 = new QLabel(page);
    textLabel4->setText(i18n("Cells:"));
    gridLayout->addWidget(textLabel4, 2, 0);

    m_cellRange = new KLineEdit(page);
    gridLayout->addWidget(m_cellRange, 2, 1);

    QLabel * textLabel1 = new QLabel(page);
    textLabel1->setText(i18n("Sheet:"));
    gridLayout->addWidget(textLabel1, 1, 0);

    m_sheets = new KComboBox(page);
    gridLayout->addWidget(m_sheets, 1, 1);

    QLabel * textLabel2 = new QLabel(page);
    textLabel2->setText(i18n("Area name:"));
    gridLayout->addWidget(textLabel2, 0, 0);

    m_areaNameEdit = new KLineEdit(page);
    gridLayout->addWidget(m_areaNameEdit, 0, 1);

    const QList<Sheet*> sheetList = m_selection->activeSheet()->map()->sheetList();
    for (int i = 0; i < sheetList.count(); ++i) {
        Sheet* sheet = sheetList.at(i);
        if (!sheet)
            continue;
        m_sheets->insertItem(i, sheet->sheetName());
    }

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(m_areaNameEdit, SIGNAL(textChanged(QString)),
            this, SLOT(slotAreaNameModified(QString)));
}

EditNamedAreaDialog::~EditNamedAreaDialog()
{
}

QString EditNamedAreaDialog::areaName() const
{
    return m_areaNameEdit->text();
}

void EditNamedAreaDialog::setAreaName(const QString& name)
{
    m_initialAreaName = name;
    m_areaNameEdit->setText(name);
    Sheet* sheet = m_selection->activeSheet()->map()->namedAreaManager()->sheet(name);
    const QString tmpName = m_selection->activeSheet()->map()->namedAreaManager()->namedArea(name).name(sheet);
    m_cellRange->setText(tmpName);
}

void EditNamedAreaDialog::setRegion(const Region& region)
{
    Sheet* sheet = region.firstSheet();
    m_sheets->setCurrentIndex(m_sheets->findText(sheet->sheetName()));
    m_cellRange->setText(region.name(sheet));
}

void EditNamedAreaDialog::slotOk()
{
    if (m_areaNameEdit->text().isEmpty())
        return;
    Sheet* sheet = m_selection->activeSheet()->map()->sheet(m_sheets->currentIndex());
    Region region(m_cellRange->text(), m_selection->activeSheet()->map(), sheet);
    if (!region.isValid())
        return;

    KUndo2Command* macroCommand = 0;
    if (!m_initialAreaName.isEmpty() && m_initialAreaName != m_areaNameEdit->text()) {
        macroCommand = new KUndo2Command(kundo2_i18n("Replace Named Area"));
        // remove the old named area
        NamedAreaCommand* command = new NamedAreaCommand(macroCommand);
        command->setAreaName(m_initialAreaName);
        command->setReverse(true);
        command->setSheet(sheet);
        command->add(region);
    }

    // insert the new named area
    NamedAreaCommand* command = new NamedAreaCommand(macroCommand);
    command->setAreaName(m_areaNameEdit->text());
    command->setSheet(sheet);
    command->add(region);

    m_selection->canvas()->addCommand(macroCommand ? macroCommand : command);

    accept();
}

void EditNamedAreaDialog::slotAreaNameModified(const QString& name)
{
    enableButtonOk(!name.isEmpty());
}
