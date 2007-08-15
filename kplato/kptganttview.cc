/* This file is part of the KDE project
  Copyright (C) 2002 - 2007 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation;
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

#include "kptganttview.h"
#include "kptnodeitemmodel.h"
#include "kptappointment.h"
#include "kptpart.h"
#include "kptview.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kpttaskappointmentsview.h"
#include "kptrelation.h"
#include "kptcontext.h"
#include "kptschedule.h"

#include <kdganttproxymodel.h>
#include <kdganttconstraintmodel.h>

#include <kdebug.h>

#include <QHeaderView>
#include <QLayout>
#include <QSplitter>

#include <klocale.h>
#include <kglobal.h>
#include <kprinter.h>
#include <kmessagebox.h>

namespace KPlato
{

MyKDGanttView::MyKDGanttView( Part *part, QWidget *parent )
    : KDGantt::View( parent ),
    m_project( 0 ),
    m_manager( 0 )
{
    kDebug()<<"------------------- create MyKDGanttView -----------------------"<<endl;
    setConstraintModel( new KDGantt::ConstraintModel() );
    KDGantt::ProxyModel *m = static_cast<KDGantt::ProxyModel*>( ganttProxyModel() );
    //m->setColumn( KDGantt::ItemTypeRole, 1 );
    m->setRole( KDGantt::ItemTypeRole, KDGantt::ItemTypeRole );
    m->setRole( KDGantt::StartTimeRole, KDGantt::StartTimeRole );
    m->setRole( KDGantt::EndTimeRole, KDGantt::EndTimeRole );
    m->setColumn( KDGantt::StartTimeRole, 18 );
    m->setColumn( KDGantt::EndTimeRole, 19 );
    m_model = new NodeItemModel( part, this );
    setModel( m_model );
    QTreeView *tv = dynamic_cast<QTreeView*>( leftView() ); //FIXME ?
    if ( tv ) {
        tv->header()->setStretchLastSection( true );
        // Only show name in treeview ;)
        for ( int i = 1; i < m_model->columnCount(); ++i ) {
            tv->hideColumn( i );
        }
    } else kDebug()<<k_funcinfo<<"No treeview !!!"<<endl;
}

void MyKDGanttView::update()
{
    kDebug()<<k_funcinfo<<endl;
    kDebug()<<"POULOU"<<endl;
}

void MyKDGanttView::setProject( Project *project )
{
    clearDependencies();
    if ( m_project ) {
        disconnect( m_project, SIGNAL( relationAdded( Relation* ) ), this, SLOT( addDependency( Relation* ) ) );
        disconnect( m_project, SIGNAL( relationToBeRemoved( Relation* ) ), this, SLOT( removeDependency( Relation* ) ) );
        disconnect( m_project, SIGNAL( projectCalculated( ScheduleManager* ) ), this, SLOT( slotProjectCalculated( ScheduleManager* ) ) );
    }
    m_model->setProject( project );
    m_project = project;
    if ( m_project ) {
        connect( m_project, SIGNAL( relationAdded( Relation* ) ), this, SLOT( addDependency( Relation* ) ) );
        connect( m_project, SIGNAL( relationToBeRemoved( Relation* ) ), this, SLOT( removeDependency( Relation* ) ) );
        connect( m_project, SIGNAL( projectCalculated( ScheduleManager* ) ), this, SLOT( slotProjectCalculated( ScheduleManager* ) ) );
    }
    createDependencies();
}

void MyKDGanttView::slotProjectCalculated( ScheduleManager *sm )
{
    if ( m_manager == sm ) {
        setScheduleManager( sm );
    }
}

void MyKDGanttView::setScheduleManager( ScheduleManager *sm )
{
    //kDebug()<<k_funcinfo<<id<<endl;
    clearDependencies();
    m_model->setManager( sm );
    m_manager = sm;
    createDependencies();
}


void MyKDGanttView::addDependency( Relation *rel )
{
    QModelIndex par = m_model->index( rel->parent() );
    QModelIndex ch = m_model->index( rel->child() );
    qDebug()<<"addDependency() "<<m_model<<par.model();
    if ( par.isValid() && ch.isValid() ) {
        KDGantt::Constraint con( par, ch );
        if ( ! constraintModel()->hasConstraint( con ) ) {
            constraintModel()->addConstraint( con );
        }
    }
}

void MyKDGanttView::removeDependency( Relation *rel )
{
    QModelIndex par = m_model->index( rel->parent() );
    QModelIndex ch = m_model->index( rel->child() );
    qDebug()<<"removeDependency() "<<m_model<<par.model();
    KDGantt::Constraint con( par, ch );
    constraintModel()->removeConstraint( con );
}

void MyKDGanttView::clearDependencies()
{
    constraintModel()->clear();
}

void MyKDGanttView::createDependencies()
{
    clearDependencies();
    if ( m_project == 0 || m_manager == 0 ) {
        return;
    }
    foreach ( Node* n, m_project->allNodes() ) {
        foreach ( Relation *r, n->dependChildNodes() ) {
            addDependency( r );
        }
    }
}

//------------------------------------------

GanttView::GanttView( Part *part, QWidget *parent, bool readWrite )
        : ViewBase( part, parent ),
        m_readWrite( readWrite ),
        m_taskView( 0 ),
        m_project( 0 )
{
    kDebug() <<" ---------------- KPlato: Creating GanttView ----------------";

    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin( 0 );
    m_splitter = new QSplitter( this );
    l->addWidget( m_splitter );
    m_splitter->setOrientation( Qt::Vertical );

    m_gantt = new MyKDGanttView( part, m_splitter );

    m_showExpected = true;
    m_showOptimistic = false;
    m_showPessimistic = false;
    m_showResources = false; // FIXME
    m_showTaskName = false; // FIXME
    m_showTaskLinks = false; // FIXME
    m_showProgress = false; //FIXME
    m_showPositiveFloat = false; //FIXME
    m_showCriticalTasks = false; //FIXME
    m_showCriticalPath = false; //FIXME
    m_showNoInformation = false; //FIXME
    m_showAppointments = false;

    m_taskView = new TaskAppointmentsView( m_splitter );
    m_taskView->hide();

    setReadWriteMode( readWrite );
    //connect( m_gantt->constraintModel(), SIGNAL( constraintAdded( const Constraint& )), this, SLOT( update() ) );
    kDebug() <<m_gantt->constraintModel();
}

void GanttView::setZoom( double )
{
    //kDebug() <<"setting gantt zoom:" << zoom;
    //m_gantt->setZoomFactor(zoom,true); NO!!! setZoomFactor() is something else
    //m_taskView->setZoom( zoom );
}

void GanttView::show()
{
    //m_gantt->show();
}

void GanttView::clear()
{
//    m_gantt->clear();
    m_taskView->clear();
}

void GanttView::setShowTaskLinks( bool on )
{
    m_showTaskLinks = on;
//    m_gantt->setShowTaskLinks( on );
}

void GanttView::setProject( Project *project )
{
    m_gantt->setProject( project );
}

void GanttView::setScheduleManager( ScheduleManager *sm )
{
    //kDebug()<<k_funcinfo<<id<<endl;
    m_gantt->setScheduleManager( sm );
}

void GanttView::draw( Project &project )
{
    setProject( &project );
}

void GanttView::drawChanges( Project &project )
{
    if ( m_project != &project ) {
        setProject( &project );
    }
}

Node *GanttView::currentNode() const
{
//    return getNode( m_currentItem );
    return 0;
}

bool GanttView::loadContext( const KoXmlElement &settings )
{
    kDebug()<<k_funcinfo<<endl;
/*    QDomElement elm = context.firstChildElement( objectName() );
    if ( elm.isNull() ) {
        return false;
    }*/
    
//     Q3ValueList<int> list = m_splitter->sizes();
//     list[ 0 ] = context.ganttviewsize;
//     list[ 1 ] = context.taskviewsize;
//     m_splitter->setSizes( list );

    //TODO this does not work yet!
    //     currentItemChanged(findItem(project.findNode(context.currentNode)));

/*    m_showResources = context.showResources ;
    m_showTaskName = context.showTaskName;*/
    m_showTaskLinks = (bool)settings.attribute( "show-dependencies" , "0" ).toInt();
/*    m_showProgress = context.showProgress;
    m_showPositiveFloat = context.showPositiveFloat;
    m_showCriticalTasks = context.showCriticalTasks;
    m_showCriticalPath = context.showCriticalPath;
    m_showNoInformation = context.showNoInformation;*/
    //TODO this does not work yet!
    //     getContextClosedNodes(context, m_gantt->firstChild());
    //     for (QStringList::ConstIterator it = context.closedNodes.begin(); it != context.closedNodes.end(); ++it) {
    //         KDGanttViewItem *item = findItem(m_project->findNode(*it));
    //         if (item) {
    //             item->setOpen(false);
    //         }
    //     }
    return true;
}

void GanttView::saveContext( QDomElement &settings ) const
{
    kDebug()<<k_funcinfo<<endl;
/*    QDomElement elm = context.firstChildElement( objectName() );
    if ( elm.isNull() ) {
        return;
    }*/
/*    Context::Ganttview &context = c.ganttview;

    //kDebug()<<k_funcinfo;
    context.ganttviewsize = m_splitter->sizes() [ 0 ];
    context.taskviewsize = m_splitter->sizes() [ 1 ];
    //kDebug()<<k_funcinfo<<"sizes="<<sizes()[0]<<","<<sizes()[1];
    if ( currentNode() ) {
        context.currentNode = currentNode() ->id();
    }
    context.showResources = m_showResources;
    context.showTaskName = m_showTaskName;*/
    settings.setAttribute( "show-dependencies", m_showTaskLinks );
/*    context.showProgress = m_showProgress;
    context.showPositiveFloat = m_showPositiveFloat;
    context.showCriticalTasks = m_showCriticalTasks;
    context.showCriticalPath = m_showCriticalPath;
    context.showNoInformation = m_showNoInformation;*/
}

void GanttView::setReadWriteMode( bool on )
{
    m_readWrite = on;
}

void GanttView::update()
{
    kDebug()<<k_funcinfo;
    kDebug()<<"POULOU";
}

}  //KPlato namespace

#include "kptganttview.moc"
