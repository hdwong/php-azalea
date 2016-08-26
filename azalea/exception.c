/*
 * azalea/exception.c
 *
 * Created by Bun Wong on 16-7-10.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/azalea.h"
#include "azalea/namespace.h"
#include "azalea/exception.h"

#include "Zend/zend_exceptions.h"  // for zend_ce_exception

zend_class_entry *azalea_exception_ce;
zend_class_entry *azalea_exception404_ce;
zend_class_entry *azalea_exception500_ce;

/* {{{ class Azalea\Exception methods
 */
static zend_function_entry azalea_exception_methods[] = {
	PHP_ME(azalea_exception, hasServiceException, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class Azalea\Exception methods
 */
static zend_function_entry azalea_e404exception_methods[] = {
	PHP_ME(azalea_exception404, getUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_exception404, getRoute, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class Azalea\Exception methods
 */
static zend_function_entry azalea_e500exception_methods[] = {
	PHP_ME(azalea_exception500, getServiceInfo, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(exception)
{
	zend_class_entry ce;
	zend_class_entry e404_ce;
	zend_class_entry e500_ce;

	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Exception), azalea_exception_methods);
	azalea_exception_ce = zend_register_internal_class_ex(&ce, zend_ce_exception);
	azalea_exception_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	INIT_CLASS_ENTRY(e404_ce, AZALEA_NS_NAME(E404Exception), azalea_e404exception_methods);
	azalea_exception404_ce = zend_register_internal_class_ex(&e404_ce, azalea_exception_ce);
	zend_declare_property_null(azalea_exception404_ce, ZEND_STRL("_uri"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_exception404_ce, ZEND_STRL("_route"), ZEND_ACC_PRIVATE);

	INIT_CLASS_ENTRY(e500_ce, AZALEA_NS_NAME(E500Exception), azalea_e500exception_methods);
	azalea_exception500_ce = zend_register_internal_class_ex(&e500_ce, azalea_exception_ce);
	zend_declare_property_null(azalea_exception500_ce, ZEND_STRL("_method"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_exception500_ce, ZEND_STRL("_url"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_exception500_ce, ZEND_STRL("_arguments"), ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

/* {{{ prote bool hasServiceException(void) */
PHP_METHOD(azalea_exception, hasServiceException)
{
	RETURN_BOOL(AZALEA_G(hasServiceException));
}
/* }}} */

/* {{{ proto string getUri(void) */
PHP_METHOD(azalea_exception404, getUri)
{
	zval *uri;

	uri = zend_read_property(azalea_exception404_ce, getThis(), ZEND_STRL("_uri"), 0, NULL);
	if (!uri) {
		RETURN_NULL();
	}
	RETURN_ZVAL(uri, 1, 0);
}
/* }}} */

/* {{{ proto string getRoute(void) */
PHP_METHOD(azalea_exception404, getRoute)
{
	zval *route;

	route = zend_read_property(azalea_exception404_ce, getThis(), ZEND_STRL("_route"), 0, NULL);
	if (!route) {
		RETURN_NULL();
	}
	RETURN_ZVAL(route, 1, 0);
}
/* }}} */

/* {{{ proto string getServiceUri(void) */
PHP_METHOD(azalea_exception500, getServiceInfo)
{
	zval *method, *url, *arguments;
	azalea_exception_t *instance = getThis();

	method = zend_read_property(azalea_exception500_ce, instance, ZEND_STRL("_method"), 0, NULL);
	url = zend_read_property(azalea_exception500_ce, instance, ZEND_STRL("_url"), 0, NULL);
	arguments = zend_read_property(azalea_exception500_ce, instance, ZEND_STRL("_arguments"), 0, NULL);

	array_init(return_value);
	add_assoc_zval_ex(return_value, ZEND_STRL("method"), method);
	add_assoc_zval_ex(return_value, ZEND_STRL("url"), url);
	add_assoc_zval_ex(return_value, ZEND_STRL("arguments"), arguments);
}
/* }}} */

/* {{{ proto throw404Str */
void throw404Str(const char *message, size_t len)
{
	zval route;
	azalea_exception_t rv = {{0}}, *exception = &rv;

	object_init_ex(exception, azalea_exception404_ce);
	// message
	zend_update_property_stringl(zend_ce_exception, exception, ZEND_STRL("message"), message, len);
	zend_update_property_long(zend_ce_exception, exception, ZEND_STRL("code"), 404);
	// uri
	zend_update_property_str(azalea_exception404_ce, exception, ZEND_STRL("_uri"), AZALEA_G(uri));
	// route
	array_init(&route);
	if (AZALEA_G(folderName)) {
		add_assoc_str(&route, "folder", zend_string_copy(AZALEA_G(folderName)));
	} else {
		add_assoc_null(&route, "folder");
	}
	add_assoc_str(&route, "controller", zend_string_copy(AZALEA_G(controllerName)));
	add_assoc_str(&route, "action", zend_string_copy(AZALEA_G(actionName)));
	add_assoc_zval(&route, "arguments", &AZALEA_G(pathArgs));
	zval_add_ref(&AZALEA_G(pathArgs));
	zend_update_property(azalea_exception404_ce, exception, ZEND_STRL("_route"), &route);
	// check environ
	if (0 == strcmp(ZSTR_VAL(AZALEA_G(environ)), "WEB")) {
		// WEB
		azaleaSetHeaderStr(ZEND_STRL("HTTP/1.1 404 Not Found"), 404);
	}
	zend_throw_exception_object(exception);
}
/* }}} */

/* {{{ proto throw500Str */
void throw500Str(const char *message, size_t len, zend_string *serverMethod, zend_string *serviceUrl, zval *arguments)
{
	azalea_exception_t rv = {{0}}, *exception = &rv;

	object_init_ex(exception, azalea_exception500_ce);
	// message
	zend_update_property_stringl(zend_ce_exception, exception, ZEND_STRL("message"), message, len);
	zend_update_property_long(zend_ce_exception, exception, ZEND_STRL("code"), 500);
	// serviceMethod
	if (serverMethod) {
		zend_update_property_str(azalea_exception500_ce, exception, ZEND_STRL("_method"), serverMethod);
	} else {
		zend_update_property_null(azalea_exception500_ce, exception, ZEND_STRL("_method"));
	}
	// serviceUrl
	if (serviceUrl) {
		zend_update_property_str(azalea_exception500_ce, exception, ZEND_STRL("_url"), serviceUrl);
	} else {
		zend_update_property_null(azalea_exception500_ce, exception, ZEND_STRL("_url"));
	}
	// serviceArguments
	if (arguments) {
		zend_update_property(azalea_exception500_ce, exception, ZEND_STRL("_arguments"), arguments);
		zval_add_ref(arguments);
	}

	// check environ
	if (0 == strcmp(ZSTR_VAL(AZALEA_G(environ)), "WEB")) {
		// WEB
		azaleaSetHeaderStr(ZEND_STRL("HTTP/1.1 500 Internal Server Error"), 500);
	}
	zend_throw_exception_object(exception);
}
/* }}} */
