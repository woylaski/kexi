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
#include "kis_curve_option.h"

#include <QDomNode>

KisCurveOption::KisCurveOption(const QString & label, const QString& name, const QString & category,
                               bool checked, qreal value, qreal min, qreal max)
    : m_name(name)
    , m_label(label)
    , m_category(category)
    , m_checkable(true)
    , m_checked(checked)
    , m_useCurve(true)
    , m_useSameCurve(true)
    ,    m_separateCurveValue(false)
{
    foreach (const KoID& sensorId, KisDynamicSensor::sensorsIds()) {
        KisDynamicSensor *sensor = KisDynamicSensor::id2Sensor(sensorId);
        sensor->setActive(false);
        replaceSensor(sensor);
    }
    m_sensorMap[PressureId.id()]->setActive(true);

    setMinimumLabel(i18n("0.0"));
    setMaximumLabel(i18n("1.0"));
    setValueRange(min, max);
    setValue(value);
}

KisCurveOption::~KisCurveOption()
{
    QList<KisDynamicSensor*> sensors = m_sensorMap.values();
    m_sensorMap.clear();
    qDeleteAll(sensors);
}

const QString& KisCurveOption::name() const
{
    return m_name;
}

const QString & KisCurveOption::label() const
{
    return m_label;
}

const QString& KisCurveOption::category() const
{
    return m_category;
}

qreal KisCurveOption::minValue() const
{
    return m_minValue;
}

qreal KisCurveOption::maxValue() const
{
    return m_maxValue;
}

qreal KisCurveOption::value() const
{
    return m_value;
}

void KisCurveOption::resetAllSensors()
{
    foreach (KisDynamicSensor *sensor, m_sensorMap.values()) {
        if (sensor->isActive()) {
            sensor->reset();
        }
    }
}

void KisCurveOption::writeOptionSetting(KisPropertiesConfiguration* setting) const
{
    if (m_checkable) {
        setting->setProperty("Pressure" + m_name, isChecked());
    }

    if (activeSensors().size() == 1) {
        setting->setProperty(m_name + "Sensor", activeSensors().first()->toXML());
    }
    else {
        QDomDocument doc = QDomDocument("params");
        QDomElement root = doc.createElement("params");
        doc.appendChild(root);
        root.setAttribute("id", "sensorslist");

        foreach (KisDynamicSensor* sensor, activeSensors()) {
            QDomElement childelt = doc.createElement("ChildSensor");
            sensor->toXML(doc, childelt);
            root.appendChild(childelt);
        }
        setting->setProperty(m_name + "Sensor", doc.toString());
    }
    setting->setProperty(m_name + "UseCurve", m_useCurve);
    setting->setProperty(m_name + "UseSameCurve", m_useSameCurve);
    setting->setProperty(m_name + "Value", m_value);

}

void KisCurveOption::readOptionSetting(const KisPropertiesConfiguration* setting)
{
    m_curveCache.clear();
    readNamedOptionSetting(m_name, setting);
}

void KisCurveOption::readNamedOptionSetting(const QString& prefix, const KisPropertiesConfiguration* setting)
{
    //qDebug() << "readNamedOptionSetting" << prefix;
    setting->dump();

    if (m_checkable) {
        setChecked(setting->getBool("Pressure" + prefix, false));
    }
    //qDebug() << "\tPressure" + prefix << isChecked();

    m_sensorMap.clear();

    // Replace all sensors with the inactive defaults
    foreach(const KoID& sensorId, KisDynamicSensor::sensorsIds()) {
        replaceSensor(KisDynamicSensor::id2Sensor(sensorId));
    }

    QString sensorDefinition = setting->getString(prefix + "Sensor");
    if (!sensorDefinition.contains("sensorslist")) {
        KisDynamicSensor *s = KisDynamicSensor::createFromXML(sensorDefinition);
        if (s) {
            replaceSensor(s);
            s->setActive(true);
            //qDebug() << "\tsingle sensor" << s->id() << s->isActive() << "added";
        }
    }
    else {
        QDomDocument doc;
        doc.setContent(sensorDefinition);
        QDomElement elt = doc.documentElement();
        QDomNode node = elt.firstChild();
        while (!node.isNull()) {
            if (node.isElement())  {
                QDomElement childelt = node.toElement();
                if (childelt.tagName() == "ChildSensor") {
                    KisDynamicSensor* s = KisDynamicSensor::createFromXML(childelt);
                    if (s) {
                        replaceSensor(s);
                        s->setActive(true);
                        //qDebug() << "\tchild sensor" << s->id() << s->isActive() << "added";
                    }
                }
            }
            node = node.nextSibling();
        }
    }

    // Only load the old curve format if the curve wasn't saved by the sensor
    // This will give every sensor the same curve.
    //qDebug() << ">>>>>>>>>>>" << prefix + "Sensor" << setting->getString(prefix + "Sensor");
    if (!setting->getString(prefix + "Sensor").contains("curve")) {
        //qDebug() << "\told format";
        if (setting->getBool("Custom" + prefix, false)) {
            foreach(KisDynamicSensor *s, m_sensorMap.values()) {
                s->setCurve(setting->getCubicCurve("Curve" + prefix));
            }
        }
    }

    // At least one sensor needs to be active
    if (activeSensors().size() == 0) {
        m_sensorMap[PressureId.id()]->setActive(true);
    }

    m_value = setting->getDouble(m_name + "Value", m_maxValue);
    //qDebug() << "\t" + m_name + "Value" << m_value;

    m_useCurve = setting->getBool(m_name + "UseCurve", true);
    //qDebug() << "\t" + m_name + "UseCurve" << m_useSameCurve;

    m_useSameCurve = setting->getBool(m_name + "UseSameCurve", true);
    //qDebug() << "\t" + m_name + "UseSameCurve" << m_useSameCurve;

    //qDebug() << "-----------------";
}

void KisCurveOption::replaceSensor(KisDynamicSensor *s)
{
    Q_ASSERT(s);

    if (m_sensorMap.contains(s->id()) && m_sensorMap[s->id()] != s) {
        delete m_sensorMap.take(s->id());
    }
    m_sensorMap[s->id()] = s;
}

KisDynamicSensor *KisCurveOption::sensor(const QString& sensorId, bool active) const
{
    if (m_sensorMap.contains(sensorId)) {
        if (!active) {
            return m_sensorMap[sensorId];
        }
        else {
             if (m_sensorMap[sensorId]->isActive()) {
                 return m_sensorMap[sensorId];
             }
        }
    }
    return 0;
}

bool KisCurveOption::isCurveUsed() const
{
    return m_useCurve;
}

bool KisCurveOption::isSameCurveUsed() const
{
    return m_useSameCurve;
}

void KisCurveOption::setSeparateCurveValue(bool separateCurveValue)
{
    m_separateCurveValue = separateCurveValue;
}

bool KisCurveOption::isCheckable()
{
    return m_checkable;
}

bool KisCurveOption::isChecked() const
{
    return m_checked;
}

void KisCurveOption::setChecked(bool checked)
{
    m_checked = checked;
}

void KisCurveOption::setCurveUsed(bool useCurve)
{
    m_useCurve = useCurve;
}

void KisCurveOption::setCurve(const QString &sensorId, bool useSameCurve, const KisCubicCurve &curve)
{
    // No switch in state, don't mess with the cache
    if (useSameCurve == m_useSameCurve) {
        if (useSameCurve) {
            foreach(KisDynamicSensor *s, m_sensorMap.values()) {
                s->setCurve(curve);
            }
        }
        else {
            KisDynamicSensor *s = sensor(sensorId, false);
            if (s) {
                s->setCurve(curve);
            }

        }
    }
    else {
        // moving from not use same curve to use same curve: backup the custom curves
        if (!m_useSameCurve && useSameCurve) {
            // Copy the custom curves to the cache and set the new curve on all sensors, active or not
            m_curveCache.clear();
            foreach(KisDynamicSensor *s, m_sensorMap.values()) {
                m_curveCache[s->id()] = s->curve();
                s->setCurve(curve);
            }
        }
        else { //if (m_useSameCurve && !useSameCurve)
            // Restore the cached curves
            KisDynamicSensor *s = 0;
            foreach(QString id, m_curveCache.keys()) {
                if (m_sensorMap.contains(id)) {
                    s = m_sensorMap[id];
                }
                else {
                    s = KisDynamicSensor::id2Sensor(id);
                }
                s->setCurve(m_curveCache[id]);
                m_sensorMap[id] = s;
            }
            s = 0;
            // And set the current sensor to the current curve
            if (!m_sensorMap.contains(sensorId)) {
                s = KisDynamicSensor::id2Sensor(sensorId);
            }
            if (s) {
                s->setCurve(curve);
                s->setCurve(m_curveCache[sensorId]);
            }

        }
        m_useSameCurve = useSameCurve;
    }
}

const QString& KisCurveOption::minimumLabel() const
{
    return m_minimumLabel;
}

const QString& KisCurveOption::maximumLabel() const
{
    return m_maximumLabel;
}

void KisCurveOption::setMinimumLabel(const QString& _label)
{
    m_minimumLabel = _label;
}

void KisCurveOption::setMaximumLabel(const QString& _label)
{
    m_maximumLabel = _label;
}

void KisCurveOption::setValueRange(qreal min, qreal max)
{
    m_minValue = qMin(min, max);
    m_maxValue = qMax(min, max);
}

void KisCurveOption::setValue(qreal value)
{
    m_value = qBound(m_minValue, value, m_maxValue);
}

double KisCurveOption::computeValue(const KisPaintInformation& info) const
{
    if (!m_useCurve) {
        if (m_separateCurveValue) {
            return 1.0;
        }
        else {
            return m_value;
        }
    }
    else {
        qreal t = 1.0;
        foreach (KisDynamicSensor* s, m_sensorMap.values()) {
            ////qDebug() << "\tTesting" << s->name() << s->isActive();
            if (s->isActive()) {
                t *= s->parameter(info);
            }
        }
        if (m_separateCurveValue) {
            return t;
        }
        else {
            return m_minValue + (m_value - m_minValue) * t;
        }
    }
}

QList<KisDynamicSensor*> KisCurveOption::sensors()
{
    QList<KisDynamicSensor*> sensorList;
    foreach(KisDynamicSensor* sensor, m_sensorMap.values()) {
        if (sensor->isActive()) {
            sensorList << sensor;
        }
    }

    //qDebug() << "ID" << name() << "has" <<  m_sensorMap.count() << "Sensors of which" << sensorList.count() << "are active.";
    return m_sensorMap.values();
}

QList<KisDynamicSensor*> KisCurveOption::activeSensors() const
{
    QList<KisDynamicSensor*> sensorList;
    foreach(KisDynamicSensor* sensor, m_sensorMap.values()) {
        if (sensor->isActive()) {
            sensorList << sensor;
        }
    }
    //qDebug() << "ID" << name() << "has" <<  m_sensorMap.count() << "Sensors of which" << sensorList.count() << "are active.";
    return sensorList;
}
