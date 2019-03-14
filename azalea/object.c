/*
 * object.c
 *
 * Created by Bun Wong on 17-11-27.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/object.h"

zend_class_entry *azaleaObjectCe;

/* {{{ class Azalea\Model methods
 */
static zend_function_entry azalea_object_methods[] = {
	PHP_ME(azalea_object, loadModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_object, getModel, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ABSTRACT_ME(azalea_object, getId, NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(object)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Object), azalea_object_methods);
	azaleaObjectCe = zend_register_internal_class(&ce);
	azaleaObjectCe->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	return SUCCESS;
}
/* }}} */

/* {{{ proto loadModel */
PHP_METHOD(azalea_object, loadModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_object, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */
