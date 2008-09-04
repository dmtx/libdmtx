/**
 *      Copyright (C) 2005 Christian Hentschel.
 *
 *      This file is part of the libdmtx-php extension.
 *
 *      libdmtx-php is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      libdmtx-php is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with Open_cli; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *      Christian Hentschel
 *      chentschel@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_dmtx.h"

#include <../../dmtx.h>

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
	DmtxEncode *ec = (DmtxEncode *)rsrc->ptr;

	if (ec)
		dmtxEncodeStructDestroy(&ec);
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

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
		&data, &data_len) == FAILURE)
	{
		RETURN_NULL();
	}

	DmtxEncode *ec = dmtxEncodeStructCreate();
	dmtxEncodeDataMatrix(ec, data_len, data, DMTX_SYMBOL_SQUARE_AUTO);

	printf("ddd");	
	fflush(stdout);

	ZEND_REGISTER_RESOURCE(return_value, ec, le_dmtx_image);
}

PHP_FUNCTION(dmtx_getSize)
{
	DmtxEncode *ec;
	zval *zImage;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zImage) == FAILURE) {
		RETURN_NULL();
	}
	ZEND_FETCH_RESOURCE(ec, DmtxEncode *, &zImage, -1, PHP_DMTX_IMAGE_RES_NAME, le_dmtx_image);

	array_init(return_value);
	add_assoc_long(return_value, "width", ec->image.width);
	add_assoc_long(return_value, "height", ec->image.height);
}

PHP_FUNCTION(dmtx_getRow)
{
	DmtxEncode *ec;
	zval *zImage;
	int i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zImage) == FAILURE) {
		RETURN_NULL();
	}

	ZEND_FETCH_RESOURCE(ec, DmtxEncode *, &zImage, -1, PHP_DMTX_IMAGE_RES_NAME, le_dmtx_image);

	DmtxImage *img = &(ec->image);

	if (DMTX_G(row_index) >= img->height)
		RETURN_NULL();

	array_init(return_value);

	for (i = 0; i < img->width; i++) {
		
		int pos = (DMTX_G(row_index) * img->width) + i;
		zval *arr;

		ALLOC_INIT_ZVAL(arr);		
		array_init(arr);

		add_assoc_long(arr, "R", img->pxl[pos].R);
		add_assoc_long(arr, "G", img->pxl[pos].G);
		add_assoc_long(arr, "B", img->pxl[pos].B);

		add_next_index_zval(return_value, arr);
	}

	DMTX_G(row_index)++;
}
