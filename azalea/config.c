/*
 * azalea/config.c
 *
 * Created by Bun Wong on 16-6-29.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"

#include "ext/standard/php_var.h"
#include "ext/standard/php_string.h"  // for php_explode

zend_class_entry *azaleaConfigCe;

/* {{{ class Azalea\Config methods */
static zend_function_entry azalea_config_methods[] = {
	PHP_ME(azalea_config, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_config, getAll, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(config)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Config), azalea_config_methods);
	azaleaConfigCe = zend_register_internal_class(&ce TSRMLS_CC);
	azaleaConfigCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ php_simple_ini_parser_cb */
static void php_simple_ini_parser_cb(zval *arg1, zval *arg2, zval *arg3, int callback_type, zval *arr)
{
	zval element;
	switch (callback_type) {
		case ZEND_INI_PARSER_ENTRY:
			if (!arg2) {
				break;
			}
			ZVAL_DUP(&element, arg2);
			zend_symtable_update(Z_ARRVAL_P(arr), Z_STR_P(arg1), &element);
			break;
		case ZEND_INI_PARSER_POP_ENTRY:
			{
				zval hash, *find_hash;
				if (!arg2) {
					break;
				}
				if (!(Z_STRLEN_P(arg1) > 1 && Z_STRVAL_P(arg1)[0] == '0') && is_numeric_string(Z_STRVAL_P(arg1), Z_STRLEN_P(arg1), NULL, NULL, 0) == IS_LONG) {
						zend_ulong key = (zend_ulong) zend_atol(Z_STRVAL_P(arg1), (int)Z_STRLEN_P(arg1));
					if ((find_hash = zend_hash_index_find(Z_ARRVAL_P(arr), key)) == NULL) {
						array_init(&hash);
						find_hash = zend_hash_index_update(Z_ARRVAL_P(arr), key, &hash);
					}
				} else {
					if ((find_hash = zend_hash_find(Z_ARRVAL_P(arr), Z_STR_P(arg1))) == NULL) {
						array_init(&hash);
						find_hash = zend_hash_update(Z_ARRVAL_P(arr), Z_STR_P(arg1), &hash);
					}
				}
				if (Z_TYPE_P(find_hash) != IS_ARRAY) {
					zval_dtor(find_hash);
					array_init(find_hash);
				}
				ZVAL_DUP(&element, arg2);
				if (!arg3 || (Z_TYPE_P(arg3) == IS_STRING && Z_STRLEN_P(arg3) == 0)) {
					add_next_index_zval(find_hash, &element);
				} else {
					array_set_zval_key(Z_ARRVAL_P(find_hash), arg3, &element);
					zval_ptr_dtor(&element);
				}
			}
			break;
		case ZEND_INI_PARSER_SECTION:
			break;
	}
}
/* }}} */

/* {{{ php_ini_parser_cb_with_sections */
static void php_ini_parser_cb_with_sections(zval *arg1, zval *arg2, zval *arg3, int callback_type, zval *arr)
{
	if (callback_type == ZEND_INI_PARSER_SECTION) {
		array_init(&BG(active_ini_file_section));
		zend_symtable_update(Z_ARRVAL_P(arr), Z_STR_P(arg1), &BG(active_ini_file_section));
	} else if (arg2) {
		zval *active_arr;
		if (Z_TYPE(BG(active_ini_file_section)) != IS_UNDEF) {
			active_arr = &BG(active_ini_file_section);
		} else {
			active_arr = arr;
		}
		php_simple_ini_parser_cb(arg1, arg2, arg3, callback_type, active_arr);
	}
}
/* }}} */

/** {{{ proto azaleaDeepCopy(zval *dst, zval *src) */
static void azaleaDeepCopy(zval *dst, zval *src) {
	zval *pzval, *dstpzval, value;
	zend_ulong idx;
	zend_string *key;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(src), idx, key, pzval) {
		if (key) {
			if (Z_TYPE_P(pzval) == IS_ARRAY) {
				array_init(&value);
				if ((dstpzval = zend_hash_find(Z_ARRVAL_P(dst), key)) && Z_TYPE_P(dstpzval) == IS_ARRAY) {
					azaleaDeepCopy(&value, dstpzval);
				}
				azaleaDeepCopy(&value, pzval);
			} else {
				ZVAL_DUP(&value, pzval);
			}
			zend_hash_update(Z_ARRVAL_P(dst), key, &value);
		} else {
			if (Z_TYPE_P(pzval) == IS_ARRAY) {
				array_init(&value);
				if ((dstpzval = zend_hash_index_find(Z_ARRVAL_P(dst), idx)) && Z_TYPE_P(dstpzval) == IS_ARRAY) {
					azaleaDeepCopy(&value, dstpzval);
				}
				azaleaDeepCopy(&value, pzval);
			} else {
				ZVAL_DUP(&value, pzval);
			}
			zend_hash_index_update(Z_ARRVAL_P(dst), idx, &value);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */


/* {{{ proto void loadConfig(mixed $config) */
void azaleaLoadConfig(zval *val)
{
	zval *config = &AZALEA_G(config);
	if (val) {
		if (Z_TYPE_P(val) == IS_STRING) {
			// load config from file
			zend_stat_t sb;
			zend_file_handle fh;
			const char *iniFile = Z_STRVAL_P(val);
			if (VCWD_STAT(iniFile, &sb) != 0) {
				php_error_docref(NULL, E_ERROR, "Unable to find config file `%s`", iniFile);
				return;
			}
			if (!(fh.handle.fp = VCWD_FOPEN(iniFile, "r"))) {
				php_error_docref(NULL, E_ERROR, "`%s` is not an valid ini file", iniFile);
				return;
			}
			fh.filename = iniFile;
			fh.type = ZEND_HANDLE_FP;
			fh.free_filename = 0;
			fh.opened_path = NULL;
			ZVAL_UNDEF(&BG(active_ini_file_section));
			if (zend_parse_ini_file(&fh, 0, 0, (zend_ini_parser_cb_t) php_ini_parser_cb_with_sections, config) == FAILURE ||
					Z_TYPE_P(config) != IS_ARRAY) {
				php_error_docref(NULL, E_ERROR, "Parsing ini file `%s` faild", iniFile);
				return;
			}
		} else if (Z_TYPE_P(val) == IS_ARRAY) {
			// copy
			azaleaDeepCopy(config, val);
		}
	}
	// DEFAULTS
	zval elSession, elPath, elService, elDispatch, elSessionEnv, *found, *field;
	// config.debug
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("debug")))) {
		add_assoc_bool_ex(config, ZEND_STRL("debug"), 0);
	} else {
		convert_to_boolean(found);
	}
	// config.timezone
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("timezone")))) {
		add_assoc_stringl_ex(config, ZEND_STRL("timezone"), ZEND_STRL("Asia/Shanghai"));
	} else if (Z_TYPE_P(found) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "Timezone must be a string");
		return;
	}
	// config.theme
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("theme")))) {
		add_assoc_null_ex(config, ZEND_STRL("theme"));
	} else if (Z_TYPE_P(found) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "Theme name must be a string");
		return;
	}
	// config.session
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("session"))) {
		array_init(&elSession);
		add_assoc_zval_ex(config, ZEND_STRL("session"), &elSession);
	}
	// config.path
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("path"))) {
		array_init(&elPath);
		add_assoc_zval_ex(config, ZEND_STRL("path"), &elPath);
	}
	// config.service
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("service"))) {
		array_init(&elService);
		add_assoc_zval_ex(config, ZEND_STRL("service"), &elService);
	}
	// config.dispatch
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("dispatch"))) {
		array_init(&elDispatch);
		add_assoc_zval_ex(config, ZEND_STRL("dispatch"), &elDispatch);
	}
	// ---------- sub of config.session ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("session"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.session.name
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("name"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field) ||
			(Z_STRVAL_P(field)[0] >= '0' && Z_STRVAL_P(field)[0] <= '9')) {
		add_assoc_stringl_ex(found, ZEND_STRL("name"), ZEND_STRL("sid"));
	}
	// config.session.lifetime
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("lifetime"));
	if (!field || (Z_TYPE_P(field) != IS_LONG && Z_TYPE_P(field) != IS_STRING) ||
			!is_numeric_string(Z_STRVAL_P(field), Z_STRLEN_P(field), NULL, NULL, 0)) {
		add_assoc_long_ex(found, ZEND_STRL("lifetime"), 0);
	}
	// config.session.path
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("path"))) {
		add_assoc_null_ex(found, ZEND_STRL("path"));
	}
	// config.session.domain
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("domain"))) {
		add_assoc_null_ex(found, ZEND_STRL("domain"));
	}
	// config.session.env
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("env"));
	array_init(&elSessionEnv);
	if (!field || Z_TYPE_P(field) != IS_STRING || Z_STRLEN_P(field) == 0) {
		add_next_index_stringl(&elSessionEnv, ZEND_STRL("WEB"));
	} else {
		zend_string *delim = zend_string_init(ZEND_STRL(","), 0);
		php_explode(delim, Z_STR_P(field), &elSessionEnv, ZEND_LONG_MAX);
		zend_string_release(delim);
	}
	add_assoc_zval_ex(found, ZEND_STRL("env"), &elSessionEnv);
	// ---------- sub of config.path ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("path"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.path.basepath
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("basepath"))) {
		add_assoc_null_ex(found, ZEND_STRL("basepath"));
	}
	// config.path.controllers
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("controllers"))) {
		add_assoc_stringl_ex(found, ZEND_STRL("controllers"), ZEND_STRL("controllers"));
	}
	// config.path.models
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("models"))) {
		add_assoc_stringl_ex(found, ZEND_STRL("models"), ZEND_STRL("models"));
	}
	// config.path.views
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("views"))) {
		add_assoc_stringl_ex(found, ZEND_STRL("views"), ZEND_STRL("views"));
	}
	// config.path.static
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("static"))) {
		add_assoc_null_ex(found, ZEND_STRL("static"));
	}
	// config.path.static_host
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("static_host"))) {
		add_assoc_null_ex(found, ZEND_STRL("static_host"));
	}
	// ---------- sub of config.service ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("service"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.service.timeout
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("timeout"))) {
		add_assoc_long_ex(found, ZEND_STRL("timeout"), 15);
	}
	// config.service.connecttimeout
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("connecttimeout"))) {
		add_assoc_long_ex(found, ZEND_STRL("connecttimeout"), 2);
	}
	// config.service.retry
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("retry"));
	if (!field || (Z_TYPE_P(field) != IS_LONG && Z_TYPE_P(field) != IS_STRING) ||
			!is_numeric_string(Z_STRVAL_P(field), Z_STRLEN_P(field), NULL, NULL, 0)) {
		add_assoc_long_ex(found, ZEND_STRL("retry"), 0);
	}
	// ---------- sub of config.dispatch ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("dispatch"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.dispatch.default_controller
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("default_controller"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_stringl_ex(found, ZEND_STRL("default_controller"), ZEND_STRL("default"));
	}
	// config.dispatch.default_action
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("default_action"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_stringl_ex(found, ZEND_STRL("default_action"), ZEND_STRL("index"));
	}
	// config.dispatch.error_controller
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("error_controller"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_stringl_ex(found, ZEND_STRL("error_controller"), ZEND_STRL("error"));
	}
	// config.dispatch.error_action
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("error_action"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_stringl_ex(found, ZEND_STRL("error_action"), ZEND_STRL("error"));
	}
}
/* }}} */

/* {{{ proto mixed azaleaConfigSubFind */
zval * azaleaConfigSubFindEx(const char *key, size_t lenKey, const char *subKey, size_t lenSubKey)
{
	zval *found = zend_hash_str_find(Z_ARRVAL(AZALEA_G(config)), key, lenKey);
	if (!found) {
		return NULL;
	}
	if (!subKey) {
		return found;
	}
	if (Z_TYPE_P(found) != IS_ARRAY) {
		return NULL;
	}
	found = zend_hash_str_find(Z_ARRVAL_P(found), subKey, lenSubKey);
	return found ? found : NULL;
}
/* }}} */

/* {{{ proto mixed get */
PHP_METHOD(azalea_config, get)
{
	zend_string *key, *subKey;
	zval *def = NULL;
	zval *val;
	char *p;
	size_t lenKey;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|z", &key, &def) == FAILURE) {
		return;
	}

	p = strchr(ZSTR_VAL(key), '.');
	if (p) {
		lenKey = p - ZSTR_VAL(key);
		val = azaleaConfigSubFindEx(ZSTR_VAL(key),lenKey, p + 1, ZSTR_LEN(key) - lenKey - 1);
	} else {
		val = azaleaConfigSubFindEx(ZSTR_VAL(key), ZSTR_LEN(key), NULL, 0);
	}
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto mixed getAll(void) */
PHP_METHOD(azalea_config, getAll)
{
	RETURN_ZVAL(&AZALEA_G(config), 1, 0);
}
/* }}} */
