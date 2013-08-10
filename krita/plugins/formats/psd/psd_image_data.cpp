/*
 *   Copyright (C) 2011 by Siddharth Sharma <siddharth.kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <psd_image_data.h>

#include <netinet/in.h> // htonl

#include <QFile>
#include <QDebug>
#include <QVector>
#include <QByteArray>
#include <QBuffer>

#include <KoChannelInfo.h>
#include <KoColorSpace.h>
#include <KoColorSpaceMaths.h>
#include <KoColorSpaceTraits.h>

#include "psd_utils.h"
#include "compression.h"

#include "kis_iterator_ng.h"
#include "kis_paint_device.h"

PSDImageData::PSDImageData(PSDHeader *header)
{
    m_header = header;
}

PSDImageData::~PSDImageData() {

}

bool PSDImageData::read(QIODevice *io, KisPaintDeviceSP dev ) {

    psdread(io, &m_compression);
    quint64 start = io->pos();
    m_channelSize = m_header->channelDepth/8;
    m_channelDataLength = m_header->height * m_header->width * m_channelSize;

    switch (m_compression) {

    case 0: // Uncompressed

        for (int channel = 0; channel < m_header->nChannels; channel++) {
            m_channelOffsets << 0;
            ChannelInfo channelInfo;
            channelInfo.channelId = channel;
            channelInfo.compressionType = Compression::Uncompressed;
            channelInfo.channelDataStart = start;
            channelInfo.channelDataLength = m_header->width * m_header->height * m_channelSize;
            start += channelInfo.channelDataLength;
            m_channelInfoRecords.append(channelInfo);

        }

        switch (m_header->colormode) {
        case Bitmap:
            break;
        case Grayscale:
            break;
        case Indexed:
            break;
        case RGB:
            readRGB(io, dev);
            break;
        case CMYK:
            readCMYK(io, dev);
            break;
        case MultiChannel:
            break;
        case DuoTone:
            break;
        case Lab:
            readLAB(io, dev);
            break;
        case UNKNOWN:
            break;
        default:
            break;
        }

        break;

    case 1: // RLE
    {

        quint32 rlelength = 0;

        // The start of the actual channel data is _after_ the RLE rowlengths block
        if (m_header->version == 1) {
            start += m_header->nChannels * m_header->height * 2;
        }
        else if (m_header->version == 2) {
            start += m_header->nChannels * m_header->height * 4;
        }

        for (int channel = 0; channel < m_header->nChannels; channel++) {
            m_channelOffsets << 0;
            quint32 sumrlelength = 0;
            ChannelInfo channelInfo;
            channelInfo.channelId = channel;
            channelInfo.channelDataStart = start;
            channelInfo.compressionType = Compression::RLE;
            for (quint32 row = 0; row < m_header->height; row++ ) {
                if (m_header->version == 1) {
                    psdread(io,(quint16*)&rlelength);
                }
                else if (m_header->version == 2) {
                    psdread(io,&rlelength);
                }
                channelInfo.rleRowLengths.append(rlelength);
                sumrlelength += rlelength;
            }
            channelInfo.channelDataLength = sumrlelength;
            start += channelInfo.channelDataLength;
            m_channelInfoRecords.append(channelInfo);
        }

        switch (m_header->colormode) {
        case Bitmap:
            break;
        case Grayscale:
            break;
        case Indexed:
            break;
        case RGB:
            readRGB(io, dev);
            break;
        case CMYK:
            readCMYK(io, dev);
            break;
        case MultiChannel:
            break;
        case DuoTone:
            break;
        case Lab:
            readLAB(io, dev);
            break;
        case UNKNOWN:
            break;
        default:
            break;
        }

        break;
    }
    case 2: // ZIP without prediction

        switch (m_header->colormode) {
        case Bitmap:
            break;
        case Grayscale:
            break;
        case Indexed:
            break;
        case RGB:
            break;
        case CMYK:
            break;
        case MultiChannel:
            break;
        case DuoTone:
            break;
        case Lab:
            break;
        case UNKNOWN:
            break;
        default:
            break;
        }
        break;

    case 3: // ZIP with prediction
        switch (m_header->colormode) {
        case Bitmap:
            break;
        case Grayscale:
            break;
        case Indexed:
            break;
        case RGB:
            break;
        case CMYK:
            break;
        case MultiChannel:
            break;
        case DuoTone:
            break;
        case Lab:
            break;
        case UNKNOWN:
            break;
        default:
            break;
        }

        break;
    default:
        break;
    }

    return true;
}

bool PSDImageData::write(QIODevice *io, KisPaintDeviceSP dev)
{
    // XXX: make the compression settting configurable. For now, always use RLE.
    psdwrite(io, (quint16)Compression::RLE);

    // now write all the channels in display order
    // fill in the channel chooser, in the display order, but store the pixel index as well.
    QRect rc = dev->exactBounds();
    QVector<quint8* > planes = dev->readPlanarBytes(rc.x(), rc.y(), rc.width(), rc.height());

    quint64 channelLengthPos = io->pos();
    // write zero's for the channel lengths section
    for (uint i = 0; i < dev->colorSpace()->channelCount() * rc.height(); ++i) {
        psdwrite(io, (quint16)0);
    }
    // here the actual channel data starts
    quint64 channelStartPos = io->pos();

    foreach (KoChannelInfo *channelInfo, KoChannelInfo::displayOrderSorted(dev->colorSpace()->channels())) {

        dbgFile << "Writing channel" << channelInfo->name() << "to image section, Display position" << channelInfo->displayPosition() << "channel index" << KoChannelInfo::displayPositionToChannelIndex(channelInfo->displayPosition(), dev->colorSpace()->channels());

        quint8 *plane = planes[KoChannelInfo::displayPositionToChannelIndex(channelInfo->displayPosition(), dev->colorSpace()->channels())];
        quint32 stride = channelInfo->size() * rc.width();
        for (qint32 row = 0; row < rc.height(); ++row) {

            QByteArray uncompressed = QByteArray::fromRawData((const char*)plane + row * stride, stride);
            if (channelInfo->size() == 1) {
            } else if (channelInfo->size() == 2) {
                quint16 *dataPtr = reinterpret_cast<quint16 *>(uncompressed.data());
                for (int i = 0; i < rc.width(); i++) {
                    quint16 val = htons(*dataPtr);
                    *dataPtr = val;
                    ++dataPtr;
                }
            } else if (channelInfo->size() == 4) {
                quint32 *dataPtr = reinterpret_cast<quint32 *>(uncompressed.data());
                for (int i = 0; i < rc.width(); i++) {
                    quint32 val = htonl(*dataPtr);
                    *dataPtr = val;
                    ++dataPtr;
                }
            }
            QByteArray compressed = Compression::compress(uncompressed, Compression::RLE);

            io->seek(channelLengthPos);
            psdwrite(io, (quint16)compressed.size());
            channelLengthPos +=2;
            io->seek(channelStartPos);

            if (!io->write(compressed) == compressed.size()) {
                error = "Could not write image data";
                return false;
            }

            channelStartPos += compressed.size();
        }
    }

    qDeleteAll(planes);
    planes.clear();

    return true;
}

bool PSDImageData::readRGB(QIODevice *io, KisPaintDeviceSP dev) {

    int channelid = 0;

    for (quint32 row = 0; row < m_header->height; row++) {

        KisHLineIteratorSP it = dev->createHLineIteratorNG(0, row, m_header->width);
        QVector<QByteArray> channelBytes;

        for (int channel = 0; channel < m_header->nChannels; channel++) {

            switch (m_compression) {
            case Compression::Uncompressed:
            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[0]);
                channelBytes.append(io->read(m_header->width*m_channelSize));
            }
                break;
            case Compression::RLE:
            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[channel]);
                int uncompressedLength = m_header->width * m_header->channelDepth / 8;
                QByteArray compressedBytes = io->read(m_channelInfoRecords[channel].rleRowLengths[row]);
                QByteArray uncompressedBytes = Compression::uncompress(uncompressedLength, compressedBytes, m_channelInfoRecords[channel].compressionType);
                channelBytes.append(uncompressedBytes);
                m_channelOffsets[channel] +=  m_channelInfoRecords[channel].rleRowLengths[row];

            }
                break;
            case Compression::ZIP:
                break;
            case Compression::ZIPWithPrediction:
                break;

            default:
                break;
            }

        }

        if (m_channelInfoRecords[channelid].compressionType == 0){
            m_channelOffsets[channelid] += (m_header->width * m_channelSize);
        }

        for (quint32 col = 0; col < m_header->width; col++) {

            if (m_channelSize == 1) {

                quint8 red = channelBytes[0].constData()[col];
                KoBgrU8Traits::setRed(it->rawData(), red);

                quint8 green = channelBytes[1].constData()[col];
                KoBgrU8Traits::setGreen(it->rawData(), green);

                quint8 blue = channelBytes[2].constData()[col];
                KoBgrU8Traits::setBlue(it->rawData(), blue);

            }

            else if (m_channelSize == 2) {

                quint16 red = ntohs(reinterpret_cast<const quint16 *>(channelBytes[0].constData())[col]);
                KoBgrU16Traits::setRed(it->rawData(), red);

                quint16 green = ntohs(reinterpret_cast<const quint16 *>(channelBytes[1].constData())[col]);
                KoBgrU16Traits::setGreen(it->rawData(), green);

                quint16 blue = ntohs(reinterpret_cast<const quint16 *>(channelBytes[2].constData())[col]);
                KoBgrU16Traits::setBlue(it->rawData(), blue);

            }

            // XXX see implementation Openexr
            else if (m_channelSize == 4) {

                quint32 red = ntohl(reinterpret_cast<const quint32 *>(channelBytes.constData())[col]);
                KoBgrTraits<quint32>::setRed(it->rawData(), red);

                quint32 green = ntohl(reinterpret_cast<const quint32 *>(channelBytes.constData())[col]);
                KoBgrTraits<quint32>::setGreen(it->rawData(), green);

                quint32 blue = ntohl(reinterpret_cast<const quint32 *>(channelBytes.constData())[col]);
                KoBgrTraits<quint32>::setBlue(it->rawData(), blue);

            }

            dev->colorSpace()->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
            it->nextPixel();
        }

    }

    return true;
}


bool PSDImageData::readCMYK(QIODevice *io, KisPaintDeviceSP dev) {

    int channelid = 0;

    for (quint32 row = 0; row < m_header->height; row++) {

        KisHLineIteratorSP it = dev->createHLineIteratorNG(0, row, m_header->width);
        QVector<QByteArray> channelBytes;

        for (int channel = 0; channel < m_header->nChannels; channel++) {


            switch (m_compression) {

            case Compression::Uncompressed:

            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[0]);
                channelBytes.append(io->read(m_header->width*m_channelSize));

            }
                break;

            case Compression::RLE:
            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[channel]);
                int uncompressedLength = m_header->width * m_header->channelDepth / 8;
                QByteArray compressedBytes = io->read(m_channelInfoRecords[channel].rleRowLengths[row]);
                QByteArray uncompressedBytes = Compression::uncompress(uncompressedLength, compressedBytes, m_channelInfoRecords[channel].compressionType);
                channelBytes.append(uncompressedBytes);
                m_channelOffsets[channel] +=  m_channelInfoRecords[channel].rleRowLengths[row];
            }
                break;

            case Compression::ZIP:
                break;

            case Compression::ZIPWithPrediction:
                break;

            default:
                break;
            }

        }

        if (m_channelInfoRecords[channelid].compressionType == 0){
            m_channelOffsets[channelid] += (m_header->width * m_channelSize);
        }

        for (quint32 col = 0; col < m_header->width; col++) {

            if (m_channelSize == 1) {

                quint8 *pixel = new quint8[5];
                memset(pixel, 0, 5);
                dev->colorSpace()->setOpacity(pixel, OPACITY_OPAQUE_U8, 1);

                memset(pixel, 255 - channelBytes[0].constData()[col], 1);
                memset(pixel + 1, 255 - channelBytes[1].constData()[col], 1);
                memset(pixel + 2, 255 - channelBytes[2].constData()[col], 1);
                memset(pixel + 3, 255 - channelBytes[3].constData()[col], 1);
                dbgFile << "C" << pixel[0] << "M" << pixel[1] << "Y" << pixel[2] << "K" << pixel[3] << "A" << pixel[4];
                memcpy(it->rawData(), pixel, 5);


            }
            else if (m_channelSize == 2) {

                quint16 C = ntohs(reinterpret_cast<const quint16 *>(channelBytes[0].constData())[col]);
                KoCmykTraits<quint16>::setC(it->rawData(),C);

                quint16 M = ntohs(reinterpret_cast<const quint16 *>(channelBytes[1].constData())[col]);
                KoCmykTraits<quint16>::setM(it->rawData(),M);

                quint16 Y = ntohs(reinterpret_cast<const quint16 *>(channelBytes[2].constData())[col]);
                KoCmykTraits<quint16>::setY(it->rawData(),Y);

                quint16 K = ntohs(reinterpret_cast<const quint16 *>(channelBytes[3].constData())[col]);
               KoCmykTraits<quint16>::setK(it->rawData(),K);

            }
            else if (m_channelSize == 4) {

                quint32 C = ntohl(reinterpret_cast<const quint32 *>(channelBytes[0].constData())[col]);
                KoCmykTraits<quint32>::setC(it->rawData(),C);

                quint32 M = ntohl(reinterpret_cast<const quint32 *>(channelBytes[1].constData())[col]);
                KoCmykTraits<quint32>::setM(it->rawData(),M);

                quint32 Y = ntohl(reinterpret_cast<const quint32 *>(channelBytes[2].constData())[col]);
                KoCmykTraits<quint32>::setY(it->rawData(),Y);

                quint32 K = ntohl(reinterpret_cast<const quint32 *>(channelBytes[3].constData())[col]);
                KoCmykTraits<quint32>::setK(it->rawData(),K);

            }

            dev->colorSpace()->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
            it->nextPixel();;
        }

    }

    return true;

}

bool PSDImageData::readLAB(QIODevice *io, KisPaintDeviceSP dev) {

    int channelid = 0;

    for (quint32 row = 0; row < m_header->height; row++) {

        KisHLineIteratorSP it = dev->createHLineIteratorNG(0, row, m_header->width);
        QVector<QByteArray> channelBytes;

        for (int channel = 0; channel < m_header->nChannels; channel++) {


            switch (m_compression) {

            case Compression::Uncompressed:

            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[0]);
                channelBytes.append(io->read(m_header->width*m_channelSize));
            }
                break;

            case Compression::RLE:
            {
                io->seek(m_channelInfoRecords[channel].channelDataStart + m_channelOffsets[channel]);
                int uncompressedLength = m_header->width * m_header->channelDepth / 8;
                QByteArray compressedBytes = io->read(m_channelInfoRecords[channel].rleRowLengths[row]);
                QByteArray uncompressedBytes = Compression::uncompress(uncompressedLength, compressedBytes, m_channelInfoRecords[channel].compressionType);
                channelBytes.append(uncompressedBytes);
                m_channelOffsets[channel] +=  m_channelInfoRecords[channel].rleRowLengths[row];

            }
                break;

            case Compression::ZIP:
                break;

            case Compression::ZIPWithPrediction:
                break;

            default:
                break;
            }

        }

        if (m_channelInfoRecords[channelid].compressionType == 0){
            m_channelOffsets[channelid] += (m_header->width * m_channelSize);
        }

        for (quint32 col = 0; col < m_header->width; col++) {

            if (m_channelSize == 1) {

                quint8 L = channelBytes[0].constData()[col];
                KoLabTraits<quint8>::setL(it->rawData(), L);

                quint8 A = channelBytes[1].constData()[col];
                KoLabTraits<quint8>::setA(it->rawData(), A);

                quint8 B = channelBytes[2].constData()[col];
                KoLabTraits<quint8>::setB(it->rawData(), B);
            }

            else if (m_channelSize == 2) {

                quint16 L = ntohs(reinterpret_cast<const quint16 *>(channelBytes[0].constData())[col]);
                KoLabU16Traits::setL(it->rawData(),L);

                quint16 A = ntohs(reinterpret_cast<const quint16 *>(channelBytes[1].constData())[col]);
                KoLabU16Traits::setA(it->rawData(),A);

                quint16 B = ntohs(reinterpret_cast<const quint16 *>(channelBytes[2].constData())[col]);
                KoLabU16Traits::setB(it->rawData(),B);

            }

            else if (m_channelSize == 4) {

                quint32 L = ntohl(reinterpret_cast<const quint32 *>(channelBytes[0].constData())[col]);
                KoLabTraits<quint32>::setL(it->rawData(),L);

                quint32 A = ntohl(reinterpret_cast<const quint32 *>(channelBytes[1].constData())[col]);
                KoLabTraits<quint32>::setA(it->rawData(),A);

                quint32 B = ntohl(reinterpret_cast<const quint32 *>(channelBytes[2].constData())[col]);
                KoLabTraits<quint32>::setB(it->rawData(),B);

            }

            dev->colorSpace()->setOpacity(it->rawData(), OPACITY_OPAQUE_U8, 1);
            it->nextPixel();;
        }

    }

    return true;
}
