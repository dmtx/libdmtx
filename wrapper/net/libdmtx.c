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
#include <stdlib.h>
#include <string.h>

DMTX_EXTERN unsigned char
dmtx_decode(const void *rgb_image,
			const dmtx_uint32_t width,
			const dmtx_uint32_t height,
			dmtx_decoded_t **decode_results,
			dmtx_uint32_t *result_count,
			const dmtx_decode_options_t *options)
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
	dmtx_decoded_t *results = NULL;
	double rotate;

	*result_count = 0;

	// Create libdmtx's image structure
	img = dmtxImageCreate(
		(unsigned char *) rgb_image,
		(int) width, (int) height,
		24, DmtxPackRGB, DmtxFlipY);
	if (img == NULL) return DMTX_RETURN_NO_MEMORY;

	// Apply options
	decode = dmtxDecodeCreate(img);
	timeout = (options->timeoutMS > 0) ? &msec : NULL;
	if (timeout != NULL)
		msec = dmtxTimeAdd(dmtxTimeNow(), options->timeoutMS);
	while (1) {
		if ((options->edgeMax >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeMax, options->edgeMax)
			) != DmtxPass)) break;
		if ((options->edgeMin >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeMin, options->edgeMin)
			) != DmtxPass)) break;
		if ((options->scanGap >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropScanGap, options->scanGap)
			) != DmtxPass)) break;
		if ((options->squareDevn >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropSquareDevn, options->squareDevn)
			) != DmtxPass)) break;
		if ((err = dmtxDecodeSetProp(decode, DmtxPropSymbolSize, options->sizeIdxExpected)
			) != DmtxPass) break;
		if ((options->edgeTresh >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropEdgeThresh, options->edgeTresh)
			) != DmtxPass)) break;
		if ((options->xMax >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropXmax, options->xMax)
			) != DmtxPass)) break;
		if ((options->xMin >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropXmin, options->xMin)
			) != DmtxPass)) break;
		if ((options->yMax >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropYmax, options->yMax)
			) != DmtxPass)) break;
		if ((options->yMin >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropYmin, options->yMin)
			) != DmtxPass)) break;
		if ((options->shrinkMax >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropShrinkMax, options->shrinkMax)
			) != DmtxPass)) break;
		if ((options->shrinkMin >= 0) &&
			((err = dmtxDecodeSetProp(decode, DmtxPropShrinkMin, options->shrinkMin)
			) != DmtxPass)) break;
		break;
	}
	if (err != DmtxPass) {
		dmtxDecodeDestroy(&decode);
		dmtxImageDestroy(&img);
		return DMTX_RETURN_INVALID_ARGUMENT;
	}

	// Prepare results buffer
	if (max_results < 1) max_results = 100;
	*decode_results = calloc(max_results, sizeof(dmtx_decoded_t));
	results = *decode_results;
	if (results == NULL) {
		dmtxImageDestroy(&img);
		return DMTX_RETURN_NO_MEMORY;
	}
	memset(results, 0, max_results * sizeof(dmtx_decoded_t));

	// Find and decode matrices in the image
	region = dmtxRegionFindNext(decode, timeout);
	while ((region != NULL) && (*result_count < max_results)) {
		p00.X = p00.Y = p10.Y = p01.X = 0.0;
		p10.X = p01.Y = p11.X = p11.Y = 1.0;
		dmtxMatrix3VMultiplyBy(&p00, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p10, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p11, region->fit2raw);
		dmtxMatrix3VMultiplyBy(&p01, region->fit2raw);
		results[*result_count].corners.corner0.x =
			(dmtx_uint16_t)(p00.X + 0.5);
		results[*result_count].corners.corner0.y =
			(dmtx_uint16_t)(height - 1 - (int)(p00.Y + 0.5));
		results[*result_count].corners.corner1.x =
			(dmtx_uint16_t)(p01.X + 0.5);
		results[*result_count].corners.corner1.y =
			(dmtx_uint16_t)(height - 1 - (int)(p01.Y + 0.5));
		results[*result_count].corners.corner2.x =
			(dmtx_uint16_t)(p10.X + 0.5);
		results[*result_count].corners.corner2.y =
			(dmtx_uint16_t)(height - 1 - (int)(p10.Y + 0.5));
		results[*result_count].corners.corner3.x =
			(dmtx_uint16_t)(p11.X + 0.5);
		results[*result_count].corners.corner3.y =
			(dmtx_uint16_t)(height - 1 - (int)(p11.Y + 0.5));

		rotate = (2 * M_PI) + (atan2(region->fit2raw[0][1], region->fit2raw[1][1]) -
			atan2(region->fit2raw[1][0], region->fit2raw[0][0])) / 2.0;
		rotate = (rotate * 180/M_PI);  // degrees
		if (rotate >= 360) rotate -= 360;
		results[*result_count].symbolInfo.angle =
			(dmtx_uint16_t) (rotate + 0.5);
		results[*result_count].symbolInfo.cols = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx);
		results[*result_count].symbolInfo.rows = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx);
		results[*result_count].symbolInfo.horizDataRegions = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx);
		results[*result_count].symbolInfo.vertDataRegions = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx);
		results[*result_count].symbolInfo.interleavedBlocks = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx);
		results[*result_count].symbolInfo.capacity = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, region->sizeIdx);
		results[*result_count].symbolInfo.errorWords = (dmtx_uint16_t)
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, region->sizeIdx);

		if (options->mocaic)
			msg = dmtxDecodeMosaicRegion(decode, region, options->correctionsMax);
		else
			msg = dmtxDecodeMatrixRegion(decode, region, options->correctionsMax);
		if (msg != NULL) {
			results[*result_count].data = malloc(msg->outputSize);
			if (results[*result_count].data != NULL) {
				memcpy(results[*result_count].data, msg->output, msg->outputSize);
				results[*result_count].dataSize = msg->outputSize;
			}
			results[*result_count].symbolInfo.padWords = (dmtx_uint16_t)
				msg->padCount;
			results[*result_count].symbolInfo.dataWords = (dmtx_uint16_t) (
				results[*result_count].symbolInfo.capacity -
				results[*result_count].symbolInfo.padWords);
			dmtxMessageDestroy(&msg);
		}
		(*result_count)++;
		region = dmtxRegionFindNext(decode, timeout);
	}

	// Clean-up
	dmtxRegionDestroy(&region);
	dmtxDecodeDestroy(&decode);
	dmtxImageDestroy(&img);

	return returncode;
}

DMTX_EXTERN void
dmtx_free_decode_result(const dmtx_decoded_t *ptr,
						const dmtx_uint32_t result_count)
{
	dmtx_uint32_t i;
	for (i = 0; i < result_count; i++)
		free(ptr[i].data);
	free((void *) ptr);  // remove const warning
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
