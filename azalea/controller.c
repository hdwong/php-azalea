/*
 * azalea/controller.c
 *
 * Created by Bun Wong on 16-7-8.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/controller.h"

zend_class_entry *azalea_controller_ce;

azalea_controller_t *azalea_controller_instance(azalea_controller_t *this_ptr)
{
	if (Z_ISUNDEF_P(this_ptr)) {
		object_init_ex(this_ptr, azalea_controller_ce);
	}
	return this_ptr;
}

/* {{{ class Azalea\Controller methods
 */
static zend_function_entry azalea_controller_methods[] = {
//	PHP_ME(azalea_controller, __init, NULL, ZEND_ACC_PROTECTED)
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

	return SUCCESS;
}
/* }}} */

///* {{{ proto bool run(void) */
//PHP_METHOD(azalea_controller, __init)
//{
//}
///* }}} */
