/* This file is part of the KDE project
   Copyright (C) 2004 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2006 Jarosław Staniek <staniek@kde.org>

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

#include "pqxxmigrate.h"
#include <migration/keximigrate_p.h>

#ifndef _MSC_VER
#  define _MSC_VER 0 // avoid warning in server/c.h
#  include <postgres.h>
#  undef _MSC_VER
#else
#  include <postgres.h>
#endif

#include <KDbCursor>
#include <KDbUtils>
#include <KDbDriverManager>
#include <KDb>
#include <kexidb/drivers/pqxx/pqxxcursor.h> //for pgsqlCStrToVariant()

#include <QString>
#include <QDebug>
#include <QStringList>

//! @todo I maybe should not use stl?
#include <string>
#include <vector>

using namespace KexiMigration;

/*
This is the implementation for the pqxx specific import routines
Thi is currently pre alpha and in no way is it meant
to compile, let alone work.  This is meant as an example of
what the system might be and is a work in progress
*/

K_EXPORT_KEXIMIGRATE_DRIVER(PqxxMigrate, pqxx)

//==================================================================================

PqxxMigrate::PqxxMigrate(QObject *parent, const QVariantList& args)
        : KexiMigrate(parent, args)
{
    m_res = 0;
    m_trans = 0;
    m_conn = 0;
    m_rows = 0;
    m_row = 0;

    KDbDriverManager manager;
    setDriver(manager.driver("pqxx"));
}
//==================================================================================
//Destructor
PqxxMigrate::~PqxxMigrate()
{
    clearResultInfo();
}

//==================================================================================
//This is probably going to be quite complex...need to get the types for all columns
//any any other attributes required by kexi
//helped by reading the 'tables' test program
bool PqxxMigrate::drv_readTableSchema(
    const QString& originalName, KDbTableSchema& tableSchema)
{
    //Perform a query on the table to get some data
    //qDebug();
    tableSchema.setName(originalName);
    if (!query("select * from " + drv_escapeIdentifier(originalName) + " limit 1"))
        return false;
    //Loop round the fields
    for (int i = 0; i < m_res->columns(); i++) {
        QString fldName(m_res->column_name(i));
        KDbField::Type fldType = type(m_res->column_type(i), fldName);
        QString fldID(KDb::stringToIdentifier(fldName));
        const pqxx::oid toid = tableOid(originalName);
        if (toid == 0)
            return false;
        KDbField *f = new KDbField(fldID, fldType);
        f->setCaption(fldName);
        f->setPrimaryKey(primaryKey(toid, i));
        f->setUniqueKey(uniqueKey(toid, i));
        f->setAutoIncrement(autoInc(toid, i));//This should be safe for all field types
        tableSchema.addField(f);
        //qDebug() << "Added field [" << f->name() << "] type [" << f->typeName() << ']';
    }
    return true;
}

//==================================================================================
//get a list of tables and put into the supplied string list
bool PqxxMigrate::drv_tableNames(QStringList& tableNames)
{
    if (!query("SELECT relname FROM pg_class WHERE ((relkind = 'r') AND ((relname !~ '^pg_') AND (relname !~ '^pga_') AND (relname !~ '^sql_')))"))
        return false;

    for (pqxx::result::const_iterator c = m_res->begin(); c != m_res->end(); ++c) {
        // Copy the result into the return list
        tableNames << QString::fromLatin1(c[0].c_str());
    }
    return true;
}

//==================================================================================
//Convert a postgresql type to a kexi type
KDbField::Type PqxxMigrate::type(int t, const QString& fname)
{
    switch (t) {
    case UNKNOWNOID:
        return KDbField::InvalidType;
    case BOOLOID:
        return KDbField::Boolean;
    case INT2OID:
        return KDbField::ShortInteger;
    case INT4OID:
        return KDbField::Integer;
    case INT8OID:
        return KDbField::BigInteger;
    case FLOAT4OID:
        return KDbField::Float;
    case FLOAT8OID:
        return KDbField::Double;
    case NUMERICOID:
        return KDbField::Double;
    case DATEOID:
        return KDbField::Date;
    case TIMEOID:
        return KDbField::Time;
    case TIMESTAMPOID:
        return KDbField::DateTime;
    case BYTEAOID:
        return KDbField::BLOB;
    case BPCHAROID:
        return KDbField::Text;
    case VARCHAROID:
        return KDbField::Text;
    case TEXTOID:
        return KDbField::LongText;
    }

    //Ask the user what to do with this field
    return userType(fname);
}

//==================================================================================
//Connect to the db backend
bool PqxxMigrate::drv_connect()
{
    //qDebug() << "drv_connect: " << data()->sourceName;

    QString conninfo;
    QString socket;

    //Setup local/remote connection
    if (data()->source->hostName().isEmpty()) {
        if (data()->source->databaseName().isEmpty()) {
            socket = "/tmp/.s.PGSQL.5432";
        } else {
            socket = data()->source->databaseName();
        }
    } else {
        conninfo = "host='" + data()->source->hostName() + '\'';
    }

    //Build up the connection string
    if (data()->source->port == 0)
        data()->source->port = 5432;

    conninfo += QString::fromLatin1(" port='%1'").arg(data()->source->port());
    conninfo += QString::fromLatin1(" dbname='%1'").arg(data()->sourceName);

    if (!data()->source->userName().isEmpty())
        conninfo += QString::fromLatin1(" user='%1'").arg(data()->source->userName());

    if (!data()->source->password().isEmpty())
        conninfo += QString::fromLatin1(" password='%1'").arg(data()->source->password());

    try {
        m_conn = new pqxx::connection(conninfo.toLatin1().constData());
        return true;
    } catch (const std::exception &e) {
        qWarning() << "exception - " << e.what();
    } catch (...) {
        qWarning() << "exception(...)??";
    }
    return false;
}

//==================================================================================
//Connect to the db backend
bool PqxxMigrate::drv_disconnect()
{
    if (m_conn) {
        m_conn->disconnect();
        delete m_conn;
        m_conn = 0;
    }
    return true;
}
//==================================================================================
//Perform a query on the database and store result in m_res
bool PqxxMigrate::query(const QString& statement)
{
    //qDebug() << "query: " << statement.toLatin1();
    if (!m_conn)
        return false;

    // Clear the last result information...
    clearResultInfo();

    try {
        //Create a transaction
        m_trans = new pqxx::nontransaction(*m_conn, "pqxxmigrate::query");
        //Create a result opject through the transaction
        m_res = new pqxx::result(m_trans->exec(statement.toLatin1().constData()));
        //Commit the transaction
        m_trans->commit();
        //If all went well then return true, errors picked up by the catch block
        return true;
    } catch (const std::exception &e) {
        //If an error ocurred then put the error description into _dbError
        qWarning() << "exception - " << e.what();
        return false;
    } catch (...) {
        qWarning() << "exception(...)??";
    }
    return true;
}

//=========================================================================
//Clears the current result
void PqxxMigrate::clearResultInfo()
{
    delete m_res;
    m_res = 0;

    delete m_trans;
    m_trans = 0;
}

//=========================================================================
//Return the OID for a table
pqxx::oid PqxxMigrate::tableOid(const QString& table)
{
    QString statement;
    static QString otable;
    static pqxx::oid toid;

    pqxx::nontransaction* tran = 0;
    pqxx::result* tmpres = 0;

    //Some simple result caching
    if (table == otable) {
        qDebug() << "Returning table OID from cache...";
        return toid;
    } else {
        otable = table;
    }

    try {
        statement = "SELECT relfilenode FROM pg_class WHERE (relname = '" +
                    table +
                    "')";

        tran = new pqxx::nontransaction(*m_conn, "find_t_oid");
        tmpres = new pqxx::result(tran->exec(statement.toLatin1().constData()));

        tran->commit();
        if (tmpres->size() > 0) {
            //We have a key field for this table, lets check if its this column
            tmpres->at(0).at(0).to(toid);
        } else {
            toid = 0;
        }
    } catch (const std::exception &e) {
        qWarning() << "exception - " << e.what();
        qWarning() << "failed statement - " << statement;
        toid = 0;
    } catch (...) {
        qWarning() << "exception(...)??";
    }
    delete tmpres;
    tmpres = 0;

    delete tran;
    tran = 0;

    //qDebug() << "OID for table [" << table << "] is [" << toid << ']';
    return toid;
}

//=========================================================================
//Return whether or not the curent field is a primary key
//! @todo Add result caching for speed
bool PqxxMigrate::primaryKey(pqxx::oid table_uid, int col) const
{
    QString statement;
    bool pkey;
    int keyf;

    pqxx::nontransaction* tran = 0;
    pqxx::result* tmpres = 0;

    try {
        statement = QString::fromLatin1(
                        "SELECT indkey FROM pg_index WHERE ((indisprimary = true) AND (indrelid = %1))")
                    .arg(table_uid);

        tran = new pqxx::nontransaction(*m_conn, "find_pkey");
        tmpres = new pqxx::result(tran->exec(statement.toLatin1().constData()));

        tran->commit();
        if (tmpres->size() > 0) {
            //We have a key field for this table, lets check if its this column
            tmpres->at(0).at(0).to(keyf);
            if (keyf - 1 == col) {//-1 because pg counts from 1 and we count from 0
                pkey = true;
                qDebug() << "Field is pkey";
            } else {
                pkey = false;
                qDebug() << "Field is NOT pkey";
            }
        } else {
            pkey = false;
            qDebug() << "Field is NOT pkey";
        }
    } catch (const std::exception &e) {
        qWarning() << "exception - " << e.what();
        qWarning() << "failed statement - " << statement;
        pkey = false;
    }
    delete tmpres;
    tmpres = 0;

    delete tran;
    tran = 0;
    return pkey;
}

//=========================================================================
/*! Fetches single string at column \a columnNumber from result obtained
 by running \a sqlStatement.
 On success the result is stored in \a string and true is returned.
 \return cancelled if there are no records available. */
tristate PqxxMigrate::drv_queryStringListFromSQL(
    const QString& sqlStatement, int columnNumber, QStringList& stringList, int numRecords)
{
    std::string result;
    int i = 0;
    if (query(sqlStatement)) {
        for (pqxx::result::const_iterator it = m_res->begin();
                it != m_res->end() && (numRecords == -1 || i < numRecords); ++it, i++) {
            if (it.size() > 0 && it.size() > columnNumber) {
                it.at(columnNumber).to(result);
                stringList.append(QString::fromUtf8(result.c_str()));
            } else {
                clearResultInfo();
                return cancelled;
            }
        }
    } else
        return false;
    clearResultInfo();

    if (i < numRecords)
        return cancelled;

    return true;
}

tristate PqxxMigrate::drv_fetchRecordFromSQL(const QString& sqlStatement,
        KDbRecordData* data, bool *firstRecord)
{
    Q_ASSERT(data);
    Q_ASSERT(firstRecord);
    if (*firstRecord || !m_res) {
        if (m_res)
            clearResultInfo();
        if (!query(sqlStatement))
            return false;
        m_fetchRecordFromSQL_iter = m_res->begin();
        *firstRecord = false;
    } else
        ++m_fetchRecordFromSQL_iter;

    if (m_fetchRecordFromSQL_iter == m_res->end()) {
        clearResultInfo();
        return cancelled;
    }

    const int numFields = m_fetchRecordFromSQL_iter.size();
    data->resize(numFields);
    for (int i = 0; i < numFields; i++)
        (*data)[i] = KDb::pgsqlCStrToVariant(m_fetchRecordFromSQL_iter.at(i)); //!< @todo KEXI3
    return true;
}

//=========================================================================
/*! Copy PostgreSQL table to KexiDB database */
bool PqxxMigrate::drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                                KDbTableSchema* dstTable)
{
    std::vector<std::string> R;

    pqxx::work T(*m_conn, "PqxxMigrate::drv_copyTable");

    pqxx::tablereader stream(T, (srcTable.toLatin1().constData()));

    //Loop round each row, reading into a vector of strings
    const KDbQueryColumnInfo::Vector fieldsExpanded(dstTable->query()->fieldsExpanded());
    for (int n = 0; (stream >> R); ++n) {
        QList<QVariant> vals;
        std::vector<std::string>::const_iterator i, end(R.end());
        int index = 0;
        for (i = R.begin(); i != end; ++i, index++) {
            KDbField *field = fieldsExpanded.at(index)->field;
            const KDbField::Type type = field->type(); // cache: evaluating type of expressions can be expensive
            if (type == KDbField::BLOB || type == KDbField::LongText) {
                vals.append(KDb::pgsqlByteaToByteArray((*i).c_str(), (*i).size()));
            } else if (type == KDbField::Boolean) {
                vals.append(QString((*i).c_str()).toLower() == "t" ? QVariant(true) : QVariant(false));
            } else {
                vals.append(KDb::cstringToVariant((*i).c_str(),
                                                     type, (*i).size()));
            }
        }
        if (!destConn->insertRecord(*dstTable, vals))
            return false;
        updateProgress();
        R.clear();
    }
    return true;
}

//=========================================================================
//Return whether or not the curent field is a primary key
//! @todo Add result caching for speed
bool PqxxMigrate::uniqueKey(pqxx::oid table_uid, int col) const
{
    QString statement;
    bool ukey;
    int keyf;

    pqxx::nontransaction* tran = 0;
    pqxx::result* tmpres = 0;

    try {
        statement = QString::fromLatin1(
                        "SELECT indkey FROM pg_index WHERE ((indisunique = true) AND (indrelid = %1))")
                    .arg(table_uid);

        tran = new pqxx::nontransaction(*m_conn, "find_ukey");
        tmpres = new pqxx::result(tran->exec(statement.toLatin1().constData()));

        tran->commit();
        if (tmpres->size() > 0) {
            //We have a key field for this table, lets check if its this column
            tmpres->at(0).at(0).to(keyf);
            if (keyf - 1 == col) {//-1 because pg counts from 1 and we count from 0
                ukey = true;
                qDebug() << "Field is unique";
            } else {
                ukey = false;
                qDebug() << "Field is NOT unique";
            }
        } else {
            ukey = false;
            qDebug() << "Field is NOT unique";
        }
    } catch (const std::exception &e) {
        qWarning() << "exception - " << e.what();
        qWarning() << "failed statement - " << statement;
        ukey = false;
    }

    delete tmpres;
    tmpres = 0;

    delete tran;
    tran = 0;
    return ukey;
}

//==================================================================================
//! @todo Implement
bool PqxxMigrate::autoInc(pqxx::oid /*table_uid*/, int /*col*/) const
{
    return false;
}

//==================================================================================
//! @todo Implement
bool PqxxMigrate::notNull(pqxx::oid /*table_uid*/, int /*col*/) const
{
    return false;
}

//==================================================================================
//! @todo Implement
bool PqxxMigrate::notEmpty(pqxx::oid /*table_uid*/, int /*col*/) const
{
    return false;
}

bool PqxxMigrate::drv_readFromTable(const QString & tableName)
{
    //qDebug();
    bool ret;
    ret = false;

    try {
        ret = query(QString("SELECT * FROM %1").arg(m_conn->esc(tableName.toLocal8Bit()).c_str()));
        if (ret) {
            m_rows = m_res->size();
            //qDebug() << m_rows;
        }

    }
    catch (const std::exception &e) {
        qWarning();
    }

    return ret;
}

bool PqxxMigrate::drv_moveNext()
{
   if (!m_res)
       return false;

   if (m_row < m_rows - 1) {
        m_row ++;
        return true;
   }
   else
   {
        return false;
   }
}

bool PqxxMigrate::drv_movePrevious()
{
    if (!m_res)
        return false;

    if (m_row > 0) {
        m_row --;
        return true;
    }
    else
    {
        return false;
    }
}

bool PqxxMigrate::drv_moveFirst()
{
    if (!m_res)
        return false;

    m_row = 0;
    return true;
}

bool PqxxMigrate::drv_moveLast()
{
    if (!m_res)
        return false;

    m_row = m_rows - 1;
    return true;
}

QVariant PqxxMigrate::drv_value(int i)
{
    if (m_row < m_rows) {
        QString str = (*m_res)[m_row][i].c_str();
        return str;
    }
    return QVariant();
}

