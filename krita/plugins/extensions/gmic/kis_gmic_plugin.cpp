/*
 * This file is part of the KDE project
 *
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_gmic_plugin.h"

#include <QApplication>

#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kglobal.h>

#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kis_debug.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>
#include <QTime>

#include <kis_view2.h>
#include <kis_action.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_layer.h>
#include <kis_paint_layer.h>
#include "kis_statusbar.h"
#include "widgets/kis_progress_widget.h"

#include <KoProgressUpdater.h>
#include <KoUpdater.h>

#include "gmic.h"
#include "kis_gmic_parser.h"
#include "Component.h"
#include "kis_gmic_filter_model.h"
#include "kis_gmic_widget.h"

#include "kis_gmic_blacklister.h"
#include "kis_input_output_mapper.h"
#include "kis_gmic_simple_convertor.h"
#include "kis_gmic_applicator.h"


K_PLUGIN_FACTORY(KisGmicPluginFactory, registerPlugin<KisGmicPlugin>();)
K_EXPORT_PLUGIN(KisGmicPluginFactory("krita"))

KisGmicPlugin::KisGmicPlugin(QObject *parent, const QVariantList &)
        : KisViewPlugin(parent, "kritaplugins/gmic.rc"),m_gmicWidget(0)
{
    KisAction *action  = new KisAction(i18n("Apply G'Mic Action..."), this);
    action->setActivationFlags(KisAction::ACTIVE_LAYER);
    action->setActivationConditions(KisAction::ACTIVE_NODE_EDITABLE);
    addAction("gmic", action);

    QString standardSettings("gmic_def.gmic");
    KGlobal::dirs()->addResourceType("gmic_definitions", "data", "krita/gmic/");
    m_gmicDefinitionFilePath = KGlobal::mainComponent().dirs()->findResource("gmic_definitions", standardSettings);

    connect(action, SIGNAL(triggered()), this, SLOT(slotGmic()));
}

KisGmicPlugin::~KisGmicPlugin()
{
    delete m_gmicWidget;
}

void KisGmicPlugin::slotGmic()
{
    KisImageWSP image = m_view->image();
    if (!image) return;

    KisLayerSP layer = m_view->activeLayer();
    if (!layer) return;

    if (m_gmicWidget)
    {
        // restart here?
        slotClose();
    }

    KisGmicParser parser(m_gmicDefinitionFilePath);
    Component * root = parser.createFilterTree();
    KisGmicFilterModel * model = new KisGmicFilterModel(root); // filter mode takes owner ship

    KisGmicBlacklister * blacklister = new KisGmicBlacklister(m_gmicDefinitionFilePath + ".blacklist");
    model->setBlacklister(blacklister);

    m_gmicWidget = new KisGmicWidget(model);
    m_gmicApplicator = new KisGmicApplicator();
#ifdef Q_WS_WIN
    // On windows, gmic needs a humongous stack for its parser
    m_gmicApplicator->setStackSize(20 * 1024 * 1024);
#endif


    // apply
    connect(m_gmicWidget, SIGNAL(sigApplyCommand(KisGmicFilterSetting*)),this, SLOT(slotApplyGmicCommand(KisGmicFilterSetting*)));
    // cancel
    connect(m_gmicWidget, SIGNAL(sigClose()),this, SLOT(slotClose()));

    m_gmicWidget->show();
}


void KisGmicPlugin::slotApplyGmicCommand(KisGmicFilterSetting* setting)
{
    QString actionName;
    KisNodeSP node;

    if (setting->isBlacklisted())
    {
        KMessageBox::sorry(m_gmicWidget, i18n("Sorry, this filter is crashing Krita and is turned off."), i18n("Krita"));
        return;
    }

    KisInputOutputMapper mapper(m_view->image(), m_view->activeNode());
    KisNodeListSP kritaNodes = mapper.inputNodes(setting->inputLayerMode());

    if (kritaNodes->isEmpty())
    {
        KMessageBox::sorry(m_gmicWidget, i18n("Sorry, this input mode is not implemented"), i18n("Krita"));
        return;
    }
    else
    {
        actionName = i18n("Gmic filter");
        node = m_view->image()->root();
    }

    if (setting->outputMode() != IN_PLACE)
    {
        KMessageBox::sorry(m_gmicWidget,QString("Sorry, this output mode is not implemented"),"Krita");
        return;
    }

    QTime myTimer;
    myTimer.start();
    qApp->setOverrideCursor(Qt::WaitCursor);

    m_gmicApplicator->setProperties(m_view->image(), node, actionName, kritaNodes, setting->gmicCommand());
    m_gmicApplicator->start();
    m_gmicApplicator->wait();
    m_view->image()->waitForDone();
    qApp->restoreOverrideCursor();

    double seconds = myTimer.elapsed() * 0.001;
    // temporary feedback
    m_gmicWidget->setWindowTitle(QString("Filtering took ") + QString::number(seconds) + QString(" seconds"));
}

void KisGmicPlugin::slotClose()
{
    bool result = m_gmicWidget->close();
    if (!result)
    {
        dbgPlugins << "Windows was not closed?";
    }
    else
    {
        // close event deletes widget
        m_gmicWidget = 0;
        delete m_gmicApplicator;
        m_gmicApplicator = 0;
    }
}



#include "kis_gmic_plugin.moc"
