/**
 * Copyright (C) 2005 Christian Hentschel
 *
 * This file is part of the libdmtx-php extension.
 *
 * libdmtx-php is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdmtx-php is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Open_cli; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Christian Hentschel
 * chentschel@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <dmtx.h>
#include "php_dmtx.h"

#define PHP_DMTX_IMAGE_RES_NAME "Datamatrix Image"
int le_dmtx_image;

ZEND_DECLARE_MODULE_GLOBALS(dmtx)

static function_entry dmtx_functions[] = {
   PHP_FE(dmtx_write, NULL)
   PHP_FE(dmtx_getRow, NULL)
   PHP_FE(dmtx_getSize, NULL)
   {NULL, NULL, NULL}
};

zend_module_entry dmtx_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
   STANDARD_MODULE_HEADER,
#endif
   PHP_DMTX_EXTNAME,
   dmtx_functions,
   PHP_MINIT(dmtx),
   PHP_MSHUTDOWN(dmtx),
   PHP_RINIT(dmtx),
   NULL,
   NULL,
#if ZEND_MODULE_API_NO >= 20010901
   PHP_DMTX_VERSION,
#endif
   STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DMTX
ZEND_GET_MODULE(dmtx)
#endif

static void php_dmtx_init_globals(zend_dmtx_globals *dmtx_globals)
{
}

PHP_RINIT_FUNCTION(dmtx)
{
   DMTX_G(row_index) = 0;

   return SUCCESS;
}

static void php_dmtx_image_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
   DmtxEncode *enc = (DmtxEncode *)rsrc->ptr;

   if(enc != NULL)
      dmtxEncodeDestroy(&enc);
}

PHP_MINIT_FUNCTION(dmtx)
{
   le_dmtx_image = zend_register_list_destructors_ex(php_dmtx_image_dtor, NULL,
         PHP_DMTX_IMAGE_RES_NAME, module_number);

   ZEND_INIT_MODULE_GLOBALS(dmtx, php_dmtx_init_globals, NULL);

   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(dmtx)
{
   return SUCCESS;
}

PHP_FUNCTION(dmtx_write)
{
   unsigned char *data;
   int data_len;
   int i;
   DmtxEncode *enc;

   if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
         &data, &data_len) == FAILURE)
      RETURN_NULL();

   enc = dmtxEncodeCreate();

   dmtxEncodeSetProp(enc, DmtxPropSizeRequest, DmtxSymbolSquareAuto);

   dmtxEncodeDataMatrix(enc, data_len, data);

   printf("ddd");
   fflush(stdout);

   ZEND_REGISTER_RESOURCE(return_value, enc, le_dmtx_image);
}

PHP_FUNCTION(dmtx_getSize)
{
   DmtxEncode *enc;
   zval *zImage;

   if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zImage) == FAILURE)
      RETURN_NULL();

   ZEND_FETCH_RESOURCE(enc, DmtxEncode *, &zImage, -1, PHP_DMTX_IMAGE_RES_NAME, le_dmtx_image);

   array_init(return_value);
   add_assoc_long(return_value, "width", enc->image->width);
   add_assoc_long(return_value, "height", enc->image->height);
}

PHP_FUNCTION(dmtx_getRow)
{
   int i;
   zval *zImage;
   DmtxEncode *enc;
   DmtxImage *img;

   if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zImage) == FAILURE)
      RETURN_NULL();

   ZEND_FETCH_RESOURCE(enc, DmtxEncode *, &zImage, -1, PHP_DMTX_IMAGE_RES_NAME, le_dmtx_image);
   img = enc->image;

   if(DMTX_G(row_index) >= img->height)
      RETURN_NULL();

   array_init(return_value);

   for(i = 0; i < img->width; i++) {

      int pos = (DMTX_G(row_index) * img->width) + i;
      zval *arr;

      ALLOC_INIT_ZVAL(arr);
      array_init(arr);

      add_assoc_long(arr, "R", img->pxl[pos * 3 + 0]);
      add_assoc_long(arr, "G", img->pxl[pos * 3 + 1]);
      add_assoc_long(arr, "B", img->pxl[pos * 3 + 2]);

      add_next_index_zval(return_value, arr);
   }

   DMTX_G(row_index)++;
}
