/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file callback.h
 */

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
