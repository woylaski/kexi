/*
 * Kivio - Visual Modelling and Flowcharting
 * Copyright (C) 2000 theKompany.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef KIVIO_DLG_LAYERNAME_H
#define KIVIO_DLG_LAYERNAME_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qdialog.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qstringlist.h>
#include <qlistbox.h>
#include <qrect.h>
#include <qcheckbox.h>

class QApplication;
class KivioView;
class KivioLayer;

class KivioLayerName : public QDialog
{ Q_OBJECT
public:
  KivioLayerName( KivioView* parent, const char* name, QString &_layerName );

  QString layerName() { return m_pLayerName->text(); }

public slots:
  void slotOk();
  void slotClose();

protected:
  KivioView* m_pView;

  QLineEdit* m_pLayerName;
  QPushButton* m_pOk;
  QPushButton* m_pClose;

  QString m_LayerName;
};





#endif

