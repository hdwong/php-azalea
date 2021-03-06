/*
 * azalea/session.c
 *
 * Created by Bun Wong on 16-7-10.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/azalea.h"
#include "azalea/namespace.h"
#include "azalea/session.h"

#include "ext/session/php_session.h"	// for php_*_session_var

zend_class_entry *azaleaSessionCe;

/* {{{ class Azalea\Session methods */
static zend_function_entry azalea_session_methods[] = {
	PHP_ME(azalea_session, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_session, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_session, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_session, clean, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(session)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Session), azalea_session_methods);
	azaleaSessionCe = zend_register_internal_class(&ce);
	azaleaSessionCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_session, __construct) {}
/* }}} */

/* {{{ proto mixed get(string name, mixed default) */
PHP_METHOD(azalea_session, get)
{
	zend_string *name;
	zval *def = NULL;
	zval *val;

	if (!AZALEA_G(startSession)) {
		RETURN_NULL();
	}

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|z", &name, &def) == FAILURE) {
		return;
	}
	val = php_get_session_var(name);
	if (val) {
		RETURN_ZVAL(val, 1, 0);
	}
	if (def) {
		RETURN_ZVAL(def, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto void set(string name, mixed value) */
PHP_METHOD(azalea_session, set)
{
	zend_string *name;
	zval *val;

	if (!AZALEA_G(startSession)) {
		return;
	}

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sz", &name, &val) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(val) == IS_NULL) {
		// unset
		// IF_SESSION_VARS()
		if (Z_ISREF_P(&PS(http_session_vars)) && Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) == IS_ARRAY) {
			zend_hash_del(Z_ARRVAL_P(Z_REFVAL(PS(http_session_vars))), name);
		}
	} else {
		php_set_session_var(name, val, NULL);
		zval_add_ref(val);
	}
}
/* }}} */

/* {{{ proto void clean(void) */
PHP_METHOD(azalea_session, clean)
{
	if (!AZALEA_G(startSession)) {
		return;
	}

	// IF_SESSION_VARS()
	if (Z_ISREF_P(&PS(http_session_vars)) && Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) == IS_ARRAY) {
		zend_hash_clean(Z_ARRVAL_P(Z_REFVAL(PS(http_session_vars))));
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */
