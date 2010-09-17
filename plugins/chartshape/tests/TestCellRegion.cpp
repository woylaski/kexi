/* This file is part of the KDE project

   Copyright (C) 2010 Johannes Simon <johannes.simon@gmail.com>
   Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
     Contact: Suresh Chande suresh.chande@nokia.com

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

// Own
#include "TestCellRegion.h"

// Qt
#include <QtTest>

// KChart
#include "CellRegion.h"

TestCellRegion::TestCellRegion()
    : QObject( 0 )
{
}

void TestCellRegion::init()
{
    m_source.clear();
    m_source.add( "Table1", &m_model1 );
    m_source.add( "Table2", &m_model2 );
    Table *t1 = m_source.get( "Table1" );
    Table *t2 = m_source.get( "Table2" );

    m_region1 = CellRegion( t1 );
    m_region1.add( CellRegion( t1, QRect( 2, 3, 10, 11 ) ) );

    m_region2 = CellRegion();
    m_region2.add( CellRegion( t1, QRect( 2, 3, 10, 11 ) ) );
    m_region2.add( CellRegion( t2, QRect( 1, 2, 5, 6 ) ) );
}

void TestCellRegion::testToStringSingleTable()
{
    QCOMPARE( m_region1.toString(), QString( "$Table1.$B$3:$K$13" ) );
}

void TestCellRegion::testFromStringSingleTable()
{
    QCOMPARE( m_region1, CellRegion( &m_source, "$Table1.$B$3:$K$13" ) );
}

void TestCellRegion::testTableNameChangeSingleTable()
{
    m_source.rename( "Table1", "DoubleBubbleGumBubblesDouble" );
    QCOMPARE( m_region1.toString(), QString( "$DoubleBubbleGumBubblesDouble.$B$3:$K$13" ) );
}

void TestCellRegion::testToStringWithSpecialCharactersSingleTable()
{
    m_source.rename( "Table1", "table-one" );
    QCOMPARE( m_region1.toString(), QString( "$'table-one'.$B$3:$K$13" ) );
}

void TestCellRegion::testFromStringWithSpecialCharactersSingleTable()
{
    m_source.rename( "Table1", "table-one" );
    CellRegion region( &m_source, QString( "$'table-one'.$B$3:$K$13" ) );
    QCOMPARE( region.table(), m_source.get( "table-one" ) );
}

void TestCellRegion::testToStringMultibleTables()
{
    QCOMPARE( m_region1.toString(), QString( "$Table1.$B$3:$K$13;$Table2.$A$2:$E$7" ) );
}

void TestCellRegion::testFromStringMultibleTables()
{
    QCOMPARE( m_region1, CellRegion( &m_source, "$Table1.$B$3:$K$13;$Table2.$A$2:$E$7" ) );
}

void TestCellRegion::testTableNameChangeMultibleTables()
{
    QCOMPARE( m_region1.toString(), QString( "$Table1.$B$3:$K$13;$Table2.$A$2:$E$7" ) );
    m_source.rename( "Table1", "AGoodCookCanCookGoodCookies" );
    QCOMPARE( m_region1.toString(), QString( "$AGoodCookCanCookGoodCookies.$B$3:$K$13;$Table2.$A$2:$E$7" ) );
    m_source.rename( "Table2", "DoubleBubbleGumBubblesDouble" );
    QCOMPARE( m_region1.toString(), QString( "$AGoodCookCanCookGoodCookies.$B$3:$K$13;$DoubleBubbleGumBubblesDouble.$A$2:$E$7" ) );
}

void TestCellRegion::testToStringWithSpecialCharactersMultibleTables()
{
    m_source.rename( "Table1", "table-one" );
    QCOMPARE( m_region1.toString(), QString( "$'table-one'.$B$3:$K$13;$Table2.$A$2:$E$7" ) );
}

void TestCellRegion::testFromStringWithSpecialCharactersMultibleTables()
{
    m_source.rename( "Table1", "table-one" );
    CellRegion region( &m_source, QString( "$'table-one'.$B$3:$K$13" ) );
    QCOMPARE( region.table(), m_source.get( "table-one" ) );
}

QTEST_MAIN( TestCellRegion )
