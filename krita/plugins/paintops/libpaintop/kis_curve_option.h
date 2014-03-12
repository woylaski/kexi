/* This file is part of the KDE project
 * Copyright (C) 2008 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2011 Silvio Heinrich <plassy@web.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KIS_CURVE_OPTION_H
#define KIS_CURVE_OPTION_H

#include <QObject>
#include <QVector>

#include "kis_global.h"
#include "kis_paintop_option.h"
#include "kis_paint_information.h"
#include "krita_export.h"
#include "kis_dynamic_sensor.h"

class KisCurveWidget;
class Ui_WdgCurveOption;
class KisDynamicSensor;

/**
 * KisCurveOption is the base class for paintop options that are
 * defined through one or more curves.
 *
 * Note: it is NOT a KisPaintOpOption, even though the API is pretty similar!
 *
 */
class PAINTOP_EXPORT KisCurveOption
{
public:
    KisCurveOption(const QString& label,
                   const QString& name,
                   const QString& category,
                   bool checked,
                   qreal value = 1.0,
                   qreal min = 0.0,
                   qreal max = 1.0);

    virtual ~KisCurveOption();

    virtual void writeOptionSetting(KisPropertiesConfiguration* setting) const;
    virtual void readOptionSetting(const KisPropertiesConfiguration* setting);

    const QString& name() const;
    const QString& label() const;
    const QString& category() const;
    qreal minValue() const;
    qreal maxValue() const;
    qreal value() const;

    void resetAllSensors();
    KisDynamicSensor *sensor(const QString &sensorId, bool active) const;
    void replaceSensor(KisDynamicSensor *sensor);
    QList<KisDynamicSensor*> sensors();
    QList<KisDynamicSensor*> activeSensors() const;

    bool isCheckable();
    bool isChecked() const;
    bool isCurveUsed() const;
    bool isSameCurveUsed() const;

    void setSeparateCurveValue(bool separateCurveValue);

    void setChecked(bool checked);
    void setCurveUsed(bool useCurve);
    void setCurve(const QString &sensorId, bool useSameCurve, const KisCubicCurve &curve);
    void setValue(qreal value);

    const QString& minimumLabel() const;
    const QString& maximumLabel() const;

    /**
     * Uses the curves set on the sensors to compute a single
     * double value that can control the parameters of a brush.
     */
    double computeValue(const KisPaintInformation& info) const;

protected:

    void setMinimumLabel(const QString& _label);
    void setMaximumLabel(const QString& _label);
    void setValueRange(qreal min, qreal max);

    /**
     * Read the option using the prefix in argument
     */
    void readNamedOptionSetting(const QString& prefix, const KisPropertiesConfiguration* setting);

    QString m_name;
    QString m_label;
    QString m_category;

    bool m_checkable;
    bool m_checked;
    bool m_useCurve;
    bool m_useSameCurve;
    bool m_separateCurveValue;

    QMap<QString, KisDynamicSensor*> m_sensorMap;
    QMap<QString, KisCubicCurve> m_curveCache;

private:

    qreal m_value;
    qreal m_minValue;
    qreal m_maxValue;
    QString m_minimumLabel;
    QString m_maximumLabel;

};

#endif
