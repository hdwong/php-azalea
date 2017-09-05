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
#include "ext/standard/php_string.h"  // for php_trim
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

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
	if (!(pReq = zend_hash_str_find(Z_ARRVAL(AZALEA_G(instances)), ZEND_STRL("_request")))) {
		azalea_request_t req = {{0}};
		pReq = &req;
		object_init_ex(pReq, azaleaRequestCe);
		add_assoc_zval_ex(&AZALEA_G(instances), ZEND_STRL("_request"), pReq);
	}
	zend_update_property(azaleaControllerCe, this, ZEND_STRL("req"), pReq);
	// response
	{
		azalea_response_t res = {{0}};
		tstr = strpprintf(0, "_response_%s", ZSTR_VAL(controllerName));
		pRes = &res;
		object_init_ex(pRes, azaleaResponseCe);
		zend_update_property(azaleaResponseCe, pRes, ZEND_STRL("_instance"), this);
		add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(tstr), ZSTR_LEN(tstr), pRes);
		zend_string_release(tstr);
		zend_update_property(azaleaControllerCe, this, ZEND_STRL("res"), pRes);
	}
	// view
	{
		azalea_view_t view = {{0}};
		zval data, env, *staticHost, *staticPath, *themeName;
		zend_string *tpldir, *tstr;

		tstr = strpprintf(0, "_view_%s", ZSTR_VAL(controllerName));
		pView = &view;
		object_init_ex(pView, azaleaViewCe);
		add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(tstr), ZSTR_LEN(tstr), pView);
		zend_string_release(tstr);
		zend_update_property(azaleaControllerCe, this, ZEND_STRL("view"), pView);
		// environ
		array_init(&env);
		// environ.tpldir
		staticHost = azaleaConfigSubFindEx(ZEND_STRL("path"), ZEND_STRL("static_host"));
		if (staticHost && Z_TYPE_P(staticHost) != IS_NULL && Z_STRLEN_P(staticHost)) {
			tpldir = zend_string_copy(Z_STR_P(staticHost));
		} else {
			tpldir = php_trim(AZALEA_G(baseUri), ZEND_STRL("/"), 2);
			staticPath = azaleaConfigSubFindEx(ZEND_STRL("path"), ZEND_STRL("static"));
			if (staticPath && Z_TYPE_P(staticPath) != IS_NULL && Z_STRLEN_P(staticPath)) {
				tstr = tpldir;
				tpldir = strpprintf(0, "%s%c%s", ZSTR_VAL(tpldir), DEFAULT_SLASH, Z_STRVAL_P(staticPath));
				zend_string_release(tstr);
			}
			themeName = azaleaConfigSubFindEx(ZEND_STRL("theme"), NULL, 0);
			if (themeName && Z_TYPE_P(themeName) != IS_NULL && Z_STRLEN_P(themeName)) {
				tstr = tpldir;
				tpldir = strpprintf(0, "%s%c%s", ZSTR_VAL(tpldir), DEFAULT_SLASH, Z_STRVAL_P(themeName));
				zend_string_release(tstr);
			}
		}
		add_assoc_str_ex(&env, ZEND_STRL("tpldir"), tpldir);
		// upate environ
		zend_update_property(azaleaViewCe, pView, ZEND_STRL("_environ"), &env);
		zval_ptr_dtor(&env);
		// data
		array_init(&data);
		zend_update_property(azaleaViewCe, pView, ZEND_STRL("_data"), &data);
		zval_ptr_dtor(&data);
	}

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
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_controller, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
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
