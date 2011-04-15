/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file rotate_test.h
 */

#ifndef __SCANDEMO_H__
#define __SCANDEMO_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "../../dmtx.h"
#include "image.h"
#include "display.h"
#include "callback.h"

#define max(X,Y) (X > Y) ? X : Y
#define min(X,Y) (X < Y) ? X : Y

extern GLfloat       view_rotx;
extern GLfloat       view_roty;
extern GLfloat       view_rotz;
extern GLfloat       angle;

extern GLuint        barcodeTexture;
extern GLint         barcodeList;

extern DmtxImage     *gImage;
extern unsigned char *capturePxl;
extern unsigned char *texturePxl;
extern unsigned char *passOnePxl;
extern unsigned char *passTwoPxl;

extern char *gFilename[];
extern int gFileIdx;
extern int gFileCount;

#endif
