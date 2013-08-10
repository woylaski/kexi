/* This file is part of the KDE project
  Copyright (C) 2007 - 2009, 2012 Dag Andersen <danders@get2net.dk>

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

#include "kptnodeitemmodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptduration.h"
#include "kptproject.h"
#include "kptnode.h"
#include "kpttaskcompletedelegate.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include "KoStore.h"
#include <KoIcon.h>

#include <QAbstractItemModel>
#include <QMimeData>
#include <QModelIndex>
#include <QWidget>
#include <QPair>
#include <QByteArray>

#include <kaction.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <kaccelgen.h>
#include <kactioncollection.h>
#include <krichtextwidget.h>
#include <kmimetype.h>

#include <kdganttglobal.h>
#include <math.h>


namespace KPlato
{


//--------------------------------------
NodeModel::NodeModel()
    : QObject(),
    m_project( 0 ),
    m_manager( 0 ),
    m_now( QDate::currentDate() ),
    m_prec( 1 )
{
}

const QMetaEnum NodeModel::columnMap() const
{
    return metaObject()->enumerator( metaObject()->indexOfEnumerator("Properties") );
}

void NodeModel::setProject( Project *project )
{
    kDebug(planDbg())<<m_project<<"->"<<project;
    m_project = project;
}

void NodeModel::setManager( ScheduleManager *sm )
{
    kDebug(planDbg())<<m_manager<<"->"<<sm;
    m_manager = sm;
}

QVariant NodeModel::name( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return node->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::DecorationRole:
            if ( node->isBaselined() ) {
                return koIcon("view-time-schedule-baselined");
            }
            break;
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task:
                    return static_cast<const Task*>( node )->completion().isFinished() ? m_project->config().taskFinishedColor() : m_project->config().taskNormalColor();
                case Node::Type_Milestone:
                    return static_cast<const Task*>( node )->completion().isFinished() ? m_project->config().milestoneFinishedColor() : m_project->config().milestoneNormalColor();
                case Node::Type_Summarytask:
                    return m_project->config().summaryTaskLevelColor( node->level() );
                default:
                    break;
            }
            break;
        }
    }
    return QVariant();
}

QVariant NodeModel::leader( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return node->leader();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::allocation( const Node *node, int role ) const
{
    if ( node->type() == Node::Type_Task ) {
        switch ( role ) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return node->requests().requestNameList().join( "," );
            case Qt::ToolTipRole: {
                QMap<QString, QStringList> lst;
                foreach ( ResourceRequest *rr, node->requests().resourceRequests( false ) ) {
                    QStringList sl;
                    foreach( Resource *r, rr->requiredResources() ) {
                        sl << r->name();
                    }
                    lst.insert( rr->resource()->name(), sl );
                }
                if ( lst.isEmpty() ) {
                    return i18nc( "@info:tooltip", "No resources has been allocated" );
                }
                QStringList sl;
                for ( QMap<QString, QStringList>::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it ) {
                    if ( it.value().isEmpty() ) {
                        sl << it.key();
                    } else {
                        sl << i18nc( "@info:tooltip 1=resource name, 2=list of requiered resources", "%1 (%2)", it.key(), it.value().join(", ") );
                    }
                }
                if ( sl.count() == 1 ) {
                    return i18nc( "@info:tooltip 1=resource name", "Allocated resource:<nl/>%1", sl.first() );
                }
                return i18nc( "@info:tooltip 1=list of resources", "Allocated resources:<nl/>%1", sl.join( "<nl/>" ) );
            }
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::description( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            KRichTextWidget w( node->description(), 0 );
            w.switchToPlainText();
            QString s = w.textOrHtml();
            int i = s.indexOf( '\n' );
            s = s.left( i );
            if ( i > 0 ) {
                s += "...";
            }
            return s;
        }
        case Qt::ToolTipRole: {
            KRichTextWidget w( node->description(), 0 );
            w.switchToPlainText();
            if ( w.textOrHtml().isEmpty() ) {
                return QVariant();
            }
            return node->description();
        }
        case Qt::EditRole:
            return node->description();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::type( const Node *node, int role ) const
{
    //kDebug(planDbg())<<node->name()<<", "<<role;
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return node->typeToString( true );
        case Qt::EditRole:
            return node->type();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignLeft|Qt::AlignVCenter);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::constraint( const Node *node, int role ) const
{
    if ( node->type() == Node::Type_Project ) {
        switch ( role ) {
            case Qt::DisplayRole:
                return i18n( "Target times" );
            case Qt::ToolTipRole:
                return i18nc( "@info:tooltip", "Earliest start and latest finish" );
            case Role::EnumList:
            case Qt::EditRole:
            case Role::EnumListValue:
                return QVariant();
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    } else if ( node->type() != Node::Type_Summarytask ) {
        switch ( role ) {
            case Qt::DisplayRole:
            case Qt::ToolTipRole:
                return node->constraintToString( true );
            case Role::EnumList:
                return Node::constraintList( true );
            case Qt::EditRole:
                return node->constraint();
            case Role::EnumListValue:
                return (int)node->constraint();
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::constraintStartTime( const Node *node, int role ) const
{
    if ( node->type() == Node::Type_Project ) {
        switch ( role ) {
            case Qt::DisplayRole: {
                return KGlobal::locale()->formatDateTime( node->constraintStartTime() );
            }
            case Qt::ToolTipRole: {
                return KGlobal::locale()->formatDateTime( node->constraintStartTime(), KLocale::LongDate, KLocale::TimeZone );
            }
            case Qt::EditRole:
                return node->constraintStartTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
        return QVariant();
    } else if ( node->type() != Node::Type_Summarytask ) {
        switch ( role ) {
            case Qt::DisplayRole: {
                QString s = KGlobal::locale()->formatDateTime( node->constraintStartTime() );
                switch ( node->constraint() ) {
                    case Node::StartNotEarlier:
                    case Node::MustStartOn:
                    case Node::FixedInterval:
                        return s;
                    default:
                        break;
                }
                return QString( "(%1)" ).arg( s );
        }
            case Qt::ToolTipRole: {
                int c = node->constraint();
                if ( c == Node::MustStartOn || c == Node::StartNotEarlier || c == Node::FixedInterval  ) {
                    return KGlobal::locale()->formatDateTime( node->constraintStartTime(), KLocale::LongDate, KLocale::TimeZone );
                }
                break;
            }
            case Qt::EditRole:
                return node->constraintStartTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::constraintEndTime( const Node *node, int role ) const
{
    if ( node->type() == Node::Type_Project ) {
        switch ( role ) {
            case Qt::DisplayRole: {
                return KGlobal::locale()->formatDateTime( node->constraintEndTime() );
            }
            case Qt::ToolTipRole: {
                return KGlobal::locale()->formatDateTime( node->constraintEndTime(), KLocale::LongDate, KLocale::TimeZone  );
            }
            case Qt::EditRole:
                return node->constraintEndTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
        return QVariant();
    } else if ( node->type() != Node::Type_Summarytask ) {
        switch ( role ) {
            case Qt::DisplayRole: {
                QString s = KGlobal::locale()->formatDateTime( node->constraintEndTime() );
                switch ( node->constraint() ) {
                    case Node::FinishNotLater:
                    case Node::MustFinishOn:
                    case Node::FixedInterval:
                        return s;
                    default:
                        break;
                }
                return QString( "(%1)" ).arg( s );
            }
            case Qt::ToolTipRole: {
                int c = node->constraint();
                if ( c == Node::FinishNotLater || c == Node::MustFinishOn || c == Node::FixedInterval ) {
                    return KGlobal::locale()->formatDateTime( node->constraintEndTime(), KLocale::LongDate, KLocale::TimeZone  );
                }
                break;
            }
            case Qt::EditRole:
                return node->constraintEndTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::estimateType( const Node *node, int role ) const
{
    if ( node->estimate() == 0 ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->typeToString( true );
            }
            return QString();
        case Role::EnumList:
            return Estimate::typeToStringList( true );
        case Qt::EditRole:
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->typeToString();
            }
            return QString();
        case Role::EnumListValue:
            return (int)node->estimate()->type();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::estimateCalendar( const Node *node, int role ) const
{
    if ( node->estimate() == 0 ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task ) {
                if ( node->estimate()->calendar() ) {
                    return node->estimate()->calendar()->name();
                }
                return i18n( "None" );
            }
            return QString();
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                if ( node->estimate()->type() == Estimate::Type_Effort ) {
                    return i18nc( "@info:tooltip", "Not applicable, estimate type is Effort" );
                }
                if ( node->estimate()->calendar() ) {
                    return node->estimate()->calendar()->name();
                }
                return QVariant();
            }
            return QString();
        case Role::EnumList:
        {
            QStringList lst; lst << i18n( "None" );
            const Node *n = const_cast<Node*>( node )->projectNode();
            if ( n ) {
                lst += static_cast<const Project*>( n )->calendarNames();
            }
            return lst;
        }
        case Qt::EditRole:
            if ( node->type() == Node::Type_Task ) {
                if ( node->estimate()->calendar() == 0 ) {
                    return i18n( "None" );
                }
                return node->estimate()->calendar()->name();
            }
            return QString();
        case Role::EnumListValue:
        {
            if ( node->estimate()->calendar() == 0 ) {
                return 0;
            }
            QStringList lst;
            const Node *n = const_cast<Node*>( node )->projectNode();
            if ( n ) {
                lst = static_cast<const Project*>( n )->calendarNames();
            }
            return lst.indexOf( node->estimate()->calendar()->name() ) + 1;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::estimate( const Node *node, int role ) const
{
    if ( node->estimate() == 0 ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone ) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = KGlobal::locale()->formatNumber( node->estimate()->expectedEstimate(), m_prec ) +  Duration::unitToString( unit, true );
                if ( node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration ) {
                    s = '(' + s + ')';
                }
                return s;
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = KGlobal::locale()->formatNumber( node->estimate()->expectedEstimate(), m_prec ) +  Duration::unitToString( unit, true );
                Estimate::Type t = node->estimate()->type();
                if ( node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration ) {
                    s = i18nc( "@info:tooltip", "Not applicable, constraint is Fixed Interval" );
                } else if ( t == Estimate::Type_Effort ) {
                    s = i18nc( "@info:tooltip", "Estimated effort: %1", s );
                } else {
                    s = i18nc( "@info:tooltip", "Estimated duration: %1", s );
                }
                return s;
            }
            break;
        case Qt::EditRole:
            return node->estimate()->expectedEstimate();
        case Role::DurationUnit:
            return static_cast<int>( node->estimate()->unit() );
        case Role::Minimum:
            return m_project->config().minimumDurationUnit();
        case Role::Maximum:
            return m_project->config().maximumDurationUnit();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticRatio( const Node *node, int role ) const
{
    if ( node->estimate() == 0 || node->type() == Node::Type_Summarytask || node->type() == Node::Type_Milestone ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task && node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration ) {
                QString s = QString::number( node->estimate()->optimisticRatio() );
                s = '(' + s + ')';
                return s;
            }
            if ( node->estimate() ) {
                return node->estimate()->optimisticRatio();
            }
            break;
        case Qt::EditRole:
            if ( node->estimate() ) {
                return node->estimate()->optimisticRatio();
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = KGlobal::locale()->formatNumber( node->estimate()->optimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true );
                Estimate::Type t = node->estimate()->type();
                if ( node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration ) {
                    s = i18nc( "@info:tooltip", "Not applicable, constraint is Fixed Interval" );
                } else if ( t == Estimate::Type_Effort ) {
                    s = i18nc( "@info:tooltip", "Optimistic effort: %1", s );
                } else {
                    s = i18nc( "@info:tooltip", "Optimistic duration: %1", s );
                }
                return s;
            }
            break;
        case Role::Minimum:
            return -99;
        case Role::Maximum:
            return 0;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticRatio( const Node *node, int role ) const
{
    if ( node->estimate() == 0 || node->type() == Node::Type_Summarytask || node->type() == Node::Type_Milestone ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task && node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration ) {
                QString s = QString::number( node->estimate()->pessimisticRatio() );
                s = '(' + s + ')';
                return s;
            }
            if ( node->estimate() ) {
                return node->estimate()->pessimisticRatio();
            }
            break;
        case Qt::EditRole:
            if ( node->estimate() ) {
                return node->estimate()->pessimisticRatio();
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = KGlobal::locale()->formatNumber( node->estimate()->pessimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true );
                Estimate::Type t = node->estimate()->type();
                if ( node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration ) {
                    s = i18nc( "@info:tooltip", "Not applicable, constraint is Fixed Interval" );
                } else if ( t == Estimate::Type_Effort ) {
                    s = i18nc( "@info:tooltip", "Pessimistic effort: %1", s );
                } else {
                    s = i18nc( "@info:tooltip", "Pessimistic duration: %1", s );
                }
                return s;
            }
            break;
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return INT_MAX;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::riskType( const Node *node, int role ) const
{
    if ( node->estimate() == 0 ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->risktypeToString( true );
            }
            return QString();
        case Role::EnumList:
            return Estimate::risktypeToStringList( true );
        case Qt::EditRole:
            if ( node->type() == Node::Type_Task ) {
                return node->estimate()->risktypeToString();
            }
            return QString();
        case Role::EnumListValue:
            return (int)node->estimate()->risktype();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::runningAccount( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task ) {
                Account *a = node->runningAccount();
                return a == 0 ? i18n( "None" ) : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Account *a = node->runningAccount();
                return a ? i18nc( "@info:tooltip", "Account for resource cost: %1", a->name() )
                         : i18nc( "@info:tooltip", "Account for resource cost" );
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->runningAccount();
            return a == 0 ? 0 : ( m_project->accounts().costElements().indexOf( a->name() ) + 1 );
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startupAccount( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone ) {
                Account *a = node->startupAccount();
                //kDebug(planDbg())<<node->name()<<": "<<a;
                return a == 0 ? i18n( "None" ) : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone ) {
                Account *a = node->startupAccount();
                //kDebug(planDbg())<<node->name()<<": "<<a;
                return a ? i18nc( "@info:tooltip", "Account for task startup cost: %1", a->name() )
                         : i18nc( "@info:tooltip", "Account for task startup cost" );
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->startupAccount();
            return a == 0 ? 0 : ( m_project->accounts().costElements().indexOf( a->name() ) + 1 );
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startupCost( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) {
                return m_project->locale()->formatMoney( node->startupCost() );
            }
            break;
        case Qt::EditRole:
            return node->startupCost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::shutdownAccount( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) {
                Account *a = node->shutdownAccount();
                return a == 0 ? i18n( "None" ) : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) {
                Account *a = node->shutdownAccount();
                return a ? i18nc( "@info:tooltip", "Account for task shutdown cost: %1", a->name() )
                         : i18nc( "@info:tooltip", "Account for task shutdown cost" );
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->shutdownAccount();
            return a == 0 ? 0 : ( m_project->accounts().costElements().indexOf( a->name() ) + 1 );
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::shutdownCost( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) {
                return m_project->locale()->formatMoney( node->shutdownCost() );
            }
            break;
        case Qt::EditRole:
            return node->shutdownCost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startTime( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( node->startTime( id() ) );
        case Qt::ToolTipRole:
            //kDebug(planDbg())<<node->name()<<", "<<role;
            return i18nc( "@info:tooltip", "Scheduled start: %1", KGlobal::locale()->formatDateTime( node->startTime( id() ), KLocale::LongDate, KLocale::TimeZone ) );
        case Qt::EditRole:
            return node->startTime( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::endTime( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( node->endTime( id() ) );
        case Qt::ToolTipRole:
            //kDebug(planDbg())<<node->name()<<", "<<role;
            return i18nc( "@info:tooltip", "Scheduled finish: %1", KGlobal::locale()->formatDateTime( node->endTime( id() ), KLocale::LongDate, KLocale::TimeZone ) );
        case Qt::EditRole:
            return node->endTime( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::duration( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->duration( id() ).toDouble( unit );
                return QVariant(KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ));
            } else if ( node->type() == Node::Type_Project ) {
                Duration::Unit unit = Duration::Unit_d;
                double v = node->duration( id() ).toDouble( unit );
                return QVariant(KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ));
            }
            break;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->duration( id() ).toDouble( unit );
                return i18nc( "@info:tooltip", "Scheduled duration: %1", KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ) );
            } else if ( node->type() == Node::Type_Project ) {
                Duration::Unit unit = Duration::Unit_d;
                double v = node->duration( id() ).toDouble( unit );
                return i18nc( "@info:tooltip", "Scheduled duration: %1", KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ) );
            }
            break;
        case Qt::EditRole: {
            return node->duration( id() ).toDouble( Duration::Unit_h );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::varianceDuration( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->variance( id(), unit );
                return KGlobal::locale()->formatNumber( v );
            }
            break;
        case Qt::EditRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                return node->variance( id(), unit );
            }
            return 0.0;
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->variance( id(), unit );
                return i18nc( "@info:tooltip", "PERT duration variance: %1", KGlobal::locale()->formatNumber( v ) );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::varianceEstimate( const Estimate *est, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = est->variance( unit );
            //kDebug(planDbg())<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return KGlobal::locale()->formatNumber( v );
        }
        case Qt::EditRole: {
            if ( est == 0 ) {
                return 0.0;
            }
            return est->variance( est->unit() );
        }
        case Qt::ToolTipRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = est->variance( unit );
            return i18nc( "@info:tooltip", "PERT estimate variance: %1", KGlobal::locale()->formatNumber( v ) + Duration::unitToString( unit, true ) );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticDuration( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( node->type() != Node::Type_Task ) {
                return QVariant();
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->optimisticRatio() ) ) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble( unit );
                //kDebug(planDbg())<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return QVariant(KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ));
            break;
        }
        case Qt::EditRole: {
            if ( node->type() != Node::Type_Task ) {
                return 0.0;
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->optimisticRatio() ) ) / 100;
            Duration::Unit unit = node->estimate()->unit();
            return d.toDouble( unit );
        }
        case Qt::ToolTipRole: {
            if ( node->type() != Node::Type_Task ) {
                return QVariant();
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->optimisticRatio() ) ) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble( unit );
            //kDebug(planDbg())<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return i18nc( "@info:tooltip", "PERT optimistic duration: %1", KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ) );
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticEstimate( const Estimate *est, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return QVariant(KGlobal::locale()->formatNumber( est->optimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true ));
            break;
        }
        case Qt::EditRole: {
            if ( est == 0 ) {
                return 0.0;
            }
            return est->optimisticEstimate();
        }
        case Qt::ToolTipRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return i18nc( "@info:tooltip", "Optimistic estimate: %1", KGlobal::locale()->formatNumber( est->optimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true ) );
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pertExpected( const Estimate *est, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = Estimate::scale( est->pertExpected(), unit, est->scales() );
            return QVariant(KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ));
        }
        case Qt::EditRole: {
            if ( est == 0 ) {
                return 0.0;
            }
            return Estimate::scale( est->pertExpected(), est->unit(), est->scales() );
        }
        case Qt::ToolTipRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = Estimate::scale( est->pertExpected(), unit, est->scales() );
            return i18nc( "@info:tooltip", "PERT expected estimate: %1", KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ) );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticDuration( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( node->type() != Node::Type_Task ) {
                return QVariant();
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->pessimisticRatio() ) ) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble( unit );
            //kDebug(planDbg())<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return QVariant(KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ));
            break;
        }
        case Qt::EditRole: {
            if ( node->type() != Node::Type_Task ) {
                return 0.0;
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->pessimisticRatio() ) ) / 100;
            return d.toDouble( node->estimate()->unit() );
        }
        case Qt::ToolTipRole: {
            if ( node->type() != Node::Type_Task ) {
                return QVariant();
            }
            Duration d = node->duration( id() );
            d = ( d * ( 100 + node->estimate()->pessimisticRatio() ) ) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble( unit );
            //kDebug(planDbg())<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return i18nc( "@info:tooltip", "PERT pessimistic duration: %1", KGlobal::locale()->formatNumber( v, m_prec ) +  Duration::unitToString( unit, true ) );
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticEstimate( const Estimate *est, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return QVariant(KGlobal::locale()->formatNumber( est->pessimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true ));
            break;
        }
        case Qt::EditRole: {
            if ( est == 0 ) {
                return 0.0;
            }
            return est->pessimisticEstimate();
        }
        case Qt::ToolTipRole: {
            if ( est == 0 ) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return i18nc( "@info:tooltip", "Pessimistic estimate: %1", KGlobal::locale()->formatNumber( est->pessimisticEstimate(), m_prec ) +  Duration::unitToString( unit, true ) );
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::earlyStart( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( t->earlyStart( id() ) );
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDate( t->earlyStart( id() ).date() );
        case Qt::EditRole:
            return t->earlyStart( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::earlyFinish( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( t->earlyFinish( id() ) );
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDate( t->earlyFinish( id() ).date() );
        case Qt::EditRole:
            return t->earlyFinish( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::lateStart( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( t->lateStart( id() ) );
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDate( t->lateStart( id() ).date() );
        case Qt::EditRole:
            return t->lateStart( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::lateFinish( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatDateTime( t->lateFinish( id() ) );
        case Qt::ToolTipRole:
            return KGlobal::locale()->formatDate( t->lateFinish( id() ).date() );
        case Qt::EditRole:
            return t->lateFinish( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::positiveFloat( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->positiveFloat( id() ).toString( Duration::Format_i18nHourFraction );
        case Qt::ToolTipRole:
            return t->positiveFloat( id() ).toString( Duration::Format_i18nDayTime );
        case Qt::EditRole:
            return t->positiveFloat( id() ).toDouble( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::freeFloat( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->freeFloat( id() ).toString( Duration::Format_i18nHourFraction );
        case Qt::ToolTipRole:
            return t->freeFloat( id() ).toString( Duration::Format_i18nDayTime );
        case Qt::EditRole:
            return t->freeFloat( id() ).toDouble( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::negativeFloat( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->negativeFloat( id() ).toString( Duration::Format_i18nHourFraction );
        case Qt::ToolTipRole:
            return t->negativeFloat( id() ).toString( Duration::Format_i18nDayTime );
        case Qt::EditRole:
            return t->negativeFloat( id() ).toDouble( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startFloat( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->startFloat( id() ).toString( Duration::Format_i18nHourFraction );
        case Qt::ToolTipRole:
            return t->startFloat( id() ).toString( Duration::Format_i18nDayTime );
        case Qt::EditRole:
            return t->startFloat( id() ).toDouble( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::finishFloat( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->finishFloat( id() ).toString( Duration::Format_i18nHourFraction );
        case Qt::ToolTipRole:
            return t->finishFloat( id() ).toString( Duration::Format_i18nDayTime );
        case Qt::EditRole:
            t->finishFloat( id() ).toDouble( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::assignedResources( const Node *node, int role ) const
{
    if ( node->type() != Node::Type_Task ) {
        return QVariant();
    }
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->assignedNameList( id() ).join(",");
        case Qt::ToolTipRole: {
            QStringList lst = node->assignedNameList( id() );
            if ( ! lst.isEmpty() ) {
                return i18nc( "@info:tooltip 1=list of resources", "Assigned resources:<nl/>%1", node->assignedNameList( id() ).join("<nl/>") );
            }
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}


QVariant NodeModel::completed( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            return t->completion().percentFinished();
        case Qt::EditRole:
            return t->completion().percentFinished();
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Task is %1% completed", t->completion().percentFinished() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::status( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole: {
            int st = t->state( id() );
            if ( st & Node::State_NotScheduled ) {
                return SchedulingState::notScheduled();
            }
            if ( st & Node::State_Finished ) {
                if ( st & Node::State_FinishedLate ) {
                    return i18n( "Finished late" );
                }
                if ( st & Node::State_FinishedEarly ) {
                    return i18n( "Finished early" );
                }
                return i18n( "Finished" );
            }
            if ( st & Node::State_Running ) {
                if ( st & Node::State_Late ) {
                    return i18n( "Running late" );
                }
                return i18n( "Running" );
            }
            if ( st & Node::State_Started ) {
                if ( st & Node::State_StartedLate ) {
                    return i18n( "Started late" );
                }
                if ( st & Node::State_StartedEarly ) {
                    return i18n( "Started early" );
                }
                if ( st & Node::State_Late ) {
                    return i18n( "Running late" );
                }
                return i18n( "Started" );
            }
            if ( st & Node::State_ReadyToStart ) {
                if ( st & Node::State_Late ) {
                    return i18n( "Not started" );
                }
                return i18n( "Can start" );
            }
            if ( st & Node::State_NotReadyToStart ) {
                if ( st & Node::State_Late ) {
                    return i18n( "Delayed" );
                }
                return i18n( "Cannot start" );
            }
            return i18n( "Not started" );
            break;
        }
        case Qt::ToolTipRole: {
            int st = t->state( id() );
            if ( st & Node::State_NotScheduled ) {
                return SchedulingState::notScheduled();
            }
            if ( st & Node::State_Finished ) {
                if ( st & Node::State_FinishedLate ) {
                    Duration d = t->completion().finishTime() - t->endTime( id() );
                    return i18nc( "@info:tooltip", "Finished %1 late", d.toString( Duration::Format_i18nDay ) );
                }
                if ( st & Node::State_FinishedEarly ) {
                    Duration d = t->endTime( id() ) - t->completion().finishTime();
                    return i18nc( "@info:tooltip", "Finished %1 early", d.toString( Duration::Format_i18nDay ) );
                }
                return i18nc( "@info:tooltip", "Finished" );
            }
            if ( st & Node::State_Started ) {
                if ( st & Node::State_StartedLate ) {
                    Duration d = t->completion().startTime() - t->startTime( id() );
                    return i18nc( "@info:tooltip", "Started %1 late", d.toString( Duration::Format_i18nDay ) );
                }
                if ( st & Node::State_StartedEarly ) {
                    Duration d = t->startTime( id() ) - t->completion().startTime();
                    return i18nc( "@info:tooltip", "Started %1 early", d.toString( Duration::Format_i18nDay ) );
                }
                return i18nc( "@info:tooltip", "Started" );
            }
            if ( st & Node::State_Running ) {
                return i18nc( "@info:tooltip", "Running" );
            }
            if ( st & Node::State_ReadyToStart ) {
                return i18nc( "@info:tooltip", "Can start" );
            }
            if ( st & Node::State_NotReadyToStart ) {
                QStringList names;
                // TODO: proxy relations
                foreach ( Relation *r, node->dependParentNodes() ) {
                    switch ( r->type() ) {
                        case Relation::FinishFinish:
                        case Relation::FinishStart:
                            if ( ! static_cast<Task*>( r->parent() )->completion().isFinished() ) {
                                if ( ! names.contains( r->parent()->name() ) ) {
                                    names << r->parent()->name();
                                }
                            }
                            break;
                        case Relation::StartStart:
                            if ( ! static_cast<Task*>( r->parent() )->completion().isStarted() ) {
                                if ( ! names.contains( r->parent()->name() ) ) {
                                    names << r->parent()->name();
                                }
                            }
                            break;
                    }
                }
                return names.isEmpty()
                    ? i18nc( "@info:tooltip", "Cannot start" )
                    : i18nc( "@info:tooltip 1=list of task names", "Cannot start, waiting for:<nl/>%1", names.join( "<nl/>" ) );
            }
            return i18nc( "@info:tooltip", "Not started" );
            break;
        }
        case Qt::EditRole:
            return t->state( id() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startedTime( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            if ( t->completion().isStarted() ) {
                return KGlobal::locale()->formatDateTime( t->completion().startTime() );
            }
            break;
        case Qt::ToolTipRole:
            if ( t->completion().isStarted() ) {
                return i18nc( "@info:tooltip", "Actual start: %1", KGlobal::locale()->formatDate( t->completion().startTime().date(), KLocale::LongDate ) );
            }
            break;
        case Qt::EditRole:
            if ( t->completion().isStarted() ) {
                return t->completion().startTime();
            }
            return QDateTime::currentDateTime();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::isStarted( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return t->completion().isStarted();
        case Qt::ToolTipRole:
            if ( t->completion().isStarted() ) {
                return i18nc( "@info:tooltip", "The task started at: %1", KGlobal::locale()->formatDate( t->completion().startTime().date(), KLocale::LongDate ) );
            }
            return i18nc( "@info:tooltip", "The task is not started" );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::finishedTime( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
            if ( t->completion().isFinished() ) {
                return KGlobal::locale()->formatDateTime( t->completion().finishTime() );
            }
            break;
        case Qt::ToolTipRole:
            if ( t->completion().isFinished() ) {
                return i18nc( "@info:tooltip", "Actual finish: %1", KGlobal::locale()->formatDateTime( t->completion().finishTime(), KLocale::LongDate, KLocale::TimeZone ) );
            }
            break;
        case Qt::EditRole:
            if ( t->completion().isFinished() ) {
                return t->completion().finishTime();
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::isFinished( const Node *node, int role ) const
{
    if ( ! ( node->type() == Node::Type_Task || node->type() == Node::Type_Milestone ) ) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>( node );
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return t->completion().isFinished();
        case Qt::ToolTipRole:
            if ( t->completion().isFinished() ) {
                return i18nc( "@info:tooltip", "The task finished at: %1", KGlobal::locale()->formatDate( t->completion().finishTime().date(), KLocale::LongDate ) );
            }
            return i18nc( "@info:tooltip", "The task is not finished" );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::plannedEffortTo( const Node *node, int role ) const
{
    KLocale *l = KGlobal::locale();
    switch ( role ) {
        case Qt::DisplayRole:
            return node->plannedEffortTo( m_now, id() ).format();
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Planned effort until %1: %2", l->formatDate( m_now ), node->plannedEffortTo( m_now, id() ).toString( Duration::Format_i18nHour ) );
        case Qt::EditRole:
            return node->plannedEffortTo( m_now, id() ).toDouble( Duration::Unit_h );
        case Role::DurationUnit:
            return static_cast<int>( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::actualEffortTo( const Node *node, int role ) const
{
    KLocale *l = KGlobal::locale();
    switch ( role ) {
        case Qt::DisplayRole:
            return node->actualEffortTo( m_now ).format();
        case Qt::ToolTipRole:
            //kDebug(planDbg())<<m_now<<node;
            return i18nc( "@info:tooltip", "Actual effort used up to %1: %2", l->formatDate( m_now ), node->actualEffortTo( m_now ).toString( Duration::Format_i18nHour ) );
        case Qt::EditRole:
            return node->actualEffortTo( m_now ).toDouble( Duration::Unit_h );
        case Role::DurationUnit:
            return static_cast<int>( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::remainingEffort( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t ) {
                return t->completion().remainingEffort().format();
            }
            break;
        }
        case Qt::ToolTipRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t ) {
                return i18nc( "@info:tooltip", "Remaining effort: %1", t->completion().remainingEffort().toString( Duration::Format_i18nHour ) );
            }
            break;
        }
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            return t->completion().remainingEffort().toDouble( Duration::Unit_h );
        }
        case Role::DurationUnit:
            return static_cast<int>( Duration::Unit_h );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::plannedCostTo( const Node *node, int role ) const
{
    KLocale *l = m_project->locale();
    switch ( role ) {
        case Qt::DisplayRole:
            return l->formatMoney( node->plannedCostTo( m_now, id() ) );
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Planned cost until %1: %2", l->formatDate( m_now ), l->formatMoney( node->plannedCostTo( m_now, id() ) ) );
        case Qt::EditRole:
            return node->plannedCostTo( m_now );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::actualCostTo( const Node *node, int role ) const
{
    KLocale *l = m_project->locale();
    switch ( role ) {
        case Qt::DisplayRole:
            return l->formatMoney( node->actualCostTo( id(), m_now ).cost() );
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Actual cost until %1: %2", l->formatDate( m_now ), l->formatMoney( node->actualCostTo( id(), m_now ).cost() ) );
        case Qt::EditRole:
            return node->actualCostTo( id(), m_now ).cost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::note( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            if ( node->type() == Node::Type_Task ) {
                Node *n = const_cast<Node*>( node );
                return static_cast<Task*>( n )->completion().note();
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeSchedulingStatus( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return node->schedulingStatus( id(), true ).value( 0 );
        case Qt::EditRole:
            return node->schedulingStatus( id(), false ).value( 0 );
        case Qt::ToolTipRole:
            return node->schedulingStatus( id(), true ).join( "\n" );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::resourceIsMissing( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->resourceError( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->resourceError( id() );
        case Qt::ToolTipRole:
            if ( node->resourceError( id() ) ) {
                return i18nc( "@info:tooltip", "Resource allocation is expected when the task estimate type is  <emphasis>Effort</emphasis>" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::resourceIsOverbooked( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->resourceOverbooked( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->resourceOverbooked( id() );
        case Qt::ToolTipRole:
            if ( node->resourceOverbooked( id() ) ) {
                return i18nc( "@info:tooltip", "A resource has been overbooked" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::resourceIsNotAvailable( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->resourceNotAvailable( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->resourceNotAvailable( id() );
        case Qt::ToolTipRole:
            if ( node->resourceNotAvailable( id() ) ) {
                return i18nc( "@info:tooltip", "No resource is available for this task" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::schedulingConstraintsError( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->constraintError( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->constraintError( id() );
        case Qt::ToolTipRole:
            if ( node->constraintError( id() ) ) {
                return i18nc( "@info:tooltip", "Failed to comply with a timing constraint" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::nodeIsNotScheduled( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->notScheduled( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->notScheduled( id() );
        case Qt::ToolTipRole:
            if ( node->notScheduled( id() ) ) {
                return i18nc( "@info:tooltip", "This task has not been scheduled" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::effortNotMet( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->effortMetError( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->effortMetError( id() );
        case Qt::ToolTipRole:
            if ( node->effortMetError( id() ) ) {
                return i18nc( "@info:tooltip", "The assigned resources cannot deliver the required estimated effort" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::schedulingError( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            if ( node->schedulingError( id() ) ) {
                return i18n( "Error" );
            }
            break;
        case Qt::EditRole:
            return node->schedulingError( id() );
        case Qt::ToolTipRole:
            if ( node->schedulingError( id() ) ) {
                return i18nc( "@info:tooltip", "Scheduling error" );
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::wbsCode( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->wbsCode();
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Work breakdown structure code: %1", node->wbsCode() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeLevel( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->level();
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Task level: %1", node->level() );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeBCWS( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney( node->bcws( m_now, id() ), QString(), 0 );
        case Qt::EditRole:
            return node->bcws( m_now, id() );
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Budgeted Cost of Work Scheduled at %1: %2", m_now.toString(), m_project->locale()->formatMoney( node->bcws( m_now, id() ), QString(), 0 ) );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeBCWP( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney( node->bcwp( id() ), QString(), 0 );
        case Qt::EditRole:
            return node->bcwp( id() );
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Budgeted Cost of Work Performed at %1: %2", m_now.toString(), m_project->locale()->formatMoney( node->bcwp( id() ), QString(), 0 ) );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeACWP( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney( node->acwp( m_now, id() ).cost(), QString(), 0 );
        case Qt::EditRole:
            return node->acwp( m_now, id() ).cost();
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Actual Cost of Work Performed at %1: %2", m_now.toString(), m_project->locale()->formatMoney( node->acwp( m_now, id() ).cost() ) );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodePerformanceIndex( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
            return KGlobal::locale()->formatNumber( node->schedulePerformanceIndex( m_now, id() ), 2 );
        case Qt::EditRole:
            return node->schedulePerformanceIndex( m_now, id() );
        case Qt::ToolTipRole:
            return i18nc( "@info:tooltip", "Schedule Performance Index at %1: %2", m_now.toString(), KGlobal::locale()->formatNumber( node->schedulePerformanceIndex( m_now, id() ), 2 ) );
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::ForegroundRole:
            return node->schedulePerformanceIndex( m_now, id() ) < 1.0 ? Qt::red : Qt::black;
    }
    return QVariant();
}

QVariant NodeModel::nodeIsCritical( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->isCritical( id() );
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskNormalColor();
                case Node::Type_Milestone: return m_project->config().milestoneNormalColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::nodeInCriticalPath( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->inCriticalPath( id() );
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if ( ! m_project ) {
                break;
            }
            switch ( node->type() ) {
                case Node::Type_Task: return m_project->config().taskNormalColor();
                case Node::Type_Milestone: return m_project->config().milestoneNormalColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::wpOwnerName( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t == 0 ) {
                return QVariant();
            }
            if ( t->wpTransmitionStatus() == WorkPackage::TS_None ) {
                return i18nc( "Not available", "NA" );
            }
            return t->wpOwnerName();
        }
        case Qt::ToolTipRole: {
            const Task *task = dynamic_cast<const Task*>( node );
            if ( task == 0 ) {
                return QVariant();
            }
            int sts = task->wpTransmitionStatus();
            QString t = wpTransmitionTime( node, Qt::DisplayRole ).toString();
            if ( sts == WorkPackage::TS_Send ) {
                return i18nc( "@info:tooltip", "Latest work package sent to %1 at %2", static_cast<const Task*>( node )->wpOwnerName(), t );
            }
            if ( sts == WorkPackage::TS_Receive ) {
                return i18nc( "@info:tooltip", "Latest work package received from %1 at %2", static_cast<const Task*>( node )->wpOwnerName(), t );
            }
            return i18nc( "@info:tooltip", "Not available" );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::wpTransmitionStatus( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t == 0 ) {
                return QVariant();
            }
            if ( t->wpTransmitionStatus() == WorkPackage::TS_None ) {
                return i18nc( "Not available", "NA" );
            }
            return WorkPackage::transmitionStatusToString( t->wpTransmitionStatus(), true );
        }
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t == 0 ) {
                return QVariant();
            }
            return WorkPackage::transmitionStatusToString( t->wpTransmitionStatus(), false );
        }
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::wpTransmitionTime( const Node *node, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>( node );
            if ( t == 0 ) {
                return QVariant();
            }
            if ( t->wpTransmitionStatus() == WorkPackage::TS_None ) {
                return i18nc( "Not available", "NA" );
            }
            return KGlobal::locale()->formatDateTime( t->wpTransmitionTime() );
        }
        case Qt::ToolTipRole: {
            const Task *task = dynamic_cast<const Task*>( node );
            if ( task == 0 ) {
                return QVariant();
            }
            int sts = task->wpTransmitionStatus();
            QString t = wpTransmitionTime( node, Qt::DisplayRole ).toString();
            if ( sts == WorkPackage::TS_Send ) {
                return i18nc( "@info:tooltip", "Latest work package sent: %1", t );
            }
            if ( sts == WorkPackage::TS_Receive ) {
                return i18nc( "@info:tooltip", "Latest work package received: %1", t );
            }
            return i18nc( "@info:tooltip", "Not available" );
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::data( const Node *n, int property, int role ) const
{
    QVariant result;
    switch ( property ) {
        // Edited by user
        case NodeName: result = name( n, role ); break;
        case NodeType: result = type( n, role ); break;
        case NodeResponsible: result = leader( n, role ); break;
        case NodeAllocation: result = allocation( n, role ); break;
        case NodeEstimateType: result = estimateType( n, role ); break;
        case NodeEstimateCalendar: result = estimateCalendar( n, role ); break;
        case NodeEstimate: result = estimate( n, role ); break;
        case NodeOptimisticRatio: result = optimisticRatio( n, role ); break;
        case NodePessimisticRatio: result = pessimisticRatio( n, role ); break;
        case NodeRisk: result = riskType( n, role ); break;
        case NodeConstraint: result = constraint( n, role ); break;
        case NodeConstraintStart: result = constraintStartTime( n, role ); break;
        case NodeConstraintEnd: result = constraintEndTime( n, role ); break;
        case NodeRunningAccount: result = runningAccount( n, role ); break;
        case NodeStartupAccount: result = startupAccount( n, role ); break;
        case NodeStartupCost: result = startupCost( n, role ); break;
        case NodeShutdownAccount: result = shutdownAccount( n, role ); break;
        case NodeShutdownCost: result = shutdownCost( n, role ); break;
        case NodeDescription: result = description( n, role ); break;

        // Based on edited values
        case NodeExpected: result = pertExpected( n->estimate(), role ); break;
        case NodeVarianceEstimate: result = varianceEstimate( n->estimate(), role ); break;
        case NodeOptimistic: result = optimisticEstimate( n->estimate(), role ); break;
        case NodePessimistic: result = pessimisticEstimate( n->estimate(), role ); break;

        // After scheduling
        case NodeStartTime: result = startTime( n, role ); break;
        case NodeEndTime: result = endTime( n, role ); break;
        case NodeEarlyStart: result = earlyStart( n, role ); break;
        case NodeEarlyFinish: result = earlyFinish( n, role ); break;
        case NodeLateStart: result = lateStart( n, role ); break;
        case NodeLateFinish: result = lateFinish( n, role ); break;
        case NodePositiveFloat: result = positiveFloat( n, role ); break;
        case NodeFreeFloat: result = freeFloat( n, role ); break;
        case NodeNegativeFloat: result = negativeFloat( n, role ); break;
        case NodeStartFloat: result = startFloat( n, role ); break;
        case NodeFinishFloat: result = finishFloat( n, role ); break;
        case NodeAssignments: result = assignedResources( n, role ); break;

        // Based on scheduled values
        case NodeDuration: result = duration( n, role ); break;
        case NodeVarianceDuration: result = varianceDuration( n, role ); break;
        case NodeOptimisticDuration: result = optimisticDuration( n, role ); break;
        case NodePessimisticDuration: result = pessimisticDuration( n, role ); break;

        // Completion
        case NodeStatus: result = status( n, role ); break;
        case NodeCompleted: result = completed( n, role ); break;
        case NodePlannedEffort: result = plannedEffortTo( n, role ); break;
        case NodeActualEffort: result = actualEffortTo( n, role ); break;
        case NodeRemainingEffort: result = remainingEffort( n, role ); break;
        case NodePlannedCost: result = plannedCostTo( n, role ); break;
        case NodeActualCost: result = actualCostTo( n, role ); break;
        case NodeActualStart: result = startedTime( n, role ); break;
        case NodeStarted: result = isStarted( n, role ); break;
        case NodeActualFinish: result = finishedTime( n, role ); break;
        case NodeFinished: result = isFinished( n, role ); break;
        case NodeStatusNote: result = note( n, role ); break;

        // Scheduling errors
        case NodeSchedulingStatus: result = nodeSchedulingStatus( n, role ); break;
        case NodeNotScheduled: result = nodeIsNotScheduled( n, role ); break;
        case NodeAssignmentMissing: result = resourceIsMissing( n, role ); break;
        case NodeResourceOverbooked: result = resourceIsOverbooked( n, role ); break;
        case NodeResourceUnavailable: result = resourceIsNotAvailable( n, role ); break;
        case NodeConstraintsError: result = schedulingConstraintsError( n, role ); break;
        case NodeEffortNotMet: result = effortNotMet( n, role ); break;
        case NodeSchedulingError: result = schedulingError( n, role ); break;

        case NodeWBSCode: result = wbsCode( n, role ); break;
        case NodeLevel: result = nodeLevel( n, role ); break;

        // Performance
        case NodeBCWS: result = nodeBCWS( n, role ); break;
        case NodeBCWP: result = nodeBCWP( n, role ); break;
        case NodeACWP: result = nodeACWP( n, role ); break;
        case NodePerformanceIndex: result = nodePerformanceIndex( n, role ); break;
        case NodeCritical: result = nodeIsCritical( n, role ); break;
        case NodeCriticalPath: result = nodeInCriticalPath( n, role ); break;

        case WPOwnerName: result = wpOwnerName( n, role ); break;
        case WPTransmitionStatus: result = wpTransmitionStatus( n, role ); break;
        case WPTransmitionTime: result = wpTransmitionTime( n, role ); break;

        default:
            //kDebug(planDbg())<<"Invalid property number: "<<property;;
            return result;
    }
    return result;
}

int NodeModel::propertyCount() const
{
    return columnMap().keyCount();
}

KUndo2Command *NodeModel::setData( Node *node, int property, const QVariant & value, int role )
{
    switch ( property ) {
        case NodeModel::NodeName: return setName( node, value, role );
        case NodeModel::NodeType: return setType( node, value, role );
        case NodeModel::NodeResponsible: return setLeader( node, value, role );
        case NodeModel::NodeAllocation: return setAllocation( node, value, role );
        case NodeModel::NodeEstimateType: return setEstimateType( node, value, role );
        case NodeModel::NodeEstimateCalendar: return setEstimateCalendar( node, value, role );
        case NodeModel::NodeEstimate: return setEstimate( node, value, role );
        case NodeModel::NodeOptimisticRatio: return setOptimisticRatio( node, value, role );
        case NodeModel::NodePessimisticRatio: return setPessimisticRatio( node, value, role );
        case NodeModel::NodeRisk: return setRiskType( node, value, role );
        case NodeModel::NodeConstraint: return setConstraint( node, value, role );
        case NodeModel::NodeConstraintStart: return setConstraintStartTime( node, value, role );
        case NodeModel::NodeConstraintEnd: return setConstraintEndTime( node, value, role );
        case NodeModel::NodeRunningAccount: return setRunningAccount( node, value, role );
        case NodeModel::NodeStartupAccount: return setStartupAccount( node, value, role );
        case NodeModel::NodeStartupCost: return setStartupCost( node, value, role );
        case NodeModel::NodeShutdownAccount: return setShutdownAccount( node, value, role );
        case NodeModel::NodeShutdownCost: return setShutdownCost( node, value, role );
        case NodeModel::NodeDescription: return setDescription( node, value, role );
        case NodeModel::NodeCompleted: return setCompletion( node, value, role );
        case NodeModel::NodeActualEffort: return setActualEffort( node, value, role );
        case NodeModel::NodeRemainingEffort: return setRemainingEffort( node, value, role );
        case NodeModel::NodeActualStart: return setStartedTime( node, value, role );
        case NodeModel::NodeActualFinish: return setFinishedTime( node, value, role );
        default:
            qWarning("data: invalid display value column %d", property);
            return 0;
    }
    return 0;
}

QVariant NodeModel::headerData( int section, int role )
{
    if ( role == Qt::DisplayRole ) {
        switch ( section ) {
            case NodeName: return i18nc( "@title:column", "Name" );
            case NodeType: return i18nc( "@title:column", "Type" );
            case NodeResponsible: return i18nc( "@title:column", "Responsible" );
            case NodeAllocation: return i18nc( "@title:column", "Allocation" );
            case NodeEstimateType: return i18nc( "@title:column", "Estimate Type" );
            case NodeEstimateCalendar: return i18nc( "@title:column", "Calendar" );
            case NodeEstimate: return i18nc( "@title:column", "Estimate" );
            case NodeOptimisticRatio: return i18nc( "@title:column", "Optimistic" ); // Ratio
            case NodePessimisticRatio: return i18nc( "@title:column", "Pessimistic" ); // Ratio
            case NodeRisk: return i18nc( "@title:column", "Risk" );
            case NodeConstraint: return i18nc( "@title:column", "Constraint" );
            case NodeConstraintStart: return i18nc( "@title:column", "Constraint Start" );
            case NodeConstraintEnd: return i18nc( "@title:column", "Constraint End" );
            case NodeRunningAccount: return i18nc( "@title:column", "Running Account" );
            case NodeStartupAccount: return i18nc( "@title:column", "Startup Account" );
            case NodeStartupCost: return i18nc( "@title:column", "Startup Cost" );
            case NodeShutdownAccount: return i18nc( "@title:column", "Shutdown Account" );
            case NodeShutdownCost: return i18nc( "@title:column", "Shutdown Cost" );
            case NodeDescription: return i18nc( "@title:column", "Description" );

            // Based on edited values
            case NodeExpected: return i18nc( "@title:column", "Expected" );
            case NodeVarianceEstimate: return i18nc( "@title:column", "Variance (Est)" );
            case NodeOptimistic: return i18nc( "@title:column", "Optimistic" );
            case NodePessimistic: return i18nc( "@title:column", "Pessimistic" );

            // After scheduling
            case NodeStartTime: return i18nc( "@title:column", "Start Time" );
            case NodeEndTime: return i18nc( "@title:column", "End Time" );
            case NodeEarlyStart: return i18nc( "@title:column", "Early Start" );
            case NodeEarlyFinish: return i18nc( "@title:column", "Early Finish" );
            case NodeLateStart: return i18nc( "@title:column", "Late Start" );
            case NodeLateFinish: return i18nc( "@title:column", "Late Finish" );
            case NodePositiveFloat: return i18nc( "@title:column", "Positive Float" );
            case NodeFreeFloat: return i18nc( "@title:column", "Free Float" );
            case NodeNegativeFloat: return i18nc( "@title:column", "Negative Float" );
            case NodeStartFloat: return i18nc( "@title:column", "Start Float" );
            case NodeFinishFloat: return i18nc( "@title:column", "Finish Float" );
            case NodeAssignments: return i18nc( "@title:column", "Assignments" );

            // Based on scheduled values
            case NodeDuration: return i18nc( "@title:column", "Duration" );
            case NodeVarianceDuration: return i18nc( "@title:column", "Variance (Dur)" );
            case NodeOptimisticDuration: return i18nc( "@title:column", "Optimistic (Dur)" );
            case NodePessimisticDuration: return i18nc( "@title:column", "Pessimistic (Dur)" );

            // Completion
            case NodeStatus: return i18nc( "@title:column", "Status" );
            // xgettext: no-c-format
            case NodeCompleted: return i18nc( "@title:column", "% Completed" );
            case NodePlannedEffort: return i18nc( "@title:column", "Planned Effort" );
            case NodeActualEffort: return i18nc( "@title:column", "Actual Effort" );
            case NodeRemainingEffort: return i18nc( "@title:column", "Remaining Effort" );
            case NodePlannedCost: return i18nc( "@title:column", "Planned Cost" );
            case NodeActualCost: return i18nc( "@title:column", "Actual Cost" );
            case NodeActualStart: return i18nc( "@title:column", "Actual Start" );
            case NodeStarted: return i18nc( "@title:column", "Started" );
            case NodeActualFinish: return i18nc( "@title:column", "Actual Finish" );
            case NodeFinished: return i18nc( "@title:column", "Finished" );
            case NodeStatusNote: return i18nc( "@title:column", "Status Note" );

            // Scheduling errors
            case NodeSchedulingStatus: return i18nc( "@title:column", "Scheduling Status" );
            case NodeNotScheduled: return i18nc( "@title:column", "Not Scheduled" );
            case NodeAssignmentMissing: return i18nc( "@title:column", "Assignment Missing" );
            case NodeResourceOverbooked: return i18nc( "@title:column", "Resource Overbooked" );
            case NodeResourceUnavailable: return i18nc( "@title:column", "Resource Unavailable" );
            case NodeConstraintsError: return i18nc( "@title:column", "Constraints Error" );
            case NodeEffortNotMet: return i18nc( "@title:column", "Effort Not Met" );
            case NodeSchedulingError: return i18nc( "@title:column", "Scheduling Error" );

            case NodeWBSCode: return i18nc( "@title:column", "WBS Code" );
            case NodeLevel: return i18nc( "@title:column Node level", "Level" );

            // Performance
            case NodeBCWS: return i18nc( "@title:column Budgeted Cost of Work Scheduled", "BCWS" );
            case NodeBCWP: return i18nc( "@title:column Budgeted Cost of Work Performed", "BCWP" );
            case NodeACWP: return i18nc( "@title:column Actual Cost of Work Performed", "ACWP" );
            case NodePerformanceIndex: return i18nc( "@title:column Schedule Performance Index", "SPI" );
            case NodeCritical: return i18nc( "@title:column", "Critical" );
            case NodeCriticalPath: return i18nc( "@title:column", "Critical Path" );

            // Work package handling
            case WPOwnerName: return i18nc( "@title:column", "Owner" );
            case WPTransmitionStatus: return i18nc( "@title:column", "Status" );
            case WPTransmitionTime: return i18nc( "@title:column", "Time" );

            default: return QVariant();
        }
    }
    if ( role == Qt::ToolTipRole ) {
        switch ( section ) {
            case NodeName: return ToolTip::nodeName();
            case NodeType: return ToolTip::nodeType();
            case NodeResponsible: return ToolTip::nodeResponsible();
            case NodeAllocation: return ToolTip::allocation();
            case NodeEstimateType: return ToolTip::estimateType();
            case NodeEstimateCalendar: return ToolTip::estimateCalendar();
            case NodeEstimate: return ToolTip::estimate();
            case NodeOptimisticRatio: return ToolTip::optimisticRatio();
            case NodePessimisticRatio: return ToolTip::pessimisticRatio();
            case NodeRisk: return ToolTip::riskType();
            case NodeConstraint: return ToolTip::nodeConstraint();
            case NodeConstraintStart: return ToolTip::nodeConstraintStart();
            case NodeConstraintEnd: return ToolTip::nodeConstraintEnd();
            case NodeRunningAccount: return ToolTip::nodeRunningAccount();
            case NodeStartupAccount: return ToolTip::nodeStartupAccount();
            case NodeStartupCost: return ToolTip::nodeStartupCost();
            case NodeShutdownAccount: return ToolTip::nodeShutdownAccount();
            case NodeShutdownCost: return ToolTip::nodeShutdownCost();
            case NodeDescription: return ToolTip::nodeDescription();

            // Based on edited values
            case NodeExpected: return ToolTip::estimateExpected();
            case NodeVarianceEstimate: return ToolTip::estimateVariance();
            case NodeOptimistic: return ToolTip::estimateOptimistic();
            case NodePessimistic: return ToolTip::estimatePessimistic();

            // After scheduling
            case NodeStartTime: return ToolTip::nodeStartTime();
            case NodeEndTime: return ToolTip::nodeEndTime();
            case NodeEarlyStart: return ToolTip::nodeEarlyStart();
            case NodeEarlyFinish: return ToolTip::nodeEarlyFinish();
            case NodeLateStart: return ToolTip::nodeLateStart();
            case NodeLateFinish: return ToolTip::nodeLateFinish();
            case NodePositiveFloat: return ToolTip::nodePositiveFloat();
            case NodeFreeFloat: return ToolTip::nodeFreeFloat();
            case NodeNegativeFloat: return ToolTip::nodeNegativeFloat();
            case NodeStartFloat: return ToolTip::nodeStartFloat();
            case NodeFinishFloat: return ToolTip::nodeFinishFloat();
            case NodeAssignments: return ToolTip::nodeAssignment();

            // Based on scheduled values
            case NodeDuration: return ToolTip::nodeDuration();
            case NodeVarianceDuration: return ToolTip::nodeVarianceDuration();
            case NodeOptimisticDuration: return ToolTip::nodeOptimisticDuration();
            case NodePessimisticDuration: return ToolTip::nodePessimisticDuration();

            // Completion
            case NodeStatus: return ToolTip::nodeStatus();
            case NodeCompleted: return ToolTip::nodeCompletion();
            case NodePlannedEffort: return ToolTip::nodePlannedEffortTo();
            case NodeActualEffort: return ToolTip::nodeActualEffortTo();
            case NodeRemainingEffort: return ToolTip::nodeRemainingEffort();
            case NodePlannedCost: return ToolTip::nodePlannedCostTo();
            case NodeActualCost: return ToolTip::nodeActualCostTo();
            case NodeActualStart: return ToolTip::completionStartedTime();
            case NodeStarted: return ToolTip::completionStarted();
            case NodeActualFinish: return ToolTip::completionFinishedTime();
            case NodeFinished: return ToolTip::completionFinished();
            case NodeStatusNote: return ToolTip::completionStatusNote();

            // Scheduling errors
            case NodeSchedulingStatus: return ToolTip::nodeSchedulingStatus();
            case NodeNotScheduled: return ToolTip::nodeNotScheduled();
            case NodeAssignmentMissing: return ToolTip::nodeAssignmentMissing();
            case NodeResourceOverbooked: return ToolTip::nodeResourceOverbooked();
            case NodeResourceUnavailable: return ToolTip::nodeResourceUnavailable();
            case NodeConstraintsError: return ToolTip::nodeConstraintsError();
            case NodeEffortNotMet: return ToolTip::nodeEffortNotMet();
            case NodeSchedulingError: return ToolTip::nodeSchedulingError();

            case NodeWBSCode: return ToolTip::nodeWBS();
            case NodeLevel: return ToolTip::nodeLevel();

            // Performance
            case NodeBCWS: return ToolTip::nodeBCWS();
            case NodeBCWP: return ToolTip::nodeBCWP();
            case NodeACWP: return ToolTip::nodeACWP();
            case NodePerformanceIndex: return ToolTip::nodePerformanceIndex();

            // Work package handling FIXME
            case WPOwnerName: return i18nc( "@info:tooltip", "Work package owner" );
            case WPTransmitionStatus: return i18nc( "@info:tooltip", "Work package status" );
            case WPTransmitionTime: return i18nc( "@info:tooltip", "Work package send/receive time" );

            default: return QVariant();
        }
    }
    if ( role == Qt::TextAlignmentRole ) {
        switch (section) {
            case NodeName:
            case NodeType:
            case NodeResponsible:
            case NodeAllocation:
            case NodeEstimateType:
            case NodeEstimateCalendar:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeEstimate:
            case NodeOptimisticRatio:
            case NodePessimisticRatio:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeRisk:
            case NodeConstraint:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeConstraintStart:
            case NodeConstraintEnd:
            case NodeRunningAccount:
            case NodeStartupAccount:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeStartupCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeShutdownAccount:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeShutdownCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeDescription:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Based on edited values
            case NodeExpected:
            case NodeVarianceEstimate:
            case NodeOptimistic:
            case NodePessimistic:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // After scheduling
            case NodeStartTime:
            case NodeEndTime:
            case NodeEarlyStart:
            case NodeEarlyFinish:
            case NodeLateStart:
            case NodeLateFinish:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodePositiveFloat:
            case NodeFreeFloat:
            case NodeNegativeFloat:
            case NodeStartFloat:
            case NodeFinishFloat:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeAssignments:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Based on scheduled values
            case NodeDuration:
            case NodeVarianceDuration:
            case NodeOptimisticDuration:
            case NodePessimisticDuration:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // Completion
            case NodeStatus:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeCompleted:
                return (int)(Qt::AlignCenter); // special, presented as a bar
            case NodePlannedEffort:
            case NodeActualEffort:
            case NodeRemainingEffort:
            case NodePlannedCost:
            case NodeActualCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeActualStart:
            case NodeStarted:
            case NodeActualFinish:
            case NodeFinished:
            case NodeStatusNote:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Scheduling errors
            case NodeSchedulingStatus:
            case NodeNotScheduled:
            case NodeAssignmentMissing:
            case NodeResourceOverbooked:
            case NodeResourceUnavailable:
            case NodeConstraintsError:
            case NodeEffortNotMet:
            case NodeSchedulingError:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            case NodeWBSCode:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeLevel:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // Performance
            case NodeBCWS:
            case NodeBCWP:
            case NodeACWP:
            case NodePerformanceIndex:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeCritical:
            case NodeCriticalPath:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            case WPOwnerName:
            case WPTransmitionStatus:
            case WPTransmitionTime:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            default:
                return QVariant();
        }
    }
    if ( role == Qt::WhatsThisRole ) {
        switch ( section ) {
            case NodeNegativeFloat: return WhatsThis::nodeNegativeFloat();

            default: return QVariant();
        }
    }
    return QVariant();
}

KUndo2Command *NodeModel::setName( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            if ( value.toString() == node->name() ) {
                return 0;
            }
            QString s = i18nc( "(qtundo-format)", "Modify name" );
            switch ( node->type() ) {
                case Node::Type_Task: s = i18nc( "(qtundo-format)", "Modify task name" ); break;
                case Node::Type_Milestone: s = i18nc( "(qtundo-format)", "Modify milestone name" ); break;
                case Node::Type_Summarytask: s = i18nc( "(qtundo-format)", "Modify summarytask name" ); break;
                case Node::Type_Project: s = i18nc( "(qtundo-format)", "Modify project name" ); break;
            }
            return new NodeModifyNameCmd( *node, value.toString(), s );
        }
    }
    return 0;
}

KUndo2Command *NodeModel::setLeader( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            if ( value.toString() != node->leader() ) {
                return new NodeModifyLeaderCmd( *node, value.toString(), i18nc( "(qtundo-format)", "Modify responsible" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setAllocation( Node */*node*/, const QVariant &/*value*/, int /*role*/ )
{
    return 0;
}

KUndo2Command *NodeModel::setDescription( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toString() == node->description() ) {
                return 0;
            }
            return new NodeModifyDescriptionCmd( *node, value.toString(), i18nc( "(qtundo-format)", "Modify task description" ) );
    }
    return 0;
}

KUndo2Command *NodeModel::setType( Node *, const QVariant &, int )
{
    return 0;
}

KUndo2Command *NodeModel::setConstraint( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            Node::ConstraintType v;
            QStringList lst = node->constraintList( false );
            if ( lst.contains( value.toString() ) ) {
                v = Node::ConstraintType( lst.indexOf( value.toString() ) );
            } else {
                v = Node::ConstraintType( value.toInt() );
            }
            //kDebug(planDbg())<<v;
            if ( v != node->constraint() ) {
                return new NodeModifyConstraintCmd( *node, v, i18nc( "(qtundo-format)", "Modify constraint type" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setConstraintStartTime( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            QDateTime dt = value.toDateTime();
            dt.setTime( QTime( dt.time().hour(), dt.time().minute() ) ); // reset possible secs/msecs
            if ( dt != node->constraintStartTime() ) {
                return new NodeModifyConstraintStartTimeCmd( *node, dt, i18nc( "(qtundo-format)", "Modify constraint start time" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setConstraintEndTime( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            QDateTime dt = value.toDateTime();
            dt.setTime( QTime( dt.time().hour(), dt.time().minute() ) ); // reset possible secs/msecs
            if ( dt != node->constraintEndTime() ) {
                return new NodeModifyConstraintEndTimeCmd( *node, dt, i18nc( "(qtundo-format)", "Modify constraint end time" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setEstimateType( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole: {
            Estimate::Type v;
            QStringList lst = node->estimate()->typeToStringList( false );
            if ( lst.contains( value.toString() ) ) {
                v = Estimate::Type( lst.indexOf( value.toString() ) );
            } else {
                v = Estimate::Type( value.toInt() );
            }
            if ( v != node->estimate()->type() ) {
                return new ModifyEstimateTypeCmd( *node, node->estimate()->type(), v, i18nc( "(qtundo-format)", "Modify estimate type" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setEstimateCalendar( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole: {
            Calendar *c = 0;
            Calendar *old = node->estimate()->calendar();
            if ( value.toInt() > 0 ) {
                QStringList lst = estimateCalendar( node, Role::EnumList ).toStringList();
                if ( value.toInt() < lst.count() ) {
                    c = m_project->calendarByName( lst.at( value.toInt() ) );
                }
            }
            if ( c != old ) {
                return new ModifyEstimateCalendarCmd( *node, old, c, i18nc( "(qtundo-format)", "Modify estimate calendar" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setEstimate( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole: {
            double d;
            Duration::Unit unit;
            if ( value.toList().count() == 2 ) {
                d =  value.toList()[0].toDouble();
                unit = static_cast<Duration::Unit>( value.toList()[1].toInt() );
            } else if ( value.canConvert<QString>() ) {
                bool ok = Duration::valueFromString( value.toString(), d, unit );
                if ( ! ok ) {
                    return 0;
                }
            } else {
                return 0;
            }
            //kDebug(planDbg())<<d<<","<<unit<<" ->"<<value.toList()[1].toInt();
            MacroCommand *cmd = 0;
            if ( d != node->estimate()->expectedEstimate() ) {
                if ( cmd == 0 ) cmd = new MacroCommand( i18nc( "(qtundo-format)", "Modify estimate" ) );
                cmd->addCommand( new ModifyEstimateCmd( *node, node->estimate()->expectedEstimate(), d ) );
            }
            if ( unit != node->estimate()->unit() ) {
                if ( cmd == 0 ) cmd = new MacroCommand( i18nc( "(qtundo-format)", "Modify estimate" ) );
                cmd->addCommand( new ModifyEstimateUnitCmd( *node, node->estimate()->unit(), unit ) );
            }
            if ( cmd ) {
                return cmd;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setOptimisticRatio( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toInt() != node->estimate()->optimisticRatio() ) {
                return new EstimateModifyOptimisticRatioCmd( *node, node->estimate()->optimisticRatio(), value.toInt(), i18nc( "(qtundo-format)", "Modify optimistic estimate" ) );
            }
            break;
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setPessimisticRatio( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole:
            if ( value.toInt() != node->estimate()->pessimisticRatio() ) {
                return new EstimateModifyPessimisticRatioCmd( *node, node->estimate()->pessimisticRatio(), value.toInt(), i18nc( "(qtundo-format)", "Modify pessimistic estimate" ) );
            }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setRiskType( Node *node, const QVariant &value, int role )
{
    if ( node->estimate() == 0 ) {
        return 0;
    }
    switch ( role ) {
        case Qt::EditRole: {
            int val = 0;
            QStringList lst = node->estimate()->risktypeToStringList( false );
            if ( lst.contains( value.toString() ) ) {
                val = lst.indexOf( value.toString() );
            } else {
                val = value.toInt();
            }
            if ( val != node->estimate()->risktype() ) {
                Estimate::Risktype v = Estimate::Risktype( val );
                return new EstimateModifyRiskCmd( *node, node->estimate()->risktype(), v, i18nc( "(qtundo-format)", "Modify risk type" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setRunningAccount( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            //kDebug(planDbg())<<node->name();
            QStringList lst = runningAccount( node, Role::EnumList ).toStringList();
            if ( value.toInt() < lst.count() ) {
                Account *a = m_project->accounts().findAccount( lst.at( value.toInt() ) );
                Account *old = node->runningAccount();
                if ( old != a ) {
                    return new NodeModifyRunningAccountCmd( *node, old, a, i18nc( "(qtundo-format)", "Modify running account" ) );
                }
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setStartupAccount( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            //kDebug(planDbg())<<node->name();
            QStringList lst = startupAccount( node, Role::EnumList ).toStringList();
            if ( value.toInt() < lst.count() ) {
                Account *a = m_project->accounts().findAccount( lst.at( value.toInt() ) );
                Account *old = node->startupAccount();
                //kDebug(planDbg())<<(value.toInt())<<";"<<(lst.at( value.toInt()))<<":"<<a;
                if ( old != a ) {
                    return new NodeModifyStartupAccountCmd( *node, old, a, i18nc( "(qtundo-format)", "Modify startup account" ) );
                }
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setStartupCost( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            double v = KGlobal::locale()->readMoney( value.toString() );
            if ( v != node->startupCost() ) {
                return new NodeModifyStartupCostCmd( *node, v, i18nc( "(qtundo-format)", "Modify startup cost" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setShutdownAccount( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            //kDebug(planDbg())<<node->name();
            QStringList lst = shutdownAccount( node, Role::EnumList ).toStringList();
            if ( value.toInt() < lst.count() ) {
                Account *a = m_project->accounts().findAccount( lst.at( value.toInt() ) );
                Account *old = node->shutdownAccount();
                if ( old != a ) {
                    return new NodeModifyShutdownAccountCmd( *node, old, a, i18nc( "(qtundo-format)", "Modify shutdown account" ) );
                }
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setShutdownCost( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            double v = KGlobal::locale()->readMoney( value.toString() );
            if ( v != node->shutdownCost() ) {
                return new NodeModifyShutdownCostCmd( *node, v, i18nc( "(qtundo-format)", "Modify shutdown cost" ) );
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setCompletion( Node */*node*/, const QVariant &/*value*/, int /*role*/ )
{
    return 0;
}

KUndo2Command *NodeModel::setRemainingEffort( Node *node, const QVariant &value, int role )
{
    if ( role == Qt::EditRole && node->type() == Node::Type_Task ) {
        Task *t = static_cast<Task*>( node );
        double d( value.toList()[0].toDouble() );
        Duration::Unit unit = static_cast<Duration::Unit>( value.toList()[1].toInt() );
        Duration dur( d, unit );
        return new ModifyCompletionRemainingEffortCmd( t->completion(), QDate::currentDate(), dur, i18nc( "(qtundo-format)", "Modify remaining effort" ) );
    }
    return 0;
}

KUndo2Command *NodeModel::setActualEffort( Node *node, const QVariant &value, int role )
{
    if ( role == Qt::EditRole && node->type() == Node::Type_Task ) {
        Task *t = static_cast<Task*>( node );
        double d( value.toList()[0].toDouble() );
        Duration::Unit unit = static_cast<Duration::Unit>( value.toList()[1].toInt() );
        Duration dur( d, unit );
        return new ModifyCompletionActualEffortCmd( t->completion(), QDate::currentDate(), dur, i18nc( "(qtundo-format)", "Modify actual effort" ) );
    }
    return 0;
}

KUndo2Command *NodeModel::setStartedTime( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>( node );
            if ( t == 0 ) {
                return 0;
            }
            MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify actual start time" ) );
            if ( ! t->completion().isStarted() ) {
                m->addCommand( new ModifyCompletionStartedCmd( t->completion(), true ) );
            }
            m->addCommand( new ModifyCompletionStartTimeCmd( t->completion(), value.toDateTime() ) );
            if ( t->type() == Node::Type_Milestone ) {
                m->addCommand( new ModifyCompletionFinishedCmd( t->completion(), true ) );
                m->addCommand( new ModifyCompletionFinishTimeCmd( t->completion(), value.toDateTime() ) );
                if ( t->completion().percentFinished() < 100 ) {
                    Completion::Entry *e = new Completion::Entry( 100, Duration::zeroDuration, Duration::zeroDuration );
                    m->addCommand( new AddCompletionEntryCmd( t->completion(), value.toDate(), e ) );
                }
            }
            return m;
        }
        default:
            break;
    }
    return 0;
}

KUndo2Command *NodeModel::setFinishedTime( Node *node, const QVariant &value, int role )
{
    switch ( role ) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>( node );
            if ( t == 0 ) {
                return 0;
            }
            MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify actual finish time" ) );
            if ( ! t->completion().isFinished() ) {
                m->addCommand( new ModifyCompletionFinishedCmd( t->completion(), true ) );
                if ( t->completion().percentFinished() < 100 ) {
                    Completion::Entry *e = new Completion::Entry( 100, Duration::zeroDuration, Duration::zeroDuration );
                    m->addCommand( new AddCompletionEntryCmd( t->completion(), value.toDate(), e ) );
                }
            }
            m->addCommand( new ModifyCompletionFinishTimeCmd( t->completion(), value.toDateTime() ) );
            if ( t->type() == Node::Type_Milestone ) {
                m->addCommand( new ModifyCompletionStartedCmd( t->completion(), true ) );
                m->addCommand( new ModifyCompletionStartTimeCmd( t->completion(), value.toDateTime() ) );
            }
            return m;
        }
        default:
            break;
    }
    return 0;
}

//----------------------------
NodeItemModel::NodeItemModel( QObject *parent )
    : ItemModelBase( parent ),
    m_node( 0 ),
    m_projectshown( false )
{
    setReadOnly( NodeModel::NodeDescription, true );
}

NodeItemModel::~NodeItemModel()
{
}

void NodeItemModel::setShowProject( bool on )
{
    m_projectshown = on;
    reset();
    emit projectShownChanged( on );
}

void NodeItemModel::slotNodeToBeInserted( Node *parent, int row )
{
    //kDebug(planDbg())<<parent->name()<<"; "<<row;
    Q_ASSERT( m_node == 0 );
    m_node = parent;
    beginInsertRows( index( parent ), row, row );
}

void NodeItemModel::slotNodeInserted( Node *node )
{
    //kDebug(planDbg())<<node->parentNode()->name()<<"-->"<<node->name();
    Q_ASSERT( node->parentNode() == m_node );
    endInsertRows();
    m_node = 0;
    emit nodeInserted( node );
}

void NodeItemModel::slotNodeToBeRemoved( Node *node )
{
    //kDebug(planDbg())<<node->name();
    Q_ASSERT( m_node == 0 );
    m_node = node;
    int row = index( node ).row();
    beginRemoveRows( index( node->parentNode() ), row, row );
}

void NodeItemModel::slotNodeRemoved( Node *node )
{
    //kDebug(planDbg())<<node->name();
    Q_ASSERT( node == m_node );
#ifdef NDEBUG
    Q_UNUSED(node)
#endif
    endRemoveRows();
    m_node = 0;
}

void NodeItemModel::slotNodeToBeMoved( Node *node, int pos, Node *newParent, int newPos )
{
    //kDebug(planDbg())<<node->parentNode()->name()<<pos<<":"<<newParent->name()<<newPos;
    beginMoveRows( index( node->parentNode() ), pos, pos, index( newParent ), newPos );
}

void NodeItemModel::slotNodeMoved( Node *node )
{
    Q_UNUSED( node );
    //kDebug(planDbg())<<node->parentNode()->name()<<node->parentNode()->indexOf( node );
    endMoveRows();
}

void NodeItemModel::slotLayoutChanged()
{
    //kDebug(planDbg())<<node->name();
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void NodeItemModel::slotProjectCalulated(ScheduleManager *sm)
{
    kDebug(planDbg())<<m_manager<<sm;
    if ( sm && sm == m_manager ) {
        slotLayoutChanged();
    }
}

void NodeItemModel::slotWbsDefinitionChanged()
{
    kDebug(planDbg());
    if ( m_project == 0 ) {
        return;
    }
    if ( m_projectshown ) {
        QModelIndex idx = createIndex( 0, NodeModel::NodeWBSCode, m_project );
        emit dataChanged( idx, idx );
    }
    foreach ( Node *n, m_project->allNodes() ) {
        int row = n->parentNode()->indexOf( n );
        QModelIndex idx = createIndex( row, NodeModel::NodeWBSCode, n );
        emit dataChanged( idx, idx );
    }
}


void NodeItemModel::setProject( Project *project )
{
    if ( m_project ) {
        disconnect( m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()) );
        disconnect( m_project, SIGNAL(wbsDefinitionChanged()), this, SLOT(slotWbsDefinitionChanged()) );
        disconnect( m_project, SIGNAL(nodeChanged(Node*)), this, SLOT(slotNodeChanged(Node*)) );
        disconnect( m_project, SIGNAL(nodeToBeAdded(Node*,int)), this, SLOT(slotNodeToBeInserted(Node*,int)) );
        disconnect( m_project, SIGNAL(nodeToBeRemoved(Node*)), this, SLOT(slotNodeToBeRemoved(Node*)) );

        disconnect( m_project, SIGNAL(nodeToBeMoved(Node*,int,Node*,int)), this, SLOT(slotNodeToBeMoved(Node*,int,Node*,int)) );
        disconnect( m_project, SIGNAL(nodeMoved(Node*)), this, SLOT(slotNodeMoved(Node*)) );

        disconnect( m_project, SIGNAL(nodeAdded(Node*)), this, SLOT(slotNodeInserted(Node*)) );
        disconnect( m_project, SIGNAL(nodeRemoved(Node*)), this, SLOT(slotNodeRemoved(Node*)) );
        disconnect( m_project, SIGNAL(projectCalculated(ScheduleManager*)), this, SLOT(slotProjectCalulated(ScheduleManager*)));
    }
    m_project = project;
    kDebug(planDbg())<<this<<m_project<<"->"<<project;
    m_nodemodel.setProject( project );
    if ( project ) {
        connect( m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()) );
        connect( m_project, SIGNAL(wbsDefinitionChanged()), this, SLOT(slotWbsDefinitionChanged()) );
        connect( m_project, SIGNAL(nodeChanged(Node*)), this, SLOT(slotNodeChanged(Node*)) );
        connect( m_project, SIGNAL(nodeToBeAdded(Node*,int)), this, SLOT(slotNodeToBeInserted(Node*,int)) );
        connect( m_project, SIGNAL(nodeToBeRemoved(Node*)), this, SLOT(slotNodeToBeRemoved(Node*)) );

        connect( m_project, SIGNAL(nodeToBeMoved(Node*,int,Node*,int)), this, SLOT(slotNodeToBeMoved(Node*,int,Node*,int)) );
        connect( m_project, SIGNAL(nodeMoved(Node*)), this, SLOT(slotNodeMoved(Node*)) );

        connect( m_project, SIGNAL(nodeAdded(Node*)), this, SLOT(slotNodeInserted(Node*)) );
        connect( m_project, SIGNAL(nodeRemoved(Node*)), this, SLOT(slotNodeRemoved(Node*)) );
        connect( m_project, SIGNAL(projectCalculated(ScheduleManager*)), this, SLOT(slotProjectCalulated(ScheduleManager*)));
    }
    reset();
}

void NodeItemModel::setScheduleManager( ScheduleManager *sm )
{
    if ( m_nodemodel.manager() ) {
    }
    m_nodemodel.setManager( sm );
    ItemModelBase::setScheduleManager( sm );
    if ( sm ) {
    }
    kDebug(planDbg())<<this<<sm;
    reset();
}

Qt::ItemFlags NodeItemModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags( index );
    if ( !index.isValid() ) {
        if ( m_readWrite ) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    }
    if ( isColumnReadOnly( index.column() ) ) {
        //kDebug(planDbg())<<"Column is readonly:"<<index.column();
        return flags;
    }
    Node *n = node( index );
    if ( m_readWrite && n != 0 ) {
        bool baselined = n->isBaselined();
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch ( index.column() ) {
            case NodeModel::NodeName: // name
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeType: break; // Node type
            case NodeModel::NodeResponsible: // Responsible
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeAllocation: // allocation
                if ( n->type() == Node::Type_Task ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeEstimateType: // estimateType
            {
                if ( ! baselined && ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeEstimate: // estimate
            {
                if ( ! baselined && ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeOptimisticRatio: // optimisticRatio
            case NodeModel::NodePessimisticRatio: // pessimisticRatio
            {
                if ( ! baselined && n->type() == Node::Type_Task ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeEstimateCalendar:
            {
                if ( ! baselined && n->type() == Node::Type_Task )
                {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeRisk: // risktype
            {
                if ( ! baselined && n->type() == Node::Type_Task ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeConstraint: // constraint type
                if ( ! baselined && ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeConstraintStart: { // constraint start
                if ( ! baselined && n->type() == Node::Type_Project ) {
                    flags |= Qt::ItemIsEditable;
                    break;
                }
                if ( ! baselined && ! ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    break;
                }
                flags |= Qt::ItemIsEditable;
                break;
            }
            case NodeModel::NodeConstraintEnd: { // constraint end
                if ( ! baselined && n->type() == Node::Type_Project ) {
                    flags |= Qt::ItemIsEditable;
                    break;
                }
                if ( ! baselined && ! ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    break;
                }
                flags |= Qt::ItemIsEditable;
                break;
            }
            case NodeModel::NodeRunningAccount: // running account
                if ( ! baselined && n->type() == Node::Type_Task ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeStartupAccount: // startup account
            case NodeModel::NodeStartupCost: // startup cost
            case NodeModel::NodeShutdownAccount: // shutdown account
            case NodeModel::NodeShutdownCost: { // shutdown cost
                if ( ! baselined && ( n->type() == Node::Type_Task || n->type() == Node::Type_Milestone ) ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeDescription: // description
                flags |= Qt::ItemIsEditable;
                break;
            default:
                break;
        }
        Task *t = static_cast<Task*>( n );
        if ( manager() && t->isScheduled( id() ) ) {
            if ( ! t->completion().isStarted() ) {
                switch ( index.column() ) {
                    case NodeModel::NodeActualStart:
                        flags |= Qt::ItemIsEditable;
                        break;
                    case NodeModel::NodeActualFinish:
                        if ( t->type() == Node::Type_Milestone ) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;
                    case NodeModel::NodeCompleted:
                        if ( t->state() & Node::State_ReadyToStart ) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;

                    default: break;
                }
            } else if ( ! t->completion().isFinished() ) {
                switch ( index.column() ) {
                    case NodeModel::NodeActualFinish:
                    case NodeModel::NodeCompleted:
                    case NodeModel::NodeRemainingEffort:
                        flags |= Qt::ItemIsEditable;
                        break;
                    case NodeModel::NodeActualEffort:
                        if ( t->completion().entrymode() == Completion::EnterEffortPerTask || t->completion().entrymode() == Completion::EnterEffortPerResource ) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;
                    default: break;
                }
            }
        }
    }
    return flags;
}


QModelIndex NodeItemModel::parent( const QModelIndex &index ) const
{
    if ( ! index.isValid() ) {
        return QModelIndex();
    }
    Node *n = node( index );
    if ( n == 0 || n == m_project ) {
        return QModelIndex();
    }
    Node *p = n->parentNode();
    if ( p == m_project ) {
        return m_projectshown ? createIndex( 0, 0, p ) : QModelIndex();
    }
    int row = p->parentNode()->indexOf( p );
    if ( row == -1 ) {
        return QModelIndex();
    }
    return createIndex( row, 0, p );
}

QModelIndex NodeItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( parent.isValid() ) {
        Q_ASSERT( parent.model() == this );
    }
    //kDebug(planDbg())<<parent<<row<<column;
    if ( m_project == 0 || column < 0 || column >= columnCount() || row < 0 ) {
        //kDebug(planDbg())<<m_project<<parent<<"No index for"<<row<<","<<column;
        return QModelIndex();
    }
    if ( m_projectshown && ! parent.isValid() ) {
        return createIndex( row, column, m_project );
    }
    Node *p = node( parent );
    if ( row >= p->numChildren() ) {
        kError()<<p->name()<<" row too high"<<row<<","<<column;
        return QModelIndex();
    }
    // now get the internal pointer for the index
    Node *n = p->childNode( row );
    QModelIndex idx = createIndex(row, column, n);
    //kDebug(planDbg())<<idx;
    return idx;
}

QModelIndex NodeItemModel::index( const Node *node, int column ) const
{
    if ( m_project == 0 || node == 0 ) {
        return QModelIndex();
    }
    Node *par = node->parentNode();
    if ( par ) {
        //kDebug(planDbg())<<par<<"-->"<<node;
        return createIndex( par->indexOf( node ), column, const_cast<Node*>(node) );
    }
    if ( m_projectshown && node == m_project ) {
        return createIndex( 0, column, m_project );
    }
    //kDebug(planDbg())<<node;
    return QModelIndex();
}

bool NodeItemModel::setType( Node *, const QVariant &, int )
{
    return false;
}

bool NodeItemModel::setAllocation( Node *node, const QVariant &value, int role )
{
    Task *task = qobject_cast<Task*>( node );
    if ( task == 0 ) {
        return false;
    }
    switch ( role ) {
        case Qt::EditRole:
        {
            MacroCommand *cmd = 0;
            QStringList res = m_project->resourceNameList();
            QStringList req = node->requestNameList();
            QStringList alloc;
            foreach ( const QString &s, value.toString().split( QRegExp(" *, *"), QString::SkipEmptyParts ) ) {
                alloc << s.trimmed();
            }
            // first add all new resources (to "default" group)
            ResourceGroup *pargr = m_project->groupByName( i18n( "Resources" ) );
            foreach ( const QString &s, alloc ) {
                Resource *r = m_project->resourceByName( s.trimmed() );
                if ( r != 0 ) {
                    continue;
                }
                if ( cmd == 0 ) cmd = new MacroCommand( i18nc( "(qtundo-format)", "Add resource" ) );
                if ( pargr == 0 ) {
                    pargr = new ResourceGroup();
                    pargr->setName( i18n( "Resources" ) );
                    cmd->addCommand( new AddResourceGroupCmd( m_project, pargr ) );
                    //kDebug(planDbg())<<"add group:"<<pargr->name();
                }
                r = new Resource();
                r->setName( s.trimmed() );
                cmd->addCommand( new AddResourceCmd( pargr, r ) );
                //kDebug(planDbg())<<"add resource:"<<r->name();
                emit executeCommand( cmd );
                cmd = 0;
            }

            QString c = i18nc( "(qtundo-format)", "Modify resource allocations" );
            // Handle deleted requests
            foreach ( const QString &s, req ) {
                // if a request is not in alloc, it must have been be removed by the user
                if ( alloc.indexOf( s ) == -1 ) {
                    // remove removed resource request
                    ResourceRequest *r = node->resourceRequest( s );
                    if ( r ) {
                        if ( cmd == 0 ) cmd = new MacroCommand( c );
                        //kDebug(planDbg())<<"delete request:"<<r->resource()->name()<<" group:"<<r->parent()->group()->name();
                        cmd->addCommand( new RemoveResourceRequestCmd( r->parent(), r ) );
                    }
                }
            }
            // Handle new requests
            QMap<ResourceGroup*, ResourceGroupRequest*> groupmap;
            foreach ( const QString &s, alloc ) {
                // if an allocation is not in req, it must be added
                if ( req.indexOf( s ) == -1 ) {
                    ResourceGroup *pargr = 0;
                    Resource *r = m_project->resourceByName( s );
                    if ( r == 0 ) {
                        // Handle request to non exixting resource
                        pargr = m_project->groupByName( i18n( "Resources" ) );
                        if ( pargr == 0 ) {
                            pargr = new ResourceGroup();
                            pargr->setName( i18n( "Resources" ) );
                            cmd->addCommand( new AddResourceGroupCmd( m_project, pargr ) );
                            //kDebug(planDbg())<<"add group:"<<pargr->name();
                        }
                        r = new Resource();
                        r->setName( s );
                        cmd->addCommand( new AddResourceCmd( pargr, r ) );
                        //kDebug(planDbg())<<"add resource:"<<r->name();
                        emit executeCommand( cmd );
                        cmd = 0;
                    } else {
                        pargr = r->parentGroup();
                        //kDebug(planDbg())<<"add '"<<r->name()<<"' to group:"<<pargr;
                    }
                    // add request
                    ResourceGroupRequest *g = node->resourceGroupRequest( pargr );
                    if ( g == 0 ) {
                        g = groupmap.value( pargr );
                    }
                    if ( g == 0 ) {
                        // create a group request
                        if ( cmd == 0 ) cmd = new MacroCommand( c );
                        g = new ResourceGroupRequest( pargr );
                        cmd->addCommand( new AddResourceGroupRequestCmd( *task, g ) );
                        groupmap.insert( pargr, g );
                        //kDebug(planDbg())<<"add group request:"<<g;
                    }
                    if ( cmd == 0 ) cmd = new MacroCommand( c );
                    cmd->addCommand( new AddResourceRequestCmd( g, new ResourceRequest( r, r->units() ) ) );
                    //kDebug(planDbg())<<"add request:"<<r->name()<<" group:"<<g;
                }
            }
            if ( cmd ) {
                emit executeCommand( cmd );
                return true;
            }
        }
    }
    return false;
}

bool NodeItemModel::setCompletion( Node *node, const QVariant &value, int role )
{
    kDebug(planDbg())<<node->name()<<value<<role;
    if ( role != Qt::EditRole ) {
        return 0;
    }
    if ( node->type() == Node::Type_Task ) {
        Completion &c = static_cast<Task*>( node )->completion();
        QDateTime dt = QDateTime::currentDateTime();
        QDate date = dt.date();
        MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Modify completion" ) );
        if ( ! c.isStarted() ) {
            m->addCommand( new ModifyCompletionStartTimeCmd( c, dt ) );
            m->addCommand( new ModifyCompletionStartedCmd( c, true ) );
        }
        m->addCommand( new ModifyCompletionPercentFinishedCmd( c, date, value.toInt() ) );
        if ( value.toInt() == 100 ) {
            m->addCommand( new ModifyCompletionFinishTimeCmd( c, dt ) );
            m->addCommand( new ModifyCompletionFinishedCmd( c, true ) );
        }
        emit executeCommand( m ); // also adds a new entry if necessary
        if ( c.entrymode() == Completion::EnterCompleted ) {
            Duration planned = static_cast<Task*>( node )->plannedEffort( m_nodemodel.id() );
            Duration actual = ( planned * value.toInt() ) / 100;
            kDebug(planDbg())<<planned.toString()<<value.toInt()<<actual.toString();
            NamedCommand *cmd = new ModifyCompletionActualEffortCmd( c, date, actual );
            cmd->execute();
            m->addCommand( cmd );
            cmd = new ModifyCompletionRemainingEffortCmd( c, date, planned - actual  );
            cmd->execute();
            m->addCommand( cmd );
        }
        return true;
    }
    if ( node->type() == Node::Type_Milestone ) {
        Completion &c = static_cast<Task*>( node )->completion();
        if ( value.toInt() > 0 ) {
            QDateTime dt = QDateTime::currentDateTime();
            QDate date = dt.date();
            MacroCommand *m = new MacroCommand( i18nc( "(qtundo-format)", "Set finished" ) );
            m->addCommand( new ModifyCompletionStartTimeCmd( c, dt ) );
            m->addCommand( new ModifyCompletionStartedCmd( c, true ) );
            m->addCommand( new ModifyCompletionFinishTimeCmd( c, dt ) );
            m->addCommand( new ModifyCompletionFinishedCmd( c, true ) );
            m->addCommand( new ModifyCompletionPercentFinishedCmd( c, date, 100 ) );
            emit executeCommand( m ); // also adds a new entry if necessary
            return true;
        }
        return false;
    }
    return false;
}

QVariant NodeItemModel::data( const QModelIndex &index, int role ) const
{
    if ( role == Qt::TextAlignmentRole ) {
        return headerData( index.column(), Qt::Horizontal, role );
    }
    Node *n = node( index );
    if ( role == Role::Object ) {
        return n ? QVariant::fromValue( static_cast<QObject*>( n ) ) : QVariant();
    }
    QVariant result;
    if ( n != 0 ) {
        result = m_nodemodel.data( n, index.column(), role );
        //kDebug(planDbg())<<n->name()<<": "<<index.column()<<", "<<role<<result;
    }
    if ( role == Qt::EditRole ) {
        switch ( index.column() ) {
            case NodeModel::NodeActualStart:
            case NodeModel::NodeActualFinish:
                if ( ! result.isValid() ) {
                    return QDateTime::currentDateTime();
                }
            break;
        }
    }
    return result;
}

bool NodeItemModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    if ( ! index.isValid() ) {
        return ItemModelBase::setData( index, value, role );
    }
    if ( ( flags(index) &Qt::ItemIsEditable ) == 0 || role != Qt::EditRole ) {
        kWarning()<<index<<value<<role;
        return false;
    }
    Node *n = node( index );
    if ( n ) {
        switch ( index.column() ) {
            case NodeModel::NodeCompleted: return setCompletion( n, value, role );
            case NodeModel::NodeAllocation: return setAllocation( n, value, role );
            default: {
                KUndo2Command *c = m_nodemodel.setData( n, index.column(), value, role );
                if ( c ) {
                    emit executeCommand( c );
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

QVariant NodeItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        if ( role == Qt::DisplayRole || role == Qt::TextAlignmentRole ) {
            return m_nodemodel.headerData( section, role );
        }
    }
    if ( role == Qt::ToolTipRole ) {
        return NodeModel::headerData( section, role );
    } else if ( role == Qt::WhatsThisRole ) {
        return NodeModel::headerData( section, role );
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *NodeItemModel::createDelegate( int column, QWidget *parent ) const
{
    switch ( column ) {
        //case NodeModel::NodeAllocation: return new ??Delegate( parent );
        case NodeModel::NodeEstimateType: return new EnumDelegate( parent );
        case NodeModel::NodeEstimateCalendar: return new EnumDelegate( parent );
        case NodeModel::NodeEstimate: return new DurationSpinBoxDelegate( parent );
        case NodeModel::NodeOptimisticRatio: return new SpinBoxDelegate( parent );
        case NodeModel::NodePessimisticRatio: return new SpinBoxDelegate( parent );
        case NodeModel::NodeRisk: return new EnumDelegate( parent );
        case NodeModel::NodeConstraint: return new EnumDelegate( parent );
        case NodeModel::NodeConstraintStart: return new DateTimeCalendarDelegate( parent );
        case NodeModel::NodeConstraintEnd: return new DateTimeCalendarDelegate( parent );
        case NodeModel::NodeRunningAccount: return new EnumDelegate( parent );
        case NodeModel::NodeStartupAccount: return new EnumDelegate( parent );
        case NodeModel::NodeStartupCost: return new MoneyDelegate( parent );
        case NodeModel::NodeShutdownAccount: return new EnumDelegate( parent );
        case NodeModel::NodeShutdownCost: return new MoneyDelegate( parent );

        case NodeModel::NodeCompleted: return new TaskCompleteDelegate( parent );
        case NodeModel::NodeRemainingEffort: return new DurationSpinBoxDelegate( parent );
        case NodeModel::NodeActualEffort: return new DurationSpinBoxDelegate( parent );

        default: return 0;
    }
    return 0;
}

int NodeItemModel::columnCount( const QModelIndex &/*parent*/ ) const
{
    return m_nodemodel.propertyCount();
}

int NodeItemModel::rowCount( const QModelIndex &parent ) const
{
    if ( m_projectshown && ! parent.isValid() ) {
        return m_project == 0 ? 0 : 1;
    }
    Node *p = node( parent );
    return p == 0 ? 0 : p->numChildren();
}

Qt::DropActions NodeItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList NodeItemModel::mimeTypes() const
{
    return QStringList() << "application/x-vnd.kde.plan.nodeitemmodel.internal"
                        << "application/x-vnd.kde.plan.resourceitemmodel.internal"
                        << "application/x-vnd.kde.plan.project"
                        << "text/uri-list";
}

QMimeData *NodeItemModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *m = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> rows;
    foreach (const QModelIndex &index, indexes) {
        if ( index.isValid() && !rows.contains( index.row() ) ) {
            //kDebug(planDbg())<<index.row();
            Node *n = node( index );
            if ( n ) {
                rows << index.row();
                stream << n->id();
            }
        }
    }
    m->setData("application/x-vnd.kde.plan.nodeitemmodel.internal", encodedData);
    return m;
}

bool NodeItemModel::dropAllowed( const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data )
{
    kDebug(planDbg());
    if ( m_projectshown && ! index.isValid() ) {
        return false;
    }
    Node *dn = node( index ); // returns project if ! index.isValid()
    if ( dn == 0 ) {
        kError()<<"no node (or project) to drop on!";
        return false; // hmmm
    }
    if ( data->hasFormat("application/x-vnd.kde.plan.resourceitemmodel.internal") ) {
        switch ( dropIndicatorPosition ) {
            case ItemModelBase::OnItem:
                if ( index.column() == NodeModel::NodeAllocation ) {
                    kDebug(planDbg())<<"resource:"<<index<<(dn->type() == Node::Type_Task);
                    return dn->type() == Node::Type_Task;
                } else if ( index.column() == NodeModel::NodeResponsible ) {
                    kDebug(planDbg())<<"resource:"<<index<<true;
                    return true;
                }
                break;
            default:
                break;
        }
    } else if ( data->hasFormat( "application/x-vnd.kde.plan.nodeitemmodel.internal")
                || data->hasFormat( "application/x-vnd.kde.plan.project" )
                || data->hasUrls() )
    {
        switch ( dropIndicatorPosition ) {
            case ItemModelBase::AboveItem:
            case ItemModelBase::BelowItem:
                // dn == sibling, if not project
                if ( dn == m_project ) {
                    return dropAllowed( dn, data );
                }
                return dropAllowed( dn->parentNode(), data );
            case ItemModelBase::OnItem:
                // dn == new parent
                return dropAllowed( dn, data );
            default:
                break;
        }
    } else {
        kDebug(planDbg())<<"Unknown mimetype";
    }
    return false;
}

QList<Resource*> NodeItemModel::resourceList( QDataStream &stream )
{
    QList<Resource*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        kDebug(planDbg())<<"id"<<id;
        Resource *r = m_project->findResource( id );
        if ( r ) {
            lst << r;
        }
    }
    kDebug(planDbg())<<lst;
    return lst;
}

bool NodeItemModel::dropAllowed( Node *on, const QMimeData *data )
{
    if ( ! m_projectshown && on == m_project ) {
        return true;
    }
    if ( on->isBaselined() && on->type() != Node::Type_Summarytask ) {
        return false;
    }
    if ( data->hasFormat( "application/x-vnd.kde.plan.nodeitemmodel.internal" ) ) {
        QByteArray encodedData = data->data( "application/x-vnd.kde.plan.nodeitemmodel.internal" );
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<Node*> lst = nodeList( stream );
        foreach ( Node *n, lst ) {
            if ( n->type() == Node::Type_Project || on == n || on->isChildOf( n ) ) {
                return false;
            }
        }
        lst = removeChildNodes( lst );
        foreach ( Node *n, lst ) {
            if ( ! m_project->canMoveTask( n, on ) ) {
                return false;
            }
        }
    }
    return true;
}

QList<Node*> NodeItemModel::nodeList( QDataStream &stream )
{
    QList<Node*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Node *node = m_project->findNode( id );
        if ( node ) {
            lst << node;
        }
    }
    return lst;
}

QList<Node*> NodeItemModel::removeChildNodes( const QList<Node*> &nodes )
{
    QList<Node*> lst;
    foreach ( Node *node, nodes ) {
        bool ins = true;
        foreach ( Node *n, lst ) {
            if ( node->isChildOf( n ) ) {
                //kDebug(planDbg())<<node->name()<<" is child of"<<n->name();
                ins = false;
                break;
            }
        }
        if ( ins ) {
            //kDebug(planDbg())<<" insert"<<node->name();
            lst << node;
        }
    }
    QList<Node*> nl = lst;
    QList<Node*> nlst = lst;
    foreach ( Node *node, nl ) {
        foreach ( Node *n, nlst ) {
            if ( n->isChildOf( node ) ) {
                //kDebug(planDbg())<<n->name()<<" is child of"<<node->name();
                int i = nodes.indexOf( n );
                lst.removeAt( i );
            }
        }
    }
    return lst;
}

bool NodeItemModel::dropResourceMimeData( const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent )
{
    QByteArray encodedData = data->data( "application/x-vnd.kde.plan.resourceitemmodel.internal" );
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    Node *n = node( parent );
    kDebug(planDbg())<<n<<parent;
    if ( n == 0 ) {
        return true;
    }
    kDebug(planDbg())<<n->name();
    if ( parent.column() == NodeModel::NodeResponsible ) {
        QString s;
        foreach ( Resource *r, resourceList( stream ) ) {
            s += r->name();
        }
        if ( ! s.isEmpty() ) {
            if ( action == Qt::CopyAction && ! n->leader().isEmpty() ) {
                s += ',' + n->leader();
            }
            KUndo2Command *cmd = m_nodemodel.setLeader( n, s, Qt::EditRole );
            if ( cmd ) {
                emit executeCommand( cmd );
            }
            kDebug(planDbg())<<s;
        }
        return true;
    }
    if ( n->type() == Node::Type_Task ) {
        QList<Resource*> lst = resourceList( stream );
        if ( action == Qt::CopyAction ) {
            lst += static_cast<Task*>( n )->requestedResources();
        }
        KUndo2Command *cmd = createAllocationCommand( static_cast<Task&>( *n ), lst );
        if ( cmd ) {
            emit executeCommand( cmd );
        }
        return true;
    }
    return true;
}

bool NodeItemModel::dropProjectMimeData( const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent )
{
    Node *n = node( parent );
    kDebug(planDbg())<<n<<parent;
    if ( n == 0 ) {
        n = m_project;
    }
    kDebug(planDbg())<<n->name()<<action<<row<<parent;

    KoXmlDocument doc;
    doc.setContent( data->data( "application/x-vnd.kde.plan.project" ) );
    KoXmlElement element = doc.documentElement().namedItem( "project" ).toElement();
    Project project;
    XMLLoaderObject status;
    status.setVersion( doc.documentElement().attribute( "version", PLAN_FILE_SYNTAX_VERSION ) );
    status.setProject( &project );
    if ( ! project.load( element, status ) ) {
        kDebug(planDbg())<<"Failed to load project";
        return false;
    }
    project.generateUniqueNodeIds();
    KUndo2Command *cmd = new InsertProjectCmd( project, n, n->childNode( row - 1 ), i18nc( "(qtundo) 1=project or task name", "Insert %1", project.name() ) );
    emit executeCommand( cmd );
    return true;
}

bool NodeItemModel::dropUrlMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
    if ( data->hasUrls() ) {
        QList<QUrl> urls = data->urls();
        kDebug(planDbg())<<urls;
        foreach ( const QUrl &url, urls ) {
            KMimeType::Ptr mime = KMimeType::findByUrl( url );
            kDebug(planDbg())<<url<<mime->name();
            if ( mime->is( "application/x-vnd.kde.plan" ) ) {
                importProjectFile( url, action, row, column, parent );
            }
        }
        return true;
    }
    return false;
}

bool NodeItemModel::importProjectFile( const KUrl &url, Qt::DropAction /*action*/, int row, int /*column*/, const QModelIndex &parent )
{
    if ( ! url.isLocalFile() ) {
        kDebug(planDbg())<<"TODO: download if url not local";
        return false;
    }
    KoStore *store = KoStore::createStore( url.path(), KoStore::Read, "", KoStore::Auto );
    if ( store->bad() ) {
        //        d->lastErrorMessage = i18n( "Not a valid Calligra file: %1", file );
        kDebug(planDbg())<<"bad store"<<url.prettyUrl();
        delete store;
        //        QApplication::restoreOverrideCursor();
        return false;
    }
    if ( ! store->open( "root" ) ) { // maindoc.xml
        kDebug(planDbg())<<"No root"<<url.prettyUrl();
        delete store;
        return false;
    }
    KoXmlDocument doc;
    doc.setContent( store->device() );
    KoXmlElement element = doc.documentElement().namedItem( "project" ).toElement();
    Project project;
    XMLLoaderObject status;
    status.setVersion( doc.documentElement().attribute( "version", PLAN_FILE_SYNTAX_VERSION ) );
    status.setProject( &project );
    if ( ! project.load( element, status ) ) {
        kDebug(planDbg())<<"Failed to load project from:"<<url;
        return false;
    }
    project.generateUniqueNodeIds();
    Node *n = node( parent );
    kDebug(planDbg())<<n<<parent;
    if ( n == 0 ) {
        n = m_project;
    }
    KUndo2Command *cmd = new InsertProjectCmd( project, n, n->childNode( row - 1 ), i18nc( "(qtundo)", "Insert %1", url.fileName() ) );
    emit executeCommand( cmd );
    return true;
}

KUndo2Command *NodeItemModel::createAllocationCommand( Task &task, const QList<Resource*> &lst )
{
    MacroCommand *cmd = new MacroCommand( i18nc( "(qtundo-format)", "Modify resource allocations" ) );
    QMap<ResourceGroup*, ResourceGroupRequest*> groups;
    foreach ( Resource *r, lst ) {
        if ( ! groups.contains( r->parentGroup() ) && task.resourceGroupRequest( r->parentGroup() ) == 0 ) {
            ResourceGroupRequest *gr = new ResourceGroupRequest( r->parentGroup() );
            groups[ r->parentGroup() ] = gr;
            cmd->addCommand( new AddResourceGroupRequestCmd( task, gr ) );
        }
    }
    QList<Resource*> resources = task.requestedResources();
    foreach ( Resource *r, lst ) {
        if ( resources.contains( r ) ) {
            continue;
        }
        ResourceGroupRequest *gr = groups.value( r->parentGroup() );
        if ( gr == 0 ) {
            gr = task.resourceGroupRequest( r->parentGroup() );
        }
        if ( gr == 0 ) {
            kError()<<"No group request found, cannot add resource request:"<<r->name();
            continue;
        }
        cmd->addCommand( new AddResourceRequestCmd( gr, new ResourceRequest( r, 100 ) ) );
    }
    foreach ( Resource *r, resources ) {
        if ( ! lst.contains( r ) ) {
            ResourceGroupRequest *gr = task.resourceGroupRequest( r->parentGroup() );
            ResourceRequest *rr = task.requests().find( r );
            if ( gr && rr ) {
                cmd->addCommand( new RemoveResourceRequestCmd( gr, rr ) );
            }
        }
    }
    if ( cmd->isEmpty() ) {
        delete cmd;
        return 0;
    }
    return cmd;
}

bool NodeItemModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
    kDebug(planDbg())<<action;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if ( data->hasFormat( "application/x-vnd.kde.plan.resourceitemmodel.internal" ) ) {
        return dropResourceMimeData( data, action, row, column, parent );
    }
    if ( data->hasFormat( "application/x-vnd.kde.plan.nodeitemmodel.internal" ) ) {
        if ( action == Qt::MoveAction ) {
            //kDebug(planDbg())<<"MoveAction";

            QByteArray encodedData = data->data( "application/x-vnd.kde.plan.nodeitemmodel.internal" );
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            Node *par = 0;
            if ( parent.isValid() ) {
                par = node( parent );
            } else {
                par = m_project;
            }
            QList<Node*> lst = nodeList( stream );
            QList<Node*> nodes = removeChildNodes( lst ); // children goes with their parent
            foreach ( Node *n, nodes ) {
                if ( ! m_project->canMoveTask( n, par ) ) {
                    //kDebug(planDbg())<<"Can't move task:"<<n->name();
                    return false;
                }
            }
            int offset = 0;
            MacroCommand *cmd = 0;
            foreach ( Node *n, nodes ) {
                if ( cmd == 0 ) cmd = new MacroCommand( i18nc( "(qtundo-format)", "Move tasks" ) );
                // append nodes if dropped *on* another node, insert if dropped *after*
                int pos = row == -1 ? -1 : row + offset;
                if ( pos >= 0 && n->parentNode() == par && par->indexOf( n ) < pos ) {
                    --pos;
                }
                cmd->addCommand( new NodeMoveCmd( m_project, n, par, pos ) );
                offset++;
            }
            if ( cmd ) {
                emit executeCommand( cmd );
            }
            //kDebug(planDbg())<<row<<","<<column<<" parent="<<parent.row()<<","<<parent.column()<<":"<<par->name();
            return true;
        }
    }
    if ( data->hasFormat( "application/x-vnd.kde.plan.project" ) ) {
        kDebug(planDbg());
        return dropProjectMimeData( data, action, row, column, parent );

    }
    if ( data->hasUrls() ) {
        return dropUrlMimeData( data, action, row, column, parent );
    }
    return false;
}

Node *NodeItemModel::node( const QModelIndex &index ) const
{
    Node *n = m_project;
    if ( index.isValid() ) {
        //kDebug(planDbg())<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
        n = static_cast<Node*>( index.internalPointer() );
        Q_ASSERT( n );
    }
    return n;
}

void NodeItemModel::slotNodeChanged( Node *node )
{
    if ( node == 0 || ( ! m_projectshown && node->type() == Node::Type_Project ) ) {
        return;
    }
    if ( node->type() == Node::Type_Project ) {
        emit dataChanged( createIndex( 0, 0, node ), createIndex( 0, columnCount()-1, node ) );
        return;
    }
    int row = node->parentNode()->findChildNode( node );
    Q_ASSERT( row >= 0 );
    emit dataChanged( createIndex( row, 0, node ), createIndex( row, columnCount()-1, node ) );
}

QModelIndex NodeItemModel::insertTask( Node *node, Node *after )
{
    MacroCommand *cmd = new MacroCommand( i18nc( "(qtundo-format)", "Add task" ) );
    cmd->addCommand( new TaskAddCmd( m_project, node, after ) );
    if ( m_project && node->type() == Node::Type_Task ) {
        QMap<ResourceGroup*, ResourceGroupRequest*> groups;
        foreach ( Resource *r, m_project->autoAllocateResources() ) {
            if ( ! groups.contains( r->parentGroup() ) ) {
                ResourceGroupRequest *gr = new ResourceGroupRequest( r->parentGroup() );
                cmd->addCommand( new AddResourceGroupRequestCmd( static_cast<Task&>(*node), gr ) );
                groups[ r->parentGroup() ] = gr;
            }
            ResourceRequest *rr = new ResourceRequest( r, 100 );
            cmd->addCommand( new AddResourceRequestCmd( groups[ r->parentGroup() ], rr ) );
        }
    }
    emit executeCommand( cmd );
    int row = -1;
    if ( node->parentNode() ) {
        row = node->parentNode()->indexOf( node );
    }
    if ( row != -1 ) {
        //kDebug(planDbg())<<"Inserted: "<<node->name()<<"; "<<row;
        return createIndex( row, 0, node );
    }
    //kDebug(planDbg())<<"Can't find "<<node->name();
    return QModelIndex();
}

QModelIndex NodeItemModel::insertSubtask( Node *node, Node *parent )
{
    emit executeCommand( new SubtaskAddCmd( m_project, node, parent, i18nc( "(qtundo-format)", "Add sub-task" ) ) );
    int row = -1;
    if ( node->parentNode() ) {
        row = node->parentNode()->indexOf( node );
    }
    if ( row != -1 ) {
        //kDebug(planDbg())<<node->parentNode()<<" inserted: "<<node->name()<<"; "<<row;
        return createIndex( row, 0, node );
    }
    //kDebug(planDbg())<<"Can't find "<<node->name();
    return QModelIndex();
}

int NodeItemModel::sortRole( int column ) const
{
    switch ( column ) {
        case NodeModel::NodeStartTime:
        case NodeModel::NodeEndTime:
        case NodeModel::NodeActualStart:
        case NodeModel::NodeActualFinish:
        case NodeModel::NodeEarlyStart:
        case NodeModel::NodeEarlyFinish:
        case NodeModel::NodeLateStart:
        case NodeModel::NodeLateFinish:
        case NodeModel::NodeConstraintStart:
        case NodeModel::NodeConstraintEnd:
            return Qt::EditRole;
        default:
            break;
    }
    return Qt::DisplayRole;
}

//------------------------------------------------
GanttItemModel::GanttItemModel( QObject *parent )
    : NodeItemModel( parent ),
    m_showSpecial( false )
{
}

GanttItemModel::~GanttItemModel()
{
    QList<void*> lst = parentmap.values();
    while ( ! lst.isEmpty() )
        delete (int*)(lst.takeFirst());
}

int GanttItemModel::rowCount( const QModelIndex &parent ) const
{
    if ( m_showSpecial ) {
        if ( parentmap.values().contains( parent.internalPointer() ) ) {
            return 0;
        }
        Node *n = node( parent );
        if ( n && n->type() == Node::Type_Task ) {
            return 5; // the task + early start + late finish ++
        }
    }
    return NodeItemModel::rowCount( parent );
}

QModelIndex GanttItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( m_showSpecial && parent.isValid()  ) {
        Node *p = node( parent );
        if ( p->type() == Node::Type_Task ) {
            void *v = 0;
            foreach ( void *i, parentmap.values( p ) ) {
                if ( *( (int*)( i ) ) == row ) {
                    v = i;
                    break;
                }
            }
            if ( v == 0 ) {
                v = new int( row );
                const_cast<GanttItemModel*>( this )->parentmap.insertMulti( p, v );
            }
            return createIndex( row, column, v );
        }
    }
    return NodeItemModel::index( row, column, parent );
}

QModelIndex GanttItemModel::parent( const QModelIndex &idx ) const
{
    if ( m_showSpecial ) {
        QList<Node*> lst = parentmap.keys( idx.internalPointer() );
        if ( ! lst.isEmpty() ) {
            Q_ASSERT( lst.count() == 1 );
            return index( lst.first() );
        }
    }
    return NodeItemModel::parent( idx );
}

QVariant GanttItemModel::data( const QModelIndex &index, int role ) const
{
    if ( ! index.isValid() ) {
        return QVariant();
    }
    if ( role == Qt::TextAlignmentRole ) {
        return headerData( index.column(), Qt::Horizontal, role );
    }
    QModelIndex idx = index;
    QList<Node*> lst;
    if ( m_showSpecial ) {
        lst = parentmap.keys( idx.internalPointer() );
    }
    if ( ! lst.isEmpty() ) {
        Q_ASSERT( lst.count() == 1 );
        int row = *((int*)(idx.internalPointer()));
        Node *n = lst.first();
        if ( role == SpecialItemTypeRole ) {
            return row; // 0=task, 1=early start, 2=late finish...
        }
        switch ( row ) {
            case 0:  // the task
                if ( idx.column() == NodeModel::NodeType && role == KDGantt::ItemTypeRole ) {
                    switch ( n->type() ) {
                        case Node::Type_Task: return KDGantt::TypeTask;
                        default: break;
                    }
                }
                break;
            case 1: { // early start
                if ( role != Qt::DisplayRole && role != Qt::EditRole && role != KDGantt::ItemTypeRole ) {
                    return QVariant();
                }
                switch ( idx.column() ) {
                    case NodeModel::NodeName: return "Early Start";
                    case NodeModel::NodeType: return KDGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->earlyStart( id() );
                    default: break;
                }
            }
            case 2: { // late finish
                if ( role != Qt::DisplayRole && role != Qt::EditRole && role != KDGantt::ItemTypeRole ) {
                    return QVariant();
                }
                switch ( idx.column() ) {
                    case NodeModel::NodeName: return "Late Finish";
                    case NodeModel::NodeType: return KDGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->lateFinish( id() );
                    default: break;
                }
            }
            case 3: { // late start
                if ( role != Qt::DisplayRole && role != Qt::EditRole && role != KDGantt::ItemTypeRole ) {
                    return QVariant();
                }
                switch ( idx.column() ) {
                    case NodeModel::NodeName: return "Late Start";
                    case NodeModel::NodeType: return KDGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->lateStart( id() );
                    default: break;
                }
            }
            case 4: { // early finish
                if ( role != Qt::DisplayRole && role != Qt::EditRole && role != KDGantt::ItemTypeRole ) {
                    return QVariant();
                }
                switch ( idx.column() ) {
                    case NodeModel::NodeName: return "Early Finish";
                    case NodeModel::NodeType: return KDGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->earlyFinish( id() );
                    default: break;
                }
            }
            default: return QVariant();
        }
        idx = createIndex( idx.row(), idx.column(), n );
    } else {
        if ( role == SpecialItemTypeRole ) {
            return 0; // task of some type
        }
        if ( idx.column() == NodeModel::NodeType && role == KDGantt::ItemTypeRole ) {
            QVariant result = NodeItemModel::data( idx, Qt::EditRole );
            switch ( result.toInt() ) {
                case Node::Type_Project: return KDGantt::TypeSummary;
                case Node::Type_Summarytask: return KDGantt::TypeSummary;
                case Node::Type_Milestone: return KDGantt::TypeEvent;
                default: return m_showSpecial ? KDGantt::TypeMulti : KDGantt::TypeTask;
            }
        }
    }
    return NodeItemModel::data( idx, role );
}

//----------------------------
MilestoneItemModel::MilestoneItemModel( QObject *parent )
    : ItemModelBase( parent )
{
}

MilestoneItemModel::~MilestoneItemModel()
{
}

QList<Node*> MilestoneItemModel::mileStones() const
{
    QList<Node*> lst;
    foreach( Node* n, m_nodemap ) {
        if ( n->type() == Node::Type_Milestone ) {
            lst << n;
        }
    }
    return lst;
}

void MilestoneItemModel::slotNodeToBeInserted( Node *parent, int row )
{
    Q_UNUSED(parent);
    Q_UNUSED(row);
}

void MilestoneItemModel::slotNodeInserted( Node *node )
{
    Q_UNUSED(node);
    resetModel();
}

void MilestoneItemModel::slotNodeToBeRemoved( Node *node )
{
    Q_UNUSED(node);
    //kDebug(planDbg())<<node->name();
/*    int row = m_nodemap.values().indexOf( node );
    if ( row != -1 ) {
        Q_ASSERT( m_nodemap.contains( node->wbsCode() ) );
        Q_ASSERT( m_nodemap.keys().indexOf( node->wbsCode() ) == row );
        beginRemoveRows( QModelIndex(), row, row );
        m_nodemap.remove( node->wbsCode() );
        endRemoveRows();
    }*/
}

void MilestoneItemModel::slotNodeRemoved( Node *node )
{
    Q_UNUSED(node);
    resetModel();
    //endRemoveRows();
}

void MilestoneItemModel::slotLayoutChanged()
{
    //kDebug(planDbg())<<node->name();
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

void MilestoneItemModel::slotNodeToBeMoved( Node *node, int pos, Node *newParent, int newPos )
{
    Q_UNUSED( node );
    Q_UNUSED( pos );
    Q_UNUSED( newParent );
    Q_UNUSED( newPos );
}

void MilestoneItemModel::slotNodeMoved( Node *node )
{
    Q_UNUSED( node );
    resetModel();
}

void MilestoneItemModel::setProject( Project *project )
{
    if ( m_project ) {
        disconnect( m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()) );
        disconnect( m_project, SIGNAL(wbsDefinitionChanged()), this, SLOT(slotWbsDefinitionChanged()) );
        disconnect( m_project, SIGNAL(nodeChanged(Node*)), this, SLOT(slotNodeChanged(Node*)) );
        disconnect( m_project, SIGNAL(nodeToBeAdded(Node*,int)), this, SLOT(slotNodeToBeInserted(Node*,int)) );
        disconnect( m_project, SIGNAL(nodeToBeRemoved(Node*)), this, SLOT(slotNodeToBeRemoved(Node*)) );

        disconnect(m_project, SIGNAL(nodeToBeMoved(Node*,int,Node*,int)), this, SLOT(slotNodeToBeMoved(Node*,int,Node*,int)));
        disconnect(m_project, SIGNAL(nodeMoved(Node*)), this, SLOT(slotNodeMoved(Node*)));

        disconnect( m_project, SIGNAL(nodeAdded(Node*)), this, SLOT(slotNodeInserted(Node*)) );
        disconnect( m_project, SIGNAL(nodeRemoved(Node*)), this, SLOT(slotNodeRemoved(Node*)) );
    }
    m_project = project;
    //kDebug(planDbg())<<m_project<<"->"<<project;
    m_nodemodel.setProject( project );
    if ( project ) {
        connect( m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()) );
        connect( m_project, SIGNAL(wbsDefinitionChanged()), this, SLOT(slotWbsDefinitionChanged()) );
        connect( m_project, SIGNAL(nodeChanged(Node*)), this, SLOT(slotNodeChanged(Node*)) );
        connect( m_project, SIGNAL(nodeToBeAdded(Node*,int)), this, SLOT(slotNodeToBeInserted(Node*,int)) );
        connect( m_project, SIGNAL(nodeToBeRemoved(Node*)), this, SLOT(slotNodeToBeRemoved(Node*)) );

        connect(m_project, SIGNAL(nodeToBeMoved(Node*,int,Node*,int)), this, SLOT(slotNodeToBeMoved(Node*,int,Node*,int)));
        connect(m_project, SIGNAL(nodeMoved(Node*)), this, SLOT(slotNodeMoved(Node*)));

        connect( m_project, SIGNAL(nodeAdded(Node*)), this, SLOT(slotNodeInserted(Node*)) );
        connect( m_project, SIGNAL(nodeRemoved(Node*)), this, SLOT(slotNodeRemoved(Node*)) );
    }
    resetModel();
}

void MilestoneItemModel::setScheduleManager( ScheduleManager *sm )
{
    if ( m_nodemodel.manager() ) {
    }
    m_nodemodel.setManager( sm );
    ItemModelBase::setScheduleManager( sm );
    if ( sm ) {
    }
    //kDebug(planDbg())<<sm;
    resetModel();
}

bool MilestoneItemModel::resetData()
{
    int cnt = m_nodemap.count();
    m_nodemap.clear();
    if ( m_project != 0 ) {
        foreach ( Node *n, m_project->allNodes() ) {
            m_nodemap.insert( n->wbsCode(), n );
        }
    }
    return cnt != m_nodemap.count();
}

void MilestoneItemModel::resetModel()
{
    resetData();
    reset();
}

Qt::ItemFlags MilestoneItemModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags( index );
    if ( !index.isValid() ) {
        if ( m_readWrite ) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    }
    if ( m_readWrite ) {
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        switch ( index.column() ) {
            case NodeModel::NodeName: // name
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeType: break; // Node type
            case NodeModel::NodeResponsible: // Responsible
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeConstraint: // constraint type
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeConstraintStart: { // constraint start
                Node *n = node( index );
                if ( n == 0 )
                    break;
                int c = n->constraint();
                if ( c == Node::MustStartOn || c == Node::StartNotEarlier || c == Node::FixedInterval ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeConstraintEnd: { // constraint end
                Node *n = node( index );
                if ( n == 0 )
                    break;
                int c = n->constraint();
                if ( c == Node::MustFinishOn || c == Node::FinishNotLater || c ==  Node::FixedInterval ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeStartupAccount: // startup account
            case NodeModel::NodeStartupCost: // startup cost
            case NodeModel::NodeShutdownAccount: // shutdown account
            case NodeModel::NodeShutdownCost: { // shutdown cost
                Node *n = node( index );
                if ( n && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) ) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeDescription: // description
                break;
            default:
                flags &= ~Qt::ItemIsEditable;
        }
    }
    return flags;
}

QModelIndex MilestoneItemModel::parent( const QModelIndex &index ) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex MilestoneItemModel::index( int row, int column, const QModelIndex &parent ) const
{
    //kDebug(planDbg())<<parent<<row<<", "<<m_nodemap.count();
    if ( m_project == 0 || row < 0 || column < 0 ) {
        //kDebug(planDbg())<<"No project"<<m_project<<" or illegal row, column"<<row<<column;
        return QModelIndex();
    }
    if ( parent.isValid() || row >= m_nodemap.count() ) {
        //kDebug(planDbg())<<"No index for"<<parent<<row<<","<<column;
        return QModelIndex();
    }
    return createIndex( row, column, m_nodemap.values().at( row ) );
}

QModelIndex MilestoneItemModel::index( const Node *node ) const
{
    if ( m_project == 0 || node == 0 ) {
        return QModelIndex();
    }
    return createIndex( m_nodemap.values().indexOf( const_cast<Node*>( node ) ), 0, const_cast<Node*>(node) );
}


QVariant MilestoneItemModel::data( const QModelIndex &index, int role ) const
{
    QVariant result;
    if ( role == Qt::TextAlignmentRole ) {
        return headerData( index.column(), Qt::Horizontal, role );
    }
    Node *n = node( index );
    if ( n != 0 ) {
        if ( index.column() == NodeModel::NodeType && role == KDGantt::ItemTypeRole ) {
            result = m_nodemodel.data( n, index.column(), Qt::EditRole );
            switch ( result.toInt() ) {
                case Node::Type_Summarytask: return KDGantt::TypeSummary;
                case Node::Type_Milestone: return KDGantt::TypeEvent;
                default: return KDGantt::TypeTask;
            }
            return result;
        }
    }
    result = m_nodemodel.data( n, index.column(), role );
    return result;
}

bool MilestoneItemModel::setData( const QModelIndex &index, const QVariant &/*value*/, int role )
{
    if ( ( flags(index) &Qt::ItemIsEditable ) == 0 || role != Qt::EditRole ) {
        return false;
    }
//     Node *n = node( index );
    switch (index.column()) {
        default:
            qWarning("data: invalid display value column %d", index.column());
            return false;
    }
    return false;
}

QVariant MilestoneItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        if ( role == Qt::DisplayRole || role == Qt::TextAlignmentRole) {
            return m_nodemodel.headerData( section, role );
        }
    }
    if ( role == Qt::ToolTipRole ) {
        return NodeModel::headerData( section, role );
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *MilestoneItemModel::createDelegate( int column, QWidget *parent ) const
{
    switch ( column ) {
        case NodeModel::NodeEstimateType: return new EnumDelegate( parent );
        case NodeModel::NodeEstimateCalendar: return new EnumDelegate( parent );
        case NodeModel::NodeEstimate: return new DurationSpinBoxDelegate( parent );
        case NodeModel::NodeOptimisticRatio: return new SpinBoxDelegate( parent );
        case NodeModel::NodePessimisticRatio: return new SpinBoxDelegate( parent );
        case NodeModel::NodeRisk: return new EnumDelegate( parent );
        case NodeModel::NodeConstraint: return new EnumDelegate( parent );
        case NodeModel::NodeRunningAccount: return new EnumDelegate( parent );
        case NodeModel::NodeStartupAccount: return new EnumDelegate( parent );
        case NodeModel::NodeStartupCost: return new MoneyDelegate( parent );
        case NodeModel::NodeShutdownAccount: return new EnumDelegate( parent );
        case NodeModel::NodeShutdownCost: return new MoneyDelegate( parent );

        case NodeModel::NodeCompleted: return new TaskCompleteDelegate( parent );
        case NodeModel::NodeRemainingEffort: return new DurationSpinBoxDelegate( parent );
        case NodeModel::NodeActualEffort: return new DurationSpinBoxDelegate( parent );

        default: return 0;
    }
    return 0;
}

int MilestoneItemModel::columnCount( const QModelIndex &/*parent*/ ) const
{
    return m_nodemodel.propertyCount();
}

int MilestoneItemModel::rowCount( const QModelIndex &parent ) const
{
    //kDebug(planDbg())<<parent;
    if ( parent.isValid() ) {
        return 0;
    }
    //kDebug(planDbg())<<m_nodemap.count();
    return m_nodemap.count();
}

Qt::DropActions MilestoneItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList MilestoneItemModel::mimeTypes() const
{
    return QStringList();
}

QMimeData *MilestoneItemModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *m = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> rows;
    foreach (const QModelIndex &index, indexes) {
        if ( index.isValid() && !rows.contains( index.row() ) ) {
            //kDebug(planDbg())<<index.row();
            Node *n = node( index );
            if ( n ) {
                rows << index.row();
                stream << n->id();
            }
        }
    }
    m->setData("application/x-vnd.kde.plan.nodeitemmodel.internal", encodedData);
    return m;
}

bool MilestoneItemModel::dropAllowed( const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data )
{
    //kDebug(planDbg());
    Node *dn = node( index );
    if ( dn == 0 ) {
        kError()<<"no node to drop on!";
        return false; // hmmm
    }
    switch ( dropIndicatorPosition ) {
        case ItemModelBase::AboveItem:
        case ItemModelBase::BelowItem:
            // dn == sibling
            return dropAllowed( dn->parentNode(), data );
        case ItemModelBase::OnItem:
            // dn == new parent
            return dropAllowed( dn, data );
        default:
            break;
    }
    return false;
}

bool MilestoneItemModel::dropAllowed( Node *on, const QMimeData *data )
{
    if ( !data->hasFormat("application/x-vnd.kde.plan.nodeitemmodel.internal") ) {
        return false;
    }
    if ( on == m_project ) {
        return true;
    }
    QByteArray encodedData = data->data( "application/x-vnd.kde.plan.nodeitemmodel.internal" );
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QList<Node*> lst = nodeList( stream );
    foreach ( Node *n, lst ) {
        if ( on == n || on->isChildOf( n ) ) {
            return false;
        }
    }
    lst = removeChildNodes( lst );
    foreach ( Node *n, lst ) {
        if ( ! m_project->canMoveTask( n, on ) ) {
            return false;
        }
    }
    return true;
}

QList<Node*> MilestoneItemModel::nodeList( QDataStream &stream )
{
    QList<Node*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Node *node = m_project->findNode( id );
        if ( node ) {
            lst << node;
        }
    }
    return lst;
}

QList<Node*> MilestoneItemModel::removeChildNodes( const QList<Node*> &nodes )
{
    QList<Node*> lst;
    foreach ( Node *node, nodes ) {
        bool ins = true;
        foreach ( Node *n, lst ) {
            if ( node->isChildOf( n ) ) {
                //kDebug(planDbg())<<node->name()<<" is child of"<<n->name();
                ins = false;
                break;
            }
        }
        if ( ins ) {
            //kDebug(planDbg())<<" insert"<<node->name();
            lst << node;
        }
    }
    QList<Node*> nl = lst;
    QList<Node*> nlst = lst;
    foreach ( Node *node, nl ) {
        foreach ( Node *n, nlst ) {
            if ( n->isChildOf( node ) ) {
                //kDebug(planDbg())<<n->name()<<" is child of"<<node->name();
                int i = nodes.indexOf( n );
                lst.removeAt( i );
            }
        }
    }
    return lst;
}

bool MilestoneItemModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent )
{
    //kDebug(planDbg())<<action;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if ( !data->hasFormat( "application/x-vnd.kde.plan.nodeitemmodel.internal" ) ) {
        return false;
    }
    if ( action == Qt::MoveAction ) {
        //kDebug(planDbg())<<"MoveAction";

        QByteArray encodedData = data->data( "application/x-vnd.kde.plan.nodeitemmodel.internal" );
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        Node *par = 0;
        if ( parent.isValid() ) {
            par = node( parent );
        } else {
            par = m_project;
        }
        QList<Node*> lst = nodeList( stream );
        QList<Node*> nodes = removeChildNodes( lst ); // children goes with their parent
        foreach ( Node *n, nodes ) {
            if ( ! m_project->canMoveTask( n, par ) ) {
                //kDebug(planDbg())<<"Can't move task:"<<n->name();
                return false;
            }
        }
        int offset = 0;
        MacroCommand *cmd = 0;
        foreach ( Node *n, nodes ) {
            if ( cmd == 0 ) cmd = new MacroCommand( i18nc( "(qtundo-format)", "Move tasks" ) );
            // append nodes if dropped *on* another node, insert if dropped *after*
            int pos = row == -1 ? -1 : row + offset;
            cmd->addCommand( new NodeMoveCmd( m_project, n, par, pos ) );
            offset++;
        }
        if ( cmd ) {
            emit executeCommand( cmd );
        }
        //kDebug(planDbg())<<row<<","<<column<<" parent="<<parent.row()<<","<<parent.column()<<":"<<par->name();
        return true;
    }
    return false;
}

Node *MilestoneItemModel::node( const QModelIndex &index ) const
{
    Node *n = 0;
    if ( index.isValid() ) {
        //kDebug(planDbg())<<index;
        n = static_cast<Node*>( index.internalPointer() );
    }
    return n;
}

void MilestoneItemModel::slotNodeChanged( Node *node )
{
    //kDebug(planDbg())<<node->name();
    if ( node == 0 ) {
        return;
    }
//    if ( ! m_nodemap.contains( node->wbsCode() ) || m_nodemap.value( node->wbsCode() ) != node ) {
        emit layoutAboutToBeChanged();
        if ( resetData() ) {
            reset();
        } else {
            emit layoutChanged();
        }
        return;
/*    }
    int row = m_nodemap.values().indexOf( node );
    kDebug(planDbg())<<node->name()<<": "<<node->typeToString()<<row;
    emit dataChanged( createIndex( row, 0, node ), createIndex( row, columnCount()-1, node ) );*/
}

void MilestoneItemModel::slotWbsDefinitionChanged()
{
    //kDebug(planDbg());
    if ( m_project == 0 ) {
        return;
    }
    if ( ! m_nodemap.isEmpty() ) {
        emit layoutAboutToBeChanged();
        resetData();
        emit layoutChanged();
    }
}

//--------------
NodeSortFilterProxyModel::NodeSortFilterProxyModel( ItemModelBase* model, QObject *parent, bool filterUnscheduled )
    : QSortFilterProxyModel( parent ),
    m_filterUnscheduled( filterUnscheduled )
{
    setSourceModel( model );
    setDynamicSortFilter( true );
}

ItemModelBase *NodeSortFilterProxyModel::itemModel() const
{
    return static_cast<ItemModelBase *>( sourceModel() );
}

void NodeSortFilterProxyModel::setFilterUnscheduled( bool on ) {
    m_filterUnscheduled = on;
    invalidateFilter();
}

bool NodeSortFilterProxyModel::filterAcceptsRow ( int row, const QModelIndex & parent ) const
{
    //kDebug(planDbg())<<sourceModel()<<row<<parent;
    if ( itemModel()->project() == 0 ) {
        //kDebug(planDbg())<<itemModel()->project();
        return false;
    }
    if ( m_filterUnscheduled ) {
        QString s = sourceModel()->data( sourceModel()->index( row, NodeModel::NodeNotScheduled, parent ), Qt::EditRole ).toString();
        if ( s == "true" ) {
            //kDebug(planDbg())<<"Filtered unscheduled:"<<sourceModel()->index( row, 0, parent );
            return false;
        }
    }
    bool accepted = QSortFilterProxyModel::filterAcceptsRow( row, parent );
    //kDebug(planDbg())<<this<<sourceModel()->index( row, 0, parent )<<"accepted ="<<accepted<<filterRegExp()<<filterRegExp().isEmpty()<<filterRegExp().capturedTexts();
    return accepted;
}

//------------------
TaskModuleModel::TaskModuleModel( QObject *parent )
    : QAbstractItemModel( parent )
{
}

void TaskModuleModel::addTaskModule( Project *project )
{
    beginInsertRows( QModelIndex(), m_modules.count(), m_modules.count() );
    m_modules << project;
    endInsertRows();
}

Qt::ItemFlags TaskModuleModel::flags( const QModelIndex &idx ) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags( idx ) | Qt::ItemIsDropEnabled;
    if ( idx.isValid() ) {
        f |=  Qt::ItemIsDragEnabled;
    }
    return f;
}

int TaskModuleModel::columnCount (const QModelIndex &/*idx*/ ) const
{
    return 1;
}

int TaskModuleModel::rowCount( const QModelIndex &idx ) const
{
    return idx.isValid() ? 0 : m_modules.count();
}

QVariant TaskModuleModel::data( const QModelIndex& idx, int role ) const
{
    switch ( role ) {
        case Qt::DisplayRole: return m_modules.value( idx.row() )->name();
        case Qt::ToolTipRole: return m_modules.value( idx.row() )->description();
        case Qt::WhatsThisRole: return m_modules.value( idx.row() )->description();
        default: break;
    }
    return QVariant();
}

QVariant TaskModuleModel::headerData( int /*section*/, Qt::Orientation orientation , int role ) const
{
    if ( orientation == Qt::Horizontal ) {
        switch ( role ) {
            case Qt::DisplayRole: return i18nc( "@title:column", "Name" );
            default: break;
        }
    }
    return QVariant();
}

QModelIndex TaskModuleModel::parent( const QModelIndex& /*idx*/ ) const
{
    return QModelIndex();
}

QModelIndex TaskModuleModel::index( int row, int column, const QModelIndex &parent ) const
{
    if ( parent.isValid() ) {
        return QModelIndex();
    }
    return createIndex( row, column, m_modules.value( row ) );
}

QStringList TaskModuleModel::mimeTypes() const
{
    return QStringList() << "application/x-vnd.kde.plan" << "text/uri-list";
}

bool TaskModuleModel::dropMimeData( const QMimeData *data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex &/*parent*/ )
{
    if ( data->hasUrls() ) {
        QList<QUrl> urls = data->urls();
        kDebug(planDbg())<<urls;
        foreach ( const QUrl &url, urls ) {
            KMimeType::Ptr mime = KMimeType::findByUrl( url );
            kDebug(planDbg())<<url<<mime->name();
            if ( mime->is( "application/x-vnd.kde.plan" ) || mime->is( "application/xml" ) ) {
                importProject( url );
            }
        }
        return true;
    }
    return false;
}

bool TaskModuleModel::importProject( const KUrl &url, bool emitsignal )
{
    if ( ! url.isLocalFile() ) {
        kDebug(planDbg())<<"TODO: download if url not local";
        return false;
    }
    KoStore *store = KoStore::createStore( url.path(), KoStore::Read, "", KoStore::Auto );
    if ( store->bad() ) {
        //        d->lastErrorMessage = i18n( "Not a valid Calligra file: %1", file );
        kDebug(planDbg())<<"bad store"<<url.prettyUrl();
        delete store;
        //        QApplication::restoreOverrideCursor();
        return false;
    }
    if ( ! store->open( "root" ) ) { // maindoc.xml
        kDebug(planDbg())<<"No root"<<url.prettyUrl();
        delete store;
        return false;
    }
    KoXmlDocument doc;
    doc.setContent( store->device() );
    KoXmlElement element = doc.documentElement().namedItem( "project" ).toElement();
    Project *project = new Project();
    XMLLoaderObject status;
    status.setVersion( doc.documentElement().attribute( "version", PLAN_FILE_SYNTAX_VERSION ) );
    status.setProject( project );
    if ( project->load( element, status ) ) {
        stripProject( project );
        addTaskModule( project );
        if ( emitsignal ) {
            emit saveTaskModule( url, project );
        }
    } else {
        kDebug(planDbg())<<"Failed to load project from:"<<url;
        delete project;
        return false;
    }
    return true;
}

QMimeData* TaskModuleModel::mimeData( const QModelIndexList &lst ) const
{
    QMimeData *mime = new QMimeData();
    if ( lst.count() == 1 ) {
        QModelIndex idx = lst.at( 0 );
        if ( idx.isValid() ) {
            Project *project = m_modules.value( idx.row() );
            QDomDocument document( "plan" );
            document.appendChild( document.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
            QDomElement doc = document.createElement( "plan" );
            doc.setAttribute( "editor", "Plan" );
            doc.setAttribute( "mime", "application/x-vnd.kde.plan" );
            doc.setAttribute( "version", PLAN_FILE_SYNTAX_VERSION );
            document.appendChild( doc );
            project->save( doc );
            mime->setData( "application/x-vnd.kde.plan.project", document.toByteArray() );
        }
    }
    return mime;
}

void TaskModuleModel::stripProject( Project *project ) const
{
    foreach ( ScheduleManager *sm, project->scheduleManagers() ) {
        DeleteScheduleManagerCmd c( *project, sm );
    }
}

void TaskModuleModel::loadTaskModules( const QStringList &files )
{
    kDebug(planDbg())<<files;
    foreach ( const QString &file, files ) {
        importProject( KUrl( file ), false );
    }
}


} //namespace KPlato

#include "kptnodeitemmodel.moc"
