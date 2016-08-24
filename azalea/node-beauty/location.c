/*
 * location.c
 *
 * Created by Bun Wong on 16-8-23.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/location.h"

#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_location_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_node_beauty_location_methods[] = {
	PHP_ME(azalea_node_beauty_location, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_location, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_location, children, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_location, ip, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(location)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(LocationModel), azalea_node_beauty_location_methods);
	azalea_node_beauty_location_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_location_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_location, __construct) {}
/* }}} */

/* {{{ proto get */
PHP_METHOD(azalea_node_beauty_location, get)
{
	zval *codes;
	zend_string *code = NULL, *tstr;
	int argc, offset = 0;
	zval ret, arg1, arg2;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "+", &codes, &argc) == FAILURE) {
		return;
	}
	do {
		if (Z_TYPE_P(codes + offset) != IS_STRING && Z_TYPE_P(codes + offset) != IS_LONG) {
			continue;
		}
		convert_to_string(codes + offset);
		if (!code) {
			code = zend_string_init(Z_STRVAL_P(codes + offset), Z_STRLEN_P(codes + offset), 0);
		} else {
			tstr = code;
			code = strpprintf(0, "%s,%s", ZSTR_VAL(code), Z_STRVAL_P(codes + offset));
			zend_string_release(tstr);
		}
	} while (++offset < argc);

	ZVAL_EMPTY_STRING(&arg1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("id"), code);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *code

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto children */
PHP_METHOD(azalea_node_beauty_location, children)
{
	zend_string *code = NULL;
	zval ret, arg1, arg2;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &code) == FAILURE) {
		return;
	}

	ZVAL_STRINGL(&arg1, "children", sizeof("children") - 1);
	if (code) {
		code = zend_string_init(ZSTR_VAL(code), ZSTR_LEN(code), 0);
		array_init(&arg2);
		add_assoc_str_ex(&arg2, ZEND_STRL("id"), code);
		zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
		zval_ptr_dtor(&arg2);  // no need to release *code
	} else {
		zend_call_method_with_1_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1);
	}
	zval_ptr_dtor(&arg1);

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto ip */
PHP_METHOD(azalea_node_beauty_location, ip)
{
	zend_string *ip = NULL;
	zval ret, arg1, arg2;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &ip) == FAILURE) {
		return;
	}
	if (ip) {
		ip = zend_string_init(ZSTR_VAL(ip), ZSTR_LEN(ip), 0);
	} else {
		ip = zend_string_copy(azaleaRequestIp());
	}

	ZVAL_STRINGL(&arg1, "ip", sizeof("ip") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("ip"), ip);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);  // no need to release *ip

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */
