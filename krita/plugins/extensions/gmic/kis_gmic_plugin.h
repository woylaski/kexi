/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#ifndef _KIS_GMIC_PLUGIN_H_
#define _KIS_GMIC_PLUGIN_H_

#include <QVariant>

#include <kis_view_plugin.h>
#include <kis_gmic_filter_settings.h>
#include "kis_gmic_parser.h"
#include <kis_types.h>

class KisGmicApplicator;
class KisGmicWidget;

class KisGmicPlugin : public KisViewPlugin
{
    Q_OBJECT
public:
    KisGmicPlugin(QObject *parent, const QVariantList &);
    virtual ~KisGmicPlugin();

private:
    KisGmicWidget * m_gmicWidget;
    KisGmicApplicator * m_gmicApplicator;
    QString m_gmicDefinitionFilePath;

private slots:
    void slotGmic();
    void slotApplyGmicCommand(KisGmicFilterSetting* setting);
    void slotClose();
};

#endif
