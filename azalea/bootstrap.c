/*
 * azalea/bootstrap.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"

zend_class_entry *azalea_bootstrap_ce;

azalea_bootstrap_t *azalea_bootstrap_insntance(azalea_bootstrap_t *this_ptr)
{
	if (Z_ISUNDEF_P(this_ptr)) {
		object_init_ex(this_ptr, azalea_bootstrap_ce);
	}
	return this_ptr;
}

/* {{{ class Azalea\Bootstrap methods
 */
static zend_function_entry azalea_bootstrap_methods[] = {
    PHP_ME(azalea_bootstrap, getBaseUri, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_bootstrap, getUri, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_bootstrap, getRequestUri, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_bootstrap, getRoute, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_bootstrap, init, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(azalea_bootstrap, run, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(bootstrap)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Bootstrap), azalea_bootstrap_methods);
	azalea_bootstrap_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_bootstrap_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto Bootstrap init(mixed $config, string $environ = AZALEA_G(environ)) */
PHP_METHOD(azalea_bootstrap, init)
{
	zval *config = NULL;
	zend_array *conf;
	zend_string *environ = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|zS", &config, &environ) == FAILURE) {
		return;
	}

	if (AZALEA_G(bootstrap)) {
		php_error_docref(NULL, E_ERROR, "Only one Azalea bootstrap can be initialized at a request");
		RETURN_FALSE;
	}
	AZALEA_G(bootstrap) = 1;

	if (environ && ZSTR_LEN(environ)) {
		// update environ global variable
		AZALEA_G(environ) = zend_string_copy(environ);
	}

	if (config && (Z_TYPE_P(config) == IS_STRING || Z_TYPE_P(config) == IS_ARRAY)) {
		conf = loadConfig(config);
	}

	azalea_bootstrap_t *instance, rv = {{0}};
	if ((instance = azalea_bootstrap_insntance(&rv)) != NULL) {
		RETURN_ZVAL(instance, 0, 0);
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

/* {{{ proto bool run(void) */
PHP_METHOD(azalea_bootstrap, run)
{
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getBaseuri(void) */
PHP_METHOD(azalea_bootstrap, getBaseUri)
{
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getUri(void) */
PHP_METHOD(azalea_bootstrap, getUri)
{
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getRequestUri(void) */
PHP_METHOD(azalea_bootstrap, getRequestUri)
{
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string getRoute(void) */
PHP_METHOD(azalea_bootstrap, getRoute)
{
    RETURN_TRUE;
}
/* }}} */
