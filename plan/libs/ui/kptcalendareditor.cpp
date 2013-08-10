/* This file is part of the KDE project
  Copyright (C) 2007, 2012 Dag Andersen <danders@get2net>

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

#include "kptcalendareditor.h"

#include "kcalendar/kdatepicker.h"
#include "kcalendar/kdatetable.h"
//#include "kptcalendarpanel.h"
#include "kptcommand.h"
#include "kptcalendarmodel.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kptintervaledit.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QDragMoveEvent>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>

#include <kaction.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kcalendarsystem.h>
#include <kactioncollection.h>
#include <kxmlguifactory.h>
#include <ksystemtimezone.h>
#include <ktimezone.h>
#include <kmenu.h>

#include <KoDocument.h>


namespace KPlato
{


//--------------------
CalendarTreeView::CalendarTreeView( QWidget *parent )
    : TreeViewBase( parent )
{
    header()->setContextMenuPolicy( Qt::CustomContextMenu );
    setModel( new CalendarItemModel() );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionModel( new QItemSelectionModel( model() ) );

    setItemDelegateForColumn( 1, new EnumDelegate( this ) ); // timezone

    connect( header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)) );
}

void CalendarTreeView::headerContextMenuRequested( const QPoint &pos )
{
    kDebug(planDbg())<<header()->logicalIndexAt(pos)<<" at"<<pos;
}

void CalendarTreeView::contextMenuEvent ( QContextMenuEvent *event )
{
    //kDebug(planDbg());
    emit contextMenuRequested( indexAt(event->pos()), event->globalPos() );
}

void CalendarTreeView::focusInEvent ( QFocusEvent *event )
{
    //kDebug(planDbg());
    TreeViewBase::focusInEvent( event );
    emit focusChanged();
}

void CalendarTreeView::focusOutEvent ( QFocusEvent * event )
{
    //kDebug(planDbg());
    TreeViewBase::focusInEvent( event );
    emit focusChanged();
}

void CalendarTreeView::selectionChanged( const QItemSelection &sel, const QItemSelection &desel )
{
    //kDebug(planDbg())<<sel.indexes().count();
    //foreach( const QModelIndex &i, selectionModel()->selectedIndexes() ) { kDebug(planDbg())<<i.row()<<","<<i.column(); }
    TreeViewBase::selectionChanged( sel, desel );
    emit selectionChanged( selectionModel()->selectedIndexes() );
}

void CalendarTreeView::currentChanged( const QModelIndex & current, const QModelIndex & previous )
{
    //kDebug(planDbg());
    TreeViewBase::currentChanged( current, previous );
    // possible bug in qt: in QAbstractItemView::SingleSelection you can select multiple items/rows
    selectionModel()->select( current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    emit currentChanged( current );
}

Calendar *CalendarTreeView::currentCalendar() const
{
    return model()->calendar( currentIndex() );
}

Calendar *CalendarTreeView::selectedCalendar() const
{
    QModelIndexList lst = selectionModel()->selectedRows();
    if ( lst.count() == 1 ) {
        return model()->calendar( lst.first() );
    }
    return 0;
}

QList<Calendar*> CalendarTreeView::selectedCalendars() const
{
    QList<Calendar *> lst;
    foreach ( const QModelIndex &i, selectionModel()->selectedRows() ) {
        Calendar *a = model()->calendar( i );
        if ( a ) {
            lst << a;
        }
    }
    return lst;
}

void CalendarTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    if (dragDropMode() == InternalMove && (event->source() != this || !(event->possibleActions() & Qt::MoveAction))) {
        return;
    }
    TreeViewBase::dragMoveEvent( event );
    if ( ! event->isAccepted() ) {
        return;
    }
    // QTreeView thinks it's ok to drop, but it might not be...
    event->ignore();
    QModelIndex index = indexAt( event->pos() );
    if ( ! index.isValid() ) {
        if ( model()->dropAllowed( 0, event->mimeData() ) ) {
            event->accept();
        }
        return;
    }
    Calendar *c = model()->calendar( index );
    if ( c == 0 ) {
        kError()<<"no calendar to drop on!";
        return; // hmmm
    }
    switch ( dropIndicatorPosition() ) {
        case AboveItem:
        case BelowItem:
            // c == sibling
            // if siblings parent is me or child of me: illegal
            if ( model()->dropAllowed( c->parentCal(), event->mimeData() ) ) {
                event->accept();
            }
            break;
        case OnItem:
            // c == new parent
            if ( model()->dropAllowed( c, event->mimeData() ) ) {
                event->accept();
            }
            break;
        default:
            break;
    }
}

//--------------------
CalendarDayView::CalendarDayView( QWidget *parent )
    : QTableView( parent ),
    m_readwrite( false )
{
    setTabKeyNavigation( false );
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    horizontalHeader()->setResizeMode( QHeaderView::Stretch );
    m_model = new CalendarDayItemModel( this );
    setModel(m_model);
    verticalHeader()->hide();

    actionSetWork = new KAction( i18n( "Work..." ), this );
    connect( actionSetWork, SIGNAL(triggered(bool)), SLOT(slotSetWork()) );
    actionSetVacation = new KAction( i18n( "Non-working" ), this );
    connect( actionSetVacation, SIGNAL(triggered(bool)), SLOT(slotSetVacation()) );
    actionSetUndefined = new KAction( i18n( "Undefined" ), this );
    connect( actionSetUndefined, SIGNAL(triggered(bool)), SLOT(slotSetUndefined()) );
}

QSize CalendarDayView::sizeHint() const
{
    QSize s = QTableView::sizeHint();
    s.setHeight( horizontalHeader()->height() + rowHeight( 0 ) + frameWidth() * 2 );
    return s;
}

void CalendarDayView::slotSetWork()
{
    kDebug(planDbg());
    if ( receivers( SIGNAL(executeCommand(KUndo2Command*)) ) == 0 ) {
        return;
    }
    Calendar *cal = model()->calendar();
    if ( cal == 0 ) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if ( lst.isEmpty() ) {
        lst << currentIndex();
    }
    if ( lst.isEmpty() ) {
        return;
    }
    QList<CalendarDay*> days;
    foreach ( const QModelIndex &i, lst ) {
        CalendarDay *day = model()->day( i );
        if ( day == 0 ) {
            continue;
        }
        days << day;
    }
    IntervalEditDialog *dlg = new IntervalEditDialog( cal, days, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotIntervalEditDialogFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

void CalendarDayView::slotIntervalEditDialogFinished( int result )
{
    IntervalEditDialog *dlg = qobject_cast<IntervalEditDialog*>( sender() );
    if ( dlg == 0 ) {
        return;
    }
    if ( result == QDialog::Accepted ) {
        MacroCommand *cmd = dlg->buildCommand();
        if ( cmd ) {
            emit executeCommand( cmd );
        }
    }
    dlg->deleteLater();
}

void CalendarDayView::slotSetVacation()
{
    kDebug(planDbg());
    if ( receivers( SIGNAL(executeCommand(KUndo2Command*)) ) == 0 ) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if ( lst.isEmpty() ) {
        lst << currentIndex();
    }
    if ( lst.isEmpty() ) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify Weekday State" ) );
    foreach ( const QModelIndex &i, lst ) {
        CalendarDay *day = model()->day( i );
        if ( day == 0 || day->state() == CalendarDay::NonWorking ) {
            continue;
        }
        mod = true;
        m->addCommand( new CalendarModifyStateCmd( model()->calendar(), day, CalendarDay::NonWorking ) );
    }
    if ( mod ) {
        emit executeCommand( m );
    } else {
        delete m;
    }
}

void CalendarDayView::slotSetUndefined()
{
    kDebug(planDbg());
    if ( receivers( SIGNAL(executeCommand(KUndo2Command*)) ) == 0 ) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if ( lst.isEmpty() ) {
        lst << currentIndex();
    }
    if ( lst.isEmpty() ) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify Weekday State" ) );
    foreach ( const QModelIndex &i, lst ) {
        CalendarDay *day = model()->day( i );
        if ( day == 0 || day->state() == CalendarDay::Undefined ) {
            continue;
        }
        mod = true;
        m->addCommand( new CalendarModifyStateCmd( model()->calendar(), day, CalendarDay::Undefined ) );
    }
    if ( mod ) {
        emit executeCommand( m );
    } else {
        delete m;
    }
}

void CalendarDayView::setCurrentCalendar( Calendar *calendar )
{
    model()->setCalendar( calendar );
}

void CalendarDayView::headerContextMenuRequested( const QPoint &/*pos*/ )
{
//    kDebug(planDbg())<<header()->logicalIndexAt(pos)<<" at"<<pos;
}

void CalendarDayView::contextMenuEvent ( QContextMenuEvent *event )
{
    //kDebug(planDbg());
    if ( ! isReadWrite() ) {
        return;
    }
    KMenu menu;
    menu.addAction( actionSetWork );
    menu.addAction( actionSetVacation );
    menu.addAction( actionSetUndefined );

    menu.exec( event->globalPos(), actionSetWork );

    //emit contextMenuRequested( indexAt(event->pos()), event->globalPos() );
}

void CalendarDayView::focusInEvent ( QFocusEvent *event )
{
    //kDebug(planDbg());
    QTableView::focusInEvent( event );
    emit focusChanged();
}

void CalendarDayView::focusOutEvent ( QFocusEvent * event )
{
    //kDebug(planDbg());
    QTableView::focusInEvent( event );
    emit focusChanged();
}

void CalendarDayView::selectionChanged( const QItemSelection &sel, const QItemSelection &desel )
{
    //kDebug(planDbg())<<sel.indexes().count();
    //foreach( QModelIndex i, selectionModel()->selectedIndexes() ) { kDebug(planDbg())<<i.row()<<","<<i.column(); }
    QTableView::selectionChanged( sel, desel );
    emit selectionChanged( selectionModel()->selectedIndexes() );
}

void CalendarDayView::currentChanged( const QModelIndex & current, const QModelIndex & previous )
{
    //kDebug(planDbg());
    QTableView::currentChanged( current, previous );
//    selectionModel()->select( current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    emit currentChanged( current );
}

CalendarDay *CalendarDayView::selectedDay() const
{
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if ( lst.count() == 1 ) {
        return model()->day( lst.first() );
    }
    return 0;
}

//-----------------------------------
CalendarEditor::CalendarEditor(KoPart *part, KoDocument *doc, QWidget *parent )
    : ViewBase(part, doc, parent ),
    m_model( new DateTableDataModel( this ) )
{
    setWhatsThis( i18nc( "@info:whatsthis",
        "<title>Work & Vacation Editor</title>"
        "<para>"
        "A calendar defines availability for resources or tasks of type <emphasis>Duration</emphasis>. "
        "A calendar can be specific to a resource or task, or shared by multiple resources or tasks. "
        "A day can be of type <emphasis>Undefined</emphasis>, <emphasis>Non-working day</emphasis> or <emphasis>Working day</emphasis>. "
        "A working day has one or more work intervals defined. "
        "</para><para>"
        "A calendar can have sub calendars. If a day is undefined in a calendar, the parent calendar is checked. "
        "An <emphasis>Undefined</emphasis> day defaults to <emphasis>Non-working</emphasis> if used by a resource, or <emphasis>available all day</emphasis> if used by a task."
        "</para><para>"
        "A calendar can be defined as the <emphasis>Default calendar</emphasis>. "
        "The default calendar is used by a working resource, when the resources calendar is not explicitly set."
        "</para>"
        ) );

    setupGui();

    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin( 0 );
    QSplitter *sp = new QSplitter( this );
    l->addWidget( sp );

    m_calendarview = new CalendarTreeView( sp );

    QFrame *f = new QFrame( sp );
    l = new QVBoxLayout( f );
    l->setMargin( 0 );

    m_dayview = new CalendarDayView( f );
    l->addWidget( m_dayview );

    sp = new QSplitter( f );
    l->addWidget( sp );
    m_datePicker = new KDatePicker( sp );
    m_datePicker->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    m_datePicker->dateTable()->setWeekNumbersEnabled( true );
    m_datePicker->dateTable()->setGridEnabled( true );
    m_datePicker->dateTable()->setSelectionMode( KDateTable::ExtendedSelection );
    m_datePicker->dateTable()->setDateDelegate( new DateTableDateDelegate( m_datePicker->dateTable() ) );
    m_datePicker->dateTable()->setModel( m_model );
    m_datePicker->dateTable()->setPopupMenuEnabled( true );

    m_calendarview->setDragDropMode( QAbstractItemView::InternalMove );
    m_calendarview->setDropIndicatorShown( true );
    m_calendarview->setDragEnabled ( true );
    m_calendarview->setAcceptDrops( true );
    m_calendarview->setAcceptDropsOnView( true );

    connect( m_datePicker->dateTable(), SIGNAL(aboutToShowContextMenu(KMenu*,QDate)), SLOT(slotContextMenuDate(KMenu*,QDate)) );
    connect( m_datePicker->dateTable(), SIGNAL(aboutToShowContextMenu(KMenu*,QList<QDate>)), SLOT(slotContextMenuDate(KMenu*,QList<QDate>)) );

/*    const QDate date(2007,7,19);
    const QColor fgColor(Qt::darkGray);
    KDateTable::BackgroundMode bgMode = KDateTable::CircleMode;
    const QColor bgColor( Qt::lightGray);
    m_datePicker->dateTable()->setCustomDatePainting( date, fgColor, bgMode, bgColor );*/


    m_calendarview->setEditTriggers( m_calendarview->editTriggers() | QAbstractItemView::EditKeyPressed );

    m_dayview->setEditTriggers( m_dayview->editTriggers() | QAbstractItemView::EditKeyPressed );

    m_calendarview->setDragDropMode( QAbstractItemView::InternalMove );
    m_calendarview->setDropIndicatorShown ( true );
    m_calendarview->setDragEnabled ( true );
    m_calendarview->setAcceptDrops( true );

    connect( m_calendarview->model(), SIGNAL(executeCommand(KUndo2Command*)), doc, SLOT(addCommand(KUndo2Command*)) );
    connect( m_dayview->model(), SIGNAL(executeCommand(KUndo2Command*)), doc, SLOT(addCommand(KUndo2Command*)) );
    connect( m_dayview, SIGNAL(executeCommand(KUndo2Command*)), doc, SLOT(addCommand(KUndo2Command*)) );

    connect( m_calendarview, SIGNAL(currentChanged(QModelIndex)), this, SLOT(slotCurrentCalendarChanged(QModelIndex)) );
    connect( m_calendarview, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotCalendarSelectionChanged(QModelIndexList)) );
    connect( m_calendarview, SIGNAL(contextMenuRequested(QModelIndex,QPoint)), this, SLOT(slotContextMenuCalendar(QModelIndex,QPoint)) );

    connect( m_dayview, SIGNAL(currentChanged(QModelIndex)), this, SLOT(slotCurrentDayChanged(QModelIndex)) );
    connect( m_dayview, SIGNAL(selectionChanged(QModelIndexList)), this, SLOT(slotDaySelectionChanged(QModelIndexList)) );
    connect( m_dayview, SIGNAL(contextMenuRequested(QModelIndex,QPoint)), this, SLOT(slotContextMenuDay(QModelIndex,QPoint)) );

    connect( m_dayview->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(slotEnableActions()) );

    connect( m_calendarview, SIGNAL(focusChanged()), this, SLOT(slotEnableActions()) );
    connect( m_dayview, SIGNAL(focusChanged()), this, SLOT(slotEnableActions()) );

}

void CalendarEditor::draw( Project &project )
{
    m_calendarview->setProject( &project );
    m_dayview->setProject( &project );
}

void CalendarEditor::draw()
{
}

void CalendarEditor::setGuiActive( bool activate )
{
    //kDebug(planDbg())<<activate;
    updateActionsEnabled( true );
    ViewBase::setGuiActive( activate );
    if ( activate ) {
        if ( !m_calendarview->currentIndex().isValid() ) {
            m_calendarview->selectionModel()->setCurrentIndex(m_calendarview->model()->index( 0, 0 ), QItemSelectionModel::NoUpdate);
        }
        //slotSelectionChanged( m_calendarview->selectionModel()->selectedRows() );
    }
}

void CalendarEditor::slotContextMenuDate( KMenu *menu, const QList<QDate> &dates )
{
    if ( ! isReadWrite() ) {
        return;
    }
    if ( dates.isEmpty() ) {
        m_currentMenuDateList << m_datePicker->date();
    } else {
        m_currentMenuDateList = dates;
    }
    menu->addAction( actionSetWork );
    menu->addAction( actionSetVacation );
    menu->addAction( actionSetUndefined );
}

void CalendarEditor::slotContextMenuDate( KMenu *menu, const QDate &date )
{
    kDebug(planDbg())<<menu<<date;
    if ( ! isReadWrite() || ! date.isValid() ) {
        return;
    }
    m_currentMenuDateList << date;
    menu->addAction( actionSetWork );
    menu->addAction( actionSetVacation );
    menu->addAction( actionSetUndefined );
}

void CalendarEditor::slotContextMenuCalendar( const QModelIndex &/*index*/, const QPoint& pos )
{
    if ( ! isReadWrite() ) {
        return;
    }
    //kDebug(planDbg())<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
/*    if ( index.isValid() ) {
        Calendar *a = m_calendarview->model()->calendar( index );
        if ( a ) {
            name = "calendareditor_calendar_popup";
        }
    }*/
    //kDebug(planDbg())<<name;
    if ( name.isEmpty() ) {
        return;
    }
    emit requestPopupMenu( name, pos );
}

void CalendarEditor::slotContextMenuDay( const QModelIndex &index, const QPoint& pos )
{
    if ( ! isReadWrite() ) {
        return;
    }
    kDebug(planDbg())<<index.row()<<","<<index.column()<<":"<<pos;
/*    QString name;
    if ( index.isValid() ) {
        if ( m_dayview->model()->day( index ) ) {
            name = "calendareditor_day_popup";
        }
    }
    kDebug(planDbg())<<name;
    if ( name.isEmpty() ) {
        return;
    }
    emit requestPopupMenu( name, pos );*/
}

Calendar *CalendarEditor::currentCalendar() const
{
    return m_calendarview->currentCalendar();
}

void CalendarEditor::slotCurrentCalendarChanged(  const QModelIndex & )
{
    //kDebug(planDbg())<<curr.row()<<","<<curr.column();
    m_dayview->setCurrentCalendar( currentCalendar() );
    if ( m_model ) {
        m_model->setCalendar( currentCalendar() );
    }
}

void CalendarEditor::slotCalendarSelectionChanged( const QModelIndexList& /*list */)
{
    //kDebug(planDbg())<<list.count();
    updateActionsEnabled( true );
}

void CalendarEditor::slotCurrentDayChanged(  const QModelIndex & )
{
    //kDebug(planDbg())<<curr.row()<<","<<curr.column();
}

void CalendarEditor::slotDaySelectionChanged( const QModelIndexList& )
{
    //kDebug(planDbg())<<list.count();
    updateActionsEnabled( true );
}

void CalendarEditor::slotEnableActions()
{
    updateActionsEnabled( true );
}

void CalendarEditor::updateActionsEnabled(  bool on )
{
    QList<Calendar *> lst = m_calendarview->selectedCalendars();
    bool one = lst.count() == 1;
    bool more = lst.count() > 1;
    actionAddCalendar ->setEnabled( on && !more );
    actionAddSubCalendar ->setEnabled( on && one );
    actionDeleteSelection->setEnabled( on && ( one || more ) );
}

void CalendarEditor::setupGui()
{
    KActionCollection *coll = actionCollection();
    QString name = "calendareditor_calendar_list";

    actionAddCalendar   = new KAction(koIcon("resource-calendar-insert"), i18n("Add Calendar"), this);
    coll->addAction("add_calendar", actionAddCalendar  );
    actionAddCalendar ->setShortcut( KShortcut( Qt::CTRL + Qt::Key_I ) );
    connect( actionAddCalendar , SIGNAL(triggered(bool)), SLOT(slotAddCalendar()) );

    actionAddSubCalendar   = new KAction(koIcon("resource-calendar-child-insert"), i18n("Add Subcalendar"), this);
    coll->addAction("add_subcalendar", actionAddSubCalendar  );
    actionAddSubCalendar ->setShortcut( KShortcut( Qt::SHIFT + Qt::CTRL + Qt::Key_I ) );
    connect( actionAddSubCalendar , SIGNAL(triggered(bool)), SLOT(slotAddSubCalendar()) );

    actionDeleteSelection  = new KAction(koIcon("edit-delete"), i18nc("@action", "Delete"), this);
    coll->addAction("delete_calendar_selection", actionDeleteSelection );
    actionDeleteSelection->setShortcut( KShortcut( Qt::Key_Delete ) );
    connect( actionDeleteSelection, SIGNAL(triggered(bool)), SLOT(slotDeleteCalendar()) );

    addAction( name, actionAddCalendar  );
    addAction( name, actionAddSubCalendar  );
    addAction( name, actionDeleteSelection );

    actionSetWork = new KAction( i18n( "Work..." ), this );
    connect( actionSetWork, SIGNAL(triggered(bool)), SLOT(slotSetWork()) );
    actionSetVacation = new KAction( i18n( "Non-working" ), this );
    connect( actionSetVacation, SIGNAL(triggered(bool)), SLOT(slotSetVacation()) );
    actionSetUndefined = new KAction( i18n( "Undefined" ), this );
    connect( actionSetUndefined, SIGNAL(triggered(bool)), SLOT(slotSetUndefined()) );

}

void CalendarEditor::updateReadWrite( bool readwrite )
{
    m_calendarview->setReadWrite( readwrite );
    m_dayview->setReadWrite( readwrite );
    ViewBase::updateReadWrite( readwrite );
}

void CalendarEditor::slotAddCalendar ()
{
    //kDebug(planDbg());
    // get parent through sibling
    Calendar *cal = m_calendarview->selectedCalendar();
    Calendar *parent = cal ? cal->parentCal() : 0;
    int pos = parent ? parent->indexOf( cal ) : project()->indexOf( cal );
    if ( pos >= 0 ) {
        ++pos; // after selected calendar
    }
    insertCalendar ( new Calendar(), parent, pos );
}

void CalendarEditor::slotAddSubCalendar ()
{
    //kDebug(planDbg());
    insertCalendar ( new Calendar (), m_calendarview->selectedCalendar () );
}

void CalendarEditor::insertCalendar ( Calendar *calendar, Calendar *parent, int pos )
{
    m_calendarview->closePersistentEditor( m_calendarview->selectionModel()->currentIndex() );
    QModelIndex i = m_calendarview->model()->insertCalendar ( calendar, pos, parent );
    if ( i.isValid() ) {
        QModelIndex p = m_calendarview->model()->parent( i );
        //if (parent) kDebug(planDbg())<<" parent="<<parent->name()<<":"<<p.row()<<","<<p.column();
        //kDebug(planDbg())<<i.row()<<","<<i.column();
        m_calendarview->setExpanded( p, true );
        m_calendarview->setCurrentIndex( i );
        m_calendarview->edit( i );
    }
}

void CalendarEditor::slotDeleteCalendar()
{
    //kDebug(planDbg());
    m_calendarview->model()->removeCalendar( m_calendarview->selectedCalendar() );
}

void CalendarEditor::slotAddInterval ()
{
    //kDebug(planDbg());
/*    CalendarDay *parent = m_dayview->selectedDay ();
    if ( parent == 0 ) {
        TimeInterval *ti = m_dayview->selectedInterval();
        if ( ti == 0 ) {
            return;
        }
        parent = m_dayview->model()->parentDay( ti );
        if ( parent == 0 ) {
            return;
        }
    }
    QModelIndex i = m_dayview->model()->insertInterval( new TimeInterval(), parent );
    if ( i.isValid() ) {
        QModelIndex p = m_dayview->model()->index( parent );
        m_dayview->setExpanded( p, true );
        m_dayview->setCurrentIndex( i );
        m_dayview->edit( i );
    }*/
}

void CalendarEditor::slotDeleteDaySelection()
{
    //kDebug(planDbg());
/*    TimeInterval *ti = m_dayview->selectedInterval();
    if ( ti != 0 ) {
        m_dayview->model()->removeInterval( ti );
        return;
    }
    CalendarDay *day = m_dayview->selectedDay();
    if ( day != 0 ) {
        m_dayview->model()->removeDay( day );
    }*/
}

void CalendarEditor::slotAddDay ()
{
    //kDebug(planDbg());
/*    Calendar *c = currentCalendar();
    if ( c == 0 ) {
        return;
    }
    QDate date = QDate::currentDate();
    while ( c->day( date ) ) {
        date = date.addDays( 1 );
    }
    QModelIndex i = m_dayview->model()->insertDay( new CalendarDay(date,  CalendarDay::NonWorking ) );
    if ( i.isValid() ) {
        QModelIndex p = m_dayview->model()->parent( i );
        m_dayview->setExpanded( p, true );
        m_dayview->setCurrentIndex( i );
        m_dayview->edit( i );
    }*/
}

void CalendarEditor::slotSetWork()
{
    kDebug(planDbg())<<currentCalendar()<<m_currentMenuDateList;
    if ( currentCalendar() == 0 || m_currentMenuDateList.isEmpty() ) {
        return;
    }
    IntervalEditDialog *dlg = new IntervalEditDialog( currentCalendar(), m_currentMenuDateList, this );
    connect(dlg, SIGNAL(finished(int)), SLOT(slotIntervalEditDialogFinished(int)));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    m_currentMenuDateList.clear();
}

void CalendarEditor::slotIntervalEditDialogFinished( int result )
{
    IntervalEditDialog *dlg = qobject_cast<IntervalEditDialog*>( sender() );
    if ( dlg == 0 ) {
        return;
    }
    if ( result == QDialog::Accepted ) {
        MacroCommand *cmd = dlg->buildCommand();
        if ( cmd ) {
            part()->addCommand( cmd );
        }
    }
    dlg->deleteLater();
}

void CalendarEditor::slotSetVacation()
{
    kDebug(planDbg())<<m_currentMenuDateList;
    if ( m_currentMenuDateList.isEmpty() || currentCalendar() == 0 ) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify Calendar" ) );
    foreach ( const QDate &date, m_currentMenuDateList ) {
        kDebug(planDbg())<<"handle:"<<date;
        CalendarDay *day = currentCalendar()->findDay( date );
        if ( day == 0 ) {
            mod = true;
            day = new CalendarDay( date, CalendarDay::NonWorking );
            m->addCommand( new CalendarAddDayCmd( currentCalendar(), day ) );
            if ( m_currentMenuDateList.count() == 1 ) {
                m->setText( i18n( "%1: Set to Non-Working", date.toString() ) );
            }
        } else if ( day->state() != CalendarDay::NonWorking ) {
            mod = true;
            m->addCommand( new CalendarModifyStateCmd( currentCalendar(), day, CalendarDay::NonWorking ) );
            if ( m_currentMenuDateList.count() == 1 ) {
                m->setText( i18n( "%1: Set to Non-Working", date.toString() ) );
            }
        }
    }
    if ( mod ) {
        part()->addCommand( m );
    } else {
        delete m;
    }
    m_currentMenuDateList.clear();
}

void CalendarEditor::slotSetUndefined()
{
    kDebug(planDbg());
    if ( m_currentMenuDateList.isEmpty() || currentCalendar() == 0 ) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify Calendar" ) );
    foreach ( const QDate &date, m_currentMenuDateList ) {
        CalendarDay *day = currentCalendar()->findDay( date );
        if ( day && day->state() != CalendarDay::Undefined ) {
            mod = true;
            m->addCommand( new CalendarRemoveDayCmd( currentCalendar(), day ) );
            if ( m_currentMenuDateList.count() == 1 ) {
                m->setText( i18n( "Set %1 to Undefined", date.toString() ) );
            }
        }
    }
    if ( mod ) {
        part()->addCommand( m );
    } else {
        delete m;
    }
    m_currentMenuDateList.clear();
}


} // namespace KPlato

#include "kptcalendareditor.moc"
