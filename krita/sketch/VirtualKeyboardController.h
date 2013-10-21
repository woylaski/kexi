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

#ifndef VIRTUALKEYBOARDCONTROLLER_H
#define VIRTUALKEYBOARDCONTROLLER_H

#include <QObject>

#include "image/krita_export.h"

class KRITASKETCH_EXPORT VirtualKeyboardController : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void requestShowKeyboard();
    Q_INVOKABLE void requestHideKeyboard();

    static VirtualKeyboardController* instance();

Q_SIGNALS:
    void showKeyboard();
    void hideKeyboard();

private:
    explicit VirtualKeyboardController(QObject* parent = 0);
    virtual ~VirtualKeyboardController();

    static VirtualKeyboardController* sm_instance;
};

#endif // VIRTUALKEYBOARDCONTROLLER_H
