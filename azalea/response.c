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

#include "ext/standard/head.h"	// for php_setcookie
#include "ext/standard/php_var.h"	// for php_var_dump
#include "main/SAPI.h"	// for sapi_header_op

zend_class_entry *azaleaResponseCe;

/* {{{ class Azalea\Response methods
 */
static zend_function_entry azalea_response_methods[] = {
	PHP_ME(azalea_response, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_response, gotoUrl, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, reload, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, gotoRoute, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_response, setHeader, NULL, ZEND_ACC_PUBLIC)
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
	azaleaResponseCe = zend_register_internal_class(&ce);
	azaleaResponseCe->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azaleaResponseCe, ZEND_STRL("_instance"), ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

azalea_response_t * azaleaGetResponse(azalea_controller_t *controller)
{
	zval *controllerName, *pRes;
	zend_string *tstr;

	if (!(controllerName = zend_read_property(azaleaControllerCe, controller, ZEND_STRL("_controller"), 1, NULL))) {
		return NULL;
	}
	tstr = strpprintf(0, "_response_%s", Z_STRVAL_P(controllerName));
	if (!(pRes = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), tstr))) {
		azalea_response_t res = {{0}};
		pRes = &res;
		object_init_ex(pRes, azaleaResponseCe);
		zend_update_property(azaleaResponseCe, pRes, ZEND_STRL("_instance"), controller);
		if (SUCCESS == add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(tstr), ZSTR_LEN(tstr), pRes)) {
			pRes = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), tstr);
		} else {
			// TODO error?
		}
	}
	zend_string_release(tstr);
	return pRes;
}

/* {{{ proto __construct */
PHP_METHOD(azalea_response, __construct) {}
/* }}} */

/* {{{ proto azaleaSetHeaderStr */
static int azaleaSetHeaderStr(char *line, size_t len)
{
	sapi_header_line ctr = {0};
	ctr.line = line;
	ctr.line_len = len;
	return sapi_header_op(SAPI_HEADER_REPLACE, &ctr) == SUCCESS;
}
/* }}} */

/* {{{ proto azaleaSetHeaderStrWithCode */
static int azaleaSetHeaderStrWithCode(char *line, size_t len, zend_long httpCode)
{
	sapi_header_line ctr = {0};
	ctr.line = line;
	ctr.line_len = len;
	ctr.response_code = httpCode;
	return sapi_header_op(SAPI_HEADER_REPLACE, &ctr) == SUCCESS;
}
/* }}} */

/* {{{ proto void gotoUrl(string url, int httpCode) */
PHP_METHOD(azalea_response, gotoUrl)
{
	zend_string *url;
	zend_long httpCode = 302;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|l", &url, &httpCode) == FAILURE) {
		return;
	}

	if (strcmp(ZSTR_VAL(AZALEA_G(environ)), ZSTR_VAL(AG(stringWeb)))) {
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
	azaleaSetHeaderStrWithCode(ZSTR_VAL(ctrLine), ZSTR_LEN(ctrLine), httpCode);
	zend_string_release(ctrLine);
	// exit()
	zend_bailout();
}
/* }}} */

/* {{{ proto void reload(void) */
PHP_METHOD(azalea_response, reload)
{
	if (strcmp(ZSTR_VAL(AZALEA_G(environ)), ZSTR_VAL(AG(stringWeb)))) {
		// not WEB
		return;
	}
	zval *field;
	field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("REQUEST_URI"));
	if (!field) {
		return;
	}
	zend_string *ctrLine = strpprintf(0, "Location: %s", Z_STRVAL_P(field));
	azaleaSetHeaderStrWithCode(ZSTR_VAL(ctrLine), ZSTR_LEN(ctrLine), 302);
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
	zend_bool isCallback = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "a", &array) == FAILURE) {
		return;
	}

	controller = zend_read_property(azaleaResponseCe, getThis(), ZEND_STRL("_instance"), 1, NULL);
	if (!controller) {
		RETURN_FALSE;
	}
	// folder
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("folder"))) && Z_TYPE_P(field) == IS_STRING) {
		folderName = zend_string_copy(Z_STR_P(field));
	} else if ((field = zend_read_property(azaleaControllerCe, controller, ZEND_STRL("_folder"), 1, NULL))
			&& Z_TYPE_P(field) == IS_STRING) {
		// from controller property
		folderName = zend_string_copy(Z_STR_P(field));
	}
	// controller
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("controller"))) && Z_TYPE_P(field) == IS_STRING) {
		controllerName = zend_string_copy(Z_STR_P(field));
	} else if ((field = zend_read_property(azaleaControllerCe, controller, ZEND_STRL("_controller"), 1, NULL))
			&& Z_TYPE_P(field) == IS_STRING) {
		// from controller property
		controllerName = zend_string_copy(Z_STR_P(field));
	} else if ((field = azaleaConfigSubFindEx(ZEND_STRL("dispatch"), ZEND_STRL("default_controller")))) {
		// default controller
		controllerName = zend_string_copy(Z_STR_P(field));
	}
	// action
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("callback"))) && Z_TYPE_P(field) == IS_STRING) {
		actionName = zend_string_copy(Z_STR_P(field));
		isCallback = 1;
	} else {
		if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("action"))) && Z_TYPE_P(field) == IS_STRING) {
			actionName = zend_string_copy(Z_STR_P(field));
		} else if ((field = azaleaConfigSubFindEx(ZEND_STRL("dispatch"), ZEND_STRL("default_action")))) {
			// default action
			actionName = zend_string_copy(Z_STR_P(field));
		}
	}
	// arguments
	if ((field = zend_hash_str_find(Z_ARRVAL_P(array), ZEND_STRL("arguments"))) && Z_TYPE_P(field) == IS_ARRAY) {
		ZVAL_DUP(&pathArgs, field);
	} else {
		array_init(&pathArgs);
	}

	// try to dispatch new route
	RETVAL_NULL();
	azaleaDispatchEx(folderName, controllerName, actionName, isCallback, &pathArgs, return_value);

	if (folderName) {
		zend_string_release(folderName);
	}
	zend_string_release(controllerName);
	zend_string_release(actionName);
	zval_ptr_dtor(&pathArgs);
}
/* }}} */

/* {{{ proto string setHeader(string key, string value) */
PHP_METHOD(azalea_response, setHeader)
{
	zend_string *header, *value;
	int result;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|S", &header, &value) == FAILURE) {
		return;
	}

	if (strcmp(ZSTR_VAL(AZALEA_G(environ)), ZSTR_VAL(AG(stringWeb)))) {
		// not WEB
		return;
	}
	zend_string *ctrLine = strpprintf(0, "%s: %s", ZSTR_VAL(header), ZSTR_VAL(value));
	result = azaleaSetHeaderStr(ZSTR_VAL(ctrLine), ZSTR_LEN(ctrLine));
	zend_string_release(ctrLine);
	RETURN_BOOL(result);
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
	zend_string *name, *value = NULL, *path = NULL;
	zend_long expires = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|SlS", &name, &value, &expires, &path) == FAILURE) {
		return;
	}
	if (expires > 0) {
		expires += (zend_long) time(NULL);
	}
	if (php_setcookie(name, value, expires, path, NULL, 0, 1, 0) == SUCCESS) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */
