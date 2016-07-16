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
	azaleaServiceRequest(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_GET);
}
/* }}} */

/* {{{ proto post */
PHP_METHOD(azalea_service, post)
{
	azaleaServiceRequest(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_POST);
}
/* }}} */

/* {{{ proto put */
PHP_METHOD(azalea_service, put)
{
	azaleaServiceRequest(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_PUT);
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_service, delete)
{
	azaleaServiceRequest(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_DELETE);
}
/* }}} */

static void azaleaServiceRequest(INTERNAL_FUNCTION_PARAMETERS, zval *instance, zend_long method)
/* {{{ proto azaleaServiceRequest */
{
	zend_string *serviceUrl;
	zval *arguments;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|a", &serviceUrl, &arguments) == FAILURE) {
		return;
	}

	if (strncasecmp(ZSTR_VAL(serviceUrl), ZEND_STRL("http://")) &&
			strncasecmp(ZSTR_VAL(serviceUrl), ZEND_STRL("https://"))) {
		// add serviceUrl prefix
		zval *purl;
		purl = zend_read_property(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), 0, NULL);
		serviceUrl = strpprintf(0, "%s/%s", Z_STRVAL_P(purl), ZSTR_VAL(serviceUrl));
	} else {
		serviceUrl = zend_string_init(ZSTR_VAL(serviceUrl), ZSTR_LEN(serviceUrl), 0);
	}

	// TODO curl exec

	zend_string_release(serviceUrl);
	RETURN_TRUE;
}
/* }}} */
