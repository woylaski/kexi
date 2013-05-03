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

#ifndef KEXIDB_ALTEREDTABLESCHEMA_H
#define KEXIDB_ALTEREDTABLESCHEMA_H

#include "kexidb_export.h"
#include "alter.h"

namespace KexiDB
{

class TableSchema;

/*! KexiDB::AlteredTableSchema extends TableSchema to provide information about altered native
  database table. */
class KEXI_DB_EXPORT AlteredTableSchema
{
public:
    /*! Constructor deeply copying existing table. */
    explicit AlteredTableSchema(TableSchema *ts);

    virtual ~AlteredTableSchema();

    /*! @return original table referencing this table. */
    const TableSchema* originalTable() const;

    /*! @return field from original table referencing field @a field.
     This information is needed because original field cannot be always find by name
      - new table could have field with the same name recreated. */
    const Field* originalField(Field *field) const;

    /*! Apply table schema altering action @a action to this table.
     Does not create any physical database objects unless execute() is called.
     @return result of the applying. */
    bool applyAction(AlterTableHandler::ActionBase *action);

    /*! @return new altered table.
      Call this method only once because it will be detached and no longer owned
      by the AlteredTableSchema object. */
    TableSchema* detachNewTable();

    /*! Executes final altering including new physical table creation, all needed data copying
     and converting.
     @return true on success, false on failure. */
    bool execute(Connection *conn, AlterTableHandler::ExecutionArguments *args);

private:
    class Private;
    Private* const d;
};

} //namespace KexiDB

#endif
