/*
 * azalea/service.c
 *
 * Created by Bun Wong on 16-7-15.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
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
	PHP_ME(azalea_service, request, NULL, ZEND_ACC_PROTECTED)
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

	zend_declare_class_constant_long(azalea_service_ce, ZEND_STRL("METHOD_GET"), AZALEA_SERVICE_METHOD_GET);
	zend_declare_class_constant_long(azalea_service_ce, ZEND_STRL("METHOD_POST"), AZALEA_SERVICE_METHOD_POST);
	zend_declare_class_constant_long(azalea_service_ce, ZEND_STRL("METHOD_PUT"), AZALEA_SERVICE_METHOD_PUT);
	zend_declare_class_constant_long(azalea_service_ce, ZEND_STRL("METHOD_DELETE"), AZALEA_SERVICE_METHOD_DELETE);

	return SUCCESS;
}
/* }}} */

/* {{{ proto get */
PHP_METHOD(azalea_service, get)
{
	azaleaServiceFunction(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_GET);
}
/* }}} */

/* {{{ proto post */
PHP_METHOD(azalea_service, post)
{
	azaleaServiceFunction(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_POST);
}
/* }}} */

/* {{{ proto put */
PHP_METHOD(azalea_service, put)
{
	azaleaServiceFunction(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_PUT);
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_service, delete)
{
	azaleaServiceFunction(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis(), AZALEA_SERVICE_METHOD_DELETE);
}
/* }}} */

/* {{{ proto request */
PHP_METHOD(azalea_service, request)
{
	zend_long method;
	zend_string *serviceUrl;
	zval *arguments = NULL, *reqHeaders = NULL;
	zend_bool returnRawContent = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "lS|azb", &method, &serviceUrl, &arguments, &reqHeaders, &returnRawContent) == FAILURE) {
		return;
	}

	azaleaServiceRequest(getThis(), method, serviceUrl, arguments, Z_TYPE_P(reqHeaders) == IS_ARRAY ? reqHeaders : NULL,
			returnRawContent, return_value);
}
/* }}} */

/* {{{ proto azaleaServiceFunction */
static void azaleaServiceFunction(INTERNAL_FUNCTION_PARAMETERS, azalea_model_t *instance, zend_long method)
{
	zend_string *serviceUrl;
	zval *arguments = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|a", &serviceUrl, &arguments) == FAILURE) {
		return;
	}

	azaleaServiceRequest(instance, method, serviceUrl, arguments, NULL, 0, return_value);
}
/* }}} */

/* {{{ proto azaleaServiceRequest */
static inline void azaleaServiceRequest(azalea_model_t *instance, zend_long method, zend_string *serviceUrl, zval *arguments,
		zval *reqHeaders, zend_bool returnRawContent, zval *return_value)
{
	// curl open once
	void *cp;
//	if (!AZALEA_G(curlHandle)) {
		cp = azaleaCurlOpen();
		if (!cp) {
			throw500Str(ZEND_STRL("Service request start failed."), NULL, NULL, NULL);
			return;
		}
//		AZALEA_G(curlHandle) = cp;
//	} else {
//		cp = AZALEA_G(curlHandle);
//	}

	if (strncasecmp(ZSTR_VAL(serviceUrl), "http://", sizeof("http://") - 1) &&
			strncasecmp(ZSTR_VAL(serviceUrl), "https://", sizeof("https://") - 1)) {
		// add serviceUrl prefix
		zval *purl;
		purl = zend_read_property(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), 0, NULL);
		serviceUrl = strpprintf(0, "%s/%s", Z_STRVAL_P(purl), ZSTR_VAL(serviceUrl));
	} else {
		serviceUrl = zend_string_init(ZSTR_VAL(serviceUrl), ZSTR_LEN(serviceUrl), 0);
	}
	zval serviceArgs;
	if (arguments) {
		array_init(&serviceArgs);
		zend_hash_copy(Z_ARRVAL(serviceArgs), Z_ARRVAL_P(arguments), (copy_ctor_func_t) zval_add_ref);
		arguments = &serviceArgs;
	}
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

	// curl exec
	zend_bool error = 1;
	zend_long retryCount = 0;
	zval *retry = azaleaConfigSubFind("service", "retry");
	if (retry) {
		convert_to_long(retry);
		if (Z_LVAL_P(retry) > 0) {
			retryCount = Z_LVAL_P(retry);
		}
	}
	do {
		zend_long statusCode = azaleaCurlExec(cp, method, &serviceUrl, &arguments, reqHeaders, return_value);
		if (statusCode == 0) {
			if (retryCount-- > 0) {
				// retry
				continue;
			}
			throw500Str(ZEND_STRL("Service response is invalid."), serviceMethod, serviceUrl, arguments);
			break;
		}
		if (statusCode == 200) {
			error = 0;
		} else if (Z_TYPE_P(return_value) == IS_OBJECT) {
			// object
			zval *message = zend_read_property(NULL, return_value, ZEND_STRL("message"), 1, NULL);
			throw500Str(message ? Z_STRVAL_P(message) : "", message ? Z_STRLEN_P(message) : 0, serviceMethod, serviceUrl, arguments);
		} else {
			// string
			throw500Str(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value), serviceMethod, serviceUrl, arguments);
		}
		break;
	} while (1);
	if (arguments) {
		zval_ptr_dtor(arguments);
	}
	zend_string_release(serviceMethod);
	zend_string_release(serviceUrl);
	azaleaCurlClose(cp);

	if (error) {
		return;
	}
	if (!returnRawContent && Z_TYPE_P(return_value) == IS_OBJECT) {
		zval *result = zend_read_property(NULL, return_value, ZEND_STRL("result"), 1, NULL);
		if (result) {
			zval_add_ref(result);
			zval_ptr_dtor(return_value);
			RETURN_ZVAL(result, 0, 0);
		}
	}
}
/* }}} */
