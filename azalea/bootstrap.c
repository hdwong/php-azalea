/*
 * azalea/bootstrap.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"

#include "ext/standard/php_var.h"	// for php_var_dump function
#include "ext/standard/php_string.h"  // for php_trim

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
	zend_string *environ = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|zS", &config, &environ) == FAILURE) {
		return;
	}

	if (AZALEA_G(bootstrap)) {
		php_error_docref(NULL, E_ERROR, "Only one Azalea bootstrap can be initialized at a request");
		RETURN_FALSE;
	}
	AZALEA_G(bootstrap) = 1;

	// ---------- START ----------

	zval *server, *field;
	zend_string *iniName, *baseUri = NULL, *uri = NULL;
	double now;

	// set timer
	now = azaleaGetMicrotime();
	AZALEA_G(request_time) = now;

	// create output buffer
	if (php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS) == FAILURE) {
		php_error_docref(NULL, E_ERROR, "Failed to create output buffer");
		RETURN_FALSE;
	}

	// set environ
	if (environ && ZSTR_LEN(environ)) {
		AZALEA_G(environ) = zend_string_copy(environ);
	}

	// load config
	azaleaLoadConfig(config);

	// set timezone
	field = azaleaGetConfig("timezone");
	convert_to_string_ex(field);
	if (Z_STRLEN_P(field)) {
		iniName = zend_string_init(AZALEA_STRING("date.timezone"), 0);
		zend_alter_ini_entry(iniName, Z_STR_P(field), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// set error reporting while debug is true
	field = azaleaGetConfig("debug");
	if (Z_TYPE_P(field) == IS_TRUE) {
		EG(error_reporting) = E_ALL;
		iniName = zend_string_init(AZALEA_STRING("display_errors"), 0);
		zend_alter_ini_entry_chars(iniName, AZALEA_STRING("on"), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// set base_uri and uri
	server = zend_hash_str_find(&EG(symbol_table), AZALEA_STRING("_SERVER"));
	if (server && Z_TYPE_P(server) == IS_ARRAY) {
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("SCRIPT_NAME"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			baseUri = zend_string_dup(Z_STR_P(field), 0);
			// dirname
			ZSTR_LEN(baseUri) = zend_dirname(ZSTR_VAL(baseUri), ZSTR_LEN(baseUri));
			if (ZSTR_LEN(baseUri) > 1) {
				// add '/'
				size_t len = ZSTR_LEN(baseUri);
				zend_string_extend(baseUri, len + 1, 0);
				ZSTR_VAL(baseUri)[len] = '/';
				ZSTR_VAL(baseUri)[len + 1] = '\0';
			}
		}
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("PATH_INFO"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			uri = php_trim(Z_STR_P(field), AZALEA_STRING("/"), 3);
		}
	}
	if (!baseUri) {
		baseUri = zend_string_init(AZALEA_STRING("/"), 0);
	}
	AZALEA_G(baseUri) = baseUri;
	if (!uri) {
		uri = zend_string_init(AZALEA_STRING(""), 0);
	}
	AZALEA_G(uri) = uri;

	// set session
	// session.name
	iniName = zend_string_init(AZALEA_STRING("session.name"), 0);
	zend_alter_ini_entry(iniName, Z_STR_P(azaleaGetSubConfig("session", "name")), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	zend_string_release(iniName);
	// session.cookie_lifetime
	field = azaleaGetSubConfig("session", "lifetime");
	convert_to_string_ex(field);
	iniName = zend_string_init(AZALEA_STRING("session.cookie_lifetime"), 0);
	zend_alter_ini_entry(iniName, Z_STR_P(field), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	zend_string_release(iniName);
	// session.cookie_path
	field = azaleaGetSubConfig("session", "path");
	convert_to_string_ex(field);
	if (!Z_STRLEN_P(field)) {
		// use uri for default path
		ZVAL_STR(field, baseUri);
	}
	iniName = zend_string_init(AZALEA_STRING("session.cookie_path"), 0);
	zend_alter_ini_entry(iniName, Z_STR_P(field), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	zend_string_release(iniName);
	// session.cooke_domain
	field = azaleaGetSubConfig("session", "domain");
	convert_to_string_ex(field);
	if (Z_STRLEN_P(field)) {
		iniName = zend_string_init(AZALEA_STRING("session.cookie_domain"), 0);
		zend_alter_ini_entry(iniName, Z_STR_P(field), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// init bootstrap instance
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
	if (!AZALEA_G(baseUri)) {
		RETURN_NULL();
	}
	RETURN_STR(zend_string_copy(AZALEA_G(baseUri)));
}
/* }}} */

/* {{{ proto string getUri(void) */
PHP_METHOD(azalea_bootstrap, getUri)
{
	if (!AZALEA_G(uri)) {
		RETURN_NULL();
	}
	RETURN_STR(zend_string_copy(AZALEA_G(uri)));
}
/* }}} */

/* {{{ proto string getRequestUri(void) */
PHP_METHOD(azalea_bootstrap, getRequestUri)
{
	zval *server, *field;

	server = zend_hash_str_find(&EG(symbol_table), AZALEA_STRING("_SERVER"));
	if (server && Z_TYPE_P(server) != IS_ARRAY) {
		RETURN_NULL();
	}
	field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("REQUEST_URI"));
	if (!field) {
		RETURN_NULL();
	}
    RETURN_ZVAL(field, 1, 0);
}
/* }}} */

/* {{{ proto string getRoute(void) */
PHP_METHOD(azalea_bootstrap, getRoute)
{
    RETURN_TRUE;
}
/* }}} */
