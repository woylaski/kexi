/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>

#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>

#include <qcolor.h>

#include "kis_clipboard.h"
#include "kis_types.h"
#include "kis_view.h"
#include "kis_doc.h"
#include "kis_image.h"
#include "kis_selection.h"
#include "kis_selection_manager.h"
#include "kis_painter.h"
#include "kis_iterators_pixel.h"
#include "kis_layer.h"
#include "kis_paint_device.h"
#include "kis_colorspace_registry.h"
#include "kis_dlg_apply_profile.h"
#include "kis_config.h"
#include "kis_global.h"

KisSelectionManager::KisSelectionManager(KisView * parent, KisDoc * doc)
	: m_parent(parent),
	  m_doc(doc),
	  m_previousSelection(0),
	  m_copy(0),
	  m_cut(0),
	  m_paste(0),
	  m_selectAll(0),
	  m_deselect(0),
	  m_clear(0),
	  m_reselect(0),
	  m_invert(0),
	  m_toNewLayer(0),
	  m_feather(0),
	  m_border(0),
	  m_expand(0),
	  m_smooth(0),
	  m_contract(0),
	  m_grow(0),
	  m_similar(0),
	  m_transform(0),
	  m_load(0),
	  m_save(0)
{
	m_pluginActions.setAutoDelete(true);
	m_clipboard = KisClipboard::instance();
}

KisSelectionManager::~KisSelectionManager()
{
	m_pluginActions.clear();
}


void KisSelectionManager::setup(KActionCollection * collection)
{
	// XXX: setup shortcuts!

        m_cut =
		KStdAction::cut(this,
				SLOT(cut()),
				collection,
				"cut");

        m_copy =
		KStdAction::copy(this,
				 SLOT(copy()),
				 collection,
				 "copy");

        m_paste =
		KStdAction::paste(this,
				  SLOT(paste()),
				  collection,
				  "paste");

        m_selectAll =
		KStdAction::selectAll(this,
				      SLOT(selectAll()),
				      collection,
				      "select_all");

        m_deselect =
		KStdAction::deselect(this,
				     SLOT(unSelectAll()),
				     collection,
				     "deselect");


        m_clear =
		KStdAction::clear(this,
				  SLOT(clear()),
				  collection,
				  "clear");

	m_reselect =
		new KAction(i18n("&Reselect"),
			    0, 0,
			    this, SLOT(reselect()),
			    collection, "reselect");


	m_invert =
		new KAction(i18n("&Invert"),
			    0, 0,
			    this, SLOT(invert()),
			    collection, "invert");


        m_toNewLayer =
		new KAction(i18n("Copy Selection to New Layer"),
			    0, 0,
			    this, SLOT(copySelectionToNewLayer()),
			    collection, "copy_selection_to_new_layer");

	m_feather =
		new KAction(i18n("Feather..."),
			    0, 0,
			    this, SLOT(feather()),
			    collection, "feather");

	m_border =
		new KAction(i18n("Border..."),
			    0, 0,
			    this, SLOT(border()),
			    collection, "border");

	m_expand =
		new KAction(i18n("Expand..."),
			    0, 0,
			    this, SLOT(expand()),
			    collection, "expand");

	m_smooth =
		new KAction(i18n("Smooth..."),
			    0, 0,
			    this, SLOT(smooth()),
			    collection, "smooth");


	m_contract =
		new KAction(i18n("Contract..."),
			    0, 0,
			    this, SLOT(contract()),
			    collection, "contract");

	m_grow =
		new KAction(i18n("Grow"),
			    0, 0,
			    this, SLOT(grow()),
			    collection, "grow");

	m_similar =
		new KAction(i18n("Similar"),
			    0, 0,
			    this, SLOT(similar()),
			    collection, "similar");


	m_transform
		= new KAction(i18n("Transform..."),
			      0, 0,
			      this, SLOT(transform()),
			      collection, "transform_selection");


	m_load
		= new KAction(i18n("Load..."),
			      0, 0,
			      this, SLOT(load()),
			      collection, "load_selection");


	m_save
		= new KAction(i18n("Save As..."),
			      0, 0,
			      this, SLOT(save()),
			      collection, "save_selection");


        QClipboard *cb = QApplication::clipboard();
        connect(cb, SIGNAL(dataChanged()), SLOT(clipboardDataChanged()));
}

void KisSelectionManager::clipboardDataChanged()
{
  updateGUI();
}


void KisSelectionManager::addSelectionAction(KAction * action)
{
	m_pluginActions.append(action);
}


void KisSelectionManager::updateGUI()
{
        KisImageSP img = m_parent -> currentImg();

        bool enable = img && img -> activeLayer() && img -> activeLayer() -> hasSelection();

	m_copy -> setEnabled(enable);
	m_cut -> setEnabled(enable);
	m_paste -> setEnabled(img != 0 && m_clipboard -> hasClip());
	m_selectAll -> setEnabled(img != 0);
	m_deselect -> setEnabled(enable);
	m_clear -> setEnabled(enable);
	m_reselect -> setEnabled(m_previousSelection != 0);
	m_invert -> setEnabled(enable);
	m_toNewLayer -> setEnabled(enable);
	m_feather -> setEnabled(enable);
	m_border -> setEnabled(enable);
	m_expand -> setEnabled(enable);
	m_smooth -> setEnabled(enable);
	m_contract -> setEnabled(enable);
	m_grow -> setEnabled(enable);
	m_similar -> setEnabled(enable);
	m_transform -> setEnabled(enable);
	m_load -> setEnabled(enable);
	m_save -> setEnabled(enable);

	m_parent -> updateStatusBarSelectionLabel();

// 	KAction a;
// 	for (Q_UINT32 i = 0; i < m_pluginActions.count(); ++i) {
// 		// ??? What was supposed to happen here?
// 	}
}

void KisSelectionManager::imgSelectionChanged(KisImageSP img)
{
        if (img == m_parent -> currentImg()) {
                updateGUI();
		m_parent -> updateCanvas();
	}

}

void KisSelectionManager::cut()
{
        copy();
        clear();
}

void KisSelectionManager::copy()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (!layer -> hasSelection()) return;

	KisSelectionSP selection = layer -> selection();
	QRect r = selection -> selectedRect();
	r = r.normalize();

// 	kdDebug() << "Selection rect: "
// 		  << r.x() << ", "
// 		  << r.y() << ", "
// 		  << r.width() << ", "
// 		  << r.height() << "\n";

	KisPaintDeviceSP clip = new KisPaintDevice(img -> activeDevice() -> colorStrategy(), 
						   "Copy from " + img -> activeDevice() -> name() );
	clip -> setCompositeOp(COMPOSITE_OVER);
	clip -> setProfile(layer -> profile());

	// TODO if the source is linked... copy from all linked layers?!?

	KisPainter gc;
	gc.begin(clip);
	// Copy image data
	gc.bitBlt(0, 0, COMPOSITE_COPY, layer.data(), r.x() - layer->getX(), r.y() - layer->getY(), r.width(), r.height());
	// Apply selection mask.
	selection -> invert(QRect(r.x() - layer->getX(), r.y() - layer->getY(), r.width(), r.height()));
	gc.bitBlt(0, 0, COMPOSITE_COPY_OPACITY, selection.data(), r.x() - layer->getX(), r.y() - layer->getY(), r.width(), r.height());
	selection -> invert(QRect(r.x() - layer->getX(), r.y() - layer->getY(), r.width(), r.height()));
	gc.end();

//      kdDebug() << "Selection copied: "
//                << r.x() << ", "
//                << r.y() << ", "
//                << r.width() << ", "
//                << r.height() << "\n";


 	m_clipboard -> setClip(clip);
 	imgSelectionChanged(m_parent -> currentImg());

}


KisLayerSP KisSelectionManager::paste()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return 0;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return 0;

	KisPaintDeviceSP clip = m_clipboard -> clip();

	if (clip) {
		KisLayerSP layer = new KisLayer(img, img -> nextLayerName() + "(pasted)", OPACITY_OPAQUE);
		KisPainter gc;
		gc.begin(layer.data());
		gc.bitBlt(0, 0, COMPOSITE_COPY, clip.data(), clip->getX(), clip->getY(), 0, 0);
		gc.end();
		
		KisConfig cfg;
		if (cfg.askProfileOnPaste() && clip -> profile() == 0 && img -> profile() != 0) {
			KisDlgApplyProfile * dlg = new KisDlgApplyProfile(m_parent);
			if (dlg -> exec() == QDialog::Accepted) {
				KisProfileSP profile = dlg -> profile();
				if (profile != img -> profile()) {
					layer -> setProfile(profile);
					layer -> convertTo(img -> colorStrategy(), img -> profile(), dlg -> renderIntent());
				}

			}
		}


		m_doc -> layerAdd(img, layer, img -> index(layer));
		layer -> move(0,0);
		img -> notify();

		return layer;
	}
	return 0;
}


void KisSelectionManager::selectAll()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	KisSelectionSP s = new KisSelection(KisPaintDeviceSP(layer), "layer selection for: " + layer -> name());
	s -> select(QRect(0, 0, img -> width(), img -> height())); //AUTOLAYER perhaps use extent ??

	s -> setVisible(true);

	layer -> setSelection(s);

}

void KisSelectionManager::unSelectAll()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	layer -> removeSelection(); // XXX save selection for reselect

	m_parent -> updateCanvas();

}


void KisSelectionManager::clear()
{
        KisImageSP img = m_parent -> currentImg();
        if (!img) return;


	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (!layer -> hasSelection()) return;

	KisSelectionSP selection = layer -> selection();
	QRect r = selection -> selectedRect();

// 	kdDebug() << "Selection rect: "
// 		  << r.x() << ", "
// 		  << r.y() << ", "
// 		  << r.width() << ", "
// 		  << r.height() << "\n";

	r = r.normalize();


// 	kdDebug() << "Normalized selection rect: "
// 		  << r.x() << ", "
// 		  << r.y() << ", "
// 		  << r.width() << ", "
// 		  << r.height() << "\n";

	// XXX: make undoable
	KisPainter p(img -> activeDevice());
 	p.bitBlt(r.x(), r.y(),
 		 COMPOSITE_COPY_OPACITY, // XXX: Is a mere copy of transparency correct?
 		 selection.data(),
 		 r.x(), r.y(),
 		 r.width(), r.height());
	p.end();

	layer -> removeSelection();
	m_parent -> updateCanvas();

}



void KisSelectionManager::reselect()
{
	// Restore the previous selection
}


void KisSelectionManager::invert()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	if (layer -> hasSelection()) {
		KisSelectionSP s = layer -> selection();
		// AUTOLAYER change the following
		s -> invert(QRect(0, 0, 100, 100));
	}
	else {
		selectAll();
	}
	m_parent -> updateCanvas(0, 0, img->width(),img->height());

}

void KisSelectionManager::copySelectionToNewLayer()
{
        KisImageSP img = m_parent -> currentImg();
	if (!img) return;

	KisLayerSP layer = img -> activeLayer();
	if (!layer) return;

	copy();
	paste();
}


void KisSelectionManager::feather() {}
void KisSelectionManager::border() {}
void KisSelectionManager::expand() {}
void KisSelectionManager::smooth() {}
void KisSelectionManager::contract() {}
void KisSelectionManager::grow() {}
void KisSelectionManager::similar() {}
void KisSelectionManager::transform() {}
void KisSelectionManager::load() {}
void KisSelectionManager::save() {}


#include "kis_selection_manager.moc"
