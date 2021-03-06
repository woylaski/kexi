/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2004-2010 Jarosław Staniek <staniek@kde.org>

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

#include "kexitablepart.h"
#include <KexiIcon.h>
#include <core/KexiMainWindowIface.h>
#include <core/kexiproject.h>
#include <core/kexipartinfo.h>
#include <widget/tableview/KexiDataTableView.h>
#include <widget/tableview/KexiDataTableScrollArea.h>
#include "kexitabledesignerview.h"
#include "kexitabledesigner_dataview.h"
#include "kexilookupcolumnpage.h"
#include <KexiWindow.h>

#include <KDbConnection>

#include <KMessageBox>

#include <QDebug>
#include <QTabWidget>

KEXI_PLUGIN_FACTORY(KexiTablePart, "kexi_tableplugin.json")

//! @internal
class KexiTablePart::Private
{
public:
    Private() {
    }
    ~Private() {
        delete static_cast<KexiLookupColumnPage*>(lookupColumnPage);
    }
    QPointer<KexiLookupColumnPage> lookupColumnPage;
};

KexiTablePart::KexiTablePart(QObject *parent, const QVariantList& l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "table"),
        xi18nc("tooltip", "Create new table"),
        xi18nc("what's this", "Creates new table."),
        l)
  , d(new Private)
{
//! @todo js: also add Kexi::TextViewMode when we'll have SQL ALTER TABLE EDITOR!!!
}

KexiTablePart::~KexiTablePart()
{
    delete d;
}

void KexiTablePart::initPartActions()
{
}

void KexiTablePart::initInstanceActions()
{
}

KexiWindowData* KexiTablePart::createWindowData(KexiWindow* window)
{
    return new KexiTablePart::TempData(window);
}

KexiView* KexiTablePart::createView(QWidget *parent, KexiWindow* window,
                                    KexiPart::Item *item, Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_ASSERT(item);
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    if (!win || !win->project() || !win->project()->dbConnection())
        return 0;


    KexiTablePart::TempData *temp
        = static_cast<KexiTablePart::TempData*>(window->data());
    if (!temp->table) {
        temp->table = win->project()->dbConnection()->tableSchema(item->name());
        qDebug() << "schema is " << temp->table;
    }

    if (viewMode == Kexi::DesignViewMode) {
        KexiTableDesignerView *t = new KexiTableDesignerView(parent);
        return t;
    } else if (viewMode == Kexi::DataViewMode) {
        if (!temp->table) {
            return 0; //!< @todo message
        }
        //we're not setting table schema here -it will be forced to set
        // in KexiTableDesigner_DataView::afterSwitchFrom()
        KexiTableDesigner_DataView *t = new KexiTableDesigner_DataView(parent);
        return t;
    }
    return 0;
}

tristate KexiTablePart::remove(KexiPart::Item *item)
{
    KexiProject *project = KexiMainWindowIface::global()->project();
    if (!project || !project->dbConnection())
        return false;

    KDbConnection *conn = project->dbConnection();
    KDbTableSchema *sch = conn->tableSchema(item->identifier());

    if (sch) {
        tristate res = KexiTablePart::askForClosingObjectsUsingTableSchema(
            KexiMainWindowIface::global()->thisWidget(), conn, sch,
            xi18n("You are about to remove table <resource>%1</resource> but following objects using this table are opened:",
                 sch->name()));
        if (res != true) {
            return res;
        }
        return conn->dropTable(sch);
    }
    //last chance: just remove item
    return conn->removeObject(item->identifier());
}

tristate KexiTablePart::rename(KexiPart::Item *item, const QString& newName)
{
    Q_ASSERT(item);
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KDbTableSchema *schema = conn->tableSchema(item->identifier());
    if (!schema)
        return false;
    tristate res = KexiTablePart::askForClosingObjectsUsingTableSchema(
        KexiMainWindowIface::global()->thisWidget(), conn, schema,
        xi18n("You are about to rename table <resource>%1</resource> but following objects using this table are opened:",
             schema->name()));
    if (res != true) {
        return res;
    }
    return conn->alterTableName(schema, newName);
}

KDbObject* KexiTablePart::loadSchemaObject(KexiWindow *window, const KDbObject& object,
                              Kexi::ViewMode viewMode, bool *ownedByWindow)
{
    Q_UNUSED(window);
    Q_UNUSED(viewMode);
    if (ownedByWindow)
        *ownedByWindow = false;
    return KexiMainWindowIface::global()->project()->dbConnection()->tableSchema(object.name());
}

//static
tristate KexiTablePart::askForClosingObjectsUsingTableSchema(
    QWidget *parent, KDbConnection *conn,
    KDbTableSchema *table, const QString& msg)
{
    Q_ASSERT(conn);
    Q_ASSERT(table);
    QSet<KDbConnection::TableSchemaChangeListenerInterface*>* listeners
        = conn->tableSchemaChangeListeners(table);
    if (!listeners || listeners->isEmpty())
        return true;

    QString openedObjectsStr = "<list>";
    foreach(KDbConnection::TableSchemaChangeListenerInterface* iface, *listeners) {
        openedObjectsStr += QString("<item>%1</item>").arg(iface->listenerInfoString);
    }
    openedObjectsStr += "</list>";
    int r = KMessageBox::questionYesNo(parent,
                                       "<para>" + msg + "</para><para>" + openedObjectsStr + "</para><para>"
                                       + xi18n("Do you want to close all windows for these objects?")
                                       + "</para>",
                                       QString(), KGuiItem(xi18nc("@action:button Close All Windows", "Close Windows"), koIconName("window-close")), KStandardGuiItem::cancel());
    tristate res;
    if (r == KMessageBox::Yes) {
        //try to close every window
        res = conn->closeAllTableSchemaChangeListeners(table);
        if (res != true) //do not expose closing errors twice; just cancel
            res = cancelled;
    } else
        res = cancelled;

    return res;
}

KLocalizedString KexiTablePart::i18nMessage(
    const QString& englishMessage, KexiWindow* window) const
{
    Q_UNUSED(window);
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of table <resource>%1</resource> has been modified."));

    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Table <resource>%1</resource> already exists."));

    if (window->currentViewMode() == Kexi::DesignViewMode && !window->neverSaved()
            && englishMessage == ":additional message before saving design")
        return kxi18nc(I18NC_NOOP("@info", "Warning! Any data in this table will be removed upon design's saving!"));

    return Part::i18nMessage(englishMessage, window);
}

void KexiTablePart::setupCustomPropertyPanelTabs(QTabWidget *tab)
{
    if (!d->lookupColumnPage) {
        d->lookupColumnPage = new KexiLookupColumnPage(0);
        connect(d->lookupColumnPage,
                SIGNAL(jumpToObjectRequested(QString,QString)),
                KexiMainWindowIface::global()->thisWidget(),
                SLOT(highlightObject(QString,QString)));

//! @todo add "Table" tab
        /*
          connect(d->dataSourcePage, SIGNAL(formDataSourceChanged(QCString,QCString)),
            KFormDesigner::FormManager::self(), SLOT(setFormDataSource(QCString,QCString)));
          connect(d->dataSourcePage, SIGNAL(dataSourceFieldOrExpressionChanged(QString,QString,KDbField::Type)),
            KFormDesigner::FormManager::self(), SLOT(setDataSourceFieldOrExpression(QString,QString,KDbField::Type)));
          connect(d->dataSourcePage, SIGNAL(insertAutoFields(QString,QString,QStringList)),
            KFormDesigner::FormManager::self(), SLOT(insertAutoFields(QString,QString,QStringList)));*/
    }

    KexiProject *prj = KexiMainWindowIface::global()->project();
    d->lookupColumnPage->setProject(prj);

//! @todo add lookup field icon
    tab->addTab(d->lookupColumnPage, koIcon("combo"), QString());
    tab->setTabToolTip(tab->indexOf(d->lookupColumnPage), xi18n("Lookup column"));
}

KexiLookupColumnPage* KexiTablePart::lookupColumnPage() const
{
    return d->lookupColumnPage;
}

//----------------

KexiTablePart::TempData::TempData(QObject* parent)
        : KexiWindowData(parent)
        , table(0)
        , tableSchemaChangedInPreviousView(true /*to force reloading on startup*/)
{
}

#include "kexitablepart.moc"
