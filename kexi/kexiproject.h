/***************************************************************************
                          kexiproject.h  -  description
                             -------------------
    begin                : Sun Nov  17 23:30:00 CET 2002
    copyright            : (C) 2002 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KEXI_PART_H
#define KEXI_PART_H

#include <qobject.h>
#include <qptrlist.h>

#include <koDocument.h>
#include <kexiDB/kexidb.h>
#include <kexiformmanager.h>

class KexiDoc;
class DCOPObject;
class KexiRelation;
class KexiProjectPart;

struct FileReference
{
	QString group;
	QString name;
	QString location;
};

struct Credentials
{
	QString host,
		database,
		port,
		driver,
		user,
		password,
		socket;
	bool savePassword;
};

typedef QMap<QString, QDomElement> Groups;
typedef QValueList<FileReference> References;
typedef QPtrList<KexiProjectPart> PartList;

class KexiProject : public KoDocument
{
	Q_OBJECT
public:
	KexiProject( QWidget *parentWidget = 0, const char *widgetName = 0, QObject* parent = 0,
		const char* name = 0, bool singleViewMode = false );
	~KexiProject();
	virtual DCOPObject* dcopObject();
	virtual void paintContent( QPainter& painter, const QRect& rect, bool transparent = FALSE,
		double zoomX = 1.0, double zoomY = 1.0 );

	virtual bool initDoc();

	virtual bool loadXML( QIODevice *, const QDomDocument & );
	virtual QDomDocument saveXML();


	void addFileReference(FileReference);
	References fileReferences(const QString &group);

	bool initDbConnection(const Credentials& cred, const bool create = false);
	bool initHostConnection(const Credentials &cred);
	void clear();

	void registerProjectPart(KexiProjectPart *part);
	PartList *getParts();

	KexiDB* db()const { return m_db; };
	KexiFormManager *formManager()const {return m_formManager;}
	bool dbIsAvaible()const { return m_dbAvaible; }
	QString boolToString(bool b);
	bool stringToBool(const QString &s);
	KexiRelation *relationManager();
signals:
	void dbAvaible();
	void saving(KoStore *);
	void partListUpdated();

/*
#undef signals
#define signals public
signals:
#undef signals
#define signals protected
*/

protected:
	virtual KoView* createViewInstance( QWidget* parent, const char* name );
	virtual bool completeSaving( KoStore* store );
	virtual bool completeLoading( KoStore* store );
	void setCurrentDB(){} ;
	bool saveProject();
	bool saveProjectAs(const QString&);
	bool loadProject(const QString&);
private:
	void saveConnectionSettings(QDomDocument&);
	void saveReferences(QDomDocument&);
	void loadConnectionSettings(QDomElement&);
	void loadReferences(QDomElement&);
	void loadHandlers();
	KexiDoc*        m_settings;
	KexiDB*         m_db;
	KexiFormManager *m_formManager;
	Credentials     m_cred;
	bool            m_dbAvaible;
	References      m_fileReferences;
	Groups          m_refGroups;
	KexiRelation	*m_relationManager;
	PartList	*m_parts;
	DCOPObject *dcop;
};

#endif
