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
import org.krita.sketch 1.0 as Krita
import "../../components"

Column {
    id: base;

    property bool fullView: true;

    function apply() {
        toolManager.currentTool.applyTransform();
    }

    height: buttonRow.height + freeModeOptions.height + Constants.DefaultMargin * 5
    anchors.margins: Constants.DefaultMargin;
    spacing: Constants.DefaultMargin;

    Item {
        height: Constants.DefaultMargin;
        width: parent.width;
    }
    Row {
        id: buttonRow;
        anchors.horizontalCenter: parent.horizontalCenter;
        spacing: Constants.DefaultMargin;

        Button {
            id: freeModeButton;

            width: base.width * 0.4;
            height: textSize + Constants.DefaultMargin * 2

            textColor: "black";
            color: "#63ffffff";

            border.width: 1;
            border.color: "silver";

            radius: Constants.DefaultMargin;

            checkable: true;
            checked: true;
            onCheckedChanged: {
                if (checked) {
                    warpModeButton.checked = false;
                    toolManager.currentTool.transformMode = 0;
                } else if ( !warpModeButton.checked ) {
                    checked = true;
                }
            }

            highlight: true;
            highlightColor: "#aaffffff";

            text: "Free"
        }

        Button {
            id: warpModeButton;

            width: base.width * 0.4;
            height: textSize + Constants.DefaultMargin * 2

            textColor: "black";
            color: "#63ffffff";

            border.width: 1;
            border.color: "silver";

            radius: Constants.DefaultMargin;

            checkable: true;
            onCheckedChanged: {
                if (checked) {
                    freeModeButton.checked = false;
                    toolManager.currentTool.transformMode = 1;
                } else if ( !freeModeButton.checked ) {
                    checked = true;
                }
            }

            highlight: true;
            highlightColor: "#aaffffff";

            text: "Warp"
        }
    }

    Item {
        width: parent.width;
        height: freeModeOptions.height;

        Column {
            id: freeModeOptions;

            width: parent.width;

            opacity: freeModeButton.checked ? 1.0 : 0.0;
            Behavior on opacity { NumberAnimation { } }

            Label { height: font.pixelSize + Constants.DefaultMargin; text: "Translation" }

            PanelTextField { id: translateX; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "X" }
            PanelTextField { id: translateY; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Y" }

            Label { height: font.pixelSize + Constants.DefaultMargin; text: "Rotation" }

//             ExpandingListView {
//                 width: parent.width;
//
//                 model: ListModel {
//                     ListElement { text: "Top Left"; }
//                     ListElement { text: "Top Center"; }
//                     ListElement { text: "Top Right"; }
//                     ListElement { text: "Middle Left"; }
//                     ListElement { text: "Center"; }
//                     ListElement { text: "Middle Right"; }
//                     ListElement { text: "Bottom Left"; }
//                     ListElement { text: "Bottom Center"; }
//                     ListElement { text: "Bottom Right"; }
//                 }
//             }

            PanelTextField { id: rotateX; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "X" }
            PanelTextField { id: rotateY; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Y" }
            PanelTextField { id: rotateZ; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Z" }

            Label { height: font.pixelSize + Constants.DefaultMargin; text: "Scaling" }

            PanelTextField { id: scaleX; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "X" }
            PanelTextField { id: scaleY; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Y" }

            Label { height: font.pixelSize + Constants.DefaultMargin; text: "Shear" }

            PanelTextField { id: shearX; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "X" }
            PanelTextField { id: shearY; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Y" }

            //Button { text: "Update"; onClicked: parent.updateTransform(); }

//             ExpandingListView {
//                 model: ListModel {
//                     ListElement { text: "Box"; }
//                     ListElement { text: "Bilinear"; }
//                     ListElement { text: "Bicubic"; }
//                 }
//             }

            Connections {
                target: toolManager.currentTool;
                ignoreUnknownSignals: true;

                onTransformModeChanged: { }
                onFreeTransformChanged: {
                    freeModeOptions.preventUpdate = true;
                    freeModeOptions.updateFreeTransformText();
                    freeModeOptions.preventUpdate = false;
                }
            }

            property bool preventUpdate: false;
            property bool preventUpdateText: false;

            function updateTransform() {
                if ( preventUpdate )
                    return;

                toolManager.currentTool.translateX = parseFloat(translateX.text);
                toolManager.currentTool.translateY = parseFloat(translateY.text);

                toolManager.currentTool.rotateX = parseFloat(rotateX.text);
                toolManager.currentTool.rotateY = parseFloat(rotateY.text);
                toolManager.currentTool.rotateZ = parseFloat(rotateZ.text);

                toolManager.currentTool.scaleX = parseFloat(scaleX.text) * 100;
                toolManager.currentTool.scaleY = parseFloat(scaleY.text) * 100;

                toolManager.currentTool.shearX = parseFloat(shearX.text);
                toolManager.currentTool.shearY = parseFloat(shearY.text);
            }

            function updateFreeTransformText() {
                if (preventUpdateText)
                    return;

                translateX.text = toolManager.currentTool.translateX;
                translateY.text = toolManager.currentTool.translateY;

                rotateX.text = toolManager.currentTool.rotateX;
                rotateY.text = toolManager.currentTool.rotateY;
                rotateZ.text = toolManager.currentTool.rotateZ;

                scaleX.text = toolManager.currentTool.scaleX;
                scaleY.text = toolManager.currentTool.scaleY;

                shearX.text = toolManager.currentTool.shearX;
                shearY.text = toolManager.currentTool.shearY;
            }

            Component.onCompleted: {
                preventUpdate = true;
                updateFreeTransformText();
                preventUpdate = false;
            }
        }

        Column {
            id: warpModeOptions;

            width: parent.width;
            spacing: Constants.DefaultMargin;

            opacity: warpModeButton.checked ? 1.0 : 0.0;
            Behavior on opacity { NumberAnimation { } }

            ExpandingListView {
                id: warpTypeCombo;

                width: parent.width;
                expandedHeight: Constants.GridHeight * 2

                onCurrentIndexChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; }

                model: ListModel {
                    ListElement { text: "Rigid"; }
                    ListElement { text: "Affine"; }
                    ListElement { text: "Similitude"; }
                }
            }

            PanelTextField { id: warpFlexibilityField; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Flexibility"; }
            PanelTextField { id: warpDensityField; onTextChanged: { parent.preventUpdateText = true; parent.updateTransform(); parent.preventUpdateText = false; } placeholder: "Density"; }

            property bool preventUpdate: false;
            property bool preventUpdateText: false;

            function updateTransform() {
                if (preventUpdate)
                    return;

                toolManager.currentTool.warpType = warpTypeCombo.currentIndex;
                toolManager.currentTool.warpFlexibility = parseFloat(warpFlexibilityField.text);
                toolManager.currentTool.warpPointDensity = parseInt(warpDensityField.text);
            }

            function updateTransformText() {
                if (preventUpdateText)
                    return;

                warpTypeCombo.currentIndex = toolManager.currentTool.warpType;
                warpFlexibilityField.text = toolManager.currentTool.warpFlexibility;
                warpDensityField.text = toolManager.currentTool.warpPointDensity;
            }

            Component.onCompleted: {
                preventUpdate = true;
                updateTransformText();
                preventUpdate = false;
            }
        }
    }
}
