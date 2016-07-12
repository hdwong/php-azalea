/*
 * azalea/response.c
 *
 * Created by Bun Wong on 16-7-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/controller.h"
#include "azalea/response.h"

#include "ext/standard/head.h"  // for php_setcookie

zend_class_entry *azalea_response_ce;

/* {{{ class Azalea\Response methods
 */
static zend_function_entry azalea_response_methods[] = {
	PHP_ME(azalea_response, gotoUrl, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, gotoRoute, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, getBody, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, setBody, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, setCookie, NULL, ZEND_ACC_PUBLIC)
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

/* {{{ proto void gotoUrl(string url, int httpCode) */
PHP_METHOD(azalea_response, gotoUrl)
{
	zend_string *url = NULL;
	zend_long httpCode = 302;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|l", &url, &httpCode) == FAILURE) {
		return;
	}

	if (strcmp(ZSTR_VAL(AZALEA_G(environ)), "WEB")) {
		// not WEB
		return;
	}
	if (strncasecmp(ZSTR_VAL(url), ZEND_STRL("http://")) &&
			strncasecmp(ZSTR_VAL(url), ZEND_STRL("https://"))) {
		// add url prefix
		url = azaleaUrl(url, false);
	}
	zend_string *ctrLine = strpprintf(0, "Location: %s", ZSTR_VAL(url));
	zend_string_release(url);
	azaleaSetHeader(ctrLine, httpCode);
	zend_string_release(ctrLine);
	// exit()
	zend_bailout();
}
/* }}} */

/* {{{ proto mixed gotoRoute(array route) */
PHP_METHOD(azalea_response, gotoRoute)
{
	zend_array *route;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "a", &route) == FAILURE) {
		return;
	}

	// TODO getRoute

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getBody(void) */
PHP_METHOD(azalea_response, getBody)
{
	if (php_output_get_contents(return_value) == FAILURE) {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void setBody(string body) */
PHP_METHOD(azalea_response, setBody)
{
	char *body;
	size_t len;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "s", &body, &len) == FAILURE) {
		return;
	}

	if (!OG(active)) {
		php_error_docref("ref.outcontrol", E_NOTICE, "failed to delete buffer. No buffer to delete");
		RETURN_FALSE;
	}
	if (SUCCESS != php_output_clean()) {
		php_error_docref("ref.outcontrol", E_NOTICE, "failed to delete buffer of %s (%d)", ZSTR_VAL(OG(active)->name), OG(active)->level);
		RETURN_FALSE;
	}
	PHPWRITE(body, len);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getBody(string key, mixed field, int expire = 0) */
PHP_METHOD(azalea_response, setCookie)
{
	zend_string *name, *value = NULL;
	zend_long expires = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|Sl", &name, &value, &expires) == FAILURE) {
		return;
	}
	if (expires > 0) {
		expires += (zend_long) time(NULL);
	}
	if (php_setcookie(name, value, expires, NULL, NULL, 0, 1, 0) == SUCCESS) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */
