#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxSobelList *
SobelListCreate(DmtxImage *img)
{
   int sWidth, sHeight;
   DmtxSobelList *list;

   list = (DmtxSobelList *)malloc(sizeof(DmtxSobelList));
   if(list == NULL)
      return NULL;

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   list->vSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeVertical);
   list->bSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeBackslash);
   list->hSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeHorizontal);
   list->sSobel = dmtxValueGridCreate(sWidth, sHeight, DmtxEdgeSlash);

   if(list->vSobel == NULL || list->bSobel == NULL || list->hSobel == NULL || list->sSobel == NULL)
   {
      SobelListDestroy(&list);
      return NULL;
   }

   return list;
}

/**
 *
 *
 */
DmtxPassFail
SobelListDestroy(DmtxSobelList **list)
{
   if(list == NULL || *list == NULL)
      return DmtxFail;

   dmtxValueGridDestroy(&((*list)->sSobel));
   dmtxValueGridDestroy(&((*list)->hSobel));
   dmtxValueGridDestroy(&((*list)->bSobel));
   dmtxValueGridDestroy(&((*list)->vSobel));

   free(*list);
   *list = NULL;

   return DmtxPass;
}

/**
 * 3x3 Sobel Kernel
 *
 */
DmtxPassFail
SobelListPopulate(DmtxDecode2 *dec)
{
   int bytesPerPixel, rowSizeBytes, colorPlane;
   int sx, sy;
   int py, pOffset;
   int vMag, bMag, hMag, sMag;
   int colorLoLf, colorLoMd, colorLoRt;
   int colorMdRt, colorHiRt, colorHiMd;
   int colorHiLf, colorMdLf, colorMdMd;
   int idx;
   int sWidth, sHeight;
   DmtxSobelList *list = dec->sobel;
   DmtxImage *img = dec->image;

   assert(dec != NULL);

   list = dec->sobel;
   img = dec->image;

   assert(list != NULL && img != NULL);

   sWidth = dmtxImageGetProp(img, DmtxPropWidth) - 2;
   sHeight = dmtxImageGetProp(img, DmtxPropHeight) - 2;

   rowSizeBytes = dmtxImageGetProp(img, DmtxPropRowSizeBytes);
   bytesPerPixel = dmtxImageGetProp(img, DmtxPropBytesPerPixel);
   colorPlane = 1; /* XXX need to make some decisions here */

   for(sy = 0; sy < sHeight; sy++)
   {
      py = sHeight - sy;

      pOffset = py * rowSizeBytes + colorPlane;
      colorHiLf = img->pxl[pOffset - rowSizeBytes];
      colorMdLf = img->pxl[pOffset];
      colorLoLf = img->pxl[pOffset + rowSizeBytes];

      pOffset += bytesPerPixel;
      colorHiMd = img->pxl[pOffset - rowSizeBytes];
      colorMdMd = img->pxl[pOffset];
      colorLoMd = img->pxl[pOffset + rowSizeBytes];

      pOffset += bytesPerPixel;
      colorHiRt = img->pxl[pOffset - rowSizeBytes];
      colorMdRt = img->pxl[pOffset];
      colorLoRt = img->pxl[pOffset + rowSizeBytes];

      for(sx = 0; sx < sWidth; sx++)
      {
         /**
          *  -1  0  1
          *  -2  0  2
          *  -1  0  1
          */
         vMag  = colorHiRt;
         vMag += colorMdRt * 2;
         vMag += colorLoRt;
         vMag -= colorHiLf;
         vMag -= colorMdLf * 2;
         vMag -= colorLoLf;

         /**
          *   0  1  2
          *  -1  0  1
          *  -2 -1  0
          */
         bMag  = colorMdLf;
         bMag += colorLoLf * 2;
         bMag += colorLoMd;
         bMag -= colorMdRt;
         bMag -= colorHiRt * 2;
         bMag -= colorHiMd;

         /**
          *   1  2  1
          *   0  0  0
          *  -1 -2 -1
          */
         hMag  = colorHiLf;
         hMag += colorHiMd * 2;
         hMag += colorHiRt;
         hMag -= colorLoLf;
         hMag -= colorLoMd * 2;
         hMag -= colorLoRt;

         /**
          *  -2 -1  0
          *  -1  0  1
          *   0  1  2
          */
         sMag  = colorLoMd;
         sMag += colorLoRt * 2;
         sMag += colorMdRt;
         sMag -= colorHiMd;
         sMag -= colorHiLf * 2;
         sMag -= colorMdLf;

         /**
          * If implementing these operations using MMX, can load 2
          * registers with 4 doubleword values and subtract (PSUBD).
          */

         idx = sy * sWidth + sx;
         list->vSobel->value[idx] = vMag;
         list->bSobel->value[idx] = bMag;
         list->hSobel->value[idx] = hMag;
         list->sSobel->value[idx] = sMag;

         colorHiLf = colorHiMd;
         colorMdLf = colorMdMd;
         colorLoLf = colorLoMd;

         colorHiMd = colorHiRt;
         colorMdMd = colorMdRt;
         colorLoMd = colorLoRt;

         pOffset += bytesPerPixel;
         colorHiRt = img->pxl[pOffset - rowSizeBytes];
         colorMdRt = img->pxl[pOffset];
         colorLoRt = img->pxl[pOffset + rowSizeBytes];
      }
   }

   dec->fn.dmtxValueGridCallback(list->vSobel, 0);
   dec->fn.dmtxValueGridCallback(list->bSobel, 1);
   dec->fn.dmtxValueGridCallback(list->hSobel, 2);
   dec->fn.dmtxValueGridCallback(list->sSobel, 3);

   return DmtxPass;
}
