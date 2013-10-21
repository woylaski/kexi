/* This file is part of the KDE project
 * Copyright (C) 2012 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

#include "PresetModel.h"

#include <KoResourceServerAdapter.h>
#include <ui/kis_resource_server_provider.h>
#include <kis_view2.h>
#include <kis_canvas_resource_provider.h>
#include <kis_canvas2.h>
#include <kis_paintop_box.h>
#include <kis_factory2.h>
#include <image/brushengine/kis_paintop_preset.h>
#include <kis_paintop_factory.h>
#include <kis_paintop_registry.h>
#include <kis_node.h>
#include <kis_image.h>

class PresetModel::Private {
public:
    Private()
        : view(0)
    {
        rserver = KisResourceServerProvider::instance()->paintOpPresetServer();
    }

    KoResourceServer<KisPaintOpPreset> * rserver;
    QString currentPreset;
    KisView2* view;

    KisPaintOpPresetSP defaultPreset(const KoID& paintOp)
    {
        QString defaultName = paintOp.id() + ".kpp";
        QString path = KGlobal::mainComponent().dirs()->findResource("kis_defaultpresets", defaultName);

        KisPaintOpPresetSP preset = new KisPaintOpPreset(path);

        if (!preset->load())
            preset = KisPaintOpRegistry::instance()->defaultPreset(paintOp, view->image());

        Q_ASSERT(preset);
        Q_ASSERT(preset->valid());

        return preset;
    }

    void setCurrentPaintop(const KoID& paintop, KisPaintOpPresetSP preset)
    {
        preset = (!preset) ? defaultPreset(paintop) : preset;

        Q_ASSERT(preset && preset->settings());

        // handle the settings and expose it through a a simple QObject property
        //m_optionWidget->setConfiguration(preset->settings());

        preset->settings()->setNode(view->resourceProvider()->currentNode());

        KisPaintOpFactory* paintOp     = KisPaintOpRegistry::instance()->get(paintop.id());
        QString            pixFilename = KisFactory2::componentData().dirs()->findResource("kis_images", paintOp->pixmap());

        view->resourceProvider()->setPaintOpPreset(preset);
    }
};

PresetModel::PresetModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new Private)
{
    QHash<int, QByteArray> roles;
    roles[ImageRole] = "image";
    roles[TextRole] = "text";
    roles[NameRole] = "name";
    setRoleNames(roles);
}

PresetModel::~PresetModel()
{
    delete d;
}

int PresetModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return d->rserver->resources().count();
}

QVariant PresetModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (index.isValid())
    {
        switch(role)
        {
        case ImageRole:
            result = QString("image://presetthumb/%1").arg(index.row());
            break;
        case TextRole:
            result = d->rserver->resources().at(index.row())->name().replace(QLatin1String("_"), QLatin1String(" "));
            break;
        case NameRole:
            result = d->rserver->resources().at(index.row())->name();
            break;
        default:
            result = "";
            break;
        }
    }
    return result;
}

QVariant PresetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    QVariant result;
    if (section == 0)
    {
        switch(role)
        {
        case ImageRole:
            result = QString("Thumbnail");
            break;
        case TextRole:
            result = QString("Name");
            break;
        default:
            result = "";
            break;
        }
    }
    return result;
}

QObject* PresetModel::view() const
{
    return d->view;
}

void PresetModel::setView(QObject* newView)
{
    d->view = qobject_cast<KisView2*>( newView );
    if (d->view)
    {
        connect(d->view->canvasBase()->resourceManager(), SIGNAL(canvasResourceChanged(int, const QVariant&)),
                this, SLOT(resourceChanged(int, const QVariant&)));
    }
    emit viewChanged();
}

QString PresetModel::currentPreset() const
{
    return d->currentPreset;
}

void PresetModel::setCurrentPreset(QString presetName)
{
    activatePreset(nameToIndex(presetName));
    // not emitting here, as that happens when the resource changes... this is more
    // a polite request than an actual setter, due to the nature of the resource system.
}

int PresetModel::nameToIndex(QString presetName) const
{
    int index = 0;
    QList<KisPaintOpPreset*> resources = d->rserver->resources();
    for(int i = 0; i < resources.count(); ++i)
    {
        if (resources.at(i)->name() == presetName || resources.at(i)->name().replace(QLatin1String("_"), QLatin1String(" ")) == presetName)
        {
            index = i;
            break;
        }
    }
    return index;
}

void PresetModel::activatePreset(int index)
{
    if ( !d->view )
        return;

    QList<KisPaintOpPreset*> resources = d->rserver->resources();
    if (index >= 0 && index < resources.count())  {
        KisPaintOpPreset* preset = resources.at( index );
        d->setCurrentPaintop(preset->paintOp(), preset->clone());
    }
}

void PresetModel::resourceChanged(int /*key*/, const QVariant& /*v*/)
{
    if (d->view)
    {
        KisPaintOpPresetSP preset = d->view->canvasBase()->resourceManager()->resource(KisCanvasResourceProvider::CurrentPaintOpPreset).value<KisPaintOpPresetSP>();
        if (preset && d->currentPreset != preset->name())
        {
            d->currentPreset = preset->name();
            emit currentPresetChanged();
        }
    }
}

#include "PresetModel.moc"
