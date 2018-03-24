/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2008, 2009 Mike Laughton. All rights reserved.
 * Copyright 2012-2016 Vadim A. Misbakh-Soloviov. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact:
 * Vadim A. Misbakh-Soloviov <dmtx@mva.name>
 * Mike Laughton <mike@dragonflylogic.com>
 *
 * \file dmtxencode.c
 * \brief Base encoding logic
 */

#undef ISDIGIT
#define ISDIGIT(n) (n > 47 && n < 58)

/**
 * \brief  Initialize encode struct with default values
 * \return Initialized DmtxEncode struct
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

   enc->fnc1 = DmtxUndefined;

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
 * \brief  Deinitialize encode struct
 * \param  enc
 * \return void
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
 * \brief  Set encoding behavior property
 * \param  enc
 * \param  prop
 * \param  value
 * \return DmtxPass | DmtxFail
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
      case DmtxPropFnc1:
         enc->fnc1 = value;
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
 * \brief  Get encoding behavior property
 * \param  enc
 * \param  prop
 * \return value
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
      case DmtxPropFnc1:
         return enc->fnc1;
      default:
         break;
   }

   return DmtxUndefined;
}

/**
 * \brief  Convert message into Data Matrix image
 * \param  enc
 * \param  inputSize
 * \param  inputString
 * \param  sizeIdxRequest
 * \return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxEncodeDataMatrix(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   int sizeIdx;
   int width, height, bitsPerPixel;
   unsigned char *pxl;
   DmtxByte outputStorage[4096];
   DmtxByteList output = dmtxByteListBuild(outputStorage, sizeof(outputStorage));
   DmtxByteList input = dmtxByteListBuild(inputString, inputSize);

   input.length = inputSize;

   /* Future: stream = StreamInit() ... */
   /* Future: EncodeDataCodewords(&stream) ... */

   /* Encode input string into data codewords */
   sizeIdx = EncodeDataCodewords(&input, &output, enc->sizeIdxRequest, enc->scheme, enc->fnc1);
   if(sizeIdx == DmtxUndefined || output.length <= 0)
      return DmtxFail;

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
   memcpy(enc->message->code, output.b, output.length);

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

/**
 * \brief  Convert message into Data Mosaic image
 *
 *  1) count how many codewords it would take to encode the whole thing
 *  2) take ceiling N of codeword count divided by 3
 *  3) using minimum symbol size that can accomodate N codewords:
 *  4) create several barcodes over iterations of increasing numbers of
 *     input codewords until you go one too far
 *  5) if codewords remain after filling R, G, and B barcodes then go back
 *     to 3 and try with next larger size
 *  6) take the 3 different images you created and write out a new barcode
 *
 * \param  enc
 * \param  inputSize
 * \param  inputString
 * \param  sizeIdxRequest
 * \return DmtxPass | DmtxFail
 */
extern DmtxPassFail
dmtxEncodeDataMosaic(DmtxEncode *enc, int inputSize, unsigned char *inputString)
{
   unsigned char *inputStringR, *inputStringG, *inputStringB;
   int tmpInputSize;
   int inputSizeR, inputSizeG, inputSizeB;
   int sizeIdxAttempt, sizeIdxFirst, sizeIdxLast;
   int row, col, mappingRows, mappingCols;
   DmtxEncode *encR, *encG, *encB;

   /* Use 1/3 (ceiling) of inputSize establish input size target */
   tmpInputSize = (inputSize + 2) / 3;
   inputSizeR = tmpInputSize;
   inputSizeG = tmpInputSize;
   inputSizeB = inputSize - (inputSizeR + inputSizeG);

   inputStringR = inputString;
   inputStringG = inputStringR + inputSizeR;
   inputStringB = inputStringG + inputSizeG;

   /* Use 1/3 (floor) of dataWordCount establish first symbol size attempt */
   sizeIdxFirst = FindSymbolSize(tmpInputSize, enc->sizeIdxRequest);
   if(sizeIdxFirst == DmtxUndefined)
      return DmtxFail;

   /* Set the last possible symbol size for this symbol shape or specific size request */
   if(enc->sizeIdxRequest == DmtxSymbolSquareAuto)
      sizeIdxLast = DmtxSymbolSquareCount - 1;
   else if(enc->sizeIdxRequest == DmtxSymbolRectAuto)
      sizeIdxLast = DmtxSymbolSquareCount + DmtxSymbolRectCount - 1;
   else
      sizeIdxLast = sizeIdxFirst;

   encR = encG = encB = NULL;

   /* Try increasing symbol sizes until 3 of them can hold all input values */
   for(sizeIdxAttempt = sizeIdxFirst; sizeIdxAttempt <= sizeIdxLast; sizeIdxAttempt++)
   {
      dmtxEncodeDestroy(&encR);
      dmtxEncodeDestroy(&encG);
      dmtxEncodeDestroy(&encB);

      encR = dmtxEncodeCreate();
      encG = dmtxEncodeCreate();
      encB = dmtxEncodeCreate();

      /* Copy all settings from master DmtxEncode, including pointer to image
         and message, which is initially null */
      *encR = *encG = *encB = *enc;

      dmtxEncodeSetProp(encR, DmtxPropSizeRequest, sizeIdxAttempt);
      dmtxEncodeSetProp(encG, DmtxPropSizeRequest, sizeIdxAttempt);
      dmtxEncodeSetProp(encB, DmtxPropSizeRequest, sizeIdxAttempt);

      /* RED LAYER - Holds temporary copy */
      dmtxEncodeDataMatrix(encR, inputSizeR, inputStringR);
      if(encR->region.sizeIdx != sizeIdxAttempt)
         continue;

      /* GREEN LAYER - Holds temporary copy */
      dmtxEncodeDataMatrix(encG, inputSizeG, inputStringG);
      if(encG->region.sizeIdx != sizeIdxAttempt)
         continue;

      /* BLUE LAYER - Holds temporary copy */
      dmtxEncodeDataMatrix(encB, inputSizeB, inputStringB);
      if(encB->region.sizeIdx != sizeIdxAttempt)
         continue;

      /* If we get this far we found a fit */
      break;
   }

   if(encR == NULL || encG == NULL || encB == NULL)
   {
      dmtxEncodeDestroy(&encR);
      dmtxEncodeDestroy(&encG);
      dmtxEncodeDestroy(&encB);
      return DmtxFail;
   }

   /* Now we have the correct sizeIdxAttempt, and they all fit into the desired size */

   /* Perform the red portion of the final encode to set internals correctly */
   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, sizeIdxAttempt);
   dmtxEncodeDataMatrix(enc, inputSizeR, inputStringR);

   /* Zero out the array and overwrite the bits in 3 passes */
   mappingRows = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixRows, sizeIdxAttempt);
   mappingCols = dmtxGetSymbolAttribute(DmtxSymAttribMappingMatrixCols, sizeIdxAttempt);
   memset(enc->message->array, 0x00, sizeof(unsigned char) *
         enc->region.mappingRows * enc->region.mappingCols);

   ModulePlacementEcc200(enc->message->array, encR->message->code, sizeIdxAttempt, DmtxModuleOnRed);

   /* Reset DmtxModuleAssigned and DMX_MODULE_VISITED bits */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encG->message->code, sizeIdxAttempt, DmtxModuleOnGreen);

   /* Reset DmtxModuleAssigned and DMX_MODULE_VISITED bits */
   for(row = 0; row < mappingRows; row++) {
      for(col = 0; col < mappingCols; col++) {
         enc->message->array[row*mappingCols+col] &= (0xff ^ (DmtxModuleAssigned | DmtxModuleVisited));
      }
   }

   ModulePlacementEcc200(enc->message->array, encB->message->code, sizeIdxAttempt, DmtxModuleOnBlue);

   /* Destroy encR, encG, and encB */
   dmtxEncodeDestroy(&encR);
   dmtxEncodeDestroy(&encG);
   dmtxEncodeDestroy(&encB);

   PrintPattern(enc);

   return DmtxPass;
}

/**
 * \brief  Convert input into message using specific encodation scheme
 * \param  buf
 * \param  inputString
 * \param  inputSize
 * \param  scheme
 * \param  sizeIdx
 * \return Count of encoded data words
 *
 * Future: pass DmtxEncode to this function with an error reason field, which
 *         goes to EncodeSingle... too
 */
static int
EncodeDataCodewords(DmtxByteList *input, DmtxByteList *output, int sizeIdxRequest, DmtxScheme scheme, int fnc1)
{
   int sizeIdx;

   /* Encode input string into data codewords */
   switch(scheme)
   {
      case DmtxSchemeAutoBest:
         sizeIdx = EncodeOptimizeBest(input, output, sizeIdxRequest, fnc1);
         break;
      case DmtxSchemeAutoFast:
         sizeIdx = DmtxUndefined; /* EncodeAutoFast(input, output, sizeIdxRequest, passFail); */
         break;
      default:
         sizeIdx = EncodeSingleScheme(input, output, sizeIdxRequest, scheme, fnc1);
         break;
   }

   return sizeIdx;
}

/**
 * \brief  Write encoded message to image
 * \param  enc
 * \return void
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

		 if (enc->image->bytesPerPixel == 1)
		 {
			 for(i = pixelRow; i < pixelRow + enc->moduleSize; i++) {
				for(j = pixelCol; j < pixelCol + enc->moduleSize; j++) {
				   rgb[0] = ((moduleStatus & DmtxModuleOnRed) != 0x00) ? 0 : 255;
				   dmtxImageSetPixelValue(enc->image, j, i, 0, rgb[0]);
				}
			 }
		 }
		 else
		 {
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
}
