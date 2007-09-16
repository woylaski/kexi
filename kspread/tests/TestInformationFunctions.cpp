/* This file is part of the KDE project
   Copyright 2007 Brad Hards <bradh@frogmouth.net>
   Copyright 2007 Sascha Pfau <MrPeacock@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; only
   version 2 of the License.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <CellStorage.h>
#include <Doc.h>
#include <Formula.h>
#include <Map.h>
#include <Sheet.h>

#include "TestKspreadCommon.h"

#include "TestInformationFunctions.h"

// because we may need to promote expected value from integer to float
#define CHECK_EVAL(x,y) { Value z(y); QCOMPARE(evaluate(x,z),(z)); }

Value TestInformationFunctions::evaluate(const QString& formula, Value& ex)
{
  Formula f;
  QString expr = formula;
  if ( expr[0] != '=' )
    expr.prepend( '=' );
  f.setExpression( expr );
  Value result = f.eval();

  if(result.isFloat() && ex.isInteger())
    ex = Value(ex.asFloat());
  if(result.isInteger() && ex.isFloat())
    result = Value(result.asFloat());

  return result;
}

void TestInformationFunctions::initTestCase()
{
    m_doc = new Doc();
    m_doc->map()->addNewSheet();
    Sheet* sheet = m_doc->map()->sheet(0);
    CellStorage* storage = sheet->cellStorage();

    //
    // Test case data set
    //


//     // A19:A29
//     storage->setValue(1,19, Value(    1 ) );
//     storage->setValue(1,20, Value(    2 ) );
//     storage->setValue(1,21, Value(    4 ) );
//     storage->setValue(1,22, Value(    8 ) );
//     storage->setValue(1,23, Value(   16 ) );
//     storage->setValue(1,24, Value(   32 ) );
//     storage->setValue(1,25, Value(   64 ) );
//     storage->setValue(1,26, Value(  128 ) );
//     storage->setValue(1,27, Value(  256 ) );
//     storage->setValue(1,28, Value(  512 ) );
//     storage->setValue(1,29, Value( 1024 ) );
//     storage->setValue(1,30, Value( 2048 ) );
//     storage->setValue(1,31, Value( 4096 ) );
// 
// 
//     // B3:B17
    storage->setValue(2, 3, Value(     "7"   ) );
    storage->setValue(2, 4, Value(      2    ) );
//     storage->setValue(2, 5, Value(      3    ) );
//     storage->setValue(2, 6, Value(    true   ) );
//     storage->setValue(2, 7, Value(   "Hello" ) );
//     // B8 leave empty
//     storage->setValue(2, 9, Value::errorDIV0() );
//     storage->setValue(2,10, Value(      0    ) );
//     storage->setValue(2,11, Value(      3    ) );
//     storage->setValue(2,12, Value(      4    ) );
//     storage->setValue(2,13, Value( "2005-0131T01:00:00" ));
//     storage->setValue(2,14, Value(      1    ) );
//     storage->setValue(2,15, Value(      2    ) );
//     storage->setValue(2,16, Value(      3    ) );
//     storage->setValue(2,17, Value(      4    ) );
// 
// 
//     // C4:C6
    storage->setValue(3, 4, Value( 4 ) );
//     storage->setValue(3, 5, Value( 5 ) );
//     storage->setValue(3, 6, Value( 7 ) );
//     // C11:C17
//     storage->setValue(3,11, Value( 5 ) );
//     storage->setValue(3,12, Value( 6 ) );
//     storage->setValue(3,13, Value( 8 ) );
//     storage->setValue(3,14, Value( 4 ) );
//     storage->setValue(3,15, Value( 3 ) );
//     storage->setValue(3,16, Value( 2 ) );
//     storage->setValue(3,17, Value( 1 ) );
//     // C19:C31
//     storage->setValue(3,19, Value( 0 ) );
//     storage->setValue(3,20, Value( 5 ) );
//     storage->setValue(3,21, Value( 2 ) );
//     storage->setValue(3,22, Value( 5 ) );
//     storage->setValue(3,23, Value( 3 ) );
//     storage->setValue(3,24, Value( 4 ) );
//     storage->setValue(3,25, Value( 4 ) );
//     storage->setValue(3,26, Value( 0 ) );
//     storage->setValue(3,27, Value( 8 ) );
//     storage->setValue(3,28, Value( 1 ) );
//     storage->setValue(3,29, Value( 9 ) );
//     storage->setValue(3,30, Value( 6 ) );
//     storage->setValue(3,31, Value( 2 ) );
//     // C51:C57
//     storage->setValue(3,51, Value(  7 ) );
//     storage->setValue(3,52, Value(  9 ) );
//     storage->setValue(3,53, Value( 11 ) );
//     storage->setValue(3,54, Value( 12 ) );
//     storage->setValue(3,55, Value( 15 ) );
//     storage->setValue(3,56, Value( 17 ) );
//     storage->setValue(3,57, Value( 19 ) );
// 
//     // D51:D57
//     storage->setValue(4,51, Value( 100 ) );
//     storage->setValue(4,52, Value( 105 ) );
//     storage->setValue(4,53, Value( 104 ) );
//     storage->setValue(4,54, Value( 108 ) );
//     storage->setValue(4,55, Value( 111 ) );
//     storage->setValue(4,56, Value( 120 ) );
//     storage->setValue(4,57, Value( 133 ) );
// 
// 
//     // F51:F60
//     storage->setValue(6,51, Value( 3 ) );
//     storage->setValue(6,52, Value( 4 ) );
//     storage->setValue(6,53, Value( 5 ) );
//     storage->setValue(6,54, Value( 2 ) );
//     storage->setValue(6,55, Value( 3 ) );
//     storage->setValue(6,56, Value( 4 ) );
//     storage->setValue(6,57, Value( 5 ) );
//     storage->setValue(6,58, Value( 6 ) );
//     storage->setValue(6,59, Value( 4 ) );
//     storage->setValue(6,60, Value( 7 ) );
// 
// 
//     // G51:G60
//     storage->setValue(7,51, Value( 23 ) );
//     storage->setValue(7,52, Value( 24 ) );
//     storage->setValue(7,53, Value( 25 ) );
//     storage->setValue(7,54, Value( 22 ) );
//     storage->setValue(7,55, Value( 23 ) );
//     storage->setValue(7,56, Value( 24 ) );
//     storage->setValue(7,57, Value( 25 ) );
//     storage->setValue(7,58, Value( 26 ) );
//     storage->setValue(7,59, Value( 24 ) );
//     storage->setValue(7,60, Value( 27 ) );
}

//
// unittests
//

void TestInformationFunctions::testAREAS()
{
    CHECK_EVAL( "AREAS(B3)",          Value( 1 ) ); // A reference to a single cell has one area
    CHECK_EVAL( "AREAS(B3:C4)",       Value( 1 ) ); // A reference to a single range has one area
    CHECK_EVAL( "AREAS(B3:C4~D5:D6)", Value( 2 ) ); // Cell concatenation creates multiple areas
    CHECK_EVAL( "AREAS(B3:C4~B3)",    Value( 2 ) ); // Cell concatenation counts, even if the cells are duplicated
}

void TestInformationFunctions::testCELL()
{
    CHECK_EVAL( "CELL(\"COL\";B7)",            Value( 2              ) ); // Column B is column number 2.
    CHECK_EVAL( "CELL(\"ADDRESS\";B7)",        Value( "$B$7"         ) ); // Absolute address
    CHECK_EVAL( "CELL(\"ADDRESS\";Sheet2!B7)", Value( "$Sheet2.$B$7" ) ); // Absolute address including sheet name

    // Absolute address including sheet name and IRI of location of documentare duplicated
    CHECK_EVAL( "CELL(\"ADDRESS\";'x:\\sample.ods'#Sheet3!B7)", Value( "'file:///x:/sample.ods'#$Sheet3.$B$7" ) );

    // The current cell is saved in a file named ��sample.ods�� which is located at ��file:///x:/��
    CHECK_EVAL( "CELL(\"FILENAME\")",          Value( "file:///x:/sample.ods" ) );
 
    CHECK_EVAL( "CELL(\"FORMAT\";C7)",         Value( "D4" ) ); // C7's number format is like ��DD-MM-YYYY HH:MM:SS��
}

void TestInformationFunctions::testCOLUMN()
{
    CHECK_EVAL( "COLUMN(B7)",       Value( 2 ) ); // Column "B" is column number 2.
    CHECK_EVAL( "COLUMN()",         Value( 5 ) ); // Column of current cell is default, here formula in column E.
    CHECK_EVAL( "{=COLUMN(B2:D2)}", Value( 2 ) ); // Array with column numbers.
}

void TestInformationFunctions::testCOLUMNS()
{
    CHECK_EVAL( "COLUMNS(C1)",      Value( 1 ) ); // Single cell range contains one column.
    CHECK_EVAL( "COLUMNS(C1:C4)",   Value( 1 ) ); // Range with only one column.
    CHECK_EVAL( "COLUMNS(A4:D100)", Value( 4 ) ); // Number of columns in range.
}

void TestInformationFunctions::testERRORTYPE()
{
    CHECK_EVAL( "ERRORTYPE(0)",    Value::errorVALUE() ); // Non-errors produce an error.
    CHECK_EVAL( "ERRORTYPE(NA())", Value( 7          ) ); // By convention, the ERROR.TYPE of NA() is 7.
    CHECK_EVAL( "ERRORTYPE(1/0)",  Value( 2          ) );
}

void TestInformationFunctions::testISEVEN()
{
    CHECK_EVAL( "ISEVEN( 2)",   Value( true    ) ); // 2 is even, because (2 modulo 2) = 0
    CHECK_EVAL( "ISEVEN( 6)",   Value( true    ) ); // 6 is even, because (6 modulo 2) = 0
    CHECK_EVAL( "ISEVEN( 2.1)", Value( true    ) ); //
    CHECK_EVAL( "ISEVEN( 2.5)", Value( true    ) ); //
    CHECK_EVAL( "ISEVEN( 2.9)", Value( true    ) ); // TRUNC(2.9)=2, and 2 is even.
    CHECK_EVAL( "ISEVEN( 3)",   Value( false   ) ); // 3 is not even.
    CHECK_EVAL( "ISEVEN( 3.9)", Value( false   ) ); // TRUNC(3.9)=3, and 3 is not even.
    CHECK_EVAL( "ISEVEN(-2)",   Value( true    ) ); //
    CHECK_EVAL( "ISEVEN(-2.1)", Value( true    ) ); //
    CHECK_EVAL( "ISEVEN(-2.5)", Value( true    ) ); //
    CHECK_EVAL( "ISEVEN(-2.9)", Value( true    ) ); // TRUNC(-2.9)=-2, and -2 is even.
    CHECK_EVAL( "ISEVEN(-3)",   Value( false   ) ); //
    CHECK_EVAL( "ISEVEN(NA())", Value::errorNA() ); //
    CHECK_EVAL( "ISEVEN( 0)",   Value( true    ) ); //
}

void TestInformationFunctions::testVALUE()
{
    CHECK_EVAL( "VALUE(\"6\")", Value( 6 ) );
    CHECK_EVAL( "VALUE(\"1E5\")", Value( 100000 ) );
    CHECK_EVAL( "VALUE(\"200%\")",  Value( 2 ) );
    CHECK_EVAL( "VALUE(\"1.5\")", Value( 1.5 ) );
    // Check fractions
    CHECK_EVAL( "VALUE(\"7 1/4\")", Value( 7.25 ) );
    CHECK_EVAL( "VALUE(\"0 1/2\")", Value( 0.5 ) );
    CHECK_EVAL( "VALUE(\"0 7/2\")", Value( 3.5 ) );
    CHECK_EVAL( "VALUE(\"-7 1/5\")", Value( -7.2 ) );
    CHECK_EVAL( "VALUE(\"-7 10/50\")", Value( -7.2 ) );
    CHECK_EVAL( "VALUE(\"-7 10/500\")", Value( -7.02 ) );
    CHECK_EVAL( "VALUE(\"-7 4/2\")", Value( -9 ) );
    CHECK_EVAL( "VALUE(\"-7 40/20\")", Value( -9 ) );
    // Check times
    CHECK_EVAL( "VALUE(\"00:00\")", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"00:00:00\")", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"02:00\")-2/24", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"02:00:00\")-2/24", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"02:00:00.0\")-2/24", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"02:00:00.00\")-2/24", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"02:00:00.000\")-2/24", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"2:03:05\") -2/24-3/(24*60) -5/(24*60*60)", Value( 0 ) );
    CHECK_EVAL( "VALUE(\"2:03\")-(2/24)-(3/(24*60))", Value( 0 ) );
    // check dates - local dependent
    CHECK_EVAL( "VALUE(\"5/21/06\")=DATE(2006;5;21)", Value( true ) );
    CHECK_EVAL( "VALUE(\"1/2/2005\")=DATE(2005;1;2)", Value( true ) );
}

//
// cleanup test
//

void TestInformationFunctions::cleanupTestCase()
{
    delete m_doc;
}

QTEST_KDEMAIN(TestInformationFunctions, GUI)

#include "TestInformationFunctions.moc"
