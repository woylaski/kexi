/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Reginald Stadlbauer <reggie@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef stylist_h
#define stylist_h

#include <kdialogbase.h>
#include <qstringlist.h>

#include "paragdia.h"

class KWDocument;
class KWFontChooser;
class KWStyle;
class KWStyleEditor;
class KWStyleManagerTab;
class KWStylePreview;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLineEdit;
class QListBox;
class QPushButton;
class QTabWidget;
class QWidget;

/******************************************************************/
/* Class: KWStyleManager                                          */
/******************************************************************/
class KWStyleManager : public KDialogBase
{
    Q_OBJECT

public:
    KWStyleManager( QWidget *_parent, KWDocument *_doc );

protected:
    KWDocument *m_doc;

    void setupWidget();
    void addGeneralTab();
    void apply();
    void updateGUI();
    void updatePreview();
    void save();
    int styleIndex( int pos );

    QTabWidget *m_tabs;
    QListBox *m_stylesList;
    QLineEdit *m_nameString;
    QComboBox *m_styleCombo;
    QPushButton *m_deleteButton;
    QPushButton *m_newButton;
    QPushButton *m_moveUpButton;
    QPushButton *m_moveDownButton;

    KWStylePreview *preview;

    KWStyle *m_currentStyle;
    QPtrList<KWStyle> m_origStyles;      // internal list of orig styles we have modified
    QPtrList<KWStyle> m_changedStyles;   // internal list of changed styles.
    QPtrList<KWStyleManagerTab> m_tabsList;
    int numStyles;
    bool noSignals;

protected slots:
    virtual void slotOk();
    virtual void slotApply();
    void switchStyle();
    void switchTabs();
    void addStyle();
    void deleteStyle();
    void moveUpStyle();
    void moveDownStyle();
    void renameStyle(const QString &);
protected:
    void addTab( KWStyleManagerTab * tab );
};

/******************************************************************/
/* Class: KWStylePreview                                         */
/******************************************************************/
class KWStylePreview : public QGroupBox
{
    Q_OBJECT

public:
    KWStylePreview( const QString &title, QWidget *parent );
    virtual ~KWStylePreview();

    void setStyle(KWStyle *_style);

protected:
    void drawContents( QPainter *painter );

    KWTextDocument *m_textdoc;
    KoZoomHandler *m_zoomHandler;
};

class KWStyleManagerTab : public QWidget {
    Q_OBJECT
public:
    KWStyleManagerTab(QWidget *parent) : QWidget(parent) {};

    /** the new style which is to be displayed */
    void setStyle(KWStyle *style) { m_style = style; }
    /**  update the GUI from the current Style*/
    virtual void update() = 0;
    /**  return the (i18n-ed) name of the tab */
    virtual QString tabName() = 0;
    /** save the GUI to the style */
    virtual void save() = 0;
protected:
    KWStyle *m_style;
};

// A tab to edit parts of the parag-layout of the style
// Acts as a wrapper around KWParagLayoutWidget [which doesn't know about styles].
class KWStyleParagTab : public KWStyleManagerTab
{
    Q_OBJECT
public:
    KWStyleParagTab( QWidget * parent )
        : KWStyleManagerTab( parent ) { m_widget = 0L; }

    // not a constructor parameter since 'this' is the parent of the widget
    void setWidget( KWParagLayoutWidget * widget );

    virtual void update();
    virtual void save();
    virtual QString tabName() { return m_widget->tabName(); }
protected:
    virtual void resizeEvent( QResizeEvent *e );
private:
    KWParagLayoutWidget * m_widget;
};

// The "font" tab. Maybe we should put the text color at the bottom ?
class KWStyleFontTab : public KWStyleManagerTab
{
    Q_OBJECT
public:
    KWStyleFontTab( QWidget * parent );
    ~KWStyleFontTab();
    virtual void update();
    virtual QString tabName();
    virtual void save();
protected:
    virtual void resizeEvent( QResizeEvent *e );
private:
    KWFontChooser* m_chooser;
    KoZoomHandler* m_zoomHandler;
};

/*
Font            simple font dia
Color           simple color dia
Spacing and Indents     paragraph spacing dia (KWParagDia)
alignments      KWParagDia alignment tab
borders         KWParagDia  borders tab
numbering       KWParagDia  tab numbering
tabulators      KWParagDia  tab tabs */

#endif
