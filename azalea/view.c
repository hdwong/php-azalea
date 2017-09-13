/*
 * azalea/view.c
 *
 * Created by Bun Wong on 16-7-11.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/loader.h"
#include "azalea/config.h"
#include "azalea/controller.h"
#include "azalea/view.h"
#include "azalea/template.h"
#include "azalea/viewtag.h"
#include "azalea/exception.h"

#include "ext/standard/php_string.h"	// for php_trim
#include "ext/standard/php_var.h"	// for php_var_dump
#include "ext/standard/php_filestat.h"	// for php_stat

zend_class_entry *azaleaViewCe;

/* {{{ class Azalea\View methods */
static zend_function_entry azalea_view_methods[] = {
	PHP_ME(azalea_view, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_view, render, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_view, assign, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_view, append, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_view, clean, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(view)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(View), azalea_view_methods);
	azaleaViewCe = zend_register_internal_class(&ce);
	azaleaViewCe->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azaleaViewCe, ZEND_STRL("_environ"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azaleaViewCe, ZEND_STRL("_data"), ZEND_ACC_PRIVATE);

	// init viewtag
	AZALEA_STARTUP(viewtag);

	return SUCCESS;
}
/* }}} */

void azaleaViewInit(azalea_controller_t *controllerInstance, zend_string *controllerName)
{
	azalea_view_t view = {{0}}, *this;
	zval data, env, *staticHost, *staticPath, *themeName;
	zend_string *tpldir, *tstr;

	tstr = strpprintf(0, "_view_%s", ZSTR_VAL(controllerName));
	this = &view;
	object_init_ex(this, azaleaViewCe);
	add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(tstr), ZSTR_LEN(tstr), this);	// 加入 instances
	zend_string_release(tstr);
	zend_update_property(azaleaControllerCe, controllerInstance, ZEND_STRL("view"), this);
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
	add_assoc_str_ex(&env, ZEND_STRL("tpldir"), tpldir);	// tpldir 不需要 release
	add_assoc_str_ex(&env, ZEND_STRL("locale"), zend_string_copy(AZALEA_G(locale)));
	// upate environ
	zend_update_property(azaleaViewCe, this, ZEND_STRL("_environ"), &env);
	zval_ptr_dtor(&env);
	// data
	array_init(&data);
	zend_update_property(azaleaViewCe, this, ZEND_STRL("_data"), &data);
	zval_ptr_dtor(&data);
}

/* {{{ proto assignToData */
static void assignToData(azalea_view_t *this, zend_string *name, zval *value)
{
	zval *data;
	if ((data = zend_read_property(azaleaViewCe, this, ZEND_STRL("_data"), 1, NULL)) &&
			Z_TYPE_P(data) == IS_ARRAY) {
		if (zend_hash_update(Z_ARRVAL_P(data), name, value) != NULL) {
			Z_TRY_ADDREF_P(value);
		}
	}
}
/* }}} */

/* {{{ proto assignToDataHt */
static void assignToDataHt(azalea_view_t *this, zend_array *ht)
{
	zval *data;
	if ((data = zend_read_property(azaleaViewCe, this, ZEND_STRL("_data"), 1, NULL)) &&
			Z_TYPE_P(data) == IS_ARRAY) {
		zend_hash_copy(Z_ARRVAL_P(data), ht, (copy_ctor_func_t) zval_add_ref);
	}
}
/* }}} */

/* {{{ proto appendToDateHt */
static void appendToData(azalea_view_t *this, zend_string *name, zval *value)
{
	zval *data;
	if ((data = zend_read_property(azaleaViewCe, this, ZEND_STRL("_data"), 1, NULL)) &&
			Z_TYPE_P(data) == IS_ARRAY) {
		zend_array *htData = Z_ARRVAL_P(data);
		zval dummy, *field = zend_hash_find(htData, name);
		if (field) {
			// found
			if (Z_TYPE_P(field) != IS_ARRAY) {
				return;
			}
			array_init(&dummy);
			// fix for opcache?
			zend_hash_copy(Z_ARRVAL(dummy), Z_ARRVAL_P(field), (copy_ctor_func_t) zval_add_ref);
			field = zend_hash_update(htData, name, &dummy);
		} else {
			// not found & init to array
			array_init(&dummy);
			field = zend_hash_add(htData, name, &dummy);
		}
		htData = Z_ARRVAL_P(field);
		if (Z_TYPE_P(value) == IS_ARRAY) {
			// array
			zval *pVal;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), pVal) {
				zend_hash_next_index_insert(htData, pVal);
				zval_add_ref(pVal);
			} ZEND_HASH_FOREACH_END();
		} else {
			// string
			zend_hash_next_index_insert(htData, value);
			zval_add_ref(value);
		}
	}
}
/* }}} */

zval * azaleaViewTpldir(zval *this)
{
	zval *pData;

	if (!(pData = zend_read_property(azaleaViewCe, this, ZEND_STRL("_environ"), 1, NULL)) ||
			Z_TYPE_P(pData) != IS_ARRAY) {
		return NULL;
	}
	if (!(pData = zend_hash_str_find(Z_ARRVAL_P(pData), ZEND_STRL("tpldir"))) ||
			Z_TYPE_P(pData) != IS_STRING) {
		return NULL;
	}
	return pData;
}

/* {{{ proto checkValidVarName */
static int checkValidVarName(char *varName, int len) /* {{{ */
{
	int i, ch;
	if (!varName) {
		return 0;
	}
	/* These are allowed as first char: [a-zA-Z_\x7f-\xff] */
	ch = (int)((unsigned char *)varName)[0];
	if (varName[0] != '_' &&
			(ch < 65 || ch > 90) &&	// [!A-Z]
			(ch < 97 || ch > 122) &&	// [!a-z]
			(ch < 0x7f || ch > 0xff)) {
		return 0;
	}
	/* And these as the rest: [a-zA-Z0-9_\x7f-\xff] */
	if (len > 1) {
		for (i = 1; i < len; i++) {
			ch = (int)((unsigned char *)varName)[i];
			if (varName[i] != '_' &&
					(ch < 48 || ch > 57) &&	// [!0-9]
					(ch < 65 || ch > 90) &&	// [!A-Z]
					(ch < 97 || ch > 122) &&	// [!a-z]
					(ch < 0x7f || ch > 0xff)) {
				return 0;
			}
		}
	}
	return 1;
}
/* }}} */

/* {{{ proto buildSymbolTable */
static zend_array * buildSymbolTable()
{
	zend_array *symbolTable;
	symbolTable = emalloc(sizeof(zend_array));
	zend_hash_init(symbolTable, 8, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_real_init(symbolTable, 0);

	return symbolTable;
}
/* }}} */

/* {{{ proto appendToSymbolTable */
static void appendToSymbolTable(zend_array *symbolTable, zval *vars)
{
	zend_string *key;
	zval *pData;
#if PHP_VERSION_ID < 70100
	zend_class_entry *scope = EG(scope);
#else
	zend_class_entry *scope = zend_get_executed_scope();
#endif

	if (symbolTable && vars && Z_TYPE_P(vars) == IS_ARRAY) {
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(vars), key, pData) {
			if (!key) {
				continue;
			}
			if (zend_string_equals_literal(key, "GLOBALS")) {
				continue;
			}
			if (zend_string_equals_literal(key, "this") && scope && ZSTR_LEN(scope->name) != 0) {
				continue;
			}
			if (checkValidVarName(ZSTR_VAL(key), ZSTR_LEN(key)) &&
					EXPECTED(zend_hash_add_new(symbolTable, key, pData))) {
				Z_TRY_ADDREF_P(pData);
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

/* {{{ proto destroySymbolTable */
static void destroySymbolTable(zend_array *symbolTable)
{
	zend_array_destroy(symbolTable);
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_view, __construct) {}
/* }}} */

/* {{{ int renderTemplateFile(azalea_view_t *this, zend_array *symbolTable, zend_string *filename, zval *ret) */
static int renderTemplateFile(azalea_view_t *this, zend_array *symbolTable, zend_string *filename, zval *ret)
{
	int status = 0;
	zend_file_handle file_handle;
	zend_op_array *op_array;
	zend_execute_data *call;
	zval result;
	char realpath[MAXPATHLEN];

	if (!VCWD_REALPATH(ZSTR_VAL(filename), realpath)) {
		return 0;
	}

	file_handle.filename = realpath;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
	file_handle.handle.fp = NULL;
	op_array = zend_compile_file(&file_handle, ZEND_INCLUDE);

	if (op_array) {
		if (file_handle.handle.stream.handle) {
			if (!file_handle.opened_path) {
				file_handle.opened_path = zend_string_copy(filename);
			}
			zend_hash_add_empty_element(&EG(included_files), file_handle.opened_path);
		}
		// execute template file

		ZVAL_UNDEF(&result);
		op_array->scope = Z_OBJCE_P(this);
		call = zend_vm_stack_push_call_frame(ZEND_CALL_NESTED_CODE
#if PHP_VERSION_ID >= 70100
				| ZEND_CALL_HAS_SYMBOL_TABLE
#endif
				,
				(zend_function*)op_array, 0, op_array->scope, Z_OBJ_P(this));
		call->symbol_table = symbolTable;
		zend_init_execute_data(call, op_array, &result);
		ZEND_ADD_CALL_FLAG(call, ZEND_CALL_TOP);
		zend_execute_ex(call);
		zend_vm_stack_free_call_frame(call);
		zval_ptr_dtor(&result);
		if (EXPECTED(EG(exception) == NULL)) {
			status = 1;
		}
		destroy_op_array(op_array);
		efree_size(op_array, sizeof(zend_op_array));
	}
	zend_destroy_file_handle(&file_handle);

	return status;
}
/* }}} */

/* {{{ proto azaleaViewRender */
void azaleaViewRender(INTERNAL_FUNCTION_PARAMETERS, azalea_view_t *viewInstance)
{
	zend_string *tplname, *viewsPath, *tplPath;
	zval *vars = NULL, exists, *environVars;
	azalea_view_t *this = viewInstance ? viewInstance : getThis();
	zend_array *symbolTable;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|z", &tplname, &vars) == FAILURE) {
		return;
	}

	// check template file exists
	viewsPath = AZALEA_G(viewsPath);
	tplPath = strpprintf(0, "%s%c%s.phtml", ZSTR_VAL(viewsPath), DEFAULT_SLASH, ZSTR_VAL(tplname));
	php_stat(ZSTR_VAL(tplPath), (php_stat_len) ZSTR_LEN(tplPath), FS_IS_R, &exists);
	if (Z_TYPE(exists) == IS_FALSE) {
		zend_string *message = strpprintf(0, "Template file `%s.phtml` not found.", ZSTR_VAL(tplname));
		throw404(message);
		zend_string_release(message);
		RETURN_FALSE;
	}

	// build a new symbol table
	symbolTable = buildSymbolTable();

	// extract environ vars
	if ((environVars = zend_read_property(azaleaViewCe, this, ZEND_STRL("_environ"), 1, NULL)) &&
			Z_TYPE_P(environVars) == IS_ARRAY) {
		appendToSymbolTable(symbolTable, environVars);
	}
	// extract vars
	if (vars) {
		assignToDataHt(this, Z_ARRVAL_P(vars));
	}
	if ((vars = zend_read_property(azaleaViewCe, this, ZEND_STRL("_data"), 1, NULL)) &&
			Z_TYPE_P(vars) == IS_ARRAY) {
		appendToSymbolTable(symbolTable, vars);
	}
	// start render
	php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS);
	azaleaRegisterTemplateFunctions();
	if (!renderTemplateFile(this, symbolTable, tplPath, NULL)) {
		RETVAL_FALSE;
	} else {
		php_output_get_contents(return_value);
		php_output_discard();
	}
	destroySymbolTable(symbolTable);
	azaleaUnregisterTemplateFunctions(0);
	zend_string_release(tplPath);
}
/* }}} */

/* {{{ proto string render(string $tplname, array $data = null) */
PHP_METHOD(azalea_view, render)
{
	azaleaViewRender(INTERNAL_FUNCTION_PARAM_PASSTHRU, NULL);
}
/* }}} */

/* {{{ proto mixed assign(string name, mixed $value = null) */
PHP_METHOD(azalea_view, assign)
{
	zval *name, *value = NULL;
	azalea_view_t *this = getThis();
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|z", &name, &value) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(name) == IS_ARRAY) {
		assignToDataHt(this, Z_ARRVAL_P(name));
	} else if (value) {
		convert_to_string(name);
		assignToData(this, Z_STR_P(name), value);
	} else {
		// ERROR
		php_error_docref(NULL, E_WARNING, "The second argument must be a valid value if the name is a string");
	}
	RETURN_ZVAL(this, 1, 0);
}
/* }}} */

/* {{{ proto mixed append(string name, mixed $value = null) */
PHP_METHOD(azalea_view, append)
{
	zend_string *name;
	zval *value;
	azalea_view_t *this = getThis();
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sz", &name, &value) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(value) == IS_ARRAY || Z_TYPE_P(value) == IS_STRING) {
		appendToData(this, name, value);
	} else {
		// ERROR
		php_error_docref(NULL, E_WARNING, "The second argument must be a stirng or an array");
	}
	RETURN_ZVAL(this, 1, 0);
}
/* }}} */

/* {{{ proto mixed clean() */
PHP_METHOD(azalea_view, clean)
{
	azalea_view_t *this = getThis();
	zval *data;
	if ((data = zend_read_property(azaleaViewCe, this, ZEND_STRL("_data"), 1, NULL)) &&
			Z_TYPE_P(data) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(data))) {
		zend_hash_clean(Z_ARRVAL_P(data));
	}
}
/* }}} */
