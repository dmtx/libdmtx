/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 */

/**
 * \file image.h
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#define IMAGE_NO_ERROR  0
#define IMAGE_ERROR     1
#define IMAGE_NOT_PNG   2

typedef enum {
   ColorWhite  = 0x01 << 0,
   ColorRed    = 0x01 << 1,
   ColorGreen  = 0x01 << 2,
   ColorBlue   = 0x01 << 3,
   ColorYellow = 0x01 << 4
} ColorEnum;

/*void captureImage(DmtxImage *img, DmtxImage *imgTmp);*/
unsigned char *loadTextureImage(int *width, int *height);
unsigned char *loadPng(char *filename, int *width, int *height);
void plotPoint(DmtxImage *img, float rowFloat, float colFloat, int targetColor);
int clampRGB(float color);

#endif
