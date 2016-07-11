/*
 * azalea/controller.c
 *
 * Created by Bun Wong on 16-7-8.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/controller.h"
#include "azalea/config.h"
#include "azalea/request.h"
#include "azalea/response.h"
#include "azalea/session.h"
#include "azalea/view.h"
#include "azalea/exception.h"

#include "ext/standard/php_var.h"  // for php_var_dump

zend_class_entry *azalea_controller_ce;

/* {{{ class Azalea\Controller methods
 */
static zend_function_entry azalea_controller_methods[] = {
	PHP_ME(azalea_controller, getRequest, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getResponse, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getSession, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getView, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, throw404, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(controller)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Controller), azalea_controller_methods);
	azalea_controller_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_controller_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	zend_declare_property_null(azalea_controller_ce, ZEND_STRL("_folderName"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_controller_ce, ZEND_STRL("_controllerName"), ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

/* {{{ proto getRequest */
PHP_METHOD(azalea_controller, getRequest)
{
	object_init_ex(return_value, azalea_request_ce);
}
/* }}} */

/* {{{ proto getResponse */
PHP_METHOD(azalea_controller, getResponse)
{
	object_init_ex(return_value, azalea_response_ce);
}
/* }}} */

/* {{{ proto getSession */
PHP_METHOD(azalea_controller, getSession)
{
	object_init_ex(return_value, azalea_session_ce);
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_controller, getModel)
{
//	object_init_ex(return_value, azalea_session_ce);
}
/* }}} */

/* {{{ proto getView */
PHP_METHOD(azalea_controller, getView)
{
	azalea_view_t *instance = return_value;
	object_init_ex(instance, azalea_view_ce);
	zval data, tpldir, *staticPath, *themeName;
	// data
	array_init(&data);
	zend_update_property(azalea_view_ce, instance, ZEND_STRL("_data"), &data);
	zval_ptr_dtor(&data);
	// environ
	array_init(&data);
	// environ.tpldir
	ZVAL_STRINGL(&tpldir, "", 0);
	staticPath = azaleaGetSubConfig("path", "static");
	if (staticPath && Z_TYPE_P(staticPath) != IS_NULL && Z_STRLEN_P(staticPath)) {
		zval t;
		ZVAL_STRING(&t, "/");
		concat_function(&tpldir, &tpldir, staticPath);
		concat_function(&tpldir, &tpldir, &t);
		zval_ptr_dtor(&t);
	}
	themeName = azaleaGetConfig("theme");
	if (themeName && Z_TYPE_P(themeName) != IS_NULL && Z_STRLEN_P(themeName)) {
		zval t;
		ZVAL_STRING(&t, "/");
		concat_function(&tpldir, &tpldir, themeName);
		concat_function(&tpldir, &tpldir, &t);
		zval_ptr_dtor(&t);
	}
	add_assoc_zval_ex(&data, ZEND_STRL("tpldir"), &tpldir);
	// environ.debug
	add_assoc_bool_ex(&data, ZEND_STRL("debug"), Z_LVAL_P(azaleaGetConfig("debug")));
	zend_update_property(azalea_view_ce, instance, ZEND_STRL("_environ"), &data);
	zval_ptr_dtor(&data);
}
/* }}} */

/* {{{ proto throw404 */
PHP_METHOD(azalea_controller, throw404)
{
	zend_string *message = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|S", &message) == FAILURE) {
		return;
	}

	throw404(message);
}
/* }}} */
