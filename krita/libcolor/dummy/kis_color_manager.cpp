/*
 *  Copyright (c) 2015 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_color_manager.h"

#include <kglobal.h>

class KisColorManager::Private {
public:
    // we are too dumb for that :)
};


KisColorManager::KisColorManager()
    : QObject()
    , d(new Private())
{
}

KisColorManager::~KisColorManager()
{
}

QString KisColorManager::deviceName(const QString &id)
{
    return QString();
}

QStringList KisColorManager::devices(DeviceType type) const
{
    return QStringList();
}

QByteArray KisColorManager::displayProfile(const QString &device, int profile) const
{
    return QByteArray();
}

KisColorManager *KisColorManager::instance()
{
    K_GLOBAL_STATIC(KisColorManager, s_instance);
    return s_instance;
}
