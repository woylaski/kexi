/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#include "PanelConfiguration.h"

#include <QDeclarativeItem>
#include <QSettings>
#include <QCoreApplication>
#include <KStandardDirs>

#include <kis_config.h>

class PanelConfiguration::Private
{
public:
    QList<QDeclarativeItem*> panels;
    QList<QDeclarativeItem*> panelAreas;

    QHash<QString, QString> panelAreaMap;
};

PanelConfiguration::PanelConfiguration(QObject* parent)
    : QObject(parent), d(new Private)
{
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), SLOT(save()));
}

PanelConfiguration::~PanelConfiguration()
{
    delete d;
}

void PanelConfiguration::componentComplete()
{
    QString configFile = KGlobal::dirs()->locate("config", "kritasketchpanelsrc");
    QSettings panelConfig(configFile, QSettings::IniFormat);

    int count = panelConfig.beginReadArray("Panels");
    for(int i = 0; i < count; ++i) {
        panelConfig.setArrayIndex(i);

        QString panel = panelConfig.value("panel").toString();
        QString area = panelConfig.value("area").toString();
        d->panelAreaMap.insert(panel, area);
    }
    panelConfig.endArray();
}

void PanelConfiguration::classBegin()
{

}

QDeclarativeListProperty< QDeclarativeItem > PanelConfiguration::panels()
{
    return QDeclarativeListProperty<QDeclarativeItem>(this, d->panels);
}

QDeclarativeListProperty< QDeclarativeItem > PanelConfiguration::panelAreas()
{
    return QDeclarativeListProperty<QDeclarativeItem>(this, d->panelAreas);
}

void PanelConfiguration::restore()
{
    if (d->panelAreaMap.count() == d->panels.count()) {
        foreach(QDeclarativeItem* panel, d->panels) {
            QString panelName = panel->objectName();
            QString area = d->panelAreaMap.value(panelName);

            foreach(QDeclarativeItem* panelArea, d->panelAreas) {
                if (panelArea->objectName() == area) {
                    panel->setParentItem(panelArea);
                    break;
                }
            }
        }
    } else if (d->panels.count() <= d->panelAreas.count()) {
        for(int i = 0; i < d->panels.count(); ++i) {
            d->panels.at(i)->setParentItem(d->panelAreas.at(i));
        }
    }
}

void PanelConfiguration::save()
{
    QString configFile = KGlobal::dirs()->locateLocal("config", "kritasketchpanelsrc");
    QSettings panelConfig(configFile, QSettings::IniFormat);

    panelConfig.beginWriteArray("Panels");
    int index = 0;
    foreach(QDeclarativeItem* panel, d->panels) {
        panelConfig.setArrayIndex(index++);

        panelConfig.setValue("panel", panel->objectName());
        panelConfig.setValue("area", panel->parentItem()->objectName());
    }
    panelConfig.endArray();
}



