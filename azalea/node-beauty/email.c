/*
 * email.c
 *
 * Created by Bun Wong on 16-9-3.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/email.h"

#include "ext/standard/php_string.h"  // for php_implode
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_email_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_node_beauty_email_methods[] = {
	PHP_ME(azalea_node_beauty_email, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_email, send, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(email)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(EmailModel), azalea_node_beauty_email_methods);
	azalea_node_beauty_email_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_email_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_email, __construct) {}
/* }}} */

/* {{{ proto send */
PHP_METHOD(azalea_node_beauty_email, send)
{
	zend_string *subject, *html, *text = NULL, *delim;
	zval *to, tos, ret, arg1, arg2;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zSS|S", &to, &subject, &html, &text) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(to) != IS_ARRAY && Z_TYPE_P(to) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or array");
		return;
	}

	// to list
	if (Z_TYPE_P(to) == IS_ARRAY) {
		delim = zend_string_init(ZEND_STRL(","), 0);
		php_implode(delim, to, &tos);
		zend_string_release(delim);
		to = &tos;
	} else {
		zval_add_ref(to);
	}

	ZVAL_STRINGL(&arg1, "send", sizeof("send") - 1);
	array_init(&arg2);
	add_assoc_zval_ex(&arg2, ZEND_STRL("to"), to);
	add_assoc_str_ex(&arg2, ZEND_STRL("subject"), zend_string_copy(subject));
	add_assoc_str_ex(&arg2, ZEND_STRL("body"), zend_string_copy(html));
	if (text) {
		add_assoc_str_ex(&arg2, ZEND_STRL("text"), zend_string_copy(text));
	}
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "post", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */
