/* This file is part of the KDE project
 * Copyright (C) 2014 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

import QtQuick 2.0
import org.calligra 1.0
import "../../components"

Rectangle {
    id: base;
    anchors.fill: parent;
    anchors.margins: Settings.theme.adjustedPixel(16);
    property int accountIndex: -1;
    onAccountIndexChanged: {
        accountDetails = cloudAccounts.accountDetails(accountIndex);
    }
    property string text;
    onTextChanged: {
        nameField.text = text;
    }
    property QtObject accountDetails;
    property Component addEmpty;
    radius: Settings.theme.adjustedPixel(8);
    color: "white";
    Rectangle {
        anchors {
            fill: parent;
            margins: -Settings.theme.adjustedPixel(16);
        }
        opacity: 0.3;
        color: "white";
        MouseArea { anchors.fill: parent; onClicked: { /*nothing */ } }
        SimpleTouchArea { anchors.fill: parent; onTouched: { /*nothing */ } }
    }
    TextField {
        id: nameField;
        anchors {
            verticalCenter: parent.verticalCenter;
            left: parent.left;
            right: parent.right;
            margins: Settings.theme.adjustedPixel(16);
        }
        placeholder: "Account Name";
    }
    CohereButton {
        id: credentialsButton;
        anchors {
            top: nameField.bottom;
            right: parent.right;
            margins: Settings.theme.adjustedPixel(8);
        }
        text: "Edit User Credentials";
        textColor: "#5b6573";
        textSize: Settings.theme.adjustedPixel(18);
        color: "#D2D4D5";
        onClicked: {
            dlgStack.push(userCredentials.item);
            if(accountDetails.readProperty("userForRemote") !== undefined) {
                dlgStack.currentPage.userForRemote = accountDetails.readProperty("userForRemote");
            }
            if(accountDetails.readProperty("privateKeyFile") !== undefined) {
                dlgStack.currentPage.privateKeyFile = accountDetails.readProperty("privateKeyFile");
            }
            if(accountDetails.readProperty("publicKeyFile") !== undefined) {
                dlgStack.currentPage.publicKeyFile = accountDetails.readProperty("publicKeyFile");
            }
            if(accountDetails.readProperty("needsPrivateKeyPassphrase") !== undefined) {
                dlgStack.currentPage.needsPrivateKeyPassphrase = accountDetails.readProperty("needsPrivateKeyPassphrase");
            }
        }
    }
    CohereButton {
        id: okButton;
        anchors {
            bottom: parent.bottom;
            right: cancelButton.left;
            margins: Settings.theme.adjustedPixel(8);
        }
        text: "Save";
        textColor: "#5b6573";
        textSize: Settings.theme.adjustedPixel(18);
        color: "#D2D4D5";
        onClicked: {
            cloudAccounts.renameAccount(base.accountIndex, nameField.text);
            dlgStack.replace(addEmpty);
        }
    }
    CohereButton {
        id: cancelButton;
        anchors {
            bottom: parent.bottom;
            right: parent.right;
            margins: Settings.theme.adjustedPixel(8);
        }
        text: "Cancel";
        textColor: "#5b6573";
        textSize: Settings.theme.adjustedPixel(18);
        color: "#D2D4D5";
        onClicked: dlgStack.replace(addEmpty);
    }

    Loader {
        id: userCredentials;
        source: "git/userCredentialsContainer.qml"
    }
}
