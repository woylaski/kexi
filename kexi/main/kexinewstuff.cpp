/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#ifdef HAVE_KNEWSTUFF

#include <KTar>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QDebug>
#include <QFileDialog>

#include "kexinewstuff.h"

KexiNewStuff::KexiNewStuff(QWidget *parent)
        : KNewStuff("kexi/template"
                    , "http://download.kde.org/khotnewstuff/kexi-providers.xml"
                    , parent)
{
    // Prevent GHNS to deny downloading a second time. If GHNS
    // fails to download something, it still marks the thing as
    // successfully downloaded and therefore we arn't able to
    // download it again :-/
    KSharedConfig::openConfig()->deleteGroup("KNewStuffStatus");
}

KexiNewStuff::~KexiNewStuff()
{
}

bool
KexiNewStuff::install(const QString &fileName)
{
    qDebug() << fileName;

    KTar archive(fileName);
    if (!archive.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Failed to open archivefile \"%1\"").arg(fileName);
        return false;
    }
    const KArchiveDirectory *archiveDir = archive.directory();
    //! @todo KEXI3 add equivalent of kfiledialog:///
    //"kfiledialog:///DownloadExampleDatabases"
    const QString destDir = QFileDialog::getExistingDirectory(
        parentWidget(), QString(), xi18n("Choose Directory Where to Install Example Database"));
    if (destDir.isEmpty()) {
        qWarning() << "Destination-directory is empty.";
        return false;
    }
    archiveDir->copyTo(destDir);
    archive.close();
    return true;
}

bool
KexiNewStuff::createUploadFile(const QString &)
{
    return true;
}
#endif

//! @todo KEXI3 noi18n # added to disable message extraction in Messages.sh
