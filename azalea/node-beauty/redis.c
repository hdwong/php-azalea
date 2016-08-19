/*
 * redis.c
 *
 * Created by Bun Wong on 16-8-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/redis.h"

#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_redis_ce;

/* {{{ class RedisModel methods
 */
static zend_function_entry azalea_node_beauty_redis_methods[] = {
	PHP_ME(azalea_node_beauty_redis, keys, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, delete, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, incr, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, clean, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, command, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(redis)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(RedisModel), azalea_node_beauty_redis_methods);
	azalea_node_beauty_redis_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_redis_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, keys)
{
	zend_string *key = NULL;
	zval ret, arg1, arg2, *keys;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &key) == FAILURE) {
		return;
	}

	if (!key) {
		key = zend_string_init(ZEND_STRL("*"), 0);
	}
	ZVAL_STRINGL(&arg1, "keys", sizeof("keys") - 1);
	array_init(&arg2);
	add_assoc_str(&arg2, "key", zend_string_copy(key));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);
	zend_string_release(key);

	if (Z_TYPE(ret) == IS_OBJECT && (keys = zend_read_property(NULL, &ret, ZEND_STRL("keys"), 1, NULL))) {
		RETVAL_ZVAL(keys, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto get */
PHP_METHOD(azalea_node_beauty_redis, get)
{
	/* $format = enum['json'(default), 'php', 'raw'] */
	zend_string *key, *format = NULL;
	zval *def = NULL;
	zval ret, arg1, arg2, *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|zS", &key, &def, &format) == FAILURE) {
		return;
	}

	// default format is json
	if (!format || strncasecmp(ZSTR_VAL(format), "json", sizeof("json") - 1) ||
			strncasecmp(ZSTR_VAL(format), "php", sizeof("php") - 1)) {
		format = zend_string_init(ZEND_STRL("json"), 0);
	}
	ZVAL_STRINGL(&arg1, "value", sizeof("value") - 1);
	array_init(&arg2);
	add_assoc_str(&arg2, "key", zend_string_copy(key));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);
	zend_string_release(format);

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("value"), 1, NULL)) &&
			Z_TYPE_P(value) != IS_NULL) {
		RETVAL_ZVAL(value, 1, 0);
	} else if (def) {
		RETVAL_ZVAL(def, 1, 0);
	} else {
		RETVAL_NULL();
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, set)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, delete)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, incr)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, clean)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, command)
{
}
/* }}} */
