/*
 * Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <Parameter.h>
#include <QString>
#include <QStringList>

#include <kis_debug.h>

Parameter::Parameter(const QString& name, bool updatePreview)
    :m_name(name),
     m_type(INVALID_P),
     m_updatePreview(updatePreview)
{

}

QString Parameter::toString()
{
    return "INVALID";
}

void Parameter::parseValues(const QString& typeDefinition)
{
    Q_UNUSED(typeDefinition);
}

QString Parameter::extractValues(const QString& typeDefinition)
{
    QString currentType = PARAMETER_NAMES[m_type];
    Q_ASSERT(typeDefinition.startsWith(currentType));

    QString onlyValues = typeDefinition;
    onlyValues = onlyValues.remove(0, currentType.size()).trimmed();

    // drop first and last character : '(1)' or '{1}' or '[1]'
    onlyValues = onlyValues.mid(1, onlyValues.size() - 2);
    return onlyValues;
}

QStringList Parameter::getValues(const QString& typeDefinition)
{
    QString onlyValues= extractValues(typeDefinition);
    QStringList result = onlyValues.split(",");
    return result;
}

bool Parameter::isPresentationalOnly() const
{
    if ((m_type == NOTE_P) || (m_type == SEPARATOR_P) || (m_type == LINK_P))
    {
        return true;
    }
    return false;
}

 QString Parameter::stripQuotes(const QString& str)
{
    if (str.startsWith("\"") && str.endsWith("\""))
    {
        return str.mid(1, str.size() - 2);
    }
    return str;
}


QString Parameter::value() const
{
    return QString();
}


void Parameter::setValue(const QString& value)
{
    Q_UNUSED(value);
    dbgPlugins << "Not implemented for type : " << PARAMETER_NAMES[m_type];
}



/**************************
    == FloatParameter ==
 ***************************/

FloatParameter::FloatParameter(const QString& name, bool updatePreview): Parameter(name,updatePreview)
{
    m_type = FLOAT_P;
}

// e.g. float(0,0,5) or float[0,0,5] or float{0,0,5}
void FloatParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    bool isOk = true;

    m_value = m_defaultValue = values.at(0).toFloat(&isOk);

    if (!isOk)
    {
        dbgPlugins << "Incorect type definition: " << typeDefinition;
    }
    Q_ASSERT(isOk);


    m_minValue = values.at(1).toFloat(&isOk);
    Q_ASSERT(isOk);
    m_maxValue = values.at(2).toFloat(&isOk);
    Q_ASSERT(isOk);
}


QString FloatParameter::value() const
{
    return QString::number(m_value);
}


void FloatParameter::setValue(const QString& value)
{
    bool isOk = true;
    float floatValue = value.toFloat(&isOk);
    if (isOk)
    {
        m_value = floatValue;
    }
}


QString FloatParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(PARAMETER_NAMES[m_type]+";");
    result.append(QString::number(m_defaultValue)+";");
    result.append(QString::number(m_minValue)+";");
    result.append(QString::number(m_maxValue)+";");
    return result;
}


void FloatParameter::reset()
{
    m_value = m_defaultValue;
}


/**************************
    == IntParameter ==
 ***************************/

IntParameter::IntParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = INT_P;
}

void IntParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    bool isOk = true;

    m_value = m_defaultValue = values.at(0).toInt(&isOk);
    Q_ASSERT(isOk);
    m_minValue = values.at(1).toInt(&isOk);
    Q_ASSERT(isOk);
    m_maxValue = values.at(2).toInt(&isOk);
    Q_ASSERT(isOk);
}


QString IntParameter::value() const
{
    return QString::number(m_value);
}


void IntParameter::setValue(const QString& value)
{
    bool isOk = true;
    int intValue = value.toInt(&isOk);
    if (isOk)
    {
        m_value = intValue;
    }
}


QString IntParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(PARAMETER_NAMES[m_type]+";");
    result.append(QString::number(m_defaultValue)+";");
    result.append(QString::number(m_minValue)+";");
    result.append(QString::number(m_maxValue)+";");
    return result;
}

void IntParameter::reset()
{
    m_value = m_defaultValue;
}

/**************************
    == SeparatorParameter ==
 ***************************/

SeparatorParameter::SeparatorParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = SEPARATOR_P;
}

void SeparatorParameter::parseValues(const QString& typeDefinition)
{
    Q_UNUSED(typeDefinition);
}

QString SeparatorParameter::toString()
{
    QString result;
    result.append(m_name+";");
    return result;
}

/**************************
    == ChoiceParameter ==
 ***************************/

ChoiceParameter::ChoiceParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = CHOICE_P;
}

void ChoiceParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    if (values.isEmpty())
    {
        dbgPlugins << "Wrong gmic_def" << typeDefinition << " not parsed correctly";
        return;
    }

    // choice(4,"Dots","Wireframe","Flat","Flat shaded","Gouraud","Phong")
    QString firstItem = values.at(0);
    bool isInteger = false;
    m_value = m_defaultValue = firstItem.toInt(&isInteger);
    if (isInteger)
    {
        // throw number out of choices
        values.takeFirst();
    }
    else
    {
        m_value = m_defaultValue = 0;
    }

    m_choices = values;

    for (int i = 0; i < values.size(); i++)
    {
        m_choices[i] = stripQuotes(m_choices[i].trimmed());
    }
}

QString ChoiceParameter::value() const
{
    return QString::number(m_value);
}

void ChoiceParameter::setValue(const QString& value)
{
    bool isInt = true;
    int choiceIndex = value.toInt(&isInt);
    if (isInt)
    {
        setIndex(choiceIndex);
    }else
    {
        setIndex(m_choices.indexOf(value));
    }
}

void ChoiceParameter::setIndex(int i)
{
    if (i >= 0 && i < m_choices.size())
    {
        m_value = i;
    }
}


QString ChoiceParameter::toString()
{
    QString result;
    result.append(m_name+";"+QString::number(m_defaultValue)+";"+QString::number(m_value));
    foreach (QString choice, m_choices)
    {
        result.append(choice+";");
    }
    return result;
}


void ChoiceParameter::reset()
{
    m_value = m_defaultValue;
}

/**************************
    == NoteParameter ==
 ***************************/

NoteParameter::NoteParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = NOTE_P;
}

void NoteParameter::parseValues(const QString& typeDefinition)
{
    QString values = extractValues(typeDefinition);
    m_label = stripQuotes(values);
}

QString NoteParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_label+";");
    return result;
}

/**************************
    == BoolParameter ==
 ***************************/

BoolParameter::BoolParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = BOOL_P;
}


void BoolParameter::initValue(bool value)
{
    m_value = m_defaultValue = value;
}

void BoolParameter::parseValues(const QString& typeDefinition)
{
    QString currentType = PARAMETER_NAMES[m_type];
    Q_ASSERT(typeDefinition.startsWith(currentType));

    QStringList values = getValues(typeDefinition);

    QString boolValue = values.at(0);
    if (boolValue == "0" || boolValue == "false")
    {
        initValue(false);
    }
    else if (boolValue == "1" || boolValue == "true")
    {
        initValue(true);
    } else
    {
        dbgPlugins << "Invalid bool value, assuming true " << m_name << ":" << boolValue;
        initValue(true);
    }
}

QString BoolParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_value+";");
    return result;
}

QString BoolParameter::value() const
{
    if (m_value)
    {
        return QString("1");
    }
    return QString("0");
}

void BoolParameter::reset()
{
    m_value = m_defaultValue;
}


/**************************
    == ColorParameter ==
 ***************************/

ColorParameter::ColorParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview),m_hasAlpha(true)
{
    m_type = COLOR_P;
}

void ColorParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    Q_ASSERT(values.size() >= 3);

    bool isOk = true;
    int r = values.at(0).toInt(&isOk);
    int g = values.at(1).toInt(&isOk);
    int b = values.at(2).toInt(&isOk);
    int a = 255;
    if (values.size() == 4)
    {
        a = values.at(2).toInt(&isOk);
        m_hasAlpha = true;
    }
    else
    {
        m_hasAlpha = false;
    }
    m_value.setRgb(r,g,b,a);
    m_defaultValue = m_value;
}

QString ColorParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_value.name() + ";");
    return result;
}

QString ColorParameter::value() const
{
    QString result =     QString::number(m_value.red()) + ","
                        +QString::number(m_value.green()) + ","
                        +QString::number(m_value.blue());
    if (m_hasAlpha)
    {
        result  += "," + QString::number(m_value.alpha());
    }
    return result;
}

void ColorParameter::reset()
{
    m_value = m_defaultValue;
}

/**************************
    == LinkParameter ==
 ***************************/

LinkParameter::LinkParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = LINK_P;
}

void LinkParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);


    QString linkAddress;
    QString linkText;
    if (values.size() == 1)
    {
        linkAddress = values.at(0);
        linkText = stripQuotes(values.at(0));

    }
    else if (values.size() == 2)
    {
        linkAddress = values.at(1);
        linkText = stripQuotes(values.at(0));
    }
    else if (values.size() == 3)
    {
        //TODO: ignored aligment at values.at(0) , mostly 0
        linkAddress = values.at(2);
        linkText = stripQuotes(values.at(1));
    }
    else
    {
        dbgPlugins << "Wrong format of link parameter";
        return;
    }

    QString link = "<a href=%1>%2</a>";
    m_link = link.arg(linkAddress).arg(linkText);
}

QString LinkParameter::toString()
{
    return m_link;
}

/**************************
    == TextParameter ==
 ***************************/

TextParameter::TextParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview), m_multiline(false)
{
    m_type = TEXT_P;
}

void TextParameter::parseValues(const QString& typeDefinition)
{
    QString currentType = PARAMETER_NAMES[m_type];
    Q_ASSERT(typeDefinition.startsWith(currentType));

    // get rid of '(', '{' and '['
    QString onlyValues = typeDefinition;
    onlyValues = onlyValues.remove(0, currentType.size() + 1);
    onlyValues.chop(1);
    QStringList values = onlyValues.split(",");

    if (values.size() == 1)
    {
        m_value = values.at(0);
    }
    else
    {
        bool isOk = true;
        int multilineFlag = values.at(0).toInt(&isOk);
        if (isOk && (values.size() == 2))
        {
            m_multiline = (multilineFlag == 1);
            m_value = values.at(1);
        }
        // e.g typeDefinition is text("0,1,0;1,-4,1;0,1,0")
        // e.g typeDefinition is text(1, "0,1,0;1,-4,1;0,1,0")
        // e.g. text("1,1")
        else
        {
            // flag is there
            if (isOk)
            {
                m_multiline = (multilineFlag == 1);
                m_value = onlyValues.mid(onlyValues.indexOf(","));
            }
            else
            {
                m_value = onlyValues;
            }
        }
    }

    // remove first and last "
    m_value = stripQuotes(m_value);
    m_defaultValue = m_value;
}

QString TextParameter::value() const
{
    return m_value;
}

QString TextParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_value + ";");
    result.append(m_multiline + ";");
    return result;
}

void TextParameter::reset()
{
    m_defaultValue = m_value;
}

/**************************
    == FolderParameter ==
 ***************************/

FolderParameter::FolderParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = FOLDER_P;
}


void FolderParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    if (!values.isEmpty())
    {
        m_folderPath = values.at(0);
    }
    m_defaultFolderPath = m_folderPath;
}


QString FolderParameter::value() const
{
    return m_folderPath;
}


QString FolderParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_folderPath + ";");
    return result;
}

void FolderParameter::reset()
{
    m_folderPath = m_defaultFolderPath;
}

/**************************
    == FileParameter ==
 ***************************/

FileParameter::FileParameter(const QString& name, bool updatePreview): Parameter(name, updatePreview)
{
    m_type = FILE_P;
}

void FileParameter::parseValues(const QString& typeDefinition)
{
    QStringList values = getValues(typeDefinition);
    if (!values.isEmpty())
    {
        m_filePath = values.at(0);
    }
    m_defaultFilePath = m_filePath;

}

QString FileParameter::value() const
{
    return m_filePath;
}

QString FileParameter::toString()
{
    QString result;
    result.append(m_name+";");
    result.append(m_filePath + ";");
    return result;
}

void FileParameter::reset()
{
    m_filePath = m_defaultFilePath;
}
