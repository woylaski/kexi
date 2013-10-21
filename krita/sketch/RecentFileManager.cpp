/* This file is part of the KDE project
 * Copyright (C) 2012 Boudewijn Rempt <boud@kogmbh.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "RecentFileManager.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <kurl.h>
#include <kglobal.h>
#include <kconfiggroup.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>

// Much of this is a gui-less clone of KRecentFilesAction, so the format of
// storing recent files is compatible.
class RecentFileManager::Private {
public:
    Private()
    {
        KConfigGroup grp(KGlobal::config(), "RecentFiles");
        maxItems = grp.readEntry("maxRecentFileItems", 100);

        loadEntries(grp);
    }

    void loadEntries(const KConfigGroup &grp)
    {
        recentFiles.clear();
        recentFilesIndex.clear();

        QString value;
        QString nameValue;
        KUrl url;

        KConfigGroup cg = grp;

        if ( cg.name().isEmpty()) {
            cg = KConfigGroup(cg.config(),"RecentFiles");
        }

        // read file list
        for (int i = 1; i <= maxItems; i++) {

            value = cg.readPathEntry(QString("File%1").arg(i), QString());
            if (value.isEmpty()) continue;
            url = KUrl(value);

            // krita sketch only handles local files
            if (!url.isLocalFile())
                continue;

            // Don't restore if file doesn't exist anymore
            if (!QFile::exists(url.toLocalFile()))
                continue;

            value = QDir::toNativeSeparators( value );

            // Don't restore where the url is already known (eg. broken config)
            if (recentFiles.contains(value))
                continue;

            nameValue = cg.readPathEntry(QString("Name%1").arg(i), url.fileName());

            if (!value.isNull())  {
                recentFilesIndex << nameValue;
                recentFiles << value;
           }
        }
    }

    void saveEntries( const KConfigGroup &grp)
    {
        KConfigGroup cg = grp;

        if (cg.name().isEmpty()) {
            cg = KConfigGroup(cg.config(),"RecentFiles");
        }
        cg.deleteGroup();

        // write file list
        for (int i = 1; i <= recentFilesIndex.size(); ++i) {
            // i - 1 because we started from 1
            cg.writePathEntry(QString("File%1").arg(i), recentFiles[i - 1]);
            cg.writePathEntry(QString("Name%1").arg(i), recentFilesIndex[i - 1]);
        }
    }

    int maxItems;
    QStringList recentFilesIndex;
    QStringList recentFiles;
};




RecentFileManager::RecentFileManager(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
}

RecentFileManager::~RecentFileManager()
{
    KConfigGroup grp(KGlobal::config(), "RecentFiles");
    grp.writeEntry("maxRecentFileItems", d->maxItems);
    delete d;
}


QStringList RecentFileManager::recentFileNames() const
{
    return d->recentFilesIndex;
}

QStringList RecentFileManager::recentFiles() const
{
    return d->recentFiles;
}

void RecentFileManager::addRecent(const QString &_url)
{
    if (d->recentFiles.size() > d->maxItems) {
        d->recentFiles.removeLast();
        d->recentFilesIndex.removeLast();
    }

    QString localFile = QDir::toNativeSeparators(_url);
    QString fileName  = QFileInfo(_url).fileName();

    if (d->recentFiles.contains(localFile)) {
        d->recentFiles.removeAll(localFile);
    }

    if (d->recentFilesIndex.contains(fileName)) {
        d->recentFilesIndex.removeAll(fileName);
    }

    d->recentFiles.insert(0, localFile);
    d->recentFilesIndex.insert(0, fileName);

    d->saveEntries(KConfigGroup(KGlobal::config(), "RecentFiles"));
    emit recentFilesListChanged();
}

int RecentFileManager::size()
{
    return d->recentFiles.size();
}

QString RecentFileManager::recentFile(int index) const
{
    if (index < d->recentFiles.size()) {
        return d->recentFiles.at(index);
    }
    return QString();
}

QString RecentFileManager::recentFileName(int index) const
{
    if (index < d->recentFilesIndex.size()) {
        return d->recentFilesIndex.at(index);
    }
    return QString();
}
