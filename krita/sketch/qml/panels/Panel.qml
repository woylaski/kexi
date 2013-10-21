/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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
import org.krita.sketch 1.0
import org.krita.draganddrop 1.0 as DnD
import "../components"

Item {
    id: base;

    property bool roundTop: false;
    property color panelColor: Constants.Theme.MainColor;
    property color textColor: "white";
    property string name;

    property alias actions: actionsLayout.children;
    property alias peekContents: peek.children;
    property alias fullContents: full.children;

    property real editWidth: Constants.GridWidth * 4;

    property Item page;
    property Item lastArea;

    signal collapsed();
    signal peek();
    signal full();
    signal dragStarted();
    signal drop(int action);

    state: "collapsed";

    Item {
        id: fill;

        width: parent.width;
        height: parent.height;

        Rectangle {
            id: rectangle3
            anchors.fill: parent;
            color: "transparent";// base.panelColor;
            clip: true;

            MouseArea {
                // This mouse area blocks any mouse click from passing through the panel. We need this to ensure we don't accidentally pass
                // any clicks to the collapsing area, or to the canvas, by accident.
                anchors.fill: parent;
                // This will always work, and never do anything - but we need some kind of processed thing in here to activate the mouse area
                onClicked: parent.focus = true;
            }
            SimpleTouchArea {
                // As above, but for touch events
                anchors.fill: parent;
                onTouched: parent.focus = true;
            }

            Rectangle {
                id: background;
                anchors {
                    top: parent.top;
                    bottom: parent.bottom;
                    left: parent.left;
                    right: parent.right;
                    topMargin: Constants.DefaultMargin;
                    bottomMargin: Constants.DefaultMargin;
                }
                color: base.panelColor;
                Rectangle {
                    id: rectangle4
                    anchors.fill: parent;
                    color: "#ffffff"
                    opacity: 0.630
                }
            }

            Item {
                id: peek;
                clip: true;
                anchors.top: parent.top;
                anchors.bottom: header.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
            }

            Item {
                id: full;
                clip: true;
                anchors.top: header.bottom;
                anchors.bottom: footer.top;
                anchors.left: parent.left;
                anchors.right: parent.right;
            }

            Item {
                id: header;

                anchors.left: parent.left
                anchors.right: parent.right;
                height: Constants.GridHeight;

                Rectangle {
                    id: rectangle1
                    anchors.fill: parent;
                    color: base.panelColor;
                    radius: Constants.DefaultMargin;
                }

                Rectangle {
                    id: headerCornerFill;
                    anchors.bottom: parent.bottom;
                    anchors.left: parent.left;
                    anchors.right: parent.right;
                    height: Constants.DefaultMargin;
                    color: base.panelColor;
                }

                DnD.DragArea {
                    anchors.fill: parent;
                    delegate: base.dragDelegate;
                    source: base;
                    enabled: base.state === "full";

                    onDragStarted: {
                        handle.opacity = 1;
                        handle.dragStarted();
                    }
                    onDrop: {
                        if (action == Qt.IgnoreAction) {
                            handle.opacity = 0;
                        }
                        handle.drop(action);
                    }

                    MouseArea {

                    }
                }

                Flow {
                    id: actionsLayout;
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        left: parent.left;
                        right: parent.right;
                    }
                    height: Constants.ToolbarButtonSize;
                }
            }

            Image {
                anchors.top: header.bottom;
                anchors.left: header.left;
                anchors.right: header.right;

                source: "../images/shadow-smooth.png";
            }

            Item {
                id: footer;

                anchors.bottom: parent.bottom;
                width: parent.width;
                height: Constants.GridHeight;
                clip: true;

                Rectangle {
                    id: rectanglefoot
                    color: base.panelColor;
                    width: parent.width;
                    height: parent.height + Constants.DefaultMargin;
                    y: -Constants.DefaultMargin;
                    radius: Constants.DefaultMargin;
                }

                Rectangle {
                    anchors.fill: parent;
                    color: "transparent";//base.panelColor;

                    Label {
                        anchors.left: parent.left;
                        anchors.leftMargin: 16;
                        anchors.baseline: parent.bottom;
                        anchors.baselineOffset: -16;

                        text: base.name;
                        color: base.textColor;
                    }
                }

                DnD.DragArea {
                    anchors.fill: parent;
                    delegate: base.dragDelegate;
                    source: base;

                    onDragStarted: {
                        handle.opacity = 1;
                        handle.dragStarted();
                    }
                    onDrop: {
                        if (action == Qt.IgnoreAction) {
                            handle.opacity = 0;
                        }
                        handle.drop(action);
                    }

                    MouseArea {

                    }
                }
            }

            Image {
                anchors.bottom: footer.top;
                anchors.left: footer.left;
                anchors.right: footer.right;

                rotation: 180;

                source: "../images/shadow-smooth.png";
            }
        }
    }

    Item {
        id: handle;

        anchors.top: fill.bottom;
        anchors.topMargin: Constants.GridHeight / 4;
        anchors.left: fill.left;
        anchors.leftMargin: Constants.GridWidth / 2;

        Behavior on y { id: yHandleAnim; enabled: false; NumberAnimation { onRunningChanged: handle.fixParent(); } }
        Behavior on x { id: xHandleAnim; enabled: false; NumberAnimation { onRunningChanged: handle.fixParent(); } }

        width: 0;
        height: 0;
        opacity: 0;

        property bool dragging: false;

        function fixParent() {
            if (!handleDragArea.dragging && !xHandleAnim.animation.running && !yHandleAnim.animation.running) {
                xHandleAnim.enabled = false;
                yHandleAnim.enabled = false;
                handle.parent = base;
                handle.anchors.top = fill.bottom;
                handle.anchors.left = fill.left;
            }
        }

        function dragStarted() {
            base.dragStarted();

            handle.anchors.top = undefined;
            handle.anchors.left = undefined;
            handle.parent = base.page;
            Krita.MouseTracker.addItem(handle);
            handle.dragging = true;
        }

        function drop(action) {
            base.drop(action);

            Krita.MouseTracker.removeItem(handle);

            xHandleAnim.enabled = true;
            yHandleAnim.enabled = true;
            dragging = false;

            var handlePos = base.mapToItem(base.page, 0, 0);
            handle.x = handlePos.x + Constants.GridWidth / 2;
            handle.y = handlePos.y + Constants.GridHeight / 4;
        }

        Rectangle {
            visible: (base.state === "collapsed") ? !base.roundTop : true;
            anchors {
                bottom: parent.top;
                left: handleBackground.left;//parent.horizontalCenter;
                //leftMargin: -handle.anchors.leftMargin;
            }
            color: base.panelColor;
            radius: 0

            width: handleBackground.width //handle.anchors.leftMargin * 2;
            height: handle.anchors.topMargin + 1
        }

        Rectangle {
            id: handleBackground
            anchors {
                top: parent.top;
                topMargin: -handle.anchors.topMargin;
                left: parent.horizontalCenter;
                leftMargin: -handle.anchors.leftMargin;
            }

            width: handle.anchors.leftMargin * 2
            height: handle.anchors.topMargin * 2
            color: base.panelColor
            radius: 8

            Label {
                id: handleLabel;

                anchors.centerIn: parent;

                text: base.name;
                color: base.textColor;

                font.pixelSize: Constants.DefaultFontSize;
            }
        }

        DnD.DragArea {
            id: handleDragArea;
            anchors.horizontalCenter: parent.horizontalCenter;
            anchors.top: parent.top;
            anchors.topMargin: -Constants.GridHeight * 0.25;

            width: Constants.GridWidth - 8;
            height: Constants.GridHeight * 0.75;

            source: base;

            onDragStarted: {
                base.lastArea = base.parent;
                handle.dragStarted();
            }
            onDrop: {
                handle.drop(action);
            }

            MouseArea {
                anchors.fill: parent;
                onClicked: base.state = base.state == "peek" ? "collapsed" : "peek";
            }
            SimpleTouchArea {
                anchors.fill: parent;
                onTouched: base.state = base.state == "peek" ? "collapsed" : "peek";
            }
        }
    }

    states: [
        State {
            name: "collapsed";

            PropertyChanges { target: base; width: Constants.GridWidth; }
            PropertyChanges { target: fill; opacity: 0; height: 0; }
            PropertyChanges { target: handle; opacity: 1; }
            PropertyChanges { target: background; anchors.topMargin: 0; }
        },
        State {
            name: "peek";

            PropertyChanges { target: base; width: Constants.IsLandscape ? Constants.GridWidth * 4 : Constants.GridWidth * 2; }
            PropertyChanges { target: fill; height: Constants.GridHeight * 3.75; }
            PropertyChanges { target: handle; opacity: 1; anchors.leftMargin: Constants.GridWidth / 2 - 4; }
            PropertyChanges { target: peek; opacity: 1; }
            PropertyChanges { target: full; opacity: 0; }
            AnchorChanges { target: header; anchors.bottom: rectangle3.bottom }
            PropertyChanges { target: footer; opacity: 0; }
            PropertyChanges { target: background; anchors.topMargin: 0; }
            PropertyChanges { target: headerCornerFill; height: Constants.GridHeight; }
        },
        State {
            name: "full";
            PropertyChanges { target: peek; opacity: 0; }
            PropertyChanges { target: full; opacity: 1; }
        },
        State {
            name: "edit";
            PropertyChanges { target: peek; opacity: 0; }
            PropertyChanges { target: full; opacity: 1; }
            PropertyChanges { target: base; width: base.editWidth; }
        }
    ]

    transitions: [
        Transition {
            from: "collapsed";
            to: "peek";

            SequentialAnimation {
                ScriptAction { script: base.peek(); }
                AnchorAnimation { targets: [ header ] ; duration: 0; }
                PropertyAction { targets: [ header, footer ]; properties: "height,width,opacity" }
                PropertyAction { targets: [ base ]; properties: "width"; }
                NumberAnimation { targets: [ base, fill, handle, peek, full ]; properties: "height,opacity"; duration: 150; }
            }
        },
        Transition {
            from: "peek";
            to: "collapsed";

            SequentialAnimation {
                NumberAnimation { targets: [ base, fill, handle, peek, full ]; properties: "height,opacity"; duration: 150; }
                AnchorAnimation { targets: [ header ] ; duration: 0; }
                PropertyAction { targets: [ base ]; properties: "width"; }
                PropertyAction { targets: [ header, footer ]; properties: "height,width,opacity" }
                ScriptAction { script: base.collapsed(); }
            }
        },
        Transition {
            from: "peek";
            to: "full";
            reversible: true;

            NumberAnimation { properties: "height,width,opacity"; duration: 0; }
            ScriptAction { script: base.full(); }
        },
        Transition {
            from: "collapsed";
            to: "full";

            NumberAnimation { properties: "opacity"; duration: 100; }
            ScriptAction { script: base.full(); }
        },
        Transition {
            from: "full";
            to: "collapsed";

            NumberAnimation { properties: "opacity"; duration: 100; }
            ScriptAction { script: base.collapsed(); }
        },
        Transition {
            from: "full"
            to: "edit"
            reversible: true;

            NumberAnimation { properties: "width"; duration: 250; }
        }
    ]
}
