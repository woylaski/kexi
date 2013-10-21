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

Item {
    id: base;

    property bool enabled: true;
    property alias placeholder: textField.placeholder;
    property real value: 0;
    property real min: 0;
    property real max: 1000;
    property int decimals: 2;
    property alias useExponentialValue: valueSlider.useExponentialValue;

    height: textField.height + valueSlider.height;

    onMinChanged: d.fixHandle();
    onMaxChanged: d.fixHandle();
    onValueChanged: {
        if (decimals === 0) {
            if (value !== Math.round(value))
            {
                value = Math.round(value);
                return;
            }
        }
        else if (value * Math.pow(10, decimals) !== Math.round(value * Math.pow(10, decimals))) {
            value = Math.round(value * Math.pow(10, decimals)) / Math.pow(10, decimals);
            return;
        }
        if (value < min) {
            value = min;
            return;
        }
        if (value > max) {
            value = max;
            return;
        }
        if (textField.text != value) {
            textField.text = value.toFixed(decimals);
        }
        if (useExponentialValue) {
             if (valueSlider.exponentialValue !== value) {
                 valueSlider.exponentialValue = ( (value - min) / (max - min) ) * 100;
             }
        }
        else {
            if (valueSlider.value !== value) {
                valueSlider.value = ( (value - min) / (max - min) ) * 100;
            }
        }
    }

    PanelTextField {
        id: textField
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
        }
        onFocusLost: value = text;
        onAccepted: value = text;
        numeric: true;
    }
    Slider {
        id: valueSlider;
        anchors {
            top: textField.bottom;
            left: parent.left;
            right: parent.right;
            leftMargin: Constants.DefaultMargin;
            rightMargin: Constants.DefaultMargin;
        }
        highPrecision: true;
        onExponentialValueChanged: {
            if (useExponentialValue) {
                base.value = base.min + ((exponentialValue / 100) * (base.max - base.min))
            }
        }
        onValueChanged: {
            if (!useExponentialValue) {
                base.value = base.min + ((value / 100) * (base.max - base.min));
            }
        }
    }
    QtObject {
        id: d;
        function fixHandle() {
            var currentVal = base.value;
            // Set the value to something it isn't currently
            base.value = base.min;
            base.value = base.max;
            // Set it back to what it was
            base.value = currentVal;
        }
    }
}
