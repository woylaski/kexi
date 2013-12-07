/* This file is part of the KDE project * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#include "kis_input_manager.h"

#include <QDebug>
#include <QQueue>
#include <QMessageBox>

#include <kaction.h>
#include <klocalizedstring.h>
#include <kactioncollection.h>
#include <QApplication>

#include "kis_tool_proxy.h"

#include <kis_canvas2.h>
#include <kis_view2.h>
#include <kis_image.h>
#include <kis_canvas_resource_provider.h>
#include <ko_favorite_resource_manager.h>

#include "kis_abstract_input_action.h"
#include "kis_tool_invocation_action.h"
#include "kis_pan_action.h"
#include "kis_alternate_invocation_action.h"
#include "kis_rotate_canvas_action.h"
#include "kis_zoom_action.h"
#include "kis_show_palette_action.h"
#include "kis_change_primary_setting_action.h"

#include "kis_shortcut_matcher.h"
#include "kis_stroke_shortcut.h"
#include "kis_single_action_shortcut.h"
#include "kis_touch_shortcut.h"

#include "kis_input_profile.h"
#include "kis_input_profile_manager.h"
#include "kis_shortcut_configuration.h"

#include <input/kis_tablet_event.h>
#include <kis_signal_compressor.h>


class KisInputManager::Private
{
public:
    Private(KisInputManager *qq)
        : q(qq)
        , toolProxy(0)
        , setMirrorMode(false)
        , forwardAllEventsToTool(false)
        , ignoreQtCursorEvents(false)
    #ifdef Q_WS_X11
        , hiResEventsWorkaroundCoeff(1.0, 1.0)
    #endif
        , lastTabletEvent(0)
        , lastTouchEvent(0)
        , moveEventCompressor(10 /* ms */, KisSignalCompressor::FIRST_ACTIVE)
        , logTabletEvents(false)
    {
    }

    bool tryHidePopupPalette();
    bool trySetMirrorMode(const QPointF &mousePosition);
    void saveTabletEvent(const QTabletEvent *event);
    void resetSavedTabletEvent(QEvent::Type type);
    void addStrokeShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, Qt::MouseButtons buttons);
    void addKeyShortcut(KisAbstractInputAction* action, int index,const QList<Qt::Key> &keys);
    void addTouchShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture );
    void addWheelShortcut(KisAbstractInputAction* action, int index, const QList< Qt::Key >& modifiers, KisShortcutConfiguration::MouseWheelMovement wheelAction);
    bool processUnhandledEvent(QEvent *event);
    Qt::Key workaroundShiftAltMetaHell(const QKeyEvent *keyEvent);
    void setupActions();
    void saveTouchEvent( QTouchEvent* event );
    bool handleKisTabletEvent(QObject *object, KisTabletEvent *tevent);

    KisInputManager *q;

    KisCanvas2 *canvas;
    KisToolProxy *toolProxy;

    bool setMirrorMode;
    bool forwardAllEventsToTool;
    bool ignoreQtCursorEvents;

    KisShortcutMatcher matcher;
#ifdef Q_WS_X11
    QPointF hiResEventsWorkaroundCoeff;
#endif
    QTabletEvent *lastTabletEvent;
    QTouchEvent *lastTouchEvent;

    KisToolInvocationAction *defaultInputAction;

    KisSignalCompressor moveEventCompressor;
    QScopedPointer<KisTabletEvent> compressedMoveEvent;

    bool logTabletEvents;
    void debugMouseEvent(QEvent *event);
    void debugTabletEvent(QEvent *event);

    class ProximityNotifier;
};

void KisInputManager::Private::debugMouseEvent(QEvent *event)
{
    if (!logTabletEvents) return;

    QString msg1 = ignoreQtCursorEvents ? "[BLOCKED] " : "[       ] ";

    QMouseEvent *mevent = static_cast<QMouseEvent*>(event);

    QString msg2 =
        mevent->type() == QEvent::MouseButtonPress ? "QEvent::MouseButtonPress" :
        mevent->type() == QEvent::MouseButtonRelease ? "QEvent::MouseButtonRelease" :
        mevent->type() == QEvent::MouseButtonDblClick ? "QEvent::MouseButtonDblClick" :
        mevent->type() == QEvent::MouseMove ? "QEvent::MouseMove" : "unknown";

    QString msg3 = QString("gpos: %1,%2").arg(mevent->globalPos().x()).arg(mevent->globalPos().y());
    QString msg4 = QString("pos: %1,%2").arg(mevent->pos().x()).arg(mevent->pos().y());

    QString msg5 = QString("button: %1").arg(mevent->button());
    QString msg6 = QString("buttons: %1").arg(mevent->buttons());

    qDebug() << msg1 << msg2 << msg3 << msg4 << msg5 << msg6;
}

void KisInputManager::Private::debugTabletEvent(QEvent *event)
{
    if (!logTabletEvents) return;

    QString msg1 = ignoreQtCursorEvents ? "[BLOCKED] " : "[       ] ";

    QTabletEvent *tevent = static_cast<QTabletEvent*>(event);

    QString msg2 =
        tevent->type() == QEvent::TabletPress ? "QEvent::TabletPress" :
        tevent->type() == QEvent::TabletRelease ? "QEvent::TabletRelease" :
        tevent->type() == QEvent::TabletMove ? "QEvent::TabletMove" : "unknown";

    QString msg3 = QString("gpos: %1,%2").arg(tevent->globalPos().x()).arg(tevent->globalPos().y());
    QString msg4 = QString("pos: %1,%2").arg(tevent->pos().x()).arg(tevent->pos().y());

    QString msg5 = QString("hres: %1,%2").arg(tevent->hiResGlobalPos().x()).arg(tevent->hiResGlobalPos().y());

    QString msg6 = QString(
        " Pressure: %1"
        " Rotation: %2"
        " Tangential pressure: %3"
        " Unique id: %4"
        " z: %5"
        " xTilt: %6"
        " yTilt: %7"
        )
        .arg(tevent->pressure())
        .arg(tevent->rotation())
        .arg(tevent->tangentialPressure())
        .arg(tevent->uniqueId())
        .arg(tevent->z())
        .arg(tevent->xTilt())
        .arg(tevent->yTilt());


    qDebug() << msg1 << msg2 << msg3 << msg4 << msg5 << msg6;
}

#define start_ignore_cursor_events() d->ignoreQtCursorEvents = true
#define stop_ignore_cursor_events() d->ignoreQtCursorEvents = false
#define break_if_should_ignore_cursor_events() if (d->ignoreQtCursorEvents) break;


static inline QList<Qt::Key> KEYS() {
    return QList<Qt::Key>();
}
static inline QList<Qt::Key> KEYS(Qt::Key key) {
    return QList<Qt::Key>() << key;
}
static inline QList<Qt::Key> KEYS(Qt::Key key1, Qt::Key key2) {
    return QList<Qt::Key>() << key1 << key2;
}
static inline QList<Qt::Key> KEYS(Qt::Key key1, Qt::Key key2, Qt::Key key3) {
    return QList<Qt::Key>() << key1 << key2 << key3;
}
static inline QList<Qt::MouseButton> BUTTONS(Qt::MouseButton button) {
    return QList<Qt::MouseButton>() << button;
}
static inline QList<Qt::MouseButton> BUTTONS(Qt::MouseButton button1, Qt::MouseButton button2) {
    return QList<Qt::MouseButton>() << button1 << button2;
}

class KisInputManager::Private::ProximityNotifier : public QObject {
public:
    ProximityNotifier(Private *_d, QObject *p) : QObject(p), d(_d) {}

    bool eventFilter(QObject* object, QEvent* event ) {
        switch (event->type()) {
        case QEvent::TabletEnterProximity:
            start_ignore_cursor_events();
            break;
        case QEvent::TabletLeaveProximity:
            stop_ignore_cursor_events();
            break;
        default:
            break;
        }
        return QObject::eventFilter(object, event);
    }

private:
    KisInputManager::Private *d;
};

void KisInputManager::Private::addStrokeShortcut(KisAbstractInputAction* action, int index,
                                                 const QList<Qt::Key> &modifiers,
                                                 Qt::MouseButtons buttons)
{
    KisStrokeShortcut *strokeShortcut =
            new KisStrokeShortcut(action, index);

    QList<Qt::MouseButton> buttonList;
    if(buttons & Qt::LeftButton) {
        buttonList << Qt::LeftButton;
    }
    if(buttons & Qt::RightButton) {
        buttonList << Qt::RightButton;
    }
    if(buttons & Qt::MidButton) {
        buttonList << Qt::MidButton;
    }
    if(buttons & Qt::XButton1) {
        buttonList << Qt::XButton1;
    }
    if(buttons & Qt::XButton2) {
        buttonList << Qt::XButton2;
    }

    if (buttonList.size() > 0) {
        strokeShortcut->setButtons(modifiers, buttonList);
        matcher.addShortcut(strokeShortcut);
    }
}

void KisInputManager::Private::addKeyShortcut(KisAbstractInputAction* action, int index,
                                              const QList<Qt::Key> &keys)
{
    if (keys.size() == 0) return;

    KisSingleActionShortcut *keyShortcut =
            new KisSingleActionShortcut(action, index);

    QList<Qt::Key> modifiers = keys.mid(1);
    keyShortcut->setKey(modifiers, keys.at(0));
    matcher.addShortcut(keyShortcut);
}

void KisInputManager::Private::addWheelShortcut(KisAbstractInputAction* action, int index,
                                                const QList<Qt::Key> &modifiers,
                                                KisShortcutConfiguration::MouseWheelMovement wheelAction)
{
    KisSingleActionShortcut *keyShortcut =
            new KisSingleActionShortcut(action, index);

    KisSingleActionShortcut::WheelAction a;
    switch(wheelAction) {
    case KisShortcutConfiguration::WheelUp:
        a = KisSingleActionShortcut::WheelUp;
        break;
    case KisShortcutConfiguration::WheelDown:
        a = KisSingleActionShortcut::WheelDown;
        break;
    case KisShortcutConfiguration::WheelLeft:
        a = KisSingleActionShortcut::WheelLeft;
        break;
    case KisShortcutConfiguration::WheelRight:
        a = KisSingleActionShortcut::WheelRight;
        break;
    default:
        return;
    }

    keyShortcut->setWheel(modifiers, a);
    matcher.addShortcut(keyShortcut);
}

void KisInputManager::Private::addTouchShortcut( KisAbstractInputAction* action, int index, KisShortcutConfiguration::GestureAction gesture)
{
    KisTouchShortcut *shortcut = new KisTouchShortcut(action, index);
    switch(gesture) {
        case KisShortcutConfiguration::PinchGesture:
            shortcut->setMinimumTouchPoints(2);
            shortcut->setMaximumTouchPoints(2);
            break;
        case KisShortcutConfiguration::PanGesture:
            shortcut->setMinimumTouchPoints(3);
            shortcut->setMaximumTouchPoints(10);
            break;
        default:
            break;
    }
    matcher.addShortcut(shortcut);
}

void KisInputManager::Private::setupActions()
{
    QList<KisAbstractInputAction*> actions = KisInputProfileManager::instance()->actions();
    foreach(KisAbstractInputAction *action, actions) {
        KisToolInvocationAction *toolAction =
            dynamic_cast<KisToolInvocationAction*>(action);

        if(toolAction) {
            defaultInputAction = toolAction;
        }
    }

    connect(KisInputProfileManager::instance(), SIGNAL(currentProfileChanged()), q, SLOT(profileChanged()));
    if(KisInputProfileManager::instance()->currentProfile()) {
        q->profileChanged();
    }
}

bool KisInputManager::Private::processUnhandledEvent(QEvent *event)
{
    bool retval = false;

    if (forwardAllEventsToTool ||
            event->type() == QEvent::KeyPress ||
            event->type() == QEvent::KeyRelease) {

        defaultInputAction->processUnhandledEvent(event);
        retval = true;
    }

    return retval && !forwardAllEventsToTool;
}

Qt::Key KisInputManager::Private::workaroundShiftAltMetaHell(const QKeyEvent *keyEvent)
{
    Qt::Key key = (Qt::Key)keyEvent->key();

    if (keyEvent->key() == Qt::Key_Meta &&
            keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {

        key = Qt::Key_Alt;
    }

    return key;
}

bool KisInputManager::Private::tryHidePopupPalette()
{
    if (canvas->favoriteResourceManager()->isPopupPaletteVisible()) {
        canvas->favoriteResourceManager()->slotShowPopupPalette();
        return true;
    }
    return false;
}

bool KisInputManager::Private::trySetMirrorMode(const QPointF &mousePosition)
{
    if (setMirrorMode) {
        canvas->resourceManager()->setResource(KisCanvasResourceProvider::MirrorAxisCenter, canvas->image()->documentToPixel(mousePosition));
        QApplication::restoreOverrideCursor();
        setMirrorMode = false;
        return true;
    }
    return false;
}

#ifdef Q_WS_X11
inline QPointF dividePoints(const QPointF &pt1, const QPointF &pt2) {
    return QPointF(pt1.x() / pt2.x(), pt1.y() / pt2.y());
}

inline QPointF multiplyPoints(const QPointF &pt1, const QPointF &pt2) {
    return QPointF(pt1.x() * pt2.x(), pt1.y() * pt2.y());
}
#endif

void KisInputManager::Private::saveTabletEvent(const QTabletEvent *event)
{
    delete lastTabletEvent;

#ifdef Q_WS_X11
    /**
     * There is a bug in Qt-x11 when working in 2 tablets + 2 monitors
     * setup. The hiResGlobalPos() value gets scaled wrongly somehow.
     * Happily, the error is linear (without the offset) so we can simply
     * scale it a bit.
     */
    if (event->type() == QEvent::TabletPress) {
        if ((event->globalPos() - event->hiResGlobalPos()).manhattanLength() > 4) {
            hiResEventsWorkaroundCoeff = dividePoints(event->globalPos(), event->hiResGlobalPos());
        } else {
            hiResEventsWorkaroundCoeff = QPointF(1.0, 1.0);
        }
    }
#endif

    lastTabletEvent =
            new QTabletEvent(event->type(),
                             event->pos(),
                             event->globalPos(),
                         #ifdef Q_WS_X11
                             multiplyPoints(event->hiResGlobalPos(), hiResEventsWorkaroundCoeff),
                         #else
                             event->hiResGlobalPos(),
                         #endif
                             event->device(),
                             event->pointerType(),
                             event->pressure(),
                             event->xTilt(),
                             event->yTilt(),
                             event->tangentialPressure(),
                             event->rotation(),
                             event->z(),
                             event->modifiers(),
                             event->uniqueId());
}

void KisInputManager::Private::saveTouchEvent( QTouchEvent* event )
{
    delete lastTouchEvent;
    lastTouchEvent = new QTouchEvent(event->type(), event->deviceType(), event->modifiers(), event->touchPointStates(), event->touchPoints());
}

void KisInputManager::Private::resetSavedTabletEvent(QEvent::Type type)
{
    bool needResetSavedEvent = true;

#ifdef Q_OS_WIN
    /**
     * For linux platform each mouse event corresponds to a single
     * tablet event so the saved tablet event is deleted after any
     * mouse event.
     *
     * For windows platform the mouse events get compressed so one
     * mouse event may correspond to a few tablet events, so we keep a
     * saved tablet event till the end of the stroke, that is till
     * mouseRelese event
     */
    needResetSavedEvent = type == QEvent::MouseButtonRelease;
#else
    Q_UNUSED(type);
#endif
    if (needResetSavedEvent) {
        delete lastTabletEvent;
        lastTabletEvent = 0;
    }
}

KisInputManager::KisInputManager(KisCanvas2 *canvas, KisToolProxy *proxy)
    : QObject(canvas), d(new Private(this))
{
    d->canvas = canvas;
    d->toolProxy = proxy;

    d->setupActions();

    /*
     * Temporary solution so we can still set the mirror axis.
     *
     * TODO: Create a proper interface for this.
     * There really should be a better way to handle this, one that neither
     * relies on "hidden" mouse interaction or shortcuts.
     */
    KAction *setMirrorAxis = new KAction(i18n("Set Mirror Axis"), this);
    d->canvas->view()->actionCollection()->addAction("set_mirror_axis", setMirrorAxis);
    setMirrorAxis->setShortcut(QKeySequence("Shift+r"));
    connect(setMirrorAxis, SIGNAL(triggered(bool)), SLOT(setMirrorAxis()));

    connect(KoToolManager::instance(), SIGNAL(changedTool(KoCanvasController*,int)),
            SLOT(slotToolChanged()));
    connect(&d->moveEventCompressor, SIGNAL(timeout()), SLOT(slotCompressedMoveEvent()));


    QApplication::instance()->
        installEventFilter(new Private::ProximityNotifier(d, this));
}

KisInputManager::~KisInputManager()
{
    delete d;
}

void KisInputManager::toggleTabletLogger()
{
    d->logTabletEvents = !d->logTabletEvents;
    QMessageBox::information(0, "Krita", d->logTabletEvents ? "Tablet Event Logging Enabled" :
                                                              "Tablet Event Logging Disabled");
    if (d->logTabletEvents) {
        qDebug() << "vvvvvvvvvvvvvvvvvvvvvvv START TABLET EVENT LOG vvvvvvvvvvvvvvvvvvvvvvv";
    }
    else {
        qDebug() << "^^^^^^^^^^^^^^^^^^^^^^^ START TABLET EVENT LOG ^^^^^^^^^^^^^^^^^^^^^^^";
    }
}

bool KisInputManager::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    bool retval = false;

    // KoToolProxy needs to pre-process some events to ensure the
    // global shortcuts (not the input manager's ones) are not
    // executed, in particular, this line will accept events when the
    // tool is in text editing, preventing shortcut triggering
    d->toolProxy->processEvent(event);

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick: {
        d->debugMouseEvent(event);
        break_if_should_ignore_cursor_events();

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (d->tryHidePopupPalette() || d->trySetMirrorMode(widgetToDocument(mouseEvent->posF()))) {
            retval = true;
        } else {
            //Make sure the input actions know we are active.
            KisAbstractInputAction::setInputManager(this);
            retval = d->matcher.buttonPressed(mouseEvent->button(), mouseEvent);
        }
        d->resetSavedTabletEvent(event->type());
        event->setAccepted(retval);
        break;
    }
    case QEvent::MouseButtonRelease: {
        d->debugMouseEvent(event);
        break_if_should_ignore_cursor_events();

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        retval = d->matcher.buttonReleased(mouseEvent->button(), mouseEvent);
        d->resetSavedTabletEvent(event->type());
        event->setAccepted(retval);
        break;
    }
    case QEvent::KeyPress: {
        if (d->logTabletEvents) qDebug() << "KeyPress";
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        Qt::Key key = d->workaroundShiftAltMetaHell(keyEvent);

        if (!keyEvent->isAutoRepeat()) {
            retval = d->matcher.keyPressed(key);
        } else {
            retval = d->matcher.autoRepeatedKeyPressed(key);
        }

        /**
         * Workaround for temporary switching of tools by
         * KoCanvasControllerWidget. We don't need this switch because
         * we handle it ourselves.
         */
        retval |= !d->forwardAllEventsToTool &&
                (keyEvent->key() == Qt::Key_Space ||
                 keyEvent->key() == Qt::Key_Escape);

        break;
    }
    case QEvent::KeyRelease: {
        if (d->logTabletEvents) qDebug() << "KeyRelease";
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (!keyEvent->isAutoRepeat()) {
            Qt::Key key = d->workaroundShiftAltMetaHell(keyEvent);
            retval = d->matcher.keyReleased(key);
        }
        break;
    }
    case QEvent::MouseMove: {
        d->debugMouseEvent(event);
        break_if_should_ignore_cursor_events();

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (!d->matcher.mouseMoved(mouseEvent)) {
            //Update the current tool so things like the brush outline gets updated.
            d->toolProxy->mouseMoveEvent(mouseEvent, widgetToDocument(mouseEvent->posF()));
        }
        retval = true;
        event->setAccepted(retval);
        d->resetSavedTabletEvent(event->type());
        break;
    }
    case QEvent::Wheel: {
        if (d->logTabletEvents) qDebug() << "Wheel";
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        KisSingleActionShortcut::WheelAction action;

        if(wheelEvent->orientation() == Qt::Horizontal) {
            if(wheelEvent->delta() < 0) {
                action = KisSingleActionShortcut::WheelRight;
            }
            else {
                action = KisSingleActionShortcut::WheelLeft;
            }
        }
        else {
            if(wheelEvent->delta() > 0) {
                action = KisSingleActionShortcut::WheelUp;
            }
            else {
                action = KisSingleActionShortcut::WheelDown;
            }
        }

        //Make sure the input actions know we are active.
        KisAbstractInputAction::setInputManager(this);
        retval = d->matcher.wheelEvent(action, wheelEvent);
        break;
    }
    case QEvent::Enter:
        //Make sure the input actions know we are active.
        KisAbstractInputAction::setInputManager(this);
        //Ensure we have focus so we get key events.
        d->canvas->canvasWidget()->setFocus();
        stop_ignore_cursor_events();
        break;
    case QEvent::Leave:
        /**
         * We won't get a TabletProximityLeave event when the tablet
         * is hovering above some other widget, so restore cursor
         * events processing right now.
         */
        stop_ignore_cursor_events();
        break;
    case QEvent::FocusIn:
        //Clear all state so we don't have half-matched shortcuts dangling around.
        d->matcher.reset();
        stop_ignore_cursor_events();
        break;
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        d->debugTabletEvent(event);
        break_if_should_ignore_cursor_events();
        //We want both the tablet information and the mouse button state.
        //Since QTabletEvent only provides the tablet information, we
        //save that and then ignore the event so it will generate a mouse
        //event.
        QTabletEvent* tabletEvent = static_cast<QTabletEvent*>(event);
        d->saveTabletEvent(tabletEvent);
        event->ignore();

        break;
    }
    case KisTabletEvent::TabletPressEx:
    case KisTabletEvent::TabletMoveEx:
    case KisTabletEvent::TabletReleaseEx: {

        stop_ignore_cursor_events();

        KisTabletEvent *tevent = static_cast<KisTabletEvent*>(event);

        if (tevent->type() == (QEvent::Type)KisTabletEvent::TabletMoveEx &&
            !d->matcher.supportsHiResInputEvents()) {

            d->compressedMoveEvent.reset(new KisTabletEvent(*tevent));
            d->moveEventCompressor.start();
            retval = true;
        } else {
            slotCompressedMoveEvent();
            retval = d->handleKisTabletEvent(object, tevent);
        }

        /**
         * The flow of tablet events means the tablet is in the
         * proximity area, so activate it even when the
         * TabletEnterProximity event was missed (may happen when
         * changing focus of the window with tablet in the proximity
         * area)
         */
        start_ignore_cursor_events();

        break;
    }

    case QEvent::TouchBegin:
        KisAbstractInputAction::setInputManager(this);

        retval = d->matcher.touchBeginEvent(static_cast<QTouchEvent*>(event));
        event->accept();
        d->resetSavedTabletEvent(event->type());
        break;
    case QEvent::TouchUpdate:
        KisAbstractInputAction::setInputManager(this);

        retval = d->matcher.touchUpdateEvent(static_cast<QTouchEvent*>(event));
        event->accept();
        d->resetSavedTabletEvent(event->type());
        break;
    case QEvent::TouchEnd:
        d->saveTouchEvent(static_cast<QTouchEvent*>(event));
        retval = d->matcher.touchEndEvent(static_cast<QTouchEvent*>(event));
        event->accept();
        d->resetSavedTabletEvent(event->type());
        delete d->lastTouchEvent;
        d->lastTouchEvent = 0;
        break;
    default:
        break;
    }

    return !retval ? d->processUnhandledEvent(event) : true;
}

bool KisInputManager::Private::handleKisTabletEvent(QObject *object, KisTabletEvent *tevent)
{
    bool retval = false;

    QTabletEvent qte = tevent->toQTabletEvent();
    qte.ignore();
    retval = q->eventFilter(object, &qte);
    tevent->setAccepted(qte.isAccepted());

    if (!retval && !qte.isAccepted()) {
        QMouseEvent qme = tevent->toQMouseEvent();
        qme.ignore();
        retval = q->eventFilter(object, &qme);
        tevent->setAccepted(qme.isAccepted());
    }

    return retval;
}

void KisInputManager::slotCompressedMoveEvent()
{
    if (d->compressedMoveEvent) {
        (void) d->handleKisTabletEvent(this, d->compressedMoveEvent.data());
        d->compressedMoveEvent.reset();
    }
}

KisCanvas2* KisInputManager::canvas() const
{
    return d->canvas;
}

KisToolProxy* KisInputManager::toolProxy() const
{
    return d->toolProxy;
}

QTabletEvent* KisInputManager::lastTabletEvent() const
{
    return d->lastTabletEvent;
}

QTouchEvent *KisInputManager::lastTouchEvent() const
{
    return d->lastTouchEvent;
}

void KisInputManager::setMirrorAxis()
{
    d->setMirrorMode = true;
    QApplication::setOverrideCursor(Qt::CrossCursor);
}

void KisInputManager::slotToolChanged()
{
    QString toolId = KoToolManager::instance()->activeToolId();
    if (toolId == "ArtisticTextToolFactoryID" || toolId == "TextToolFactory_ID") {
        d->forwardAllEventsToTool = true;
        d->matcher.suppressAllActions(true);
    } else {
        d->forwardAllEventsToTool = false;
        d->matcher.suppressAllActions(false);
    }
}

QPointF KisInputManager::widgetToDocument(const QPointF& position)
{
    QPointF pixel = QPointF(position.x() + 0.5f, position.y() + 0.5f);
    return d->canvas->coordinatesConverter()->widgetToDocument(pixel);
}

void KisInputManager::profileChanged()
{
    d->matcher.reset();
    d->matcher.clearShortcuts();

    KisInputProfile *profile = KisInputProfileManager::instance()->currentProfile();
    if (profile) {
        QList<KisShortcutConfiguration*> shortcuts = profile->allShortcuts();
        foreach(KisShortcutConfiguration *shortcut, shortcuts) {
            switch(shortcut->type()) {
            case KisShortcutConfiguration::KeyCombinationType:
                d->addKeyShortcut(shortcut->action(), shortcut->mode(), shortcut->keys());
                break;
            case KisShortcutConfiguration::MouseButtonType:
                d->addStrokeShortcut(shortcut->action(), shortcut->mode(), shortcut->keys(), shortcut->buttons());
                break;
            case KisShortcutConfiguration::MouseWheelType:
                d->addWheelShortcut(shortcut->action(), shortcut->mode(), shortcut->keys(), shortcut->wheel());
                break;
            case KisShortcutConfiguration::GestureType:
                d->addTouchShortcut(shortcut->action(), shortcut->mode(), shortcut->gesture());
                break;
            default:
                break;
            }
        }
    }
    else {
        kWarning() << "No Input Profile Found: canvas interaction will be impossible";

    }
}

