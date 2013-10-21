/* This file is part of the KDE project
 *
 * Copyright (c) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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



#include "FileSystemModel.h"
#include <KDE/KDirLister>
#include <KDE/KDirModel>

class FileSystemModel::Private
{
public:
    QDir dir;
    QFileInfoList list;

    static const QString drivesPath;
};

const QString FileSystemModel::Private::drivesPath("special://drives");

FileSystemModel::FileSystemModel(QObject* parent)
    : QAbstractListModel(parent), d(new Private)
{
    d->dir.setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    d->dir.setSorting(QDir::DirsFirst | QDir::Name);

    QHash<int, QByteArray> roles;
    roles.insert(FileNameRole, "fileName");
    roles.insert(FilePathRole, "path");
    roles.insert(FileIconRole, "icon");
    roles.insert(FileTypeRole, "fileType");
    roles.insert(FileDateRole, "date");
    setRoleNames(roles);
}

FileSystemModel::~FileSystemModel()
{
    delete d;
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid()) {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, d->list.at(index.row()).absoluteFilePath(), false);
        if (!item.isNull()) {
            switch(role) {
                case FileNameRole:
                    return item.name();
                    break;
                case FilePathRole:
                    return item.mostLocalUrl().toLocalFile();
                    break;
                case FileIconRole:
                     return item.mimetype() == "inode/directory" ? "image://icon/inode-directory" : QString("image://recentimage/%1").arg(item.url().toLocalFile());
                    break;
                case FileTypeRole:
                    return item.mimetype();
                    break;
                case FileDateRole:
                    return item.time(KFileItem::ModificationTime).dateTime().toString(Qt::SystemLocaleShortDate);
            }
        }
    }
    return QVariant();
}

int FileSystemModel::rowCount(const QModelIndex&) const
{
    return d->list.count();
}

void FileSystemModel::classBegin()
{

}

void FileSystemModel::componentComplete()
{
    setPath(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
}

QString FileSystemModel::path()
{
    QString path = d->dir.absolutePath();
    if (path.isEmpty()) {
        return Private::drivesPath;
    } else {
        return d->dir.absolutePath();
    }
}

void FileSystemModel::setPath(const QString& path)
{
    if (path != d->dir.path()) {
        if (d->list.count() > 0) {
            beginRemoveRows(QModelIndex(), 0, d->list.count() - 1);
            endRemoveRows();
        }

        if (path != Private::drivesPath) {
            d->dir.setPath(path);
            d->dir.refresh();
            d->list = d->dir.entryInfoList();
            if (d->list.count() > 0) {
                beginInsertRows(QModelIndex(), 0, d->list.count() - 1);
                endInsertRows();
            }
        } else {
            d->dir.setPath("");
            d->dir.refresh();
            d->list = QDir::drives();

            beginInsertRows(QModelIndex(), 0, d->list.count() - 1);
            endInsertRows();
        }
        emit pathChanged();
    }
}

QString FileSystemModel::parentFolder()
{
    if (path() != Private::drivesPath) {
        KUrl root = QUrl::fromLocalFile(path());
        if (QRegExp("^[A-Z]{1,3}:/$").exactMatch(root.toLocalFile())) {
            return Private::drivesPath;
        } else {
            root.cd("..");
            return root.toLocalFile();
        }
    } else {
        return Private::drivesPath;
    }
}

QString FileSystemModel::filter()
{
    return d->dir.nameFilters().join(" ");
}

void FileSystemModel::setFilter(const QString& filter)
{
    d->dir.setNameFilters(filter.split(" "));
}

