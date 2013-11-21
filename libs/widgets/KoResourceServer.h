/*  This file is part of the KDE project

    Copyright (c) 1999 Matthias Elter <elter@kde.org>
    Copyright (c) 2003 Patrick Julien <freak@codepimps.org>
    Copyright (c) 2005 Sven Langkamp <sven.langkamp@gmail.com>
    Copyright (c) 2007 Jan Hambrecht <jaham@gmx.net>
    Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
    Copyright (c) 2013 Sascha Suelzer <s.suelzer@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef KORESOURCESERVER_H
#define KORESOURCESERVER_H

#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QList>
#include <QFileInfo>
#include <QDir>
#include <QMultiMap>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kcomponentdata.h>

#include <QXmlStreamReader>
#include <QTemporaryFile>
#include <QDomDocument>
#include "KoResource.h"
#include "KoResourceServerObserver.h"
#include "KoResourceTagging.h"

#include "kowidgets_export.h"

#include <kdebug.h>

class KoResource;

/**
 * KoResourceServerBase is the base class of all resource servers
 */
class KOWIDGETS_EXPORT KoResourceServerBase {

public:
    /**
    * Constructs a KoResourceServerBase
    * @param resource type, has to be the same as used by KStandardDirs
    * @param extensions the file extensions separate by ':', e.g. "*.kgr:*.svg:*.ggr"
    */
    KoResourceServerBase(const QString& type, const QString& extensions)
        : m_type(type)
        , m_extensions(extensions)
        , m_cancelled(false)
    {
    }

    virtual ~KoResourceServerBase() {}

    virtual void loadResources(QStringList filenames) = 0;
    virtual QStringList blackListedFiles() const = 0;
    QString type() const { return m_type; }

    /**
    * File extensions for resources of the server
    * @returns the file extensions separated by ':', e.g. "*.kgr:*.svg:*.ggr"
    */
    QString extensions() const { return m_extensions; }

    void cancel() { m_cancelled = true; }

    QStringList fileNames() const
    {
        QStringList extensionList = m_extensions.split(':');
        QStringList fileNames;

        foreach (const QString &extension, extensionList) {
            fileNames += KGlobal::mainComponent().dirs()->findAllResources(type().toLatin1(), extension, KStandardDirs::Recursive | KStandardDirs::NoDuplicates);

        }
        return fileNames;
    }


private:
    QString m_type;
    QString m_extensions;

protected:

    bool   m_cancelled;
    QMutex m_loadLock;

};

/**
 * KoResourceServer manages the resources of one type. It stores, loads and saves the resources.
 * To keep track of changes the server can be observed with a KoResourceServerObserver
 */
template <class T> class KoResourceServer : public KoResourceServerBase {

public:
    KoResourceServer(const QString& type, const QString& extensions, bool deleteResource = true)
        : KoResourceServerBase(type, extensions)
        , m_deleteResource(deleteResource)
    {
        m_blackListFile = KStandardDirs::locateLocal("data", "krita/" + type + ".blacklist");
        m_blackListFileNames = readBlackListFile();
        m_tagObject = new KoResourceTagging(type, extensions);
    }

    virtual ~KoResourceServer()
    {
        if (m_deleteResource) {
            qDeleteAll(m_resources);
        }
        m_resources.clear();
        delete m_tagObject;
    }

    /**
     * Loads a set of resources and adds them to the resource server.
     * If a filename appears twice the resource will only be added once. Resources that can't
     * be loaded or and invalid aren't added to the server.
     * @param filenames list of filenames to be loaded
     */
    void loadResources(QStringList filenames) {
        kDebug(30009) << "loading  resources for type " << type();
        QStringList uniqueFiles;

        while (!filenames.empty() && !m_cancelled)
        {
            QString front = filenames.first();
            filenames.pop_front();

            QString fname = QFileInfo(front).fileName();

            // XXX: Don't load resources with the same filename. Actually, we should look inside
            //      the resource to find out whether they are really the same, but for now this
            //      will prevent the same brush etc. showing up twice.
            if (uniqueFiles.empty() || uniqueFiles.indexOf(fname) == -1) {
                m_loadLock.lock();
                uniqueFiles.append(fname);
                QList<T*> resources = createResources(front);
                foreach(T* resource, resources) {
                    Q_CHECK_PTR(resource);
                    if (resource->load() && resource->valid()) {
                        m_resourcesByFilename[resource->shortFilename()] = resource;

                        if ( resource->name().isEmpty() ) {
                            resource->setName( fname );
                        }
                        if (m_resourcesByName.contains(resource->name())) {
                            resource->setName(resource->name() + "(" + resource->shortFilename() + ")");
                        }
                        m_resourcesByName[resource->name()] = resource;
                        notifyResourceAdded(resource);
                    }
                    else {
                        if (m_deleteResource) {
                            delete resource;
                        }
                    }
                }
                m_loadLock.unlock();
            }
        }

        m_resources = sortedResources();

        kDebug(30009) << "done loading  resources for type " << type();
    }


    /// Adds an already loaded resource to the server
    bool addResource(T* resource, bool save = true, bool infront = false) {
        if (!resource->valid()) {
            kWarning(30009) << "Tried to add an invalid resource!";
            return false;
        }
        QFileInfo fileInfo(resource->filename());

        if (fileInfo.exists()) {
            QString filename = fileInfo.path() + "/" + fileInfo.baseName() + "XXXXXX" + "." + fileInfo.suffix();
            kDebug() << "fileName is " << filename;
            QTemporaryFile file(filename);
            if (file.open()) {
                kDebug() << "now " << file.fileName();
                resource->setFilename(file.fileName());
            }
        }

        if( save && ! resource->save()) {
            kWarning(30009) << "Could not save resource!";
            return false;
        }

        Q_ASSERT( !resource->filename().isEmpty() || !resource->name().isEmpty() );
        if ( resource->filename().isEmpty() ) {
            resource->setFilename( resource->name() );
        }
        else if ( resource->name().isEmpty() ) {
            resource->setName( resource->filename() );
        }

        m_resourcesByFilename[resource->shortFilename()] = resource;
        m_resourcesByName[resource->name()] = resource;
        if (infront) {
            m_resources.insert(0, resource);
        }
        else {
            m_resources.append(resource);
        }

        notifyResourceAdded(resource);

        return true;
    }

    /// Remove a resource from Resource Server but not from a file
    bool removeResourceFromServer(T* resource){
        if ( !m_resourcesByFilename.contains( resource->shortFilename() ) ) {
            return false;
        }

        m_resourcesByName.remove(resource->name());
        m_resourcesByFilename.remove(resource->shortFilename());
        m_resources.removeAt(m_resources.indexOf(resource));
        notifyRemovingResource(resource);

        if (m_deleteResource) {
            delete resource;
        }

        return true;
    }

    /// Remove a resource from resourceserver and hard disk
    bool removeResource(T* resource) {
        if ( !m_resourcesByFilename.contains( resource->shortFilename() ) ) {
            return false;
        }

        m_resourcesByName.remove(resource->name());
        m_resourcesByFilename.remove(resource->shortFilename());
        m_resources.removeAt(m_resources.indexOf(resource));
        notifyRemovingResource(resource);

        m_blackListFileNames.append(resource->filename());
        writeBlackListFile();
        if (m_deleteResource && resource) {
            delete resource;
        }
        return true;
    }

    QList<T*> resources() {
        m_loadLock.lock();
        QList<T*> resourceList = m_resources;
        foreach(T* r, m_resourceBlackList) {
            resourceList.removeOne(r);
        }
        m_loadLock.unlock();
        return resourceList;
    }

    /// Returns path where to save user defined and imported resources to
    virtual QString saveLocation() {
        return KGlobal::mainComponent().dirs()->saveLocation(type().toLatin1());
    }

    /**
     * Creates a new resource from a given file and adds them to the resource server
     * The base implementation does only load one resource per file, override to implement collections
     * @param filename file name of the resource file to be imported
     * @param fileCreation decides whether to create the file in the saveLocation() directory
     */
    virtual void importResourceFile(const QString & filename , bool fileCreation=true) {
        QFileInfo fi( filename );
        if( fi.exists() == false )
            return;
        if ( fi.size() == 0)
            return;

        T* resource = createResource( filename );
        resource->load();
        if(!resource->valid()){
            kWarning(30009) << "Import failed! Resource is not valid";
            if (m_deleteResource) {
                delete resource;
            }
            return;
        }

        if(fileCreation) {
            Q_ASSERT(!resource->defaultFileExtension().isEmpty());
            Q_ASSERT(!saveLocation().isEmpty());

            QString newFilename = saveLocation() + fi.baseName() + resource->defaultFileExtension();
            QFileInfo fileInfo(newFilename);

            int i = 1;
            while (fileInfo.exists()) {
                fileInfo.setFile(saveLocation() + fi.baseName() + QString("%1").arg(i) + resource->defaultFileExtension());
                i++;
            }
            resource->setFilename(fileInfo.filePath());
        }

        if(!addResource(resource)) {
            if (m_deleteResource) {
                delete resource;
            }
        }
    }

    /// Removes the resource file from the resource server
    virtual void removeResourceFile(const QString & filename)
    {
        QFileInfo fi(filename);

        T* resource = resourceByFilename(fi.fileName());
        if (!resource) {
            kWarning(30009) << "Resource file do not exist ";
            return;
        }

        if (!removeResourceFromServer(resource))
            return;
    }


    /**
     * Addes an observer to the server
     * @param observer the observer to be added
     * @param notifyLoadedResources determines if the observer should be notified about the already loaded resources
     */
    void addObserver(KoResourceServerObserver<T>* observer, bool notifyLoadedResources = true)
    {
        m_loadLock.lock();
        if(observer && !m_observers.contains(observer)) {
            m_observers.append(observer);

            if(notifyLoadedResources) {
                foreach(T* resource, m_resourcesByFilename) {
                    observer->resourceAdded(resource);
                }
            }
        }
        m_loadLock.unlock();
    }

    /**
     * Removes an observer from the server
     * @param observer the observer to be removed
     */
    void removeObserver(KoResourceServerObserver<T>* observer)
    {
        int index = m_observers.indexOf( observer );
        if( index < 0 )
            return;

        m_observers.removeAt( index );
    }

    T* resourceByFilename( const QString& filename ) const
    {
        return m_resourcesByFilename.value(filename);
    }


    T* resourceByName( const QString& name ) const
    {
        return m_resourcesByName.value(name);
    }

    /**
     * Call after changing the content of a resource;
     * Notifies the connected views.
     */
    void updateResource( T* resource )
    {
        notifyResourceChanged(resource);
    }

    QStringList blackListedFiles() const
    {
        return m_blackListFileNames;
    }

    void removeBlackListedFiles() {
        QStringList remainingFiles; // Files that can't be removed e.g. no rights will stay blacklisted
        foreach(const QString &filename, m_blackListFileNames) {
            QFile file( filename );
            if( ! file.remove() ) {
                remainingFiles.append(filename);
            }
        }
        m_blackListFileNames = remainingFiles;
        writeBlackListFile();
    }

    /// the below functions helps to access tagObject functions
    QStringList assignedTagsList( KoResource* resource ) const
    {
        return m_tagObject->assignedTagsList(resource);
    }

    QStringList tagNamesList() const
    {
        return m_tagObject->tagNamesList();
    }

    void addTag( KoResource* resource,const QString& tag)
    {
        m_tagObject->addTag(resource,tag);
    }

    void delTag( KoResource* resource,const QString& tag)
    {
        m_tagObject->delTag(resource,tag);
    }

    QStringList searchTag(const QString& lineEditText)
    {
        return m_tagObject->searchTag(lineEditText);
    }

    void tagCategoryAdded(const QString& tag)
    {
        m_tagObject->serializeTags();
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->syncTagAddition(tag);
        }
    }

    void tagCategoryRemoved(const QString& tag)
    {
        m_tagObject->serializeTags();
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->syncTagRemoval(tag);
        }
    }

    void tagCategoryMembersChanged()
    {
        m_tagObject->serializeTags();
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->syncTaggedResourceView();
        }
    }

    KoResourceTagging * tagObject() const
    {
        return m_tagObject;
    }


#ifdef NEPOMUK
    void updateNepomukXML(bool nepomukOn)
    {
        KoResourceTagging* tagObject = new KoResourceTagging(type(),extensions());
        tagObject->setNepomukBool(nepomukOn);
        tagObject->updateNepomukXML(nepomukOn);
        delete tagObject;
        m_tagObject->setNepomukBool(nepomukOn);
    }
#endif
protected:

    /**
     * Create one or more resources from a single file. By default one resource is created.
     * Overide to create more resources from the file.
     * @param filename the filename of the resource or resource collection
     */
    virtual QList<T*> createResources( const QString & filename )
    {
        QList<T*> createdResources;
        createdResources.append(createResource(filename));
        return createdResources;
    }

    virtual T* createResource( const QString & filename ) { return new T(filename); }

    /// Return the currently stored resources in alphabetical order, overwrite for customized sorting
    virtual QList<T*> sortedResources()
    {
        QMap<QString, T*> sortedNames;
        foreach(const QString &name, m_resourcesByName.keys()) {
            sortedNames.insert(name.toLower(), m_resourcesByName[name]);
        }
        return sortedNames.values();
    }

    void notifyResourceAdded(T* resource)
    {
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->resourceAdded(resource);
        }
    }

    void notifyRemovingResource(T* resource)
    {
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->removingResource(resource);
        }
    }

    void notifyResourceChanged(T* resource)
    {
        foreach(KoResourceServerObserver<T>* observer, m_observers) {
            observer->resourceChanged(resource);
        }
    }

    /// Reads the xml file and returns the filenames as a list
    QStringList readBlackListFile()
    {
        QStringList filenameList;

        QFile f(m_blackListFile);
        if (!f.open(QIODevice::ReadOnly)) {
            return filenameList;
        }

        QDomDocument doc;
        if (!doc.setContent(&f)) {
            kWarning() << "The file could not be parsed.";
            return filenameList;
        }

        QDomElement root = doc.documentElement();
        if (root.tagName() != "resourceFilesList") {
            kWarning() << "The file doesn't seem to be of interest.";
            return filenameList;
        }

        QDomElement file = root.firstChildElement("file");

        while (!file.isNull()) {
            QDomNode n = file.firstChild();
            QDomElement e = n.toElement();
            if (e.tagName() == "name") {
                filenameList.append((e.text()).replace(QString("~"),QDir::homePath()));
            }
            file = file.nextSiblingElement("file");
        }
        return filenameList;
    }

    /// write the blacklist file entries to an xml file
    void writeBlackListFile()
    {
        QFile f(m_blackListFile);

        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            kWarning() << "Cannot write meta information to '" << m_blackListFile << "'." << endl;
            return;
        }

        QDomDocument doc;
        QDomElement root;

        QDomDocument docTemp("m_blackListFile");
        doc = docTemp;
        doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
        root = doc.createElement("resourceFilesList");
        doc.appendChild(root);

        foreach(QString filename, m_blackListFileNames) {
            QDomElement fileEl = doc.createElement("file");
            QDomElement nameEl = doc.createElement("name");
            QDomText nameText = doc.createTextNode(filename.replace(QDir::homePath(),QString("~")));
            nameEl.appendChild(nameText);
            fileEl.appendChild(nameEl);
            root.appendChild(fileEl);
        }

        QTextStream metastream(&f);
        metastream << doc.toByteArray();
        f.close();
    }

private:

    QHash<QString, T*> m_resourcesByName;
    QHash<QString, T*> m_resourcesByFilename;
    QList<T*> m_resourceBlackList;
    QList<T*> m_resources; ///< list of resources in order of addition
    QList<KoResourceServerObserver<T>*> m_observers;
    bool m_deleteResource; // false if the resource is a shared pointer object
    QString m_blackListFile;
    QStringList m_blackListFileNames;
    KoResourceTagging* m_tagObject;

};

#endif // KORESOURCESERVER_H
