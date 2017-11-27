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

#include "ext/standard/php_var.h"	// for php_var_dump
#include "ext/standard/php_string.h"	// for php_trim
#include "Zend/zend_interfaces.h"	// for zend_call_method_with_*

zend_class_entry *azaleaControllerCe;

/* {{{ class Azalea\Controller methods
 */
static zend_function_entry azalea_controller_methods[] = {
	PHP_ME(azalea_controller, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_controller, getSession, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, loadModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, getModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_controller, notFound, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(controller)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Controller), azalea_controller_methods);
	azaleaControllerCe = zend_register_internal_class(&ce);
	azaleaControllerCe->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	zend_declare_property_null(azaleaControllerCe, ZEND_STRL("_folder"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azaleaControllerCe, ZEND_STRL("_controller"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azaleaControllerCe, ZEND_STRL("req"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(azaleaControllerCe, ZEND_STRL("res"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(azaleaControllerCe, ZEND_STRL("view"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_controller, __construct) {}
/* }}} */

/* {{{ proto azaleaControllerInit */
void azaleaControllerInit(zval *this, zend_class_entry *ce, zend_string *folderName, zend_string *controllerName)
{
	azalea_request_t *pReq;
	azalea_response_t *pRes;
	azalea_view_t *pView;
	zend_string *tstr;

	if (folderName) {
		zend_update_property_str(azaleaControllerCe, this, ZEND_STRL("_folder"), folderName);
	} else {
		zend_update_property_null(azaleaControllerCe, this, ZEND_STRL("_folder"));
	}
	zend_update_property_str(azaleaControllerCe, this, ZEND_STRL("_controller"), controllerName);
	// request
	zend_update_property(azaleaControllerCe, this, ZEND_STRL("req"), azaleaGetRequest());
	// response
	pRes = azaleaGetResponse(this);
	if (pRes) {
		zend_update_property(azaleaControllerCe, this, ZEND_STRL("res"), pRes);
	} else {
		zend_update_property_null(azaleaControllerCe, this, ZEND_STRL("res"));
	}

	// view
	azaleaViewInit(this, controllerName);

	// call __init method
	if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__init"))) {
		zend_call_method_with_0_params(this, ce, NULL, "__init", NULL);
	}
}
/* }}} */

/* {{{ proto getSession */
PHP_METHOD(azalea_controller, getSession)
{
	object_init_ex(return_value, azaleaSessionCe);
}
/* }}} */

/* {{{ proto loadModel */
PHP_METHOD(azalea_controller, loadModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_controller, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto notFound */
PHP_METHOD(azalea_controller, notFound)
{
	zend_string *message = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|S", &message) == FAILURE) {
		return;
	}

	throw404(message);
}
/* }}} */
