/* This file is part of the KDE project
   Copyright (C) 2012 Jarosław Staniek <staniek@kde.org>

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

#include "alteredtableschema.h"
#include <kexiutils/utils.h>

#include <db/tableschema.h>
#include <db/transaction.h>
#include <db/utils.h>

#include <KDebug>

using namespace KexiDB;

class AlteredTableSchema::Private
{
public:
    Private(TableSchema* originalTable_)
     : originalTable(originalTable_)
    {
        originalTable->debug();
        newTable = new TableSchema(*originalTable, false /* do not copy ID */);
        newTable->debug();
    }
    ~Private() {
        delete newTable;
    }
    bool applyChangeFieldPropertyAction(AlterTableHandler::ChangeFieldPropertyAction *action);
    bool applyRemoveFieldAction(AlterTableHandler::RemoveFieldAction *action);
    bool applyInsertFieldAction(AlterTableHandler::InsertFieldAction *action);
    bool applyMoveFieldPositionAction(AlterTableHandler::MoveFieldPositionAction *action);

    /*! @return requrements computed, a combination of AlterTablehandler::AlteringRequirements values. */
    int alteringRequirements(bool *ok);

    /*! Copy properties from new table to original table. Only extended properties should be there. */
    bool copyPropertiesToOriginalTable();

    TableSchema *newTable;
    TableSchema* const originalTable;
    //! Original fields of originalTable pointed by fields of newTable.
    //! Needed to detect situation when field was removed and then added with the same name.
    QMap<Field*, Field*> originalFields;
};

bool AlteredTableSchema::Private::applyChangeFieldPropertyAction(
    AlterTableHandler::ChangeFieldPropertyAction *action)
{
    Field *field = newTable->field(action->fieldName());
    if (!field) {
        //! @todo errmsg
        return false;
    }
    return KexiDB::setFieldProperty(*field, action->propertyName().toLatin1(), action->newValue());
}

bool AlteredTableSchema::Private::applyRemoveFieldAction(AlterTableHandler::RemoveFieldAction *action)
{
    Field *field = newTable->field(action->fieldName());
    if (!field) {
        //! @todo errmsg
        return false;
    }
    return newTable->removeField(field);
}

bool AlteredTableSchema::Private::applyInsertFieldAction(AlterTableHandler::InsertFieldAction *action)
{
    newTable->insertField(action->index(), new Field(*action->field()));
    return true;
}

bool AlteredTableSchema::Private::applyMoveFieldPositionAction(AlterTableHandler::MoveFieldPositionAction *action)
{
    Field *field = newTable->field(action->fieldName());
    if (!field) {
        //! @todo errmsg
        return false;
    }
    return newTable->moveField(field, action->index());
}

int AlteredTableSchema::Private::alteringRequirements(bool *ok)
{
    Q_ASSERT(ok);
    if (!newTable) {
        return 0;
    }
    int r = 0;
    const Field::List *origFields = originalTable->fields();
    const Field::List *newFields = newTable->fields();
    // simple check: both tables should have the same field count
    if (origFields->count() != newFields->count()) {
        KexiDBDbg << "RETURNING PhysicalAlteringRequired & MainSchemaAlteringRequired (old/new tables have different field count)";
        return AlterTableHandler::PhysicalAlteringRequired
               | AlterTableHandler::MainSchemaAlteringRequired;
    }
    Field::ListIterator origIt = originalTable->fieldsIterator();
    Field::ListIterator it = newTable->fieldsIterator();
    for (; it != newTable->fieldsIteratorConstEnd(); ++origIt, ++it)
    {
        QMap<QByteArray, QVariant> origValues;
        QMap<QByteArray, QVariant> newValues;
        KexiDB::getFieldProperties(**origIt, &origValues);
        KexiDB::getFieldProperties(**it, &newValues);
        KexiDBDbg << "orig" << (*origIt)->name() << "properties:" << origValues;
        KexiDBDbg << "new" << (*it)->name() << "properties:" << origValues;
    }

    // new table should have original fields
    origIt = originalTable->fieldsIterator();
    it = newTable->fieldsIterator();
    int i = 0;
    for (; it != newTable->fieldsIteratorConstEnd(); ++origIt, ++it, ++i) {
        Field *origField = originalFields.value(*it);
        KexiDBDbg << "--- comparing for field" << (*it)->name()
                  << "(" << (i+1) << "of" << newTable->fieldCount() << ")";
        if (*origIt != origField) {
            KexiDBDbg << "RETURNING PhysicalAlteringRequired & MainSchemaAlteringRequired (orig field"
                      << (origField ? origField->name() : QString("NULL")) << "!= expected orig field"
                      << (*origIt)->name() << ")";
            return AlterTableHandler::PhysicalAlteringRequired
                   | AlterTableHandler::MainSchemaAlteringRequired;
        }
        QMap<QByteArray, QVariant> origValues;
        QMap<QByteArray, QVariant> newValues;
        KexiDB::getFieldProperties(**origIt, &origValues);
        KexiDB::getFieldProperties(**it, &newValues);
        QMap<QByteArray, QVariant>::ConstIterator origValuesIt = origValues.constBegin();
        QMap<QByteArray, QVariant>::ConstIterator newValuesIt = newValues.constBegin();
        if (origValues.count() != newValues.count()) {
            KexiDBWarn << "RETURNING ERROR (# of properties for new field"
                       << (*it)->name() << "(" << origValues.count()
                       << ") differs compared to original (" << newValues.count() << "))";
            *ok = false;
            return 0;
        }
        for (; origValuesIt != origValues.constEnd(); ++origValuesIt, ++newValuesIt) {
            //KexiDBDrvDbg << origValuesIt.key() << "?" << newValuesIt.key();
            if (origValuesIt.key() != newValuesIt.key()) {
                KexiDBWarn << "RETURNING ERROR (old property name="
                          << origValuesIt.key() << "-differs from new=" << newValuesIt.key() <<")";
                *ok = false;
                return 0;
            }
            if (origValuesIt.value() == newValuesIt.value()) {
//                KexiDBDbg << "unchanged property=" << origValuesIt.key()
//                          << origValuesIt.value();
            }
            else if (origValuesIt.value() != newValuesIt.value()) {
                int newReq = AlterTableHandler::alteringTypeForProperty(origValuesIt.key());
                KexiDBDbg << "CHANGED property=" << origValuesIt.key()
                          << "orig=" << origValuesIt.value() << "new=" << newValuesIt.value()
                          << "req=" << newReq;
                r |= newReq;
            }
        }

//        KexiDBDbg << "orig" << origField->name() << "properties:" << origValues;
//        KexiDBDbg << "new" << newField->name() << "properties:" << origValues;
//        for (QMap<QByteArray, QVariant>::ConstIterator valuesIt(origValues.constBegin());
//            valuesIt != origValues.constEnd(); ++val
        // simple check: both
    }
    *ok = true;

    return r;
}

bool AlteredTableSchema::Private::copyPropertiesToOriginalTable()
{
    if (!newTable) {
        return 0;
    }
    originalTable->debug();
    const Field::List *origFields = originalTable->fields();
    const Field::List *newFields = newTable->fields();
    if (origFields->count() != newFields->count()) {
        KexiDBWarn << "Could not copy properties when old/new tables have different field count";
        return false;
    }
    Field::ListIterator origIt = originalTable->fieldsIterator();
    Field::ListIterator it = newTable->fieldsIterator();
    for (; it != newTable->fieldsIteratorConstEnd(); ++origIt, ++it) {
        Field *origField = originalFields.value(*it);
        if (*origIt != origField) {
            KexiDBWarn << "orig field"
                       << (origField ? origField->name() : QString("NULL")) << "!= expected orig field"
                       << (*origIt)->name();
            return false;
        }
        QMap<QByteArray, QVariant> origValues;
        QMap<QByteArray, QVariant> newValues;
        KexiDB::getFieldProperties(**origIt, &origValues);
        KexiDB::getFieldProperties(**it, &newValues);
        QMap<QByteArray, QVariant>::ConstIterator origValuesIt = origValues.constBegin();
        QMap<QByteArray, QVariant>::ConstIterator newValuesIt = newValues.constBegin();
        if (origValues.count() != newValues.count()) {
            KexiDBWarn << "RETURNING ERROR (# of properties for new field"
                       << (*it)->name() << "(" << origValues.count()
                       << ") differs compared to original (" << newValues.count() << "))";
            return false;
        }
        QMap<QByteArray, QVariant> newValuesToSet;
        for (; origValuesIt != origValues.constEnd(); ++origValuesIt, ++newValuesIt) {
            //KexiDBDrvDbg << origValuesIt.key() << "?" << newValuesIt.key();
            if (origValuesIt.key() != newValuesIt.key()) {
                KexiDBWarn << "RETURNING ERROR (old property name="
                          << origValuesIt.key() << "-differs from new=" << newValuesIt.key() <<")";
                return false;
            }
            if (origValuesIt.value() != newValuesIt.value()) {
                KexiDBDbg << "SETTING property=" << origValuesIt.key()
                          << "from:" << origValuesIt.value() << "to:" << newValuesIt.value();
                newValuesToSet.insert(newValuesIt.key(), newValuesIt.value());
            }
        }
        if (!newValuesToSet.isEmpty() && !KexiDB::setFieldProperties(*origField, newValuesToSet)) {
            return false;
        }
    }
    originalTable->debug();
    return true;
}

// ----

AlteredTableSchema::AlteredTableSchema(TableSchema *ts)
: d(new Private(ts))
{
    d->newTable->debug();
    Field::ListIterator origIt = d->originalTable->fieldsIterator();
    Field::ListIterator it = d->newTable->fieldsIterator();
    for (; it != d->newTable->fieldsIteratorConstEnd(); ++origIt, ++it) {
        d->originalFields.insert(*it, *origIt);
    }
}

AlteredTableSchema::~AlteredTableSchema()
{
    delete d;
}

const TableSchema* AlteredTableSchema::originalTable() const
{
    return d->originalTable;
}

const Field* AlteredTableSchema::originalField(Field *field) const
{
    return d->originalFields.value(field);
}

#define CALL_APPLY_ACTION(actionClass) \
    AlterTableHandler::actionClass *actionClass ## _ \
        = dynamic_cast<AlterTableHandler::actionClass*>(action); \
    if (actionClass ## _) { \
        return d->apply ## actionClass(actionClass ## _); \
    }

bool AlteredTableSchema::applyAction(AlterTableHandler::ActionBase *action)
{
    if (!d->newTable) {
        KexiDBWarn << "New table detached.";
        return false;
    }
    CALL_APPLY_ACTION(ChangeFieldPropertyAction);
    CALL_APPLY_ACTION(RemoveFieldAction);
    CALL_APPLY_ACTION(InsertFieldAction);
    CALL_APPLY_ACTION(MoveFieldPositionAction);
    return false;
}

#undef CALL_APPLY_ACTION

TableSchema* AlteredTableSchema::detachNewTable()
{
    TableSchema *s = d->newTable;
    d->newTable = 0;
    return s;
}

bool AlteredTableSchema::execute(Connection *conn, AlterTableHandler::ExecutionArguments *args)
{
#ifdef KEXI_DEBUG_GUI
    KexiDB::alterTableActionDebugGUI("** Original table:", 0);
    KexiDB::alterTableActionDebugGUI(d->originalTable->debugString(true /*includeTableName*/), 1);
    KexiDB::alterTableActionDebugGUI("** New table:", 0);
    KexiDB::alterTableActionDebugGUI(d->newTable->debugString(true /*includeTableName*/), 1);
    KexiDB::alterTableActionDebugGUI("** New table's fields:", 0);
    QString dbg;
    int i = 0;
    for (Field::ListIterator it = d->newTable->fieldsIterator();
         it != d->newTable->fieldsIteratorConstEnd(); ++it, ++i)
    {
        const Field *origField = originalField(*it);
        if (origField) {
            dbg = QString("Field %1: \"%2\" field was \"%3\"").arg(i+1).arg((*it)->name()).arg(origField->name());
        }
        else {
            dbg = QString("Field %1: \"%2\" is a new field").arg(i+1).arg((*it)->name());
        }
        KexiDB::alterTableActionDebugGUI(dbg, 1);
    }
#endif
    KexiDB::TransactionGuard tg;
    if (args->withinTransaction) {
        tg.setTransaction(conn->beginTransaction());
        if (tg.transaction().isNull()) {
            return false;
        }
    }

    //TableSchema *table;
    bool ok;
    args->requirements = d->alteringRequirements(&ok);
    KexiDBDbg << "altering requirements:" << args->requirements << ok;
    if (!ok) {
        return false;
    }

    if (args->requirements == AlterTableHandler::ExtendedSchemaAlteringRequired) {
        if (!d->copyPropertiesToOriginalTable()) {
            return false;
        }

        if (!conn->storeExtendedTableSchemaData(*d->originalTable)) {
            return false;
        }
        delete d->newTable;
        d->newTable = 0;
    }

    const bool recreateTable = args->requirements & AlterTableHandler::PhysicalAlteringRequired;
    KexiDBDbg << "recreateTable:" << recreateTable;
    if (recreateTable) {
        // Create physical table
        QString newName = KexiDB::temporaryTableName(conn, d->newTable->name());
        d->newTable->setName(newName);

        if (!conn->createTable(d->newTable, false /* don't replace existing */)) {
            return false;
        }
        //table = d->newTable;
    }
    else {
        //table = d->originalTable;
    }

    // Update table metadata
    // - TODO
    // Copy data to the new table
    // - TODO
    if (recreateTable) {
        // Rename the new table to existing one
        const QString origTableName(d->originalTable->name());
    //    if (!conn->dropTable(d->originalTable->name())) {
    //        return false;
    //    }
        //! @todo backup the original table instead dropping it
        if (!conn->alterTableName(*d->newTable, origTableName, true /* replace */)) {
            return false;
        }
        d->originalFields.clear();
    }

    if (args->withinTransaction) {
        if (!tg.commit()) {
            return false;
        }
    }
    return true;
}

