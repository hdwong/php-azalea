/*
 * azalea/request.c
 *
 * Created by Bun Wong on 16-7-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/azalea.h"
#include "azalea/namespace.h"
#include "azalea/request.h"

#include "main/SAPI.h"  // for request_info

zend_class_entry *azalea_request_ce;

/* {{{ class Azalea\Request methods
 */
static zend_function_entry azalea_request_methods[] = {
	PHP_ME(azalea_request, getBaseUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getRequestUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isPost, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isAjax, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getQuery, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getPost, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getCookie, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(request)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Request), azalea_request_methods);
	azalea_request_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_request_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto azaleaGetBaseUri */
PHPAPI zend_string * azaleaGetBaseUri(void)
{
	if (!AZALEA_G(baseUri)) {
		return NULL;
	}
	return zend_string_copy(AZALEA_G(baseUri));
}
/* }}} */

/* {{{ proto azaleaGetUri */
PHPAPI zend_string * azaleaGetUri(void)
{
	if (!AZALEA_G(uri)) {
		return NULL;
	}
	return zend_string_copy(AZALEA_G(uri));
}
/* }}} */

/* {{{ proto azaleaGetRequestUri */
PHPAPI zend_string * azaleaGetRequestUri(void)
{
	zval *field;
	field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("REQUEST_URI"));
	return field ? zend_string_copy(Z_STR_P(field)) : NULL;
}
/* }}} */

/* {{{ proto string getBaseuri(void) */
PHP_METHOD(azalea_request, getBaseUri)
{
	zend_string *ret = azaleaGetBaseUri();
	if (!ret) {
		RETURN_NULL();
	} else {
		RETURN_STR(ret);
	}
}
/* }}} */

/* {{{ proto string getBaseuri(void) */
PHP_METHOD(azalea_request, getUri)
{
	zend_string *ret = azaleaGetUri();
	if (!ret) {
		RETURN_NULL();
	} else {
		RETURN_STR(ret);
	}
}
/* }}} */

/* {{{ proto string getBaseuri(void) */
PHP_METHOD(azalea_request, getRequestUri)
{
	zend_string *ret = azaleaGetRequestUri();
	if (!ret) {
		RETURN_NULL();
	} else {
		RETURN_STR(ret);
	}
}
/* }}} */

/* {{{ proto bool isPost(void) */
PHP_METHOD(azalea_request, isPost)
{
	if (!SG(request_info).request_method) {
		RETURN_FALSE;
	}
	RETURN_BOOL(0 == strcasecmp(SG(request_info).request_method, "POST"));
}
/* }}} */


/* {{{ proto bool isAjax(void) */
PHP_METHOD(azalea_request, isAjax)
{
	zval *field;

	field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_X_REQUESTED_WITH"));
	if (!field) {
		RETURN_FALSE;
	}
	RETURN_BOOL(0 == strcasecmp(Z_STRVAL_P(field), "XMLHttpRequest"));
}
/* }}} */

/* {{{ proto mixed getQuery(name, default) */
PHP_METHOD(azalea_request, getQuery)
{
	zend_string *name = NULL;
	zval *def = NULL;
	zval *val;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|Sz", &name, &def) == FAILURE) {
		return;
	}

	val = azaleaGlobalsFind(TRACK_VARS_GET, name);
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto mixed getPost(name, default) */
PHP_METHOD(azalea_request, getPost)
{
	zend_string *name = NULL;
	zval *def = NULL;
	zval *val;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|Sz", &name, &def) == FAILURE) {
		return;
	}

	val = azaleaGlobalsFind(TRACK_VARS_POST, name);
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto mixed getCookie(name, default) */
PHP_METHOD(azalea_request, getCookie)
{
	zend_string *name = NULL;
	zval *def = NULL;
	zval *val;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|Sz", &name, &def) == FAILURE) {
		return;
	}

	val = azaleaGlobalsFind(TRACK_VARS_COOKIE, name);
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */
