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

#include "Zend/zend_smart_str.h"  // for smart_str_*
#include "ext/standard/php_var.h"  // for php_var_serialize
#include "ext/json/php_json.h"  // for php_json_*
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_redis_ce;

/* {{{ class RedisModel methods
 */
static zend_function_entry azalea_node_beauty_redis_methods[] = {
	PHP_ME(azalea_node_beauty_redis, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
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

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_redis, __construct) {}
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
	} else {
		key = zend_string_init(ZSTR_VAL(key), ZSTR_LEN(key), 0);
	}

	ZVAL_STRINGL(&arg1, "keys", sizeof("keys") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("key"), key);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *key

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
	key = zend_string_init(ZSTR_VAL(key), ZSTR_LEN(key), 0);

	ZVAL_STRINGL(&arg1, "value", sizeof("value") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("key"), key);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *key

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("value"), 1, NULL)) &&
			Z_TYPE_P(value) == IS_STRING) {
		if (format && !strncasecmp(ZSTR_VAL(format), "raw", sizeof("raw") - 1)) {
			// raw
			RETVAL_ZVAL(value, 1, 0);
		}  else if (format && !strncasecmp(ZSTR_VAL(format), "php", sizeof("php") - 1)) {
			// php
			php_unserialize_data_t var_hash;
			const unsigned char *p = (const unsigned char*) Z_STRVAL_P(value);
			PHP_VAR_UNSERIALIZE_INIT(var_hash);
			if (!php_var_unserialize(return_value, &p, p + Z_STRLEN_P(value), &var_hash)) {
				PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
				zval_ptr_dtor(return_value);
				RETVAL_FALSE;
			} else {
				var_push_dtor(&var_hash, return_value);
				PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			}
		} else {
			// default format is json
			php_json_decode(return_value, Z_STRVAL_P(value), Z_STRLEN_P(value), 0, 0);
		}
	} else if (def) {
		RETVAL_ZVAL(def, 1, 0);
	} else {
		RETVAL_NULL();
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto set */
PHP_METHOD(azalea_node_beauty_redis, set)
{
	/* $format = enum['json'(default), 'php', 'raw'] */
	zend_string *key, *strValue, *format = NULL;
	zval *val;
	zend_long lifetime = 0;
	zval ret, arg1, arg2, *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sz|lS", &key, &val, &lifetime, &format) == FAILURE) {
		return;
	}
	key = zend_string_init(ZSTR_VAL(key), ZSTR_LEN(key), 0);
	if (lifetime < 0) {
		lifetime = 0;
	}

	if (format && !strncasecmp(ZSTR_VAL(format), "raw", sizeof("raw") - 1)) {
		// raw
		if (Z_TYPE_P(val) > IS_STRING) {
			php_error_docref(NULL, E_ERROR, "Value must be a string in raw format");
			return;
		}
		convert_to_string(val);
		strValue = zend_string_init(Z_STRVAL_P(val), Z_STRLEN_P(val), 0);
	} else if (format && !strncasecmp(ZSTR_VAL(format), "php", sizeof("php") - 1)) {
		// php
		php_serialize_data_t var_hash;
		smart_str buf = {0};
		PHP_VAR_SERIALIZE_INIT(var_hash);
		php_var_serialize(&buf, val, &var_hash);
		PHP_VAR_SERIALIZE_DESTROY(var_hash);
		strValue = zend_string_dup(buf.s, 0);
		smart_str_free(&buf);
	} else {
		// default format is json
		smart_str buf = {0};
		zend_long options = 0, depth = PHP_JSON_PARSER_DEFAULT_DEPTH;
		JSON_G(error_code) = PHP_JSON_ERROR_NONE;
		JSON_G(encode_max_depth) = (int)depth;
		php_json_encode(&buf, val, (int)options);
		if (JSON_G(error_code) != PHP_JSON_ERROR_NONE && !(options & PHP_JSON_PARTIAL_OUTPUT_ON_ERROR)) {
			smart_str_free(&buf);
			RETURN_FALSE;
		}
		smart_str_0(&buf);
		strValue = zend_string_dup(buf.s, 0);
		smart_str_free(&buf);
	}
	ZVAL_STRINGL(&arg1, "value", sizeof("value") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("key"), key);
	add_assoc_str_ex(&arg2, ZEND_STRL("value"), strValue);
	add_assoc_long_ex(&arg2, ZEND_STRL("ex"), lifetime);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "put", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *key and *strValue

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("affected"), 1, NULL)) &&
			Z_TYPE_P(value) == IS_LONG) {
		RETVAL_BOOL(Z_LVAL_P(value));
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_node_beauty_redis, delete)
{
	zval *keys;
	int argc, offset = 0;
	zval ret, arg1, arg2, argKeys, *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "+", &keys, &argc) == FAILURE) {
		return;
	}
	array_init(&argKeys);
	do {
		if (Z_TYPE_P(keys + offset) != IS_STRING) {
			continue;
		}
		add_next_index_str(&argKeys, zend_string_init(Z_STRVAL_P(keys + offset), Z_STRLEN_P(keys + offset), 0));
	} while (++offset < argc);

	ZVAL_STRINGL(&arg1, "value", sizeof("value") - 1);
	array_init(&arg2);
	add_assoc_zval_ex(&arg2, ZEND_STRL("key"), &argKeys);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "delete", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release argKeys

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("affected"), 1, NULL)) &&
			Z_TYPE_P(value) == IS_LONG) {
		RETVAL_ZVAL(value, 1, 0);
	} else {
		RETVAL_LONG(0);
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto incr */
PHP_METHOD(azalea_node_beauty_redis, incr)
{
	zend_string *key = NULL;
	zend_long increment = 1;
	zval ret, arg1, arg2, *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|l", &key, &increment) == FAILURE) {
		return;
	}
	key = zend_string_init(ZSTR_VAL(key), ZSTR_LEN(key), 0);

	ZVAL_STRINGL(&arg1, "incr", sizeof("incr") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("key"), key);
	add_assoc_long_ex(&arg2, ZEND_STRL("increment"), increment);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "put", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *key

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("value"), 1, NULL)) &&
			Z_TYPE_P(value) == IS_LONG) {
		RETVAL_ZVAL(value, 1, 0);
	} else {
		RETVAL_LONG(0);
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto clean */
PHP_METHOD(azalea_node_beauty_redis, clean)
{
	zval arg1, arg2;

	ZVAL_STRINGL(&arg1, "command", sizeof("command") - 1);
	array_init(&arg2);
	add_assoc_stringl_ex(&arg2, ZEND_STRL("command"), ZEND_STRL("flushall"));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "post", NULL, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release argArguments
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto command */
PHP_METHOD(azalea_node_beauty_redis, command)
{
	zend_string *command;
	zval *args;
	int argc, offset = 0;
	zval ret, arg1, arg2, argArguments, *value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|+", &command, &args, &argc) == FAILURE) {
		return;
	}
	array_init(&argArguments);
	while (offset < argc) {
		if (Z_TYPE_P(args + offset) > IS_STRING) {
			php_error_docref(NULL, E_ERROR, "Command argument must be a stirng");
			zval_ptr_dtor(&argArguments);
			RETURN_FALSE;
		}
		convert_to_string(args + offset);
		add_next_index_str(&argArguments, zend_string_init(Z_STRVAL_P(args + offset), Z_STRLEN_P(args + offset), 0));
		++offset;
	}
	command = zend_string_init(ZSTR_VAL(command), ZSTR_LEN(command), 0);
	zend_str_tolower(ZSTR_VAL(command), ZSTR_LEN(command));

	ZVAL_STRINGL(&arg1, "command", sizeof("command") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("command"), command);
	add_assoc_zval_ex(&arg2, ZEND_STRL("arguments"), &argArguments);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "post", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *command and argArguments

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("value"), 1, NULL))) {
		RETVAL_ZVAL(value, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */
