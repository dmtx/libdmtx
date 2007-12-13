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

/* $Id: callback.h,v 1.4 2006/10/13 01:54:35 mblaughton Exp $ */

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

/*
void CrossScanCallback(DmtxScanRange *range, DmtxGradient *gradient, DmtxEdgeScan *scan);
void FollowScanCallback(DmtxEdgeFollower *follower);
void FinderBarCallback(DmtxRay2 *ray);
*/
void BuildMatrixCallback2(DmtxMatrixRegion *region);
void BuildMatrixCallback3(DmtxMatrix3 region);
void BuildMatrixCallback4(DmtxMatrix3 region);
void PlotPointCallback(DmtxVector2 point, int colorInt, int paneNbr, int dispType);
void XfrmPlotPointCallback(DmtxVector2 point, DmtxMatrix3 xfrm, int paneNbr, int dispType);
void FinalCallback(DmtxDecode *decode, DmtxMatrixRegion *region);
void PlotModuleCallback(DmtxDecode *info, DmtxMatrixRegion *region, int row, int col, DmtxColor3 color);

#endif
