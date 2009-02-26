/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

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
dmtxDecodeCreate(DmtxImage *img, int scale)
{
   DmtxDecode *dec;
   int width, height;

   dec = (DmtxDecode *)calloc(1, sizeof(DmtxDecode));
   if(dec == NULL)
      return NULL;

   width = dmtxImageGetProp(img, DmtxPropWidth) / scale;
   height = dmtxImageGetProp(img, DmtxPropHeight) / scale;

   dec->edgeMin = DmtxUndefined;
   dec->edgeMax = DmtxUndefined;
   dec->squareDevn = cos(50 * (M_PI/180));
   dec->sizeIdxExpected = DmtxSymbolShapeAuto;
   dec->edgeThresh = 10;
   dec->scale = scale;

   /* Unscaled values */
   dec->scanGap = 1;
   dec->xMin = 0;
   dec->xMax = width - 1;
   dec->yMin = 0;
   dec->yMax = height - 1;

   dec->cache = (unsigned char *)calloc(width * height, sizeof(unsigned char));
   if(dec->cache == NULL) {
      free(dec);
      return NULL;
   }

   dec->image = img;
   dec->grid = InitScanGrid(dec);

   return dec;
}

/**
 * @brief  Deinitialize decode struct
 * @param  dec
 * @return void
 */
extern DmtxPassFail
dmtxDecodeDestroy(DmtxDecode **dec)
{
   if(dec == NULL || *dec == NULL)
      return DmtxFail;

   if((*dec)->cache != NULL)
      free((*dec)->cache);

   free(*dec);

   *dec = NULL;

   return DmtxPass;
}

/**
 * @brief  Set decoding behavior property
 * @param  dec
 * @param  prop
 * @param  value
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxDecodeSetProp(DmtxDecode *dec, int prop, int value)
{
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
      /* Min and Max values arrive unscaled */
      case DmtxPropXmin:
         dec->xMin = value / dec->scale;
         break;
      case DmtxPropXmax:
         dec->xMax = value / dec->scale;
         break;
      case DmtxPropYmin:
         dec->yMin = value / dec->scale;
         break;
      case DmtxPropYmax:
         dec->yMax = value / dec->scale;
         break;
      default:
         break;
   }

   if(dec->squareDevn <= 0.0 || dec->squareDevn >= 1.0)
      return DmtxFail;

   if(dec->scanGap < 1)
      return DmtxFail;

   if(dec->edgeThresh < 1 || dec->edgeThresh > 100)
      return DmtxFail;

   /* Reinitialize scangrid in case any inputs changed */
   dec->grid = InitScanGrid(dec);

   return DmtxPass;
}

/**
 * @brief  Get decoding behavior property
 * @param  dec
 * @param  prop
 * @return value
 */
extern int
dmtxDecodeGetProp(DmtxDecode *dec, int prop)
{
   switch(prop) {
      case DmtxPropEdgeMin:
         return dec->edgeMin;
      case DmtxPropEdgeMax:
         return dec->edgeMax;
      case DmtxPropScanGap:
         return dec->scanGap;
      case DmtxPropSquareDevn:
         return (int)(acos(dec->squareDevn) * 180.0/M_PI);
      case DmtxPropSymbolSize:
         return dec->sizeIdxExpected;
      case DmtxPropEdgeThresh:
         return dec->edgeThresh;
      case DmtxPropXmin:
         return dec->xMin;
      case DmtxPropXmax:
         return dec->xMax;
      case DmtxPropYmin:
         return dec->yMin;
      case DmtxPropYmax:
         return dec->yMax;
      case DmtxPropScale:
         return dec->scale;
      case DmtxPropWidth:
         return dmtxImageGetProp(dec->image, DmtxPropWidth) / dec->scale;
      case DmtxPropHeight:
         return dmtxImageGetProp(dec->image, DmtxPropHeight) / dec->scale;
      default:
         break;
   }

   return DmtxUndefined;
}

/**
 * @brief  Returns xxx
 * @param  img
 * @param  Scaled x coordinate
 * @param  Scaled y coordinate
 * @return Scaled pixel offset
 */
extern unsigned char *
dmtxDecodeGetCache(DmtxDecode *dec, int x, int y)
{
   int width, height;

   assert(dec != NULL);

   width = dmtxDecodeGetProp(dec, DmtxPropWidth);
   height = dmtxDecodeGetProp(dec, DmtxPropHeight);

   if(x < 0 || x >= width || y < 0 || y >= height)
      return NULL;

   return &(dec->cache[y * width + x]);
}

/**
 *
 *
 */
extern DmtxPassFail
dmtxDecodeGetPixelValue(DmtxDecode *dec, int x, int y, int channel, int *value)
{
   int xUnscaled, yUnscaled;
   DmtxPassFail err;

   xUnscaled = x * dec->scale;
   yUnscaled = y * dec->scale;

/* Remove spherical lens distortion */
/* int width, height;
   double radiusPow2, radiusPow4;
   double factor;
   DmtxVector2 pointShifted;
   DmtxVector2 correctedPoint;

   width = dmtxImageGetProp(img, DmtxPropWidth);
   height = dmtxImageGetProp(img, DmtxPropHeight);

   pointShifted.X = point.X - width/2.0;
   pointShifted.Y = point.Y - height/2.0;

   radiusPow2 = pointShifted.X * pointShifted.X + pointShifted.Y * pointShifted.Y;
   radiusPow4 = radiusPow2 * radiusPow2;

   factor = 1 + (k1 * radiusPow2) + (k2 * radiusPow4);

   correctedPoint.X = pointShifted.X * factor + width/2.0;
   correctedPoint.Y = pointShifted.Y * factor + height/2.0;

   return correctedPoint; */

   err = dmtxImageGetPixelValue(dec->image, xUnscaled, yUnscaled, channel, value);

   return err;
}

/**
 * @brief  Convert fitted Data Matrix region into a decoded message
 * @param  dec
 * @param  reg
 * @param  fix
 * @return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *reg, int fix)
{
   int row, col;
   unsigned char *cache;
   DmtxMessage *msg;
   DmtxVector2 p;

   msg = dmtxMessageCreate(reg->sizeIdx, DmtxFormatMatrix);
   if(msg == NULL)
      return NULL;

   if(PopulateArrayFromMatrix(dec, reg, msg) != DmtxPass) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   ModulePlacementEcc200(msg->array, msg->code,
         reg->sizeIdx, DmtxModuleOnRed | DmtxModuleOnGreen | DmtxModuleOnBlue);

   if(DecodeCheckErrors(msg->code, reg->sizeIdx, fix) != DmtxPass) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   for(row = dec->yMin; row < dec->yMax; row++) {
      for(col = dec->xMin; col < dec->xMax; col++) {
         p.X = col;
         p.Y = row;
         dmtxMatrix3VMultiplyBy(&p, reg->raw2fit);
         /* XXX tighten these boundaries can by accounting for barcode size */
         if(p.X >= -0.1 && p.X <= 1.1 && p.Y >= -0.1 && p.Y <= 1.1) {

            cache = dmtxDecodeGetCache(dec, col, row);
            if(cache == NULL)
               continue;
            else
               *cache |= 0x80; /* Mark as visited */
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
dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *reg, int fix)
{
   int offset;
   int colorPlane;
   DmtxMessage *oMsg, *rMsg, *gMsg, *bMsg;

   colorPlane = reg->flowBegin.plane;

   reg->flowBegin.plane = 0; /* kind of a hack */
   rMsg = dmtxDecodeMatrixRegion(dec, reg, fix);

   reg->flowBegin.plane = 1; /* kind of a hack */
   gMsg = dmtxDecodeMatrixRegion(dec, reg, fix);

   reg->flowBegin.plane = 2; /* kind of a hack */
   bMsg = dmtxDecodeMatrixRegion(dec, reg, fix);

   reg->flowBegin.plane = colorPlane;

   oMsg = dmtxMessageCreate(reg->sizeIdx, DmtxFormatMosaic);

   if(oMsg == NULL || rMsg == NULL || gMsg == NULL || bMsg == NULL) {
      dmtxMessageDestroy(&oMsg);
      dmtxMessageDestroy(&rMsg);
      dmtxMessageDestroy(&gMsg);
      dmtxMessageDestroy(&bMsg);
      return NULL;
   }

   offset = 0;
   memcpy(oMsg->output + offset, rMsg->output, rMsg->outputIdx);
   offset += rMsg->outputIdx;
   memcpy(oMsg->output + offset, gMsg->output, gMsg->outputIdx);
   offset += gMsg->outputIdx;
   memcpy(oMsg->output + offset, bMsg->output, bMsg->outputIdx);
   offset += bMsg->outputIdx;

   oMsg->outputIdx = offset;

   dmtxMessageDestroy(&rMsg);
   dmtxMessageDestroy(&gMsg);
   dmtxMessageDestroy(&bMsg);

   return oMsg;
}

/**
 *
 *
 */
extern unsigned char *
dmtxDecodeCreateDiagnostic(DmtxDecode *dec, int *totalBytes, int *headerBytes, int style)
{
   int i, row, col;
   int width, height;
   int widthDigits, heightDigits;
   int count, channelCount;
   int rgb[3];
   double shade;
   unsigned char *pnm, *output, *cache;

   width = dmtxDecodeGetProp(dec, DmtxPropWidth);
   height = dmtxDecodeGetProp(dec, DmtxPropHeight);
   channelCount = dmtxImageGetProp(dec->image, DmtxPropChannelCount);

   style = 1; /* this doesn't mean anything yet */

   /* Count width digits */
   for(widthDigits = 0, i = width; i > 0; i /= 10)
      widthDigits++;

   /* Count height digits */
   for(heightDigits = 0, i = height; i > 0; i /= 10)
      heightDigits++;

   *headerBytes = widthDigits + heightDigits + 9;
   *totalBytes = *headerBytes + width * height * 3;

   pnm = (unsigned char *)malloc(*totalBytes);
   if(pnm == NULL)
      return NULL;

#ifdef _VISUALC_
   count = sprintf_s((char *)pnm, *headerBytes + 1, "P6\n%d %d\n255\n", width, height);
#else
   count = snprintf((char *)pnm, *headerBytes + 1, "P6\n%d %d\n255\n", width, height);
#endif

   if(count != *headerBytes) {
      free(pnm);
      return NULL;
   }

   output = pnm + (*headerBytes);
   for(row = height - 1; row >= 0; row--) {
      for(col = 0; col < width; col++) {
         cache = dmtxDecodeGetCache(dec, col, row);
         if(cache == NULL) {
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 128;
         }
         else if(*cache & 0x40) {
            rgb[0] = 255;
            rgb[1] = 0;
            rgb[2] = 0;
         }
         else {
            shade = (*cache & 0x80) ? 0.0 : 0.7;
            for(i = 0; i < 3; i++) {
               if(i < channelCount)
                  dmtxDecodeGetPixelValue(dec, col, row, i, &rgb[i]);
               else
                  dmtxDecodeGetPixelValue(dec, col, row, 0, &rgb[i]);

               rgb[i] += (int)(shade * (double)(255 - rgb[i]) + 0.5);
               if(rgb[i] > 255)
                  rgb[i] = 255;
            }
         }
         *(output++) = (unsigned char)rgb[0];
         *(output++) = (unsigned char)rgb[1];
         *(output++) = (unsigned char)rgb[2];
      }
   }
   assert(output == pnm + *totalBytes);

   return pnm;
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
            ptr = DecodeSchemeAsciiExt(msg, ptr);
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
 *
 *
 */
static void
PushOutputWord(DmtxMessage *msg, int value)
{
   assert(value >= 0 && value < 256);

   msg->output[msg->outputIdx++] = (unsigned char)value;
}

/**
 *
 *
 */
static void
PushOutputC40TextWord(DmtxMessage *msg, C40TextState *state, int value)
{
   assert(value >= 0 && value < 256);

   msg->output[msg->outputIdx] = (unsigned char)value;

   if(state->upperShift == DmtxTrue) {
      assert(value < 128);
      msg->output[msg->outputIdx] += 128;
   }

   msg->outputIdx++;

   state->shift = DmtxC40TextBasicSet;
   state->upperShift = DmtxFalse;
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
      PushOutputWord(msg, *ptr - 1);
   }
   else if(*ptr == 129) {
      assert(dataEnd >= ptr);
      assert(dataEnd - ptr <= MAXINT);
      msg->padCount = (int)(dataEnd - ptr);
      return dataEnd;
   }
   else if(*ptr <= 229) {
      digits = *ptr - 130;
      PushOutputWord(msg, digits/10 + '0');
      PushOutputWord(msg, digits - (digits/10)*10 + '0');
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
DecodeSchemeAsciiExt(DmtxMessage *msg, unsigned char *ptr)
{
   PushOutputWord(msg, *ptr + 128);

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
   int c40Values[3];
   C40TextState state;

   state.shift = DmtxC40TextBasicSet;
   state.upperShift = DmtxFalse;

   assert(encScheme == DmtxSchemeDecodeC40 || encScheme == DmtxSchemeDecodeText);

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      c40Values[0] = ((packed - 1)/1600);
      c40Values[1] = ((packed - 1)/40) % 40;
      c40Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(state.shift == DmtxC40TextBasicSet) { /* Basic set */
            if(c40Values[i] <= 2) {
               state.shift = c40Values[i] + 1;
            }
            else if(c40Values[i] == 3) {
               PushOutputC40TextWord(msg, &state, ' ');
            }
            else if(c40Values[i] <= 13) {
               PushOutputC40TextWord(msg, &state, c40Values[i] - 13 + '9'); /* 0-9 */
            }
            else if(c40Values[i] <= 39) {
               if(encScheme == DmtxSchemeDecodeC40) {
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'Z'); /* A-Z */
               }
               else if(encScheme == DmtxSchemeDecodeText) {
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 39 + 'z'); /* a-z */
               }
            }
         }
         else if(state.shift == DmtxC40TextShift1) { /* Shift 1 set */
            PushOutputC40TextWord(msg, &state, c40Values[i]); /* ASCII 0 - 31 */
         }
         else if(state.shift == DmtxC40TextShift2) { /* Shift 2 set */
            if(c40Values[i] <= 14) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 33); /* ASCII 33 - 47 */
            }
            else if(c40Values[i] <= 21) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 43); /* ASCII 58 - 64 */
            }
            else if(c40Values[i] <= 26) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 69); /* ASCII 91 - 95 */
            }
            else if(c40Values[i] == 27) {
               PushOutputC40TextWord(msg, &state, 0x1d); /* FNC1 -- XXX depends on position? */
            }
            else if(c40Values[i] == 30) {
               state.upperShift = DmtxTrue;
               state.shift = DmtxC40TextBasicSet;
            }
         }
         else if(state.shift == DmtxC40TextShift3) { /* Shift 3 set */
            if(encScheme == DmtxSchemeDecodeC40) {
               PushOutputC40TextWord(msg, &state, c40Values[i] + 96);
            }
            else if(encScheme == DmtxSchemeDecodeText) {
               if(c40Values[i] == 0)
                  PushOutputC40TextWord(msg, &state, c40Values[i] + 96);
               else if(c40Values[i] <= 26)
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 26 + 'Z'); /* A-Z */
               else
                  PushOutputC40TextWord(msg, &state, c40Values[i] - 31 + 127); /* { | } ~ DEL */
            }
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
   int x12Values[3];

   while(ptr < dataEnd) {

      /* FIXME Also check that ptr+1 is safe to access */
      packed = (*ptr << 8) | *(ptr+1);
      x12Values[0] = ((packed - 1)/1600);
      x12Values[1] = ((packed - 1)/40) % 40;
      x12Values[2] =  (packed - 1) % 40;
      ptr += 2;

      for(i = 0; i < 3; i++) {
         if(x12Values[i] == 0)
            PushOutputWord(msg, 13);
         else if(x12Values[i] == 1)
            PushOutputWord(msg, 42);
         else if(x12Values[i] == 2)
            PushOutputWord(msg, 62);
         else if(x12Values[i] == 3)
            PushOutputWord(msg, 32);
         else if(x12Values[i] <= 13)
            PushOutputWord(msg, x12Values[i] + 44);
         else if(x12Values[i] <= 90)
            PushOutputWord(msg, x12Values[i] + 51);
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

         PushOutputWord(msg, unpacked[i] ^ (((unpacked[i] & 0x20) ^ 0x20) << 1));
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
      PushOutputWord(msg, value ^ (((value & 0x20) ^ 0x20) << 1));

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
   int idx;
   unsigned char *ptrEnd;

   /* Find positional index used for unrandomizing */
   assert(ptr + 1 >= msg->code);
   assert(ptr + 1 - msg->code <= MAXINT);
   idx = (int)(ptr + 1 - msg->code);

   d0 = UnRandomize255State(*(ptr++), idx++);
   if(d0 == 0) {
      ptrEnd = dataEnd;
   }
   else if(d0 <= 249) {
      ptrEnd = ptr + d0;
   }
   else {
      d1 = UnRandomize255State(*(ptr++), idx++);
      ptrEnd = ptr + (d0 - 249) * 250 + d1;
   }

   if(ptrEnd > dataEnd) {
      exit(40); /* XXX needs cleaner error handling */
   }

   while(ptr < ptrEnd) {
      PushOutputWord(msg, UnRandomize255State(*(ptr++), idx++));
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
   if(tmp < 1)
      tmp += 254;

   assert(tmp >= 0 && tmp < 256);

   return (unsigned char)tmp;
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
   if(tmp < 0)
      tmp += 256;

   assert(tmp >= 0 && tmp < 256);

   return (unsigned char)tmp;
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
TallyModuleJumps(DmtxDecode *dec, DmtxRegion *reg, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, DmtxDirection dir)
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
      color = ReadModuleColor(dec, reg, symbolRow, symbolCol, reg->sizeIdx, reg->flowBegin.plane);
      tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

      statusModule = (travelStep == 1 || (*line & 0x01) == 0) ? DmtxModuleOnRGB : DmtxModuleOff;

      weight = extent;

      while((*travel += travelStep) != travelStop) {

         tPrev = tModule;
         statusPrev = statusModule;

         /* For normal data-bearing modules capture color and decide
            module status based on comparison to previous "known" module */

         color = ReadModuleColor(dec, reg, symbolRow, symbolCol, reg->sizeIdx, reg->flowBegin.plane);
         tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

         if(statusPrev == DmtxModuleOnRGB) {
            if(tModule < tPrev - jumpThreshold)
               statusModule = DmtxModuleOff;
            else
               statusModule = DmtxModuleOnRGB;
         }
         else if(statusPrev == DmtxModuleOff) {
            if(tModule > tPrev + jumpThreshold)
               statusModule = DmtxModuleOnRGB;
            else
               statusModule = DmtxModuleOff;
         }

         mapRow = symbolRow - yOrigin;
         mapCol = symbolCol - xOrigin;
         assert(mapRow < 24 && mapCol < 24);

         if(statusModule == DmtxModuleOnRGB)
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
 * @return DmtxPass | DmtxFail
 */
static DmtxPassFail
PopulateArrayFromMatrix(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg)
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
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {

               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;

               if(tally[mapRow][mapCol]/(double)weightFactor >= 0.5)
                  msg->array[idx] = DmtxModuleOnRGB;
               else
                  msg->array[idx] = DmtxModuleOff;

               msg->array[idx] |= DmtxModuleAssigned;
            }
         }
      }
   }

   return DmtxPass;
}
