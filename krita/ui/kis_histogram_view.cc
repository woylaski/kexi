/*
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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

#include "kis_histogram_view.h"

#include <math.h>

#include <QPainter>
#include <QPixmap>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollBar>
#include <QMouseEvent>
#include <QFrame>
#include <QBrush>
#include <QLinearGradient>
#include <QImage>

#include <kis_debug.h>

#include "KoChannelInfo.h"
#include "KoBasicHistogramProducers.h"
#include "KoColorSpace.h"

#include "kis_histogram.h"
#include "kis_global.h"
#include "kis_types.h"
#include "kis_layer.h"

#include "kis_paint_device.h"

KisHistogramView::KisHistogramView(QWidget *parent, const char *name, Qt::WFlags f)
        : QLabel(parent, f)
{
    setObjectName(name);
    // This is needed until we can computationally scale it well. Until then, this is needed
    // And when we have it, it won't hurt to have it around
    setScaledContents(true);
    setFrameShape(QFrame::Box); // Draw a box around ourselves
}

KisHistogramView::~KisHistogramView()
{
}

void KisHistogramView::setPaintDevice(KisPaintDeviceSP dev, const QRect &bounds)
{
    m_cs = dev->colorSpace();

    setChannels(); // Sets m_currentProducer to the first in the list

    if (!m_currentProducer)
        return;

    m_from = m_currentProducer->viewFrom();
    m_width = m_currentProducer->viewWidth();

    m_histogram = new KisHistogram(dev, bounds, m_currentProducer, LINEAR);

    updateHistogram();
}

void KisHistogramView::setHistogram(KisHistogramSP histogram)
{
    if (!histogram) return;
    m_cs = 0;
    m_histogram = histogram;
    m_currentProducer = m_histogram->producer();
    m_from = m_currentProducer->viewFrom();
    m_width = m_currentProducer->viewWidth();

    m_comboInfo.clear();
    m_channelStrings.clear();
    m_channels.clear();
    m_channelToOffset.clear();

    addProducerChannels(m_currentProducer);

    // Set the currently viewed channel:
    m_color = false;
    m_channels.append(m_comboInfo.at(1).channel);
    m_channelToOffset.append(0);

    updateHistogram();
}

void KisHistogramView::setView(double from, double size)
{
    m_from = from;
    m_width = size;
    if (m_from + m_width > 1.0)
        m_from = 1.0 - m_width;
    m_histogram->producer()->setView(m_from, m_width);

    m_histogram->updateHistogram();
    updateHistogram();
}

KoHistogramProducerSP KisHistogramView::currentProducer()
{
    return m_currentProducer;
}

QStringList KisHistogramView::channelStrings()
{
    return m_channelStrings;
}

QList<QString> KisHistogramView::producers()
{
    if (m_cs)
        return KoHistogramProducerFactoryRegistry::instance()->keysCompatibleWith(m_cs);
    return QList<QString>();
}

void KisHistogramView::setCurrentChannels(const KoID& producerID, QList<KoChannelInfo *> channels)
{
    setCurrentChannels(
        KoHistogramProducerFactoryRegistry::instance()->value(producerID.id())->generate(),
        channels);
}

void KisHistogramView::setCurrentChannels(KoHistogramProducerSP producer, QList<KoChannelInfo *> channels)
{
    m_currentProducer = producer;
    m_currentProducer->setView(m_from, m_width);
    m_histogram->setProducer(m_currentProducer);
    m_histogram->updateHistogram();
    m_histogram->setChannel(0); // Set a default channel, just being nice

    m_channels.clear();
    m_channelToOffset.clear();

    if (channels.count() == 0) {
        updateHistogram();
        return;
    }

    QList<KoChannelInfo *> producerChannels = m_currentProducer->channels();

    for (int i = 0; i < channels.count(); i++) {
        // Also makes sure the channel is actually in the producer's list
        for (int j = 0; j < producerChannels.count(); j++) {
            if (channels.at(i)->name() == producerChannels.at(j)->name()) {
                m_channelToOffset.append(m_channels.count()); // The first we append maps to 0
                m_channels.append(channels.at(i));
            }
        }
    }

    updateHistogram();
}

bool KisHistogramView::hasColor()
{
    return m_color;
}

void KisHistogramView::setColor(bool set)
{
    if (set != m_color) {
        m_color = set;
        updateHistogram();
    }
}

void KisHistogramView::setActiveChannel(int channel)
{
    ComboboxInfo info = m_comboInfo.at(channel);
    if (info.producer.data() != m_currentProducer.data()) {
        m_currentProducer = info.producer;
        m_currentProducer->setView(m_from, m_width);
        m_histogram->setProducer(m_currentProducer);
        m_histogram->updateHistogram();
    }

    m_channels.clear();
    m_channelToOffset.clear();

    if (!m_currentProducer) {
        updateHistogram();
        return;
    }

    if (info.isProducer) {
        m_color = true;
        m_channels = m_currentProducer->channels();
        for (int i = 0; i < m_channels.count(); i++)
            m_channelToOffset.append(i);
        m_histogram->setChannel(0); // Set a default channel, just being nice
    } else {
        m_color = false;
        QList<KoChannelInfo *> channels = m_currentProducer->channels();
        for (int i = 0; i < channels.count(); i++) {
            KoChannelInfo* channel = channels.at(i);
            if (channel->name() == info.channel->name()) {
                m_channels.append(channel);
                m_channelToOffset.append(i);
                break;
            }
        }
    }

    updateHistogram();
}

void KisHistogramView::setHistogramType(enumHistogramType type)
{
    m_histogram->setHistogramType(type);
    updateHistogram();
}

void KisHistogramView::setChannels()
{
    m_comboInfo.clear();
    m_channelStrings.clear();
    m_channels.clear();
    m_channelToOffset.clear();

    QList<QString> list = KoHistogramProducerFactoryRegistry::instance()->keysCompatibleWith(m_cs);

    if (list.count() == 0) {
        // XXX: No native histogram for this colorspace. Using converted RGB. We should have a warning
        KoGenericRGBHistogramProducerFactory f;
        addProducerChannels(f.generate());
    } else {
        foreach (const QString &id, list) {
            KoHistogramProducerSP producer = KoHistogramProducerFactoryRegistry::instance()->value(id)->generate();
            if (producer) {
                addProducerChannels(producer);
            }
        }
    }

    m_currentProducer = m_comboInfo.at(0).producer;
    m_color = false;
    // The currently displayed channel and its offset
    m_channels.append(m_comboInfo.at(1).channel);
    m_channelToOffset.append(0);
}

void KisHistogramView::addProducerChannels(KoHistogramProducerSP producer)
{
    if (!producer) return;

    ComboboxInfo info;
    info.isProducer = true;
    info.producer = producer;
    // channel not used for a producer
    QList<KoChannelInfo *> channels = info.producer->channels();
    int count = channels.count();
    m_comboInfo.append(info);
    m_channelStrings.append(producer->id() . name());
    for (int j = 0; j < count; j++) {
        info.isProducer = false;
        info.channel = channels.at(j);
        m_comboInfo.append(info);
        m_channelStrings.append(QString(" ").append(info.channel->name()));
    }
}

void KisHistogramView::updateHistogram()
{
    quint32 height = this->height();
    int selFrom, selTo; // from - to in bins

    if (!m_currentProducer) { // Something's very wrong: no producer for this colorspace to update histogram with!
        return;
    }

    qint32 bins = m_histogram->producer()->numberOfBins();

    m_pix = QPixmap(bins, height);
    m_pix.fill();

    QPainter p(&m_pix);
    p.setBrush(Qt::black);

    // Draw the box of the selection, if any
    if (m_histogram->hasSelection()) {
        double width = m_histogram->selectionTo() - m_histogram->selectionFrom();
        double factor = static_cast<double>(bins) / m_histogram->producer()->viewWidth();
        selFrom = static_cast<int>(m_histogram->selectionFrom() * factor);
        selTo = selFrom + static_cast<int>(width * factor);
        p.drawRect(selFrom, 0, selTo - selFrom, height);
    } else {
        // We don't want the drawing to think we're in a selected area
        selFrom = -1;
        selTo = 2;
    }

    qint32 i = 0;
    double highest = 0;
    bool blackOnBlack = false;

    // First we iterate once, so that we have the overall maximum. This is a bit inefficient,
    // but not too much since the histogram caches the calculations
    for (int chan = 0; chan < m_channels.count(); chan++) {
        m_histogram->setChannel(m_channelToOffset.at(chan));
        if ((double)m_histogram->calculations().getHighest() > highest)
            highest = (double)m_histogram->calculations().getHighest();
    }

    QPen pen(Qt::white);

    for (int chan = 0; chan < m_channels.count(); chan++) {
        QColor color;
        m_histogram->setChannel(m_channelToOffset.at(chan));

        if (m_color) {
            color = m_channels.at(chan)->color();
            QLinearGradient gradient;
            gradient.setColorAt(0, Qt::white);
            gradient.setColorAt(1, color);
            QBrush brush(gradient);
            pen = QPen(brush, 0);
        } else {
            color = Qt::black;
        }

        blackOnBlack = (color == Qt::black);

        if (m_histogram->getHistogramType() == LINEAR) {

            double factor = (double)height / highest;
            for (i = 0; i < bins; ++i) {
                // So that we get a good view even with a selection box with
                // black colors on background of black selection
                if (i >= selFrom && i < selTo && blackOnBlack) {
                    p.setPen(Qt::white);
                } else {
                    p.setPen(pen);
                }
                p.drawLine(i, height, i, height - int(m_histogram->getValue(i) * factor));
            }
        } else {

            double factor = (double)height / (double)log(highest);
            for (i = 0; i < bins; ++i) {
                // Same as above
                if (i >= selFrom && i < selTo && blackOnBlack) {
                    p.setPen(Qt::white);
                } else {
                    p.setPen(pen);
                }
                if (m_histogram->getValue(i) > 0) { // Don't try to calculate log(0)
                    p.drawLine(i, height, i,
                               height - int(log((double)m_histogram->getValue(i)) * factor));
                }
            }
        }
    }

    setPixmap(m_pix);
}

void KisHistogramView::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::RightButton)
        emit rightClicked(e->globalPos());
    else
        QLabel::mousePressEvent(e);
}


