/*
 * azalea/response.c
 *
 * Created by Bun Wong on 16-7-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/controller.h"
#include "azalea/response.h"

zend_class_entry *azalea_response_ce;

/* {{{ class Azalea\Response methods
 */
static zend_function_entry azalea_response_methods[] = {
//	PHP_ME(azalea_response, __init, NULL, ZEND_ACC_PROTECTED)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(response)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Response), azalea_response_methods);
	azalea_response_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_response_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */
