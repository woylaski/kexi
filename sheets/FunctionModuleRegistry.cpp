/* This file is part of the KDE project
   Copyright 2008 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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
   Boston, MA 02110-1301, USA.
*/

#include "FunctionModuleRegistry.h"

#include "Function.h"
#include "FunctionRepository.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#ifndef SHEETS_NO_PLUGINMODULES
#include <kplugininfo.h>
#include <kservicetypetrader.h>
#else
#include "functions/BitOpsModule.h"
#include "functions/ConversionModule.h"
#include "functions/DatabaseModule.h"
#include "functions/DateTimeModule.h"
#include "functions/EngineeringModule.h"
#include "functions/FinancialModule.h"
#include "functions/InformationModule.h"
#include "functions/LogicModule.h"
#include "functions/MathModule.h"
#include "functions/ReferenceModule.h"
#include "functions/StatisticalModule.h"
#include "functions/TextModule.h"
#include "functions/TrigonometryModule.h"
#endif

using namespace Calligra::Sheets;

class FunctionModuleRegistry::Private
{
public:
    void registerFunctionModule(FunctionModule* module);
    void removeFunctionModule(FunctionModule* module);

public:
    bool repositoryInitialized;
};

void FunctionModuleRegistry::Private::registerFunctionModule(FunctionModule* module)
{
    const QList<QSharedPointer<Function> > functions = module->functions();
    for (int i = 0; i < functions.count(); ++i) {
        FunctionRepository::self()->add(functions[i]);
    }
    Q_ASSERT(!module->descriptionFileName().isEmpty());
    const KStandardDirs* dirs = KGlobal::activeComponent().dirs();
    const QString fileName = dirs->findResource("functions", module->descriptionFileName());
    if (fileName.isEmpty()) {
        kDebug(36002) << module->descriptionFileName() << "not found.";
        return;
    }
    FunctionRepository::self()->loadFunctionDescriptions(fileName);
}

void FunctionModuleRegistry::Private::removeFunctionModule(FunctionModule* module)
{
    const QList<QSharedPointer<Function> > functions = module->functions();
    for (int i = 0; i < functions.count(); ++i) {
        FunctionRepository::self()->remove(functions[i]);
    }
}


FunctionModuleRegistry::FunctionModuleRegistry()
        : d(new Private)
{
    d->repositoryInitialized = false;
}

FunctionModuleRegistry::~FunctionModuleRegistry()
{
    foreach(const QString &id, keys()) {
        get(id)->deleteLater();
    }
    qDeleteAll(doubleEntries());
    delete d;
}

FunctionModuleRegistry* FunctionModuleRegistry::instance()
{
    K_GLOBAL_STATIC(FunctionModuleRegistry, s_instance)
    return s_instance;
}

void FunctionModuleRegistry::loadFunctionModules()
{
#ifndef SHEETS_NO_PLUGINMODULES
    const quint32 minKSpreadVersion = CALLIGRA_MAKE_VERSION(2, 1, 0);
    const QString serviceType = QLatin1String("CalligraSheets/Plugin");
    const QString query = QLatin1String("([X-CalligraSheets-InterfaceVersion] == 0) and "
                                        "([X-KDE-PluginInfo-Category] == 'FunctionModule')");
    const KService::List offers = KServiceTypeTrader::self()->query(serviceType, query);
    const KConfigGroup moduleGroup = KGlobal::config()->group("Plugins");
    const KPluginInfo::List pluginInfos = KPluginInfo::fromServices(offers, moduleGroup);
    kDebug(36002) << pluginInfos.count() << "function modules found.";
    foreach(KPluginInfo pluginInfo, pluginInfos) {
        pluginInfo.load(); // load activation state
        KPluginLoader loader(*pluginInfo.service());
        // Let's be paranoid: do not believe the service type.
        if (loader.pluginVersion() < minKSpreadVersion) {
            kDebug(36002) << pluginInfo.name()
            << "was built against Caligra Sheets" << loader.pluginVersion()
            << "; required version >=" << minKSpreadVersion;
            continue;
        }
        if (pluginInfo.isPluginEnabled() && !contains(pluginInfo.pluginName())) {
            // Plugin enabled, but not registered. Add it.
            KPluginFactory* const factory = loader.factory();
            if (!factory) {
                kDebug(36002) << "Unable to create plugin factory for" << pluginInfo.name();
                continue;
            }
            FunctionModule* const module = factory->create<FunctionModule>();
            if (!module) {
                kDebug(36002) << "Unable to create function module for" << pluginInfo.name();
                continue;
            }
            add(pluginInfo.pluginName(), module);

            // Delays the function registration until the user needs one.
            if (d->repositoryInitialized) {
                d->registerFunctionModule(module);
            }
        } else if (!pluginInfo.isPluginEnabled() && contains(pluginInfo.pluginName())) {
            // Plugin disabled, but registered. Remove it.
            FunctionModule* const module = get(pluginInfo.pluginName());
            // Delay the function registration until the user needs one.
            if (d->repositoryInitialized) {
                d->removeFunctionModule(module);
            }
            remove(pluginInfo.pluginName());
            if (module->isRemovable()) {
                delete module;
                delete loader.factory();
                loader.unload();
            } else {
                // Put it back in.
                add(pluginInfo.pluginName(), module);
                // Delay the function registration until the user needs one.
                if (d->repositoryInitialized) {
                    d->registerFunctionModule(module);
                }
            }
        }
    }
#else
    QList<FunctionModule*> modules;
    QObject *parent = 0;

    modules << new BitOpsModule(parent);
    modules << new ConversionModule(parent);
    modules << new DatabaseModule(parent);
    modules << new DateTimeModule(parent);
    modules << new EngineeringModule(parent);
    modules << new FinancialModule(parent);
    modules << new InformationModule(parent);
    modules << new LogicModule(parent);
    modules << new MathModule(parent);
    modules << new ReferenceModule(parent);
    modules << new StatisticalModule(parent);
    modules << new TextModule(parent);
    modules << new TrigonometryModule(parent);

    Q_FOREACH(FunctionModule* module, modules) {
        add(module->id(), module);
        d->registerFunctionModule(module);
    }
#endif
}

void FunctionModuleRegistry::registerFunctions()
{
    d->repositoryInitialized = true;
    const QList<FunctionModule*> modules = values();
    for (int i = 0; i < modules.count(); ++i) {
        d->registerFunctionModule(modules[i]);
    }
}
