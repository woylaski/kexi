/*
 * This file is part of the KDE project
 * Copyright (C) 2014 Denis Kuplyakov <dener.kup@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.*/

#include "RenameSectionCommand.h"
#include <KoSection.h>

#include <klocale.h>
#include <kundo2command.h>

RenameSectionCommand::RenameSectionCommand(KoSection* section, QString newName)
    : KUndo2Command ()
    , m_section(section)
    , m_newName(newName)
    , m_first(true)
{
    setText(kundo2_i18n("Rename Section"));
}

RenameSectionCommand::~RenameSectionCommand()
{
}

void RenameSectionCommand::undo()
{
    KUndo2Command::undo();
    m_section->setName(m_oldName);
}

void RenameSectionCommand::redo()
{
    if (!m_first) {
        KUndo2Command::redo();
    }
    m_oldName = m_section->name();
    m_section->setName(m_newName);
    m_first = false;
}