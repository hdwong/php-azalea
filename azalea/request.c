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

#include "ext/standard/php_string.h"  // for php_trim
#include "main/SAPI.h"  // for request_info

zend_class_entry *azalea_request_ce;

/* {{{ class Azalea\Request methods
 */
static zend_function_entry azalea_request_methods[] = {
	PHP_ME(azalea_request, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_request, getBaseUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getRequestUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isPost, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isAjax, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getQuery, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getQueryTrim, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getPost, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getPostTrim, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getCookie, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, getHeader, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isMobile, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isWechat, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isQq, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isIosDevice, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_request, isAndroidDevice, NULL, ZEND_ACC_PUBLIC)
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
zend_string * azaleaGetBaseUri(void)
{
	if (!AZALEA_G(baseUri)) {
		return NULL;
	}
	return zend_string_copy(AZALEA_G(baseUri));
}
/* }}} */

/* {{{ proto azaleaGetUri */
zend_string * azaleaGetUri(void)
{
	if (!AZALEA_G(uri)) {
		return NULL;
	}
	return zend_string_copy(AZALEA_G(uri));
}
/* }}} */

/* {{{ proto azaleaGetRequestUri */
zend_string * azaleaGetRequestUri(void)
{
	zval *field;
	zend_string *p = NULL;
	field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("REQUEST_URI"));
	if (field) {
		p = zend_string_copy(Z_STR_P(field));
	} else if (AZALEA_G(baseUri) && AZALEA_G(uri)) {
		p = strpprintf(0, "%s%s", ZSTR_VAL(AZALEA_G(baseUri)), ZSTR_VAL(AZALEA_G(uri)));
	}
	return p;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_request, __construct) {}
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

/* {{{ proto mixed getQueryTrim(name, default) */
PHP_METHOD(azalea_request, getQueryTrim)
{
	PHP_MN(azalea_request_getQuery)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	if (Z_TYPE_P(return_value) == IS_STRING) {
		zend_string *t = Z_STR_P(return_value);
		RETVAL_STR(php_trim(t, ZEND_STRL(" "), 3));
		zend_string_release(t);
	}
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

/* {{{ proto mixed getPostTrim(name, default) */
PHP_METHOD(azalea_request, getPostTrim)
{
	PHP_MN(azalea_request_getPost)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	if (Z_TYPE_P(return_value) == IS_STRING) {
		zend_string *t = Z_STR_P(return_value);
		RETVAL_STR(php_trim(t, ZEND_STRL(" "), 3));
		zend_string_release(t);
	}
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

/* {{{ proto mixed getHeader(name, default) */
PHP_METHOD(azalea_request, getHeader)
{
	zend_string *name = NULL, *tstr;
	zval *def = NULL;
	zval *val;
	char *p, *end;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|z", &name, &def) == FAILURE) {
		return;
	}

	name = php_string_toupper(name);
	p = ZSTR_VAL(name);
	end = p + ZSTR_LEN(name);
	while (p < end) {
		if (*p == '-') {
			*p = '_';
		}
		++p;
	};
	tstr = strpprintf(0, "HTTP_%s", ZSTR_VAL(name));
	val = azaleaGlobalsFind(TRACK_VARS_SERVER, tstr);
	zend_string_release(tstr);
	zend_string_release(name);
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */


/* {{{ proto isMobile */
PHP_METHOD(azalea_request, isMobile)
{
	zval *server, *field;
	zend_string *tstr;

	server = &PG(http_globals)[TRACK_VARS_SERVER];
	if (!server || Z_TYPE_P(server) != IS_ARRAY) {
		RETURN_FALSE;
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_USER_AGENT")))) {
		static const char * uastrs[] = {
			"nokia", "sony", "ericsson", "mot",
			"samsung", "htc", "sgh", "lg", "sharp", "sie-",
			"philips", "panasonic", "alcatel", "lenovo", "iphone",
			"ipod", "ipad", "blackberry", "meizu", "android",
			"netfront", "symbian", "ucweb", "windowsce", "palm",
			"operamini", "operamobi", "openwave", "nexusone", "cldc",
			"midp", "wap", "mobile"
		};
		int i;
		tstr = php_string_tolower(Z_STR_P(field));
		for (i = (sizeof(uastrs) / sizeof(uastrs[0]) - 1); i >= 0; i--) {
			if (php_memnstr(ZSTR_VAL(tstr), uastrs[i], strlen(uastrs[i]), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
				zend_string_release(tstr);
				RETURN_TRUE;
			}
		}
		zend_string_release(tstr);
	}
	if (zend_hash_str_exists(Z_ARRVAL_P(server), ZEND_STRL("HTTP_X_WAP_PROFILE"))) {
		RETURN_TRUE;
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_VIA")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		// find "wap"
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("wap"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_ACCEPT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		// find "vnd.wap.wml"
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("vnd.wap.wml"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto isMobile */
PHP_METHOD(azalea_request, isWechat)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("micromessenger"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto isQq */
PHP_METHOD(azalea_request, isQq)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("qqbrowser"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto isIosDevice */
PHP_METHOD(azalea_request, isIosDevice)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("iphone"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr)) ||
				php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("ipod"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr)) ||
				php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("ipad"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto isAndroidDevice */
PHP_METHOD(azalea_request, isAndroidDevice)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("android"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */
