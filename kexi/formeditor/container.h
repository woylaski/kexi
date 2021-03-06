/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2008-2011 Jarosław Staniek <staniek@kde.org>

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

#ifndef FORMEDITORCONTAINER_H
#define FORMEDITORCONTAINER_H

#include "kformdesigner_export.h"
#include "utils.h"
#include "form.h"

#include <QPointer>
#include <QWidget>
#include <QMouseEvent>

class QEvent;
class QLayout;

namespace KFormDesigner
{

class Container;
class ObjectTreeItem;

/**
 * This class is used to filter the events from any widget (and all its subwidgets)
 * and direct it to the Container.
 */
//! A class for redirecting events
class KFORMDESIGNER_EXPORT EventEater : public QObject
{
    Q_OBJECT

public:
    /*! Constructs eater object. All events for \a widget and it's subwidgets
    will be redirected to \a container. \a container will be also parent of eater object,
    so you don't need to care about deleting it. */
    EventEater(QWidget *widget, QObject *container);

    ~EventEater();

    //! Sets the object which will receive the events
    void setContainer(QObject *container);

protected:
    bool eventFilter(QObject *o, QEvent *ev);

private:
    QPointer<QWidget>  m_widget;
    QPointer<QObject>  m_container;
};

/**
 * This class makes a container out of any QWidget. You can then create child widgets, and
 the background is dotted.
 */
//! A class to make a container from any widget
class KFORMDESIGNER_EXPORT Container : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a Container from the widget \a container, which have
     \a toplevel as parent Container. */
    Container(Container *toplevel, QWidget *container, QObject *parent = 0);

    virtual ~Container();

    //! \return a pointer to the toplevel Container, or 0 if this Container is toplevel
    Container* toplevel();

    //! \return the same as toplevel()->widget()
    QWidget* topLevelWidget() const;

    //! \return The form this Container belongs to.
    Form* form() const;

    //! \return The watched widget.
    QWidget* widget() const;

    //! \return The ObjectTreeItem associated with this Container's widget.
    ObjectTreeItem* objectTree() const;

    //! Sets the Form which this Container belongs to.
    void setForm(Form *form);

    /*! Sets the ObjectTree of this Container.\n
     * NOTE: this is needed only if we are toplevel. */
    void setObjectTree(ObjectTreeItem *t);

    //! \return a pointer to the QLayout of this Container, or 0 if there is not.
    QLayout* layout() const;

    //! \return the type of the layout associated to this Container's widget (see Form::LayoutType enum).
    Form::LayoutType layoutType() const;

    //! \return the margin of this Container.
    int layoutMargin() const;

    //! \return the spacing of this Container.
    int layoutSpacing() const;

    /*! Sets this Container to use \a type of layout. The widget are inserted
     automatically in the layout following their positions.
      \sa createBoxLayout(), createGridLayout() */
    void setLayoutType(Form::LayoutType type);

    //! Sets the spacing of this Container.
    void setLayoutSpacing(int spacing);

    //! Sets the margin of this Container.
    void setLayoutMargin(int margin);

    //! \return the string representing the layoutType \a type.
    static QString layoutTypeToString(Form::LayoutType type);

    //! \return the LayoutType (an int) for a given layout name.
    static Form::LayoutType stringToLayoutType(const QString &name);

    /*! Stops the inline editing of the current widget (as when you click
     on another widget or press Esc). */
    void stopInlineEditing();

    /*! This is the main function of Container, which filters the event sent
       to the watched widget.\n It takes care of drawing the background and
       the insert rect, of creating the new child widgets, of moving the widgets
        and pop up a menu when right-clicking. */
    virtual bool eventFilter(QObject *o, QEvent *e);

public Q_SLOTS:
    /*! Sets \a selected to be the selected widget of this container
      (and so of the Form). See Form::WidgetSelectionFlags description
      for exmplanation of possible combination of @a flags flags.
      \sa Form::selectWidget() */
      void selectWidget(QWidget *w, Form::WidgetSelectionFlags flags = Form::DefaultWidgetSelectionFlags);

    /*! Deselects the widget \a w. The widget is removed from the Form's list
     and its resizeHandles are removed. */
    void deselectWidget(QWidget *w);

    /*! Deletes the widget \a w. Removes it from ObjectTree, and sets selection
     to Container's widget. */
    void deleteWidget(QWidget *w);

    /*! Recreates the Container layout. Calls this when a widget has been moved
     or added to update the layout. */
    void reloadLayout();

    //! Used by handler-based resizing.
    void startChangingGeometryPropertyForSelectedWidget();

    //! Used by handler-based resizing.
    void setGeometryPropertyForSelectedWidget(const QRect &newGeometry);

protected Q_SLOTS:
    /*! This slot is called when the watched widget is deleted. Deletes the Container too. */
    void widgetDeleted();

protected:
    /*! Internal function to create a HBoxLayout or VBoxLayout for this container.
     \a list is a subclass of CustomSortableWidgetList that can sort widgets
     depending on their orientation (i.e. HorizontalWidgetList or VerticalWidgetList). */
    void createBoxLayout(CustomSortableWidgetList* list);

    /*! Internal function to create a GridLayout. if \a testOnly is true, the layout
      is simulated, and only the widget's grid info aris filled. */
    void createGridLayout(bool testOnly = false);

    void setLayout(QLayout *layout);

#ifdef KFD_SIGSLOTS
    //! Drawing functions used by eventFilter
    void drawConnection(QMouseEvent *mev);
#endif

    void moveSelectedWidgetsBy(int realdx, int realdy, QMouseEvent *mev = 0);

private:
    bool handleMouseReleaseEvent(QObject *s, QMouseEvent *mev);

    QRect selectionOrInsertingRectangle() const;

    QPoint selectionOrInsertingBegin() const;

    void selectionWidgetsForRectangle(const QPoint& secondPoint);

    class Private;
    Private * const d;
    friend class InsertWidgetCommand;
    friend class PasteWidgetCommand;
    friend class DeleteWidgetCommand;
    friend class FormIO;
};

//! Interface for adding dynamically created (at design time) widget to event eater.
/*! This is currently used by KexiDBFieldEdit from Kexi forms. */
class KFORMDESIGNER_EXPORT DesignTimeDynamicChildWidgetHandler
{
public:
    DesignTimeDynamicChildWidgetHandler();
    ~DesignTimeDynamicChildWidgetHandler();

protected:
    void childWidgetAdded(QWidget* w);
    void assignItem(ObjectTreeItem* item);

private:
    class Private;
    Private * const d;
    friend class InsertWidgetCommand;
    friend class FormIO;
};

}

#endif
