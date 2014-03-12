/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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

#include "kis_config.h"

#include <limits.h>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#include <QX11Info>
#endif


#include <QMutex>
#include <QFont>
#include <QThread>
#include <QStringList>

#include <kglobalsettings.h>
#include <kglobal.h>
#include <kconfig.h>

#include <KoDocument.h>

#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>

#include <kis_debug.h>

#include "kis_canvas_resource_provider.h"
#include "kis_global.h"

#include <config-ocio.h>


namespace
{
const double IMAGE_DEFAULT_RESOLUTION = 100.0; // dpi
const qint32 IMAGE_DEFAULT_WIDTH = 1600;
const qint32 IMAGE_DEFAULT_HEIGHT = 1200;
const enumCursorStyle DEFAULT_CURSOR_STYLE = CURSOR_STYLE_OUTLINE;
const qint32 DEFAULT_MAX_TILES_MEM = 5000;

static QMutex s_synchLocker;

}

KisConfig::KisConfig()
    : m_cfg(KGlobal::config()->group(""))
{
}

KisConfig::~KisConfig()
{
    s_synchLocker.lock();
    m_cfg.sync();
    s_synchLocker.unlock();
}


bool KisConfig::useProjections() const
{
    return m_cfg.readEntry("useProjections", true);
}

void KisConfig::setUseProjections(bool useProj) const
{
    m_cfg.writeEntry("useProjections", useProj);
}

bool KisConfig::undoEnabled() const
{
    return m_cfg.readEntry("undoEnabled", true);
}

void KisConfig::setUndoEnabled(bool undo) const
{
    m_cfg.writeEntry("undoEnabled", undo);
}

int KisConfig::undoStackLimit() const
{
    return m_cfg.readEntry("undoStackLimit", 30);
}

void KisConfig::setUndoStackLimit(int limit) const
{
    m_cfg.writeEntry("undoStackLimit", limit);
}

qint32 KisConfig::defImageWidth() const
{
    return m_cfg.readEntry("imageWidthDef", IMAGE_DEFAULT_WIDTH);
}

qint32 KisConfig::defImageHeight() const
{
    return m_cfg.readEntry("imageHeightDef", IMAGE_DEFAULT_HEIGHT);
}

double KisConfig::defImageResolution() const
{
    return m_cfg.readEntry("imageResolutionDef", IMAGE_DEFAULT_RESOLUTION) / 72.0;
}

QString KisConfig::defColorModel() const
{
    return m_cfg.readEntry("colorModelDef", KoColorSpaceRegistry::instance()->rgb8()->colorModelId().id());
}

void KisConfig::defColorModel(const QString & model) const
{
    m_cfg.writeEntry("colorModelDef", model);
}

QString KisConfig::defaultColorDepth() const
{
    return m_cfg.readEntry("colorDepthDef", KoColorSpaceRegistry::instance()->rgb8()->colorDepthId().id());
}

void KisConfig::setDefaultColorDepth(const QString & depth) const
{
    m_cfg.writeEntry("colorDepthDef", depth);
}

QString KisConfig::defColorProfile() const
{
    return m_cfg.readEntry("colorProfileDef", KoColorSpaceRegistry::instance()->rgb8()->profile()->name());
}

void KisConfig::defColorProfile(const QString & profile) const
{
    m_cfg.writeEntry("colorProfileDef", profile);
}

void KisConfig::defImageWidth(qint32 width) const
{
    m_cfg.writeEntry("imageWidthDef", width);
}

void KisConfig::defImageHeight(qint32 height) const
{
    m_cfg.writeEntry("imageHeightDef", height);
}

void KisConfig::defImageResolution(double res) const
{
    m_cfg.writeEntry("imageResolutionDef", res*72.0);
}

enumCursorStyle KisConfig::cursorStyle() const
{
    return (enumCursorStyle) m_cfg.readEntry("cursorStyleDef", int(DEFAULT_CURSOR_STYLE));
}

enumCursorStyle KisConfig::getDefaultCursorStyle() const
{
    return DEFAULT_CURSOR_STYLE;
}

void KisConfig::setCursorStyle(enumCursorStyle style) const
{
    m_cfg.writeEntry("cursorStyleDef", (int)style);
}

QString KisConfig::monitorProfile() const
{
    QString profile = m_cfg.readEntry("monitorProfile", "");
    //qDebug() << "KisConfig::monitorProfile()" << profile;
    return profile;
}

void KisConfig::setMonitorProfile(const QString & monitorProfile, bool override) const
{
    m_cfg.writeEntry("monitorProfile/OverrideX11", override);
    m_cfg.writeEntry("monitorProfile", monitorProfile);
}

const KoColorProfile *KisConfig::getScreenProfile(int screen)
{
#ifdef Q_WS_X11

    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    quint8 * str;

    static Atom icc_atom = XInternAtom(QX11Info::display(), "_ICC_PROFILE", True);

    if (XGetWindowProperty(QX11Info::display(),
                           QX11Info::appRootWindow(screen),
                           icc_atom,
                           0,
                           INT_MAX,
                           False,
                           XA_CARDINAL,
                           &type,
                           &format,
                           &nitems,
                           &bytes_after,
                           (unsigned char **) &str) == Success
       ) {
        QByteArray bytes(nitems, '\0');
        bytes = QByteArray::fromRawData((char*)str, (quint32)nitems);
        // XXX: this assumes the screen is 8 bits -- which might not be true
        const KoColorProfile *profile = KoColorSpaceRegistry::instance()->createColorProfile(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), bytes);
        //qDebug() << "KisConfig::getScreenProfile for screen" << screen << profile->name();
        return profile;
    }
    else {
        return 0;
    }
#else
    return 0;

#endif
}

const KoColorProfile *KisConfig::displayProfile(int screen) const
{
    // first try to get the screen profile set by the X11 _ICC_PROFILE atom (compatible with colord,
    // but colord can set the atom to none, in which case we cannot create a suitable profile)

    // if the user plays with the settings, they can override the display profile, in which case
    // we don't want the X11 atom setting.
    bool override = m_cfg.readEntry("monitorProfile/OverrideX11", false);
    //qDebug() << "KisConfig::displayProfile(). Override X11:" << override;
    const KoColorProfile *profile = 0;
    if (override) {
        //qDebug() << "\tGoing to get the screen profile";
        profile = KisConfig::getScreenProfile(screen);
    }

    // if it fails. check the configuration
    if (!profile || !profile->isSuitableForDisplay()) {
        //qDebug() << "\tGoing to get the monitor profile";
        QString monitorProfileName = monitorProfile();
        //qDebug() << "\t\tmonitorProfileName:" << monitorProfileName;
        if (!monitorProfileName.isEmpty()) {
            profile = KoColorSpaceRegistry::instance()->profileByName(monitorProfileName);
        }
        //qDebug() << "\t\tsuitable for display6" << profile->isSuitableForDisplay();
    }
    // if we still don't have a profile, or the profile isn't suitable for display,
    // we need to get a last-resort profile. the built-in sRGB is a good choice then.
    if (!profile || !profile->isSuitableForDisplay()) {
        //qDebug() << "\tnothing worked, going to get sRGB built-in";
        profile = KoColorSpaceRegistry::instance()->profileByName("sRGB Built-in");
    }

    //qDebug() << "\tKisConfig::displayProfile for screen" << screen << "is" << profile->name();

    return profile;
}

QString KisConfig::workingColorSpace() const
{
    return m_cfg.readEntry("workingColorSpace", "RGBA");
}

void KisConfig::setWorkingColorSpace(const QString & workingColorSpace) const
{
    m_cfg.writeEntry("workingColorSpace", workingColorSpace);
}

QString KisConfig::printerColorSpace() const
{
    //TODO currently only rgb8 is supported
    //return m_cfg.readEntry("printerColorSpace", "RGBA");
    return QString("RGBA");
}

void KisConfig::setPrinterColorSpace(const QString & printerColorSpace) const
{
    m_cfg.writeEntry("printerColorSpace", printerColorSpace);
}


QString KisConfig::printerProfile() const
{
    return m_cfg.readEntry("printerProfile", "");
}

void KisConfig::setPrinterProfile(const QString & printerProfile) const
{
    m_cfg.writeEntry("printerProfile", printerProfile);
}


bool KisConfig::useBlackPointCompensation() const
{
    return m_cfg.readEntry("useBlackPointCompensation", true);
}

void KisConfig::setUseBlackPointCompensation(bool useBlackPointCompensation) const
{
    m_cfg.writeEntry("useBlackPointCompensation", useBlackPointCompensation);
}

bool KisConfig::allowLCMSOptimization() const
{
    return m_cfg.readEntry("allowLCMSOptimization", true);
}

void KisConfig::setAllowLCMSOptimization(bool allowLCMSOptimization)
{
    m_cfg.writeEntry("allowLCMSOptimization", allowLCMSOptimization);
}


bool KisConfig::showRulers() const
{
    return m_cfg.readEntry("showrulers", false);
}

void KisConfig::setShowRulers(bool rulers) const
{
    m_cfg.writeEntry("showrulers", rulers);
}


qint32 KisConfig::pasteBehaviour() const
{
    return m_cfg.readEntry("pasteBehaviour", 2);
}

void KisConfig::setPasteBehaviour(qint32 renderIntent) const
{
    m_cfg.writeEntry("pasteBehaviour", renderIntent);
}


qint32 KisConfig::renderIntent() const
{
    qint32 intent = m_cfg.readEntry("renderIntent", INTENT_PERCEPTUAL);
    if (intent > 3) intent = 3;
    if (intent < 0) intent = 0;
    return intent;
}

void KisConfig::setRenderIntent(qint32 renderIntent) const
{
    if (renderIntent > 3) renderIntent = 3;
    if (renderIntent < 0) renderIntent = 0;
    m_cfg.writeEntry("renderIntent", renderIntent);
}

bool KisConfig::useOpenGL() const
{
    if (qApp->applicationName() == "krita" ) {
        QString canvasState = m_cfg.readEntry("canvasState");
        return (m_cfg.readEntry("useOpenGL", false) && (canvasState == "OPENGL_SUCCESS" || canvasState == "TRY_OPENGL"));
    }
    else if (qApp->applicationName() == "kritasketch" || qApp->applicationName() == "kritagemini") {
        return true; // for sketch and gemini
    }
    else {
        return false;
    }
}

void KisConfig::setUseOpenGL(bool useOpenGL) const
{
    m_cfg.writeEntry("useOpenGL", useOpenGL);
}

int KisConfig::openGLFilteringMode() const
{
    return m_cfg.readEntry("OpenGLFilterMode", 1);
}

void KisConfig::setOpenGLFilteringMode(int filteringMode)
{
    m_cfg.writeEntry("OpenGLFilterMode", filteringMode);
}

bool KisConfig::useOpenGLTextureBuffer() const
{
    return m_cfg.readEntry("useOpenGLTextureBuffer", false);
}

void KisConfig::setUseOpenGLTextureBuffer(bool useBuffer)
{
    m_cfg.writeEntry("useOpenGLTextureBuffer", useBuffer);
}

int KisConfig::openGLTextureSize() const
{
    return m_cfg.readEntry("textureSize", 256);
}

int KisConfig::numMipmapLevels() const
{
    return m_cfg.readEntry("numMipmapLevels", 4);
}

int KisConfig::textureOverlapBorder() const
{
    return 1 << qMax(0, numMipmapLevels());
}

qint32 KisConfig::maxNumberOfThreads()
{
    return m_cfg.readEntry("maxthreads", QThread::idealThreadCount());
}

void KisConfig::setMaxNumberOfThreads(qint32 maxThreads)
{
    m_cfg.writeEntry("maxthreads", maxThreads);
}

qint32 KisConfig::maxTilesInMem() const
{
    return m_cfg.readEntry("maxtilesinmem", DEFAULT_MAX_TILES_MEM);
}

void KisConfig::setMaxTilesInMem(qint32 tiles) const
{
    m_cfg.writeEntry("maxtilesinmem", tiles);
}

quint32 KisConfig::getGridMainStyle() const
{
    quint32 v = m_cfg.readEntry("gridmainstyle", 0);
    if (v > 2)
        v = 2;
    return v;
}

void KisConfig::setGridMainStyle(quint32 v) const
{
    m_cfg.writeEntry("gridmainstyle", v);
}

quint32 KisConfig::getGridSubdivisionStyle() const
{
    quint32 v = m_cfg.readEntry("gridsubdivisionstyle", 1);
    if (v > 2) v = 2;
    return v;
}

void KisConfig::setGridSubdivisionStyle(quint32 v) const
{
    m_cfg.writeEntry("gridsubdivisionstyle", v);
}

QColor KisConfig::getGridMainColor() const
{
    QColor col(99, 99, 99);
    return m_cfg.readEntry("gridmaincolor", col);
}

void KisConfig::setGridMainColor(const QColor & v) const
{
    m_cfg.writeEntry("gridmaincolor", v);
}

QColor KisConfig::getGridSubdivisionColor() const
{
    QColor col(150, 150, 150);
    return m_cfg.readEntry("gridsubdivisioncolor", col);
}

void KisConfig::setGridSubdivisionColor(const QColor & v) const
{
    m_cfg.writeEntry("gridsubdivisioncolor", v);
}

quint32 KisConfig::getGridHSpacing() const
{
    qint32 v = m_cfg.readEntry("gridhspacing", 10);
    return (quint32)qMax(1, v);
}

void KisConfig::setGridHSpacing(quint32 v) const
{
    m_cfg.writeEntry("gridhspacing", v);
}

quint32 KisConfig::getGridVSpacing() const
{
    qint32 v = m_cfg.readEntry("gridvspacing", 10);
    return (quint32)qMax(1, v);
}

void KisConfig::setGridVSpacing(quint32 v) const
{
    m_cfg.writeEntry("gridvspacing", v);
}

bool KisConfig::getGridSpacingAspect() const
{
    bool v = m_cfg.readEntry("gridspacingaspect", false);
    return v;
}

void KisConfig::setGridSpacingAspect(bool v) const
{
    m_cfg.writeEntry("gridspacingaspect", v);
}

quint32 KisConfig::getGridSubdivisions() const
{
    qint32 v = m_cfg.readEntry("gridsubsivisons", 2);
    return (quint32)qMax(1, v);
}

void KisConfig::setGridSubdivisions(quint32 v) const
{
    m_cfg.writeEntry("gridsubsivisons", v);
}

quint32 KisConfig::getGridOffsetX() const
{
    qint32 v = m_cfg.readEntry("gridoffsetx", 0);
    return (quint32)qMax(0, v);
}

void KisConfig::setGridOffsetX(quint32 v) const
{
    m_cfg.writeEntry("gridoffsetx", v);
}

quint32 KisConfig::getGridOffsetY() const
{
    qint32 v = m_cfg.readEntry("gridoffsety", 0);
    return (quint32)qMax(0, v);
}

void KisConfig::setGridOffsetY(quint32 v) const
{
    m_cfg.writeEntry("gridoffsety", v);
}

bool KisConfig::getGridOffsetAspect() const
{
    bool v = m_cfg.readEntry("gridoffsetaspect", false);
    return v;
}

void KisConfig::setGridOffsetAspect(bool v) const
{
    m_cfg.writeEntry("gridoffsetaspect", v);
}

qint32 KisConfig::checkSize() const
{
    return m_cfg.readEntry("checksize", 32);
}

void KisConfig::setCheckSize(qint32 checksize) const
{
    m_cfg.writeEntry("checksize", checksize);
}

bool KisConfig::scrollCheckers() const
{
    return m_cfg.readEntry("scrollingcheckers", false);
}

void KisConfig::setScrollingCheckers(bool sc) const
{
    m_cfg.writeEntry("scrollingcheckers", sc);
}

QColor KisConfig::canvasBorderColor() const
{
    QColor color(QColor(128,128,128));
    return m_cfg.readEntry("canvasBorderColor", color);
}

void KisConfig::setCanvasBorderColor(const QColor& color) const
{
    m_cfg.writeEntry("canvasBorderColor", color);
}


QColor KisConfig::checkersColor1() const
{
    QColor col(220, 220, 220);
    return m_cfg.readEntry("checkerscolor", col);
}

void KisConfig::setCheckersColor1(const QColor & v) const
{
    m_cfg.writeEntry("checkerscolor", v);
}

QColor KisConfig::checkersColor2() const
{
    return m_cfg.readEntry("checkerscolor2", QColor(Qt::white));
}

void KisConfig::setCheckersColor2(const QColor & v) const
{
    m_cfg.writeEntry("checkerscolor2", v);
}

bool KisConfig::antialiasCurves() const
{
    return m_cfg.readEntry("antialiascurves", true);
}

void KisConfig::setAntialiasCurves(bool v) const
{
    m_cfg.writeEntry("antialiascurves", v);
}

bool KisConfig::antialiasSelectionOutline() const
{
    return m_cfg.readEntry("AntialiasSelectionOutline", false);
}

void KisConfig::setAntialiasSelectionOutline(bool v) const
{
    m_cfg.writeEntry("AntialiasSelectionOutline", v);
}

bool KisConfig::showRootLayer() const
{
    return m_cfg.readEntry("ShowRootLayer", false);
}

void KisConfig::setShowRootLayer(bool showRootLayer) const
{
    m_cfg.writeEntry("ShowRootLayer", showRootLayer);
}

bool KisConfig::showOutlineWhilePainting() const
{
    return m_cfg.readEntry("ShowOutlineWhilePainting", true);
}

void KisConfig::setShowOutlineWhilePainting(bool showOutlineWhilePainting) const
{
    m_cfg.writeEntry("ShowOutlineWhilePainting", showOutlineWhilePainting);
}

qreal KisConfig::outlineSizeMinimum() const
{
    return m_cfg.readEntry("OutlineSizeMinimum", 1.0);
}

void KisConfig::setOutlineSizeMinimum(qreal outlineSizeMinimum) const
{
    m_cfg.writeEntry("OutlineSizeMinimum", outlineSizeMinimum);
}

int KisConfig::autoSaveInterval()  const
{
    return m_cfg.readEntry("AutoSaveInterval", KoDocument::defaultAutoSave());
}

void KisConfig::setAutoSaveInterval(int seconds)  const
{
    return m_cfg.writeEntry("AutoSaveInterval", seconds);
}

bool KisConfig::backupFile() const
{
    return m_cfg.readEntry("CreateBackupFile", true);
}

void KisConfig::setBackupFile(bool backupFile) const
{
    m_cfg.writeEntry("CreateBackupFile", backupFile);
}

bool KisConfig::showFilterGallery() const
{
    return m_cfg.readEntry("showFilterGallery", false);
}

void KisConfig::setShowFilterGallery(bool showFilterGallery) const
{
    m_cfg.writeEntry("showFilterGallery", showFilterGallery);
}

bool KisConfig::showFilterGalleryLayerMaskDialog() const
{
    return m_cfg.readEntry("showFilterGalleryLayerMaskDialog", true);
}

void KisConfig::setShowFilterGalleryLayerMaskDialog(bool showFilterGallery) const
{
    m_cfg.writeEntry("setShowFilterGalleryLayerMaskDialog", showFilterGallery);
}

QString KisConfig::defaultPainterlyColorModelId() const
{
    return m_cfg.readEntry("defaultpainterlycolormodel", "KS6");
}

void KisConfig::setDefaultPainterlyColorModelId(const QString& def) const
{
    m_cfg.writeEntry("defaultpainterlycolormodel", def);
}

QString KisConfig::defaultPainterlyColorDepthId() const
{
    return m_cfg.readEntry("defaultpainterlycolordepth", "F32");
}

void KisConfig::setDefaultPainterlyColorDepthId(const QString& def) const
{
    m_cfg.writeEntry("defaultpainterlycolordepth", def);
}

QString KisConfig::canvasState() const
{
    return m_cfg.readEntry("canvasState", "OPENGL_NOT_TRIED");
}

void KisConfig::setCanvasState(const QString& state) const
{
    static QStringList acceptableStates;
    if (acceptableStates.isEmpty()) {
        acceptableStates << "OPENGL_SUCCESS" << "TRY_OPENGL" << "OPENGL_NOT_TRIED" << "OPENGL_FAILED";
    }
    if (acceptableStates.contains(state)) {
        m_cfg.writeEntry("canvasState", state);
        m_cfg.sync();
    }
}

bool KisConfig::paintopPopupDetached() const
{
    return m_cfg.readEntry("PaintopPopupDetached", false);
}

void KisConfig::setPaintopPopupDetached(bool detached) const
{
    m_cfg.writeEntry("PaintopPopupDetached", detached);
}

QString KisConfig::pressureTabletCurve() const
{
    return m_cfg.readEntry("tabletPressureCurve","0,0;1,1;");
}

void KisConfig::setPressureTabletCurve(const QString& curveString) const
{
    m_cfg.writeEntry("tabletPressureCurve", curveString);
}

qreal KisConfig::vastScrolling() const
{
    return m_cfg.readEntry("vastScrolling", 0.9);
}

void KisConfig::setVastScrolling(const qreal factor) const
{
    m_cfg.writeEntry("vastScrolling", factor);
}

int KisConfig::presetChooserViewMode() const
{
    return m_cfg.readEntry("presetChooserViewMode", 0);
}

void KisConfig::setPresetChooserViewMode(const int mode) const
{
    m_cfg.writeEntry("presetChooserViewMode", mode);
}

bool KisConfig::firstRun() const
{
    return m_cfg.readEntry("firstRun", true);
}

void KisConfig::setFirstRun(const bool first) const
{
    m_cfg.writeEntry("firstRun", first);
}

int KisConfig::horizontalSplitLines() const
{
    return m_cfg.readEntry("horizontalSplitLines", 1);
}
void KisConfig::setHorizontalSplitLines(const int numberLines) const
{
    m_cfg.writeEntry("horizontalSplitLines", numberLines);
}

int KisConfig::verticalSplitLines() const
{
    return m_cfg.readEntry("verticalSplitLines", 1);
}

void KisConfig::setVerticalSplitLines(const int numberLines) const
{
    m_cfg.writeEntry("verticalSplitLines", numberLines);
}

bool KisConfig::clicklessSpacePan() const
{
    return m_cfg.readEntry("clicklessSpacePan", true);
}

void KisConfig::setClicklessSpacePan(const bool toggle) const
{
    m_cfg.writeEntry("clicklessSpacePan", toggle);
}


int KisConfig::hideDockersFullscreen() const
{
    return m_cfg.readEntry("hideDockersFullScreen", (int)Qt::Checked);
}

void KisConfig::setHideDockersFullscreen(const int value) const
{
    m_cfg.writeEntry("hideDockersFullScreen", value);
}

int KisConfig::hideMenuFullscreen() const
{
    return m_cfg.readEntry("hideMenuFullScreen", (int)Qt::Checked);
}

void KisConfig::setHideMenuFullscreen(const int value) const
{
    m_cfg.writeEntry("hideMenuFullScreen", value);
}

int KisConfig::hideScrollbarsFullscreen() const
{
    return m_cfg.readEntry("hideScrollbarsFullScreen", (int)Qt::Checked);
}

void KisConfig::setHideScrollbarsFullscreen(const int value) const
{
    m_cfg.writeEntry("hideScrollbarsFullScreen", value);
}

int KisConfig::hideStatusbarFullscreen() const
{
    return m_cfg.readEntry("hideStatusbarFullScreen", (int)Qt::Checked);
}

void KisConfig::setHideStatusbarFullscreen(const int value) const
{
    m_cfg.writeEntry("hideStatusbarFullScreen", value);
}

int KisConfig::hideTitlebarFullscreen() const
{
    return m_cfg.readEntry("hideTitleBarFullscreen", (int)Qt::Checked);
}

void KisConfig::setHideTitlebarFullscreen(const int value) const
{
    m_cfg.writeEntry("hideTitleBarFullscreen", value);
}

int KisConfig::hideToolbarFullscreen() const
{
    return m_cfg.readEntry("hideToolbarFullscreen", (int)Qt::Checked);
}

void KisConfig::setHideToolbarFullscreen(const int value) const
{
    m_cfg.writeEntry("hideToolbarFullscreen", value);
}

QStringList KisConfig::favoriteCompositeOps() const
{
    return m_cfg.readEntry("favoriteCompositeOps", QStringList());
}

void KisConfig::setFavoriteCompositeOps(const QStringList& compositeOps) const
{
    m_cfg.writeEntry("favoriteCompositeOps", compositeOps);
}

QString KisConfig::exportConfiguration(const QString &filterId) const
{
    return m_cfg.readEntry("ExportConfiguration-" + filterId, QString());
}

void KisConfig::setExportConfiguration(const QString &filterId, const KisPropertiesConfiguration &properties) const
{
    QString exportConfig = properties.toXML();
    m_cfg.writeEntry("ExportConfiguration-" + filterId, exportConfig);

}

bool KisConfig::useOcio() const
{
#ifdef HAVE_OCIO
    return m_cfg.readEntry("Krita/Ocio/UseOcio", false);
#else
    return false;
#endif
}

void KisConfig::setUseOcio(bool useOCIO) const
{
    m_cfg.writeEntry("Krita/Ocio/UseOcio", useOCIO);
}

bool KisConfig::useOcioEnvironmentVariable() const
{
    return m_cfg.readEntry("Krita/Ocio/UseEnvironment", false);
}

void KisConfig::setUseOcioEnvironmentVariable(bool useOCIO) const
{
    m_cfg.writeEntry("Krita/Ocio/UseEnvironment", useOCIO);
}

QString KisConfig::ocioConfigurationPath() const
{
    return m_cfg.readEntry("Krita/Ocio/OcioConfigPath", QString());
}

void KisConfig::setOcioConfigurationPath(const QString &path) const
{
    m_cfg.writeEntry("Krita/Ocio/OcioConfigPath", path);
}

QString KisConfig::ocioLutPath() const
{
    return m_cfg.readEntry("Krita/Ocio/OcioLutPath", QString());
}

void KisConfig::setOcioLutPath(const QString &path) const
{
    m_cfg.writeEntry("Krita/Ocio/OcioLutPath", path);
}

QString KisConfig::defaultPalette() const
{
    return m_cfg.readEntry("defaultPalette", QString());
}

void KisConfig::setDefaultPalette(const QString& name) const
{
    m_cfg.writeEntry("defaultPalette", name);
}

QString KisConfig::toolbarSlider(int sliderNumber)
{
    QString def = "flow";
    if (sliderNumber == 1) {
        def = "opacity";
    }
    if (sliderNumber == 2) {
        def = "size";
    }
    return m_cfg.readEntry(QString("toolbarslider_%1").arg(sliderNumber), def);
}

void KisConfig::setToolbarSlider(int sliderNumber, const QString &slider)
{
    m_cfg.writeEntry(QString("toolbarslider_%1").arg(sliderNumber), slider);
}

QString KisConfig::currentInputProfile() const
{
    return m_cfg.readEntry("currentInputProfile", QString());
}

void KisConfig::setCurrentInputProfile(const QString& name)
{
    m_cfg.writeEntry("currentInputProfile", name);
}

bool KisConfig::useSystemMonitorProfile() const
{
    return m_cfg.readEntry("ColorManagement/UseSystemMonitorProfile", false);
}

void KisConfig::setUseSystemMonitorProfile(bool _useSystemMonitorProfile) const
{
    m_cfg.writeEntry("ColorManagement/UseSystemMonitorProfile", _useSystemMonitorProfile);
}

bool KisConfig::presetStripVisible() const
{
    return m_cfg.readEntry("presetStripVisible", true);
}

void KisConfig::setPresetStripVisible(bool visible)
{
    m_cfg.writeEntry("presetStripVisible", visible);
}

bool KisConfig::scratchpadVisible() const
{
    return m_cfg.readEntry("scratchpadVisible", true);
}

void KisConfig::setScratchpadVisible(bool visible)
{
    m_cfg.writeEntry("scratchpadVisible", visible);
}


bool KisConfig::showSingleChannelAsColor() const
{
    return m_cfg.readEntry("showSingleChannelAsColor", false);
}

void KisConfig::setShowSingleChannelAsColor(bool asColor)
{
    m_cfg.writeEntry("showSingleChannelAsColor", asColor);
}

int KisConfig::numDefaultLayers() const
{
    return m_cfg.readEntry("NumberOfLayersForNewImage", 2);
}

void KisConfig::setNumDefaultLayers(int num)
{
    m_cfg.writeEntry("NumberOfLayersForNewImage", num);
}
