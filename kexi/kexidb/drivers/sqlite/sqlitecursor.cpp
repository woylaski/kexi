/* This file is part of the KDE project
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "sqlitecursor.h"

#include "sqliteconnection.h"
#include "sqliteconnection_p.h"


#include <kexidb/error.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>

#include <qptrvector.h>

using namespace KexiDB;

class KexiDB::SQLiteCursorData
{
	public:
		SQLiteCursorData()
			:
//			: curr_cols(0)
			res(SQLITE_OK)
			, curr_coldata(0)
			, curr_colname(0)
//			, rec_stored(false)
/* MOVED TO Cursor:
			, cols_pointers_mem_size(0)
			, records_in_buf(0)
			, buffering_completed(false)
			, at_buffer(false)*/
		{
		}

		QCString st;
		//for sqlite:
		sqlite *data; 
		sqlite_vm *vm;
		char *utail;
		char *errmsg;
		
		int res;

//		int curr_cols;
		const char **curr_coldata;
		const char **curr_colname;

		int next_cols;
//		const char **next_coldata;
//		const char **next_colname;
//		bool rec_stored : 1; //! true, current record is stored in next_coldata

/* MOVED TO Cursor:
		uint cols_pointers_mem_size; //! size of record's array of pointers to values
		int records_in_buf; //! number of records currently stored in the buffer
		bool buffering_completed; //! true if we have already all records stored in the buffer
		QPtrVector<const char*> records; //buffer data
		bool at_buffer; //! true if we already point to the buffer with curr_coldata
*/

/*		int prev_cols;
		const char **prev_coldata;
		const char **prev_colname;*/
};



SQLiteCursor::SQLiteCursor(Connection* conn, const QString& statement, uint options)
	: Cursor( conn, statement, options )
	, m_data( new SQLiteCursorData() )
{
	m_data->data = ((SQLiteConnection*)conn)->d->data;
}

SQLiteCursor::SQLiteCursor(Connection* conn, QuerySchema& query, uint options )
	: Cursor( conn, query, options )
	, m_data( new SQLiteCursorData() )
{
	m_data->data = ((SQLiteConnection*)conn)->d->data;
}

SQLiteCursor::~SQLiteCursor()
{
	close();
}

bool SQLiteCursor::drv_open(const QString& statement)
{
	m_data->st.resize(statement.length()*2);
	m_data->st = m_conn->escapeString( statement.local8Bit() );

	m_data->res = sqlite_compile(
		m_data->data,
		m_data->st.data(),
		(const char**)&m_data->utail,
		&m_data->vm,
		&m_data->errmsg );
	if (m_data->res!=SQLITE_OK) {
		return false;
	}
//cursor is automatically @ first record
//	m_beforeFirst = true;

	if (isBuffered()) {
		m_records.resize(128); //TODO: manage size dynamically
	}

	return true;
}

/*bool SQLiteCursor::drv_getFirstRecord()
{
	bool ok = drv_getNextRecord();*/
/*	if ((m_options & Buffered) && ok) { //1st record is there:
		//compute parameters for cursor's buffer:
		//-size of record's array of pointer to values
		m_data->cols_pointers_mem_size = m_data->curr_cols * sizeof(char*);
		m_data->records_in_buf = 1;
	}*/
	/*return ok;
}*/

bool SQLiteCursor::drv_close()
{
	m_data->res = sqlite_finalize( m_data->vm, &m_data->errmsg );
	if (m_data->res!=SQLITE_OK) {
		return false;
	}
	return true;
}

void SQLiteCursor::drv_getNextRecord()
{
	if ((m_data->res = sqlite_step(
	 m_data->vm,
	 &m_fieldCount,
	 &m_data->curr_coldata,
	 &m_data->curr_colname))==SQLITE_ROW)
		m_result = FetchOK;
	else if (m_data->res==SQLITE_DONE)
		m_result = FetchEnd;
	else
		m_result = FetchError;
			
	//debug
	if (m_result == FetchOK && m_data->curr_coldata) {
		for (int i=0;i<m_fieldCount;i++) {
			KexiDBDrvDbg<<"col."<< i<<": "<< m_data->curr_colname[i]<<" "<< m_data->curr_colname[m_fieldCount+i]
			<< " = " << (m_data->curr_coldata[i] ? QString::fromLocal8Bit(m_data->curr_coldata[i]) : "(NULL)") <<endl;
		}
	}
}

//store currently fetched record's values in appropriate place of the buffer
void SQLiteCursor::drv_appendCurrentRecordToBuffer()
{
	if (!m_cols_pointers_mem_size)
		m_cols_pointers_mem_size = m_fieldCount * sizeof(char*);
	const char **record = (const char**)malloc(m_cols_pointers_mem_size);
	const char **src_col = m_data->curr_coldata;
	const char **dest_col = record;
	for (int i=0; i<m_fieldCount; i++,src_col++,dest_col++) {
		*dest_col = strdup(*src_col);
	}
	m_records.insert(m_records_in_buf,record);
}

void SQLiteCursor::drv_bufferMovePointerNext()
{
	m_data->curr_coldata++; //move to next record in the buffer
}

void SQLiteCursor::drv_bufferMovePointerPrev()
{
	m_data->curr_coldata--; //move to prev record in the buffer
}

//compute a place in the buffer that contain next record's data
//and move internal buffer pointer to that place
void SQLiteCursor::drv_bufferMovePointerTo(Q_LLONG at)
{
	m_data->curr_coldata = m_records.at(at);
}


#if 0 //old impl.
bool SQLiteCursor::drv_getNextRecord()
{
	m_data->res = -1;

	if ((m_options & Buffered) && (m_at < (m_records_in_buf-1)) ) {
		//this cursor is buffered:
		//-we have next record already buffered:
		if (m_at_buffer) {//we already have got a pointer to buffer
			m_data->curr_coldata++; //just move to next record in the buffer
		} else {//we have no pointer
			//compute a place in the buffer that contain next record's data
			m_data->curr_coldata = m_records.at(m_at+1);
			m_at_buffer = true; //now current record is stored in the buffer
		}
	}
	else {//we are after last retrieved record: we need to physically fetch a record:
		if (!m_readAhead) {//we have no record that was read ahead
			if (!m_buffering_completed) {
				//for buffered cursor: only retrieve record 
				//if we are not at after last buffer's item when buffer is fully filled
				KexiDBDrvDbg<<"==== sqlite_step ===="<<endl;
				m_data->res = sqlite_step(
					m_data->vm,
//					&m_data->curr_cols,
					&m_fieldCount,
					&m_data->curr_coldata,
					&m_data->curr_colname);
			}
			if (m_data->res!=SQLITE_ROW) {//there is no record
				if (m_options & Buffered) {
					m_buffering_completed = true; //no more records to buffer
				}
				KexiDBDrvDbg<<"res!=SQLITE_ROW ********"<<endl;
				m_validRecord = false;
				m_afterLast = true;
				m_at = -1;
				if (m_data->res==SQLITE_DONE) {
					return false;
				}
				//SQLITE_ERROR:
				setError(ERR_CURSOR_RECORD_FETCHING, I18N_NOOP("Cannot fetch a record with a cursor"));
				return false;
			}
			
			//we have a record
			if (m_data->curr_coldata) {
				for (int i=0;i<m_fieldCount;i++) {
					KexiDBDrvDbg<<"col."<< i<<": "<< m_data->curr_colname[i]<<" "<< m_data->curr_colname[m_fieldCount+i]
					<< " = " << (m_data->curr_coldata[i] ? QString::fromLocal8Bit(m_data->curr_coldata[i]) : "(NULL)") <<endl;
				}
			}
			if (m_options & Buffered) {
				//store this record's values in the buffer
				if (!m_cols_pointers_mem_size)
					m_cols_pointers_mem_size = m_fieldCount * sizeof(char*);
//						m_data->cols_pointers_mem_size = m_data->curr_cols * sizeof(char*);
				const char **record = (const char**)malloc(m_cols_pointers_mem_size);
				const char **src_col = m_data->curr_coldata;
				const char **dest_col = record;
//					for (int i=0; i<m_data->curr_cols; i++,src_col++,dest_col++) {
				for (int i=0; i<m_fieldCount; i++,src_col++,dest_col++) {
					*dest_col = strdup(*src_col);
				}
				m_records.insert(m_records_in_buf++,record);
			}
		}
		else //we have a record that was read ahead: eat this
			m_readAhead = false;
	}

	m_at++;

	
//	if (m_data->curr_colname && m_data->curr_coldata)
//		for (int i=0;i<m_data->curr_cols;i++) {
//			KexiDBDrvDbg<<i<<": "<< m_data->curr_colname[i]<<" == "<< m_data->curr_coldata[i]<<endl;
//		}
	KexiDBDrvDbg<<"m_at == "<<(long)m_at<<endl;

	
	m_validRecord = true;
	return true;
}

bool SQLiteCursor::drv_getPrevRecord()
{
#ifndef Q_WS_WIN
#warning todo
#endif

		
	if (m_options & Buffered) {
		if ((m_at <= 0) || (m_records_in_buf <= 0)) {
			m_at=-1;
			m_beforeFirst = true;
			return false;
		}

		m_at--;
		if (m_at_buffer) {//we already have got a pointer to buffer
			m_data->curr_coldata--; //just move to prev record in the buffer
		} else {//we have no pointer
			//compute a place in the buffer that contain next record's data
			m_data->curr_coldata = m_records.at(m_at);
			m_at_buffer = true; //now current record is stored in the buffer
		}
		return true;
	}
//		setError( NOT_SUPPORTED,  )
	return false;
}
#endif

void SQLiteCursor::drv_clearBuffer()
{
	if (!m_cols_pointers_mem_size)
		return;
	const uint records_in_buf = m_records_in_buf;
	const char ***r_ptr = m_records.data();
	for (uint i=0; i<records_in_buf; i++, r_ptr++) {
//		const char **record = m_records.at(i);
		const char **field_data = *r_ptr;
//		for (int col=0; col<m_data->curr_cols; col++, field_data++) {
		for (int col=0; col<m_fieldCount; col++, field_data++) {
			free((void*)*field_data); //free field memory
		}
		free(*r_ptr); //free pointers to fields array
	}
//	m_data->curr_cols=0;
//	m_fieldCount=0;
	m_records_in_buf=0;
	m_cols_pointers_mem_size=0;
	m_at_buffer=false;
	m_records.clear();
}

/*
void SQLiteCursor::drv_storeCurrentRecord()
{
#if 0
	assert(!m_data->rec_stored);
	m_data->rec_stored = true;
	m_data->next_cols = m_data->curr_cols;
	for (int i=0;i<m_data->curr_cols;i++) {
		KexiDBDrvDbg<<"[COPY] "<<i<<": "<< m_data->curr_coldata[i]<<endl;
		if (m_data->curr_coldata[i])
			m_data->next_coldata[i] = strdup( m_data->curr_coldata[i] );
		else
			m_data->next_coldata[i] = 0;
	}
#endif
}
*/

/*TODO
const char *** SQLiteCursor::bufferData()
{
	if (!isBuffered())
		return 0;
	return m_records.data();
}*/

const char ** SQLiteCursor::recordData() const
{
	return m_data->curr_coldata;
}

void SQLiteCursor::storeCurrentRecord(RecordData &data) const
{
	if (!m_data)
		return;
	const char **col = m_data->curr_coldata;
	data.reserve(m_fieldCount);
    for( int i=0; i<m_fieldCount; i++, col++ ) {
		KexiDBDrvDbg << "SQLiteCursor::storeCurrentRecord(): col=" << (col ? *col : 0) << endl;
		data[i] = QVariant( *col );
	}
}

QVariant SQLiteCursor::value(int i) const
{
//	if (i > (m_data->curr_cols-1)) //range checking
	if (i > (m_fieldCount-1)) //range checking
		return QVariant();
//TODO: allow disable range checking! - performance reasons
	return QVariant( m_data->curr_coldata[i] );
}

//bool SQLiteCursor::moveLast()
//{
	//TODO
//	return true;
//}

/*bool SQLiteCursor::moveNext()
{
	//TODO
	return true;
}*/

/*
bool SQLiteCursor::eof()
{
	return m_afterLast;
}

int SQLiteCursor::at()
{
	if (m_readAhead)
		return 0;
	return m_at;
}

*/


int SQLiteCursor::serverResult()
{
	return m_data->res;
}

QString SQLiteCursor::serverResultName()
{
	return QString::fromLatin1( sqlite_error_string(m_data->res) );
}

void SQLiteCursor::drv_clearServerResult()
{
	m_data->res = SQLITE_OK;
}

QString SQLiteCursor::serverErrorMsg()
{
	return QString::fromLatin1( m_data->errmsg );
}
