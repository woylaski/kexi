/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2008,2009 Lukáš Tvrdý <lukast.dev@gmail.com>
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
#ifndef KIS_DEFORM_PAINTOP_SETTINGS_H_
#define KIS_DEFORM_PAINTOP_SETTINGS_H_

class KisDeformPaintOpSettingsWidget;

#include <kis_paintop_settings.h>
#include <kis_types.h>
#include <kis_outline_generation_policy.h>

class KisDeformPaintOpSettings : public KisOutlineGenerationPolicy<KisPaintOpSettings>
{

public:
    KisDeformPaintOpSettings();

    QPainterPath brushOutline(const KisPaintInformation &info, OutlineMode mode) const;

    bool paintIncremental();
    bool isAirbrushing() const;
    int rate() const;

private:
    KisDeformPaintOpSettingsWidget* m_options;
};
#endif
