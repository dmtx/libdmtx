#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxValueGrid *
AccelGridCreate(DmtxValueGrid *sobel, DmtxEdgeType accelEdgeType)
{
   int x, y;
   int aIdx, sInc, sIdx, sIdxNext;
   int sWidth, sHeight;
   int aWidth, aHeight;
   DmtxValueGrid *accel;

   assert(sobel != NULL);

   sWidth = dmtxValueGridGetWidth(sobel);
   sHeight = dmtxValueGridGetHeight(sobel);

   switch(accelEdgeType) {
      case DmtxEdgeVertical:
         aWidth = sWidth - 1;
         aHeight = sHeight;
         sInc = 1;
         break;

      case DmtxEdgeHorizontal:
         aWidth = sWidth;
         aHeight = sHeight - 1;
         sInc = sWidth;
         break;

      default:
         return NULL;
   }

   accel = dmtxValueGridCreate(aWidth, aHeight, accelEdgeType);
   if(accel == NULL)
      return NULL;

   accel->ref = sobel;

   for(y = 0; y < aHeight; y++)
   {
      sIdx = y * sWidth;
      aIdx = y * aWidth;

      for(x = 0; x < aWidth; x++)
      {
         sIdxNext = sIdx + sInc;
         accel->value[aIdx] = sobel->value[sIdxNext] - sobel->value[sIdx];
         aIdx++;
         sIdx++;
      }
   }

   return accel;
}

/**
 *
 *
 */
DmtxPassFail
AccelGridPopulate(DmtxDecode2 *dec)
{
   DmtxSobelList *sobel;

   assert(dec != NULL);
   sobel = dec->sobel;

   dec->vvAccel = AccelGridCreate(sobel->vSobel, DmtxEdgeVertical);
   dec->vbAccel = AccelGridCreate(sobel->bSobel, DmtxEdgeVertical);
   dec->vsAccel = AccelGridCreate(sobel->sSobel, DmtxEdgeVertical);
   dec->hbAccel = AccelGridCreate(sobel->bSobel, DmtxEdgeHorizontal);
   dec->hhAccel = AccelGridCreate(sobel->hSobel, DmtxEdgeHorizontal);
   dec->hsAccel = AccelGridCreate(sobel->sSobel, DmtxEdgeHorizontal);

   if(dec->vvAccel == NULL || dec->vbAccel == NULL || dec->vsAccel == NULL ||
         dec->hbAccel == NULL || dec->hhAccel == NULL || dec->hsAccel == NULL)
      return DmtxFail; /* Memory cleanup will be handled by caller */

   dec->fn.dmtxValueGridCallback(dec->vvAccel, 4);
   dec->fn.dmtxValueGridCallback(dec->vbAccel, 5);
   dec->fn.dmtxValueGridCallback(dec->vsAccel, 6);
   dec->fn.dmtxValueGridCallback(dec->hbAccel, 7);
   dec->fn.dmtxValueGridCallback(dec->hhAccel, 8);
   dec->fn.dmtxValueGridCallback(dec->hsAccel, 9);

   return DmtxPass;
}
