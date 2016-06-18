/*
 * azalea/bootstrap.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "azalea.h"
#include "bootstrap.h"

/* {{{ class Azalea\Bootstrap
 */
zend_class_entry *azalea_ce_bootstrap;

static zend_function_entry bootstrap_methods[] = {
    PHP_ME(Bootstrap, test, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void azalea_init_bootstrap(TSRMLS_D)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Bootstrap), bootstrap_methods);
    azalea_ce_bootstrap = zend_register_internal_class(&ce TSRMLS_CC);
}

/* {{{ proto bool test(void) */
PHP_METHOD(Bootstrap, test)
{
    RETURN_TRUE;
}
/* }}} */
