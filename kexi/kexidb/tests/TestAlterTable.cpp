/* This file is part of the KDE project
   Copyright (C) 2012-2013 Jarosław Staniek <staniek@kde.org>

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

#include "TestAlterTable.h"

#include <db/driver.h>
#include <db/connection.h>
#include <db/utils.h>
#include <db/tableschema.h>
#include <db/cursor.h>
#include <kexidb/alter.h>

#include <KDebug>

#include <QFile>
#include <QTest>

#define FILES_DATA_DIR KDESRCDIR "data/"

#define JOIN2(a, b) a#b
#define JOIN(a, b) JOIN2( a, b) // indirection needed because only function-like macro parameters can be pasted

#define STRINGIFY(s) JOIN("", s)

using namespace KexiDB;

void TestAlterTable::initTestCase()
{
    m_conn = 0;
}

//! Imports SQL file into SQLite binary
static bool importSqliteFile(const QString &baseName)
{
    QString fname(QFile::decodeName(FILES_DATA_DIR + baseName.toLatin1() + ".kexi.sql"));
    QString outfname(QFile::decodeName(FILES_OUTPUT_DIR + baseName.toLatin1() + ".kexi"));
    bool ok = importSqliteFile(fname, outfname);
    if (!ok) {
        qWarning() << QString::fromLatin1("Could not import .sql file \"%1\"").arg(fname).toLatin1();
        return false;
    }
    return true;
}

//! Imports SQL file into SQLite binary and opens it
static Connection* openSqliteFile(const QString &baseName)
{
    if (!importSqliteFile(baseName)) {
        return 0;
    }
    QString db_name(QFile::decodeName(FILES_OUTPUT_DIR + baseName.toLatin1() + ".kexi"));
    QString drv_name = "sqlite3";

    DriverManager manager;
    if (manager.error()) {
        kWarning() << "Error in driver manager";
        return 0;
    }

    //get driver
    const Driver::Info drv_info = manager.driverInfo(drv_name);
    Driver *driver = manager.driver(drv_name);
    if (manager.error() || !driver) {
        kWarning() << "Error in driver manager after DriverManager::driver() call:" << manager.errorMsg();
        return 0;
    }
    if (drv_info.name.isEmpty() || drv_info.name != drv_name) {
        kWarning() << "Driver info is invalid";
        return 0;
    }

    //open connection
    ConnectionData cdata;
    cdata.setFileName(db_name);
    Connection *conn = driver->createConnection(cdata);
    if (driver->error() || !conn) {
        kWarning() << "Failed to create connection:" << driver->errorMsg();
        delete conn;
        return 0;
    }
    if (!conn->connect()) {
        kWarning() << "Failed to connect";
        delete conn;
        return 0;
    }
    if (!conn->useDatabase(db_name)) {
        kWarning() << "Failed to use db";
        conn->disconnect();
        delete conn;
        return 0;
    }
    return conn;
}

static bool closeSqliteFile(Connection *conn)
{
    if (!conn) {
        return false;
    }
    if (!conn->disconnect()) {
        kWarning() << "Failed to disconnect database";
        delete conn;
        return false;
    }
    delete conn;
    return true;
}

static QList<KexiDB::RecordData*> fetchRecords(Cursor *c, bool *ok)
{
    QList<KexiDB::RecordData*> records;
    for (int i = 0; !c->eof(); ++i) {
        KexiDB::RecordData *record = c->storeCurrentRow();
        if (!record) {
            kWarning() << "Could not rerieve record from table";
            qDeleteAll(records);
            *ok = false;
            return QList<KexiDB::RecordData*>();
        }
        records.append(record);
        kDebug() << "record" << i << *record;
        if (c->moveNext() && c->error()) {
            kWarning() << "Could not move to next record";
            qDeleteAll(records);
            *ok = false;
            return QList<KexiDB::RecordData*>();
        }
    }
    *ok = true;
    return records;
}

static QList<KexiDB::RecordData*> fetchRecords(Connection* conn, TableSchema* ts, bool *ok)
{
    Cursor *c = conn->executeQuery(*ts);
    if (!c) {
        kWarning() << "Could not open" << ts->name() << "cars table data";
        return QList<KexiDB::RecordData*>();
    }
    QList<KexiDB::RecordData*> records(fetchRecords(c, ok));
    conn->deleteCursor(c);
    return records;
}

static bool compareData(const QList<KexiDB::RecordData*> &records,
                        const QString &expectedDbName,
                        const QString &expectedTableName)
{
    Connection *conn = openSqliteFile(expectedDbName);
    if (!conn) {
        return false;
    }
    bool ok;
    QList<KexiDB::RecordData*> expectedRecords(fetchRecords(conn, conn->tableSchema(expectedTableName), &ok));
    if (records.count() != expectedRecords.count()) {
        kWarning() << "Found" << records.count() << "record(s) in table" << expectedTableName
                   << "expected" << expectedRecords.count();
        return false;
    }

    if (!closeSqliteFile(conn)) {
        return false;
    }
    return true;
}

//! Finds table @a tableName and allocates pointer named t_<tableName> and initializes it.
//! Verifies the result, on failure sets friendly message.
#define GET_TABLE(tableName) \
    TableSchema *t_##tableName = m_conn->tableSchema(STRINGIFY(tableName)); \
    QVERIFY2(t_##tableName, "table \"" STRINGIFY(tableName) "\" not found")

//! Finds field @a fieldName for table @a tableName and allocates pointer named
//! f_<tableName>_<fieldName> and initializes it.
//! Verifies the result, on failure sets friendly message.
#define GET_FIELD(tableName, fieldName) \
    Field *f_##tableName##_##fieldName; \
    { \
        GET_TABLE(tableName); \
        f_##tableName##_##fieldName = t_##tableName->field(STRINGIFY(fieldName)); \
        QVERIFY2(f_##tableName##_##fieldName, "field \"" STRINGIFY(tableName) "." STRINGIFY(fieldName) "\" not found"); \
    }

void TestAlterTable::testAddLookupToField()
{
    m_conn = openSqliteFile("cars_persons_nocombo");
    QVERIFY(m_conn);

    AlterTableHandler::ActionList actions;
    GET_TABLE(cars);
    GET_TABLE(persons);
    GET_FIELD(persons, id);
    GET_FIELD(persons, name);

    // retrieve orig records
    bool ok;
    QList<KexiDB::RecordData*> origRecords(fetchRecords(m_conn, t_cars, &ok));
    QVERIFY2(ok, "Retrieve orig records for cars");
    // e.g. record 0 QVector(QVariant(int, 1) ,  QVariant(int, 1) ,  QVariant(QString, "Fiat") ,  QVariant(qlonglong, 1) )

    // alter table
    actions
    << new AlterTableHandler::ChangeFieldPropertyAction(
        "owner", "rowSourceType", "table")
    << new AlterTableHandler::ChangeFieldPropertyAction(
        "owner", "rowSource", "persons")
    << new AlterTableHandler::ChangeFieldPropertyAction(
        "owner", "boundColumn", t_persons->indexOf(f_persons_id))
    << new AlterTableHandler::ChangeFieldPropertyAction(
    "owner", "visibleColumn", t_persons->indexOf(f_persons_name));

    AlterTableHandler::ExecutionArguments args;
    AlterTableHandler handler(*m_conn);
    handler.setActions(actions);
    TableSchema *new_cars = handler.execute(t_cars->name(), &args);
    if (true != args.result) {
        handler.debugError();
    }
    QVERIFY(true == args.result);
    QVERIFY(new_cars);
    QCOMPARE(new_cars, t_cars);
    QCOMPARE(args.requirements, int(KexiDB::AlterTableHandler::ExtendedSchemaAlteringRequired));

    // retrieve new records
    QList<KexiDB::RecordData*> records(fetchRecords(m_conn, new_cars, &ok));
    QVERIFY2(ok, "Retrieve new records for cars");

    // compare
    QVERIFY2(compareData(records, "cars_persons-expected", "cars"), "Unexpected contents of table cars");

    qDeleteAll(origRecords);
    origRecords.clear();
    qDeleteAll(records);
    records.clear();

    QVERIFY(closeSqliteFile(m_conn));
}

void TestAlterTable::cleanupTestCase()
{
}

QTEST_MAIN(TestAlterTable)
