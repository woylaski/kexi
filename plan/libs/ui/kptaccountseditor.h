/* This file is KoDocument of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net>

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

#ifndef KPTACCOUNTSEDITOR_H
#define KPTACCOUNTSEDITOR_H

#include "kplatoui_export.h"

#include <kptviewbase.h>
#include "kptaccountsmodel.h"

#include <kpagedialog.h>

class KoDocument;

class QPoint;


namespace KPlato
{

class Project;
class Account;
class AccountTreeView;

class AccountseditorConfigDialog : public KPageDialog {
    Q_OBJECT
public:
    AccountseditorConfigDialog( ViewBase *view, AccountTreeView *treeview, QWidget *parent );

public slots:
    void slotOk();

private:
    ViewBase *m_view;
    AccountTreeView *m_treeview;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
};

class KPLATOUI_EXPORT AccountTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit AccountTreeView(QWidget *parent);

    AccountItemModel *model() const { return static_cast<AccountItemModel*>( TreeViewBase::model() ); }

    Project *project() const { return model()->project(); }
    void setProject( Project *project ) { model()->setProject( project ); }

    Account *currentAccount() const;
    Account *selectedAccount() const;
    QList<Account*> selectedAccounts() const;
    
signals:
    void currentChanged( const QModelIndex& );
    void currentColumnChanged( const QModelIndex&, const QModelIndex& );
    void selectionChanged( const QModelIndexList& );

    void contextMenuRequested( const QModelIndex&, const QPoint& );
    
protected slots:
    void headerContextMenuRequested( const QPoint &pos );
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    virtual void currentChanged ( const QModelIndex & current, const QModelIndex & previous );

protected:
    void contextMenuEvent ( QContextMenuEvent * event );
    
};

class KPLATOUI_EXPORT AccountsEditor : public ViewBase
{
    Q_OBJECT
public:
    AccountsEditor(KoPart *part, KoDocument *document, QWidget *parent);
    
    void setupGui();
    Project *project() const { return m_view->project(); }
    virtual void draw( Project &project );
    virtual void draw();

    AccountItemModel *model() const { return m_view->model(); }
    
    virtual void updateReadWrite( bool readwrite );

    virtual Account *currentAccount() const;
    
    KoPrintJob *createPrintJob();
    
signals:
    void addAccount( Account *account );
    void deleteAccounts( const QList<Account*>& );
    
public slots:
    /// Activate/deactivate the gui
    virtual void setGuiActive( bool activate );

protected:
    void updateActionsEnabled( bool on );
    void insertAccount( Account *account, Account *parent, int row );

protected slots:
    virtual void slotOptions();
    
private slots:
    void slotContextMenuRequested( const QModelIndex &index, const QPoint& pos );
    void slotHeaderContextMenuRequested( const QPoint &pos );

    void slotSelectionChanged( const QModelIndexList& );
    void slotCurrentChanged( const QModelIndex& );
    void slotEnableActions( bool on );

    void slotAddAccount();
    void slotAddSubAccount();
    void slotDeleteSelection();

    void slotAccountsOk();

private:
    AccountTreeView *m_view;

    KAction *actionAddAccount;
    KAction *actionAddSubAccount;
    KAction *actionDeleteSelection;

};

}  //KPlato namespace

#endif
