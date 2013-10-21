/* This file is part of the KDE project
  Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
  Copyright (C) 2004 - 2010 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
  Copyright (C) 2007 Thorsten Zachmann <zachmann@kde.org>

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

#ifndef KPTMAINDOCUMENT_H
#define KPTMAINDOCUMENT_H

#include "kplato_export.h"

#include "kpttask.h"
#include "kptconfig.h"
#include "kptwbsdefinition.h"
#include "kptxmlloaderobject.h"
#include "about/aboutpage.h"

#include "KoDocument.h"

#include <QFileInfo>

#include <kdialog.h>

class KoView;

#define PLAN_MIME_TYPE "application/x-vnd.kde.plan"

/// The main namespace.
namespace KPlato
{

class DocumentChild;
class Project;
class Context;
class GanttView;
class SchedulerPlugin;
class ViewListItem;
class View;

class Package;

class KPLATO_EXPORT MainDocument : public KoDocument
{
    Q_OBJECT

public:
    explicit MainDocument(KoPart *part = 0);
    ~MainDocument();


    /// reimplemented from KoDocument
    virtual QByteArray nativeFormatMimeType() const { return PLAN_MIME_TYPE; }
    /// reimplemented from KoDocument
    virtual QByteArray nativeOasisMimeType() const { return ""; }
    /// reimplemented from KoDocument
    virtual QStringList extraNativeMimeTypes() const
    {
        return QStringList() << PLAN_MIME_TYPE;
    }

    void setReadWrite( bool rw );
    void configChanged();

    virtual void paintContent( QPainter& painter, const QRect& rect);

    void setProject( Project *project );
    Project &getProject() { return *m_project; }
    const Project &getProject() const { return * m_project; }

    /**
     * Return the set of SupportedSpecialFormats that the kplato wants to
     * offer in the "Save" file dialog.
     * Note: SaveEncrypted is not supported.
     */
    virtual int supportedSpecialFormats() const { return SaveAsDirectoryStore; }

    // The load and save functions. Look in the file kplato.dtd for info
    virtual bool loadXML( const KoXmlDocument &document, KoStore *store );
    virtual QDomDocument saveXML();
    /// Save a workpackage file containing @p node with schedule identity @p id, owned by @p resource
    QDomDocument saveWorkPackageXML( const Node *node, long id, Resource *resource = 0 );

    bool saveOdf( SavingContext &/*documentContext */) { return false; }
    bool loadOdf( KoOdfReadStore & odfStore );

    Config &config() { return m_config; }
    Context *context() const { return m_context; }

    WBSDefinition &wbsDefinition() { return m_project->wbsDefinition(); }

    const XMLLoaderObject &xmlLoader() const { return m_xmlLoader; }

    DocumentChild *createChild( KoDocument *doc, const QRect &geometry = QRect() );

    bool saveWorkPackageToStream( QIODevice * dev, const Node *node, long id, Resource *resource = 0 );
    bool saveWorkPackageFormat( const QString &file, const Node *node, long id, Resource *resource = 0 );
    bool saveWorkPackageUrl( const KUrl & _url, const Node *node, long id, Resource *resource = 0  );
    void mergeWorkPackages();
    void mergeWorkPackage( const Package *package );
    void terminateWorkPackage( const Package *package );

    /// Load the workpackage from @p url into @p project. Return true if successful, else false.
    bool loadWorkPackage( Project &project, const KUrl &url );
    Package *loadWorkPackageXML( Project& project, QIODevice*, const KoXmlDocument& document, const KUrl& url );
    QMap<KDateTime, Package*> workPackages() const { return m_workpackages; }

    void insertFile( const QString &filename, Node *parent, Node *after = 0 );
    bool insertProject( Project &project, Node *parent, Node *after );

    KPlatoAboutPage &aboutPage() { return m_aboutPage; }

    bool extractFiles( KoStore *store, Package *package );
    bool extractFile( KoStore *store, Package *package, const Document *doc );

    void registerView( View *view );

    /// Create a new project from this project
    /// Generates new project id and task ids
    /// Keeps resource- and calendar ids
    void createNewProject();

    using KoDocument::setModified;
public slots:
    void setModified( bool mod );

    /// Inserts an item into all other views than @p view
    void insertViewListItem( View *view, const ViewListItem *item, const ViewListItem *parent, int index );
    /// Removes the view list item from all other views than @p view
    void removeViewListItem( View *view, const ViewListItem *item );
    /// View selector has been modified
    void viewlistModified();
    /// Check for workpackages
    /// If @p keep is true, packages that has been refused will not be checked for again
    void checkForWorkPackages( bool keep = false );

    void setLoadingTemplate( bool );

signals:
    void changed();
    void workPackageLoaded();
    void viewlistModified( bool );
    void viewListItemAdded(const ViewListItem *item, const ViewListItem *parent, int index);
    void viewListItemRemoved(const ViewListItem *item);

protected:
    /// Load kplato specific files
    virtual bool completeLoading( KoStore* store );
    /// Save kplato specific files
    virtual bool completeSaving( KoStore* store );

    void mergeWorkPackage( Task *to, const Task *from, const Package *package );

    // used by insert file
    struct InsertFileInfo {
        QString url;
        Node *parent;
        Node *after;
    } m_insertFileInfo;


protected slots:
    void slotViewDestroyed();
    void addSchedulerPlugin( const QString&, SchedulerPlugin *plugin );

    void autoCheckForWorkPackages();
    void checkForWorkPackage();

    void insertFileCompleted();
    void insertFileCancelled( const QString& );

    void workPackageMergeDialogFinished( int result );

private:
    bool loadAndParse(KoStore* store, const QString& filename, KoXmlDocument& doc);

    void loadSchedulerPlugins();

private:
    Project *m_project;
    QWidget* m_parentWidget;

    Config m_config;
    Context *m_context;

    XMLLoaderObject m_xmlLoader;
    bool m_loadingTemplate;

    QMap<QString, SchedulerPlugin*> m_schedulerPlugins;
    QMap<KDateTime, Package*> m_workpackages;
    QFileInfoList m_infoList;
    QMap<KDateTime, Project*> m_mergedPackages;

    KPlatoAboutPage m_aboutPage;

    QDomDocument m_reports;

    bool m_viewlistModified;
    bool m_checkingForWorkPackages;

    QList<QPointer<View> > m_views;
};


}  //KPlato namespace

#endif
