/*
 *  Copyright (c) 2013 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
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

#include "kis_gmic_tests.h"


#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <KoColorSpaceRegistry.h>
#include <kis_image.h>
#include <KoColorSpace.h>
#include <KoColorModelStandardIds.h>
#include <KoColor.h>
#include <KoProperties.h>
#include <kis_surrogate_undo_adapter.h>

#include <qtest_kde.h>
#include <QImage>
#include <QTextDocument>

#include <gmic.h>
#include <kis_gmic_parser.h>
#include <Component.h>
#include <kis_gmic_filter_model.h>
#include <Command.h>
#include <kis_gmic_simple_convertor.h>
#include <kis_gmic_blacklister.h>
#include <kis_input_output_mapper.h>

#include <kglobal.h>
#include <kstandarddirs.h>

#include <kis_group_layer.h>
#include <kis_painter.h>
#include <testutil.h>

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing the importing of files in krita"
#endif


using namespace cimg_library;

const static QString EXTENSION = ".png";

void KisGmicTests::initTestCase()
{
    KGlobal::dirs()->addResourceType("gmic_definitions", "data", "krita/gmic/");

    QString standardSettings("gmic_def.gmic");
    QString definitionFilePath = KGlobal::mainComponent().dirs()->findResource("gmic_definitions", standardSettings);
    m_blacklistFilePath = KGlobal::mainComponent().dirs()->findResource("gmic_definitions", standardSettings + ".blacklist");

    KisGmicParser parser(definitionFilePath);
    m_root = parser.createFilterTree();

    m_blacklister = new KisGmicBlacklister(m_blacklistFilePath);

    m_qimage = QImage(QString(FILES_DATA_DIR)+"/"+"poster_rodents_bunnysize.jpg");
    m_qimage = m_qimage.convertToFormat(QImage::Format_ARGB32);

    m_gmicImage.assign(m_qimage.width(),m_qimage.height(),1,4); // rgba

    KisGmicSimpleConvertor::convertFromQImage(m_qimage, m_gmicImage);;

    m_images.assign(1);
}

void KisGmicTests::cleanupTestCase()
{
    const unsigned int zero(0);
    m_images.assign(zero);
    delete m_blacklister;
    delete m_root;
}

#ifdef RUN_FILTERS
void KisGmicTests::testColorizeFilter()
{
    QString filterName = "Colorize [comics]";
    QString filterCategory = "Black & white";

    Component * c = KisGmicBlacklister::findFilter(m_root, filterCategory, filterName);

    KisGmicFilterSetting filterSettings;
    if (c == 0)
    {
            qDebug() << "Filter not found!";
    }else
    {
        Command * cmd = static_cast<Command *>(c);
        cmd->setParameter("Input layers", "Lineart + color spots");
        cmd->setParameter("Output layers", "Lineart + color spots + extrapolated colors");
        cmd->setParameter("Smoothness", "0.05");
        cmd->writeConfiguration(&filterSettings);
    }

    QVERIFY(!filterSettings.gmicCommand().isEmpty());

    // load 3 layers here
    QStringList layerNames;
    layerNames << "02_Artline.png";
    layerNames << "01_ColorMarks.png";
    layerNames << "00_BG.png";

    m_images.assign(layerNames.size());

    int layerIndex = 0;
    for (int i = 0; i < layerNames.size(); i++)
    {
        const QString& layerName = layerNames.at(i);
        QImage layerImage = QImage(QString(FILES_DATA_DIR) + QDir::separator() + layerName).convertToFormat(QImage::Format_ARGB32);
        gmic_image<float> gmicImage;
        gmicImage.assign(layerImage.width(), layerImage.height(), 1, 4);

        KisGmicSimpleConvertor::convertFromQImage(layerImage, gmicImage);
        m_images[layerIndex] = gmicImage;
        layerIndex++;
    }

    filterWithGmic(&filterSettings, "multi_colorize", m_images);
}
#endif

void KisGmicTests::testGatherLayers()
{
    const KoColorSpace * colorSpace = KoColorSpaceRegistry::instance()->rgb8();

    QImage background(QString(FILES_DATA_DIR) + QDir::separator() + "00_BG.png");
    QImage colorMarks(QString(FILES_DATA_DIR) + QDir::separator() + "01_ColorMarks.png");
    QImage artLine(QString(FILES_DATA_DIR) + QDir::separator() + "02_Artline.png");

    KisImageSP image = new KisImage(0, 2408, 3508, colorSpace, "filter test");

    KisPaintDeviceSP device1 = new KisPaintDevice(colorSpace);
    KisPaintDeviceSP device2 = new KisPaintDevice(colorSpace);
    KisPaintDeviceSP device3 = new KisPaintDevice(colorSpace);
    device1->convertFromQImage(background, 0, 0, 0);
    device2->convertFromQImage(colorMarks, 0, 0, 0);
    device3->convertFromQImage(artLine, 0, 0, 0);

    KisLayerSP paintLayer1 = new KisPaintLayer(image, "background", OPACITY_OPAQUE_U8, device1);
    KisLayerSP paintLayer2 = new KisPaintLayer(image, "colorMarks", OPACITY_OPAQUE_U8, device2);
    KisLayerSP paintLayer3 = new KisPaintLayer(image, "artLine", OPACITY_OPAQUE_U8, device3);

    image->addNode(paintLayer1, image->rootLayer());
    image->addNode(paintLayer2, image->rootLayer());
    image->addNode(paintLayer3, image->rootLayer());

    KisNodeSP activeNode = static_cast<KisNodeSP>(paintLayer2);

    KisNodeListSP result;
    KisInputOutputMapper mapper(image, activeNode);

    result = mapper.inputNodes(ACTIVE_LAYER);
    QCOMPARE(result->at(0)->name(), activeNode->name());

    result = mapper.inputNodes(ACTIVE_LAYER_ABOVE_LAYER);
    QCOMPARE(result->size(), 2);
    QCOMPARE(result->at(0)->name(), activeNode->name());
    QCOMPARE(result->at(1)->name(), paintLayer3->name());

    result = mapper.inputNodes(ACTIVE_LAYER_BELOW_LAYER);
    QCOMPARE(result->size(), 2);
    QCOMPARE(result->at(0)->name(), activeNode->name());
    QCOMPARE(result->at(1)->name(), paintLayer1->name());

    result = mapper.inputNodes(ALL_LAYERS);
    QCOMPARE(result->size(), 3);
    QCOMPARE(result->at(0)->name(), paintLayer3->name());
    QCOMPARE(result->at(1)->name(), paintLayer2->name());
    QCOMPARE(result->at(2)->name(), paintLayer1->name());
}

void KisGmicTests::testAllFilters()
{

    QQueue<Component *> q;
    q.enqueue(m_root);

    KisGmicFilterSetting filterSettings;
    int filters = 0;

    int failed = 0;
    int success = 0;

    QVector<QString> failedFilters;

    while (!q.isEmpty())
    {
        Component * c = q.dequeue();
        if (c->childCount() == 0)
        {
            Command * cmd = static_cast<Command *>(c);
            cmd->writeConfiguration(&filterSettings);
            //qDebug() << "Filter: " << c->name() << filterSettings.gmicCommand();
            if (!filterSettings.gmicCommand().startsWith("-_none_"))
            {
                filters++;

#ifdef RUN_FILTERS
                QString filterName = KisGmicBlacklister::toPlainText(cmd->name());
                QString categoryName = KisGmicBlacklister::toPlainText(cmd->parent()->name()); // parent is category

                if (isAlreadyThere( filePathify( filterName ) ))
                {
                    qDebug() << "Already works, skipping filter" << filterName;
                    success++;
                }
                else if (m_blacklister->isBlacklisted(filterName, categoryName))
                {
                    qDebug() << "Blacklisted filter, increase fails" << filterName;
                    failed++;
                    failedFilters.append(categoryName+":"+filterName);
                }
                else
                {
                    qDebug() << "Filtering with:";
                    qDebug() << QString("<category name=\"%0\">").arg(categoryName);
                    qDebug() << QString("<filter name=\"%0\" />").arg(filterName);

                    // clear previous data?
                    m_images.assign(1);
                    // copy?
                    m_images._data[0] = m_gmicImage;
                    bool result = filterWithGmic(&filterSettings, filterName, m_images);
                    result ? success++ : failed++;
                    qDebug() << "Progress status:" << "Failed:" << failed << " Success: " << success;
                    if (result == false)
                    {
                        failedFilters.append(categoryName+":"+filterName);
                    }
                }
#endif
            }
        }
        else
        {
            for (int i=0; i < c->childCount(); i++)
            {
                q.enqueue(c->child(i));
            }
        }
    }

#ifdef RUN_FILTERS
    qDebug() << "Finish status:" << "Failed:" << failed << " Success: " << success;
    qDebug() << "== Failed filters ==";
    foreach (const QString &item, failedFilters)
    {
        qDebug() << item;
    }


#else
    Q_UNUSED(success);
    Q_UNUSED(failed);
#endif



    QCOMPARE(filters,260);
}


bool KisGmicTests::filterWithGmic(KisGmicFilterSetting* gmicFilterSetting, const QString &filterName, gmic_list<float> &images)
{
    QString fileName = filePathify(filterName);
    qDebug() << "Filename for filter " << filterName << " : " << fileName;
    // Second step : Call G'MIC API to process input images.
    //------------------------------------------------------
    std::fprintf(stderr,"\n- 2st step : Call G'MIC interpreter.\n");
    gmic_list<char> images_names;
    try
    {
        QString gmicCmd = "-* 255 ";
        gmicCmd.append(gmicFilterSetting->gmicCommand());
        gmic(gmicCmd.toLocal8Bit().constData(), images, images_names);
    }
    // Catch exception, if an error occured in the interpreter.
    catch (gmic_exception &e)
    {
        dbgPlugins << "\n- Error encountered when calling G'MIC : '%s'\n" << e.what();
        return false;
    }

    // Third step : get back modified image data.
    //-------------------------------------------
    std::fprintf(stderr,"\n- 3st step : Returned %u output images.\n", images._width);
    for (unsigned int i = 0; i < images._width; ++i)
    {
        std::fprintf(stderr,"   Output image %u = %ux%ux%ux%u, buffer : %p\n",i,
                    images._data[i]._width,
                    images._data[i]._height,
                    images._data[i]._depth,
                    images._data[i]._spectrum,
                    images._data[i]._data);
    }

    // Forth step : convert to QImage and save
    for (unsigned int i = 0; i < images._width; ++i)
    {
        KisGmicSimpleConvertor convertor;
        KisPaintDeviceSP device = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
        convertor.convertFromGmicImage(images._data[i], device, 255.0f);
        QImage result = device->convertToQImage(0, 0,0,images._data[i]._width, images._data[i]._height);
        QString indexString("_%1");
        if (images._width > 1)
        {
            indexString = indexString.arg(i,4, 10, QLatin1Char('_'));
        }else
        {
            indexString = QString();
        }
        QString fullFileName(QString(FILES_OUTPUT_DIR) + QDir::separator() + fileName + indexString + ".png");
        QFileInfo info(fullFileName);
        if (info.exists())
        {
            continue;
        }

        bool isSaved = result.save(fullFileName);
        if (!isSaved)
        {
            qDebug() << "Saving " << fileName  << "failed";
            return false;
        }else
        {
            qDebug() << "Saved " << fullFileName << " -- OK";
        }
    }

    return true;
}

QString KisGmicTests::filePathify(const QString& fileName)
{
    QStringList illegalCharacters;
    illegalCharacters << QLatin1String("/")
    << QLatin1String("\\")
    << QLatin1String("?")
    << QLatin1String(":");

    QString result = fileName;
    foreach(const QString& item, illegalCharacters)
    {
        result = result.replace(item ,"_");
    }
    return result;
}

void KisGmicTests::testBlacklister()
{
    QString category("Colors");
    QString filterName("User-defined");
    QVERIFY(m_blacklister->isBlacklisted(filterName, category));
}

bool KisGmicTests::isAlreadyThere(QString fileName)
{
    QString path = QString(FILES_OUTPUT_DIR) + QDir::separator() + fileName + EXTENSION;
    QFileInfo info(path);
    return info.exists();
}

void KisGmicTests::testConvertGrayScaleGmic()
{
    KisPaintDeviceSP resultDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    KisPaintDeviceSP resultDevFast = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());

    gmic_image<float> gmicImage;
    gmicImage.assign(m_qimage.width(),m_qimage.height(),1,1);

    KisGmicSimpleConvertor::convertFromQImage(m_qimage, gmicImage, 1.0);
    KisGmicSimpleConvertor::convertFromGmicImage(gmicImage, resultDev, 1.0);
    KisGmicSimpleConvertor::convertFromGmicFast(gmicImage, resultDevFast, 1.0);


    QImage slowQImage = resultDev->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);
    QImage fastQImage = resultDevFast->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);

    QPoint errpoint;
    if (!TestUtil::compareQImages(errpoint,slowQImage,fastQImage))
    {
        QFAIL(QString("Slow method produces different result then fast to convert gmic grayscale pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        slowQImage.save("grayscale.bmp");
        fastQImage.save("grayscale_fast.bmp");
    }
}

void KisGmicTests::testConvertGrayScaleAlphaGmic()
{
    KisPaintDeviceSP resultDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    KisPaintDeviceSP resultDevFast = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());

    gmic_image<float> gmicImage;
    gmicImage.assign(m_qimage.width(),m_qimage.height(),1,2);

    KisGmicSimpleConvertor::convertFromQImage(m_qimage, gmicImage, 1.0);
    KisGmicSimpleConvertor::convertFromGmicImage(gmicImage, resultDev, 1.0);
    KisGmicSimpleConvertor::convertFromGmicFast(gmicImage, resultDevFast, 1.0);

    QImage slowQImage = resultDev->convertToQImage(0, 0,0,gmicImage._width, gmicImage._height);
    QImage fastQImage = resultDevFast->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);

    QPoint errpoint;
    if (!TestUtil::compareQImages(errpoint,slowQImage,fastQImage))
    {
        QFAIL(QString("Slow method produces different result then fast to convert gmic grayscale pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        slowQImage.save("grayscale.bmp");
        fastQImage.save("grayscale_fast.bmp");
    }
}

void KisGmicTests::testConvertRGBgmic()
{
    KisPaintDeviceSP resultDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    KisPaintDeviceSP resultDevFast = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());

    gmic_image<float> gmicImage;
    gmicImage.assign(m_qimage.width(),m_qimage.height(),1,3);

    KisGmicSimpleConvertor::convertFromQImage(m_qimage, gmicImage, 1.0);
    KisGmicSimpleConvertor::convertFromGmicImage(gmicImage, resultDev, 1.0);
    KisGmicSimpleConvertor::convertFromGmicFast(gmicImage, resultDevFast, 1.0);

    QImage slowQImage = resultDev->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);
    QPoint errpoint;
    if (!TestUtil::compareQImages(errpoint,slowQImage,m_qimage))
    {
        QFAIL(QString("Slow method failed to convert gmic RGB pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        slowQImage.save("RGB.bmp");
    }

    QImage fastQImage = resultDevFast->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);
    if (!TestUtil::compareQImages(errpoint,fastQImage,m_qimage))
    {
        QFAIL(QString("Fast method failed to convert gmic RGB pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        fastQImage.save("RGB_fast.bmp");
    }
}


void KisGmicTests::testConvertRGBAgmic()
{
    KisPaintDeviceSP resultDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    KisPaintDeviceSP resultDevFast = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());

    gmic_image<float> gmicImage;
    gmicImage.assign(m_qimage.width(),m_qimage.height(),1,4);

    KisGmicSimpleConvertor::convertFromQImage(m_qimage, gmicImage, 1.0);
    KisGmicSimpleConvertor::convertFromGmicImage(gmicImage, resultDev, 1.0);
    KisGmicSimpleConvertor::convertFromGmicFast(gmicImage, resultDevFast, 1.0);

    QImage slowQImage = resultDev->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);
    QPoint errpoint;
    if (!TestUtil::compareQImages(errpoint,slowQImage,m_qimage))
    {
        QFAIL(QString("Slow method failed to convert gmic RGBA pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        slowQImage.save("RGBA.bmp");
    }

    QImage fastQImage = resultDevFast->convertToQImage(0,0,0,gmicImage._width, gmicImage._height);
    if (!TestUtil::compareQImages(errpoint,fastQImage,m_qimage))
    {
        QFAIL(QString("Fast method failed to convert gmic RGBA pixel format, first different pixel: %1,%2 ").arg(errpoint.x()).arg(errpoint.y()).toLatin1());
        fastQImage.save("RGBA_fast.bmp");
    }
}


QTEST_KDEMAIN(KisGmicTests, NoGUI)

#include "kis_gmic_tests.moc"
