/*
 * azalea/controller.c
 *
 * Created by Bun Wong on 16-7-8.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/controller.h"
#include "azalea/request.h"
#include "azalea/response.h"
#include "azalea/session.h"

zend_class_entry *azalea_controller_ce;

/* {{{ class Azalea\Controller methods
 */
static zend_function_entry azalea_controller_methods[] = {
	PHP_ME(azalea_controller, getRequest, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getResponse, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getSession, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(controller)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Controller), azalea_controller_methods);
	azalea_controller_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_controller_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	return SUCCESS;
}
/* }}} */

/* {{{ proto getRequest */
PHP_METHOD(azalea_controller, getRequest)
{
	object_init_ex(return_value, azalea_request_ce);
}
/* }}} */

/* {{{ proto getResponse */
PHP_METHOD(azalea_controller, getResponse)
{
	object_init_ex(return_value, azalea_response_ce);
}
/* }}} */

/* {{{ proto getSession */
PHP_METHOD(azalea_controller, getSession)
{
	object_init_ex(return_value, azalea_session_ce);
}
/* }}} */
