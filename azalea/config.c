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
#include "ext/standard/php_array.h"

zend_class_entry *azalea_config_ce;

/* {{{ class Azalea\Config methods
 */
static zend_function_entry azalea_config_methods[] = {
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

/* {{{ proto array loadConfig(mixed $config) */
HashTable * loadConfig(const zval *val)
{
	HashTable *ht = AZALEA_G(config);
	if (val) {
		if (Z_TYPE_P(val) == IS_STRING) {
			// load config from file
		} else if (Z_TYPE_P(val) == IS_ARRAY) {
			zend_hash_copy(ht, Z_ARRVAL_P(val), NULL);
		}
	}
	// DEFAULTS
	zval el, subEl, *found;
	// config.debug
	ZVAL_BOOL(&el, 0);
	zend_hash_str_add(ht, "debug", sizeof("debug") - 1, &el);
	// config.timezone
	ZVAL_STRING(&el, "RPC");
	zend_hash_str_add(ht, "timezone", sizeof("timezone") - 1, &el);
	zval_ptr_dtor(&el);
	// config.theme
	ZVAL_NULL(&el);
	zend_hash_str_add(ht, "theme", sizeof("theme") - 1, &el);
	// config.session
	array_init(&el);
	zend_hash_str_add(ht, "session", sizeof("session") - 1, &el);
	zval_ptr_dtor(&el);
	// sub of config.session
	found = zend_hash_str_find(ht, "session", sizeof("session") - 1);
	if (Z_TYPE_P(found) == IS_ARRAY) {
		// config.session.path
		ZVAL_NULL(&subEl);
		zend_hash_str_add(Z_ARRVAL_P(found), "path", sizeof("path") - 1, &subEl);
		// config.session.domain
		ZVAL_NULL(&subEl);
		zend_hash_str_add(Z_ARRVAL_P(found), "domain", sizeof("domain") - 1, &subEl);
		// config.session.name
		ZVAL_STRING(&subEl, "sid");
		zend_hash_str_add(Z_ARRVAL_P(found), "name", sizeof("name") - 1, &subEl);
		zval_ptr_dtor(&subEl);
		// config.session.lifetime
		ZVAL_LONG(&subEl, 0);
		zend_hash_str_add(Z_ARRVAL_P(found), "lifetime", sizeof("lifetime") - 1, &subEl);
	}
	php_var_dump(found, 0);
	// config.



//	zval t;
//	ZVAL_ARR(&t, ht);
//	php_var_dump(&t, 0);

	return ht;
}
/* }}} */

/* {{{ proto mixed get(string $key, mixed $default = null) */
PHP_METHOD(azalea_config, get)
{

}
/* }}} */

/* {{{ proto mixed set(string $key, mixed $value) */
PHP_METHOD(azalea_config, set)
{

}
/* }}} */
