/*
 *  Copyright (c) 2007 Sven Langkamp <sven.langkamp@gmail.com>
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

#include "kis_selection_tool_helper.h"


#include <kundo2command.h>

#include <KoShapeController.h>
#include <KoPathShape.h>
#include <KoShapeManager.h>

#include "kis_pixel_selection.h"
#include "kis_shape_selection.h"
#include "kis_image.h"
#include "canvas/kis_canvas2.h"
#include "kis_view2.h"
#include "kis_selection_manager.h"
#include "kis_transaction.h"
#include "commands/kis_selection_commands.h"
#include "kis_shape_controller.h"

#include <KoIcon.h>
#include "kis_processing_applicator.h"
#include "kis_transaction_based_command.h"
#include "kis_gui_context_command.h"


KisSelectionToolHelper::KisSelectionToolHelper(KisCanvas2* canvas, const QString& name)
        : m_canvas(canvas)
        , m_name(name)
{
    m_image = m_canvas->view()->image();
}

KisSelectionToolHelper::~KisSelectionToolHelper()
{
}

struct LazyInitGlobalSelection : public KisTransactionBasedCommand {
    LazyInitGlobalSelection(KisView2 *view) : m_view(view) {}
    KisView2 *m_view;

    KUndo2Command* paint() {
        return !m_view->selection() ?
            new KisSetEmptyGlobalSelectionCommand(m_view->image()) : 0;
    }
};

void KisSelectionToolHelper::selectPixelSelection(KisPixelSelectionSP selection, SelectionAction action)
{
    KisView2* view = m_canvas->view();
    if (selection->selectedRect().isEmpty()) {
        m_canvas->view()->selectionManager()->deselect();
        return;
    }

    KisProcessingApplicator applicator(view->image(),
                                       0 /* we need no automatic updates */,
                                       KisProcessingApplicator::SUPPORTS_WRAPAROUND_MODE,
                                       KisImageSignalVector() << ModifiedSignal,
                                       m_name);

    applicator.applyCommand(new LazyInitGlobalSelection(view));

    struct ApplyToPixelSelection : public KisTransactionBasedCommand {
        ApplyToPixelSelection(KisView2 *view,
                              KisPixelSelectionSP selection,
                              SelectionAction action) : m_view(view),
                                                        m_selection(selection),
                                                        m_action(action) {}
        KisView2 *m_view;
        KisPixelSelectionSP m_selection;
        SelectionAction m_action;

        KUndo2Command* paint() {

            KisPixelSelectionSP pixelSelection = m_view->selection()->pixelSelection();
            KIS_ASSERT_RECOVER(pixelSelection) { return 0; }

            bool hasSelection = !pixelSelection->isEmpty();

            KisSelectionTransaction transaction("", pixelSelection);

            if (!hasSelection && m_action == SELECTION_SUBTRACT) {
                pixelSelection->invert();
            }

            pixelSelection->applySelection(m_selection, m_action);

            QRect dirtyRect = m_view->image()->bounds();
            if (hasSelection && m_action != SELECTION_REPLACE && m_action != SELECTION_INTERSECT) {
                dirtyRect = m_selection->selectedRect();
            }
            m_view->selection()->updateProjection(dirtyRect);

            KUndo2Command *savedCommand = transaction.endAndTake();
            pixelSelection->setDirty(dirtyRect);

            return savedCommand;
        }
    };

    applicator.applyCommand(new ApplyToPixelSelection(view, selection, action));
    applicator.end();
}

void KisSelectionToolHelper::addSelectionShape(KoShape* shape)
{
    KisView2* view = m_canvas->view();

    if (view->image()->wrapAroundModePermitted()) {
        view->showFloatingMessage(
            i18n("Shape selection does not fully "
                 "support wraparound mode. Please "
                 "use pixel selection instead"),
                 koIcon("selection-info"));
    }

    KisProcessingApplicator applicator(view->image(),
                                       0 /* we need no automatic updates */,
                                       KisProcessingApplicator::NONE,
                                       KisImageSignalVector() << ModifiedSignal,
                                       m_name);

    applicator.applyCommand(new LazyInitGlobalSelection(view));

    struct ClearPixelSelection : public KisTransactionBasedCommand {
        ClearPixelSelection(KisView2 *view) : m_view(view) {}
        KisView2 *m_view;

        KUndo2Command* paint() {

            KisPixelSelectionSP pixelSelection = m_view->selection()->pixelSelection();
            KIS_ASSERT_RECOVER(pixelSelection) { return 0; }

            KisSelectionTransaction transaction("", pixelSelection);
            pixelSelection->clear();
            return transaction.endAndTake();
        }
    };

    applicator.applyCommand(new ClearPixelSelection(view));

    struct AddSelectionShape : public KisTransactionBasedCommand {
        AddSelectionShape(KisView2 *view, KoShape* shape) : m_view(view),
                                                            m_shape(shape) {}
        KisView2 *m_view;
        KoShape* m_shape;

        KUndo2Command* paint() {
            /**
             * Mark a shape that it belongs to a shape selection
             */
            if(!m_shape->userData()) {
                m_shape->setUserData(new KisShapeSelectionMarker);
            }

            return m_view->canvasBase()->shapeController()->addShape(m_shape);
        }
    };

    applicator.applyCommand(
        new KisGuiContextCommand(new AddSelectionShape(view, shape), view));

    applicator.end();
}

void KisSelectionToolHelper::cropRectIfNeeded(QRect *rect)
{
    KisImageWSP image = m_canvas->view()->image();

    if (!image->wrapAroundModePermitted()) {
        *rect &= image->bounds();
    }
}

void KisSelectionToolHelper::cropPathIfNeeded(QPainterPath *path)
{
    KisImageWSP image = m_canvas->view()->image();

    if (!image->wrapAroundModePermitted()) {
        QPainterPath cropPath;
        cropPath.addRect(image->bounds());
        *path &= cropPath;
    }
}

