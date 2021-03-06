/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2015 Jarosław Staniek <staniek@kde.org>

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
 * Boston, MA 02110-1301, USA.
*/

#include "kexiformscrollview.h"
#include "KexiFormScrollAreaWidget.h"
#include "widgets/kexidbform.h"

#include <formeditor/form.h>
#include <formeditor/objecttree.h>
#include <formeditor/commands.h>
#include <utils/kexirecordnavigator.h>
#include <core/kexi.h>
#include <kexiutils/utils.h>
#include <kexi_global.h>

#include <QDebug>
#include <QMenu>
#include <QPalette>
#include <QScrollBar>
#include <QHeaderView>
#include <QTimer>

class KexiFormScrollView::Private
{
public:
    Private(KexiFormScrollView * view, bool preview_)
        : q(view)
        , resizingEnabled(true)
        , preview(preview_)
        , scrollBarPolicySet(false)
        , scrollViewNavPanel(0)
        , scrollViewNavPanelVisible(false)
        , mainAreaWidget(0)
        , currentLocalSortColumn(-1) /* no column */
        , localSortOrder(Qt::AscendingOrder)
        , previousRecord(0)
    {
    }

    void setHorizontalScrollBarPolicyDependingOnNavPanel() {
        q->setHorizontalScrollBarPolicy(
            (scrollViewNavPanel && scrollViewNavPanelVisible)
                    ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded);
    }

    KexiFormScrollView * const q;
    bool resizingEnabled;
    QFont helpFont;
    QColor helpColor;
    QTimer delayedResize;
    //! for refreshContentsSizeLater()
    Qt::ScrollBarPolicy verticalScrollBarPolicy;
    Qt::ScrollBarPolicy horizontalScrollBarPolicy;
    bool preview;
    bool scrollBarPolicySet;
    bool outerAreaVisible;
    KexiRecordNavigator* scrollViewNavPanel;
    bool scrollViewNavPanelVisible; //!< Needed because visibility depends on form's visibility but we want to know earlier
    QMargins viewportMargins;
    QWidget *mainAreaWidget;

    KFormDesigner::Form *form;
    int currentLocalSortColumn;
    Qt::SortOrder localSortOrder;
    //! Used in selectCellInternal() to avoid fetching the same record twice
    KDbRecordData *previousRecord;
};

KexiFormScrollView::KexiFormScrollView(QWidget *parent, bool preview)
        : QScrollArea(parent)
        , KexiRecordNavigatorHandler()
        , KexiSharedActionClient()
        , KexiDataAwareObjectInterface()
        , KexiFormDataProvider()
        , KexiFormEventHandler()
        , d(new Private(this, preview))
{
    setObjectName("KexiFormScrollView");
    setAttribute(Qt::WA_StaticContents, true);
    setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    if (!d->preview) {
        QPalette pal(viewport()->palette());
        pal.setBrush(viewport()->backgroundRole(), pal.brush(QPalette::Mid));
        viewport()->setPalette(pal);
    }
    const QColor fc = palette().color(QPalette::WindowText);
    const QColor bc = viewport()->palette().color(QPalette::Window);
    d->helpColor = KexiUtils::blendedColors(fc, bc, 1, 2);
    d->helpFont = font();
    d->helpFont.setPointSize(d->helpFont.pointSize() * 3);
    setFocusPolicy(Qt::WheelFocus);
    d->outerAreaVisible = true;

    d->delayedResize.setSingleShot(true);
    connect(&(d->delayedResize), SIGNAL(timeout()), this, SLOT(refreshContentsSize()));
    if (d->preview) {
//! @todo allow to hide navigator
        d->scrollViewNavPanel = new KexiRecordNavigator(*this, this);
    }
    else {
        KexiFormScrollAreaWidget *scrollAreaWidget = new KexiFormScrollAreaWidget(this);
        setWidget(scrollAreaWidget);
        connect(scrollAreaWidget, SIGNAL(resized()), this, SIGNAL(resized()));
    }
    m_navPanel = recordNavigator(); //copy this pointer from KexiFormScrollView
    if (d->preview) {
        setRecordNavigatorVisible(true);
        refreshContentsSizeLater();
    }
    m_contextMenu = new QMenu(this);
    m_contextMenu->setObjectName("m_contextMenu");
//! @todo sorting temporarily disabled because not it's not implemented in forms (bug 150372)
    setSortingEnabled(false);
}

KexiFormScrollView::~KexiFormScrollView()
{
    if (m_owner)
        delete m_data;
    m_data = 0;
    delete d;
}

int KexiFormScrollView::recordsPerPage() const
{
    //! @todo
    return 10;
}

void KexiFormScrollView::selectCellInternal(int previousRecord, int previousColumn)
{
    Q_UNUSED(previousRecord);
    Q_UNUSED(previousColumn);

    //m_currentRecord is already set by KexiDataAwareObjectInterface::setCursorPosition()
    if (m_currentRecord) {
        if (m_currentRecord != d->previousRecord) {
            fillDataItems(m_currentRecord, cursorAtNewRecord());
            d->previousRecord = m_currentRecord;
            QWidget *w = 0;
            if (m_curColumn >= 0 && m_curColumn < dbFormWidget()->orderedDataAwareWidgets()->count()) {
                w = dbFormWidget()->orderedDataAwareWidgets()->at(m_curColumn);
            }
            if (w) {
                w->setFocus(); // re-focus, as we could have lost focus, e.g. when navigator button was clicked
                // select all
                KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>(w);
//! @todo add option for not selecting the field
                if (iface) {
                    iface->selectAllOnFocusIfNeeded();
                }
            }
        }
    } else {
        d->previousRecord = 0;
    }
}

void KexiFormScrollView::ensureCellVisible(int record, int column)
{
    Q_UNUSED(record);
    Q_UNUSED(column);
    //! @todo
}

void KexiFormScrollView::ensureColumnVisible(int col)
{
    Q_UNUSED(col);
    //! @todo
}

void KexiFormScrollView::moveToRecordRequested(int r)
{
    //! @todo
    selectRecord(r);
}

void KexiFormScrollView::moveToLastRecordRequested()
{
    //! @todo
    selectLastRecord();
}

void KexiFormScrollView::moveToPreviousRecordRequested()
{
    //! @todo
    selectPreviousRecord();
}

void KexiFormScrollView::moveToNextRecordRequested()
{
    //! @todo
    selectNextRecord();
}

void KexiFormScrollView::moveToFirstRecordRequested()
{
    //! @todo
    selectFirstRecord();
}

void KexiFormScrollView::clearColumnsInternal(bool repaint)
{
    Q_UNUSED(repaint);
    //! @todo
}

Qt::SortOrder KexiFormScrollView::currentLocalSortOrder() const
{
    //! @todo
    return d->localSortOrder;
}

int KexiFormScrollView::currentLocalSortColumn() const
{
    return d->currentLocalSortColumn;
}

void KexiFormScrollView::setLocalSortOrder(int column, Qt::SortOrder order)
{
    //! @todo
    d->currentLocalSortColumn = column;
    d->localSortOrder = order;
}

void KexiFormScrollView::sortColumnInternal(int col, int order)
{
    Q_UNUSED(col);
    Q_UNUSED(order);
    //! @todo
}

void KexiFormScrollView::updateGUIAfterSorting(int previousRecord)
{
    Q_UNUSED(previousRecord);
    //! @todo
}

void KexiFormScrollView::createEditor(int record, int column, const QString& addText,
                                      CreateEditorFlags flags)
{
    Q_UNUSED(addText);
    Q_UNUSED(flags);

    if (record < 0) {
        qWarning() << "RECORD NOT SPECIFIED!" << record;
        return;
    }
    if (isReadOnly()) {
        qWarning() << "DATA IS READ ONLY!";
        return;
    }
    if (this->column(column)->isReadOnly()) {
        qWarning() << "COL IS READ ONLY!";
        return;
    }
    if (recordEditing() >= 0 && record != recordEditing()) {
        if (!acceptRecordEditing()) {
            return;
        }
    }
    //! @todo
    const bool startRecordEditing = recordEditing() == -1; //remember if we're starting record edit
    if (startRecordEditing) {
        //we're starting record editing session
        m_data->clearRecordEditBuffer();
        setRecordEditing(record);
        //indicate on the vheader that we are editing:
        if (verticalHeader()) {
            updateVerticalHeaderSection(currentRecord());
        }
        if (isInsertingEnabled() && record == recordCount()) {
            //we should know that we are in state "new record editing"
            m_newRecordEditing = true;
            //'insert' record editing: show another record after that:
            m_data->append(m_insertRecord);
            //new empty insert item
            m_insertRecord = m_data->createItem();
            updateWidgetContentsSize();
        }
    }

    m_editor = editor(column); //m_dataItems.at(col);
    if (!m_editor)
        return;

    if (startRecordEditing) {
        recordNavigator()->showEditingIndicator(true);
        //emit recordEditingStarted(record);
    }
}

KexiDataItemInterface *KexiFormScrollView::editor(int col, bool ignoreMissingEditor)
{
    Q_UNUSED(ignoreMissingEditor);

    if (!m_data || col < 0 || col >= columnCount())
        return 0;

    return dynamic_cast<KexiFormDataItemInterface*>(dbFormWidget()->orderedDataAwareWidgets()->at(col));
}

void KexiFormScrollView::editorShowFocus(int record, int column)
{
    Q_UNUSED(record);
    Q_UNUSED(column);
    //! @todo
}

void KexiFormScrollView::updateCell(int record, int column)
{
    Q_UNUSED(record);
    Q_UNUSED(column);
    //! @todo
}

void KexiFormScrollView::updateCurrentCell()
{
}

void KexiFormScrollView::updateRecord(int record)
{
    Q_UNUSED(record)
    //! @todo
}

void KexiFormScrollView::updateWidgetContents()
{
    //! @todo
}

void KexiFormScrollView::updateWidgetContentsSize()
{
    //! @todo
}

void KexiFormScrollView::slotRecordRepaintRequested(KDbRecordData* data)
{
    Q_UNUSED(data);
    //! @todo
}

void KexiFormScrollView::slotRecordInserted(KDbRecordData* data, bool repaint)
{
    Q_UNUSED(data);
    Q_UNUSED(repaint);
    //! @todo
}

void KexiFormScrollView::slotRecordInserted(KDbRecordData* data, int record, bool repaint)
{
    Q_UNUSED(data);
    Q_UNUSED(record);
    Q_UNUSED(repaint);
    //! @todo
}

void KexiFormScrollView::slotRecordsDeleted(const QList<int> &)
{
    //! @todo
}

KexiDBForm* KexiFormScrollView::dbFormWidget() const
{
    return qobject_cast<KexiDBForm*>(d->preview ? widget() : mainAreaWidget());
}

int KexiFormScrollView::columnCount() const
{
    return dbFormWidget()->orderedDataAwareWidgets()->count(); //m_dataItems.count();
}

int KexiFormScrollView::recordCount() const
{
    return KexiDataAwareObjectInterface::recordCount();
}

int KexiFormScrollView::currentRecord() const
{
    return KexiDataAwareObjectInterface::currentRecord();
}

void KexiFormScrollView::setForm(KFormDesigner::Form *form)
{
    d->form = form;
}

KFormDesigner::Form* KexiFormScrollView::form() const
{
    return d->form;
}

bool KexiFormScrollView::columnEditable(int col)
{
    //qDebug() << "col=" << col;
    foreach(KexiFormDataItemInterface *dataItemIface, m_dataItems) {
        qDebug() << (dynamic_cast<QWidget*>(dataItemIface)
                     ? dynamic_cast<QWidget*>(dataItemIface)->objectName() : "")
            << " " << dataItemIface->dataSource();
    }
    //qDebug() << "-- focus widgets --";
    foreach(QWidget* widget, *dbFormWidget()->orderedFocusWidgets()) {
        qDebug() << widget->objectName();
    }
    //qDebug() << "-- data-aware widgets --";
    foreach(QWidget *widget, *dbFormWidget()->orderedDataAwareWidgets()) {
        qDebug() << widget->objectName();
    }

    KexiFormDataItemInterface *item = dynamic_cast<KexiFormDataItemInterface*>(
                                          dbFormWidget()->orderedDataAwareWidgets()->at(col));

    if (!item || item->isReadOnly())
        return false;

    return KexiDataAwareObjectInterface::columnEditable(col);
}

void KexiFormScrollView::valueChanged(KexiDataItemInterface* item)
{
    if (!item)
        return;
    //only signal start editing when no record editing was started already
    /*qDebug() << "** editedItem="
        << (dbFormWidget()->editedItem ? dbFormWidget()->editedItem->value().toString() : QString())
        << ", "
        << (item ? item->value().toString() : QString());*/
    if (dbFormWidget()->editedItem != item) {
        //qDebug() << "**>>> dbFormWidget()->editedItem = dynamic_cast<KexiFormDataItemInterface*>(item)";
        dbFormWidget()->editedItem = dynamic_cast<KexiFormDataItemInterface*>(item);
        startEditCurrentCell();
    }
    fillDuplicatedDataItems(dynamic_cast<KexiFormDataItemInterface*>(item), item->value());

    //value changed: clear 'default value' mode (e.g. a blue italic text)
    dynamic_cast<KexiFormDataItemInterface*>(item)->setDisplayDefaultValue(dynamic_cast<QWidget*>(item), false);
}

bool KexiFormScrollView::cursorAtNewRecord() const
{
    return isInsertingEnabled() && (m_currentRecord == m_insertRecord || m_newRecordEditing);
}

void KexiFormScrollView::lengthExceeded(KexiDataItemInterface *item, bool lengthExceeded)
{
    showLengthExceededMessage(item, lengthExceeded);
}

void KexiFormScrollView::updateLengthExceededMessage(KexiDataItemInterface *item)
{
    showUpdateForLengthExceededMessage(item);
}

void KexiFormScrollView::initDataContents()
{
    KexiDataAwareObjectInterface::initDataContents();

    if (isPreviewing()) {
//! @todo here we can react if user wanted to show the navigator
        setRecordNavigatorVisible(m_data);
        recordNavigator()->setEnabled(m_data);
        if (m_data) {
            recordNavigator()->setEditingIndicatorEnabled(!isReadOnly());
            recordNavigator()->showEditingIndicator(false);
        }

        dbFormWidget()->updateReadOnlyFlags();
    }
}

KDbTableViewColumn* KexiFormScrollView::column(int col)
{
    const int id = fieldNumberForColumn(col);
    return (id >= 0) ? m_data->column(id) : 0;
}

bool KexiFormScrollView::shouldDisplayDefaultValueForItem(KexiFormDataItemInterface* itemIface) const
{
    return cursorAtNewRecord()
           && !itemIface->columnInfo()->field->defaultValue().isNull()
           && !itemIface->columnInfo()->field->isAutoIncrement(); // default value defined
}

bool KexiFormScrollView::cancelEditor()
{
    if (!dynamic_cast<KexiFormDataItemInterface*>(m_editor))
        return false;

    if (m_errorMessagePopup)
        m_errorMessagePopup->close();

    KexiFormDataItemInterface *itemIface = dynamic_cast<KexiFormDataItemInterface*>(m_editor);
    itemIface->undoChanges();

    const bool displayDefaultValue = shouldDisplayDefaultValueForItem(itemIface);
    // now disable/enable "display default value" if needed (do it after setValue(), before setValue() turns it off)
    if (itemIface->hasDisplayedDefaultValue() != displayDefaultValue)
        itemIface->setDisplayDefaultValue(dynamic_cast<QWidget*>(itemIface), displayDefaultValue);

    fillDuplicatedDataItems(itemIface, m_editor->value());

    // this will clear editor pointer and close message popup (if present)
    return KexiDataAwareObjectInterface::cancelEditor();
}

void KexiFormScrollView::updateAfterCancelRecordEditing()
{
    foreach(KexiFormDataItemInterface *dataItemIface, m_dataItems) {
        if (dynamic_cast<QWidget*>(dataItemIface)) {
            qDebug()
                << dynamic_cast<QWidget*>(dataItemIface)->metaObject()->className() << " "
                << dynamic_cast<QWidget*>(dataItemIface)->objectName();
        }
        const bool displayDefaultValue = shouldDisplayDefaultValueForItem(dataItemIface);
        dataItemIface->undoChanges();
        if (dataItemIface->hasDisplayedDefaultValue() != displayDefaultValue)
            dataItemIface->setDisplayDefaultValue(
                dynamic_cast<QWidget*>(dataItemIface), displayDefaultValue);
    }
    recordNavigator()->showEditingIndicator(false);
    dbFormWidget()->editedItem = 0;
    KexiFormDataItemInterface *item = dynamic_cast<KexiFormDataItemInterface*>(focusWidget());
    if (item) {
        item->selectAllOnFocusIfNeeded();
    }
}

void KexiFormScrollView::updateAfterAcceptRecordEditing()
{
    if (!m_currentRecord)
        return;
    recordNavigator()->showEditingIndicator(false);
    dbFormWidget()->editedItem = 0;
    //update visible data because there could be auto-filled (eg. autonumber) fields
    fillDataItems(m_currentRecord, cursorAtNewRecord());
    d->previousRecord = m_currentRecord;
    KexiFormDataItemInterface *item = dynamic_cast<KexiFormDataItemInterface*>(focusWidget());
    if (item) {
        item->selectAllOnFocusIfNeeded();
    }
}

int KexiFormScrollView::fieldNumberForColumn(int col)
{
    KexiFormDataItemInterface *item = dynamic_cast<KexiFormDataItemInterface*>(
                                            dbFormWidget()->orderedDataAwareWidgets()->at(col));
    if (!item)
        return -1;
    KexiFormDataItemInterfaceToIntMap::ConstIterator it(m_fieldNumbersForDataItems.find(item));
    return it != m_fieldNumbersForDataItems.constEnd() ? (int)it.value() : -1;
}


void KexiFormScrollView::beforeSwitchView()
{
    m_editor = 0;
}

void KexiFormScrollView::refreshContentsSize()
{
    if (!widget())
        return;
    if (d->preview) {
        setVerticalScrollBarPolicy(d->verticalScrollBarPolicy);
        //setHorizontalScrollBarPolicy(d->horizontalScrollBarPolicy);
        d->scrollBarPolicySet = false;
        updateScrollBars();
    }
    else {
        // Ensure there is always space to resize Form
        int w = viewport()->width();
        int h = viewport()->height();
        bool change = false;
        const int delta_x = 300;
        const int delta_y = 300;
        if ((widget()->width() + delta_x * 2 / 3) > w) {
            w = widget()->width() + delta_x;
            change = true;
        } else if ((w - widget()->width()) > delta_x) {
            w = widget()->width() + delta_x;
            change = true;
        }
        if ((widget()->height() + delta_y * 2 / 3) > h) {
            h = widget()->height() + delta_y;
            change = true;
        } else if ((h - widget()->height()) > delta_y) {
            h = widget()->height() + delta_y;
            change = true;
        }
        if (change) {
            widget()->resize(w, h);
        }
        updateScrollBars();
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        d->setHorizontalScrollBarPolicyDependingOnNavPanel();
    }
    updateScrollBars();

    qDebug() << widget()->size() << d->form->widget()->size() << dbFormWidget()->size();
    if (!d->preview) {
        widget()->resize(dbFormWidget()->size() + QSize(300, 300));
    }
    else {
        widget()->resize(viewport()->size());
    }

    //only clear cmd history when KexiScrollView::refreshContentsSizeLater() has been called
    if (!d->preview && sender() == delayedResizeTimer()) {
        if (d->form)
            d->form->clearUndoStack();
    }
}

void KexiFormScrollView::handleDataWidgetAction(const QString& actionName)
{
    QWidget *w = focusWidget();
    KexiFormDataItemInterface *item = 0;
    while (w) {
        item = dynamic_cast<KexiFormDataItemInterface*>(w);
        if (item)
            break;
        w = w->parentWidget();
    }
    if (item)
        item->handleAction(actionName);
}

void KexiFormScrollView::copySelection()
{
    handleDataWidgetAction("edit_copy");
}

void KexiFormScrollView::cutSelection()
{
    handleDataWidgetAction("edit_cut");
}

void KexiFormScrollView::paste()
{
    handleDataWidgetAction("edit_paste");
}

int KexiFormScrollView::lastVisibleRecord() const
{
//! @todo unimplemented for now, this will be used for continuous forms
    return -1;
}

QScrollBar* KexiFormScrollView::verticalScrollBar() const
{
    return QScrollArea::verticalScrollBar();
}

void KexiFormScrollView::setRecordNavigatorVisible(bool visible)
{
    if (d->scrollViewNavPanel) {
        d->scrollViewNavPanel->setVisible(visible);
        d->scrollViewNavPanelVisible = visible;
    }
}

bool KexiFormScrollView::isOuterAreaVisible() const
{
    return d->outerAreaVisible;
}

void KexiFormScrollView::setOuterAreaIndicatorVisible(bool visible)
{
    d->outerAreaVisible = visible;
}

bool KexiFormScrollView::isResizingEnabled() const
{
    return d->resizingEnabled;
}

void KexiFormScrollView::setResizingEnabled(bool enabled)
{
    d->resizingEnabled = enabled;
}

void KexiFormScrollView::refreshContentsSizeLater()
{
    if (!d->scrollBarPolicySet) {
        d->scrollBarPolicySet = true;
        d->verticalScrollBarPolicy = verticalScrollBarPolicy();
        d->horizontalScrollBarPolicy = horizontalScrollBarPolicy();
    }
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->setHorizontalScrollBarPolicyDependingOnNavPanel();
    updateScrollBars();
    d->delayedResize.start(100);
}

void KexiFormScrollView::setHBarGeometry(QScrollBar & hbar, int x, int y, int w, int h)
{
    if (d->scrollViewNavPanel && d->scrollViewNavPanel->isVisible()) {
        d->scrollViewNavPanel->setHBarGeometry(hbar, x, y, w, h);
    } else {
        hbar.setGeometry(x, y, w, h);
    }
}

KexiRecordNavigator* KexiFormScrollView::recordNavigator() const
{
    return d->scrollViewNavPanel;
}


bool KexiFormScrollView::isPreviewing() const
{
    return d->preview;
}

const QTimer *KexiFormScrollView::delayedResizeTimer() const
{
  return &(d->delayedResize);
}

void KexiFormScrollView::setViewportMargins(const QMargins &margins)
{
    QScrollArea::setViewportMargins(margins);
    d->viewportMargins = margins;
}

QMargins KexiFormScrollView::viewportMargins() const
{
    return d->viewportMargins;
}

void KexiFormScrollView::setMainAreaWidget(QWidget* widget)
{
    d->mainAreaWidget = widget;
}

QWidget* KexiFormScrollView::mainAreaWidget() const
{
    return d->mainAreaWidget;
}

QRect KexiFormScrollView::viewportGeometry() const
{
    return viewport()->geometry();
}

void KexiFormScrollView::updateVerticalHeaderSection(int section)
{
    Q_UNUSED(section);
    //! @todo implement when header is used
}

