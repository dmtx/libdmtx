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
 * @brief  XXX
 * @param  image
 * @return Initialized DmtxDecode struct
 */
extern DmtxDecode
dmtxDecodeStructInit(DmtxImage *img)
{
   DmtxDecode dec;
   int cacheSize;

   memset(&dec, 0x00, sizeof(DmtxDecode));

   dec.image = img;

   cacheSize = dmtxImageGetProp(img, DmtxPropWidth) *
         dmtxImageGetProp(img, DmtxPropHeight) * sizeof(DmtxCompassEdge);

   memset(dec.image->compass, 0x00, cacheSize);

   dec.shrinkMin = 1;
   dec.shrinkMax = 1;
   dec.edgeThresh = 10;
   dec.squareDevn = cos(40 * (M_PI/180));
   dec.scanGap = 1;

   dec.grid = InitScanGrid(img, dec.scanGap);

   return dec;
}

/**
 * @brief  XXX
 * @param  dec
 * @return void
 */
extern void
dmtxDecodeStructDeInit(DmtxDecode *dec)
{
   memset(dec, 0x00, sizeof(DmtxDecode));
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
      case DmtxPropShrinkMin:
         dec->shrinkMin = value;
         break;
      case DmtxPropShrinkMax:
         dec->shrinkMax = value;
         err = dmtxImageSetProp(dec->image, DmtxPropScale, value);
         if(err == DMTX_FAILURE)
            return DMTX_FAILURE;
         dec->grid = InitScanGrid(dec->image, dec->scanGap);
         break;
      case DmtxPropEdgeThresh:
         dec->edgeThresh = value;
         break;
      case DmtxPropSquareDevn:
         dec->squareDevn = cos(value * (M_PI/180.0));
         break;
      case DmtxPropScanGap:
         dec->scanGap = value;
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
   }

   /* Minimum image scale can't be larger than maximum image scale */
   if(dec->shrinkMin < 1 || dec->shrinkMax < dec->shrinkMin)
      return DMTX_FAILURE;

   if(dec->squareDevn < 0.0 || dec->squareDevn > 1.0)
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
 * @brief  XXX
 * @param  dec
 * @param  region
 * @param  fix
 * @return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *region, int fix)
{
   DmtxMessage *message;

   message = dmtxMessageMalloc(region->sizeIdx, DMTX_FORMAT_MATRIX);
   if(message == NULL)
      return NULL;

   if(PopulateArrayFromMatrix(message, dec->image, region) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   ModulePlacementEcc200(message->array, message->code,
         region->sizeIdx, DMTX_MODULE_ON_RED | DMTX_MODULE_ON_GREEN | DMTX_MODULE_ON_BLUE);

   if(DecodeCheckErrors(message->code, region->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   DecodeDataStream(message, region->sizeIdx, NULL);

   return message;
}

/**
 * @brief  XXX
 * @param  dec
 * @param  region
 * @param  fix
 * @return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *region, int fix)
{
   int row, col;
   int mappingRows, mappingCols;
   DmtxMessage *message;
   DmtxMessage rMesg, gMesg, bMesg;

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, region->sizeIdx);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, region->sizeIdx);

   message = dmtxMessageMalloc(region->sizeIdx, DMTX_FORMAT_MOSAIC);
   if(message == NULL)
      return NULL;

   rMesg = gMesg = bMesg = *message;
   rMesg.codeSize = gMesg.codeSize = bMesg.codeSize = message->codeSize/3;

   gMesg.code += gMesg.codeSize;
   bMesg.code += (bMesg.codeSize * 2);

   if(PopulateArrayFromMosaic(message, dec->image, region) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   ModulePlacementEcc200(message->array, rMesg.code, region->sizeIdx, DMTX_MODULE_ON_RED);
   if(DecodeCheckErrors(rMesg.code, region->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   for(row = 0; row < mappingRows; row++)
      for(col = 0; col < mappingCols; col++)
         message->array[row*mappingCols+col] &= (0xff ^ DMTX_MODULE_VISITED);

   ModulePlacementEcc200(message->array, gMesg.code, region->sizeIdx, DMTX_MODULE_ON_GREEN);
   if(DecodeCheckErrors(gMesg.code, region->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   for(row = 0; row < mappingRows; row++)
      for(col = 0; col < mappingCols; col++)
         message->array[row*mappingCols+col] &= (0xff ^ DMTX_MODULE_VISITED);

   ModulePlacementEcc200(message->array, bMesg.code, region->sizeIdx, DMTX_MODULE_ON_BLUE);
   if(DecodeCheckErrors(bMesg.code, region->sizeIdx, fix) != DMTX_SUCCESS) {
      dmtxMessageFree(&message);
      return NULL;
   }

   DecodeDataStream(&rMesg, region->sizeIdx, NULL);
   DecodeDataStream(&gMesg, region->sizeIdx, rMesg.output + rMesg.outputIdx);
   DecodeDataStream(&bMesg, region->sizeIdx, gMesg.output + gMesg.outputIdx);

   message->outputIdx = rMesg.outputIdx + gMesg.outputIdx + bMesg.outputIdx;

   return message;
}

/**
 * @brief  XXX
 * @param  message
 * @param  sizeIdx
 * @param  outputStart
 * @return void
 */
static void
DecodeDataStream(DmtxMessage *message, int sizeIdx, unsigned char *outputStart)
{
   DmtxSchemeDecode encScheme;
   unsigned char *ptr, *dataEnd;

   message->output = (outputStart == NULL) ? message->output : outputStart;
   message->outputIdx = 0;

   ptr = message->code;
   dataEnd = ptr + dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, sizeIdx);

   while(ptr < dataEnd) {

      ptr = NextEncodationScheme(&encScheme, ptr);

      switch(encScheme) {
         case DmtxSchemeDecodeAsciiStd:
            ptr = DecodeSchemeAsciiStd(message, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeAsciiExt:
            ptr = DecodeSchemeAsciiExt(message, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeC40:
         case DmtxSchemeDecodeText:
            ptr = DecodeSchemeC40Text(message, ptr, dataEnd, encScheme);
            break;

         case DmtxSchemeDecodeX12:
            ptr = DecodeSchemeX12(message, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeEdifact:
            ptr = DecodeSchemeEdifact(message, ptr, dataEnd);
            break;

         case DmtxSchemeDecodeBase256:
            ptr = DecodeSchemeBase256(message, ptr, dataEnd);
            break;
      }
   }
}

/**
 * @brief  XXX
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
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeAsciiStd(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd)
{
   int digits;

   if(*ptr <= 128) {
      message->output[message->outputIdx++] = *ptr - 1;
   }
   else if(*ptr == 129) {
      return dataEnd;
   }
   else if(*ptr <= 229) {
      digits = *ptr - 130;
      message->output[message->outputIdx++] = digits/10 + '0';
      message->output[message->outputIdx++] = digits - (digits/10)*10 + '0';
   }

   return ptr + 1;
}

/**
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeAsciiExt(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd)
{
   message->output[message->outputIdx++] = *ptr + 128;

   return ptr + 1;
}

/**
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @param  encScheme
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeC40Text(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd, DmtxSchemeDecode encScheme)
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
               message->output[message->outputIdx++] = ' '; /* Space */
            }
            else if(c40Values[i] <= 13) {
               message->output[message->outputIdx++] = c40Values[i] - 13 + '9'; /* 0-9 */
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeDecodeC40) {
                  message->output[message->outputIdx++] = c40Values[i] - 39 + 'Z'; /* A-Z */
               }
               else if(encScheme == DmtxSchemeDecodeText) {
                  message->output[message->outputIdx++] = c40Values[i] - 39 + 'z'; /* a-z */
               }
            }
         }
         else if(shift == 1) { /* Shift 1 set */
            message->output[message->outputIdx++] = c40Values[i]; /* ASCII 0 - 31 */

            shift = 0;
         }
         else if(shift == 2) { /* Shift 2 set */
            if(c40Values[i] <= 14)
               message->output[message->outputIdx++] = c40Values[i] + 33; /* ASCII 33 - 47 */
            else if(c40Values[i] <= 21)
               message->output[message->outputIdx++] = c40Values[i] + 43; /* ASCII 58 - 64 */
            else if(c40Values[i] <= 26)
               message->output[message->outputIdx++] = c40Values[i] + 69; /* ASCII 91 - 95 */
            else if(c40Values[i] == 27)
               fprintf(stdout, "FNC1 (?)"); /* FNC1 (eh?) */
            else if(c40Values[i] == 30)
               fprintf(stdout, "Upper Shift (?)"); /* Upper Shift (eh?) */

            shift = 0;
         }
         else if(shift == 3) { /* Shift 3 set */
            if(encScheme == DmtxSchemeDecodeC40) {
               message->output[message->outputIdx++] = c40Values[i] + 96;
            }
            else if(encScheme == DmtxSchemeDecodeText) {
               if(c40Values[i] == 0)
                  message->output[message->outputIdx++] = c40Values[i] + 96;
               else if(c40Values[i] <= 26)
                  message->output[message->outputIdx++] = c40Values[i] - 26 + 'Z'; /* A-Z */
               else
                  message->output[message->outputIdx++] = c40Values[i] - 31 + 127; /* { | } ~ DEL */
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
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeX12(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd)
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
            message->output[message->outputIdx++] = 13;
         else if(x12Values[i] == 1)
            message->output[message->outputIdx++] = 42;
         else if(x12Values[i] == 2)
            message->output[message->outputIdx++] = 62;
         else if(x12Values[i] == 3)
            message->output[message->outputIdx++] = 32;
         else if(x12Values[i] <= 13)
            message->output[message->outputIdx++] = x12Values[i] + 44;
         else if(x12Values[i] <= 90)
            message->output[message->outputIdx++] = x12Values[i] + 51;
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
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeEdifact(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd)
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
            assert(message->output[message->outputIdx] == 0); /* XXX dirty why? */
            return ptr;
         }

         message->output[message->outputIdx++] = unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1);
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
      message->output[message->outputIdx++] = value ^ (((value & 0x20) ^ 0x20) << 1);

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
 * @brief  XXX
 * @param  message
 * @param  ptr
 * @param  dataEnd
 * @return Pointer to next undecoded codeword
 */
static unsigned char *
DecodeSchemeBase256(DmtxMessage *message, unsigned char *ptr, unsigned char *dataEnd)
{
   int d0, d1;
   int i;
   unsigned char *ptrEnd;

   /* XXX i is the positional index used for unrandomizing */
   i = ptr - message->code + 1;

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
      message->output[message->outputIdx++] = UnRandomize255State(*(ptr++), i++);
   }

   return ptr;
}

/**
 *
 * @param XXX
 * @return XXX
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
 * @brief  XXX
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
 * @brief  XXX
 * @param  message
 * @param  image
 * @param  region
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PopulateArrayFromMatrix(DmtxMessage *message, DmtxImage *image, DmtxRegion *region)
{
   int weightFactor;
   int mapWidth, mapHeight;
   int xRegionTotal, yRegionTotal;
   int xRegionCount, yRegionCount;
   int xOrigin, yOrigin;
   int mapCol, mapRow;
   int colTmp, rowTmp, idx;
   int tally[24][24]; /* Large enough to map largest single region */

/* memset(message->array, 0x00, message->arraySize); */

   /* Capture number of regions present in barcode */
   xRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx);
   yRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx);

   /* Capture region dimensions (not including border modules) */
   mapWidth = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, region->sizeIdx);
   mapHeight = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, region->sizeIdx);

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
         TallyModuleJumps(image, region, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(image, region, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(image, region, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(image, region, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {

               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;

               if(tally[mapRow][mapCol]/(double)weightFactor > 0.5)
                  message->array[idx] = DMTX_MODULE_ON_RGB;
               else
                  message->array[idx] = DMTX_MODULE_OFF;

               message->array[idx] |= DMTX_MODULE_ASSIGNED;
            }
         }
      }
   }

   return DMTX_SUCCESS;
}

/**
 * @brief  XXX
 * @param  image
 * @param  region
 * @param  tally
 * @param  xOrigin
 * @param  yOrigin
 * @param  mapWidth
 * @param  mapHeight
 * @param  dir
 * @return void
 */
static void
TallyModuleJumps(DmtxImage *image, DmtxRegion *region, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, DmtxDirection dir)
{
   int extent, weight;
   int travelStep;
   int symbolRow, symbolCol;
   int mapRow, mapCol;
   int lineStart, lineStop;
   int travelStart, travelStop;
   int *line, *travel;
   double jumpThreshold;
   DmtxColor3 color;
   int statusPrev, statusModule;
   double tPrev, tModule;

   assert(dir == DmtxDirUp || dir == DmtxDirLeft || dir == DmtxDirDown || dir == DmtxDirRight);

   travelStep = (dir == DmtxDirUp || dir == DmtxDirRight) ? 1 : -1;

   /* Abstract row and column progress using pointers to allow grid
      traversal in all 4 directions using same logic */

   if(dir & DmtxDirHorizontal) {
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

   jumpThreshold = 0.3 * (region->gradient.tMax - region->gradient.tMin);

   assert(jumpThreshold > 0);

   for(*line = lineStart; *line < lineStop; (*line)++) {

      /* Capture tModule for each leading border module as normal but
         decide status based on predictable barcode border pattern */

      *travel = travelStart;
      color = ReadModuleColor(image, region, symbolRow, symbolCol, region->sizeIdx);
      tModule = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);

      statusModule = (travelStep == 1 || !(*line & 0x01)) ? DMTX_MODULE_ON_RGB : DMTX_MODULE_OFF;

      weight = extent;

      while((*travel += travelStep) != travelStop) {

         tPrev = tModule;
         statusPrev = statusModule;

         /* For normal data-bearing modules capture color and decide
            module status based on comparison to previous "known" module */

         color = ReadModuleColor(image, region, symbolRow, symbolCol, region->sizeIdx);
         tModule = dmtxDistanceAlongRay3(&(region->gradient.ray), &color);

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
 * @brief  XXX
 * @param  message
 * @param  image
 * @param  region
 * @return DMTX_SUCCESS | DMTX_FAILURE
 */
static int
PopulateArrayFromMosaic(DmtxMessage *message, DmtxImage *image, DmtxRegion *region)
{
   int col, row, rowTmp;
   int symbolRow, symbolCol;
   int dataRegionRows, dataRegionCols;
   DmtxColor3 color;

   dataRegionRows = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, region->sizeIdx);
   dataRegionCols = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, region->sizeIdx);

   memset(message->array, 0x00, message->arraySize);

   for(row = 0; row < region->mappingRows; row++) {

      /* Transform mapping row to symbol row (Swap because the array's
         origin is top-left and everything else is bottom-left) */
      rowTmp = region->mappingRows - row - 1;
      symbolRow = rowTmp + 2 * (rowTmp / dataRegionRows) + 1;

      for(col = 0; col < region->mappingCols; col++) {

         /* Transform mapping col to symbol col */
         symbolCol = col + 2 * (col / dataRegionCols) + 1;

         color = ReadModuleColor(image, region, symbolRow, symbolCol, region->sizeIdx);

         /* Value has been assigned, but not visited */
         if(color.R < 50)
            message->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_RED;
         if(color.G < 50)
            message->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_GREEN;
         if(color.B < 50)
            message->array[row*region->mappingCols+col] |= DMTX_MODULE_ON_BLUE;

         message->array[row*region->mappingCols+col] |= DMTX_MODULE_ASSIGNED;
      }
   }

   /* Ideal barcode drawn in lower-right (final) window pane */
/* CALLBACK_DECODE_FUNC2(finalCallback, dec, dec, region); */

   return DMTX_SUCCESS;
}
