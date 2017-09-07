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

#include "Zend/zend_exceptions.h"	// for zend_ce_exception

zend_class_entry *azaleaExceptionCe;
zend_class_entry *azaleaException404Ce;
zend_class_entry *azaleaException500Ce;

/* {{{ class Azalea\Exception methods
 */
static zend_function_entry azalea_exception_methods[] = {
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
	azaleaExceptionCe = zend_register_internal_class_ex(&ce, zend_ce_exception);
	azaleaExceptionCe->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	INIT_CLASS_ENTRY(e404_ce, AZALEA_NS_NAME(E404Exception), azalea_e404exception_methods);
	azaleaException404Ce = zend_register_internal_class_ex(&e404_ce, azaleaExceptionCe);
	zend_declare_property_null(azaleaException404Ce, ZEND_STRL("_uri"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azaleaException404Ce, ZEND_STRL("_route"), ZEND_ACC_PRIVATE);

	INIT_CLASS_ENTRY(e500_ce, AZALEA_NS_NAME(E500Exception), azalea_e500exception_methods);
	azaleaException500Ce = zend_register_internal_class_ex(&e500_ce, azaleaExceptionCe);

	return SUCCESS;
}
/* }}} */

/* {{{ proto string getUri(void) */
PHP_METHOD(azalea_exception404, getUri)
{
	zval *uri;

	uri = zend_read_property(azaleaException404Ce, getThis(), ZEND_STRL("_uri"), 0, NULL);
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

	route = zend_read_property(azaleaException404Ce, getThis(), ZEND_STRL("_route"), 0, NULL);
	if (!route) {
		RETURN_NULL();
	}
	RETURN_ZVAL(route, 1, 0);
}
/* }}} */

/* {{{ proto throw404Str */
void throw404Str(const char *message, size_t len)
{
	zval route;
	azalea_exception_t rv = {{0}}, *exception = &rv;

	object_init_ex(exception, azaleaException404Ce);
	// message
	zend_update_property_stringl(zend_ce_exception, exception, ZEND_STRL("message"), message, len);
	zend_update_property_long(zend_ce_exception, exception, ZEND_STRL("code"), 404);
	// uri
	zend_update_property_str(azaleaException404Ce, exception, ZEND_STRL("_uri"), AZALEA_G(uri));
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
	zend_update_property(azaleaException404Ce, exception, ZEND_STRL("_route"), &route);
	zval_ptr_dtor(&route);
	zend_throw_exception_object(exception);
}
/* }}} */

/* {{{ proto throw500Str */
void throw500Str(const char *message, size_t len)
{
	azalea_exception_t rv = {{0}}, *exception = &rv;

	object_init_ex(exception, azaleaException500Ce);
	// message
	zend_update_property_stringl(zend_ce_exception, exception, ZEND_STRL("message"), message, len);
	zend_update_property_long(zend_ce_exception, exception, ZEND_STRL("code"), 500);
	zend_throw_exception_object(exception);
}
/* }}} */
