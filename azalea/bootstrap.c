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

#include "ext/standard/php_var.h"  // for php_var_dump function
#include "ext/standard/php_string.h"  // for php_trim
#include "ext/standard/php_filestat.h"  // for php_stat

zend_class_entry *azalea_bootstrap_ce;

azalea_bootstrap_t *azalea_bootstrap_instance(azalea_bootstrap_t *this_ptr)
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

	zval *server, *field, conf;
	zend_string *iniName, *directory = NULL, *baseUri = NULL, *uri = NULL;
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
	ZVAL_COPY(&conf, field);
	convert_to_string_ex(&conf);
	if (Z_STRLEN(conf)) {
		iniName = zend_string_init(AZALEA_STRING("date.timezone"), 0);
		zend_alter_ini_entry(iniName, Z_STR(conf), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}
	zval_ptr_dtor(&conf);

	// set error reporting while debug is true
	field = azaleaGetConfig("debug");
	if (Z_TYPE_P(field) == IS_TRUE) {
		EG(error_reporting) = E_ALL;
		iniName = zend_string_init(AZALEA_STRING("display_errors"), 0);
		zend_alter_ini_entry_chars(iniName, AZALEA_STRING("on"), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// set directory / base_uri / uri
	server = zend_hash_str_find(&EG(symbol_table), AZALEA_STRING("_SERVER"));
	if (server && Z_TYPE_P(server) == IS_ARRAY) {
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("SCRIPT_FILENAME"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			directory = zend_string_dup(Z_STR_P(field), 0);
			// dirname
			ZSTR_LEN(directory) = zend_dirname(ZSTR_VAL(directory), ZSTR_LEN(directory));
		}
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("SCRIPT_NAME"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			baseUri = zend_string_dup(Z_STR_P(field), 0);
			// dirname
			size_t len;
			len = ZSTR_LEN(baseUri) = zend_dirname(ZSTR_VAL(baseUri), ZSTR_LEN(baseUri));
			if (len > 1) {
				// add '/'
				zend_string *t = zend_string_alloc(len + 1, 0);
				memcpy(ZSTR_VAL(t), ZSTR_VAL(baseUri), len);
				ZSTR_VAL(t)[len] = '/';
				ZSTR_VAL(t)[len + 1] = '\0';
				zend_string_free(baseUri);
				baseUri = t;
			}
		}
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), AZALEA_STRING("PATH_INFO"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			uri = php_trim(Z_STR_P(field), AZALEA_STRING("/"), 3);
		}
	}
	if (!directory) {
		directory = zend_string_init(AZALEA_STRING("/"), 0);
	}
	AZALEA_G(directory) = directory;
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
	ZVAL_COPY(&conf, field);
	convert_to_string_ex(&conf);
	iniName = zend_string_init(AZALEA_STRING("session.cookie_lifetime"), 0);
	zend_alter_ini_entry(iniName, Z_STR(conf), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	zend_string_release(iniName);
	zval_ptr_dtor(&conf);
	// session.cookie_path
	field = azaleaGetSubConfig("session", "path");
	ZVAL_COPY(&conf, field);
	convert_to_string_ex(&conf);
	if (!Z_STRLEN(conf)) {
		// use baseUri for default path
		ZVAL_STR(&conf, baseUri);
	}
	iniName = zend_string_init(AZALEA_STRING("session.cookie_path"), 0);
	zend_alter_ini_entry(iniName, Z_STR(conf), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	zend_string_release(iniName);
	zval_ptr_dtor(&conf);
	// session.cooke_domain
	field = azaleaGetSubConfig("session", "domain");
	ZVAL_COPY(&conf, field);
	convert_to_string_ex(&conf);
	if (Z_STRLEN(conf)) {
		iniName = zend_string_init(AZALEA_STRING("session.cookie_domain"), 0);
		zend_alter_ini_entry(iniName, Z_STR(conf), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}
	zval_ptr_dtor(&conf);

	// ---------- END ----------

	// init bootstrap instance
	azalea_bootstrap_t *instance, rv = {{0}};
	if ((instance = azalea_bootstrap_instance(&rv)) != NULL) {
		RETURN_ZVAL(instance, 0, 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool run(void) */
PHP_METHOD(azalea_bootstrap, run)
{
	zend_string *uri, *controllersPath, *modelsPath, *viewsPath;
	zval *field, paths, basePath;
	zend_ulong pathsOffset = 0;

	// request uri
	uri = AZALEA_G(uri);
	// static router
	field = azaleaGetConfig("router");
	if (field) {
		// TODO check static router
		zval staticRouter;
		ZVAL_COPY(&staticRouter, field);
		zval_ptr_dtor(&staticRouter);
	}

	// get paths
	array_init(&paths);
	if (ZSTR_LEN(uri)) {
		zend_string *delim = zend_string_init(AZALEA_STRING("/"), 0);
		php_explode(delim, uri, &paths, ZEND_LONG_MAX);
		zend_string_release(delim);
	}
	field = azaleaGetSubConfig("path", "controllers");
	controllersPath = zend_string_init(Z_STRVAL_P(field), Z_STRLEN_P(field), 0);
	field = azaleaGetSubConfig("path", "models");
	modelsPath = zend_string_init(Z_STRVAL_P(field), Z_STRLEN_P(field), 0);
	field = azaleaGetSubConfig("path", "views");
	viewsPath = zend_string_init(Z_STRVAL_P(field), Z_STRLEN_P(field), 0);
	field = azaleaGetSubConfig("path", "basepath");
	if (field && Z_TYPE_P(field) == IS_STRING) {
		ZVAL_COPY(&basePath, field);
	} else {
		ZVAL_STR(&basePath, AZALEA_G(directory));
	}
	if (Z_STRVAL(basePath)[Z_STRLEN(basePath) - 1] != '/') {
		zval t;
		ZVAL_STRINGL(&t, "/", sizeof("/") - 1);
		concat_function(&basePath, &basePath, &t);
		zval_ptr_dtor(&t);
	}
	if (ZSTR_VAL(controllersPath)[0] != '/') {
		// relative path
		zval t1, t2;
		ZVAL_COPY(&t1, &basePath);
		ZVAL_STR(&t2, controllersPath);
		concat_function(&t1, &t1, &t2);
		controllersPath = zend_string_copy(Z_STR(t1));
		zval_ptr_dtor(&t1);
		zval_ptr_dtor(&t2);
	}
	if (ZSTR_VAL(modelsPath)[0] != '/') {
		// relative path
		zval t1, t2;
		ZVAL_COPY(&t1, &basePath);
		ZVAL_STR(&t2, modelsPath);
		concat_function(&t1, &t1, &t2);
		modelsPath = zend_string_copy(Z_STR(t1));
		zval_ptr_dtor(&t1);
		zval_ptr_dtor(&t2);
	}
	if (ZSTR_VAL(viewsPath)[0] != '/') {
		// relative path
		zval t1, t2;
		ZVAL_COPY(&t1, &basePath);
		ZVAL_STR(&t2, viewsPath);
		concat_function(&t1, &t1, &t2);
		viewsPath = zend_string_copy(Z_STR(t1));
		zval_ptr_dtor(&t1);
		zval_ptr_dtor(&t2);
	}
	field = azaleaGetConfig("theme");
	if (field && Z_TYPE_P(field) == IS_STRING && Z_STRLEN_P(field)) {
		// add views subpath
		zval t1, t2, t3;
		ZVAL_STR(&t1, viewsPath);
		ZVAL_STRINGL(&t2, "/", sizeof("/") - 1);
		ZVAL_COPY(&t3, field);
		concat_function(&t1, &t1, &t2);
		concat_function(&t1, &t1, &t3);
		viewsPath = zend_string_copy(Z_STR(t1));
		zval_ptr_dtor(&t1);
		zval_ptr_dtor(&t2);
		zval_ptr_dtor(&t3);
	}
	zval_ptr_dtor(&basePath);
	AZALEA_G(controllersPath) = controllersPath;
	AZALEA_G(modelsPath) = modelsPath;
	AZALEA_G(viewsPath) = viewsPath;

	// folder
	field = zend_hash_index_find(Z_ARRVAL(paths), pathsOffset);
	if (!field) {
		goto labelDispatch;
	}
	zval exists, t1, t2, t3;
	zend_string *lc = php_string_tolower(Z_STR_P(field));
	ZVAL_STR(&t1, controllersPath);
	ZVAL_STRINGL(&t2, "/", sizeof("/") - 1);
	ZVAL_STR(&t3, lc);
	concat_function(&t1, &t1, &t2);
	concat_function(&t1, &t1, &t3);
	php_stat(Z_STRVAL(t1), (php_stat_len) Z_STRLEN(t1), FS_IS_DIR, &exists);
	if (Z_TYPE(exists) == IS_TRUE) {
		++pathsOffset;
		AZALEA_G(folderName) = zend_string_copy(lc);
		zval_ptr_dtor(&t1);
	}
	zval_ptr_dtor(&t2);
	zval_ptr_dtor(&t3);

	// controller
	field = zend_hash_index_find(Z_ARRVAL(paths), pathsOffset);
	if (!field) {
		goto labelDispatch;
	}
	++pathsOffset;
	if (AZALEA_G(controllerName)) {
		zend_string_release(AZALEA_G(controllerName));
	}
	AZALEA_G(controllerName) = php_string_tolower(Z_STR_P(field));

	// action
	field = zend_hash_index_find(Z_ARRVAL(paths), pathsOffset);
	if (!field) {
		goto labelDispatch;
	}
	++pathsOffset;
	if (AZALEA_G(actionName)) {
		zend_string_release(AZALEA_G(actionName));
	}
	AZALEA_G(actionName) = php_string_tolower(Z_STR_P(field));

//	php_printf("ControllersPath: %s <br> ModelsPath: %s <br> ViewsPath: %s<br>", ZSTR_VAL(controllersPath), ZSTR_VAL(modelsPath), ZSTR_VAL(viewsPath));
//	php_printf("Folder: %s <br> Controller: %s <br> Action: %s", AZALEA_G(folderName) ? ZSTR_VAL(AZALEA_G(folderName)) : "--", ZSTR_VAL(AZALEA_G(controllerName)), ZSTR_VAL(AZALEA_G(actionName)));

	// arguments

labelDispatch:

	zval_ptr_dtor(&paths);

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
    array_init(return_value);
    if (AZALEA_G(folderName)) {
    	add_assoc_str(return_value, "folder", AZALEA_G(folderName));
    } else {
    	add_assoc_null(return_value, "folder");
    }
    add_assoc_str(return_value, "controller", AZALEA_G(controllerName));
    add_assoc_str(return_value, "action", AZALEA_G(actionName));
}
/* }}} */
