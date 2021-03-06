/* This file is part of the KDE project
   Copyright (C) 2005-2009 Jarosław Staniek <staniek@kde.org>

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
#ifndef KEXIDATASOURCEPAGE_H
#define KEXIDATASOURCEPAGE_H

#include "kexiformutils_export.h"
#include <widget/properties/KexiPropertyPaneViewBase.h>

#include <KDbField>
#include <KDbUtils>
#include <KDbTableOrQuerySchema>

#include <KPropertySet>

class KexiDataSourceComboBox;
class KexiFieldComboBox;
class KexiFieldListView;
class KexiProject;
class QToolButton;
class QLabel;

//! A page within form designer's property tabbed pane, providing data source editor
class KEXIFORMUTILS_EXPORT KexiDataSourcePage : public KexiPropertyPaneViewBase
{
    Q_OBJECT

public:
    explicit KexiDataSourcePage(QWidget *parent);
    virtual ~KexiDataSourcePage();

public Q_SLOTS:
    void setProject(KexiProject *prj);
    void clearFormDataSourceSelection(bool alsoClearComboBox = true);
    void clearWidgetDataSourceSelection();

    //! Sets data source of a currently selected form.
    //! This is performed on form initialization and on activating.
    void setFormDataSource(const QString& pluginId, const QString& name);

    //! Receives a pointer to a new property \a set (from KexiFormView::managerPropertyChanged())
    void assignPropertySet(KPropertySet* propertySet);

Q_SIGNALS:
    //! Signal emitted when helper button 'go to selected data source' is clicked.
    void jumpToObjectRequested(const QString& mime, const QString& name);

    //! Signal emitted when form's data source has been changed. It's connected to the Form Manager.
    void formDataSourceChanged(const QString& mime, const QString& name);

    /*! Signal emitted when current widget's data source (field/expression)
     has been changed. It's connected to the Form Manager.
     \a caption for this field is also provided (e.g. AutoField form widget use it) */
    void dataSourceFieldOrExpressionChanged(const QString& string, const QString& caption,
                                            KDbField::Type type);

    /*! Signal emitted when 'insert fields' button has been clicked */
    void insertAutoFields(const QString& sourcePartClass, const QString& sourceName,
                          const QStringList& fields);

protected Q_SLOTS:
    void slotWidgetDataSourceTextChanged(const QString &text);
    void slotFormDataSourceTextChanged(const QString &text);
    void slotFormDataSourceChanged();
    void slotFieldSelected();
    void slotGotoSelected();
    void slotInsertSelectedFields();
    void slotFieldListViewSelectionChanged();
    void slotFieldDoubleClicked(const QString& sourcePluginId, const QString& sourceName,
                                const QString& fieldName);

protected:
    void updateSourceFieldWidgetsAvailability();

    KexiFieldComboBox *m_widgetDataSourceCombo;
    QWidget *m_widgetDataSourceComboSpacer;
    KexiDataSourceComboBox* m_formDataSourceCombo;
    QWidget *m_formDataSourceComboSpacer;
    QLabel *m_dataSourceLabel, *m_noDataSourceAvailableLabel,
    *m_widgetDSLabel, *m_availableFieldsLabel,
    *m_mousePointerLabel, *m_availableFieldsDescriptionLabel;
    QToolButton *m_gotoButton, *m_addField;
    QString m_noDataSourceAvailableSingleText;
    QString m_noDataSourceAvailableMultiText;
    bool m_insideClearFormDataSourceSelection;
#ifdef KEXI_NO_AUTOFIELD_WIDGET
    KDbTableOrQuerySchema *m_tableOrQuerySchema; //!< temp.
#else
    KexiFieldListView* m_fieldListView;
#endif

    //! Used only in assignPropertySet() to check whether we already have the set assigned
    QString m_currentObjectName;
};

#endif
