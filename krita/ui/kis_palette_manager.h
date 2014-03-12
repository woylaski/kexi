/* This file is part of the KDE project
   Copyright 2009 Vera Lukman <shicmap@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

*/

#ifndef KIS_PALETTE_MANAGER_H
#define KIS_PALETTE_MANAGER_H

#include <kis_paintop_preset.h>
#include <kis_types.h>
#include <kdialog.h>

class KoID;
class QStringListModel;
class QListView;
class QLabel;
class QPushButton;
class KoFavoriteResourceManager;
class KisPaintopBox;
class KisPresetChooser;

class KisPaletteManager : public KDialog
{
    Q_OBJECT

public:
    KisPaletteManager(KoFavoriteResourceManager*, KisPaintopBox*);
    ~KisPaletteManager();

    virtual void showEvent(QShowEvent* );

    void updatePaletteView();

private slots:
    void slotUpdateAddButton();
    void slotEnableRemoveButton();
    void slotDeleteBrush();
    void slotAddBrush();
    void slotThumbnailMode();
    void slotDetailMode();

private:
    QPushButton *m_saveButton;
    QPushButton *m_removeButton;
    KoFavoriteResourceManager *m_resourceManager;
    KisPresetChooser* m_allPresetsView;
    KisPresetChooser* m_palettePresetsView;
};



#endif // PALETTEMANAGER_H
