/*
 * This file is part of the KDE project
 * Copyright (C) 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_mouse_input_editor.h"

#include <QWidgetAction>
#include <QMenu>
#include <QTimer>

#include "KoIcon.h"

#include "ui_kis_mouse_input_editor.h"

class KisMouseInputEditor::Private
{
public:
    Private() { }

    Ui::KisMouseInputEditor *ui;
};

KisMouseInputEditor::KisMouseInputEditor(QWidget *parent)
    : KPushButton(parent), d(new Private)
{
    QWidget *popup = new QWidget();

    d->ui = new Ui::KisMouseInputEditor;
    d->ui->setupUi(popup);
    d->ui->mouseButton->setType(KisInputButton::MouseType);

    d->ui->clearModifiersButton->setIcon(koIcon("edit-clear-locationbar-rtl"));
    d->ui->clearMouseButton->setIcon(koIcon("edit-clear-locationbar-rtl"));

    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);

    QMenu *menu = new QMenu(this);
    menu->addAction(action);
    setMenu(menu);

    QTimer::singleShot(0, this, SLOT(showMenu()));

    connect(d->ui->mouseButton, SIGNAL(dataChanged()), SLOT(updateLabel()));
    connect(d->ui->modifiersButton, SIGNAL(dataChanged()), SLOT(updateLabel()));
    connect(d->ui->clearMouseButton, SIGNAL(clicked(bool)), d->ui->mouseButton, SLOT(clear()));
    connect(d->ui->clearModifiersButton, SIGNAL(clicked(bool)), d->ui->modifiersButton, SLOT(clear()));
}

KisMouseInputEditor::~KisMouseInputEditor()
{
    delete d->ui;
    delete d;
}

QList< Qt::Key > KisMouseInputEditor::keys() const
{
    return d->ui->modifiersButton->keys();
}

void KisMouseInputEditor::setKeys(const QList< Qt::Key > &newKeys)
{
    d->ui->modifiersButton->setKeys(newKeys);
    updateLabel();
}

Qt::MouseButtons KisMouseInputEditor::buttons() const
{
    return d->ui->mouseButton->buttons();
}

void KisMouseInputEditor::setButtons(Qt::MouseButtons newButtons)
{
    d->ui->mouseButton->setButtons(newButtons);
    updateLabel();
}

void KisMouseInputEditor::updateLabel()
{
    QString text;

    if (d->ui->modifiersButton->keys().size() > 0) {
        text.append(KisShortcutConfiguration::keysToText(d->ui->modifiersButton->keys()));
        text.append(" + ");
    }

    text.append(KisShortcutConfiguration::buttonsToText(d->ui->mouseButton->buttons()));

    setText(text);
}
