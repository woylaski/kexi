/*
 *  kis_tool_factory.cc - part of Krayon
 *
 *  Copyright (c) 2000 Patrick Julien <freak@ideasandassociates.com>
 *
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
 *  along with view program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "kis_global.h"
#include "kis_tool_factory.h"

// tools
#include "kis_tool_select_freehand.h"
#include "kis_tool_select_rectangular.h"
#include "kis_tool_select_polygonal.h"
#include "kis_tool_select_elliptical.h"
#include "kis_tool_select_contiguous.h"

#include "kis_tool_paste.h"
#include "kis_tool_move.h"
#include "kis_tool_zoom.h"
#include "kis_tool_brush.h"
#include "kis_tool_airbrush.h"
#include "kis_tool_pen.h"
#include "kis_tool_line.h"
#include "kis_tool_polyline.h"
#include "kis_tool_polygon.h"
#include "kis_tool_rectangle.h"
#include "kis_tool_ellipse.h"
#include "kis_tool_colorpicker.h"
#include "kis_tool_colorchanger.h"
#include "kis_tool_eraser.h"
#include "kis_tool_fill.h"
#include "kis_tool_stamp.h"

/*
 * toolFactory
 *
 * Load every know tool, and make it available to the KisView.  Hopefully, someday
 * these tools will be parts, so we can discover which tools are available at runtime 
 * instead of compile time.
 *
 * The view becomes the parent of these tools, so you don't need to delete them. When
 * the view is destroyed, they will get automatically deleted.  In other words, to be 
 * safe, you should store the vector returned by this function inside the parent.  This
 * way, nobody will use this vector after the parent dies.
 */

ktvector toolFactory(KisCanvas *canvas, KisBrush *brush, KisPattern *pattern, KisDoc *doc, KisView *view)
{
	ktvector tools;

	tools.reserve(100);

	tools.push_back(new ZoomTool(view));
	tools.push_back(new MoveTool(doc, view));

	// selection tools
	tools.push_back(new FreehandSelectTool(doc, view, canvas));
	tools.push_back(new RectangularSelectTool(doc, view, canvas));
	tools.push_back(new PolygonalSelectTool(doc, view, canvas));
	tools.push_back(new EllipticalSelectTool(doc, view, canvas));
	tools.push_back(new ContiguousSelectTool(doc, view, canvas));

	// painting tools
	tools.push_back(new PasteTool(doc, view, canvas));
	tools.push_back(new BrushTool(doc, view, brush));
	tools.push_back(new AirBrushTool(doc, view, brush));
	tools.push_back(new PenTool(doc, view, canvas, brush));
	tools.push_back(new EraserTool(doc, view, brush));
	tools.push_back(new ColorPicker(doc, view));
	tools.push_back(new ColorChangerTool(doc, view));
	tools.push_back(new FillTool(doc, view));
	tools.push_back(new StampTool(doc, view, canvas, pattern));

	// drawing tools
	tools.push_back(new LineTool(doc, view, canvas));
	tools.push_back(new PolyLineTool(doc, view, canvas));
	tools.push_back(new PolyGonTool(doc, view, canvas));
	tools.push_back(new RectangleTool(doc, view, canvas));
	tools.push_back(new EllipseTool(doc, view, canvas));

	return tools;
}

