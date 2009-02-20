/*
libdmtx-net - .NET wrapper for libdmtx

Copyright (C) 2009 Joseph Ferner /Tom Vali

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

Contact: libdmtx@fernsroth.com
*/

/* $Id$ */

#ifndef __LIBDMTX_H__
#define __LIBDMTX_H__

#define DMTX_RETURN_OK                0
#define DMTX_RETURN_NO_MEMORY         1
#define DMTX_RETURN_INVALID_ARGUMENT  2
#define DMTX_RETURN_ENCODE_ERROR      3

#include "dmtx.h"

#ifdef _MSC_VER
typedef signed   __int32 dmtx_int32_t;
typedef signed   __int16 dmtx_int16_t;
typedef unsigned __int32 dmtx_uint32_t;
typedef unsigned __int16 dmtx_uint16_t;
#	define DMTX_EXTERN __declspec(dllexport)
#else
#error The integral types sizes might not work as expected. \
	Check your compiler behaviour and comment out this line.
typedef signed   long    dmtx_int32_t;
typedef signed   short   dmtx_int16_t;
typedef unsigned long    dmtx_uint32_t;
typedef unsigned short   dmtx_uint16_t;
#	define DMTX_EXTERN extern
#endif

typedef struct dmtx_decode_options_t {
	dmtx_int16_t edgeMin;
	dmtx_int16_t edgeMax;
	dmtx_int16_t scanGap;
	dmtx_int16_t squareDevn;
	dmtx_int32_t timeoutMS;
	dmtx_int16_t sizeIdxExpected;
	dmtx_int16_t edgeThresh;
	dmtx_int16_t maxCodes;
	dmtx_int16_t xMin;
	dmtx_int16_t xMax;
	dmtx_int16_t yMin;
	dmtx_int16_t yMax;
	dmtx_int16_t correctionsMax;
	dmtx_uint16_t mosaic;
	dmtx_int16_t shrink;
} dmtx_decode_options_t;

typedef struct dmtx_encode_options_t {
	dmtx_uint16_t marginSize;
	dmtx_uint16_t moduleSize;
	dmtx_uint16_t scheme;
	dmtx_uint16_t rotate;
	dmtx_int16_t sizeIdx;
	dmtx_uint16_t mosaic;
} dmtx_encode_options_t;

typedef struct dmtx_point_t
{
	dmtx_uint16_t x;
	dmtx_uint16_t y;
} dmtx_point_t;

typedef struct dmtx_corners_t
{
	dmtx_point_t corner0;
	dmtx_point_t corner1;
	dmtx_point_t corner2;
	dmtx_point_t corner3;
} dmtx_corners_t;

typedef struct dmtx_symbolinfo_t
{
	dmtx_uint16_t rows;
	dmtx_uint16_t cols;
	dmtx_uint16_t capacity;
	dmtx_uint16_t dataWords;
	dmtx_uint16_t padWords;
	dmtx_uint16_t errorWords;
	dmtx_uint16_t horizDataRegions;
	dmtx_uint16_t vertDataRegions;
	dmtx_uint16_t interleavedBlocks;
	dmtx_uint16_t angle;
} dmtx_symbolinfo_t;

typedef struct dmtx_decoded_t
{
	dmtx_symbolinfo_t symbolInfo;
	dmtx_corners_t corners;
	char *data;
	dmtx_uint32_t dataSize;
} dmtx_decoded_t;

typedef struct dmtx_encoded_t
{
	dmtx_symbolinfo_t symbolInfo;
	dmtx_uint32_t width;
	dmtx_uint32_t height;
	DmtxEncode *data;
} dmtx_encoded_t;

DMTX_EXTERN unsigned char
dmtx_decode(const void *rgb_image,
			const dmtx_uint32_t width,
			const dmtx_uint32_t height,
			const dmtx_uint32_t bitmapStride,
			const dmtx_decode_options_t *options,
			int(*callbackFunc)(dmtx_decoded_t *decode_result));

DMTX_EXTERN unsigned char
dmtx_encode(const void *plain_text,
			const dmtx_uint16_t text_size,
			dmtx_encoded_t **result,
			const dmtx_encode_options_t *options);

DMTX_EXTERN void
dmtx_copy_encode_result(const DmtxEncode *enc,
						const dmtx_uint32_t stride,
						const unsigned char *bitmap);

DMTX_EXTERN void
dmtx_free_encode_result(const DmtxEncode *enc);

DMTX_EXTERN char *
dmtx_version(void);

#endif  // #ifndef _LIBDMTX_H
