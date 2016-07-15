/*
 * azalea/service.c
 *
 * Created by Bun Wong on 16-7-15.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/service.h"

zend_class_entry *azalea_service_ce;

/* {{{ class Azalea\ServiceModel methods
 */
static zend_function_entry azalea_service_methods[] = {
	PHP_ME(azalea_service, get, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_service, post, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_service, put, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_service, delete, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(service)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(ServiceModel), azalea_service_methods);
	azalea_service_ce = zend_register_internal_class_ex(&ce, azalea_model_ce);
	azalea_service_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	zend_declare_property_null(azalea_service_ce, ZEND_STRL("serviceUrl"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}
/* }}} */

/* {{{ proto get */
PHP_METHOD(azalea_service, get)
{
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto post */
PHP_METHOD(azalea_service, post)
{
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto put */
PHP_METHOD(azalea_service, put)
{
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_service, delete)
{
	RETURN_TRUE;
}
/* }}} */
