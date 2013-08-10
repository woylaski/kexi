/*
 * Copyright (c) 2005-2009 Thomas Zander <zander@kde.org>
 * Copyright (c) 2009 Peter Simonsson <peter.simonsson@gmail.com>
 * Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2011 C. Boemann <cbo@boemann.dk>
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

#include "KoModeBox_p.h"

#include <KoCanvasControllerWidget.h>
#include <KoCanvasBase.h>
#include <KoToolManager.h>
#include <KoShapeLayer.h>
#include <KoInteractionTool.h>

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kconfiggroup.h>
#include <klocale.h>
#include <kselectaction.h>

#include <QMap>
#include <QList>
#include <QToolButton>
#include <QHash>
#include <QSet>
#include <QRect>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QApplication>
#include <QTabBar>
#include <QStackedWidget>
#include <QPainter>
#include <QTextLayout>
#include <QMenu>
#include <QScrollArea>

class KoModeBox::Private
{
public:
    Private(KoCanvasController *c)
        : canvas(c->canvas())
        , activeId(-1)
        , iconTextFitted(true)
        , fittingIterations(0)
        , iconMode(IconAndText)
    {
    }

    KoCanvasBase *canvas;
    QList<KoToolButton> buttons; // buttons maintained by toolmanager
    QList<KoToolButton> addedButtons; //buttons in the order added to QToolBox
    QMap<int, QWidget *> addedWidgets;
    QSet<QWidget *> currentAuxWidgets;
    int activeId;
    QTabBar *tabBar;
    QStackedWidget *stack;
    bool iconTextFitted;
    int fittingIterations;
    IconMode iconMode;
};

QString KoModeBox::applicationName;

static bool compareButton(const KoToolButton &b1, const KoToolButton &b2)
{
    int b1Level;
    int b2Level;
    if (b1.section.contains(KoModeBox::applicationName)) {
        b1Level = 0;
    } else if (b1.section.contains("main")) {
        b1Level = 1;
    } else {
        b1Level = 2;
    }

    if (b2.section.contains(KoModeBox::applicationName)) {
        b2Level = 0;
    } else if (b2.section.contains("main")) {
        b2Level = 1;
    } else {
        b2Level = 2;
    }

    if (b1Level == b2Level) {
        return b1.priority < b2.priority;
    } else {
        return b1Level < b2Level;
    }
}


KoModeBox::KoModeBox(KoCanvasControllerWidget *canvas, const QString &appName)
    : QWidget()
    , d(new Private(canvas))
{
    applicationName = appName;

    KConfigGroup cfg = KGlobal::config()->group("calligra");
    d->iconMode = (IconMode)cfg.readEntry("ModeBoxIconMode", (int)IconAndText);

    QGridLayout *layout = new QGridLayout();
    d->tabBar = new QTabBar();
    d->tabBar->setShape(QTabBar::RoundedWest);
    d->tabBar->setExpanding(false);
    if (d->iconMode == IconAndText) {
        d->tabBar->setIconSize(QSize(32,64));
    } else {
        d->tabBar->setIconSize(QSize(22,22));
    }
    d->tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(d->tabBar, 0, 0);

    d->stack = new QStackedWidget();
    layout->addWidget(d->stack, 0, 1);

    layout->setContentsMargins(0,0,0,0);
    layout->setColumnStretch(1, 100);
    setLayout(layout);

    foreach(const KoToolButton &button, KoToolManager::instance()->createToolList(canvas->canvas())) {
        addButton(button);
    }

    qSort(d->buttons.begin(), d->buttons.end(), compareButton);

    // Update visibility of buttons
    updateShownTools(canvas, QList<QString>());

    d->tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(d->tabBar, SIGNAL(currentChanged(int)), this, SLOT(toolSelected(int)));
    connect(d->tabBar, SIGNAL(currentChanged(int)), d->stack, SLOT(setCurrentIndex(int)));
    connect(d->tabBar, SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotContextMenuRequested(QPoint)));

    connect(KoToolManager::instance(), SIGNAL(changedTool(KoCanvasController *, int)),
            this, SLOT(setActiveTool(KoCanvasController *, int)));
    connect(KoToolManager::instance(), SIGNAL(currentLayerChanged(const KoCanvasController *,const KoShapeLayer*)),
            this, SLOT(setCurrentLayer(const KoCanvasController *,const KoShapeLayer *)));
    connect(KoToolManager::instance(), SIGNAL(toolCodesSelected(const KoCanvasController*, QList<QString>)),
            this, SLOT(updateShownTools(const KoCanvasController *, QList<QString>)));
    connect(KoToolManager::instance(),
            SIGNAL(addedTool(const KoToolButton, KoCanvasController*)),
            this, SLOT(toolAdded(const KoToolButton, KoCanvasController*)));

    connect(canvas, SIGNAL(toolOptionWidgetsChanged(const QList<QWidget *> &)),
         this, SLOT(setOptionWidgets(const QList<QWidget *> &)));
}

KoModeBox::~KoModeBox()
{
    delete d;
}

void KoModeBox::addButton(const KoToolButton &button)
{
    d->buttons.append(button);
    button.button->setVisible(false);
}

void KoModeBox::setActiveTool(KoCanvasController *canvas, int id)
{
    if (canvas->canvas() == d->canvas) {
        d->activeId = id;
        d->tabBar->blockSignals(true);
        int i = 0;
        foreach (const KoToolButton &button, d->addedButtons) {
            if (button.buttonGroupId == d->activeId) {
                d->tabBar->setCurrentIndex(i);
                d->stack->setCurrentIndex(i);
                break;
            }
            ++i;
        }
        d->tabBar->blockSignals(false);
        return;
    }
}

QIcon KoModeBox::createRotatedIcon(const KoToolButton button)
{
    QSize iconSize = d->tabBar->iconSize();
    QFont smallFont  = KGlobalSettings::generalFont();
    qreal pointSize = KGlobalSettings::smallestReadableFont().pointSizeF();
    smallFont.setPointSizeF(pointSize);
    // This must be a QImage, as drawing to a QPixmap outside the
    // UI thread will cause sporadic crashes.
    QImage pm(iconSize, QImage::Format_ARGB32_Premultiplied);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.rotate(90);
    p.translate(0,-iconSize.width());

    button.button->icon().paint(&p, 0, 0, iconSize.height(), 22);

    QTextLayout textLayout(button.button->toolTip(), smallFont, p.device());
    QTextOption option(Qt::AlignTop | Qt::AlignHCenter);
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(option);
    textLayout.beginLayout();
    qreal height = 0;
    while (1) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(iconSize.height());
        line.setPosition(QPointF(0, height));
        height += line.height();
    }
    textLayout.endLayout();

    if (textLayout.lineCount() > 2) {
        iconSize.setHeight(iconSize.height() + 8);
        d->tabBar->setIconSize(iconSize);
        d->iconTextFitted = false;
    } else if (height > iconSize.width() - 22) {
        iconSize.setWidth(22 + height);
        d->tabBar->setIconSize(iconSize);
        d->iconTextFitted = false;
    }

    p.setFont(smallFont);
    textLayout.draw(&p, QPoint(0, 22));
    p.end();

    return QIcon(QPixmap::fromImage(pm));
}

QIcon KoModeBox::createSimpleIcon(const KoToolButton button)
{
    QSize iconSize = d->tabBar->iconSize();

    // This must be a QImage, as drawing to a QPixmap outside the
    // UI thread will cause sporadic crashes.
    QImage pm(iconSize, QImage::Format_ARGB32_Premultiplied);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.rotate(90);
    p.translate(0,-iconSize.width());

    button.button->icon().paint(&p, 0, 0, iconSize.height(), iconSize.width());

    return QIcon(QPixmap::fromImage(pm));
}

void KoModeBox::addItem(const KoToolButton button)
{
    QWidget *oldwidget = d->addedWidgets[button.buttonGroupId];
    QWidget *widget;

    // We need to create a new widget in all cases as QToolBox seeems to crash if we reuse
    // a widget (even though the item had been removed)
    QLayout *layout;
    if (!oldwidget) {
        layout = new QGridLayout();
    } else {
        layout = oldwidget->layout();
    }
    widget = new QWidget();
    widget->setLayout(layout);
    layout->setContentsMargins(0,0,0,0);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    d->addedWidgets[button.buttonGroupId] = widget;

    // Create a rotated icon with text
    d->tabBar->blockSignals(true);
    if (d->iconMode == IconAndText) {
        d->tabBar->addTab(createRotatedIcon(button), QString());
    } else {
        int index = d->tabBar->addTab(createSimpleIcon(button), QString());
        d->tabBar->setTabToolTip(index, button.button->toolTip());
    }
    d->tabBar->blockSignals(false);
    QScrollArea *sa = new QScrollArea();
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setWidgetResizable(true);
    sa->setContentsMargins(0,0,0,0);
    sa->setWidget(widget);
    sa->setFrameShape(QFrame::NoFrame);
    d->stack->addWidget(sa);
    d->addedButtons.append(button);
}

void KoModeBox::updateShownTools(const KoCanvasController *canvas, const QList<QString> &codes)
{
    if (canvas->canvas() != d->canvas) {
        return;
    }

    if (d->iconTextFitted) {
        d->fittingIterations = 0;
    }
    d->iconTextFitted = true;

    d->tabBar->blockSignals(true);

    while (d->tabBar->count()) {
        d->tabBar->removeTab(0);
        d->stack->removeWidget(d->stack->widget(0));
    }

    d->addedButtons.clear();

    int newIndex = -1;
    foreach (const KoToolButton button, d->buttons) {
        QString code = button.visibilityCode;
        if (button.buttonGroupId == d->activeId) {
            newIndex = d->addedButtons.length();
        }
        if (button.section.contains(applicationName)) {
            addItem(button);
            continue;
        } else if (!button.section.contains("dynamic")
            && !button.section.contains("main")) {
            continue;
        }

        if (code.startsWith(QLatin1String("flake/"))) {
            addItem(button);
            continue;
        }

        if (code.endsWith( QLatin1String( "/always"))) {
            addItem(button);
            continue;
        } else if (code.isEmpty() && codes.count() != 0) {
            addItem(button);
            continue;
        } else if (codes.contains(code)) {
            addItem(button);
            continue;
        }
    }
    if (newIndex != -1) {
        d->tabBar->setCurrentIndex(newIndex);
    }
    d->tabBar->blockSignals(false);

    if (!d->iconTextFitted &&  d->fittingIterations++ < 8) {
        updateShownTools(canvas, codes);
    }
    d->iconTextFitted = true;
}

void KoModeBox::setOptionWidgets(const QList<QWidget *> &optionWidgetList)
{
    if (! d->addedWidgets.contains(d->activeId)) return;

    // For some reason we need to set some attr on our placeholder widget here
    // eventhough these settings should be default
    // Otherwise Sheets' celltool's optionwidget looks ugly
    d->addedWidgets[d->activeId]->setAutoFillBackground(false);
    d->addedWidgets[d->activeId]->setBackgroundRole(QPalette::NoRole);

    qDeleteAll(d->currentAuxWidgets);
    d->currentAuxWidgets.clear();

    int cnt = 0;
    QGridLayout *layout = (QGridLayout *)d->addedWidgets[d->activeId]->layout();

    // need to unstretch row that have previously been stretched
    layout->setRowStretch(layout->rowCount()-1, 0);
    layout->setColumnMinimumWidth(0, 0); // used to be indent
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 2);
    layout->setColumnStretch(3, 1);
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(2);
    int specialCount = 0;
    foreach(QWidget *widget, optionWidgetList) {
        if (!widget->windowTitle().isEmpty()) {
            QLabel *l;
            layout->addWidget(l = new QLabel(widget->windowTitle()), cnt++, 1, 1, 3, Qt::AlignHCenter);
            d->currentAuxWidgets.insert(l);
        }
        layout->addWidget(widget, cnt++, 1, 1, 3);
        QLayout *subLayout = widget->layout();
        if (subLayout) {
            for (int i = 0; i < subLayout->count(); ++i) {
                QWidget *spacerWidget = subLayout->itemAt(i)->widget();
                if (spacerWidget && spacerWidget->objectName().contains("SpecialSpacer")) {
                    specialCount++;
                }
            }
        }
        widget->show();
        if (widget != optionWidgetList.last()) {
            QFrame *s;
            layout->addWidget(s = new QFrame(), cnt, 2, 1, 1);
            layout->setRowMinimumHeight(cnt++, 16);
            s->setFrameStyle(QFrame::HLine | QFrame::Sunken);
            d->currentAuxWidgets.insert(s);
        }
    }
    if (specialCount == optionWidgetList.count()) {
        layout->setRowStretch(cnt, 100);
    }
}

void KoModeBox::setCurrentLayer(const KoCanvasController *canvas, const KoShapeLayer *layer)
{
    Q_UNUSED(canvas);
    Q_UNUSED(layer);
    //Since tageted application don't use this we won't bother implemeting
}

void KoModeBox::setCanvas(KoCanvasBase *canvas)
{
    KoCanvasControllerWidget *ccwidget;

    if (d->canvas) {
        ccwidget = dynamic_cast<KoCanvasControllerWidget *>(d->canvas->canvasController());
        disconnect(ccwidget, SIGNAL(toolOptionWidgetsChanged(const QList<QWidget *> &)),
                    this, SLOT(setOptionWidgets(const QList<QWidget *> &)));
    }

    d->canvas = canvas;

    ccwidget = dynamic_cast<KoCanvasControllerWidget *>(d->canvas->canvasController());
    connect(
        ccwidget, SIGNAL(toolOptionWidgetsChanged(const QList<QWidget *> &)),
         this, SLOT(setOptionWidgets(const QList<QWidget *> &)));
}

void KoModeBox::unsetCanvas()
{
    d->canvas = 0;
}

void KoModeBox::toolAdded(const KoToolButton &button, KoCanvasController *canvas)
{
    if (canvas->canvas() == d->canvas) {
        addButton(button);

        qSort(d->buttons.begin(), d->buttons.end(), compareButton);

        updateShownTools(canvas, QList<QString>());
    }
}

void KoModeBox::toolSelected(int index)
{
    if (index != -1) {
        d->addedButtons[index].button->click();
    }
}

void KoModeBox::slotContextMenuRequested(const QPoint &pos)
{
    QMenu menu;
    KSelectAction* selectionAction = new KSelectAction(i18n("Text"), &menu);
    connect(selectionAction, SIGNAL(triggered(int)), SLOT(switchIconMode(int)));
    menu.addAction(selectionAction);
    selectionAction->addAction(i18n("Icon and Text"));
    selectionAction->addAction(i18n("Icon only"));
    selectionAction->setCurrentItem(d->iconMode);

    menu.exec(d->tabBar->mapToGlobal(pos));
}

void KoModeBox::switchIconMode(int mode)
{
    d->iconMode = static_cast<IconMode>(mode);
    if (d->iconMode == IconAndText) {
        d->tabBar->setIconSize(QSize(32,64));
    } else {
        d->tabBar->setIconSize(QSize(22,22));
    }
    updateShownTools(d->canvas->canvasController(), QList<QString>());

    KConfigGroup cfg = KGlobal::config()->group("calligra");
    cfg.writeEntry("ModeBoxIconMode", (int)d->iconMode);

}
