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

#ifndef __SCANDEMO_H__
#define __SCANDEMO_H__

#include <GL/gl.h>
#include "dmtx.h"

#define max(X,Y) (X > Y) ? X : Y
#define min(X,Y) (X < Y) ? X : Y

extern GLfloat       view_rotx;
extern GLfloat       view_roty;
extern GLfloat       view_rotz;
extern GLfloat       angle;

extern GLuint        barcodeTexture;
extern GLint         barcodeList;

extern DmtxImage     *captured;
extern DmtxImage     *textureImage;
extern DmtxImage     *passOneImage;
extern DmtxImage     *passTwoImage;

extern char *gFilename[];
extern int gFileIdx;
extern int gFileCount;

#endif
