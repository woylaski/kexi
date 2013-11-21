/* This file is part of the KDE project

   Copyright 1999-2006 The KSpread Team <calligra-devel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#ifndef CALLIGRA_SHEETS_CELL_EDITOR
#define CALLIGRA_SHEETS_CELL_EDITOR

#include <kglobalsettings.h>
#include <ktextedit.h>


#include "calligra_sheets_export.h"
#include "CellEditorBase.h"

#include <QCompleter>
#include <QAbstractItemModel>
#include <QThread>
#include <QHash>
class KoViewConverter;

namespace Calligra
{
namespace Sheets
{
class CellToolBase;
class Selection;

/**
 * class CellEditor
 */
class CellEditor : public KTextEdit, public CellEditorBase
{
    Q_OBJECT
public:
    /**
    * Creates a new CellEditor.
    * \param cellTool the cell tool
    * \param parent the parent widget
    */
    explicit CellEditor(CellToolBase *cellTool, QHash<int, QString> &wordList, QWidget *parent = 0);
    ~CellEditor();

    Selection* selection() const;

    void setEditorFont(QFont const & font, bool updateSize, const KoViewConverter *viewConverter);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    QPoint globalCursorPosition() const;
    QAbstractItemModel *model();

    /**
     * Replaces the current formula token(/reference) with the name of the
     * selection's active sub-region name.
     * This is called after selection changes to sync the formula expression.
     */
    void selectionChanged();

    /**
     * Activates the sub-region belonging to the \p index 'th range.
     */
    void setActiveSubRegion(int index);

    // CellEditorBase interface
    virtual QWidget* widget() { return this; }
    virtual void cut() { KTextEdit::cut(); }
    virtual void copy() { KTextEdit::copy(); }
    virtual void paste() { KTextEdit::paste(); }
    virtual QString toPlainText() const { return KTextEdit::toPlainText(); }
Q_SIGNALS:
    void textChanged(const QString &text);

public slots:
    void setText(const QString& text, int cursorPos = -1);

    /**
     * Permutes the fixation of the reference, at which the editor's cursor
     * is placed. It is only active, if a formula is edited.
     */
    void permuteFixation();
    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

private slots:
    void  slotTextChanged();
    void  slotCompletionModeChanged(KGlobalSettings::Completion _completion);
    void  slotCursorPositionChanged();
    void insertCompletion(const QString &completion);

protected: // reimplementations
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);

private:
    Q_DISABLE_COPY(CellEditor)
    QString textUnderCursor() const;

    class Private;
    Private * const d;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_CELL_EDITOR
