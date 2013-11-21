/*
 *  Copyright (C) 2013 Juan Palacios <jpalaciosdev@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KOSIZEGROUPPRIVATE_H
#define KOSIZEGROUPPRIVATE_H

#include <QObject>
#include <QWidgetItem>
#include <QList>
#include <QSize>

#include "KoSizeGroup.h"

class QWidget;
class QEvent;
class QTimer;

class GroupItem;
class KoSizeGroupPrivate : public QObject
{
    Q_OBJECT

public:
    KoSizeGroupPrivate(KoSizeGroup *q_ptr, KoSizeGroup::mode mode, bool ignoreHidden);

    void addWidget(QWidget *widget);
    void removeWidget(QWidget *widget);

    /// Schedules an update of all widgets size
    void scheduleSizeUpdate();

    /// Returns the current maximunt size hint of all widgets inside the size group.
    const QSize getMaxSizeHint() const { return m_maxSizeHint; }


private Q_SLOTS:
    void updateSize();

public:
    KoSizeGroup* q;
    KoSizeGroup::mode m_mode;
    bool m_ignoreHidden;

private:
    QTimer* m_updateTimer; // used to filter multiple calls to scheduleSizeUpdate() into one single updateSize()
    QList<GroupItem*> m_groupItems;
    QSize m_maxSizeHint;
};


class GroupItem : public QObject, public QWidgetItem
{
    Q_OBJECT

public:
    GroupItem(QWidget* widget);
    ~GroupItem() {}

    void setSize(const QSize &size) { m_size = size; }

    int getWidth() const { return m_size.width(); }
    void setWidth(int width) { m_size.setWidth(width); }

    int getHeight() const { return m_size.height(); }
    void setHeight(int height) { m_size.setHeight(height); }

    bool hidden() const { return m_hidden; }

    KoSizeGroupPrivate* getGroup() { return m_group; }
    void setGroup(KoSizeGroupPrivate* group) { m_group = group; }

    QSize sizeHint() const;
    QSize minimumSize() const;

    bool eventFilter(QObject*, QEvent *event);

private:
    bool m_hidden;
    QSize m_size;
    KoSizeGroupPrivate* m_group;
};

#endif // KOSIZEGROUPPRIVATE_H
