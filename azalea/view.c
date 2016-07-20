/*
 * azalea/view.c
 *
 * Created by Bun Wong on 16-7-11.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/azalea.h"
#include "azalea/namespace.h"
#include "azalea/config.h"
#include "azalea/view.h"
#include "azalea/exception.h"

#include "ext/standard/php_var.h"  // for php_var_dump
#include "ext/standard/php_filestat.h"  // for php_stat
#include "ext/standard/html.h"  // for php_escape_html_entities
#include "main/SAPI.h"  // for SG

zend_class_entry *azalea_view_ce;

/* {{{ class Azalea\View methods
 */
static zend_function_entry azalea_view_methods[] = {
	PHP_ME(azalea_view, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_view, render, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_view, assign, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_view, plain, NULL, ZEND_ACC_PROTECTED)
	ZEND_FENTRY(url, ZEND_FN(azalea_url), NULL, ZEND_ACC_PROTECTED)  // alias for Azalea\url
	ZEND_FENTRY(getConfig, ZEND_MN(azalea_config_get), NULL, ZEND_ACC_PROTECTED)  // alias for Azalea\Config::get
	ZEND_FENTRY(getConfigSub, ZEND_MN(azalea_config_getSub), NULL, ZEND_ACC_PROTECTED)  // alias for Azalea\Config::getSub
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(view)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(View), azalea_view_methods);
	azalea_view_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_view_ce->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azalea_view_ce, ZEND_STRL("_environ"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_view_ce, ZEND_STRL("_data"), ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

/* {{{ proto assignToData */
void assignToData(azalea_view_t *instance, zend_string *name, zval *value)
{
	zval *data;
	if ((data = zend_read_property(azalea_view_ce, instance, ZEND_STRL("_data"), 0, NULL))) {
		zend_hash_update(Z_ARRVAL_P(data), name, value);
	}
}
/* }}} */

/* {{{ proto assignToDataHt */
void assignToDataHt(azalea_view_t *instance, zend_array *ht)
{
	zval *data;
	if (ht && (data = zend_read_property(azalea_view_ce, instance, ZEND_STRL("_data"), 0, NULL))) {
		zend_array *pData = Z_ARRVAL_P(data);
		zend_string *key;
		zval *pVal;
		ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, pVal) {
			if (key) {
				zend_hash_update(pData, key, pVal);
				zval_add_ref(pVal);
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

static int checkValidVarName(char *varName, int len) /* {{{ */
{
	int i, ch;
	if (!varName) {
		return 0;
	}
	/* These are allowed as first char: [a-zA-Z_\x7f-\xff] */
	ch = (int)((unsigned char *)varName)[0];
	if (varName[0] != '_' && (ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
			(ch < 97  /* a    */ || /* z    */ ch > 122) &&
			(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)) {
		return 0;
	}
	/* And these as the rest: [a-zA-Z0-9_\x7f-\xff] */
	if (len > 1) {
		for (i = 1; i < len; i++) {
			ch = (int)((unsigned char *)varName)[i];
			if (varName[i] != '_' && (ch < 48  /* 0    */ || /* 9    */ ch > 57)  &&
					(ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
					(ch < 97  /* a    */ || /* z    */ ch > 122) &&
					(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)) {
				return 0;
			}
		}
	}
	return 1;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_view, __construct) {}
/* }}} */

/* {{{ proto string render(string $tplname, array $data = null) */
PHP_METHOD(azalea_view, render)
{
	zend_string *tplname;
	zval *vars = NULL;
	azalea_view_t *instance = getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|z", &tplname, &vars) == FAILURE) {
		return;
	}

	// check template file exists
	zval exists;
	zend_string *viewsPath = AZALEA_G(viewsPath), *tplPath;
	tplPath = strpprintf(0, "%s%c%s.phtml", ZSTR_VAL(viewsPath), DEFAULT_SLASH, ZSTR_VAL(tplname));
	php_stat(ZSTR_VAL(tplPath), (php_stat_len) ZSTR_LEN(tplPath), FS_IS_R, &exists);
	if (Z_TYPE(exists) == IS_FALSE) {
		zend_string *message = strpprintf(0, "Template file `%s.phtml` not found.", ZSTR_VAL(tplname));
		throw404(message);
		zend_string_release(message);
		RETURN_FALSE;
	}

	zend_string *key;
	zval *pVal;
	// extract environ vars
	zval *environVars, *data;
	if ((environVars = zend_read_property(azalea_view_ce, instance, ZEND_STRL("_environ"), 0, NULL)) &&
			Z_TYPE_P(environVars) == IS_ARRAY) {
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(environVars), key, pVal) {
			if (checkValidVarName(ZSTR_VAL(key), ZSTR_LEN(key)) &&
					EXPECTED(zend_set_local_var(key, pVal, 1) == SUCCESS)) {
				Z_TRY_ADDREF_P(pVal);
			}
		} ZEND_HASH_FOREACH_END();
	}
	// extract vars
	if (vars) {
		assignToDataHt(instance, Z_ARRVAL_P(vars));
	}
	if ((data = zend_read_property(azalea_view_ce, instance, ZEND_STRL("_data"), 0, NULL)) &&
			Z_TYPE_P(data) == IS_ARRAY) {
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(data), key, pVal) {
			if (checkValidVarName(ZSTR_VAL(key), ZSTR_LEN(key)) &&
					EXPECTED(zend_set_local_var(key, pVal, 1) == SUCCESS)) {
				Z_TRY_ADDREF_P(pVal);
			}
		} ZEND_HASH_FOREACH_END();
	}

	php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS);
	if (!azaleaRequire(ZSTR_VAL(tplPath), ZSTR_LEN(tplPath))) {
		zend_string *message = strpprintf(0, "Failed to open template file `%s.phtml`.", ZSTR_VAL(tplname));
		throw404(message);
		zend_string_release(message);
		zend_string_release(tplPath);
		RETURN_FALSE;
	}
	php_output_get_contents(return_value);
	php_output_discard();
	zend_string_release(tplPath);
}
/* }}} */

/* {{{ proto mixed assign(string name, mixed $value = null) */
PHP_METHOD(azalea_view, assign)
{
	zval *name, *value = NULL;
	azalea_view_t *instance = getThis();
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|z", &name, &value) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(name) == IS_ARRAY) {
		assignToDataHt(instance, Z_ARRVAL_P(name));
	} else if (value) {
		convert_to_string_ex(name);
		assignToData(instance, Z_STR_P(name), value);
	} else {
		// ERROR
		php_error_docref(NULL, E_WARNING, "The second argument must be a valid value if the name is a string");
	}
	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ get_default_charset
 */
static char *get_default_charset(void) {
	if (PG(internal_encoding) && PG(internal_encoding)[0]) {
		return PG(internal_encoding);
	} else if (SG(default_charset) && SG(default_charset)[0] ) {
		return SG(default_charset);
	}
	return NULL;
}
/* }}} */

/* {{{ proto string plain(string $string) */
PHP_METHOD(azalea_view, plain)
{
	zend_string *text;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &text) == FAILURE) {
		return;
	}
	text = php_escape_html_entities_ex((unsigned char *) ZSTR_VAL(text), ZSTR_LEN(text), 0, ENT_QUOTES, get_default_charset(), 1);
	RETURN_STR(text);
}
/* }}} */
