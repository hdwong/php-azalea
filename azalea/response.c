/*
 * azalea/response.c
 *
 * Created by Bun Wong on 16-7-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"
#include "azalea/controller.h"
#include "azalea/response.h"

#include "ext/standard/head.h"  // for php_setcookie
#include "ext/standard/php_var.h"  // for php_var_dump

zend_class_entry *azalea_response_ce;

/* {{{ class Azalea\Response methods
 */
static zend_function_entry azalea_response_methods[] = {
	PHP_ME(azalea_response, __construct, NULL, ZEND_ACC_PRIVATE)
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
	zend_declare_property_null(azalea_response_ce, ZEND_STRL("_instance"), ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_response, __construct) {}
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
		url = azaleaUrl(url, 0);
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
	zval *array, *field, pathArgs;
	azalea_controller_t *controller;
	zend_string *folderName = NULL, *controllerName = NULL, *actionName = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "a", &array) == FAILURE) {
		return;
	}

	controller = zend_read_property(azalea_response_ce, getThis(), ZEND_STRL("_instance"), 0, NULL);
	if (!controller) {
		// TODO controller not set
	}
	// folder
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("folder"))) && Z_TYPE_P(field) == IS_STRING) {
		folderName = zend_string_copy(Z_STR_P(field));
	} else if ((field = zend_read_property(azalea_controller_ce, controller, ZEND_STRL("_folderName"), 0, NULL))
			&& Z_TYPE_P(field) == IS_STRING) {
		// from controller property
		folderName = zend_string_copy(Z_STR_P(field));
	}
	// controller
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("controller"))) && Z_TYPE_P(field) == IS_STRING) {
		controllerName = zend_string_copy(Z_STR_P(field));
	} else if ((field = zend_read_property(azalea_controller_ce, controller, ZEND_STRL("_controllerName"), 0, NULL))
			&& Z_TYPE_P(field) == IS_STRING) {
		// from controller property
		controllerName = zend_string_copy(Z_STR_P(field));
	} else if ((field = azaleaConfigSubFind("dispatch", "default_controller"))) {
		// default controller
		controllerName = zend_string_copy(Z_STR_P(field));
	}
	// action
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("action"))) && Z_TYPE_P(field) == IS_STRING) {
		actionName = zend_string_copy(Z_STR_P(field));
	} else if ((field = azaleaConfigSubFind("dispatch", "default_action"))) {
		// default action
		actionName = zend_string_copy(Z_STR_P(field));
	}
	// arguments
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("arguments"))) && Z_TYPE_P(field) == IS_ARRAY) {
		ZVAL_COPY(&pathArgs, field);
	} else {
		array_init(&pathArgs);
	}

	// try to dispatch new route
	RETVAL_NULL();
	azaleaDispatch(folderName, controllerName, actionName, &pathArgs, return_value);

	if (folderName) {
		zend_string_release(folderName);
	}
	zend_string_release(controllerName);
	zend_string_release(actionName);
	zval_ptr_dtor(&pathArgs);
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
