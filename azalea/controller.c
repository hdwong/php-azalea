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
#include "azalea/model.h"
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
	PHP_ME(azalea_controller, loadModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
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
	zend_update_property(azalea_request_ce, return_value, ZEND_STRL("_instance"), getThis());
}
/* }}} */

/* {{{ proto getResponse */
PHP_METHOD(azalea_controller, getResponse)
{
	object_init_ex(return_value, azalea_response_ce);
	zend_update_property(azalea_response_ce, return_value, ZEND_STRL("_instance"), getThis());
}
/* }}} */

/* {{{ proto getSession */
PHP_METHOD(azalea_controller, getSession)
{
	object_init_ex(return_value, azalea_session_ce);
}
/* }}} */

/* {{{ proto loadModel */
PHP_METHOD(azalea_controller, loadModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_controller, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */

/* {{{ proto getView */
PHP_METHOD(azalea_controller, getView)
{
	azalea_view_t *instance;
	if ((instance = zend_hash_str_find(Z_ARRVAL(AZALEA_G(instances)), ZEND_STRL("_view")))) {
		RETURN_ZVAL(instance, 1, 0);
	}
	azalea_view_t rv = {{0}};
	zval data, *staticPath, *themeName;
	zend_string *tpldir, *tstr;
	// new instance
	instance = &rv;
	object_init_ex(instance, azalea_view_ce);
	add_assoc_zval_ex(&AZALEA_G(instances), ZEND_STRL("_view"), instance);
	// data
	array_init(&data);
	zend_update_property(azalea_view_ce, instance, ZEND_STRL("_data"), &data);
	zval_ptr_dtor(&data);
	// environ
	array_init(&data);
	// environ.tpldir
	staticPath = azaleaConfigSubFind("path", "static");
	if (staticPath && Z_TYPE_P(staticPath) != IS_NULL && Z_STRLEN_P(staticPath)) {
		tpldir = strpprintf(0, "%s%c%s", ZSTR_VAL(tpldir), DEFAULT_SLASH, Z_STRVAL_P(staticPath));
	} else {
		tpldir = zend_string_init(ZEND_STRL(""), 0);
	}
	themeName = azaleaConfigFind("theme");
	if (themeName && Z_TYPE_P(themeName) != IS_NULL && Z_STRLEN_P(themeName)) {
		tstr = tpldir;
		tpldir = strpprintf(0, "%s%c%s", ZSTR_VAL(tpldir), DEFAULT_SLASH, Z_STRVAL_P(themeName));
		zend_string_release(tstr);
	}
	add_assoc_str_ex(&data, ZEND_STRL("tpldir"), tpldir);
	// upate environ
	zend_update_property(azalea_view_ce, instance, ZEND_STRL("_environ"), &data);
	zval_ptr_dtor(&data);
	RETURN_ZVAL(instance, 1, 0);
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
