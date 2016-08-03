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
#include "azalea/exception.h"
#include "azalea/transport_curl.h"

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
	zval *arguments = NULL, serviceArgs;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|a", &serviceUrl, &arguments) == FAILURE) {
		return;
	}

	if (strncasecmp(ZSTR_VAL(serviceUrl), "http://", sizeof("http://") - 1) &&
			strncasecmp(ZSTR_VAL(serviceUrl), "https://", sizeof("https://") - 1)) {
		// add serviceUrl prefix
		zval *purl;
		purl = zend_read_property(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), 0, NULL);
		serviceUrl = strpprintf(0, "%s/%s", Z_STRVAL_P(purl), ZSTR_VAL(serviceUrl));
	} else {
		serviceUrl = zend_string_init(ZSTR_VAL(serviceUrl), ZSTR_LEN(serviceUrl), 0);
	}
	if (arguments) {
		array_init(&serviceArgs);
		azaleaDeepCopy(&serviceArgs, arguments);
		arguments = &serviceArgs;
	}

	// curl exec
	void *cp = azaleaCurlOpen();
	if (!cp) {
		zend_string_release(serviceUrl);
		throw500Str(ZEND_STRL("Service request start failed."), "", "", NULL);
		return;
	}
	long statusCode = azaleaCurlExec(cp, method, &serviceUrl, &arguments, return_value);
	azaleaCurlClose(cp);

	char *pServiceMethod;
	switch (method) {
		case AZALEA_SERVICE_METHOD_GET:
			pServiceMethod = "GET";
			break;
		case AZALEA_SERVICE_METHOD_POST:
			pServiceMethod = "POST";
			break;
		case AZALEA_SERVICE_METHOD_PUT:
			pServiceMethod = "PUT";
			break;
		case AZALEA_SERVICE_METHOD_DELETE:
			pServiceMethod = "DELETE";
			break;
		default:
			pServiceMethod = "Unknown method";
	}
	if (statusCode == 0) {
		throw500Str(ZEND_STRL("Service response is invalid."), pServiceMethod, ZSTR_VAL(serviceUrl), arguments);
		zend_string_release(serviceUrl);
		return;
	}
	if (statusCode != 200) {
		if (Z_TYPE_P(return_value) == IS_OBJECT) {
			// array
			zval *message = zend_read_property(NULL, return_value, ZEND_STRL("message"), 1, NULL);
			throw500Str(message ? Z_STRVAL_P(message) : "", message ? Z_STRLEN_P(message) : 0, pServiceMethod, ZSTR_VAL(serviceUrl), arguments);
		} else {
			// string
			throw500Str(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value), pServiceMethod, ZSTR_VAL(serviceUrl), arguments);
		}
		zend_string_release(serviceUrl);
		return;
	}
	if (arguments) {
		zval_ptr_dtor(arguments);
	}
	zend_string_release(serviceUrl);
	if (Z_TYPE_P(return_value) == IS_OBJECT) {
		zval *result = zend_read_property(NULL, return_value, ZEND_STRL("result"), 1, NULL);
		if (result) {
			zval_add_ref(result);
			zval_ptr_dtor(return_value);
			RETURN_ZVAL(result, 0, 0);
		}
	}
}
/* }}} */
