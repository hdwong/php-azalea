/*
 * azalea/request.c
 *
 * Created by Bun Wong on 16-7-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/controller.h"
#include "azalea/request.h"

zend_class_entry *azalea_request_ce;

/* {{{ class Azalea\Request methods
 */
static zend_function_entry azalea_request_methods[] = {
//	PHP_ME(azalea_request, __init, NULL, ZEND_ACC_PROTECTED)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(request)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Controller), azalea_request_methods);
	azalea_request_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_request_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */
