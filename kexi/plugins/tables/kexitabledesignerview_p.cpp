/* This file is part of the KDE project
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kexitabledesignerview_p.h"
#include "kexitabledesignerview.h"
#include <kexi_global.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include <widget/dataviewcommon/kexidataawarepropertyset.h>
#include <widget/properties/KexiCustomPropertyFactory.h>
#include <widget/tableview/KexiTableScrollArea.h>
#include <kexiutils/utils.h>
#include <KexiWindow.h>
#include "kexitabledesignercommands.h"

#include <KPropertySet>

#include <KDbCursor>
#include <KDbTableSchema>
#include <KDbConnection>
#include <KDbUtils>
#include <KDbRecordEditBuffer>
#include <KDbError>
#include <KDb>

#include <KActionCollection>
#include <KLocalizedString>

#include <QDebug>

using namespace KexiTableDesignerCommands;

KexiTableDesignerViewPrivate::KexiTableDesignerViewPrivate(
    KexiTableDesignerView* aDesignerView)
        : designerView(aDesignerView)
        , sets(0)
        , uniqueIdCounter(0)
        , dontAskOnStoreData(false)
        , slotTogglePrimaryKeyCalled(false)
        , primaryKeyExists(false)
        , slotPropertyChanged_primaryKey_enabled(true)
        , slotPropertyChanged_subType_enabled(true)
        , addHistoryCommand_in_slotPropertyChanged_enabled(true)
        , addHistoryCommand_in_slotRecordUpdated_enabled(true)
        , addHistoryCommand_in_slotAboutToDeleteRecord_enabled(true)
        , addHistoryCommand_in_slotRecordInserted_enabled(true)
        , slotBeforeCellChanged_enabled(true)
        , tempStoreDataUsingRealAlterTable(false)
{
    historyActionCollection = new KActionCollection((QWidget*)0);
    history = new KUndo2Stack();
    historyActionCollection->addAction("edit_undo", history->createUndoAction(historyActionCollection, "edit_undo"));
    historyActionCollection->addAction("edit_redo", history->createRedoAction(historyActionCollection, "edit_redo"));

    internalPropertyNames
        << "subType" << "uid" << "newrecord" << "rowSource" << "rowSourceType"
        << "boundColumn" << "visibleColumn";
}

KexiTableDesignerViewPrivate::~KexiTableDesignerViewPrivate()
{
    delete sets;
    delete historyActionCollection;
    delete history;
}

int KexiTableDesignerViewPrivate::generateUniqueId()
{
    return ++uniqueIdCounter;
}

void KexiTableDesignerViewPrivate::setPropertyValueIfNeeded(
    const KPropertySet& set, const QByteArray& propertyName,
    const QVariant& newValue, const QVariant& oldValue, Command* commandGroup,
    bool forceAddCommand, bool rememberOldValue,
    QStringList* const slist, QStringList* const nlist)
{
    KProperty& property = set[propertyName];

    //remember because we'll change list data soon
    KPropertyListData *oldListData = property.listData() ?
            new KPropertyListData(*property.listData()) : 0;
    if (slist && nlist) {
        if (slist->isEmpty() || nlist->isEmpty()) {
            property.setListData(0);
        } else {
            property.setListData(*slist, *nlist);
        }
    }
    if (oldValue.type() == newValue.type()
            && (oldValue == newValue || (!oldValue.isValid() && !newValue.isValid()))
            && !forceAddCommand) {
        return;
    }

    const bool prev_addHistoryCommand_in_slotPropertyChanged_enabled
        = addHistoryCommand_in_slotPropertyChanged_enabled; //remember
    addHistoryCommand_in_slotPropertyChanged_enabled = false;
    if (property.value() != newValue)
        property.setValue(newValue, rememberOldValue);
    if (commandGroup) {
            new ChangeFieldPropertyCommand(commandGroup, designerView, set, propertyName, oldValue, newValue,
                                           oldListData, property.listData());
    }
    delete oldListData;
    addHistoryCommand_in_slotPropertyChanged_enabled
        = prev_addHistoryCommand_in_slotPropertyChanged_enabled; //restore
}

void KexiTableDesignerViewPrivate::setPropertyValueIfNeeded(
    const KPropertySet& set, const QByteArray& propertyName,
    const QVariant& newValue, Command* commandGroup,
    bool forceAddCommand, bool rememberOldValue,
    QStringList* const slist, QStringList* const nlist)
{
    KProperty& property = set[propertyName];
    QVariant oldValue(property.value());
    setPropertyValueIfNeeded(set, propertyName, newValue, property.value(),
                             commandGroup, forceAddCommand, rememberOldValue, slist, nlist);
}

void KexiTableDesignerViewPrivate::setVisibilityIfNeeded(const KPropertySet& set, KProperty* prop,
        bool visible, bool *changed, Command *commandGroup)
{
    Q_ASSERT(changed);
    if (prop->isVisible() != visible) {
        if (commandGroup) {
                new ChangePropertyVisibilityCommand(commandGroup, designerView, set, prop->name(), visible);
        }
        prop->setVisible(visible);
        *changed = true;
    }
}

bool KexiTableDesignerViewPrivate::updatePropertiesVisibility(KDbField::Type fieldType, KPropertySet &set,
        Command *commandGroup)
{
    bool changed = false;
    KProperty *prop;
    bool visible;

    prop = &set["subType"];
    qDebug() << "subType=" << prop->value().toInt()
        << " type=" << set["type"].value().toInt();

    //if there is no more than 1 subType name or it's a PK: hide the property
    visible =
        (prop->listData() && prop->listData()->keys.count() > 1 /*disabled || isObjectTypeGroup*/)
        && set["primaryKey"].value().toBool() == false;
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["objectType"];
    const bool isObjectTypeGroup
        = set["type"].value().toInt() == (int)KDbField::BLOB; // used only for BLOBs
    visible = isObjectTypeGroup;
    setVisibilityIfNeeded(set, prop,  visible, &changed, commandGroup);

    prop = &set["unsigned"];
    visible = KDbField::isNumericType(fieldType);
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["maxLength"];
    visible = (fieldType == KDbField::Text);
    if (prop->isVisible() != visible) {
//    prop->setVisible( visible );
        //update the length when it makes sense
        const int lengthToSet = visible ? KDbField::defaultMaxLength() : 0;
        setPropertyValueIfNeeded(set, "maxLength", lengthToSet,
                                 commandGroup, false, false /*!rememberOldValue*/);
//  if (lengthToSet != prop->value().toInt())
//   prop->setValue( lengthToSet, false );
//    changed = true;
    }
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);
#ifdef KEXI_SHOW_UNFINISHED
    prop = &set["precision"];
    visible = KDbField::isFPNumericType(fieldType);
    setVisibilityIfNeeded(set, prop, visible, changed, commandGroup);
#endif
    prop = &set["visibleDecimalPlaces"];
    visible = KDb::supportsVisibleDecimalPlacesProperty(fieldType);
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["unique"];
    visible = fieldType != KDbField::BLOB;
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["indexed"];
    visible = fieldType != KDbField::BLOB;
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["allowEmpty"];
    visible = KDbField::hasEmptyProperty(fieldType);
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

    prop = &set["autoIncrement"];
    visible = KDbField::isAutoIncrementAllowed(fieldType);
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);

//! @todo remove this when BLOB supports default value
#ifndef KEXI_SHOW_UNFINISHED
    prop = &set["defaultValue"];
    visible = !isObjectTypeGroup;
    setVisibilityIfNeeded(set, prop, visible, &changed, commandGroup);
#endif

    return changed;
}

QString KexiTableDesignerViewPrivate::messageForSavingChanges(bool *emptyTable, bool skipWarning)
{
    Q_ASSERT(emptyTable);
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    *emptyTable = true == conn->isEmpty(designerView->tempData()->table);
    return xi18n("Do you want to save the design now?")
           + ((*emptyTable || skipWarning) ? QString() :
              (QString("\n\n") + designerView->part()->i18nMessage(":additional message before saving design",
                      designerView->window()).toString()));
}

void KexiTableDesignerViewPrivate::updateIconForRecord(KDbRecordData *data, KPropertySet *set)
{
    QVariant icon;
    if (!set->property("rowSource").value().toString().isEmpty()
        && !set->property("rowSourceType").value().toString().isEmpty())
    {
        icon = "combo";
    }
    //show/hide icon in the table
    view->data()->clearRecordEditBuffer();
    view->data()->updateRecordEditBuffer(data, COLUMN_ID_ICON, icon);
    view->data()->saveRecordChanges(data, true);
}
