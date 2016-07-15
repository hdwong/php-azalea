/*
 * azalea/model.c
 *
 * Created by Bun Wong on 16-7-15.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"

zend_class_entry *azalea_model_ce;

/* {{{ class Azalea\Model methods
 */
static zend_function_entry azalea_model_methods[] = {
	PHP_ME(azalea_model, getModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(model)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Model), azalea_model_methods);
	azalea_model_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_model_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	return SUCCESS;
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_model, getModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */
