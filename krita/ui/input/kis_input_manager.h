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

#ifndef KIS_INPUTMANAGER_H
#define KIS_INPUTMANAGER_H

#include <QObject>
#include <krita_export.h>

class QPointF;
class QTabletEvent;
class QTouchEvent;
class KisToolProxy;
class KisCanvas2;
class KisInputAction;
/**
 * \brief Central object to manage canvas input.
 *
 * The Input Manager class manages all canvas input. It is created
 * by KisCanvas2 and processes all events related to input sent to the
 * canvas.
 *
 * The Input Manager keeps track of a set of actions and a set of
 * shortcuts. The actions are pre-defined while the shortcuts are
 * set from configuration.
 *
 * For each event, it will try to determine if there is a shortcut that
 * matches the input. It will then activate this action and pass all
 * consecutive events on to this action.
 *
 * \sa KisAbstractInputAction
 *
 * \todo Implement shortcut configuration
 */
class KRITAUI_EXPORT KisInputManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * \param canvas The parent canvas.
     * \param proxy The application's tool proxy.
     */
    explicit KisInputManager(KisCanvas2* canvas, KisToolProxy* proxy);
    /**
     * Destructor.
     */
    ~KisInputManager();

    void toggleTabletLogger();

    /**
     * Installs the input manager as an event filter for \p receiver.
     * Please note that KisInputManager is supposed to handle events
     * for a single receiver only. This is defined by the fact that it
     * resends some of the events back through the Qt's queue to the
     * reciever. That is why the input manager will assert when it gets
     * an event with wrong destination.
     */
    void setupAsEventFilter(QObject *receiver);

    /**
     * Event filter method. Overridden from QObject.
     */
    bool eventFilter(QObject* object, QEvent* event );

    void attachPriorityEventFilter(QObject *filter);
    void detachPriorityEventFilter(QObject *filter);

    /**
     * Return the canvas this input manager is associated with.
     */
    KisCanvas2 *canvas() const;

    /**
     * The tool proxy of the current application.
     */
    KisToolProxy *toolProxy() const;

    /**
     * Returns the event object for the last tablet event
     * happened. Returns null if there was no tablet event recently
     */
    QTabletEvent *lastTabletEvent() const;

    /**
     * Touch events are special, too.
     *
     * \return a touch event if there was one, otherwise 0
     */
    QTouchEvent *lastTouchEvent() const;

    /**
     * Convert a widget position to a document position.
     */
    QPointF widgetToDocument(const QPointF &position);

public Q_SLOTS:
    void setMirrorAxis();
	void stopIgnoringEvents();

private Q_SLOTS:
    void slotToolChanged();
    void profileChanged();
    void slotCompressedMoveEvent();

private:
    class Private;
    Private* const d;
};

#endif // KIS_INPUTMANAGER_H
