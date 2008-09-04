#ifndef PHP_DMTX_H
#define PHP_DMTX_H 1

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(dmtx)
	long row_index;
ZEND_END_MODULE_GLOBALS(dmtx)

#ifdef ZTS
#define DMTX_G(v) TSRMG(dmtx_globals_id, zend_dmtx_globals *, v)
#else
#define DMTX_G(v) (dmtx_globals.v)
#endif


#define PHP_DMTX_VERSION "1.0"
#define PHP_DMTX_EXTNAME "dmtx"


PHP_MINIT_FUNCTION(dmtx);
PHP_MSHUTDOWN_FUNCTION(dmtx);
PHP_RINIT_FUNCTION(dmtx);

PHP_FUNCTION(dmtx_write);
PHP_FUNCTION(dmtx_getRow);
PHP_FUNCTION(dmtx_getSize);

extern zend_module_entry dmtx_module_entry;
#define phpext_libdmtx_ptr &dmtx_module_entry

#endif
