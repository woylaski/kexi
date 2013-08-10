/* This file is part of the KDE project
  Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

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


#include <kptdurationspinbox.h>

#include "kptnode.h"

#include <QEvent>
#include <QLineEdit>
#include <QLocale>
#include <QValidator>
#include <QKeyEvent>

#include <kdebug.h>
#include <knumvalidator.h>

#include <math.h>
#include <limits.h>

namespace KPlato
{

DurationSpinBox::DurationSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent),
    m_unit( Duration::Unit_d ),
    m_minunit( Duration::Unit_h ),
    m_maxunit( Duration::Unit_Y )
{
    setUnit( Duration::Unit_h );
    setMaximum(140737488355328.0); //Hmmmm

    connect( lineEdit(), SIGNAL(textChanged(QString)), SLOT(editorTextChanged(QString)) );

}

void DurationSpinBox::setUnit( Duration::Unit unit )
{
    if ( unit < m_maxunit ) {
        m_maxunit = unit;
    } else if ( unit > m_minunit ) {
        m_minunit = unit;
    }
    m_unit = unit;
    setValue( value() );
}

void DurationSpinBox::setMaximumUnit( Duration::Unit unit )
{
    //NOTE Year = 0, Milliseconds = 7 !!!
    m_maxunit = unit;
    if ( m_minunit < unit ) {
        m_minunit = unit;
    }
    if ( m_unit < unit ) {
        setUnit( unit );
        emit unitChanged( m_unit );
    }
}

void DurationSpinBox::setMinimumUnit( Duration::Unit unit )
{
    //NOTE Year = 0, Milliseconds = 7 !!!
    m_minunit = unit;
    if ( m_maxunit > unit ) {
        m_maxunit = unit;
    }
    if ( m_unit > unit ) {
        setUnit( unit );
        emit unitChanged( m_unit );
    }
}

void DurationSpinBox::stepUnitUp()
{
    //kDebug(planDbg())<<m_unit<<">"<<m_maxunit;
    if ( m_unit > m_maxunit ) {
        setUnit( static_cast<Duration::Unit>(m_unit - 1) );
        // line may change length, make sure cursor stays within unit
        lineEdit()->setCursorPosition( lineEdit()->displayText().length() - suffix().length() );
        emit unitChanged( m_unit );
    }
}

void DurationSpinBox::stepUnitDown()
{
    //kDebug(planDbg())<<m_unit<<"<"<<m_minunit;
    if ( m_unit < m_minunit ) {
        setUnit( static_cast<Duration::Unit>(m_unit + 1) );
        // line may change length, make sure cursor stays within unit
        lineEdit()->setCursorPosition( lineEdit()->displayText().length() - suffix().length() );
        emit unitChanged( m_unit );
    }
}

void DurationSpinBox::stepBy( int steps )
{
    //kDebug(planDbg())<<steps;
    int cpos = lineEdit()->cursorPosition();
    if ( isOnUnit() ) {
        // we are in unit
        if ( steps > 0 ) {
            stepUnitUp();
        } else if ( steps < 0 ) {
            stepUnitDown();
        }
        lineEdit()->setCursorPosition( cpos );
        return;
    }
    QDoubleSpinBox::stepBy( steps );
    // QDoubleSpinBox selects the whole text and the cursor might end up at the end (in the unit field)
    lineEdit()->setCursorPosition( cpos ); // also deselects
}

QAbstractSpinBox::StepEnabled DurationSpinBox::stepEnabled () const
{
    if ( isOnUnit() ) {
        if ( m_unit >= m_minunit ) {
            //kDebug(planDbg())<<"inside unit, up"<<m_unit<<m_minunit<<m_maxunit;
            return QAbstractSpinBox::StepUpEnabled;
        }
        if ( m_unit <= m_maxunit ) {
            //kDebug(planDbg())<<"inside unit, down"<<m_unit<<m_minunit<<m_maxunit;
            return QAbstractSpinBox::StepDownEnabled;
        }
        //kDebug(planDbg())<<"inside unit, up|down"<<m_unit<<m_minunit<<m_maxunit;
        return QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    }
    return QDoubleSpinBox::stepEnabled();
}

bool DurationSpinBox::isOnUnit() const
{
    int pos = lineEdit()->cursorPosition();
    return ( pos <= text().size() - suffix().size() ) &&
           ( pos > text().size() - suffix().size() - Duration::unitToString( m_unit, true ).size() );
}

void DurationSpinBox::keyPressEvent( QKeyEvent * event )
{
    //kDebug(planDbg())<<lineEdit()->cursorPosition()<<","<<(text().size() - Duration::unitToString( m_unit, true ).size())<<""<<event->text().isEmpty();
    if ( isOnUnit() ) {
        // we are in unit
        switch (event->key()) {
        case Qt::Key_Up:
            event->accept();
            stepBy( 1 );
            return;
        case Qt::Key_Down:
            event->accept();
            stepBy( -1 );
            return;
        default:
            break;
        }
    }
    QDoubleSpinBox::keyPressEvent(event);
}

// handle unit, QDoubleSpinBox handles value, signals etc
void DurationSpinBox::editorTextChanged( const QString &text ) {
    //kDebug(planDbg())<<text;
    QString s = text;
    int pos = lineEdit()->cursorPosition();
    if ( validate( s, pos ) == QValidator::Acceptable ) {
        s = extractUnit( s );
        if ( ! s.isEmpty() ) {
            updateUnit( (Duration::Unit)Duration::unitList( true ).indexOf( s ) );
        }
    }
}

double DurationSpinBox::valueFromText( const QString & text ) const
{
    QString s = extractValue( text ).remove( KGlobal::locale()->thousandsSeparator() );
    bool ok = false;
    double v = KGlobal::locale()->readNumber( s, &ok );
    if ( ! ok ) {
        v = QDoubleSpinBox::valueFromText( s );
    }
    return v;
}

QString DurationSpinBox::textFromValue ( double value ) const
{
    QString s = KGlobal::locale()->formatNumber( qMin( qMax( minimum(), value ), maximum() ), decimals() );
    s += Duration::unitToString( m_unit, true );
    //kDebug(planDbg())<<2<<value<<s;
    return s;
}

QValidator::State DurationSpinBox::validate ( QString & input, int & pos ) const
{
    //kDebug(planDbg())<<input;
    KDoubleValidator validator( minimum(), maximum(), decimals(), 0 );
    if ( input.isEmpty() ) {
        return validator.validate ( input, pos );
    }
    QString s = extractUnit( input );
    if ( s.isEmpty() ) {
        return validator.validate ( input, pos );
    }
    int idx = Duration::unitList( true ).indexOf( s ); 
    if ( idx < m_maxunit || idx > m_minunit ) {
        return QValidator::Invalid;
    }
    s = extractValue( input );
    int p = 0;
    return validator.validate ( s, p ); // pos doesn't matter
}

QString DurationSpinBox::extractUnit ( const QString &text ) const
{
    //kDebug(planDbg())<<text;
    QString s;
    for ( int i = text.length() - 1; i >= 0; --i ) {
        QChar c = text[ i ];
        if ( ! c.isLetter() ) {
            break;
        }
        s.prepend( c );
    }
    if ( Duration::unitList( true ).contains( s ) ) {
        return s;
    }
    return QString();
}

QString DurationSpinBox::extractValue ( const QString &text ) const
{
    //kDebug(planDbg())<<text;
    QString s = extractUnit( text );
    if ( Duration::unitList( true ).contains( s ) ) {
        return text.left( text.length() - s.length() );
    }
    return text;
}

void DurationSpinBox::updateUnit( Duration::Unit unit )
{
    if ( unit < m_maxunit ) {
        m_unit = m_maxunit;
    } else if ( unit > m_minunit ) {
        m_unit = m_minunit;
    }
    if ( m_unit != unit ) {
        m_unit = unit;
        emit unitChanged( unit );
    }
}


} //namespace KPlato

#include "kptdurationspinbox.moc"

