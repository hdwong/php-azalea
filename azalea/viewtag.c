/*
 * azalea/viewtag.c
 *
 * Created by Bun Wong on 19-9-12.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/view.h"
#include "azalea/viewtag.h"

#include "Zend/zend_smart_str.h"	// for smart_str
#include "ext/standard/php_var.h"

/* {{{ class Azalea\View tag functions */
static zend_function_entry azalea_viewtag_functions[] = {
	ZEND_NAMED_FE(js, ZEND_FN(azalea_viewtag_js), NULL)
	ZEND_NAMED_FE(css, ZEND_FN(azalea_viewtag_css), NULL)
	PHP_FE_END
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION  */
AZALEA_STARTUP_FUNCTION(viewtag)
{
	zend_hash_init(AG(viewTagFunctions), 0, NULL, ZVAL_PTR_DTOR, 1);
	zend_register_functions(NULL, azalea_viewtag_functions, AG(viewTagFunctions), MODULE_PERSISTENT);

	return SUCCESS;
}
/* }}} */

void azaleaViewtagCallFunction(INTERNAL_FUNCTION_PARAMETERS)
{
	zval *args = NULL, retval, *this;
	zend_string *functionName, *lcName;
	zend_fcall_info fci;
	zend_fcall_info_cache fcic;
	zend_function *function;
	int argc = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|*", &functionName, &args, &argc) == FAILURE) {
		return;
	}
	lcName = zend_string_tolower(functionName);
	if (!(function = zend_hash_find_ptr(AG(viewTagFunctions), lcName))) {
		php_error_docref0(NULL, E_WARNING, "Couldn't find tag `%s`", ZSTR_VAL(functionName));
		zend_string_release(lcName);
		return;
	}
	zend_string_release(lcName);

	if (execute_data->prev_execute_data &&
				instanceof_function(Z_OBJCE(execute_data->prev_execute_data->This), azaleaViewCe)) {
		this = &(execute_data->prev_execute_data->This);
	} else {
		this = NULL;
	}

	return_value = &retval;
	fci.size = sizeof(fci);
	fci.retval = return_value;
	fci.param_count = argc;
	fci.params = args;
	fci.no_separation = 1;
#if PHP_VERSION_ID < 70100
	fci.symbol_table = NULL;
#endif
	fcic.initialized = 1;
	fcic.function_handler = function;	// 设置 function_handler
	fcic.object = Z_OBJ_P(this);

	if (zend_call_function(&fci, &fcic) == SUCCESS) {
		if (Z_TYPE_P(return_value) == IS_STRING) {
			PHPWRITE(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
		}
	} else {
		php_error_docref0(NULL, E_WARNING, "Call tag `%s` error", ZSTR_VAL(functionName));
	}
	zval_ptr_dtor(return_value);
}

/* {{{ proto _js */
PHP_FUNCTION(azalea_viewtag_js)
{
	zval *pData, *js, *tpldir, *this = getThis();
	zend_string *tstr;
	smart_str buf = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &js) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(js) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(js)) == 0) {
		return;
	}
	if (!(tpldir = azaleaViewTpldir(this))) {
		return;
	}

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(js), pData) {
		if (Z_TYPE_P(pData) == IS_STRING) {
			if (strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("http://")) &&
					strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("https://")) &&
					strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("//"))) {
				// 相对路径
				tstr = strpprintf(0, "<script src=\"%s/%s\"></script>\n", Z_STRVAL_P(tpldir), Z_STRVAL_P(pData));
			} else {
				tstr = strpprintf(0, "<script src=\"%s\"></script>\n", Z_STRVAL_P(pData));
			}
			smart_str_append(&buf, tstr);
			zend_string_release(tstr);
		}
	} ZEND_HASH_FOREACH_END();

	smart_str_0(&buf);
	RETVAL_STRINGL(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
	smart_str_free(&buf);
}
/* }}} */

/* {{{ proto _css */
PHP_FUNCTION(azalea_viewtag_css)
{
	zval *pData, *css, *tpldir, *this = getThis();
	zend_string *media = NULL, *tstr;
	smart_str buf = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|S", &css, &media) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(css) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(css)) == 0) {
		return;
	}
	if (!(tpldir = azaleaViewTpldir(this))) {
		return;
	}
	if (!media) {
		media = zend_string_init(ZEND_STRL("all"), 0);
	} else {
		zend_string_addref(media);
	}

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(css), pData) {
		if (Z_TYPE_P(pData) == IS_STRING) {
			if (strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("http://")) &&
					strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("https://")) &&
					strncasecmp(Z_STRVAL_P(pData), ZEND_STRL("//"))) {
				// 相对路径
				tstr = strpprintf(0, "<link rel=\"stylesheet\" href=\"%s/%s\" media=\"%s\">\n", Z_STRVAL_P(tpldir), Z_STRVAL_P(pData), ZSTR_VAL(media));
			} else {
				tstr = strpprintf(0, "<link rel=\"stylesheet\" href=\"%s\" media=\"%s\">\n", Z_STRVAL_P(pData), ZSTR_VAL(media));
			}
			smart_str_append(&buf, tstr);
			zend_string_release(tstr);
		}
	} ZEND_HASH_FOREACH_END();

	zend_string_release(media);
	smart_str_0(&buf);
	RETVAL_STRINGL(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
	smart_str_free(&buf);
}
/* }}} */
