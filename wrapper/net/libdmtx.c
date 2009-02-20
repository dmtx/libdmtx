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

#include "libdmtx.h"
#include "dmtx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

DMTX_EXTERN unsigned char
dmtx_decode(const unsigned char *rgb_image,
			const dmtx_uint32_t width,
			const dmtx_uint32_t height,
			const dmtx_decode_options_t *options,
			int(*callbackFunc)(dmtx_decoded_t *decode_result))
{
	DmtxImage *img = NULL;
	DmtxDecode *decode = NULL;
	DmtxRegion *region = NULL;
	DmtxMessage *msg = NULL;
	unsigned char returncode = DMTX_RETURN_OK;
	dmtx_uint16_t max_results = options->maxCodes;
	DmtxPassFail err = DmtxPass;
	DmtxTime msec, *timeout = NULL;
	DmtxVector2 p00, p10, p11, p01;
	double rotate;
	int result_count;

	// Create libdmtx's image structure
	img = dmtxImageCreate((unsigned char *)rgb_image, (int) width, (int) height, DmtxPack24bppBGR);
	if (img == NULL) return DMTX_RETURN_NO_MEMORY;

	// Apply options
	decode = dmtxDecodeCreate(img, 1);
	timeout = (options->timeoutMS != DmtxUndefined) ? &msec : NULL;
	if (timeout != NULL)
		msec = dmtxTimeAdd(dmtxTimeNow(), options->timeoutMS);
	while (1) {
		if ((options->edgeMax != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeMax, options->edgeMax)
			) != DmtxPass)) break;
		if ((options->edgeMin != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeMin, options->edgeMin)
			) != DmtxPass)) break;
		if ((options->scanGap != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropScanGap, options->scanGap)
			) != DmtxPass)) break;
		if ((options->squareDevn != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropSquareDevn, options->squareDevn)
			) != DmtxPass)) break;
		if ((err = dmtxDecodeSetProp(decode, DmtxPropSymbolSize, options->sizeIdxExpected)
			) != DmtxPass) break;
		if ((options->edgeThresh != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeThresh, options->edgeThresh)
			) != DmtxPass)) break;
		if ((options->xMax != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropXmax, options->xMax)
			) != DmtxPass)) break;
		if ((options->xMin != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropXmin, options->xMin)
			) != DmtxPass)) break;
		if ((options->yMax != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropYmax, options->yMax)
			) != DmtxPass)) break;
		if ((options->yMin != DmtxUndefined) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropYmin, options->yMin)
			) != DmtxPass)) break;
		break;
	}
	if (err != DmtxPass) {
		dmtxDecodeDestroy(&decode);
		dmtxImageDestroy(&img);
		return DMTX_RETURN_INVALID_ARGUMENT;
	}

	// Find and decode matrices in the image
	region = dmtxRegionFindNext(decode, timeout);
	result_count = 0;
	while ((region != NULL) && (result_count < max_results)) {
		dmtx_decoded_t result;

		p00.X = p00.Y = p10.Y = p01.X = 0.0;
		p10.X = p01.Y = p11.X = p11.Y = 1.0;
		dmtxMatrix3VMultiplyBy(&p00, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p10, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p11, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p01, region->fit2raw);
		result.corners.corner0.x = (dmtx_uint16_t)(p00.X + 0.5);
		result.corners.corner0.y = (dmtx_uint16_t)(height - 1 - (int)(p00.Y + 0.5));
		result.corners.corner1.x = (dmtx_uint16_t)(p01.X + 0.5);
		result.corners.corner1.y = (dmtx_uint16_t)(height - 1 - (int)(p01.Y + 0.5));
		result.corners.corner2.x = (dmtx_uint16_t)(p10.X + 0.5);
		result.corners.corner2.y = (dmtx_uint16_t)(height - 1 - (int)(p10.Y + 0.5));
		result.corners.corner3.x = (dmtx_uint16_t)(p11.X + 0.5);
		result.corners.corner3.y = (dmtx_uint16_t)(height - 1 - (int)(p11.Y + 0.5));

		rotate = (2 * M_PI) + (atan2(region->fit2raw[0][1], region->fit2raw[1][1]) -
			atan2(region->fit2raw[1][0], region->fit2raw[0][0])) / 2.0;
		rotate = (rotate * 180/M_PI);  // degrees
		if (rotate >= 360) rotate -= 360;
		result.symbolInfo.angle = (dmtx_uint16_t) (rotate + 0.5);
		result.symbolInfo.cols = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
		result.symbolInfo.rows = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
		result.symbolInfo.horizDataRegions = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx);
		result.symbolInfo.vertDataRegions = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx);
		result.symbolInfo.interleavedBlocks = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx);
		result.symbolInfo.capacity = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, region->sizeIdx);
		result.symbolInfo.errorWords = (dmtx_uint16_t) dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, region->sizeIdx);

		if (options->mosaic)
			msg = dmtxDecodeMosaicRegion(decode, region, options->correctionsMax);
		else
			msg = dmtxDecodeMatrixRegion(decode, region, options->correctionsMax);
		if (msg != NULL) {
			result.data = malloc(msg->outputSize);
			if (result.data != NULL) {
				memcpy(result.data, msg->output, msg->outputSize);
				result.dataSize = msg->outputSize;
			}
			result.symbolInfo.padWords = (dmtx_uint16_t) msg->padCount;
			result.symbolInfo.dataWords = (dmtx_uint16_t) (
				result.symbolInfo.capacity -
				result.symbolInfo.padWords);
			dmtxMessageDestroy(&msg);
		}

		if(callbackFunc(&result)==0) {
			break;
		}

		result_count++;
		region = dmtxRegionFindNext(decode, timeout);
	}

	// Clean-up
	dmtxRegionDestroy(&region);
	dmtxDecodeDestroy(&decode);
	dmtxImageDestroy(&img);

	return returncode;
}

DMTX_EXTERN void
dmtx_bitmap_to_byte_array(const unsigned char* src,
						  int stride,
						  int width,
						  int height,
						  unsigned char* dest)
{
	int y;

	for (y = 0; y < height; y++) {
		memcpy(dest, src, width*3);
		src += stride;
		dest += width * 3;
    }
}

DMTX_EXTERN unsigned char
dmtx_encode(const void *plain_text,
			const dmtx_uint16_t text_size,
			dmtx_encoded_t **result,
			const dmtx_encode_options_t *options)
{
	DmtxEncode *enc;
	DmtxPassFail err = DmtxPass;
	DmtxRegion *region = NULL;
	dmtx_encoded_t *res = NULL;
	*result = NULL;

	enc = dmtxEncodeCreate();
	if (enc == NULL) return DMTX_RETURN_NO_MEMORY;
	while (1) {
		if ((err = dmtxEncodeSetProp(enc, DmtxPropMarginSize, options->marginSize))
			!= DmtxPass) break;
		if ((err = dmtxEncodeSetProp(enc, DmtxPropModuleSize, options->moduleSize))
			!= DmtxPass) break;
		if ((err = dmtxEncodeSetProp(enc, DmtxPropSizeRequest, options->sizeIdx))
			!= DmtxPass) break;
		if ((err = dmtxEncodeSetProp(enc, DmtxPropScheme, options->scheme))
			!= DmtxPass) break;
		if ((err = dmtxEncodeSetProp(enc, DmtxPropImageFlip, DmtxFlipY))
			!= DmtxPass) break;
		break;
	}
	if (err != DmtxPass) {
		dmtxEncodeDestroy(&enc);
		return DMTX_RETURN_INVALID_ARGUMENT;
	}

	if (options->mosaic)
		err = dmtxEncodeDataMosaic(enc, (int) text_size, (void *) plain_text);
	else
		err = dmtxEncodeDataMatrix(enc, (int) text_size, (void *) plain_text);
	if (err != DmtxPass) {
		dmtxEncodeDestroy(&enc);
		return DMTX_RETURN_ENCODE_ERROR;
	}

	res = *result = malloc(sizeof(dmtx_encoded_t));
	if (res == NULL) {
		dmtxEncodeDestroy(&enc);
		return DMTX_RETURN_NO_MEMORY;
	}

	res->symbolInfo.angle = options->rotate;
	region = &enc->region;
	res->symbolInfo.cols = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
	res->symbolInfo.rows = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
	res->symbolInfo.horizDataRegions = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx);
	res->symbolInfo.vertDataRegions = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx);
	res->symbolInfo.interleavedBlocks = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx);
	res->symbolInfo.capacity = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, region->sizeIdx);
	res->symbolInfo.errorWords = (dmtx_uint16_t)
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, region->sizeIdx);
	res->symbolInfo.padWords = (dmtx_uint16_t) enc->message->padCount;
	res->symbolInfo.dataWords = (dmtx_uint16_t) (
		res->symbolInfo.capacity -
		res->symbolInfo.padWords);
	res->width = (dmtx_uint16_t) dmtxImageGetProp(enc->image, DmtxPropWidth);
	res->height = (dmtx_uint16_t) dmtxImageGetProp(enc->image, DmtxPropHeight);
	res->data = enc;
	return DMTX_RETURN_OK;
}

DMTX_EXTERN void
dmtx_copy_encode_result(const DmtxEncode *enc,
						const dmtx_uint32_t stride,
						const unsigned char *bitmap)
{
	dmtx_uint32_t i;
	dmtx_uint32_t width = dmtxImageGetProp(enc->image, DmtxPropWidth);
	dmtx_uint32_t height = dmtxImageGetProp(enc->image, DmtxPropHeight);

	for (i = 0; i < height; i++)
		memcpy((void *) (bitmap + (height-i-1) * stride),
		enc->image->pxl + i * 3 * width,
		3 * width);
}

DMTX_EXTERN void
dmtx_free_encode_result(const DmtxEncode *enc)
{
	DmtxEncode **pEnc = &((DmtxEncode *) enc);
	dmtxEncodeDestroy(pEnc);
}

DMTX_EXTERN char *
dmtx_version(void)
{
	return dmtxVersion();
}
