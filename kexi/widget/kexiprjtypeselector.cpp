/* This file is part of the KDE project
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>

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

#include "kexiprjtypeselector.h"
#include <KexiIcon.h>
#include <core/kexi.h>

#include <KDbDriver>
#include <KDbUtils>

KexiPrjTypeSelector::KexiPrjTypeSelector(QWidget* parent)
        : QWidget(parent)
{
    setupUi(this);
    setObjectName("KexiPrjTypeSelector");
    icon_file->setPixmap(Kexi::defaultFileBasedDriverIcon().pixmap(48,48));
    icon_file->setFixedSize(icon_file->pixmap()->size() / 2);
    icon_server->setPixmap(Kexi::serverIcon().pixmap(48,48));
    icon_server->setFixedSize(icon_server->pixmap()->size() / 2);

    connect(buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(slotSelectionChanged(QAbstractButton*)));

    slotSelectionChanged(option_file);
}

KexiPrjTypeSelector::~KexiPrjTypeSelector()
{
}

void KexiPrjTypeSelector::slotSelectionChanged(QAbstractButton *btn)
{
    frame_server->setEnabled(btn == option_server);
}

