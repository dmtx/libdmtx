#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include "../../dmtx.h"
#include "multi_test.h"

/**
 *
 *
 */
DmtxAccel *
AccelCreate(DmtxSobel *sobel)
{
   int sWidth, sHeight;
   int vWidth, vHeight;
   int hWidth, hHeight;
   DmtxAccel *accel;

   accel = (DmtxAccel *)calloc(1, sizeof(DmtxAccel));
   if(accel == NULL)
      return NULL;

   sWidth = dmtxValueGridGetWidth(sobel->v);
   sHeight = dmtxValueGridGetHeight(sobel->v);

   vWidth = sWidth - 1;
   vHeight = sHeight;

   hWidth = sWidth;
   hHeight = sHeight - 1;

   accel->vv = dmtxValueGridCreate(vWidth, vHeight, DmtxEdgeVertical, sobel->v);
   accel->vb = dmtxValueGridCreate(vWidth, vHeight, DmtxEdgeVertical, sobel->b);
   accel->vs = dmtxValueGridCreate(vWidth, vHeight, DmtxEdgeVertical, sobel->s);
   accel->hb = dmtxValueGridCreate(hWidth, hHeight, DmtxEdgeHorizontal, sobel->b);
   accel->hh = dmtxValueGridCreate(hWidth, hHeight, DmtxEdgeHorizontal, sobel->h);
   accel->hs = dmtxValueGridCreate(hWidth, hHeight, DmtxEdgeHorizontal, sobel->s);

   if(accel->vv == NULL || accel->vb == NULL || accel->vs == NULL ||
         accel->hb == NULL || accel->hh == NULL || accel->hs == NULL)
   {
      AccelDestroy(&accel);
      return NULL;
   }

   return accel;
}

/**
 *
 *
 */
DmtxPassFail
AccelDestroy(DmtxAccel **accel)
{
   if(accel == NULL || *accel == NULL)
      return DmtxFail;

   dmtxValueGridDestroy(&((*accel)->vs));
   dmtxValueGridDestroy(&((*accel)->hs));
   dmtxValueGridDestroy(&((*accel)->hh));
   dmtxValueGridDestroy(&((*accel)->hb));
   dmtxValueGridDestroy(&((*accel)->vb));
   dmtxValueGridDestroy(&((*accel)->vv));

   free(*accel);
   *accel = NULL;

   return DmtxPass;
}

#define RETURN_FAIL_IF(c) if(c) { return DmtxFail; }

/**
 *
 *
 */
DmtxPassFail
AccelPopulate(DmtxDecode2 *dec)
{
   DmtxAccel *accel;

   assert(dec != NULL && dec->accel != NULL);

   accel = dec->accel;

   RETURN_FAIL_IF(AccelPopulateLocal(accel->vv) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulateLocal(accel->vb) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulateLocal(accel->hb) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulateLocal(accel->hh) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulateLocal(accel->hs) == DmtxFail);
   RETURN_FAIL_IF(AccelPopulateLocal(accel->vs) == DmtxFail);

   dec->fn.dmtxValueGridCallback(accel->vv, 4);
   dec->fn.dmtxValueGridCallback(accel->vb, 5);
   dec->fn.dmtxValueGridCallback(accel->hb, 7);
   dec->fn.dmtxValueGridCallback(accel->hh, 8);
   dec->fn.dmtxValueGridCallback(accel->hs, 9);
   dec->fn.dmtxValueGridCallback(accel->vs, 6);

   return DmtxPass;
}

#undef RETURN_FAIL_IF

/**
 *
 *
 */
DmtxPassFail
AccelPopulateLocal(DmtxValueGrid *acc)
{
   int sWidth, sHeight;
   int aWidth, aHeight;
   int sIdx, sIdxNext, sInc, aIdx;
   int x, y;
   DmtxValueGrid *sob;

   sob = acc->ref;

   sWidth = dmtxValueGridGetWidth(sob);
   sHeight = dmtxValueGridGetHeight(sob);

   switch(acc->type) {
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
         return DmtxFail;
   }

   for(y = 0; y < aHeight; y++)
   {
      sIdx = y * sWidth;
      aIdx = y * aWidth;

      for(x = 0; x < aWidth; x++)
      {
         sIdxNext = sIdx + sInc;
         acc->value[aIdx] = sob->value[sIdxNext] - sob->value[sIdx];
         aIdx++;
         sIdx++;
      }
   }

   return DmtxPass;
}
