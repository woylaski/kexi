/*
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_CUSTOM_CONVOLUTION_FILTER_CONFIGURATION_WIDGET_H_
#define _KIS_CUSTOM_CONVOLUTION_FILTER_CONFIGURATION_WIDGET_H_

#include "kis_filter_configuration_widget.h"
#include <vector> 

class KisCustomConvolutionFilterConfigurationBaseWidget;
class KisMatrixWidget;

class KisCustomConvolutionFilterConfigurationWidget : public KisFilterConfigurationWidget
{
	Q_OBJECT
	public:
		KisCustomConvolutionFilterConfigurationWidget( KisFilter* nfilter, QWidget * parent, const char * name);
		inline KisCustomConvolutionFilterConfigurationBaseWidget* matrixWidget(int i) { return m_ccfcws[i]; };
		inline int pos(int i) { return m_pos[i]; };
	private:
		KisCustomConvolutionFilterConfigurationBaseWidget** m_ccfcws;
		int* m_pos;
};

#endif
