/* This file is part of the KDE project
   Copyright (C) 2006-2012 Jarosław Staniek <staniek@kde.org>

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

#include "alter.h"
#include "alteredtableschema.h"
#include <db/utils.h>
#include <kexiutils/utils.h>
#include <QMap>
#include <stdlib.h>

namespace KexiDB
{
class AlterTableHandler::Private
{
public:
    Private() {}
    ~Private() {
        qDeleteAll(actions);
    }
    ActionList actions;
    QPointer<Connection> conn;
};
}

using namespace KexiDB;

//! a global instance used to when returning null is needed
AlterTableHandler::ChangeFieldPropertyAction nullChangeFieldPropertyAction(true);
AlterTableHandler::RemoveFieldAction nullRemoveFieldAction(true);
AlterTableHandler::InsertFieldAction nullInsertFieldAction(true);
AlterTableHandler::MoveFieldPositionAction nullMoveFieldPositionAction(true);

//--------------------------------------------------------

AlterTableHandler::ActionBase::ActionBase(bool null)
        : m_order(-1)
        , m_null(null)
{
}

AlterTableHandler::ActionBase::~ActionBase()
{
}

AlterTableHandler::ChangeFieldPropertyAction& AlterTableHandler::ActionBase::toChangeFieldPropertyAction()
{
    if (dynamic_cast<ChangeFieldPropertyAction*>(this))
        return *dynamic_cast<ChangeFieldPropertyAction*>(this);
    return nullChangeFieldPropertyAction;
}

AlterTableHandler::RemoveFieldAction& AlterTableHandler::ActionBase::toRemoveFieldAction()
{
    if (dynamic_cast<RemoveFieldAction*>(this))
        return *dynamic_cast<RemoveFieldAction*>(this);
    return nullRemoveFieldAction;
}

AlterTableHandler::InsertFieldAction& AlterTableHandler::ActionBase::toInsertFieldAction()
{
    if (dynamic_cast<InsertFieldAction*>(this))
        return *dynamic_cast<InsertFieldAction*>(this);
    return nullInsertFieldAction;
}

AlterTableHandler::MoveFieldPositionAction& AlterTableHandler::ActionBase::toMoveFieldPositionAction()
{
    if (dynamic_cast<MoveFieldPositionAction*>(this))
        return *dynamic_cast<MoveFieldPositionAction*>(this);
    return nullMoveFieldPositionAction;
}

//--------------------------------------------------------

AlterTableHandler::FieldActionBase::FieldActionBase(const QString& fieldName)
        : ActionBase()
        , m_fieldName(fieldName)
{
}

AlterTableHandler::FieldActionBase::FieldActionBase(bool)
        : ActionBase(true)
{
}

AlterTableHandler::FieldActionBase::~FieldActionBase()
{
}

//--------------------------------------------------------

//! @internal
struct KexiDB_AlterTableHandlerStatic {
    KexiDB_AlterTableHandlerStatic() {
#define I(name, type) \
    types.insert(QByteArray(name).toLower(), (int)AlterTableHandler::type)
#define I2(name, type1, type2) \
    flag = (int)AlterTableHandler::type1|(int)AlterTableHandler::type2; \
    if (flag & AlterTableHandler::PhysicalAlteringRequired) \
        flag |= AlterTableHandler::MainSchemaAlteringRequired; \
    types.insert(QByteArray(name).toLower(), flag)

        /* useful links:
          http://dev.mysql.com/doc/refman/5.0/en/create-table.html
        */
        // ExtendedSchemaAlteringRequired is here because when the field is renamed,
        // we need to do the same rename in extended table schema: <field name="...">
        int flag;
        I2("name", PhysicalAlteringRequired, MainSchemaAlteringRequired);
        I2("type", PhysicalAlteringRequired, DataConversionRequired);
        I("caption", MainSchemaAlteringRequired);
        I("description", MainSchemaAlteringRequired);
        I2("unsigned", PhysicalAlteringRequired, DataConversionRequired); // always?
        I2("maxLength", PhysicalAlteringRequired, DataConversionRequired); // always?
        I2("precision", PhysicalAlteringRequired, DataConversionRequired); // always?
        I("defaultWidth", ExtendedSchemaAlteringRequired);
        // defaultValue: depends on backend, for mysql it can only by a constant or now()...
        // -- should we look at Driver here?
#ifndef KEXI_SHOW_UNFINISHED
//! @todo reenable
        I("defaultValue", MainSchemaAlteringRequired);
#else
        I2("defaultValue", PhysicalAlteringRequired, MainSchemaAlteringRequired);
#endif
        I2("primaryKey", PhysicalAlteringRequired, DataConversionRequired);
        I2("unique", PhysicalAlteringRequired, DataConversionRequired); // we may want to add an Index here
        I2("notNull", PhysicalAlteringRequired, DataConversionRequired); // we may want to add an Index here
        // allowEmpty: only support it just at kexi level? maybe there is a backend that supports this?
        I2("allowEmpty", PhysicalAlteringRequired, MainSchemaAlteringRequired);
        I2("autoIncrement", PhysicalAlteringRequired, DataConversionRequired); // data conversion may be hard here
        I2("indexed", PhysicalAlteringRequired, DataConversionRequired); // we may want to add an Index here

        // easier cases follow...
        I("visibleDecimalPlaces", ExtendedSchemaAlteringRequired);

        // lookup-field-related properties...
        /*moved to KexiDB::isExtendedTableFieldProperty()
            I("boundColumn", ExtendedSchemaAlteringRequired);
            I("rowSource", ExtendedSchemaAlteringRequired);
            I("rowSourceType", ExtendedSchemaAlteringRequired);
            I("rowSourceValues", ExtendedSchemaAlteringRequired);
            I("visibleColumn", ExtendedSchemaAlteringRequired);
            I("columnWidths", ExtendedSchemaAlteringRequired);
            I("showColumnHeaders", ExtendedSchemaAlteringRequired);
            I("listRows", ExtendedSchemaAlteringRequired);
            I("limitToList", ExtendedSchemaAlteringRequired);
            I("displayWidget", ExtendedSchemaAlteringRequired);*/

        //more to come...
#undef I
#undef I2
    }

    QHash<QByteArray, int> types;
};

K_GLOBAL_STATIC(KexiDB_AlterTableHandlerStatic, KexiDB_alteringTypeForProperty)

//! @internal
int AlterTableHandler::alteringTypeForProperty(const QByteArray& propertyName)
{
    const int res = KexiDB_alteringTypeForProperty->types[propertyName.toLower()];
    if (res == 0) {
        if (KexiDB::isExtendedTableFieldProperty(propertyName))
            return (int)ExtendedSchemaAlteringRequired;
        KexiDBWarn <<
        QString("AlterTableHandler::alteringTypeForProperty(): property \"%1\" not found!")
        .arg(QString(propertyName));
    }
    return res;
}

//---

AlterTableHandler::ChangeFieldPropertyAction::ChangeFieldPropertyAction(
    const QString& fieldName, const QString& propertyName, const QVariant& newValue)
        : FieldActionBase(fieldName)
        , m_propertyName(propertyName)
        , m_newValue(newValue)
{
}

AlterTableHandler::ChangeFieldPropertyAction::ChangeFieldPropertyAction(bool)
        : FieldActionBase(true)
{
}

AlterTableHandler::ChangeFieldPropertyAction::~ChangeFieldPropertyAction()
{
}

int AlterTableHandler::ChangeFieldPropertyAction::alteringRequirements() const
{
    return alteringTypeForProperty(m_propertyName.toLatin1());
}

QString AlterTableHandler::ChangeFieldPropertyAction::debugString(const DebugOptions& debugOptions)
{
    QString s = QString("Set \"%1\" property for table field \"%2\" to \"%3\"")
                .arg(m_propertyName).arg(fieldName()).arg(m_newValue.toString());
    return s;
}

static void debugAction(AlterTableHandler::ActionBase *action, int nestingLevel,
                        bool simulate, const QString& prependString = QString(), QString * debugTarget = 0)
{
    Q_UNUSED(simulate);
    Q_UNUSED(nestingLevel);
    
    QString debugString;
    if (!debugTarget)
        debugString = prependString;
    if (action) {
        AlterTableHandler::ActionBase::DebugOptions debugOptions;
        debugOptions.showFieldDebug = debugTarget != 0;
        debugString += action->debugString(debugOptions);
    } else {
        if (!debugTarget)
            debugString += "[No action]"; //hmm
    }
    if (debugTarget) {
        if (!debugString.isEmpty())
            *debugTarget += debugString + '\n';
    } else {
        KexiDBDbg << debugString;
#ifdef KEXI_DEBUG_GUI
        if (simulate)
            KexiDB::alterTableActionDebugGUI(debugString, nestingLevel);
#endif
    }
}

static void debugActionDict(AlterTableHandler::ActionDict *dict, bool simulate)
{
    QString fieldName;
    AlterTableHandler::ActionDictConstIterator it(dict->constBegin());
    if (it != dict->constEnd() && dynamic_cast<AlterTableHandler::FieldActionBase*>(it.value())) {
        //retrieve field name from the 1st related action
        fieldName = dynamic_cast<AlterTableHandler::FieldActionBase*>(it.value())->fieldName();
    }
    else {
        fieldName = "??";
    }
    QString dbg = QString("Action dict for field \"%1\" (%2):")
                  .arg(fieldName).arg(dict->count());
    KexiDBDbg << dbg;
#ifdef KEXI_DEBUG_GUI
    if (simulate)
        KexiDB::alterTableActionDebugGUI(dbg, 1);
#endif
    for (;it != dict->constEnd(); ++it) {
        debugAction(it.value(), 2, simulate);
    }
}

static void debugFieldActions(const AlterTableHandler::ActionDictDict &fieldActions, bool simulate)
{
#ifdef KEXI_DEBUG_GUI
    if (simulate)
        KexiDB::alterTableActionDebugGUI("** Simplified Field Actions:");
#endif
    for (AlterTableHandler::ActionDictDictConstIterator it(fieldActions.constBegin()); it != fieldActions.constEnd(); ++it) {
        debugActionDict(it.value(), simulate);
    }
}

//--------------------------------------------------------

AlterTableHandler::RemoveFieldAction::RemoveFieldAction(const QString& fieldName)
        : FieldActionBase(fieldName)
{
}

AlterTableHandler::RemoveFieldAction::RemoveFieldAction(bool)
        : FieldActionBase(true)
{
}

AlterTableHandler::RemoveFieldAction::~RemoveFieldAction()
{
}

int AlterTableHandler::RemoveFieldAction::alteringRequirements() const
{
//! @todo sometimes add DataConversionRequired (e.g. when relationships require removing orphaned records) ?
    return PhysicalAlteringRequired;
    //! @todo
}

QString AlterTableHandler::RemoveFieldAction::debugString(const DebugOptions& debugOptions)
{
    QString s = QString("Remove table field \"%1\"").arg(fieldName());
    return s;
}

//--------------------------------------------------------

AlterTableHandler::InsertFieldAction::InsertFieldAction(int fieldIndex, KexiDB::Field *field)
        : FieldActionBase(field->name())
        , m_index(fieldIndex)
        , m_field(0)
{
    Q_ASSERT(field);
    setField(field);
}

AlterTableHandler::InsertFieldAction::InsertFieldAction(const InsertFieldAction& action)
        : FieldActionBase(action)
        , m_index(action.index())
{
    m_field = new KexiDB::Field(*action.field());
}

AlterTableHandler::InsertFieldAction::InsertFieldAction(bool)
        : FieldActionBase(true)
        , m_index(0)
        , m_field(0)
{
}

AlterTableHandler::InsertFieldAction::~InsertFieldAction()
{
    delete m_field;
}

void AlterTableHandler::InsertFieldAction::setField(KexiDB::Field* field)
{
    if (m_field)
        delete m_field;
    m_field = field;
    setFieldName(m_field ? m_field->name() : QString());
}

int AlterTableHandler::InsertFieldAction::alteringRequirements() const
{
//! @todo sometimes add DataConversionRequired (e.g. when relationships require removing orphaned records) ?
    return PhysicalAlteringRequired;
    //! @todo
}

QString AlterTableHandler::InsertFieldAction::debugString(const DebugOptions& debugOptions)
{
    QString s = QString("Insert table field \"%1\" at position %2")
                .arg(m_field->name()).arg(m_index);
    if (debugOptions.showFieldDebug)
        s.append(QString(" (%1)").arg(m_field->debugString()));
    return s;
}

//--------------------------------------------------------

AlterTableHandler::MoveFieldPositionAction::MoveFieldPositionAction(
    int fieldIndex, const QString& fieldName)
        : FieldActionBase(fieldName)
        , m_index(fieldIndex)
{
}

AlterTableHandler::MoveFieldPositionAction::MoveFieldPositionAction(bool)
        : FieldActionBase(true)
{
}

AlterTableHandler::MoveFieldPositionAction::~MoveFieldPositionAction()
{
}

int AlterTableHandler::MoveFieldPositionAction::alteringRequirements() const
{
    return MainSchemaAlteringRequired;
    //! @todo
}

QString AlterTableHandler::MoveFieldPositionAction::debugString(const DebugOptions& debugOptions)
{
    QString s = QString("Move table field \"%1\" to position %2")
                .arg(fieldName()).arg(m_index);
    return s;
}

//--------------------------------------------------------

AlterTableHandler::AlterTableHandler(Connection &conn)
        : Object()
        , d(new Private())
{
    d->conn = &conn;
}

AlterTableHandler::~AlterTableHandler()
{
    delete d;
}

void AlterTableHandler::addAction(ActionBase* action)
{
    d->actions.append(action);
}

AlterTableHandler& AlterTableHandler::operator<< (ActionBase* action)
{
    d->actions.append(action);
    return *this;
}

const AlterTableHandler::ActionList& AlterTableHandler::actions() const
{
    return d->actions;
}

void AlterTableHandler::removeAction(int index)
{
    d->actions.removeAt(index);
}

void AlterTableHandler::clear()
{
    d->actions.clear();
}

void AlterTableHandler::setActions(const ActionList& actions)
{
    qDeleteAll(d->actions);
    d->actions = actions;
}

void AlterTableHandler::debug()
{
    KexiDBDbg << "AlterTableHandler's actions:";
    foreach(ActionBase* action, d->actions) {
        action->debug();
    }
}

TableSchema* AlterTableHandler::execute(const QString& tableName, ExecutionArguments *args)
{
    args->result = false;
    if (!d->conn) {
//! @todo err msg?
        return 0;
    }
    if (d->conn->isReadOnly()) {
//! @todo err msg?
        return 0;
    }
    if (!d->conn->isDatabaseUsed()) {
//! @todo err msg?
        return 0;
    }
    TableSchema *oldTable = d->conn->tableSchema(tableName);
    if (!oldTable) {
//! @todo err msg?
        return 0;
    }

    if (!args->debugString)
        debug();

    // 1. Apply actions to copied table
    AlteredTableSchema alteredTable(oldTable);
    QString dbg;
    dbg = QString("** Input actions (%1):").arg(d->actions.count());
    KexiDBDbg << dbg;
#ifdef KEXI_DEBUG_GUI
    if (args->simulate)
        KexiDB::alterTableActionDebugGUI(dbg, 0);
#endif
    int i = 0;
    foreach(ActionBase* action, d->actions) {
        debugAction(action, 1, args->simulate, QString("%1: ").arg(i + 1), args->debugString);
        if (!alteredTable.applyAction(action)) {
            //! @todo err msg?
            return 0;
        }
        ++i;
    }

    // 2. Execute physical altering
    if (!alteredTable.execute(d->conn, args)) {
        return 0;
    }

#if 0
    // 2. Compute altering requirements
    args.requirements = alteredTable.alteringRequirements();
    dbg = QString("** Overall altering requirements: %1").arg(args.requirements);
    KexiDBDbg << dbg;
#ifdef KEXI_DEBUG_GUI
    if (args.simulate)
        KexiDB::alterTableActionDebugGUI(dbg, 0);
#endif
#endif

    if (args->onlyComputeRequirements) {
        args->result = true;
        return oldTable;
    }
    if (args->simulate) {//do not execute
        args->result = true;
        return oldTable;
    }
    args->result = true;
    TableSchema *newTable = alteredTable.detachNewTable();
    return newTable ? newTable : oldTable;
}
