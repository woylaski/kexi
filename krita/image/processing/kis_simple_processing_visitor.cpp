/*
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_simple_processing_visitor.h"

#include "kis_group_layer.h"
#include "kis_paint_layer.h"
#include "kis_adjustment_layer.h"
#include "generator/kis_generator_layer.h"

#include "kis_transparency_mask.h"
#include "kis_filter_mask.h"
#include "kis_transform_mask.h"
#include "kis_selection_mask.h"

#include "kis_selection.h"

#include "kis_do_something_command.h"


KisSimpleProcessingVisitor::~KisSimpleProcessingVisitor()
{
}

void KisSimpleProcessingVisitor::visit(KisNode *node, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(node);
    Q_UNUSED(undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisCloneLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisExternalLayer *layer, KisUndoAdapter *undoAdapter)
{
    visitExternalLayer(layer, undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisPaintLayer *layer, KisUndoAdapter *undoAdapter)
{
    visitNodeWithPaintDevice(layer, undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisGroupLayer *layer, KisUndoAdapter *undoAdapter)
{
    using namespace KisDoSomethingCommandOps;
    undoAdapter->addCommand(new KisDoSomethingCommand<ResetOp, KisGroupLayer*>(layer, false));
    undoAdapter->addCommand(new KisDoSomethingCommand<ResetOp, KisGroupLayer*>(layer, true));
}

void KisSimpleProcessingVisitor::visit(KisAdjustmentLayer *layer, KisUndoAdapter *undoAdapter)
{
    using namespace KisDoSomethingCommandOps;
    undoAdapter->addCommand(new KisDoSomethingCommand<ResetOp, KisAdjustmentLayer*>(layer, false));
    visitNodeWithPaintDevice(layer, undoAdapter);
    undoAdapter->addCommand(new KisDoSomethingCommand<ResetOp, KisAdjustmentLayer*>(layer, true));
}

void KisSimpleProcessingVisitor::visit(KisGeneratorLayer *layer, KisUndoAdapter *undoAdapter)
{
    using namespace KisDoSomethingCommandOps;
    undoAdapter->addCommand(new KisDoSomethingCommand<UpdateOp, KisGeneratorLayer*>(layer, false));
    visitNodeWithPaintDevice(layer, undoAdapter);
    undoAdapter->addCommand(new KisDoSomethingCommand<UpdateOp, KisGeneratorLayer*>(layer, true));
}

void KisSimpleProcessingVisitor::visit(KisFilterMask *mask, KisUndoAdapter *undoAdapter)
{
    visitNodeWithPaintDevice(mask, undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisTransformMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(undoAdapter);

    // If (when) it had paint device, we would implement different default
    // strategy for it. Right now it has neither selection nor a paint device.
    KIS_ASSERT_RECOVER_NOOP(!mask->selection() && !mask->paintDevice());
}

void KisSimpleProcessingVisitor::visit(KisTransparencyMask *mask, KisUndoAdapter *undoAdapter)
{
    visitNodeWithPaintDevice(mask, undoAdapter);
}

void KisSimpleProcessingVisitor::visit(KisSelectionMask *mask, KisUndoAdapter *undoAdapter)
{
    visitNodeWithPaintDevice(mask, undoAdapter);
}
