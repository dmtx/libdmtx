/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2009 Mackenzie Straight. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 * Copyright 2016 Tim Zaman. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxdecode.c
 * \brief Decode regions
 */

/**
 * \brief  Initialize decode struct with default values
 * \param  img
 * \return Initialized DmtxDecode struct
 */

#include <stdio.h> // for snprintf

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
   dec->scanGap = 1;
   dec->squareDevn = cos(50 * (M_PI/180));
   dec->sizeIdxExpected = DmtxSymbolShapeAuto;
   dec->edgeThresh = 10;

   dec->xMin = 0;
   dec->xMax = width - 1;
   dec->yMin = 0;
   dec->yMax = height - 1;
   dec->scale = scale;

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
 * \brief  Deinitialize decode struct
 * \param  dec
 * \return void
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
 * \brief  Set decoding behavior property
 * \param  dec
 * \param  prop
 * \param  value
 * \return DmtxPass | DmtxFail
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
         dec->scanGap = value; /* XXX Should this be scaled? */
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
 * \brief  Get decoding behavior property
 * \param  dec
 * \param  prop
 * \return value
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
 * \brief  Returns xxx
 * \param  img
 * \param  Scaled x coordinate
 * \param  Scaled y coordinate
 * \return Scaled pixel offset
 */
extern unsigned char *
dmtxDecodeGetCache(DmtxDecode *dec, int x, int y)
{
   int width, height;

   assert(dec != NULL);

/* if(dec.cacheComplete == DmtxFalse)
      CacheImage(); */

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
 * \brief  Fill the region covered by the quadrilateral given by (p0,p1,p2,p3) in the cache.
 */
static void
CacheFillQuad(DmtxDecode *dec, DmtxPixelLoc p0, DmtxPixelLoc p1, DmtxPixelLoc p2, DmtxPixelLoc p3)
{
   DmtxBresLine lines[4];
   DmtxPixelLoc pEmpty = { 0, 0 };
   unsigned char *cache;
   int *scanlineMin, *scanlineMax;
   int minY, maxY, sizeY, posY, posX;
   int i, idx;

   lines[0] = BresLineInit(p0, p1, pEmpty);
   lines[1] = BresLineInit(p1, p2, pEmpty);
   lines[2] = BresLineInit(p2, p3, pEmpty);
   lines[3] = BresLineInit(p3, p0, pEmpty);

   minY = dec->yMax;
   maxY = 0;

   minY = min(minY, p0.Y); maxY = max(maxY, p0.Y);
   minY = min(minY, p1.Y); maxY = max(maxY, p1.Y);
   minY = min(minY, p2.Y); maxY = max(maxY, p2.Y);
   minY = min(minY, p3.Y); maxY = max(maxY, p3.Y);

   sizeY = maxY - minY + 1;

   scanlineMin = (int *)malloc(sizeY * sizeof(int));
   scanlineMax = (int *)calloc(sizeY, sizeof(int));

   assert(scanlineMin); /* XXX handle this better */
   assert(scanlineMax); /* XXX handle this better */

   for(i = 0; i < sizeY; i++)
      scanlineMin[i] = dec->xMax;

   for(i = 0; i < 4; i++) {
      while(lines[i].loc.X != lines[i].loc1.X || lines[i].loc.Y != lines[i].loc1.Y) {
         idx = lines[i].loc.Y - minY;
         scanlineMin[idx] = min(scanlineMin[idx], lines[i].loc.X);
         scanlineMax[idx] = max(scanlineMax[idx], lines[i].loc.X);
         BresLineStep(lines + i, 1, 0);
      }
   }

   for(posY = minY; posY < maxY && posY < dec->yMax; posY++) {
      idx = posY - minY;
      for(posX = scanlineMin[idx]; posX < scanlineMax[idx] && posX < dec->xMax; posX++) {
         cache = dmtxDecodeGetCache(dec, posX, posY);
         if(cache != NULL)
            *cache |= 0x80;
      }
   }

   free(scanlineMin);
   free(scanlineMax);
}

/**
 * \brief  Convert fitted Data Matrix region into a decoded message
 * \param  dec
 * \param  reg
 * \param  fix
 * \return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMatrixRegion(DmtxDecode *dec, DmtxRegion *reg, int fix)
{
   //fprintf(stdout, "libdmtx::dmtxDecodeMatrixRegion()\n");
   DmtxMessage *msg;
   DmtxVector2 topLeft, topRight, bottomLeft, bottomRight;
   DmtxPixelLoc pxTopLeft, pxTopRight, pxBottomLeft, pxBottomRight;

   msg = dmtxMessageCreate(reg->sizeIdx, DmtxFormatMatrix);
   if(msg == NULL)
      return NULL;

   if(PopulateArrayFromMatrix(dec, reg, msg) != DmtxPass) {
      dmtxMessageDestroy(&msg);
      return NULL;
   }

   topLeft.X = bottomLeft.X = topLeft.Y = topRight.Y = -0.1;
   topRight.X = bottomRight.X = bottomLeft.Y = bottomRight.Y = 1.1;

   dmtxMatrix3VMultiplyBy(&topLeft, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&topRight, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&bottomLeft, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&bottomRight, reg->fit2raw);

   pxTopLeft.X = (int)(0.5 + topLeft.X);
   pxTopLeft.Y = (int)(0.5 + topLeft.Y);
   pxBottomLeft.X = (int)(0.5 + bottomLeft.X);
   pxBottomLeft.Y = (int)(0.5 + bottomLeft.Y);
   pxTopRight.X = (int)(0.5 + topRight.X);
   pxTopRight.Y = (int)(0.5 + topRight.Y);
   pxBottomRight.X = (int)(0.5 + bottomRight.X);
   pxBottomRight.Y = (int)(0.5 + bottomRight.Y);

   CacheFillQuad(dec, pxTopLeft, pxTopRight, pxBottomRight, pxBottomLeft);

   return dmtxDecodePopulatedArray(reg->sizeIdx, msg, fix);
}

/**
 * \brief  Ripped out a part of dmtxDecodeMatrixRegion function to this one to parse own array
 * \param  sizeIdx
 * \param  msg
 * \param  fix
 * \return Decoded message (msg pointer) or NULL in case of failure.
 * \note You should reaffect msg with the result of this call
 *       since a NULL result means msg gets freed and should not be used anymore.
 *       ex: msg = dmtxDecodePopulatedArray(sizeidx, msg, fix);
 */
DmtxMessage *
dmtxDecodePopulatedArray(int sizeIdx, DmtxMessage *msg, int fix)
{
   /*
    * Example msg->array indices for a 12x12 datamatrix.
    *  also, the 'L' color (usually black) is defined as 'DmtxModuleOnRGB'
    *
    * XX    XX    XX    XX    XX    XX   
    * XX 0   1  2  3  4  5  6  7  8  9 XX 
    * XX 10 11 12 13 14 15 16 17 18 19 
    * XX 20 21 22 23 24 25 26 27 28 29 XX
    * XX 30 31 32 33 34 35 36 37 38 39 
    * XX 40 41 42 43 44 45 46 47 48 49 XX
    * XX 50 51 52 53 54 55 56 57 58 59 
    * XX 60 61 62 63 64 65 66 67 68 69 XX
    * XX 70 71 72 73 74 75 76 77 78 79 
    * XX 80 81 82 83 84 85 86 87 88 89 XX
    * XX 90 91 92 93 94 95 96 97 98 99 
    * XX XX XX XX XX XX XX XX XX XX XX XX
    *
    */
    
   ModulePlacementEcc200(msg->array, msg->code, sizeIdx, DmtxModuleOnRed | DmtxModuleOnGreen | DmtxModuleOnBlue);

   if(RsDecode(msg->code, sizeIdx, fix) == DmtxFail){
      dmtxMessageDestroy(&msg);
      msg = NULL;
      return NULL;
   }

   DecodeDataStream(msg, sizeIdx, NULL);

   return msg;
}

/**
 * \brief  Convert fitted Data Mosaic region into a decoded message
 * \param  dec
 * \param  reg
 * \param  fix
 * \return Decoded message
 */
extern DmtxMessage *
dmtxDecodeMosaicRegion(DmtxDecode *dec, DmtxRegion *reg, int fix)
{
   int offset;
   int colorPlane;
   DmtxMessage *oMsg, *rMsg, *gMsg, *bMsg;

   colorPlane = reg->flowBegin.plane;

   /**
    * Consider performing a color cube fit here to identify exact RGB of
    * all 6 "cube-like" corners based on pixels located within region. Then
    * force each sample pixel to the "cube-like" corner based o which one
    * is nearest "sqrt(dr^2+dg^2+db^2)" (except sqrt is unnecessary).
    * colorPlane = reg->flowBegin.plane;
    *
    * To find RGB values of primary colors, perform something like a
    * histogram except instead of going from black to color N, go from
    * (127,127,127) to color. Use color bins along with distance to
    * identify value. An additional method will be required to get actual
    * RGB instead of just a plane in 3D. */

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
 * \brief  Increment counters used to determine module values
 * \param  img
 * \param  reg
 * \param  tally
 * \param  xOrigin
 * \param  yOrigin
 * \param  mapWidth
 * \param  mapHeight
 * \param  dir
 * \return void
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
            if(tModule < tPrev - jumpThreshold){
               statusModule = DmtxModuleOff;
            } else {
               statusModule = DmtxModuleOnRGB;
            }
         }
         else if(statusPrev == DmtxModuleOff) {
            if(tModule > tPrev + jumpThreshold) {
               statusModule = DmtxModuleOnRGB;
            } else {
               statusModule = DmtxModuleOff;
            }
         }

         mapRow = symbolRow - yOrigin;
         mapCol = symbolCol - xOrigin;
         assert(mapRow < 24 && mapCol < 24);

         if(statusModule == DmtxModuleOnRGB){
            tally[mapRow][mapCol] += (2 * weight);
         }

         weight--;
      }

      assert(weight == 0);
   }
}

/**
 * \brief  Populate array with codeword values based on module colors
 * \param  msg
 * \param  img
 * \param  reg
 * \return DmtxPass | DmtxFail
 */
static DmtxPassFail
PopulateArrayFromMatrix(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg)
{
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix()\n");
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

   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::reg->sizeIdx: %d\n", reg->sizeIdx);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::reg->flowBegin.plane: %d\n", reg->flowBegin.plane);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::reg->onColor: %d\n", reg->onColor);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::reg->offColor: %d\n", reg->offColor);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::xRegionTotal: %d\n", xRegionTotal);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::yRegionTotal: %d\n", yRegionTotal);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::mapWidth: %d\n", mapWidth);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::mapHeight: %d\n", mapHeight);
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::weightFactor: %d\n", weightFactor);
   //reg->fit2raw[1][0]=0;
   //reg->fit2raw[0][1]=0;
   //reg->fit2raw[0][2]=0;
   //reg->fit2raw[2][2]=1;
   //reg->fit2raw[1][2]=0;
   //reg->fit2raw[2][0]=10; //translation
   //reg->fit2raw[2][1]=10; //translation
   //reg->fit2raw[0][0]=60; //scale
   //reg->fit2raw[1][1]=60; //scale
   //dmtxMatrix3Print(reg->fit2raw);
   

   /* Tally module changes for each region in each direction */
   for(yRegionCount = 0; yRegionCount < yRegionTotal; yRegionCount++) {

      /* Y location of mapping region origin in symbol coordinates */
      yOrigin = yRegionCount * (mapHeight + 2) + 1;

      for(xRegionCount = 0; xRegionCount < xRegionTotal; xRegionCount++) {

         /* X location of mapping region origin in symbol coordinates */
         xOrigin = xRegionCount * (mapWidth + 2) + 1;
         //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::xOrigin: %d\n", xOrigin);

         memset(tally, 0x00, 24 * 24 * sizeof(int));
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
         //for(mapRow = mapHeight-1; mapRow >= 0; mapRow--) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {
               
               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;
               //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::idx: %d @ %d,%d\n", idx, mapCol, mapRow);
               //fprintf(stdout, "%c ",tally[mapRow][mapCol]==DmtxModuleOff ? 'X' : ' ');
               if(tally[mapRow][mapCol]/(double)weightFactor >= 0.5){
                  msg->array[idx] = DmtxModuleOnRGB;
                  //fprintf(stdout, "X ");
               } else {
                  msg->array[idx] = DmtxModuleOff;
                  //fprintf(stdout, "  ");
               }

               msg->array[idx] |= DmtxModuleAssigned;
            }
            //fprintf(stdout, "\n");
         }
      }
   }

   return DmtxPass;
}
