/* This file is part of the KDE project
   Copyright (C) 2009 Thorsten Zachmann <zachmann@kde.org>
   Copyright (C) 2009 Johannes Simon <johannes.simon@gmail.com>
   Copyright (C) 2010,2011 Jan Hambrecht <jaham@gmx.net>
   Copyright 2012 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "KoOdfWorkaround.h"

#include "KoShapeLoadingContext.h"
#include "KoShape.h"
#include "KoPathShape.h"
#include "KoColorBackground.h"
#include <KoOdfLoadingContext.h>
#include <KoXmlReader.h>
#include <KoXmlNS.h>
#include <KoStyleStack.h>
#include <KoUnit.h>

#include <QPen>
#include <QColor>

#include <kdebug.h>

static bool s_workaroundPresentationPlaceholderBug = false;

void KoOdfWorkaround::fixPenWidth(QPen & pen, KoShapeLoadingContext &context)
{
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice && pen.widthF() == 0.0) {
        pen.setWidthF(0.5);
        kDebug(30003) << "Work around OO bug with pen width 0";
    }
}

void KoOdfWorkaround::fixEnhancedPath(QString & path, const KoXmlElement &element, KoShapeLoadingContext &context)
{
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
        if (path.isEmpty() && element.attributeNS(KoXmlNS::draw, "type", "") == "ellipse") {
            path = "U 10800 10800 10800 10800 0 360 Z N";
        }
    }
}

void KoOdfWorkaround::fixEnhancedPathPolarHandlePosition(QString &position, const KoXmlElement &element, KoShapeLoadingContext &context)
{
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
        if (element.hasAttributeNS(KoXmlNS::draw, "handle-polar")) {
            QStringList tokens = position.simplified().split(' ');
            if (tokens.count() == 2) {
                position = tokens[1] + ' ' + tokens[0];
            }
        }
    }
}

QColor KoOdfWorkaround::fixMissingFillColor(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    // Default to an invalid color
    QColor color;

    if (element.prefix() == "chart") {
        KoStyleStack &styleStack = context.odfLoadingContext().styleStack();
        styleStack.save();

        bool hasStyle = element.hasAttributeNS(KoXmlNS::chart, "style-name");
        if (hasStyle) {
            context.odfLoadingContext().fillStyleStack(element, KoXmlNS::chart, "style-name", "chart");
            styleStack.setTypeProperties("graphic");
        }

        if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
            if (hasStyle && !styleStack.hasProperty(KoXmlNS::draw, "fill") &&
                             styleStack.hasProperty(KoXmlNS::draw, "fill-color")) {
                color = QColor(styleStack.property(KoXmlNS::draw, "fill-color"));
            } else if (!hasStyle || (!styleStack.hasProperty(KoXmlNS::draw, "fill")
                                    && !styleStack.hasProperty(KoXmlNS::draw, "fill-color"))) {
                KoXmlElement plotAreaElement = element.parentNode().toElement();
                KoXmlElement chartElement = plotAreaElement.parentNode().toElement();

                if (element.tagName() == "wall") {
                    if (chartElement.hasAttributeNS(KoXmlNS::chart, "class")) {
                        QString chartType = chartElement.attributeNS(KoXmlNS::chart, "class");
                        // TODO: Check what default backgrounds for surface, stock and gantt charts are
                        if (chartType == "chart:line" ||
                             chartType == "chart:area" ||
                             chartType == "chart:bar" ||
                             chartType == "chart:scatter")
                        color = QColor(0xe0e0e0);
                    }
                } else if (element.tagName() == "series") {
                    if (chartElement.hasAttributeNS(KoXmlNS::chart, "class")) {
                        QString chartType = chartElement.attributeNS(KoXmlNS::chart, "class");
                        // TODO: Check what default backgrounds for surface, stock and gantt charts are
                        if (chartType == "chart:area" ||
                             chartType == "chart:bar")
                            color = QColor(0x99ccff);
                    }
                }
                else if (element.tagName() == "chart")
                    color = QColor(0xffffff);
            }
        }

        styleStack.restore();
    }

    return color;
}

bool KoOdfWorkaround::fixMissingStroke(QPen &pen, const KoXmlElement &element, KoShapeLoadingContext &context, const KoShape *shape)
{
    bool fixed = false;

    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
        KoStyleStack &styleStack = context.odfLoadingContext().styleStack();
        if (element.prefix() == "chart") {
            styleStack.save();

            bool hasStyle = element.hasAttributeNS(KoXmlNS::chart, "style-name");
            if (hasStyle) {
                context.odfLoadingContext().fillStyleStack(element, KoXmlNS::chart, "style-name", "chart");
                styleStack.setTypeProperties("graphic");
            }

            if (hasStyle && styleStack.hasProperty(KoXmlNS::draw, "stroke") &&
                            !styleStack.hasProperty(KoXmlNS::svg, "stroke-color")) {
                fixed = true;
                pen.setColor(Qt::black);
            } else if (!hasStyle) {
                KoXmlElement plotAreaElement = element.parentNode().toElement();
                KoXmlElement chartElement = plotAreaElement.parentNode().toElement();

                if (element.tagName() == "series") {
                    QString chartType = chartElement.attributeNS(KoXmlNS::chart, "class");
                    if (!chartType.isEmpty()) {
                        // TODO: Check what default backgrounds for surface, stock and gantt charts are
                        if (chartType == "chart:line" ||
                             chartType == "chart:scatter") {
                            fixed = true;
                            pen = QPen(0x99ccff);
                        }
                    }
                } else if (element.tagName() == "legend") {
                    fixed = true;
                    pen = QPen(Qt::black);
                }
            }
            styleStack.restore();
        }
        else {
            const KoPathShape *pathShape = dynamic_cast<const KoPathShape*>(shape);
            if (pathShape) {
                const QString strokeColor(styleStack.property(KoXmlNS::svg, "stroke-color"));
                if (strokeColor.isEmpty()) {
                    pen.setColor(Qt::black);
                } else {
                    pen.setColor(strokeColor);
                }
                fixed = true;
            }
        }
    }

    return fixed;
}

bool KoOdfWorkaround::fixMissingStyle_DisplayLabel(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    Q_UNUSED(element);
    // If no axis style is specified, OpenOffice.org hides the axis' data labels
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice)
        return false;

    // In all other cases, they're visible
    return true;
}

void KoOdfWorkaround::setFixPresentationPlaceholder(bool fix, KoShapeLoadingContext &context)
{
    KoOdfLoadingContext::GeneratorType type(context.odfLoadingContext().generatorType());
    if (type == KoOdfLoadingContext::OpenOffice || type == KoOdfLoadingContext::MicrosoftOffice) {
        s_workaroundPresentationPlaceholderBug = fix;
    }
}

bool KoOdfWorkaround::fixPresentationPlaceholder()
{
    return s_workaroundPresentationPlaceholderBug;
}

void KoOdfWorkaround::fixPresentationPlaceholder(KoShape *shape)
{
    if (s_workaroundPresentationPlaceholderBug && !shape->hasAdditionalAttribute("presentation:placeholder")) {
        shape->setAdditionalAttribute("presentation:placeholder", "true");
    }
}

QSharedPointer<KoColorBackground> KoOdfWorkaround::fixBackgroundColor(const KoShape *shape, KoShapeLoadingContext &context)
{
    QSharedPointer<KoColorBackground> colorBackground;
    KoOdfLoadingContext &odfContext = context.odfLoadingContext();
    if (odfContext.generatorType() == KoOdfLoadingContext::OpenOffice) {
        const KoPathShape *pathShape = dynamic_cast<const KoPathShape*>(shape);
        //check shape type
        if (pathShape) {
            KoStyleStack &styleStack = odfContext.styleStack();
            const QString color(styleStack.property(KoXmlNS::draw, "fill-color"));
            if (color.isEmpty()) {
                colorBackground = QSharedPointer<KoColorBackground>(new KoColorBackground(QColor(153, 204, 255)));
            } else {
                colorBackground = QSharedPointer<KoColorBackground>(new KoColorBackground(color));
            }
        }
    }
    return colorBackground;
}

void KoOdfWorkaround::fixGluePointPosition(QString &positionString, KoShapeLoadingContext &context)
{
    KoOdfLoadingContext::GeneratorType type(context.odfLoadingContext().generatorType());
    if (type == KoOdfLoadingContext::OpenOffice && !positionString.endsWith('%')) {
        const qreal pos = KoUnit::parseValue(positionString);
        positionString = QString("%1%%").arg(KoUnit::toMillimeter(pos));
    }
}

void KoOdfWorkaround::fixMissingFillRule(Qt::FillRule& fillRule, KoShapeLoadingContext& context)
{
    if ((context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice)) {
        fillRule = Qt::OddEvenFill;
    }
}

bool KoOdfWorkaround::fixAutoGrow(KoTextShapeDataBase::ResizeMethod method, KoShapeLoadingContext &context)
{
    bool fix = false;
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
        if (method == KoTextShapeDataBase::AutoGrowWidth || method == KoTextShapeDataBase::AutoGrowHeight || method == KoTextShapeDataBase::AutoGrowWidthAndHeight) {
            fix = true;
        }
    }
    return fix;
}

bool KoOdfWorkaround::fixEllipse(const QString &kind, KoShapeLoadingContext &context)
{
    bool radiusGiven = false;
    if (context.odfLoadingContext().generatorType() == KoOdfLoadingContext::OpenOffice) {
        if (kind == "section" || kind == "arc") {
            radiusGiven = true;
        }
    }
    return radiusGiven;
}

void KoOdfWorkaround::fixBadFormulaHiddenForStyleCellProtect(QString& value)
{
    if (value.endsWith(QLatin1String("Formula.hidden"))) {
        const int length = value.length();
        value[length-14] = QLatin1Char('f');
        value[length-7] = QLatin1Char('-');
    }
}

void KoOdfWorkaround::fixBadDateForTextTime(QString &value)
{
    if (value.startsWith(QLatin1String("0-00-00T"))) {
        value.remove(0, 8);
    }
}

void KoOdfWorkaround::fixClipRectOffsetValuesString(QString &offsetValuesString)
{
    if (! offsetValuesString.contains(QLatin1Char(','))) {
        // assumes no spaces existing between values and units
        offsetValuesString = offsetValuesString.simplified().replace(QLatin1Char(' '), QLatin1Char(','));
    }
}

QString KoOdfWorkaround::fixTableTemplateName(const KoXmlElement &e)
{
    return e.attributeNS(KoXmlNS::text, "style-name", QString());
}

QString KoOdfWorkaround::fixTableTemplateCellStyleName(const KoXmlElement &e)
{
    return e.attributeNS(KoXmlNS::text, "style-name", QString());
}
