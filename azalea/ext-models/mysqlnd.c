/*
 * mysqlnd.c
 *
 * Created by Bun Wong on 17-9-3.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#ifdef WITH_MYSQLND

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/exception.h"
#include "azalea/ext-models/mysqlnd.h"

zend_class_entry *azalea_ext_model_mysqlnd_ce;

/* {{{ class MysqlndModel methods */
static zend_function_entry azalea_ext_model_mysqlnd_methods[] = {
	PHP_ME(azalea_ext_model_mysqlnd, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(MysqlndModel), azalea_ext_model_mysqlnd_methods);
	azalea_ext_model_mysqlnd_ce = zend_register_internal_class_ex(&ce, azaleaModelCe);
	azalea_ext_model_mysqlnd_ce->ce_flags |= ZEND_ACC_FINAL;

	mysqlnd_library_init();

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_ext_model_mysqlnd, __construct) {}
/* }}} */


//#endif
