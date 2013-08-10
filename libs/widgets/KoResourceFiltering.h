
/*  This file is part of the KDE project

    Copyright (c) 2013 Sascha Suelzer <s_suelzer@lavabit.com>

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

#ifndef KORESOURCEFILTER_H
#define KORESOURCEFILTER_H

#include <QStringList>
#include <QString>

#include "KoResource.h"

#include "kowidgets_export.h"

class KoResourceTagging;

class KOWIDGETS_EXPORT KoResourceFiltering
{

public:
    KoResourceFiltering();
    virtual ~KoResourceFiltering();
    bool hasFilters() const;
    bool filtersHaveChanged() const;
    void setTagSetFilenames(const QStringList& filenames);
    void setCurrentTag(const QString& tagSet);
    void rebuildCurrentTagFilenames();
    void setTagObject(KoResourceTagging* tagObject);
    void setFilters(const QString& searchString);
    QList<KoResource*> filterResources(QList< KoResource* > resources);
    void setInclusions(const QStringList &inclusions);
    void setExclusions(const QStringList &exclusions);
private:

    void setDoneFiltering();
    bool presetMatchesSearch(KoResource * resource) const;
    void setChanged();
    bool excludeFilterIsValid(const QString &exclusion);
    bool matchesResource(const QString &resourceName, const QString &resourceFileName,const QStringList &filterList) const;
    void populateIncludeExcludeFilters(const QStringList& filteredNames);
    void sanitizeExclusionList();
    QStringList tokenizeSearchString(const QString& searchString) const;
    class Private;
    Private * const d;

};

#endif // KORESOURCEFILTER_H
