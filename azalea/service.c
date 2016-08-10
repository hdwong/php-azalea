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
	PHP_ME(azalea_service, get, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(azalea_service, post, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(azalea_service, put, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(azalea_service, delete, NULL, ZEND_ACC_PROTECTED)
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
	zval *arguments = NULL, *reqHeaders = NULL, serviceArgs;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|aa", &serviceUrl, &arguments, &reqHeaders) == FAILURE) {
		return;
	}

	// curl open
	void *cp = azaleaCurlOpen();
	if (!cp) {
		throw500Str(ZEND_STRL("Service request start failed."), NULL, NULL, NULL);
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
		zend_hash_copy(Z_ARRVAL(serviceArgs), Z_ARRVAL_P(arguments), (copy_ctor_func_t) zval_add_ref);
		arguments = &serviceArgs;
	}

	// curl exec
	zend_long statusCode = azaleaCurlExec(cp, method, &serviceUrl, &arguments, reqHeaders, return_value);
	azaleaCurlClose(cp);

	zend_string *serviceMethod;
	switch (method) {
		case AZALEA_SERVICE_METHOD_GET:
			serviceMethod = zend_string_init(ZEND_STRL("GET"), 0);
			break;
		case AZALEA_SERVICE_METHOD_POST:
			serviceMethod = zend_string_init(ZEND_STRL("POST"), 0);
			break;
		case AZALEA_SERVICE_METHOD_PUT:
			serviceMethod = zend_string_init(ZEND_STRL("PUT"), 0);
			break;
		case AZALEA_SERVICE_METHOD_DELETE:
			serviceMethod = zend_string_init(ZEND_STRL("DELETE"), 0);
			break;
		default:
			serviceMethod = zend_string_init(ZEND_STRL("Unknown method"), 0);
	}
	zend_bool error = 1;
	do {
		if (statusCode == 0) {
			throw500Str(ZEND_STRL("Service response is invalid."), serviceMethod, serviceUrl, arguments);
			break;
		}
		if (statusCode != 200) {
			if (Z_TYPE_P(return_value) == IS_OBJECT) {
				// array
				zval *message = zend_read_property(NULL, return_value, ZEND_STRL("message"), 1, NULL);
				throw500Str(message ? Z_STRVAL_P(message) : "", message ? Z_STRLEN_P(message) : 0, serviceMethod, serviceUrl, arguments);
			} else {
				// string
				throw500Str(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value), serviceMethod, serviceUrl, arguments);
			}
			break;
		}
		error = 0;
	} while (0);
	if (arguments) {
		zval_ptr_dtor(arguments);
	}
	zend_string_release(serviceMethod);
	zend_string_release(serviceUrl);
	if (error) {
		return;
	}
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
