/* This file is part of the KDE project
  Copyright (C) 2007 Florian Piquemal <flotueur@yahoo.fr>
  Copyright (C) 2007 Alexis Ménard <darktears31@gmail.com>
  Copyright (C) 2007 Dag Andersen <kplato@kde.org>

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

#include "kptperteditor.h"
#include "kptproject.h"
#include "kptrelationeditor.h"

#include <KoDocument.h>

#include <kglobal.h>
#include <klocale.h>

namespace KPlato
{

//-----------------------------------
PertEditor::PertEditor( KoDocument *part, QWidget *parent ) 
    : ViewBase( part, parent ),
    m_project( 0 )
{
    kDebug() <<" ---------------- KPlato: Creating PertEditor ----------------";
    widget.setupUi(this);

    //FIXME:
    QTreeWidgetItem item;
    m_enabledFont = item.font( 0 );
    m_enabledBrush = item.foreground( 0 );
    m_disabledFont = m_enabledFont;
    m_disabledFont.setItalic( true );
    m_disabledBrush = m_enabledBrush;
    m_disabledBrush.setColor( Qt::lightGray );
    
    m_tasktree = widget.taskList;
    m_tasktree->setSelectionMode( QAbstractItemView::SingleSelection );
    
    m_availableList = widget.available;
    m_availableList->setSelectionMode( QAbstractItemView::SingleSelection );
    
    m_requiredList = widget.required;
    m_requiredList->hideColumn( 1 ); // child node name
    m_requiredList->setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );
    connect( m_requiredList->model(), SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );
    updateReadWrite( part->isReadWrite() );
    
    widget.addBtn->setIcon( KIcon( "arrow-right-double" ) );
    widget.removeBtn->setIcon( KIcon( "arrow-left-double" ) );
    
    connect( m_tasktree, SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ), SLOT( slotCurrentTaskChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ) );
    connect( m_availableList, SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ), SLOT( slotAvailableChanged( QTreeWidgetItem * ) ) );
    
    connect( widget.addBtn, SIGNAL(clicked()), this, SLOT(slotAddClicked() ) );
    connect( widget.removeBtn, SIGNAL(clicked() ), this, SLOT(slotRemoveClicked() ) );

    connect( this, SIGNAL( executeCommand( QUndoCommand* ) ), part, SLOT( addCommand( QUndoCommand* ) ) );
}

void PertEditor::slotCurrentTaskChanged( QTreeWidgetItem *curr, QTreeWidgetItem *prev )
{
    //kDebug()<<curr<<prev;
    if ( curr == 0 ) {
        m_availableList->clear();
        loadRequiredTasksList( 0 );
    } else if ( prev == 0 ) {
        dispAvailableTasks();
    } else {
        updateAvailableTasks();
        loadRequiredTasksList( itemToNode( curr ) );
    }
}

void PertEditor::slotAvailableChanged( QTreeWidgetItem *item )
{
    //kDebug()<<item;
    widget.addBtn->setEnabled( item != 0 );
}

void PertEditor::slotAddClicked()
{
    QTreeWidgetItem *item = m_availableList->currentItem();
    //kDebug()<<item;
    addTaskInRequiredList( item );
}

void PertEditor::addTaskInRequiredList(QTreeWidgetItem * currentItem)
{
    //kDebug()<<currentItem;
    if ( currentItem == 0 ) {
        return;
    }
    if ( m_project == 0 ) {
        return;
    }
    // add the relation between the selected task and the current task
    QTreeWidgetItem *selectedTask = m_tasktree->selectedItems().first();
    if ( selectedTask == 0 ) {
        return;
    }

    Node *par = itemToNode( currentItem );
    Node *child = itemToNode( selectedTask );
    if ( par == 0 || child == 0 || ! m_project->legalToLink( par, child ) ) {
        return;
    }
    Relation *rel = new Relation ( par, child );
    AddRelationCmd *addCmd = new AddRelationCmd( *m_project,rel,currentItem->text( 0 ) );
    emit executeCommand( addCmd );

}

void PertEditor::slotRemoveClicked()
{
    Node *n = 0;
    Relation *r = m_requiredList->currentRelation();
    if ( r ) {
        n = r->parent();
    }
    removeTaskFromRequiredList();
    setAvailableItemEnabled( n );
}

void PertEditor::removeTaskFromRequiredList()
{
    //kDebug();
    Relation *r = m_requiredList->currentRelation();
    if ( r == 0 ) {
        return;
    }
    // remove the relation
    emit executeCommand( new DeleteRelationCmd( *m_project, r, i18n( "Remove task dependency" ) ) );
}

void PertEditor::setProject( Project *project )
{
    if ( m_project ) {
        disconnect( m_project, SIGNAL( nodeAdded( Node* ) ), this, SLOT( slotNodeAdded( Node* ) ) );
        disconnect( m_project, SIGNAL( nodeToBeRemoved( Node* ) ), this, SLOT( slotNodeRemoved( Node* ) ) );
        disconnect( m_project, SIGNAL( nodeChanged( Node* ) ), this, SLOT( slotNodeChanged( Node* ) ) );
    }
    m_project = project;
    if ( m_project ) {
        connect( m_project, SIGNAL( nodeAdded( Node* ) ), this, SLOT( slotNodeAdded( Node* ) ) );
        connect( m_project, SIGNAL( nodeToBeRemoved( Node* ) ), this, SLOT( slotNodeRemoved( Node* ) ) );
        connect( m_project, SIGNAL( nodeChanged( Node* ) ), this, SLOT( slotNodeChanged( Node* ) ) );
    }
    m_requiredList->setProject( project );
    draw();
}

void PertEditor::slotNodeAdded( Node *node )
{
    //kDebug();
    Node *parent = node->parentNode();
    int index = parent->indexOf( node );
    QTreeWidgetItem *pitem = findNodeItem( parent, m_tasktree->invisibleRootItem() );
    if ( pitem == 0 ) {
        pitem = m_tasktree->invisibleRootItem();
    }
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText( 0, node->name() );
    item->setData( 0, Qt::UserRole + 1, node->id() );
    pitem->insertChild( index, item );
    
    pitem = findNodeItem( parent, m_availableList->invisibleRootItem() );
    if ( pitem == 0 ) {
        pitem = m_availableList->invisibleRootItem();
    }
    item = new QTreeWidgetItem();
    item->setText( 0, node->name() );
    item->setData( 0, Qt::UserRole + 1, node->id() );
    pitem->insertChild( index, item );
    setAvailableItemEnabled( item );
}

void PertEditor::slotNodeRemoved( Node *node )
{
    //kDebug();
    QTreeWidgetItem *item = findNodeItem( node, m_tasktree->invisibleRootItem() );
    if ( item ) {
        QTreeWidgetItem *parent = item->parent();
        if ( parent == 0 ) {
            parent = m_tasktree->invisibleRootItem();
        }
        Q_ASSERT( parent );
        parent->removeChild( item );
        delete item;
    }
    item = findNodeItem( node, m_availableList->invisibleRootItem() );
    if ( item ) {
        QTreeWidgetItem *parent = item->parent();
        if ( parent == 0 ) {
            parent = m_availableList->invisibleRootItem();
        }
        Q_ASSERT( parent );
        parent->removeChild( item );
        delete item;
    }
}

void PertEditor::slotNodeChanged( Node *node )
{
    QTreeWidgetItem *item = findNodeItem( node, m_tasktree->invisibleRootItem() );
    if ( item ) {
        item->setText( 0, node->name() );
    }
    item = findNodeItem( node, m_availableList->invisibleRootItem() );
    if ( item ) {
        item->setText( 0, node->name() );
    }
}

void PertEditor::draw( Project &project)
{
    setProject( &project );
    draw();
}

void PertEditor::draw()
{
    m_tasktree->clear();
    if ( m_project == 0 ) {
        return;
    }
    drawSubTasksName( m_tasktree->invisibleRootItem(), m_project );
}

void PertEditor::drawSubTasksName( QTreeWidgetItem *parent, Node * currentNode)
{
    foreach(Node * currentChild, currentNode->childNodeIterator()){
        QTreeWidgetItem * item = new QTreeWidgetItem( parent );
        item->setText( 0, currentChild->name());
        item->setData( 0, Qt::UserRole + 1, currentChild->id() );
        drawSubTasksName( item, currentChild);
        //kDebug() << SUBTASK FOUND";
    }
}

void PertEditor::updateReadWrite( bool rw )
{
    m_requiredList->setReadWrite( rw );
}

QTreeWidgetItem *PertEditor::findNodeItem( Node *node, QTreeWidgetItem *item ) {
    if ( node->id() == item->data( 0, Qt::UserRole + 1 ).toString() ) {
        return item;
    }
    for ( int i = 0; i < item->childCount(); ++i ) {
        QTreeWidgetItem *itm = findNodeItem( node, item->child( i ) );
        if ( itm != 0 ) {
            return itm;
        }
    }
    return 0;
}


void PertEditor::dispAvailableTasks( Relation *rel ){
    dispAvailableTasks();
}

void PertEditor::dispAvailableTasks( Node *parent, Node *selectedTask )
{
    QTreeWidgetItem *pitem = findNodeItem( parent, m_availableList->invisibleRootItem() );
    if ( pitem == 0 ) {
        pitem = m_availableList->invisibleRootItem();
    }
    foreach(Node * currentNode, parent->childNodeIterator() )
    {
        //kDebug()<<currentNode->name()<<"level="<<currentNode->level();
        QTreeWidgetItem *item = new QTreeWidgetItem( QStringList()<<currentNode->name() );
        item->setData( 0, Qt::UserRole + 1, currentNode->id() );
        pitem->addChild(item);
        // Checks it isn't the same as the selected task in the m_tasktree
        setAvailableItemEnabled( item );
        dispAvailableTasks( currentNode, selectedTask );
    }
}

void PertEditor::dispAvailableTasks()
{
    m_availableList->clear();

    if ( m_project == 0 ) {
        return;
    }
    Node *selectedTask = itemToNode( m_tasktree->currentItem() );
    
    loadRequiredTasksList(selectedTask);
    
    dispAvailableTasks( m_project, selectedTask );
}

void PertEditor::updateAvailableTasks( QTreeWidgetItem *item )
{
    //kDebug()<<m_project<<item;
    if ( m_project == 0 ) {
        return;
    }
    if ( item == 0 ) {
        item = m_availableList->invisibleRootItem();
    } else {
        setAvailableItemEnabled( item );
    }
    for ( int i=0; i < item->childCount(); ++i ) {
        updateAvailableTasks( item->child( i ) );
    }
}

void PertEditor::setAvailableItemEnabled( QTreeWidgetItem *item )
{
    //kDebug()<<item;
    Node *node = itemToNode( item );
    Q_ASSERT( node != 0 );
    Node *selected = itemToNode( m_tasktree->currentItem() );
    if ( selected == 0 || ! m_project->legalToLink( node, selected ) ) {
        //kDebug()<<"Disable:"<<node->name();
        item->setFont( 0, m_disabledFont) ;
        item->setForeground( 0, m_disabledBrush );
        item->setFlags( item->flags() & ~Qt::ItemIsSelectable );
    } else {
        //kDebug()<<"Enable:"<<node->name();
        item->setFont( 0, m_enabledFont) ;
        item->setForeground( 0, m_enabledBrush );
        item->setFlags( item->flags() | Qt::ItemIsSelectable );
    }
}

void PertEditor::setAvailableItemEnabled( Node *node )
{
    //kDebug()<<node->name();
    setAvailableItemEnabled( nodeToItem( node, m_availableList->invisibleRootItem() ) );
}

QTreeWidgetItem *PertEditor::nodeToItem( Node *node, QTreeWidgetItem *item )
{
    if ( itemToNode( item ) == node ) {
        return item;
    }
    for ( int i=0; i < item->childCount(); ++i ) {
        QTreeWidgetItem *itm = nodeToItem( node, item->child( i ) );
        if ( itm ) {
            return itm;
        }
    }
    return 0;
}

Node * PertEditor::itemToNode( QTreeWidgetItem *item )
{
    if ( m_project == 0 || item == 0 ) {
        return 0;
    }
    return m_project->findNode( item->data( 0, Qt::UserRole + 1 ).toString() );
}

void PertEditor::loadRequiredTasksList(Node * taskNode){
    m_requiredList->setNode( taskNode );
}

void PertEditor::slotUpdate()
{
 draw();
}

} // namespace KPlato

#include "kptperteditor.moc"
