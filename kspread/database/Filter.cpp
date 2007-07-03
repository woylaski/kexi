/* This file is part of the KDE project
   Copyright 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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
   Boston, MA 02110-1301, USA.
*/

#include "Filter.h"

#include <QList>
#include <QRect>
#include <QString>

#include <KoXmlNS.h>
#include <KoXmlWriter.h>

#include "CellStorage.h"
#include "Database.h"
#include "Doc.h"
#include "RowColumnFormat.h"
#include "Region.h"
#include "Sheet.h"
#include "Util.h"
#include "Value.h"
#include "ValueConverter.h"

using namespace KSpread;

class AbstractCondition
{
public:
    virtual ~AbstractCondition() {}
    enum Type { And, Or, Condition };
    virtual Type type() const = 0;
    virtual void loadOdf(const KoXmlElement& element) = 0;
    virtual void saveOdf(KoXmlWriter& xmlWriter) = 0;
    virtual bool evaluate(const Database* database, int index) const = 0;
    virtual bool isEmpty() const = 0;
    virtual void removeConditions(int fieldNumber) = 0;
    virtual void dump() const = 0;
};

/**
 * OpenDocument, 8.7.2 Filter And
 */
class Filter::And : public AbstractCondition
{
public:
    And() {}
    And(const And& other);
    virtual ~And() { qDeleteAll(list); }
    virtual Type type() const { return AbstractCondition::And; }
    virtual void loadOdf(const KoXmlElement& parent);
    virtual void saveOdf(KoXmlWriter& xmlWriter)
    {
        if (!list.count())
            return;
        xmlWriter.startElement("table:filter-and");
        for (int i = 0; i < list.count(); ++i)
            list[i]->saveOdf(xmlWriter);
        xmlWriter.endElement();
    }
    virtual bool evaluate(const Database* database, int index) const
    {
        for (int i = 0; i < list.count(); ++i)
        {
            // lazy evaluation, stop on first false
            if (!list[i]->evaluate(database, index))
                return false;
        }
        return true;
    }
    virtual bool isEmpty() const { return list.isEmpty(); }
    virtual void removeConditions(int fieldNumber)
    {
        for (int i = 0; i < list.count(); ++i)
            list[i]->removeConditions(fieldNumber);
        QList<AbstractCondition*> newList;
        for (int i = 0; i < list.count(); ++i)
        {
            if (!list[i]->isEmpty())
                newList.append(list[i]);
        }
        list = newList;
    }
    virtual void dump() const
    {
        for (int i = 0; i < list.count(); ++i)
        {
            if (i)
                kDebug() << "AND" << endl;
            list[i]->dump();
        }
    }

public:
    QList<AbstractCondition*> list; // allowed: Or or Condition
};

/**
 * OpenDocument, 8.7.3 Filter Or
 */
class Filter::Or : public AbstractCondition
{
public:
    Or() {}
    Or(const Or& other);
    virtual ~Or() { qDeleteAll(list); }
    virtual Type type() const { return AbstractCondition::Or; }
    virtual void loadOdf(const KoXmlElement& element);
    virtual void saveOdf(KoXmlWriter& xmlWriter)
    {
        if (!list.count())
            return;
        xmlWriter.startElement("table:filter-or");
        for (int i = 0; i < list.count(); ++i)
            list[i]->saveOdf(xmlWriter);
        xmlWriter.endElement();
    }
    virtual bool evaluate(const Database* database, int index) const
    {
        for (int i = 0; i < list.count(); ++i)
        {
            // lazy evaluation, stop on first true
            if (list[i]->evaluate(database, index))
                return true;
        }
        return false;
    }
    virtual bool isEmpty() const { return list.isEmpty(); }
    virtual void removeConditions(int fieldNumber)
    {
        for (int i = 0; i < list.count(); ++i)
            list[i]->removeConditions(fieldNumber);
        QList<AbstractCondition*> newList;
        for (int i = 0; i < list.count(); ++i)
        {
            if (!list[i]->isEmpty())
                newList.append(list[i]);
        }
        list = newList;
    }
    virtual void dump() const
    {
        for (int i = 0; i < list.count(); ++i)
        {
            if (i)
                kDebug() << "OR" << endl;
            list[i]->dump();
        }
    }

public:
    QList<AbstractCondition*> list; // allowed: And or Condition
};

/**
 * OpenDocument, 8.7.4 Filter Condition
 */
class Filter::Condition : public AbstractCondition
{
public:
    Condition()
        : fieldNumber(-1)
        , operation(Match)
        , caseSensitivity(Qt::CaseInsensitive)
        , dataType(Text)
    {
    }
    Condition(int _fieldNumber, Comparison _comparison, const QString& _value,
              Qt::CaseSensitivity _caseSensitivity, Mode _mode)
        : fieldNumber(_fieldNumber)
        , value(_value)
        , operation(_comparison)
        , caseSensitivity(_caseSensitivity)
        , dataType(_mode)
    {
    }
    Condition(const Condition& other)
        : AbstractCondition()
        , fieldNumber(other.fieldNumber)
        , value(other.value)
        , operation(other.operation)
        , caseSensitivity(other.caseSensitivity)
        , dataType(other.dataType)
    {
    }
    virtual ~Condition() {}

    virtual Type type() const { return AbstractCondition::Condition; }
    virtual void loadOdf(const KoXmlElement& element)
    {
        if (element.hasAttributeNS(KoXmlNS::table, "field-number"))
        {
            bool ok = false;
            fieldNumber = element.attributeNS(KoXmlNS::table, "field-number", QString()).toInt(&ok);
            if (!ok || fieldNumber < 0)
            {
                fieldNumber = -1;
                return;
            }
        }
        if (element.hasAttributeNS(KoXmlNS::table, "value"))
            value = element.attributeNS(KoXmlNS::table, "value", QString());
        if (element.hasAttributeNS(KoXmlNS::table, "operator"))
        {
            const QString string = element.attributeNS(KoXmlNS::table, "operator", QString());
            if (string == "match")
                operation = Match;
            else if (string == "!match")
                operation = NotMatch;
            else if (string == "=")
                operation = Equal;
            else if (string == "!=")
                operation = NotEqual;
            else if (string == "<")
                operation = Less;
            else if (string == ">")
                operation = Greater;
            else if (string == "<=")
                operation = LessOrEqual;
            else if (string == ">=")
                operation = GreaterOrEqual;
            else if (string == "empty")
                operation = Empty;
            else if (string == "!empty")
                operation = NotEmpty;
            else if (string == "top values")
                operation = TopValues;
            else if (string == "bottom values")
                operation = BottomValues;
            else if (string == "top percent")
                operation = TopPercent;
            else if (string == "bottom percent")
                operation = BottomPercent;
            else
            {
                kDebug() << "table:operator: unknown value" << endl;
                return;
            }
        }
        if (element.hasAttributeNS(KoXmlNS::table, "case-sensitive"))
        {
            if (element.attributeNS(KoXmlNS::table, "case-sensitive", "false") == "true")
                caseSensitivity = Qt::CaseSensitive;
            else
                caseSensitivity = Qt::CaseInsensitive;
        }
        if (element.hasAttributeNS(KoXmlNS::table, "data-type"))
        {
            if (element.attributeNS(KoXmlNS::table, "data-type", "text") == "number")
                dataType = Number;
            else
                dataType = Text;
        }
    }
    virtual void saveOdf(KoXmlWriter& xmlWriter)
    {
        if (fieldNumber < 0)
            return;
        xmlWriter.startElement("table:filter-condition");
        xmlWriter.addAttribute("table:field-number", fieldNumber);
        xmlWriter.addAttribute("table:value", value);
        switch (operation)
        {
            case Match:
                xmlWriter.addAttribute("table:operator", "match");
                break;
            case NotMatch:
                xmlWriter.addAttribute("table:operator", "!match");
                break;
            case Equal:
                xmlWriter.addAttribute("table:operator", "=");
                break;
            case NotEqual:
                xmlWriter.addAttribute("table:operator", "!=");
                break;
            case Less:
                xmlWriter.addAttribute("table:operator", "<");
                break;
            case Greater:
                xmlWriter.addAttribute("table:operator", ">");
                break;
            case LessOrEqual:
                xmlWriter.addAttribute("table:operator", "<=");
                break;
            case GreaterOrEqual:
                xmlWriter.addAttribute("table:operator", ">=");
                break;
            case Empty:
                xmlWriter.addAttribute("table:operator", "empty");
                break;
            case NotEmpty:
                xmlWriter.addAttribute("table:operator", "!empty");
                break;
            case TopValues:
                xmlWriter.addAttribute("table:operator", "top values");
                break;
            case BottomValues:
                xmlWriter.addAttribute("table:operator", "bottom values");
                break;
            case TopPercent:
                xmlWriter.addAttribute("table:operator", "top percent");
                break;
            case BottomPercent:
                xmlWriter.addAttribute("table:operator", "bottom percent");
                break;
        }
        if (caseSensitivity == Qt::CaseSensitive)
            xmlWriter.addAttribute("table:case-sensitive", true);
        if (dataType == Text)
            xmlWriter.addAttribute("table:data-type", "number");
        xmlWriter.endElement();
    }
    virtual bool evaluate(const Database* database, int index) const
    {
        const Sheet* sheet = (*database->range().constBegin())->sheet();
        const QRect range = database->range().lastRange();
        const int start = database->orientation() == Qt::Vertical ? range.left() : range.top();
        kDebug() << "index: " << index << " start: " << start << " fieldNumber: " << fieldNumber << endl;
        const Value value = database->orientation() == Qt::Vertical
                            ? sheet->cellStorage()->value(start + fieldNumber, index)
                            : sheet->cellStorage()->value(index, start + fieldNumber);
        const QString testString = sheet->doc()->converter()->asString(value).asString();
        switch (operation)
        {
            case Match:
            {
                kDebug() << "Match? " << this->value << " " << testString << endl;
                if (QString::compare(this->value, testString, caseSensitivity) == 0)
                    return true;
                break;
            }
            case NotMatch:
            {
                kDebug() << "Not Match? " << this->value << " " << testString << endl;
                if (QString::compare(this->value, testString, caseSensitivity) != 0)
                    return true;
                break;
            }
            default:
                break;
        }
        return false;
    }
    virtual bool isEmpty() const { return fieldNumber == -1; }
    virtual void removeConditions(int fieldNumber)
    {
        if (this->fieldNumber == fieldNumber)
        {
            kDebug() << "removing fieldNumber " << fieldNumber << endl;
            this->fieldNumber = -1;
        }
    }
    virtual void dump() const
    {
        kDebug() << "Condition: fieldNumber: " << fieldNumber << " value: " << value << endl;
    }

public:
    int fieldNumber;
    QString value; // Value?
    Comparison operation;
    Qt::CaseSensitivity caseSensitivity;
    Mode dataType;
};

Filter::And::And(const And& other)
    : AbstractCondition()
{
    for (int i = 0; i < other.list.count(); ++i)
    {
        if (!other.list[i])
            continue;
        else if (other.list[i]->type() == AbstractCondition::And)
            continue;
        else if (other.list[i]->type() == AbstractCondition::Or)
            list.append(new Filter::Or(*static_cast<Filter::Or*>(other.list[i])));
        else
            list.append(new Filter::Condition(*static_cast<Filter::Condition*>(other.list[i])));
    }
}

void Filter::And::loadOdf(const KoXmlElement& parent)
{
    KoXmlElement element;
    AbstractCondition* condition;
    forEachElement(element, parent)
    {
        if (element.namespaceURI() != KoXmlNS::table)
            continue;
        if (element.localName() == "filter-or")
            condition = new Filter::Or();
        else if (element.localName() == "filter-condition")
            condition = new Filter::Condition();
        else
            continue;
        condition->loadOdf(element);
    }
}

Filter::Or::Or(const Or& other)
    : AbstractCondition()
{
    for (int i = 0; i < other.list.count(); ++i)
    {
        if (!other.list[i])
            continue;
        else if (other.list[i]->type() == AbstractCondition::And)
            list.append(new Filter::And(*static_cast<Filter::And*>(other.list[i])));
        else if (other.list[i]->type() == AbstractCondition::Or)
            continue;
        else
            list.append(new Filter::Condition(*static_cast<Filter::Condition*>(other.list[i])));
    }
}

void Filter::Or::loadOdf(const KoXmlElement& parent)
{
    KoXmlElement element;
    AbstractCondition* condition;
    forEachElement(element, parent)
    {
        if (element.namespaceURI() != KoXmlNS::table)
            continue;
        if (element.localName() == "filter-and")
            condition = new Filter::And();
        else if (element.localName() == "filter-condition")
            condition = new Filter::Condition();
        else
            continue;
        condition->loadOdf(element);
    }
}


class Filter::Private
{
public:
    Private()
        : condition( 0 )
        , conditionSource(Self)
        , displayDuplicates(true)
    {
    }

    AbstractCondition* condition;
    Region targetRangeAddress;
    enum { Self, CellRange } conditionSource;
    Region conditionSourceRangeAddress;
    bool displayDuplicates;
};

Filter::Filter()
    : d(new Private)
{
}

Filter::Filter(const Filter& other)
    : d(new Private)
{
    if (!other.d->condition)
        d->condition = 0;
    else if (other.d->condition->type() == AbstractCondition::And)
        d->condition = new And(*static_cast<And*>(other.d->condition));
    else if (other.d->condition->type() == AbstractCondition::Or)
        d->condition = new Or(*static_cast<Or*>(other.d->condition));
    else
        d->condition = new Condition(*static_cast<Condition*>(other.d->condition));
    d->targetRangeAddress = other.d->targetRangeAddress;
    d->conditionSource = other.d->conditionSource;
    d->conditionSourceRangeAddress = other.d->conditionSourceRangeAddress;
    d->displayDuplicates = other.d->displayDuplicates;
}

Filter::~Filter()
{
    delete d->condition;
    delete d;
}

void Filter::addCondition(Composition composition,
                          int fieldNumber, Comparison comparison, const QString& value,
                          Qt::CaseSensitivity caseSensitivity, Mode mode)
{
    kDebug() << k_funcinfo << endl;
    Condition* condition = new Condition(fieldNumber, comparison, value, caseSensitivity, mode);
    if (!d->condition)
    {
        kDebug() << "no condition yet" << endl;
        d->condition = condition;
    }
    else if (composition == AndComposition)
    {
        kDebug() << "AndComposition" << endl;
        if (d->condition->type() == AbstractCondition::And)
        {
            static_cast<And*>(d->condition)->list.append(condition);
        }
        else
        {
            And* andComposition = new And();
            andComposition->list.append(d->condition);
            andComposition->list.append(condition);
            d->condition = andComposition;
        }
    }
    else // composition == OrComposition
    {
        if (d->condition->type() == AbstractCondition::Or)
        {
            static_cast<Or*>(d->condition)->list.append(condition);
        }
        else
        {
            Or* orComposition = new Or();
            orComposition->list.append(d->condition);
            orComposition->list.append(condition);
            d->condition = orComposition;
        }
    }
}

void Filter::removeConditions(int fieldNumber)
{
    if (fieldNumber == -1)
    {
        kDebug() << "removing all conditions" << endl;
        delete d->condition;
        d->condition = 0;
        return;
    }
    if (!d->condition)
        return;
    kDebug() << "removing condition for field " << fieldNumber << " from " << d->condition <<  endl;
    d->condition->removeConditions(fieldNumber);
    if (d->condition->isEmpty())
    {
        delete d->condition;
        d->condition = 0;
    }
}

bool Filter::isEmpty() const
{
    return d->condition ? d->condition->isEmpty() : true;
}

void Filter::apply(const Database* database) const
{
    Sheet* const sheet = (*database->range().constBegin())->sheet();
    const QRect range = database->range().lastRange();
    const int start = database->orientation() == Qt::Vertical ? range.top() : range.left();
    const int end = database->orientation() == Qt::Vertical ? range.bottom() : range.right();
    for (int i = start + 1; i <= end; ++i)
    {
        kDebug() << endl << "Checking column/row " << i << endl;
        if (database->orientation() == Qt::Vertical)
        {
            sheet->nonDefaultRowFormat(i)->setFiltered(d->condition ? !d->condition->evaluate(database, i) : false);
/*            if (d->condition->evaluate(database, i))
                kDebug() << "showing row " << i << endl;
            else
                kDebug() << "hiding row " << i << endl;*/
            sheet->emitHideRow();
        }
        else // database->orientation() == Qt::Horizontal
        {
            sheet->nonDefaultColumnFormat(i)->setFiltered(d->condition ? !d->condition->evaluate(database, i) : false);
            sheet->emitHideColumn();
        }
    }
}

void Filter::loadOdf(const KoXmlElement& element, Sheet* const sheet)
{
    if (element.hasAttributeNS(KoXmlNS::table, "target-range-address"))
    {
        const QString address = element.attributeNS(KoXmlNS::table, "target-range-address", QString());
        d->targetRangeAddress = Region(sheet->map(), address, sheet);
    }
    if (element.hasAttributeNS(KoXmlNS::table, "condition-source"))
    {
        if (element.attributeNS(KoXmlNS::table, "condition-source", "self") == "cell-range")
            d->conditionSource = Private::CellRange;
        else
            d->conditionSource = Private::Self;
    }
    if (element.hasAttributeNS(KoXmlNS::table, "condition-source-range-address"))
    {
        const QString address = element.attributeNS(KoXmlNS::table, "condition-source-range-address", QString());
        d->conditionSourceRangeAddress = Region(sheet->map(), address, sheet);
    }
    if (element.hasAttributeNS(KoXmlNS::table, "display-duplicates"))
    {
        if (element.attributeNS(KoXmlNS::table, "display-duplicates", "true") == "false")
            d->displayDuplicates = false;
        else
            d->displayDuplicates = true;
    }
    const KoXmlElement conditionElement = element.firstChild().toElement();
    if (conditionElement.isNull() || conditionElement.namespaceURI() != KoXmlNS::table)
        return;
    if (conditionElement.localName() == "filter-and")
        d->condition = new And();
    else if (conditionElement.localName() == "filter-or")
        d->condition = new Or();
    else if (conditionElement.localName() == "filter-condition")
        d->condition = new Condition();
    d->condition->loadOdf(conditionElement);
}

void Filter::saveOdf(KoXmlWriter& xmlWriter) const
{
    if (!d->condition)
        return;
    xmlWriter.startElement("table:filter");
    if (!d->targetRangeAddress.isEmpty())
    {
        const QString address = Oasis::encodeFormula(d->targetRangeAddress.name());
        xmlWriter.addAttribute("table:target-range-address", address);
    }
    if (d->conditionSource != Private::Self)
        xmlWriter.addAttribute("table:condition-source", "cell-range");
    if (!d->conditionSourceRangeAddress.isEmpty())
    {
        const QString address = Oasis::encodeFormula(d->conditionSourceRangeAddress.name());
        xmlWriter.addAttribute("table:condition-source-range-address", address);
    }
    if (!d->displayDuplicates)
        xmlWriter.addAttribute("table:display-duplicates", "false");
    d->condition->saveOdf(xmlWriter);
    xmlWriter.endElement();
}

void Filter::dump() const
{
    if (d->condition)
        d->condition->dump();
    else
        kDebug() << "Condition: 0" << endl;
}
