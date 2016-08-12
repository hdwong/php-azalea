/*
 * loader.c
 *
 * Created by Bun Wong on 16-8-10.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/loader.h"
#include "azalea/config.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"

#include "ext/standard/php_filestat.h"	// for php_stat

zend_class_entry *azalea_loader_ce;

/* {{{ class Azalea\Loader methods */
static zend_function_entry azalea_loader_methods[] = {
	PHP_ME(azalea_loader, load, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(loader)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Loader), azalea_loader_methods);
	azalea_loader_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_loader_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto bool load(string $filename) */
PHP_METHOD(azalea_loader, load)
{
	zend_string *filename;
	int result;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &filename) == FAILURE) {
		return;
	}

	result = azaleaRequire(ZSTR_VAL(filename), 1);
	if (0 == result) {
		php_error_docref(NULL, E_ERROR, "No such file `%s`", ZSTR_VAL(filename));
	}
	RETURN_LONG(result);
}
/* }}} */

/** {{{ int azaleaRequiree(char *filename, zend_bool once) */
int azaleaRequire(char *filename, zend_bool once)
{
	zend_file_handle file_handle;
	zend_op_array *op_array;
	zend_string *path;

	char realpath[MAXPATHLEN];

	if (!VCWD_REALPATH(filename, realpath)) {
		return 0;
	}

	path = zend_string_init(realpath, strlen(realpath), 0);
	if (once && zend_hash_exists(&EG(included_files), path)) {
		zend_string_release(path);
		return -1;
	}

	file_handle.filename = realpath;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
	file_handle.handle.fp = NULL;
	op_array = zend_compile_file(&file_handle, ZEND_REQUIRE);

	if (op_array && file_handle.handle.stream.handle) {
		zval dummy;
		ZVAL_NULL(&dummy);
		if (!file_handle.opened_path) {
			file_handle.opened_path = zend_string_copy(path);
		}
		zend_hash_add(&EG(included_files), file_handle.opened_path, &dummy);
	}
	zend_destroy_file_handle(&file_handle);
	zend_string_release(path);

	if (op_array) {
		zval result;
		ZVAL_UNDEF(&result);
		zend_execute(op_array, &result);
		destroy_op_array(op_array);
		efree(op_array);
		if (!EG(exception)) {
			zval_ptr_dtor(&result);
		}
		return 1;
	}
	return 0;
}
/* }}} */
