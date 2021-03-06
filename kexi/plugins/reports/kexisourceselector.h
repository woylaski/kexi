/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg (adam@piggz.co.uk)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KEXISOURCESELECTOR_H
#define KEXISOURCESELECTOR_H

#include <QWidget>

#include <KDbConnection>

#include <KoReportData>

#include "kexidbreportdata.h"
#ifdef HAVE_KEXI_MIGRATE
#include "keximigratereportdata.h"
#endif
class KexiProject;

//! @todo rename to KexiReportDataSourcePage
//! @todo use KexiPropertyPaneViewBase
class KexiSourceSelector : public QWidget
{
    Q_OBJECT
public:
    explicit KexiSourceSelector(KexiProject* project, QWidget* parent = 0);
    ~KexiSourceSelector();

    KoReportData* sourceData();
    void setConnectionData(QDomElement);
    QDomElement connectionData();

Q_SIGNALS:
    void setData(KoReportData*);

private Q_SLOTS:
    void setDataClicked();

private:
    class Private;
    Private * const d;
};

#endif // KEXISOURCESELECTOR_H
