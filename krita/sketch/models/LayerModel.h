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

#ifndef LAYERMODEL_H
#define LAYERMODEL_H

#include <QAbstractListModel>
#include <QImage>
#include <kis_types.h>

class LayerModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QObject* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(QObject* engine READ engine WRITE setEngine NOTIFY engineChanged)
    // This might seem a slightly odd position, but think of it as the thumbnail of all the currently visible layers merged down
    Q_PROPERTY(QString fullImageThumbUrl READ fullImageThumbUrl NOTIFY viewChanged);

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged);

    Q_PROPERTY(QString activeName READ activeName WRITE setActiveName NOTIFY activeNameChanged);
    Q_PROPERTY(QString activeType READ activeType NOTIFY activeTypeChanged);
    Q_PROPERTY(int activeCompositeOp READ activeCompositeOp WRITE setActiveCompositeOp NOTIFY activeCompositeOpChanged);
    Q_PROPERTY(int activeOpacity READ activeOpacity WRITE setActiveOpacity NOTIFY activeOpacityChanged);
    Q_PROPERTY(bool activeVisible READ activeVisible WRITE setActiveVisibile NOTIFY activeVisibleChanged);
    Q_PROPERTY(bool activeLocked READ activeLocked WRITE setActiveLocked NOTIFY activeLockedChanged);
    Q_PROPERTY(bool activeRChannelActive READ activeRChannelActive WRITE setActiveRChannelActive NOTIFY activeRChannelActiveChanged);
    Q_PROPERTY(bool activeGChannelActive READ activeGChannelActive WRITE setActiveGChannelActive NOTIFY activeGChannelActiveChanged);
    Q_PROPERTY(bool activeBChannelActive READ activeBChannelActive WRITE setActiveBChannelActive NOTIFY activeBChannelActiveChanged);
    Q_PROPERTY(bool activeAChannelActive READ activeAChannelActive WRITE setActiveAChannelActive NOTIFY activeAChannelActiveChanged);
    Q_PROPERTY(bool activeRChannelLocked READ activeRChannelLocked WRITE setActiveRChannelLocked NOTIFY activeRChannelLockedChanged);
    Q_PROPERTY(bool activeGChannelLocked READ activeGChannelLocked WRITE setActiveGChannelLocked NOTIFY activeGChannelLockedChanged);
    Q_PROPERTY(bool activeBChannelLocked READ activeBChannelLocked WRITE setActiveBChannelLocked NOTIFY activeBChannelLockedChanged);
    Q_PROPERTY(bool activeAChannelLocked READ activeAChannelLocked WRITE setActiveAChannelLocked NOTIFY activeAChannelLockedChanged);
    Q_PROPERTY(QObject* activeFilterConfig READ activeFilterConfig WRITE setActiveFilterConfig NOTIFY activeFilterConfigChanged);
public:
    enum LayerRoles {
        IconRole = Qt::UserRole + 1,
        NameRole,
        ActiveLayerRole,
        OpacityRole,
        PercentOpacityRole,
        VisibleRole,
        LockedRole,
        CompositeDetailsRole,
        FilterRole,
        ChildCountRole,
        DeepChildCountRole,
        DepthRole,
        PreviousItemDepthRole,
        NextItemDepthRole,
        CanMoveLeftRole,
        CanMoveRightRole,
        CanMoveUpRole,
        CanMoveDownRole
    };
    explicit LayerModel(QObject* parent = 0);
    virtual ~LayerModel();

    QObject* view() const;
    void setView(QObject* newView);
    QObject* engine() const;
    void setEngine(QObject* newEngine);
    QString fullImageThumbUrl() const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    Q_INVOKABLE virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    Q_INVOKABLE void setActive(int index);
    Q_INVOKABLE void moveUp();
    Q_INVOKABLE void moveDown();
    Q_INVOKABLE void moveLeft();
    Q_INVOKABLE void moveRight();
    void emitActiveChanges();
    Q_INVOKABLE void setOpacity(int index, float newOpacity);
    Q_INVOKABLE void setVisible(int index, bool newVisible);
    Q_INVOKABLE void setLocked(int index, bool newLocked);
    QImage layerThumbnail(QString layerID) const;

    Q_INVOKABLE void deleteCurrentLayer();
    Q_INVOKABLE void deleteLayer(int index);
    Q_INVOKABLE void addLayer(int layerType);

    QString activeName() const;
    void setActiveName(QString newName);
    QString activeType() const;
    int activeCompositeOp() const;
    void setActiveCompositeOp(int newOp);
    int activeOpacity() const;
    void setActiveOpacity(int newOpacity);
    bool activeVisible() const;
    void setActiveVisibile(bool newVisible);
    bool activeLocked() const;
    void setActiveLocked(bool newLocked);
    bool activeRChannelActive() const;
    void setActiveRChannelActive(bool newActive);
    bool activeGChannelActive() const;
    void setActiveGChannelActive(bool newActive);
    bool activeBChannelActive() const;
    void setActiveBChannelActive(bool newActive);
    bool activeAChannelActive() const;
    void setActiveAChannelActive(bool newActive);
    bool activeRChannelLocked() const;
    void setActiveRChannelLocked(bool newLocked);
    bool activeGChannelLocked() const;
    void setActiveGChannelLocked(bool newLocked);
    bool activeBChannelLocked() const;
    void setActiveBChannelLocked(bool newLocked);
    bool activeAChannelLocked() const;
    void setActiveAChannelLocked(bool newLocked);
    QObject* activeFilterConfig() const;
    void setActiveFilterConfig(QObject* newConfig);
Q_SIGNALS:
    void viewChanged();
    void engineChanged();
    void countChanged();

    void activeNameChanged();
    void activeTypeChanged();
    void activeCompositeOpChanged();
    void activeOpacityChanged();
    void activeVisibleChanged();
    void activeLockedChanged();
    void activeRChannelActiveChanged();
    void activeGChannelActiveChanged();
    void activeBChannelActiveChanged();
    void activeAChannelActiveChanged();
    void activeRChannelLockedChanged();
    void activeGChannelLockedChanged();
    void activeBChannelLockedChanged();
    void activeAChannelLockedChanged();
    void activeFilterConfigChanged();

private slots:
    void source_rowsAboutToBeInserted(QModelIndex, int, int);
    void source_rowsAboutToBeRemoved(QModelIndex, int, int);
    void source_rowsInserted(QModelIndex, int, int);
    void source_rowsRemoved(QModelIndex, int, int);
    void source_dataChanged(QModelIndex, QModelIndex);
    void source_modelReset();
    void currentNodeChanged(KisNodeSP newActiveNode);
    void notifyImageDeleted();
    void nodeChanged(KisNodeSP node);
    void imageChanged();
    void imageHasChanged();
    void aboutToRemoveNode(KisNodeSP node);
    void updateActiveLayerWithNewFilterConfig();

private:
    class Private;
    Private* d;
};

#endif // LAYERMODEL_H
