/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2007 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_filter_handler.h"

#include <QApplication>

#include <kmessagebox.h>
#include <kguiitem.h>

#include <KoColorSpace.h>
#include <KoProgressUpdater.h>

#include <kis_action_recorder.h>
#include <kis_bookmarked_configuration_manager.h>
#include <kis_filter.h>
#include <kis_filter_configuration.h>
#include <kis_filter_processing_information.h>
#include <kis_layer.h>
#include <kis_recorded_filter_action.h>

#include "kis_statusbar.h"
#include "kis_doc2.h"
#include "kis_filter_dialog.h"
#include "kis_filter_manager.h"
#include "kis_transaction.h"
#include "kis_view2.h"
#include "kis_painter.h"
#include "kis_threaded_applicator.h"
#include "kis_filter_job.h"

struct KisFilterHandler::Private {

    Private()
        : view(0)
        , manager(0)
        , lastConfiguration(0)
        {
        }

    ~Private()
        {
        }

    KisFilterSP filter;
    KisView2* view;
    KisFilterManager* manager;
    KisFilterConfiguration* lastConfiguration;
};

KisFilterHandler::KisFilterHandler(KisFilterManager* parent, KisFilterSP f, KisView2* view) 
    : QObject(parent)
    , m_d(new Private)
{
    m_d->filter = f;
    m_d->view = view;
    m_d->manager = parent;
}

KisFilterHandler::~KisFilterHandler()
{
}

void KisFilterHandler::showDialog()
{
    KisPaintDeviceSP dev = m_d->view->activeDevice();
    if (dev->colorSpace()->willDegrade(m_d->filter->colorSpaceIndependence())) {
        // Warning bells!
        if (m_d->filter->colorSpaceIndependence() == TO_LAB16) {
            if (KMessageBox::warningContinueCancel(m_d->view,
                                                   i18n("The %1 filter will convert your %2 data to 16-bit L*a*b* and vice versa. ",
                                                        m_d->filter->name(),
                                                        dev->colorSpace()->name()),
                                                   i18n("Filter Will Convert Your Layer Data"),
                                                   KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                                                   "lab16degradation") != KMessageBox::Continue) return;

        }
        else if (m_d->filter->colorSpaceIndependence() == TO_RGBA16) {
            if (KMessageBox::warningContinueCancel(m_d->view,
                                                   i18n("The %1 filter will convert your %2 data to 16-bit RGBA and vice versa. ",
                                                        m_d->filter->name() , dev->colorSpace()->name()),
                                                   i18n("Filter Will Convert Your Layer Data"),
                                                   KStandardGuiItem::cont(), KStandardGuiItem::cancel(),
                                                   "rgba16degradation") != KMessageBox::Continue) return;
        }
    }


    KisFilterDialog* dialog = new KisFilterDialog( m_d->view , m_d->view->activeLayer());
    dialog->setFilter( m_d->filter );
    connect(dialog, SIGNAL(sigPleaseApplyFilter(KisLayerSP, KisFilterConfiguration*)),
            SLOT(apply(KisLayerSP, KisFilterConfiguration*)));
    dialog->setVisible(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void KisFilterHandler::reapply()
{
    apply(m_d->view->activeLayer(), m_d->lastConfiguration);
}

void KisFilterHandler::apply(KisLayerSP layer, KisFilterConfiguration* config)
{
    kDebug(41007) <<"Applying a filter";
    if( not layer ) return;

    KisPaintDeviceSP dev = layer->paintDevice();

    QRect r1 = dev->extent();
    QRect r2 = layer->image()->bounds();

    // Filters should work only on the visible part of an image.
    QRect rect = r1.intersect(r2);

    if (KisSelectionSP selection = layer->selection() ) {
        QRect r3 = selection->selectedExactRect();
        rect = rect.intersect(r3);
    }

    KisTransaction * cmd = 0;
    if (layer->image()->undo()) cmd = new KisTransaction(m_d->filter->name(), dev);

    if ( !m_d->filter->supportsThreading() ) {
        m_d->filter->process(dev, rect, config);
    }
    else {
        // Chop up in rects.
        KisFilterJobFactory factory( m_d->filter, config );
        KoProgressUpdater updater( m_d->view->statusBar()->progress() );
        KisThreadedApplicator applicator(dev, rect, &factory, &updater, m_d->filter->overlapMarginNeeded( config ));
        applicator.execute();
    }
/*    if (m_d->filter->cancelRequested()) { // TODO: port to the progress display reporter
        delete config;
        if (cmd) {
            cmm_d->undo();
            delete cmd;
        }
    } else */{
        dev->setDirty(rect);
        m_d->view->document()->setModified(true);
        if (cmd) m_d->view->document()->addCommand(cmd);
        if(m_d->filter->bookmarkManager())
        {
            m_d->filter->bookmarkManager()->save(KisBookmarkedConfigurationManager::ConfigLastUsed.id(), config);
        }
        if(m_d->lastConfiguration != config)
        {
            delete m_d->lastConfiguration;
        }
        m_d->lastConfiguration = config;
        m_d->manager->setLastFilterHandler(this);

        layer->image()->actionRecorder()->addAction( KisRecordedFilterAction(m_d->filter->name(), layer, m_d->filter, config));
    }

    QApplication::restoreOverrideCursor();
}

const KisFilterSP KisFilterHandler::filter() const
{
    return m_d->filter;
}


#include "kis_filter_handler.moc"
