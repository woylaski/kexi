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

#include <kis_debug.h>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTreeView>

#include "kis_gmic_parser.h"

#include <Parameter.h>
#include <Command.h>
#include <Category.h>
#include "kis_gmic_widget.h"


KisGmicParser::KisGmicParser(const QString& filePath):m_fileName(filePath)
{

}

KisGmicParser::~KisGmicParser()
{

}

bool KisGmicParser::matchesRegExp(const QRegExp& regExp,const QString& line)
{
    int indexOfMatch = regExp.indexIn(line);
    int matchedLength = regExp.matchedLength();
    return (indexOfMatch == 0) && (matchedLength > 0);
}

bool KisGmicParser::isCommand(const QString& line)
{
    return matchesRegExp(COMMAND_NAME_RX, line);
}

bool KisGmicParser::isParameter(const QString& line)
{
    return matchesRegExp(PARAMETER_RX,line);
}

bool KisGmicParser::isCategory(const QString& line)
{
    return matchesRegExp(CATEGORY_NAME_RX,line);
}

QString KisGmicParser::parseCategoryName(const QString& line)
{
    QString result = line;
    return result.remove(0, GIMP_COMMENT.size()).trimmed();
}

Component* KisGmicParser::createFilterTree()
{
    QFile file(m_fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        dbgPlugins << "Can't open file " << m_fileName << file.errorString();
        return 0;
    }

    QTextStream in(&file);

    Category * rootCategory = new Category();
    rootCategory->setName("Filters");

    Command * command = 0;
    Category * category = rootCategory;
    int lineNum = 0;
    while(!in.atEnd()) {
        QString line = fetchLine(in, lineNum);

        if (line.startsWith(GIMP_COMMENT))
        {
            if (isCategory(line))
            {
                QString categoryName = parseCategoryName(line);

                if (categoryName.startsWith("_"))
                {
                    // root category
                    if (categoryName != "_")
                    {
                        category = new Category();
                        category->setName(categoryName.remove(0,1)); // remove _
                        rootCategory->add(category);
                    } else
                    {
                        // set current category to parent category
                        category = static_cast<Category*>(category->parent());
                    }
                }
                else
                {
                    // set current category as parent
                    Category * childCategory = new Category(category);
                    childCategory->setName(categoryName);
                    category->add(childCategory);
                    category = childCategory; // set current category to child
                }
            }
            else if (isCommand(line))
            {
                // dbgPlugins << "command" << line;
                command = new Command(category);
                command->processCommandName(line);
                category->add(command);
            }
            else if (isParameter(line))
            {
                if (command)
                {
                    QStringList block;
                    block.append(line);
                    bool parameterIsComplete = false;
                    int lines = 1;
                    while (!parameterIsComplete)
                    {
                        //dbgPlugins << "Line number" << lineNum;
                        parameterIsComplete = command->processParameter(block);
                        if (!parameterIsComplete)
                        {

                            QString anotherLine = fetchLine(in, lineNum);
                            if (!anotherLine.isNull())
                            {
                                block.append(anotherLine);
                                lines++;
                            }
                            else
                            {
                                warnPlugins << "We are and the end of the file unexpectedly"; // we are at the end of the file
                                break;
                            }
                        }
                        else if (lines > 1)
                        {
                            dbgPlugins << "At " << lineNum << " lines: " << lines << " multiline: " << block;
                        }
                    }
                }
                else
                {
                    dbgPlugins << "No command for given parameter, invalid gmic definition file";
                }
            }
            else if (line.startsWith(GIMP_COMMENT+"_"))
            {
                // TODO: do something with those translations
            }
            else
            {
                dbgPlugins << "IGNORING:" << line;
            }
        }
    }
    //command->debug();

    //rootCategory->print();
    file.close();

    return rootCategory;
}


QString KisGmicParser::fetchLine(QTextStream& input, int& lineCounter)
{
    if (!input.atEnd())
    {
        QString line = input.readLine();
        lineCounter++;
        return line;
    }
    return QString();
}
