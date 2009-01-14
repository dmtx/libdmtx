/*
libdmtx - Data Matrix Encoding/Decoding Library
Copyright (C) 2007 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mblaughton@users.sourceforge.net
*/

/* $Id$ */

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

void BuildMatrixCallback2(DmtxRegion *region);
void BuildMatrixCallback3(DmtxMatrix3 region);
void BuildMatrixCallback4(DmtxMatrix3 region);
void PlotPointCallback(DmtxPixelLoc loc, int colorInt, int paneNbr, int dispType);
void XfrmPlotPointCallback(DmtxVector2 point, DmtxMatrix3 xfrm, int paneNbr, int dispType);
void FinalCallback(DmtxDecode *decode, DmtxRegion *region);
/*void PlotModuleCallback(DmtxDecode *info, DmtxRegion *region, int row, int col, DmtxColor3 color);*/

#endif
