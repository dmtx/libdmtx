/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

Contact: mike@dragonflylogic.com
*/

/* $Id$ */

/**
 * @file dmtxdecode.c
 * @brief Decode regions
 */

/**
 * @brief  Initialize decode struct with default values
 * @param  img
 * @return Initialized DmtxDecode struct
 */
extern DmtxDecode *
dmtxDecodeStructCreate(DmtxImage *img)
{
   DmtxDecode *dec;
   int cacheSize;

   dec = (DmtxDecode *)calloc(1, sizeof(DmtxDecode));
   if(dec == NULL)
      return NULL;

   dec->image = img;

   cacheSize = dmtxImageGetProp(img, DmtxPropWidth) *
         dmtxImageGetProp(img, DmtxPropHeight) * sizeof(unsigned char);

   memset(dec->image->cache, 0x00, cacheSize);

   /* These values should probably be stored in the decode struct */
   img->scale = 1;
   img->xMin = img->xMinScaled = 0;
   img->xMax = img->xMaxScaled = img->width - 1;
   img->yMin = img->yMinScaled = 0;
   img->yMax = img->yMaxScaled = img->height - 1;

   dec->edgeMin = -1;
   dec->edgeMax = -1;
   dec->scanGap = 1;
   dec->squareDevn = cos(50 * (M_PI/180));
   dec->sizeIdxExpected = DmtxSymbolShapeAuto;
   dec->edgeThresh = 10;
   dec->shrinkMin = 1;
   dec->shrinkMax = 1;

   dec->grid = InitScanGrid(img, dec->scanGap);

   return dec;
}

/**
 * @brief  Deinitialize decode struct
 * @param  dec
 * @return void
 */
extern void
dmtxDecodeStructDestroy(DmtxDecode **dec)
{
   if(dec == NULL)
      return;

   if(*dec != NULL)
      free(*dec);

   *dec = NULL;
}

/**
 * @brief  Set decoding behavior property
 * @param  dec
 * @param  prop
 * @param  value
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
extern int
dmtxDecodeSetProp(DmtxDecode *dec, int prop, int value)
{
   int err;

   switch(prop) {
      case DmtxPropEdgeMin:
         dec->edgeMin = value;
         break;
      case DmtxPropEdgeMax:
         dec->edgeMax = value;
         break;
      case DmtxPropScanGap:
         dec->scanGap = value;
         break;
      case DmtxPropSquareDevn:
         dec->squareDevn = cos(value * (M_PI/180.0));
         break;
      case DmtxPropSymbolSize:
         dec->sizeIdxExpected = value;
         break;
      case DmtxPropEdgeThresh:
         dec->edgeThresh = value;
         break;
      case DmtxPropXmin:
         err = dmtxImageSetProp(dec->image, DmtxPropXmin, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         break;
      case DmtxPropXmax:
         err = dmtxImageSetProp(dec->image, DmtxPropXmax, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         break;
      case DmtxPropYmin:
         err = dmtxImageSetProp(dec->image, DmtxPropYmin, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         break;
      case DmtxPropYmax:
         err = dmtxImageSetProp(dec->image, DmtxPropYmax, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         break;
      case DmtxPropShrinkMin:
         dec->shrinkMin = value;
         break;
      case DmtxPropShrinkMax:
         dec->shrinkMax = value;
         err = dmtxImageSetProp(dec->image, DmtxPropScale, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         break;
   }

   /* Minimum image scale can't be larger than maximum image scale */
   if(dec->shrinkMin < 1 || dec->shrinkMax < dec->shrinkMin)
      return DMTX_FAILURE;

   if(dec->squareDevn <= 0.0 || dec->squareDevn >= 1.0)
      return DMTX_FAILURE;

   if(dec->scanGap < 1)
      return DMTX_FAILURE;

   if(dec->edgeThresh < 1 || dec->edgeThresh > 100)
      return DMTX_FAILURE;

   /* Reinitialize scangrid if any inputs changed */
   dec->grid = InitScanGrid(dec->image, dec->scanGap);

   return DMTX_SUCCESS;
}

/**
 * @brief  Convert fitted Data Matrix region into a decoded message
 * @param  dec
 * @param  reg
 * @param  fix
 * @return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMatrixRegion(DmtxImage *img, DmtxRegion *reg, int fix)
{
   int row, col;
   int width, height;
   int offset;
   DmtxMessage *msg;
   DmtxVector2 p;

   msg = dmtxMessageCreate(reg->sizeIdx, DMTX_FORMAT_MATRIX);
   if(msg == NULL)
      return NULL;

   if(PopulateArrayFromMatrix(msg, img, reg) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   ModulePlacementEcc200(msg->array, msg->code,
         reg->sizeIdx, DMTX_MODULE_ON_RED | DMTX_MODULE_ON_GREEN | DMTX_MODULE_ON_BLUE);

   if(DecodeCheckErrors(msg->code, reg->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   width = dmtxImageGetProp(img, DmtxPropScaledWidth);
   height = dmtxImageGetProp(img, DmtxPropScaledHeight);
   for(row = 0; row < height; row++) {
      for(col = 0; col < width; col++) {
         p.X = col;
         p.Y = row;
         dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);
         /* XXX tighten these boundaries can by accounting for barcode size */
         if(p.X >= -0.1 && p.X <= 1.1 && p.Y >= -0.1 && p.Y <= 1.1) {

            offset = dmtxImageGetPixelOffset(img, col, row);
            if(offset == DMTX_BAD_OFFSET)
               continue;
            else
               img->cache[offset] |= 0x80; /* Mark as visited */
         }
      }
   }

   DecodeDataStream(msg, reg->sizeIdx, NULL);

   return msg;
}

/**
 * @brief  Convert fitted Data Mosaic region into a decoded message
 * @param  dec
 * @param  reg
 * @param  fix
 * @return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMosaicRegion(DmtxImage *img, DmtxRegion *reg, int fix)
{
   int row, col;
   int mappingRows, mappingCols;
   DmtxMessage *msg;
   DmtxMessage rMesg, gMesg, bMesg;

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, reg->sizeIdx);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, reg->sizeIdx);

   msg = dmtxMessageCreate(reg->sizeIdx, DMTX_FORMAT_MOSAIC);
   if(msg == NULL)
      return NULL;

   rMesg = gMesg = bMesg = *msg;
   rMesg.codeSize = gMesg.codeSize = bMesg.codeSize = msg->codeSize/3;

   gMesg.code += gMesg.codeSize;
   bMesg.code += (bMesg.codeSize * 2);

   if(PopulateArrayFromMosaic(msg, img, reg) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   ModulePlacementEcc200(msg->array, rMesg.code, reg->sizeIdx, DMTX_MODULE_ON_RED);
   if(DecodeCheckErrors(rMesg.code, reg->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   for(row = 0; row < mappingRows; row++)
      for(col = 0; col < mappingCols; col++)
         msg->array[row*mappingCols+col] &= (0xff ^ DMTX_MODULE_VISITED);

   ModulePlacementEcc200(msg->array, gMesg.code, reg->sizeIdx, DMTX_MODULE_ON_GREEN);
   if(DecodeCheckErrors(gMesg.code, reg->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   for(row = 0; row < mappingRows; row++)
      for(col = 0; col < mappingCols; col++)
         msg->array[row*mappingCols+col] &= (0xff ^ DMTX_MODULE_VISITED);

   ModulePlacementEcc200(msg->array, bMesg.code, reg->sizeIdx, DMTX_MODULE_ON_BLUE);
   if(DecodeCheckErrors(bMesg.code, reg->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   DecodeDataStream(&rMesg, reg->sizeIdx, NULL);
   DecodeDataStream(&gMesg, reg->sizeIdx, rMesg.output + rMesg.outputIdx);
   DecodeDataStream(&bMesg, reg->sizeIdx, gMesg.output + gMesg.outputIdx);

   msg->outputIdx = rMesg.outputIdx + gMesg.outputIdx + bMesg.outputIdx;

   return msg;
}

/**
 * @brief  Translate encoded data stream into final output
 * @param  msg
 * @param  sizeIdx
 * @param  outputStart
 * @return void
 */
static void
DecodeDataStream(DmtxMessage *msg, int sizeIdx, unsigned char *outputStart)
{
   DmtxSchemeDecode encScheme;
   unsigned char *ptr, *dataEnd;

   msg->output = (outputStart == NULL) ? msg->output : outputStart;
   msg->outputIdx = 0;

   ptr = msg->code;
   dataEnd = ptr + dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);

   while(ptr < dataEnd) {

      ptr = NextEncodationScheme(&encScheme, ptr);

      switch(encScheme) {
         case DmtxSchemeDecodeAsciiStd:
            ptr = DecodeSchemeAsciiStd(msg, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeAsciiExt:
            ptr = DecodeSchemeAsciiExt(msg, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeC40:
         case DmtxSchemeDecodeText:
            ptr = DecodeSchemeC40Text(msg, ptr, dataEnd, encScheme);
            break;

         case DmtxSchemeDecodeX12:
            ptr = DecodeSchemeX12(msg, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeEdifact:
            ptr = DecodeSchemeEdifact(msg, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeBase256:
            ptr = DecodeSchemeBase256(msg, ptr, dataEnd);
            break;
      }
   }
}

/**
 * @brief  Determine next encodation scheme
 * @param  encScheme
 * @param  ptr
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
NextEncodationScheme(DmtxSchemeDecode *encScheme, unsigned char *ptr)
{
   switch(*ptr) {
      case 230:
         *encScheme = DmtxSchemeDecodeC40;
         break;
      case 231:
         *encScheme = DmtxSchemeDecodeBase256;
         break;
      case 235:
         *encScheme = DmtxSchemeDecodeAsciiExt;
         break;
      case 238:
         *encScheme = DmtxSchemeDecodeX12;
         break;
      case 239:
         *encScheme = DmtxSchemeDecodeText;
         break;
      case 240:
         *encScheme = DmtxSchemeDecodeEdifact;
         break;
      default:
         *encScheme = DmtxSchemeDecodeAsciiStd;
         return ptr;
   }

   return ptr + 1;
}

/**
 * @brief  Decode stream assuming standard ASCII encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeAsciiStd(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int digits;

   if(*ptr <= 128) {
      msg->output[msg->outputIdx++] = *ptr - 1;
   }
   else if(*ptr == 129) {
      return dataEnd;
   }
   else if(*ptr <= 229) {
      digits = *ptr - 130;
      msg->output[msg->outputIdx++] = digits/10 + '0';
      msg->output[msg->outputIdx++] = digits - (digits/10)*10 + '0';
   }

   return ptr + 1;
}

/**
 * @brief  Decode stream assuming extended ASCII encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeAsciiExt(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   msg->output[msg->outputIdx++] = *ptr + 128;

   return ptr + 1;
}

/**
 * @brief  Decode stream assuming C40 or Text encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @param  encScheme
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeC40Text(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd, DmtxSchemeDecode encScheme)
{
   int i;
   int packed;
   int shift = 0;
   unsigned char c40Values[3];

   assert(encScheme == DmtxSchemeDecodeC40 || encScheme == DmtxSchemeDecodeText);

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      c40Values[0] = ((packed - 1)/1600);
      c40Values[1] = ((packed - 1)/40) % 40;
      c40Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(shift == 0) { /* Basic set */
            if(c40Values[i] <= 2) {
               shift = c40Values[i] + 1;
            }
            else if(c40Values[i] == 3) {
               msg->output[msg->outputIdx++] = ' '; /* Space */
            }
            else if(c40Values[i] <= 13) {
               msg->output[msg->outputIdx++] = c40Values[i] - 13 + '9'; /* 0-9 */
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeDecodeC40) {
                  msg->output[msg->outputIdx++] = c40Values[i] - 39 + 'Z'; /* A-Z */
               }
               else if(encScheme == DmtxSchemeDecodeText) {
                  msg->output[msg->outputIdx++] = c40Values[i] - 39 + 'z'; /* a-z */
               }
            }
         }
         else if(shift == 1) { /* Shift 1 set */
            msg->output[msg->outputIdx++] = c40Values[i]; /* ASCII 0 - 31 */

            shift = 0;
         }
         else if(shift == 2) { /* Shift 2 set */
            if(c40Values[i] <= 14)
               msg->output[msg->outputIdx++] = c40Values[i] + 33; /* ASCII 33 - 47 */
            else if(c40Values[i] <= 21)
               msg->output[msg->outputIdx++] = c40Values[i] + 43; /* ASCII 58 - 64 */
            else if(c40Values[i] <= 26)
               msg->output[msg->outputIdx++] = c40Values[i] + 69; /* ASCII 91 - 95 */
            else if(c40Values[i] == 27)
               fprintf(stdout, "FNC1 (?)"); /* FNC1 (eh?) */
            else if(c40Values[i] == 30)
               fprintf(stdout, "Upper Shift (?)"); /* Upper Shift (eh?) */

            shift = 0;
         }
         else if(shift == 3) { /* Shift 3 set */
            if(encScheme == DmtxSchemeDecodeC40) {
               msg->output[msg->outputIdx++] = c40Values[i] + 96;
            }
            else if(encScheme == DmtxSchemeDecodeText) {
               if(c40Values[i] == 0)
                  msg->output[msg->outputIdx++] = c40Values[i] + 96;
               else if(c40Values[i] <= 26)
                  msg->output[msg->outputIdx++] = c40Values[i] - 26 + 'Z'; /* A-Z */
               else
                  msg->output[msg->outputIdx++] = c40Values[i] - 31 + 127; /* { | } ~ DEL */
            }

            shift = 0;
         }
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == 254)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
      if(dataEnd - ptr == 1)
         return ptr;
   }

   return ptr;
}

/**
 * @brief  Decode stream assuming X12 encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeX12(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   int packed;
   unsigned char x12Values[3];

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      x12Values[0] = ((packed - 1)/1600);
      x12Values[1] = ((packed - 1)/40) % 40;
      x12Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(x12Values[i] == 0)
            msg->output[msg->outputIdx++] = 13;
         else if(x12Values[i] == 1)
            msg->output[msg->outputIdx++] = 42;
         else if(x12Values[i] == 2)
            msg->output[msg->outputIdx++] = 62;
         else if(x12Values[i] == 3)
            msg->output[msg->outputIdx++] = 32;
         else if(x12Values[i] <= 13)
            msg->output[msg->outputIdx++] = x12Values[i] + 44;
         else if(x12Values[i] <= 90)
            msg->output[msg->outputIdx++] = x12Values[i] + 51;
      }

      /* Unlatch if codeword 254 follows 2 codewords in C40/Text encodation */
      if(*ptr == 254)
         return ptr + 1;

      /* Unlatch is implied if only one codeword remains */
      if(dataEnd - ptr == 1)
         return ptr;
   }

   return ptr;
}

/**
 * @brief  Decode stream assuming EDIFACT encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeEdifact(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int i;
   unsigned char unpacked[4];

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+2 is safe to access -- shouldn't be a
         problem because I'm guessing you can guarantee there will always
         be at least 3 error codewords */
      unpacked[0] = (*ptr & 0xfc) >> 2;
      unpacked[1] = (*ptr & 0x03) << 4 | (*(ptr+1) & 0xf0) >> 4;
      unpacked[2] = (*(ptr+1) & 0x0f) << 2 | (*(ptr+2) & 0xc0) >> 6;
      unpacked[3] = *(ptr+2) & 0x3f;

      for(i = 0; i < 4; i++) {

         /* Advance input ptr (4th value comes from already-read 3rd byte) */
         if(i < 3)
            ptr++;

         /* Test for unlatch condition */
         if(unpacked[i] == 0x1f) {
            assert(msg->output[msg->outputIdx] == 0); /* XXX dirty why? */
            return ptr;
         }

         msg->output[msg->outputIdx++] = unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1);
      }

      /* Unlatch is implied if fewer than 3 codewords remain */
      if(dataEnd - ptr < 3) {
         return ptr;
      }
   }

   return ptr;

/* XXX the following version should be safer, but requires testing before replacing the old version
   int bits = 0;
   int bitCount = 0;
   int value;

   while(ptr < dataEnd) {

      if(bitCount < 6) {
         bits = (bits << 8) | *(ptr++);
         bitCount += 8;
      }

      value = bits >> (bitCount - 6);
      bits -= (value << (bitCount - 6));
      bitCount -= 6;

      if(value == 0x1f) {
         assert(bits == 0); // should be padded with zero-value bits
         return ptr;
      }
      msg->output[msg->outputIdx++] = value ^ (((value & 0x20) ^ 0x20) << 1);

      // Unlatch implied if just completed triplet and 1 or 2 words are left
      if(bitCount == 0 && dataEnd - ptr - 1 > 0 && dataEnd - ptr - 1 < 3)
         return ptr;
   }

   assert(bits == 0); // should be padded with zero-value bits
   assert(bitCount == 0); // should be padded with zero-value bits
   return ptr;
*/
}

/**
 * @brief  Decode stream assuming Base 256 encodation
 * @param  msg
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeBase256(DmtxMessage *msg, unsigned char *ptr, unsigned char *dataEnd)
{
   int d0, d1;
   int i;
   unsigned char *ptrEnd;

   /* XXX i is the positional index used for unrandomizing */
   i = ptr - msg->code + 1;

   d0 = UnRandomize255State(*(ptr++), i++);
   if(d0 == 0) {
      ptrEnd = dataEnd;
   }
   else if(d0 <= 249) {
      ptrEnd = ptr + d0;
   }
   else {
      d1 = UnRandomize255State(*(ptr++), i++);
      ptrEnd = ptr + (d0 - 249) * 250 + d1;
   }

   if(ptrEnd > dataEnd) {
      exit(40); /* XXX needs cleaner error handling */
   }

   while(ptr < ptrEnd) {
      msg->output[msg->outputIdx++] = UnRandomize255State(*(ptr++), i++);
   }

   return ptr;
}

/**
 * @brief  Unrandomize 253 state
 * @param  codewordValue
 * @return codewordPosition
 */
/**
static unsigned char
UnRandomize253State(unsigned char codewordValue, int codewordPosition)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * codewordPosition) % 253) + 1;
   tmp = codewordValue - pseudoRandom;

   return (tmp >= 1) ? tmp : tmp + 254;
}
*/

/**
 * @brief  Unrandomize 255 state
 * @param  value
 * @param  idx
 * @return Unrandomized value
 */
static unsigned char
UnRandomize255State(unsigned char value, int idx)
{
   int pseudoRandom;
   int tmp;

   pseudoRandom = ((149 * idx) % 255) + 1;
   tmp = value - pseudoRandom;

   return (tmp >= 0) ? tmp : tmp + 256;
}

/**
 * @brief  Populate array with codeword values based on module colors
 * @param  msg
 * @param  img
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PopulateArrayFromMatrix(DmtxMessage *msg, DmtxImage *img, DmtxRegion *reg)
{
   int weightFactor;
   int mapWidth, mapHeight;
   int xRegionTotal, yRegionTotal;
   int xRegionCount, yRegionCount;
   int xOrigin, yOrigin;
   int mapCol, mapRow;
   int colTmp, rowTmp, idx;
   int tally[24][24]; /* Large enough to map largest single region */

/* memset(msg->array, 0x00, msg->arraySize); */

   /* Capture number of regions present in barcode */
   xRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx);
   yRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx);

   /* Capture region dimensions (not including border modules) */
   mapWidth = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, reg->sizeIdx);
   mapHeight = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, reg->sizeIdx);

   weightFactor = 2 * (mapHeight + mapWidth + 2);
   assert(weightFactor > 0);

   /* Tally module changes for each region in each direction */
   for(yRegionCount = 0; yRegionCount < yRegionTotal; yRegionCount++) {

      /* Y location of mapping region origin in symbol coordinates */
      yOrigin = yRegionCount * (mapHeight + 2) + 1;

      for(xRegionCount = 0; xRegionCount < xRegionTotal; xRegionCount++) {

         /* X location of mapping region origin in symbol coordinates */
         xOrigin = xRegionCount * (mapWidth + 2) + 1;

         memset(tally, 0x00, 24 * 24 * sizeof(int));
         TallyModuleJumps(img, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(img, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(img, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(img, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {

               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;

               if(tally[mapRow][mapCol]/(double)weightFactor >= 0.5)
                  msg->array[idx] = DMTX_MODULE_ON_RGB;
               else
                  msg->array[idx] = DMTX_MODULE_OFF;

               msg->array[idx] |= DMTX_MODULE_ASSIGNED;
            }
         }
      }
   }

   return DMTX_SUCCESS;
}

/**
 * @brief  Increment counters used to determine module values
 * @param  img
 * @param  reg
 * @param  tally
 * @param  xOrigin
 * @param  yOrigin
 * @param  mapWidth
 * @param  mapHeight
 * @param  dir
 * @return void
 */
static void
TallyModuleJumps(DmtxImage *img, DmtxRegion *reg, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, DmtxDirection dir)
{
   int extent, weight;
   int travelStep;
   int symbolRow, symbolCol;
   int mapRow, mapCol;
   int lineStart, lineStop;
   int travelStart, travelStop;
   int *line, *travel;
   int jumpThreshold;
   int darkOnLight;
   int color;
   int statusPrev, statusModule;
   int tPrev, tModule;

   assert(dir == DmtxDirUp || dir == DmtxDirLeft || dir == DmtxDirDown || dir == DmtxDirRight);

   travelStep = (dir == DmtxDirUp || dir == DmtxDirRight) ? 1 : -1;

   /* Abstract row and column progress using pointers to allow grid
      traversal in all 4 directions using same logic */

   if((dir & DmtxDirHorizontal) != 0x00) {
      line = &symbolRow;
      travel = &symbolCol;
      extent = mapWidth;
      lineStart = yOrigin;
      lineStop = yOrigin + mapHeight;
      travelStart = (travelStep == 1) ? xOrigin - 1 : xOrigin + mapWidth;
      travelStop = (travelStep == 1) ? xOrigin + mapWidth : xOrigin - 1;
   }
   else {
      assert(dir & DmtxDirVertical);
      line = &symbolCol;
      travel = &symbolRow;
      extent = mapHeight;
      lineStart = xOrigin;
      lineStop = xOrigin + mapWidth;
      travelStart = (travelStep == 1) ? yOrigin - 1: yOrigin + mapHeight;
      travelStop = (travelStep == 1) ? yOrigin + mapHeight : yOrigin - 1;
   }


   darkOnLight = (int)(reg->offColor > reg->onColor);
   jumpThreshold = abs((int)(0.4 * (reg->offColor - reg->onColor) + 0.5));

   assert(jumpThreshold >= 0);

   for(*line = lineStart; *line < lineStop; (*line)++) {

      /* Capture tModule for each leading border module as normal but
         decide status based on predictable barcode border pattern */

      *travel = travelStart;
      color = ReadModuleColor(img, reg, symbolRow, symbolCol, reg->sizeIdx);
      tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

      statusModule = (travelStep == 1 || (*line & 0x01) == 0) ? DMTX_MODULE_ON_RGB : DMTX_MODULE_OFF;

      weight = extent;

      while((*travel += travelStep) != travelStop) {

         tPrev = tModule;
         statusPrev = statusModule;

         /* For normal data-bearing modules capture color and decide
            module status based on comparison to previous "known" module */

         color = ReadModuleColor(img, reg, symbolRow, symbolCol, reg->sizeIdx);
         tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

         if(statusPrev == DMTX_MODULE_ON_RGB) {
            if(tModule < tPrev - jumpThreshold)
               statusModule = DMTX_MODULE_OFF;
            else
               statusModule = DMTX_MODULE_ON_RGB;
         }
         else if(statusPrev == DMTX_MODULE_OFF) {
            if(tModule > tPrev + jumpThreshold)
               statusModule = DMTX_MODULE_ON_RGB;
            else
               statusModule = DMTX_MODULE_OFF;
         }

         mapRow = symbolRow - yOrigin;
         mapCol = symbolCol - xOrigin;
         assert(mapRow < 24 && mapCol < 24);

         if(statusModule == DMTX_MODULE_ON_RGB)
            tally[mapRow][mapCol] += (2 * weight);

         weight--;
      }

      assert(weight == 0);
   }
}

/**
 * @brief  Populate array with codeword values based on module colors
 * @param  msg
 * @param  img
 * @param  reg
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PopulateArrayFromMosaic(DmtxMessage *msg, DmtxImage *img, DmtxRegion *reg)
{
   int col, row, rowTmp;
   int symbolRow, symbolCol;
   int dataRegionRows, dataRegionCols;
   int color;

   dataRegionRows = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, reg->sizeIdx);
   dataRegionCols = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, reg->sizeIdx);

   memset(msg->array, 0x00, msg->arraySize);

   for(row = 0; row < reg->mappingRows; row++) {

      /* Transform mapping row to symbol row (Swap because the array's
         origin is top-left and everything else is bottom-left) */
      rowTmp = reg->mappingRows - row - 1;
      symbolRow = rowTmp + 2 * (rowTmp / dataRegionRows) + 1;

      for(col = 0; col < reg->mappingCols; col++) {

         /* Transform mapping col to symbol col */
         symbolCol = col + 2 * (col / dataRegionCols) + 1;

/* to fix this function, add rColor, gColor, bColor, and change ReadModuleColor() to accept plane as a parameter */
         color = ReadModuleColor(img, reg, symbolRow, symbolCol, reg->sizeIdx);

         /* Value has been assigned, but not visited */
/*       if(color.R < 50) this is broken for the moment */
         if(color < 50)
            msg->array[row*reg->mappingCols+col] |= DMTX_MODULE_ON_RED;
/*       if(color.G < 50) this is broken for the moment */
         if(color < 50)
            msg->array[row*reg->mappingCols+col] |= DMTX_MODULE_ON_GREEN;
/*       if(color.B < 50) this is broken for the moment */
         if(color < 50)
            msg->array[row*reg->mappingCols+col] |= DMTX_MODULE_ON_BLUE;

         msg->array[row*reg->mappingCols+col] |= DMTX_MODULE_ASSIGNED;
      }
   }

   /* Ideal barcode drawn in lower-right (final) window pane */
/* CALLBACK_DECODE_FUNC2(finalCallback, dec, dec, reg); */

   return DMTX_SUCCESS;
}
