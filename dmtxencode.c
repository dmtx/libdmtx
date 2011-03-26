/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in parent directory for full terms of
 * use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencode.c
 * \brief Encode messages
 */

#undef ISDIGIT
#define ISDIGIT(n) (n > 47 && n < 58)

/**
 * @brief  Initialize encode struct with default values
 * @return Initialized DmtxEncode struct
 */
extern DmtxEncode *
dmtxEncodeCreate(void)
{
   DmtxEncode *enc;

   enc = (DmtxEncode *)calloc(1, sizeof(DmtxEncode));
   if(enc == NULL)
      return NULL;

   enc->scheme = DmtxSchemeAscii;
   enc->sizeIdxRequest = DmtxSymbolSquareAuto;
   enc->marginSize = 10;
   enc->moduleSize = 5;
   enc->pixelPacking = DmtxPack24bppRGB;
   enc->imageFlip = DmtxFlipNone;
   enc->rowPadBytes = 0;

   /* Initialize background color to white */
/* enc.region.gradient.ray.p.R = 255.0;
   enc.region.gradient.ray.p.G = 255.0;
   enc.region.gradient.ray.p.B = 255.0; */

   /* Initialize foreground color to black */
/* enc.region.gradient.tMin = 0.0;
   enc.region.gradient.tMax = xyz; */

   dmtxMatrix3Identity(enc->xfrm);

   return enc;
}

/**
 * @brief  Deinitialize encode struct
 * @param  enc
 * @return void
 */
extern DmtxPassFail
dmtxEncodeDestroy(DmtxEncode **enc)
{
   if(enc == NULL || *enc == NULL)
      return DmtxFail;

   /* Free pixel array allocated in dmtxEncodeDataMatrix() */
   if((*enc)->image != NULL && (*enc)->image->pxl != NULL) {
      free((*enc)->image->pxl);
      (*enc)->image->pxl = NULL;
   }

   dmtxImageDestroy(&((*enc)->image));
   dmtxMessageDestroy(&((*enc)->message));

   free(*enc);

   *enc = NULL;

   return DmtxPass;
}

/**
 * @brief  Set encoding behavior property
 * @param  enc
 * @param  prop
 * @param  value
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxEncodeSetProp(DmtxEncode *enc, int prop, int value)
{
   switch(prop) {

      /* Encoding details */
      case DmtxPropScheme:
         enc->scheme = value;
         break;
      case DmtxPropSizeRequest:
         if(value == DmtxSymbolShapeAuto)
            return DmtxFail;
         enc->sizeIdxRequest = value;
         break;

      /* Presentation details */
      case DmtxPropMarginSize:
         enc->marginSize = value;
         break;
      case DmtxPropModuleSize:
         enc->moduleSize = value;
         break;

      /* Image properties */
      case DmtxPropPixelPacking:
         enc->pixelPacking = value;
         break;
      case DmtxPropImageFlip:
         enc->imageFlip = value;
         break;
      case DmtxPropRowPadBytes:
         enc->rowPadBytes = value;
      default:
         break;
   }

   return DmtxPass;
}

/**
 * @brief  Get encoding behavior property
 * @param  enc
 * @param  prop
 * @return value
 */
extern int
dmtxEncodeGetProp(DmtxEncode *enc, int prop)
{
   switch(prop) {
      case DmtxPropMarginSize:
         return enc->marginSize;
      case DmtxPropModuleSize:
         return enc->moduleSize;
      case DmtxPropScheme:
         return enc->scheme;
      default:
         break;
   }

   return DmtxUndefined;
}

/**
 * @brief  Convert message into Data Matrix image
 * @param  enc
 * @param  inputSize
 * @param  inputString
 * @param  sizeIdxRequest
 * @return DmtxPass | DmtxFail
 */
#ifndef CUSTOM_ENCODEDATAMATRIX
extern DmtxPassFail
dmtxEncodeDataMatrix(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   int dataWordCount;
   int sizeIdx;
   int width, height, bitsPerPixel;
   unsigned char buf[4096];
   unsigned char *pxl;

   /* Encode input string into data codewords */
   sizeIdx = enc->sizeIdxRequest;
   dataWordCount = EncodeDataCodewords(enc, buf, inputString, inputSize, &sizeIdx);
   if(dataWordCount <= 0)
      return(DmtxFail);

   /* EncodeDataCodewords() should have updated any auto sizeIdx to a real one */
   assert(sizeIdx != DmtxSymbolSquareAuto && sizeIdx != DmtxSymbolRectAuto);

   /* XXX we can remove a lot of this redundant data */
   enc->region.sizeIdx = sizeIdx;
   enc->region.symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   enc->region.symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);
   enc->region.mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdx);
   enc->region.mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdx);

   /* Allocate memory for message and array */
   enc->message = dmtxMessageCreate(sizeIdx, DmtxFormatMatrix);
   enc->message->padCount = 0; /* XXX this needs to be added back */
   memcpy(enc->message->code, buf, dataWordCount);

   /* Generate error correction codewords */
   RsEncode(enc->message, enc->region.sizeIdx);

   /* Module placement in region */
   ModulePlacementEcc200(enc->message->array, enc->message->code,
         enc->region.sizeIdx, DmtxModuleOnRGB);

   width = 2 * enc->marginSize + (enc->region.symbolCols * enc->moduleSize);
   height = 2 * enc->marginSize + (enc->region.symbolRows * enc->moduleSize);
   bitsPerPixel = GetBitsPerPixel(enc->pixelPacking);
   if(bitsPerPixel == DmtxUndefined)
      return DmtxFail;
   assert(bitsPerPixel % 8 == 0);

   /* Allocate memory for the image to be generated */
   pxl = (unsigned char *)malloc(width * height * (bitsPerPixel/8) + enc->rowPadBytes);
   if(pxl == NULL) {
      perror("pixel malloc error");
      return DmtxFail;
   }

   enc->image = dmtxImageCreate(pxl, width, height, enc->pixelPacking);
   if(enc->image == NULL) {
      perror("image malloc error");
      return DmtxFail;
   }

   dmtxImageSetProp(enc->image, DmtxPropImageFlip, enc->imageFlip);
   dmtxImageSetProp(enc->image, DmtxPropRowPadBytes, enc->rowPadBytes);

   /* Insert finder and aligment pattern modules */
   PrintPattern(enc);

   return DmtxPass;
}
#endif

/**
 * @brief  Convert message into Data Mosaic image
 * @param  enc
 * @param  inputSize
 * @param  inputString
 * @param  sizeIdxRequest
 * @return DmtxPass | DmtxFail
 */
#ifndef CUSTOM_ENCODEDATAMOSAIC
extern DmtxPassFail
dmtxEncodeDataMosaic(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   int dataWordCount;
   int tmpInputSize;
   unsigned char *inputStart;
   int splitInputSize[3];
   int sizeIdx, sizeIdxRequest;
   int splitSizeIdxAttempt, splitSizeIdxFirst, splitSizeIdxLast;
   unsigned char buf[3][4096];
   DmtxEncode encGreen, encBlue;
   int row, col, mappingRows, mappingCols;

   /* 1) count how many codewords it would take to encode the whole thing
    * 2) take ceiling N of codeword count divided by 3
    * 3) using minimum symbol size that can accomodate N codewords:
    * 4) create several barcodes over iterations of increasing numbers of
    *    input codewords until you go one too far
    * 5) if codewords remain after filling R, G, and B barcodes then go back
    *    to 3 and try with next larger size
    * 6) take the 3 different images you created and write out a new barcode
    */

   /* Encode full input string to establish baseline data codeword count */
   sizeIdx = sizeIdxRequest = enc->sizeIdxRequest;
   /* XXX buf can be changed here to use all 3 buffers' length */
   dataWordCount = EncodeDataCodewords(enc, buf[0], inputString, inputSize, &sizeIdx);
   if(dataWordCount <= 0)
      return DmtxFail;

   /* Use 1/3 (ceiling) of inputSize establish input size target */
   tmpInputSize = (inputSize + 2) / 3;
   splitInputSize[0] = tmpInputSize;
   splitInputSize[1] = tmpInputSize;
   splitInputSize[2] = inputSize - (splitInputSize[0] + splitInputSize[1]);
   /* XXX clean up above lines later for corner cases */

   /* Use 1/3 (floor) of dataWordCount establish first symbol size attempt */
   splitSizeIdxFirst = FindSymbolSize(tmpInputSize, sizeIdxRequest);
   if(splitSizeIdxFirst == DmtxUndefined)
      return DmtxFail;

   /* Set the last possible symbol size for this symbol shape or specific size request */
   if(sizeIdxRequest == DmtxSymbolSquareAuto)
      splitSizeIdxLast = DmtxSymbolSquareCount - 1;
   else if(sizeIdxRequest == DmtxSymbolRectAuto)
      splitSizeIdxLast = DmtxSymbolSquareCount + DmtxSymbolRectCount - 1;
   else
      splitSizeIdxLast = splitSizeIdxFirst;

   /* XXX would be nice if we could choose a size and then fill up each
      layer as we go, but this can cause problems with all data fits on
      first 2 layers.  Revisit this later after things look a bit cleaner. */

   /* Try increasing symbol sizes until 3 of them can hold all input values */
   for(splitSizeIdxAttempt = splitSizeIdxFirst; splitSizeIdxAttempt <= splitSizeIdxLast; splitSizeIdxAttempt++) {

      assert(splitSizeIdxAttempt >= 0);

      /* RED LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart = inputString;
      EncodeDataCodewords(enc, buf[0], inputStart, splitInputSize[0], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      /* GREEN LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart += splitInputSize[0];
      EncodeDataCodewords(enc, buf[1], inputStart, splitInputSize[1], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      /* BLUE LAYER */
      sizeIdx = splitSizeIdxAttempt;
      inputStart += splitInputSize[1];
      EncodeDataCodewords(enc, buf[2], inputStart, splitInputSize[2], &sizeIdx);
      if(sizeIdx != splitSizeIdxAttempt)
         continue;

      break;
   }

   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, splitSizeIdxAttempt);

   /* Now we have the correct lengths for splitInputSize, and they all fit into the desired size */
   encGreen = *enc;
   encBlue = *enc;

   /* First encode red to the main encode struct (image portion will be overwritten) */
   inputStart = inputString;
   dmtxEncodeDataMatrix(enc, splitInputSize[0], inputStart);

   inputStart += splitInputSize[0];
   dmtxEncodeDataMatrix(&encGreen, splitInputSize[1], inputStart);

   inputStart += splitInputSize[1];
   dmtxEncodeDataMatrix(&encBlue, splitInputSize[2], inputStart);

   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, splitSizeIdxAttempt);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, splitSizeIdxAttempt);

   memset(enc->message->array, 0x00, sizeof(unsigned char) * enc->region.mappingRows * enc->region.mappingCols);
   ModulePlacementEcc200(enc->message->array, enc->message->code, enc->region.sizeIdx, DmtxModuleOnRed);

   /* Data Mosaic will traverse this array multiple times -- reset
      DmtxModuleAssigned and DMX_MODULE_VISITED bits before starting */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encGreen.message->code, enc->region.sizeIdx, DmtxModuleOnGreen);

   /* Data Mosaic will traverse this array multiple times -- reset
      DmtxModuleAssigned and DMX_MODULE_VISITED bits before starting */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encBlue.message->code, enc->region.sizeIdx, DmtxModuleOnBlue);

/* dmtxEncodeStructDeInit(&encGreen);
   dmtxEncodeStructDeInit(&encBlue); */

   PrintPattern(enc);

   return DmtxPass;
}
#endif

/**
 * @brief  Convert input into message using specific encodation scheme
 * @param  buf
 * @param  inputString
 * @param  inputSize
 * @param  scheme
 * @param  sizeIdx
 * @return Count of encoded data words
 */
static int
EncodeDataCodewords(DmtxEncode *enc, unsigned char *buf, unsigned char *inputString,
      int inputSize, int *sizeIdx)
{
   int dataWordCount;
   DmtxEncodeStream stream;

   /*
    * This function needs to take both dataWordCount and sizeIdx into account
    * because symbol size is tied to an encodation. That is, a data stream
    * might be different from one symbol size to another
    */

   /* Encode input string into data codewords */
   switch(enc->scheme) {
      case DmtxSchemeAutoBest:
         dataWordCount = EncodeAutoBest(enc, buf, inputString, inputSize);
         break;
      case DmtxSchemeAutoFast:
         dataWordCount = 0;
         /* dataWordCount = EncodeAutoFast(enc, buf, inputString, inputSize); */
         break;
      default:
/*
         dataWordCount = EncodeSingleScheme(enc, buf, inputString, inputSize, enc->scheme);
*/
         stream = StreamInit(inputString, inputSize, buf, 4096);
         EncodeSingleScheme2(&stream, enc->scheme, DmtxSymbolSquareAuto);
         dataWordCount = stream.output.length;
         *sizeIdx = stream.sizeIdx;
/*       dmtxByteListPrint(&(stream.output), "xxx:"); */
         break;
   }


   /* XXX must fix ... will need to handle sizeIdx requests here because it is
      needed by Encode...() for triplet termination */

   /* parameter sizeIdx is requested value, returned sizeIdx is decision */
/* *sizeIdx = FindSymbolSize(dataWordCount, *sizeIdx); */
   if(*sizeIdx == DmtxUndefined)
      return 0;

   return dataWordCount;
}

/**
 * @brief  Write encoded message to image
 * @param  enc
 * @return void
 */
static void
PrintPattern(DmtxEncode *enc)
{
   int i, j;
   int symbolRow, symbolCol;
   int pixelRow, pixelCol;
   int moduleStatus;
   size_t rowSize, height;
   int rgb[3];
   double sxy, txy;
   DmtxMatrix3 m1, m2;
   DmtxVector2 vIn, vOut;

   txy = enc->marginSize;
   sxy = 1.0/enc->moduleSize;

   dmtxMatrix3Translate(m1, -txy, -txy);
   dmtxMatrix3Scale(m2, sxy, -sxy);
   dmtxMatrix3Multiply(enc->xfrm, m1, m2);

   dmtxMatrix3Translate(m1, txy, txy);
   dmtxMatrix3Scale(m2, enc->moduleSize, enc->moduleSize);
   dmtxMatrix3Multiply(enc->rxfrm, m2, m1);

   rowSize = dmtxImageGetProp(enc->image, DmtxPropRowSizeBytes);
   height = dmtxImageGetProp(enc->image, DmtxPropHeight);

   memset(enc->image->pxl, 0xff, rowSize * height);

   for(symbolRow = 0; symbolRow < enc->region.symbolRows; symbolRow++) {
      for(symbolCol = 0; symbolCol < enc->region.symbolCols; symbolCol++) {

         vIn.X = symbolCol;
         vIn.Y = symbolRow;

         dmtxMatrix3VMultiply(&vOut, &vIn, enc->rxfrm);

         pixelCol = (int)(vOut.X);
         pixelRow = (int)(vOut.Y);

         moduleStatus = dmtxSymbolModuleStatus(enc->message,
               enc->region.sizeIdx, symbolRow, symbolCol);

         for(i = pixelRow; i < pixelRow + enc->moduleSize; i++) {
            for(j = pixelCol; j < pixelCol + enc->moduleSize; j++) {
               rgb[0] = ((moduleStatus & DmtxModuleOnRed) != 0x00) ? 0 : 255;
               rgb[1] = ((moduleStatus & DmtxModuleOnGreen) != 0x00) ? 0 : 255;
               rgb[2] = ((moduleStatus & DmtxModuleOnBlue) != 0x00) ? 0 : 255;
/*             dmtxImageSetRgb(enc->image, j, i, rgb); */
               dmtxImageSetPixelValue(enc->image, j, i, 0, rgb[0]);
               dmtxImageSetPixelValue(enc->image, j, i, 1, rgb[1]);
               dmtxImageSetPixelValue(enc->image, j, i, 2, rgb[2]);
            }
         }

      }
   }
}

/**
 * @brief  Encode message using best possible encodation (combine schemes)
 * @param  buf
 * @param  codewords
 * @param  length
 * @return Encoded length of winning channel
 */
static int
EncodeAutoBest(DmtxEncode *enc, unsigned char *buf, unsigned char *codewords, int length)
{
   return 0;
}
