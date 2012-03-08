/* This file is part of the KDE project
   Copyright (C) 2005 Tomas Mecir <mecirt@gmail.com>
   Copyright (C) 2000 Torben Weis <weis@kde.org>

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
#include "Cluster.h"

#include <stdlib.h>

#include <kdebug.h>

#include "Cell.h"
#include "RowColumnFormat.h"

using namespace Calligra::Tables;

#if 0
/****************************************************
 *
 * Cluster
 *
 ****************************************************/

/* Generate a matrix LEVEL1 with the size LEVEL1*LEVEL1 */
Cluster::Cluster()
        : m_first(0), m_autoDelete(false), m_biggestX(0), m_biggestY(0)
{
    m_cluster = (Cell***)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL1 * CALLIGRA_TABLES_CLUSTER_LEVEL1 * sizeof(Cell**));

    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x)
        for (int y = 0; y < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++y)
            m_cluster[ y * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x ] = 0;
}

/* Delete the matrix LEVEL1 and all existing LEVEL2 matrizes */
Cluster::~Cluster()
{
// Can't we use clear(), to remove double code - Philipp?
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x)
        for (int y = 0; y < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++y) {
            Cell** cl = m_cluster[ y * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x ];
            if (cl) {
                free(cl);
                m_cluster[ y * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x ] = 0;
            }
        }

    if (m_autoDelete) {
        Cell* cell = m_first;
        while (cell) {
            Cell* n = cell->nextCell();
            delete cell;
            cell = n;
        }
    }

    free(m_cluster);
}

void Cluster::clear()
{
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x)
        for (int y = 0; y < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++y) {
            Cell** cl = m_cluster[ y * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x ];
            if (cl) {
                free(cl);
                m_cluster[ y * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x ] = 0;
            }
        }

    if (m_autoDelete) {
        Cell* cell = m_first;
        while (cell) {
            Cell* n = cell->nextCell();
            delete cell;
            cell = n;
        }
    }

    m_first = 0;
    m_biggestX = m_biggestY = 0;
}

Cell* Cluster::lookup(int x, int y) const
{
    if (x >= CALLIGRA_TABLES_CLUSTER_MAX || x < 0 || y >= CALLIGRA_TABLES_CLUSTER_MAX || y < 0) {
        kDebug(36001) << "Cluster::lookup: invalid column or row value (col:"
        << x << "  | row: " << y << ")" << endl;
        return 0;
    }
    int cx = x / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = y / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = x % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = y % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
    if (!cl)
        return 0;

    return cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ];
}

/* Paste a cell in LEVEL2 (it's more paste than insert) */
void Cluster::insert(Cell* cell, int x, int y)
{
    if (x >= CALLIGRA_TABLES_CLUSTER_MAX || x < 0 || y >= CALLIGRA_TABLES_CLUSTER_MAX || y < 0) {
        kDebug(36001) << "Cluster::insert: invalid column or row value (col:"
        << x << "  | row: " << y << ")" << endl;
        return;
    }

    int cx = x / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = y / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = x % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = y % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
    if (!cl) {
        cl = (Cell**)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL2 * CALLIGRA_TABLES_CLUSTER_LEVEL2 * sizeof(Cell*));
        m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ] = cl;

        for (int a = 0; a < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++a)
            for (int b = 0; b < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++b)
                cl[ b * CALLIGRA_TABLES_CLUSTER_LEVEL2 + a ] = 0;
    }

    if (cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ])
        remove(x, y);

    cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ] = cell;

    if (m_first) {
        cell->setNextCell(m_first);
        m_first->setPreviousCell(cell);
    }
    m_first = cell;

    if (x > m_biggestX) m_biggestX = x;
    if (y > m_biggestY) m_biggestY = y;
}

/* Removes the cell of a matrix, the matrix itself keeps unchanged */
void Cluster::remove(int x, int y)
{
    if (x >= CALLIGRA_TABLES_CLUSTER_MAX || x < 0 || y >= CALLIGRA_TABLES_CLUSTER_MAX || y < 0) {
        kDebug(36001) << "Cluster::remove: invalid column or row value (col:"
        << x << "  | row: " << y << ")" << endl;
        return;
    }

    int cx = x / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = y / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = x % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = y % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
    if (!cl)
        return;

    Cell* c = cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ];
    if (!c)
        return;

    cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ] = 0;

    if (m_autoDelete) {
        if (m_first == c)
            m_first = c->nextCell();
        if (c->doesMergeCells()) {
            c->mergeCells(c->column(), c->row(), 0, 0);
        }
        delete c;
    } else {
        if (m_first == c)
            m_first = c->nextCell();
        if (c->previousCell())
            c->previousCell()->setNextCell(c->nextCell());
        if (c->nextCell())
            c->nextCell()->setPreviousCell(c->previousCell());
        c->setNextCell(0);
        c->setPreviousCell(0);
    }
}

bool Cluster::insertShiftRight(const QPoint& marker)
{
    bool dummy;
    return insertShiftRight(marker, dummy);
}

bool Cluster::insertShiftDown(const QPoint& marker)
{
    bool dummy;
    return insertShiftDown(marker, dummy);
}

void Cluster::removeShiftUp(const QPoint& marker)
{
    bool dummy;
    removeShiftUp(marker, dummy);
}

void Cluster::removeShiftLeft(const QPoint& marker)
{
    bool dummy;
    removeShiftLeft(marker, dummy);
}

void Cluster::setAutoDelete(bool b)
{
    m_autoDelete = b;
}

bool Cluster::autoDelete() const
{
    return m_autoDelete;
}

Cell* Cluster::firstCell() const
{
    return m_first;
}

bool Cluster::insertShiftRight(const QPoint& marker, bool& work)
{
    work = false;

    if (marker.x() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.x() < 0 ||
            marker.y() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.y() < 0) {
        kDebug(36001) << "Cluster::insertShiftRight: invalid column or row value (col:"
        << marker.x() << "  | row: " << marker.y() << ")" << endl;
        return false;
    }

    int cx = marker.x() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = marker.y() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = marker.x() % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = marker.y() % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    // Is there a cell at the bottom most position ?
    // In this case the shift is impossible.
    Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1 ];
    if (cl && cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1 ])
        return false;

    bool a = autoDelete();
    setAutoDelete(false);

    // Move cells in this row one down.
    for (int i = CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1; i >= cx ; --i) {
        Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + i ];
        if (cl) {
            work = true;
            int left = 0;
            if (i == cx)
                left = dx;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            if (i == CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1)
                right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 2;
            for (int k = right; k >= left; --k) {
                Cell* c = cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + k ];
                if (c) {
                    remove(c->column(), c->row());
                    c->move(c->column() + 1, c->row());
                    insert(c, c->column(), c->row());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

bool Cluster::insertShiftDown(const QPoint& marker, bool& work)
{
    work = false;

    if (marker.x() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.x() < 0 ||
            marker.y() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.y() < 0) {
        kDebug(36001) << "Cluster::insertShiftDown: invalid column or row value (col:"
        << marker.x() << "  | row: " << marker.y() << ")" << endl;
        return false;
    }

    int cx = marker.x() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = marker.y() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = marker.x() % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = marker.y() % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    // Is there a cell at the right most position ?
    // In this case the shift is impossible.
    Cell** cl = m_cluster[ CALLIGRA_TABLES_CLUSTER_LEVEL1 * (CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1) + cx ];
    if (cl && cl[ CALLIGRA_TABLES_CLUSTER_LEVEL2 *(CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1) + dx ])
        return false;

    bool a = autoDelete();
    setAutoDelete(false);

    // Move cells in this column one right.
    for (int i = CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1; i >= cy ; --i) {
        Cell** cl = m_cluster[ i * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
        if (cl) {
            work = true;

            int top = 0;
            if (i == cy)
                top = dy;
            int bottom = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            if (i == CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1)
                bottom = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 2;
            for (int k = bottom; k >= top; --k) {
                Cell* c = cl[ k * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ];
                if (c) {
                    remove(c->column(), c->row());
                    c->move(c->column(), c->row() + 1);
                    insert(c, c->column(), c->row());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

bool Cluster::insertColumn(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "Cluster::insertColumn: invalid column value (col:"
        << col << ")" << endl;
        return false;
    }

    // Is there a cell at the right most position ?
    // In this case the shift is impossible.
    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        Cell** cl = m_cluster[ t1 * CALLIGRA_TABLES_CLUSTER_LEVEL1 + CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1 ];
        if (cl)
            for (int t2 = 0; t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
                if (cl[ t2 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1 ])
                    return false;
    }

    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        bool work = true;
        for (int t2 = 0; work && t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
            insertShiftRight(QPoint(col, t1 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + t2), work);
    }

    return true;
}

bool Cluster::insertRow(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "Cluster::insertRow: invalid row value (row:"
        << row << ")" << endl;
        return false;
    }

    // Is there a cell at the bottom most position ?
    // In this case the shift is impossible.
    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        Cell** cl = m_cluster[ CALLIGRA_TABLES_CLUSTER_LEVEL1 * (CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1) + t1 ];
        if (cl)
            for (int t2 = 0; t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
                if (cl[ CALLIGRA_TABLES_CLUSTER_LEVEL2 *(CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1) + t2 ])
                    return false;
    }

    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        bool work = true;
        for (int t2 = 0; work && t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
            insertShiftDown(QPoint(t1 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + t2, row), work);
    }

    return true;
}

void Cluster::removeShiftUp(const QPoint& marker, bool& work)
{
    work = false;

    if (marker.x() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.x() < 0 ||
            marker.y() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.y() < 0) {
        kDebug(36001) << "Cluster::removeShiftUp: invalid column or row value (col:"
        << marker.x() << "  | row: " << marker.y() << ")" << endl;
        return;
    }

    int cx = marker.x() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = marker.y() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = marker.x() % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = marker.y() % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    bool a = autoDelete();
    setAutoDelete(false);

    // Move cells in this column one column to the left.
    for (int i = cy; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        Cell** cl = m_cluster[ i * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
        if (cl) {
            work = true;

            int top = 0;
            if (i == cy)
                top = dy + 1;
            int bottom = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            for (int k = top; k <= bottom; ++k) {
                Cell* c = cl[ k * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ];
                if (c) {
                    remove(c->column(), c->row());
                    c->move(c->column(), c->row() - 1);
                    insert(c, c->column(), c->row());
                }
            }
        }
    }

    setAutoDelete(a);
}

void Cluster::removeShiftLeft(const QPoint& marker, bool& work)
{
    work = false;

    if (marker.x() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.x() < 0 ||
            marker.y() >= CALLIGRA_TABLES_CLUSTER_MAX || marker.y() < 0) {
        kDebug(36001) << "Cluster::removeShiftLeft: invalid column or row value (col:"
        << marker.x() << "  | row: " << marker.y() << ")" << endl;
        return;
    }

    int cx = marker.x() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = marker.y() / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = marker.x() % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = marker.y() % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    bool a = autoDelete();
    setAutoDelete(false);

    // Move cells in this row one row up.
    for (int i = cx; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + i ];
        if (cl) {
            work = true;

            int left = 0;
            if (i == cx)
                left = dx + 1;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            for (int k = left; k <= right; ++k) {
                Cell* c = cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + k ];
                if (c) {
                    remove(c->column(), c->row());
                    c->move(c->column() - 1, c->row());
                    insert(c, c->column(), c->row());
                }
            }
        }
    }

    setAutoDelete(a);
}

void Cluster::removeColumn(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "Cluster::removeColumn: invalid column value (col:"
        << col << ")" << endl;
        return;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    for (int y1 = 0; y1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++y1) {
        Cell** cl = m_cluster[ y1 * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
        if (cl)
            for (int y2 = 0; y2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++y2)
                if (cl[ y2 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ])
                    remove(col, y1 * CALLIGRA_TABLES_CLUSTER_LEVEL1 + y2);
    }

    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        bool work = true;
        for (int t2 = 0; work && t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
            removeShiftLeft(QPoint(col, t1 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + t2), work);
    }
}

void Cluster::removeRow(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "Cluster::removeRow: invalid row value (row:"
        << row << ")" << endl;
        return;
    }

    int cy = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    for (int x1 = 0; x1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x1) {
        Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + x1 ];
        if (cl)
            for (int x2 = 0; x2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++x2)
                if (cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + x2 ])
                    remove(x1 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + x2, row);
    }

    for (int t1 = 0; t1 < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++t1) {
        bool work = true;
        for (int t2 = 0; work && t2 < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++t2)
            removeShiftUp(QPoint(t1 * CALLIGRA_TABLES_CLUSTER_LEVEL2 + t2, row), work);
    }
}

void Cluster::clearColumn(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "Cluster::clearColumn: invalid column value (col:"
        << col << ")" << endl;
        return;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    for (int cy = 0; cy < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++cy) {
        Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ];
        if (cl)
            for (int dy = 0; dy < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++dy)
                if (cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ]) {
                    int row = cy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dy ;
                    remove(col, row);
                }
    }
}

void Cluster::clearRow(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "Cluster::clearRow: invalid row value (row:"
        << row << ")" << endl;
        return;
    }

    int cy = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    for (int cx = 0; cx < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++cx) {
        Cell** cl = m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + cx ];
        if (cl)
            for (int dx = 0; dx < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++dx)
                if (cl[ dy * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ]) {
                    int column = cx * CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx ;
                    remove(column, row);
                }
    }
}

Value Cluster::valueRange(int col1, int row1,
                          int col2, int row2) const
{
    Value empty;

    //swap first/second values if needed
    if (col1 > col2) {
        int p = col1; col1 = col2; col2 = p;
    }
    if (row1 > row2) {
        int p = row1; row1 = col2; row2 = p;
    }
    if ((row1 < 0) || (col1 < 0) || (row2 > CALLIGRA_TABLES_CLUSTER_MAX) ||
            (col2 > CALLIGRA_TABLES_CLUSTER_MAX))
        return empty;

    // if we are out of range occupied by cells, we return an empty
    // array of the requested size
    if ((row1 > m_biggestY) || (col1 > m_biggestX))
        return Value(Value::Array);

    return makeArray(col1, row1, col2, row2);
}

Value Cluster::makeArray(int col1, int row1, int col2, int row2) const
{
    // this generates an array of values
    Value array(Value::Array);
    for (int row = row1; row <= row2; ++row) {
        for (Cell* cell = getFirstCellRow(row); cell; cell = getNextCellRight(cell->column(), row)) {
            if (cell->column() >= col1 && cell->column() <= col2)
                array.setElement(cell->column() - col1, row - row1, cell->value());
        }
    }
    //return the result
    return array;
}

Cell* Cluster::getFirstCellColumn(int col) const
{
    Cell* cell = lookup(col, 1);

    if (cell == 0) {
        cell = getNextCellDown(col, 1);
    }
    return cell;
}

Cell* Cluster::getLastCellColumn(int col) const
{
    Cell* cell = lookup(col, KS_rowMax);

    if (cell == 0) {
        cell = getNextCellUp(col, KS_rowMax);
    }
    return cell;
}

Cell* Cluster::getFirstCellRow(int row) const
{
    Cell* cell = lookup(1, row);

    if (cell == 0) {
        cell = getNextCellRight(1, row);
    }
    return cell;
}

Cell* Cluster::getLastCellRow(int row) const
{
    Cell* cell = lookup(KS_colMax, row);

    if (cell == 0) {
        cell = getNextCellLeft(KS_colMax, row);
    }
    return cell;
}

Cell* Cluster::getNextCellUp(int col, int row) const
{
    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = (row - 1) / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = (row - 1) % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    while (cy >= 0) {
        if (m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ] != 0) {
            while (dy >= 0) {

                if (m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx]
                        [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx] != 0) {
                    return m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ]
                           [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx];
                }
                dy--;
            }
        }
        cy--;
        dy = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
    }
    return 0;
}

Cell* Cluster::getNextCellDown(int col, int row) const
{
    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = (row + 1) / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = (row + 1) % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    while (cy < CALLIGRA_TABLES_CLUSTER_LEVEL1) {
        if (m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ] != 0) {
            while (dy < CALLIGRA_TABLES_CLUSTER_LEVEL2) {

                if (m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx]
                        [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx] != 0) {
                    return m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ]
                           [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx];
                }
                dy++;
            }
        }
        cy++;
        dy = 0;
    }
    return 0;
}

Cell* Cluster::getNextCellLeft(int col, int row) const
{
    int cx = (col - 1) / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = (col - 1) % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    while (cx >= 0) {
        if (m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ] != 0) {
            while (dx >= 0) {

                if (m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx]
                        [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx] != 0) {
                    return m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ]
                           [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx];
                }
                dx--;
            }
        }
        cx--;
        dx = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
    }
    return 0;
}

Cell* Cluster::getNextCellRight(int col, int row) const
{
    int cx = (col + 1) / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int cy = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = (col + 1) % CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dy = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    while (cx < CALLIGRA_TABLES_CLUSTER_LEVEL1) {
        if (m_cluster[ cy * CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ] != 0) {
            while (dx < CALLIGRA_TABLES_CLUSTER_LEVEL2) {

                if (m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx]
                        [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx] != 0) {
                    return m_cluster[ cy*CALLIGRA_TABLES_CLUSTER_LEVEL1 + cx ]
                           [ dy*CALLIGRA_TABLES_CLUSTER_LEVEL2 + dx];
                }
                dx++;
            }
        }
        cx++;
        dx = 0;
    }
    return 0;
}
#endif

/****************************************************
 *
 * ColumnCluster
 *
 ****************************************************/

ColumnCluster::ColumnCluster()
        : m_first(0), m_autoDelete(false)
{
    m_cluster = (ColumnFormat***)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL1 * sizeof(ColumnFormat**));

    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x)
        m_cluster[ x ] = 0;
}

ColumnCluster::~ColumnCluster()
{
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x) {
        ColumnFormat** cl = m_cluster[ x ];
        if (cl) {
            free(cl);
            m_cluster[ x ] = 0;
        }
    }

    if (m_autoDelete) {
        ColumnFormat* cell = m_first;
        while (cell) {
            ColumnFormat* n = cell->next();
            delete cell;
            cell = n;
        }
    }


    free(m_cluster);
}

ColumnFormat* ColumnCluster::lookup(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::lookup: invalid column value (col:"
        << col << ")" << endl;
        return 0;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    ColumnFormat** cl = m_cluster[ cx ];
    if (!cl)
        return 0;

    return cl[ dx ];
}

const ColumnFormat* ColumnCluster::lookup(int col) const
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::lookup: invalid column value (col:"
        << col << ")" << endl;
        return 0;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    ColumnFormat** cl = m_cluster[ cx ];
    if (!cl)
        return 0;

    return cl[ dx ];
}

void ColumnCluster::clear()
{
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x) {
        ColumnFormat** cl = m_cluster[ x ];
        if (cl) {
            free(cl);
            m_cluster[ x ] = 0;
        }
    }

    if (m_autoDelete) {
        ColumnFormat* cell = m_first;
        while (cell) {
            ColumnFormat* n = cell->next();
            delete cell;
            cell = n;
        }
    }

    m_first = 0;
}

void ColumnCluster::insertElement(ColumnFormat* lay, int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::insertElement: invalid column value (col:"
        << col << ")" << endl;
        return;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    ColumnFormat** cl = m_cluster[ cx ];
    if (!cl) {
        cl = (ColumnFormat**)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL2 * sizeof(ColumnFormat*));
        m_cluster[ cx ] = cl;

        for (int a = 0; a < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++a)
            cl[ a ] = 0;
    }

    if (cl[ dx ])
        removeElement(col);

    cl[ dx ] = lay;

    if (m_first) {
        lay->setNext(m_first);
        m_first->setPrevious(lay);
    }
    m_first = lay;
}

void ColumnCluster::removeElement(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::removeElement: invalid column value (col:"
        << col << ")" << endl;
        return;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    ColumnFormat** cl = m_cluster[ cx ];
    if (!cl)
        return;

    ColumnFormat* c = cl[ dx ];
    if (!c)
        return;

    cl[ dx ] = 0;

    if (m_autoDelete) {
        if (m_first == c)
            m_first = c->next();
        delete c;
    } else {
        if (m_first == c)
            m_first = c->next();
        if (c->previous())
            c->previous()->setNext(c->next());
        if (c->next())
            c->next()->setPrevious(c->previous());
        c->setNext(0);
        c->setPrevious(0);
    }
}

bool ColumnCluster::insertColumn(int col)
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::insertColumn: invalid column value (col:"
        << col << ")" << endl;
        return false;
    }

    int cx = col / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = col % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    // Is there a column layout at the right most position ?
    // In this case the shift is impossible.
    ColumnFormat** cl = m_cluster[ CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1 ];
    if (cl && cl[ CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1 ])
        return false;

    bool a = autoDelete();
    setAutoDelete(false);

    for (int i = CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1; i >= cx ; --i) {
        ColumnFormat** cl = m_cluster[ i ];
        if (cl) {
            int left = 0;
            if (i == cx)
                left = dx;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            if (i == CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1)
                right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 2;
            for (int k = right; k >= left; --k) {
                ColumnFormat* c = cl[ k ];
                if (c) {
                    removeElement(c->column());
                    c->setColumn(c->column() + 1);
                    insertElement(c, c->column());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

bool ColumnCluster::removeColumn(int column)
{
    if (column >= CALLIGRA_TABLES_CLUSTER_MAX || column < 0) {
        kDebug(36001) << "ColumnCluster::removeColumn: invalid column value (col:"
        << column << ")" << endl;
        return false;
    }

    int cx = column / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = column % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    removeElement(column);

    bool a = autoDelete();
    setAutoDelete(false);

    for (int i = cx; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        ColumnFormat** cl = m_cluster[ i ];
        if (cl) {
            int left = 0;
            if (i == cx)
                left = dx + 1;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            for (int k = left; k <= right; ++k) {
                ColumnFormat* c = cl[ k ];
                if (c) {
                    removeElement(c->column());
                    c->setColumn(c->column() - 1);
                    insertElement(c, c->column());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

void ColumnCluster::setAutoDelete(bool a)
{
    m_autoDelete = a;
}

bool ColumnCluster::autoDelete() const
{
    return m_autoDelete;
}

ColumnFormat* ColumnCluster::next(int col) const
{
    if (col >= CALLIGRA_TABLES_CLUSTER_MAX || col < 0) {
        kDebug(36001) << "ColumnCluster::next: invalid column value (col:"
        << col << ")" << endl;
        return 0;
    }

    int cx = (col + 1) / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = (col + 1) % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    while (cx < CALLIGRA_TABLES_CLUSTER_LEVEL1) {
        if (m_cluster[ cx ]) {
            while (dx < CALLIGRA_TABLES_CLUSTER_LEVEL2) {

                if (m_cluster[ cx ][  dx ]) {
                    return m_cluster[ cx ][ dx ];
                }
                ++dx;
            }
        }
        ++cx;
        dx = 0;
    }
    return 0;
}

void ColumnCluster::operator=(const ColumnCluster & other)
{
    m_first = 0;
    m_autoDelete = other.m_autoDelete;
    // TODO Stefan: Optimize!
    m_cluster = (ColumnFormat***)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL1 * sizeof(ColumnFormat**));
    for (int i = 0; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        if (other.m_cluster[i]) {
            m_cluster[i] = (ColumnFormat**)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL2 * sizeof(ColumnFormat*));
            for (int j = 0; j < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++j) {
                m_cluster[i][j] = 0;
                if (other.m_cluster[i][j]) {
                    ColumnFormat* columnFormat = new ColumnFormat(*other.m_cluster[i][j]);
                    columnFormat->setNext(0);
                    columnFormat->setPrevious(0);
                    insertElement(columnFormat, columnFormat->column());
                }
            }
        } else
            m_cluster[i] = 0;
    }
}

/****************************************************
 *
 * RowCluster
 *
 ****************************************************/

RowCluster::RowCluster()
        : m_first(0), m_autoDelete(false)
{
    m_cluster = (RowFormat***)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL1 * sizeof(RowFormat**));

    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x)
        m_cluster[ x ] = 0;
}

RowCluster::~RowCluster()
{
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x) {
        RowFormat** cl = m_cluster[ x ];
        if (cl) {
            free(cl);
            m_cluster[ x ] = 0;
        }
    }

    if (m_autoDelete) {
        RowFormat* cell = m_first;
        while (cell) {
            RowFormat* n = cell->next();
            delete cell;
            cell = n;
        }
    }

    free(m_cluster);
}

const RowFormat* RowCluster::lookup(int row) const
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::lookup: invalid row value (row:"
        << row << ")" << endl;
        return 0;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    RowFormat** cl = m_cluster[ cx ];
    if (!cl)
        return 0;

    return cl[ dx ];
}

RowFormat* RowCluster::lookup(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::lookup: invalid row value (row:"
        << row << ")" << endl;
        return 0;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    RowFormat** cl = m_cluster[ cx ];
    if (!cl)
        return 0;

    return cl[ dx ];
}

void RowCluster::clear()
{
    for (int x = 0; x < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++x) {
        RowFormat** cl = m_cluster[ x ];
        if (cl) {
            free(cl);
            m_cluster[ x ] = 0;
        }
    }

    if (m_autoDelete) {
        RowFormat* cell = m_first;
        while (cell) {
            RowFormat* n = cell->next();
            delete cell;
            cell = n;
        }
    }

    m_first = 0;
}

void RowCluster::insertElement(RowFormat* lay, int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::insertElement: invalid row value (row:"
        << row << ")" << endl;
        return;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    RowFormat** cl = m_cluster[ cx ];
    if (!cl) {
        cl = (RowFormat**)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL2 * sizeof(RowFormat*));
        m_cluster[ cx ] = cl;

        for (int a = 0; a < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++a)
            cl[ a ] = 0;
    }

    if (cl[ dx ])
        removeElement(row);

    cl[ dx ] = lay;

    if (m_first) {
        lay->setNext(m_first);
        m_first->setPrevious(lay);
    }
    m_first = lay;
}

void RowCluster::removeElement(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::removeElement: invalid row value (row:"
        << row << ")" << endl;
        return;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    RowFormat** cl = m_cluster[ cx ];
    if (!cl)
        return;

    RowFormat* c = cl[ dx ];
    if (!c)
        return;

    cl[ dx ] = 0;

    if (m_autoDelete) {
        if (m_first == c)
            m_first = c->next();
        delete c;
    } else {
        if (m_first == c)
            m_first = c->next();
        if (c->previous())
            c->previous()->setNext(c->next());
        if (c->next())
            c->next()->setPrevious(c->previous());
        c->setNext(0);
        c->setPrevious(0);
    }
}

bool RowCluster::insertRow(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::insertRow: invalid row value (row:"
        << row << ")" << endl;
        return false;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    // Is there a row layout at the bottom most position ?
    // In this case the shift is impossible.
    RowFormat** cl = m_cluster[ CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1 ];
    if (cl && cl[ CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1 ])
        return false;

    bool a = autoDelete();
    setAutoDelete(false);

    for (int i = CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1; i >= cx ; --i) {
        RowFormat** cl = m_cluster[ i ];
        if (cl) {
            int left = 0;
            if (i == cx)
                left = dx;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            if (i == CALLIGRA_TABLES_CLUSTER_LEVEL1 - 1)
                right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 2;
            for (int k = right; k >= left; --k) {
                RowFormat* c = cl[ k ];
                if (c) {
                    removeElement(c->row());
                    c->setRow(c->row() + 1);
                    insertElement(c, c->row());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

bool RowCluster::removeRow(int row)
{
    if (row >= CALLIGRA_TABLES_CLUSTER_MAX || row < 0) {
        kDebug(36001) << "RowCluster::removeRow: invalid row value (row:"
        << row << ")" << endl;
        return false;
    }

    int cx = row / CALLIGRA_TABLES_CLUSTER_LEVEL2;
    int dx = row % CALLIGRA_TABLES_CLUSTER_LEVEL2;

    removeElement(row);

    bool a = autoDelete();
    setAutoDelete(false);

    for (int i = cx; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        RowFormat** cl = m_cluster[ i ];
        if (cl) {
            int left = 0;
            if (i == cx)
                left = dx + 1;
            int right = CALLIGRA_TABLES_CLUSTER_LEVEL2 - 1;
            for (int k = left; k <= right; ++k) {
                RowFormat* c = cl[ k ];
                if (c) {
                    removeElement(c->row());
                    c->setRow(c->row() - 1);
                    insertElement(c, c->row());
                }
            }
        }
    }

    setAutoDelete(a);

    return true;
}

void RowCluster::setAutoDelete(bool a)
{
    m_autoDelete = a;
}

bool RowCluster::autoDelete() const
{
    return m_autoDelete;
}

void RowCluster::operator=(const RowCluster & other)
{
    m_first = 0;
    m_autoDelete = other.m_autoDelete;
    // TODO Stefan: Optimize!
    m_cluster = (RowFormat***)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL1 * sizeof(RowFormat**));
    for (int i = 0; i < CALLIGRA_TABLES_CLUSTER_LEVEL1; ++i) {
        if (other.m_cluster[i]) {
            m_cluster[i] = (RowFormat**)malloc(CALLIGRA_TABLES_CLUSTER_LEVEL2 * sizeof(RowFormat*));
            for (int j = 0; j < CALLIGRA_TABLES_CLUSTER_LEVEL2; ++j) {
                m_cluster[i][j] = 0;
                if (other.m_cluster[i][j]) {
                    RowFormat* rowFormat = new RowFormat(*other.m_cluster[i][j]);
                    rowFormat->setNext(0);
                    rowFormat->setPrevious(0);
                    insertElement(rowFormat, rowFormat->row());
                }
            }
        } else
            m_cluster[i] = 0;
    }
}