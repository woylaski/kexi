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

import QtQuick 1.1
import "../../components"

Item {
    id: base
    property QtObject configuration;
    function applyConfigurationChanges() {
        fullFilters.applyConfiguration(configuration);
    }
    function setProp(name, value) {
        if(configuration !== null) {
            configuration.writeProperty(name, value);
            base.applyConfigurationChanges();
        }
    }
    onConfigurationChanged: {
        blurAngle.value = configuration.readProperty("blurAngle");
        blurLength.value = configuration.readProperty("blurLength");
    }
    Column {
        anchors.fill: parent;
        RangeInput {
            id: blurAngle;
            width: parent.width;
            placeholder: "Angle";
            min: 0; max: 360; decimals: 0;
            onValueChanged: setProp("blurAngle", value);
        }
        RangeInput {
            id: blurLength;
            width: parent.width;
            placeholder: "Length";
            min: 0; max: 256; decimals: 0;
            useExponentialValue: true;
            onValueChanged: setProp("blurLength", value);
        }
    }
}
