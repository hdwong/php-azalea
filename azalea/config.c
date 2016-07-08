/*
 * azalea/config.c
 *
 * Created by Bun Wong on 16-6-29.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/config.h"

#include "ext/standard/php_var.h"

zend_class_entry *azalea_config_ce;

/* {{{ class Azalea\Config methods
 */
static zend_function_entry azalea_config_methods[] = {
    PHP_ME(azalea_config, all, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_config, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_config, set, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(config)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Config), azalea_config_methods);
	azalea_config_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_config_ce->ce_flags |= ZEND_ACC_FINAL;

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

/* {{{ proto array loadConfig(mixed $config) */
zval * azaleaLoadConfig(zval *val)
{
	zval *config = &AZALEA_G(config);
	if (val) {
		if (Z_TYPE_P(val) == IS_STRING) {
			// load config from file
			zend_stat_t sb;
			zend_file_handle fh;
			char *ini_file = Z_STRVAL_P(val);
			if (VCWD_STAT(ini_file, &sb) != 0) {
				php_error_docref(NULL, E_ERROR, "Unable to find config file `%s`", ini_file);
				return NULL;
			}
			if (!(fh.handle.fp = VCWD_FOPEN(ini_file, "r"))) {
				php_error_docref(NULL, E_ERROR, "`%s` is not an valid ini file", ini_file);
				return NULL;
			}
			fh.filename = ini_file;
			fh.type = ZEND_HANDLE_FP;
			fh.free_filename = 0;
			fh.opened_path = NULL;
			ZVAL_UNDEF(&BG(active_ini_file_section));
			if (zend_parse_ini_file(&fh, 0, 0, (zend_ini_parser_cb_t) php_ini_parser_cb_with_sections, config) == FAILURE ||
					Z_TYPE_P(config) != IS_ARRAY) {
				php_error_docref(NULL, E_ERROR, "Parsing ini file `%s` faild", ini_file);
				return NULL;
			}
		} else if (Z_TYPE_P(val) == IS_ARRAY) {
			// copy
			zval_ptr_dtor(config);
			ZVAL_COPY(config, val);
		}
	}
	// DEFAULTS
	zval el, *found, *field;
	// config.debug
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("debug")))) {
		add_assoc_bool(config, "debug", 0);
	} else {
		convert_to_boolean(found);
	}
	// config.timezone
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("timezone")))) {
		add_assoc_string(config, "timezone", "Asia/Shanghai");
	} else {
		convert_to_string_ex(found);
	}
	// config.theme
	if (!(found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("theme")))) {
		add_assoc_null(config, "theme");
	} else {
		convert_to_string_ex(found);
	}
	// config.session
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("session"))) {
		array_init(&el);
		add_assoc_zval(config, "session", &el);
	}
	// config.path
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("path"))) {
		array_init(&el);
		add_assoc_zval(config, "path", &el);
	}
	// config.service
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("service"))) {
		array_init(&el);
		add_assoc_zval(config, "service", &el);
	}
	// config.dispatch
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("dispatch"))) {
		array_init(&el);
		add_assoc_zval(config, "dispatch", &el);
	}
	// config.router
	if (!zend_hash_str_exists(Z_ARRVAL_P(config), ZEND_STRL("router"))) {
		array_init(&el);
		add_assoc_zval(config, "router", &el);
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
		add_assoc_string(found, "name", "sid");
	}
	// config.session.lifetime
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("lifetime"))) {
		add_assoc_long(found, "lifetime", 0);
	}
	// config.session.path
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("path"))) {
		add_assoc_null(found, "path");
	}
	// config.session.domain
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("domain"))) {
		add_assoc_null(found, "domain");
	}
	// ---------- sub of config.path ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("path"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.path.basepath
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("basepath"))) {
		add_assoc_null(found, "basepath");
	}
	// config.path.controllers
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("controllers"))) {
		add_assoc_string(found, "controllers", "controllers");
	}
	// config.path.models
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("models"))) {
		add_assoc_string(found, "models", "models");
	}
	// config.path.views
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("views"))) {
		add_assoc_string(found, "views", "views");
	}
	// config.path.static
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("static"))) {
		add_assoc_string(found, "static", "static");
	}
	// ---------- sub of config.service ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("service"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}
	// config.service.timeout
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("timeout"))) {
		add_assoc_long(found, "timeout", 15);
	}
	// config.service.connecttimeout
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("connecttimeout"))) {
		add_assoc_long(found, "connecttimeout", 2);
	}
	// config.service.retry
	if (!zend_hash_str_exists(Z_ARRVAL_P(found), ZEND_STRL("retry"))) {
		add_assoc_long(found, "retry", 0);
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
		add_assoc_string(found, "default_controller", "default");
	}
	// config.dispatch.default_action
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("default_action"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_string(found, "default_action", "index");
	}
	// config.dispatch.error_controller
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("error_controller"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_string(found, "error_controller", "error");
	}
	// config.dispatch.error_action
	field = zend_hash_str_find(Z_ARRVAL_P(found), ZEND_STRL("error_action"));
	if (!field || Z_TYPE_P(field) != IS_STRING || !Z_STRLEN_P(field)) {
		add_assoc_string(found, "error_action", "error");
	}
	// ---------- sub of config.router ----------
	found = zend_hash_str_find(Z_ARRVAL_P(config), ZEND_STRL("router"));
	if (Z_TYPE_P(found) != IS_ARRAY) {
		zval_ptr_dtor(found);
		array_init(found);
	}

	return config;
}
/* }}} */

/* {{{ proto mixed azaleaGetSubConfig(string $key) */
zval * azaleaGetSubConfig(const char *key, const char *subKey)
{
	zval *found = zend_hash_str_find(Z_ARRVAL(AZALEA_G(config)), key, strlen(key));
	if (!found) {
		return NULL;
	}
	if (!subKey) {
		return found;
	}
	if (Z_TYPE_P(found) != IS_ARRAY) {
		return NULL;
	}
	found = zend_hash_str_find(Z_ARRVAL_P(found), subKey, strlen(subKey));
	return found ? found : NULL;
}
/* }}} */

/* {{{ proto mixed get(string $key = null, mixed $default = null) */
PHP_METHOD(azalea_config, all)
{
	RETURN_ZVAL(&AZALEA_G(config), 1, 0);
}
/* }}} */

/* {{{ proto mixed get(string $key = null, mixed $default = null) */
PHP_METHOD(azalea_config, get)
{
	zend_string *key = NULL;
	zval *def = NULL;
	zval *found, *val;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|z", &key, &def) == FAILURE) {
		return;
	}

	val = azaleaGetConfig(ZSTR_VAL(key));
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto mixed set(string $key, mixed $value) */
PHP_METHOD(azalea_config, set)
{
	zend_string *key = NULL;
	zval *val = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sz", &key, &val) == FAILURE) {
		return;
	}

	add_assoc_zval_ex(&AZALEA_G(config), ZSTR_VAL(key), ZSTR_LEN(key), val);
}
/* }}} */
