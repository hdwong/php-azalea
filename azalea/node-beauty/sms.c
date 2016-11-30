/*
 * sms.c
 *
 * Created by Bun Wong on 16-9-4.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/sms.h"

#include "ext/standard/php_var.h"
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_sms_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_node_beauty_sms_methods[] = {
	PHP_ME(azalea_node_beauty_sms, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_sms, send, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(sms)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(NBSmsModel), azalea_node_beauty_sms_methods);
	azalea_node_beauty_sms_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_sms_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_sms, __construct) {}
/* }}} */

/* {{{ proto send */
PHP_METHOD(azalea_node_beauty_sms, send)
{
	zend_string *to, *body;
	zval ret, arg1, arg2, *pError;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &to, &body) == FAILURE) {
		return;
	}

	ZVAL_STRINGL(&arg1, "send", sizeof("send") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("mobile"), zend_string_copy(to));
	add_assoc_str_ex(&arg2, ZEND_STRL("message"), zend_string_copy(body));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "post", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);

	if (Z_TYPE(ret) == IS_OBJECT && (pError = zend_read_property(NULL, &ret, ZEND_STRL("error"), 1, NULL)) &&
			Z_TYPE_P(pError) == IS_LONG && Z_LVAL_P(pError) == 0) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */
