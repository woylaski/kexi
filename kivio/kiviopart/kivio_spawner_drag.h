/*
 * Kivio - Visual Modelling and Flowcharting
 * Copyright (C) 2000 theKompany.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef KIVIO_SPAWNER_DRAG_H
#define KIVIO_SPAWNER_DRAG_H

#include <qdragobject.h>
#include <qstringlist.h>
#include <qiconview.h>

class KivioStencilSpawner;
class KivioIconView;

class KivioSpawnerDrag : public QIconDrag
{
public:
    KivioSpawnerDrag( KivioIconView *, QWidget *parent=0, const char *name=0);
    virtual ~KivioSpawnerDrag();

    const char *format(int i) const;
    QByteArray encodedData( const char *mime ) const;
    static bool canDecode( QMimeSource* e );
    void append( const QIconDragItem &item, const QRect &pr,
                const QRect &tr, KivioStencilSpawner &spawner );

private:
    QStringList m_spawners;
    KivioIconView *m_pView;

};

#endif

