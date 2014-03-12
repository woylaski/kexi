/*
 *  Copyright (c) 2010 Adam Celarek <kdedev at xibo dot at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_shade_selector_line_combo_box.h"

#include <QGridLayout>
#include <QPainter>

#include <klocale.h>

#include "kis_shade_selector_line.h"
#include "kis_shade_selector_line_combo_box_popup.h"

KisShadeSelectorLineComboBox::KisShadeSelectorLineComboBox(QWidget *parent) :
    QComboBox(parent),
    m_popup(new KisShadeSelectorLineComboBoxPopup(this)),
    m_currentLine(new KisShadeSelectorLine(0,0,0,this))
{
    QGridLayout* l = new QGridLayout(this);
    l->addWidget(m_currentLine);

    m_currentLine->setEnabled(false);
    m_currentLine->setColor(QColor(190,50,50));

    updateSettings();
}

void KisShadeSelectorLineComboBox::hidePopup()
{
    QComboBox::hidePopup();
    m_popup->hide();
}

void KisShadeSelectorLineComboBox::showPopup()
{
    // only show if this is not the popup
    QComboBox::showPopup();
    m_popup->move(mapToGlobal(QPoint(0,-300)));
    m_popup->show();
    m_popup->setConfiguration(m_currentLine->toString());
}

void KisShadeSelectorLineComboBox::setConfiguration(const QString &stri)
{
    m_currentLine->fromString(stri);
}

QString KisShadeSelectorLineComboBox::configuration() const
{
    return m_currentLine->toString();
}

void KisShadeSelectorLineComboBox::setLineNumber(int n)
{
    m_currentLine->setLineNumber(n);
    for(int i=0; i<m_popup->layout()->count(); i++) {
        KisShadeSelectorLine* item = dynamic_cast<KisShadeSelectorLine*>(m_popup->layout()->itemAt(i)->widget());
        if(item!=0) {
            item->setLineNumber(n);
        }
    }
}

void KisShadeSelectorLineComboBox::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    m_currentLine->setMaximumWidth(width()-30-m_popup->spacing);
    m_popup->setMinimumWidth(qMax(280, width()));
    m_popup->setMaximumWidth(qMax(280, width()));
}

void KisShadeSelectorLineComboBox::updateSettings()
{
    m_currentLine->updateSettings();
    for(int i=0; i<m_popup->layout()->count(); i++) {
        KisShadeSelectorLine* item = dynamic_cast<KisShadeSelectorLine*>(m_popup->layout()->itemAt(i)->widget());
        if(item!=0) {
            item->updateSettings();
            item->m_lineHeight=30;
            item->setMaximumHeight(30);
            item->setMinimumHeight(30);
        }
    }

    setLineHeight(m_currentLine->m_lineHeight);
}


void KisShadeSelectorLineComboBox::setGradient(bool b)
{
    m_currentLine->m_gradient=b;
    for(int i=0; i<m_popup->layout()->count(); i++) {
        KisShadeSelectorLine* item = dynamic_cast<KisShadeSelectorLine*>(m_popup->layout()->itemAt(i)->widget());
        if(item!=0) {
            item->m_gradient=b;
        }
    }

    update();
}

void KisShadeSelectorLineComboBox::setPatches(bool b)
{
    m_currentLine->m_gradient=!b;
    for(int i=0; i<m_popup->layout()->count(); i++) {
        KisShadeSelectorLine* item = dynamic_cast<KisShadeSelectorLine*>(m_popup->layout()->itemAt(i)->widget());
        if(item!=0) {
            item->m_gradient=!b;
        }
    }

    update();
}

void KisShadeSelectorLineComboBox::setPatchCount(int count)
{
    m_currentLine->m_patchCount=count;
    for(int i=0; i<m_popup->layout()->count(); i++) {
        KisShadeSelectorLine* item = dynamic_cast<KisShadeSelectorLine*>(m_popup->layout()->itemAt(i)->widget());
        if(item!=0) {
            item->m_patchCount=count;
        }
    }

    update();
}

void KisShadeSelectorLineComboBox::setLineHeight(int height)
{
    m_currentLine->m_lineHeight=height;
    m_currentLine->setMinimumHeight(height);
    m_currentLine->setMaximumHeight(height);
    setMinimumHeight(height+m_popup->spacing);
    setMaximumHeight(height+m_popup->spacing);

    update();
}
