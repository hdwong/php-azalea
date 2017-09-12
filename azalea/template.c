/*
 * azalea/template.c
 *
 * Created by Bun Wong on 16-7-23.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/view.h"
#include "azalea/template.h"
#include "azalea/viewtag.h"
#include "azalea/i18n.h"

#include "ext/standard/html.h"	// for php_escape_html_entities
#include "main/SAPI.h"	// for SG

/* {{{ get_default_charset */
static char *get_default_charset(void) {
	if (PG(internal_encoding) && PG(internal_encoding)[0]) {
		return PG(internal_encoding);
	} else if (SG(default_charset) && SG(default_charset)[0] ) {
		return SG(default_charset);
	}
	return NULL;
}
/* }}} */

/* {{{ class Azalea\View template functions */
static zend_function_entry azalea_template_functions[] = {
	ZEND_NAMED_FE(_p, ZEND_FN(azalea_template_print), NULL)	// escape & echo
	ZEND_NAMED_FE(_sp, ZEND_FN(azalea_template_return), NULL)	// escape & return
	ZEND_NAMED_FE(_render, ZEND_FN(azalea_template_render), NULL)	// render & echo
	ZEND_NAMED_FE(_t, ZEND_FN(azalea_template_translate), NULL)	// translate & echo
	ZEND_NAMED_FE(_tp, ZEND_FN(azalea_template_translatePlural), NULL)	// translatePlural & echo
	ZEND_NAMED_FE(_url, ZEND_FN(azalea_url), NULL)	// like Azalea\url
	ZEND_NAMED_FE(_conf, ZEND_MN(azalea_config_get), NULL)	// like Azalea\Config::get
	ZEND_NAMED_FE(_debug, ZEND_FN(azalea_debug), NULL)	// like Azalea\debug
	ZEND_NAMED_FE(_tag, azaleaViewtagCallFunction, NULL)	// output tag & echo
	PHP_FE_END
};
/* }}} */

/* {{{ proto _p */
PHP_FUNCTION(azalea_template_print)
{
	zend_string *text;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &text) == FAILURE) {
		return;
	}
	text = php_escape_html_entities_ex((unsigned char *) ZSTR_VAL(text), ZSTR_LEN(text), 0, ENT_QUOTES, get_default_charset(), 1);
	PHPWRITE(ZSTR_VAL(text), ZSTR_LEN(text));
	zend_string_release(text);
}
/* }}} */

/* {{{ proto _sp */
PHP_FUNCTION(azalea_template_return)
{
	zend_string *text;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &text) == FAILURE) {
		return;
	}
	text = php_escape_html_entities_ex((unsigned char *) ZSTR_VAL(text), ZSTR_LEN(text), 0, ENT_QUOTES, get_default_charset(), 1);
	RETVAL_STR(text);
}
/* }}} */

/* {{{ proto _render */
PHP_FUNCTION(azalea_template_render)
{
	if (execute_data->prev_execute_data &&
			instanceof_function(Z_OBJCE(execute_data->prev_execute_data->This), azaleaViewCe)) {
		zval retval;
		return_value = &retval;
		azaleaViewRender(INTERNAL_FUNCTION_PARAM_PASSTHRU, &(execute_data->prev_execute_data->This));
		if (Z_TYPE_P(return_value) == IS_STRING) {
			PHPWRITE(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
		}
		zval_ptr_dtor(return_value);
	}
}
/* }}} */

/* {{{ proto _t */
PHP_FUNCTION(azalea_template_translate)
{
	zval retval;
	return_value = &retval;
	azaleaI18nTranslate(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	if (Z_TYPE_P(return_value) == IS_STRING) {
		PHPWRITE(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
	}
	zval_ptr_dtor(return_value);
}
/* }}} */

/* {{{ proto _tp */
PHP_FUNCTION(azalea_template_translatePlural)
{
	zval retval;
	return_value = &retval;
	azaleaI18nTranslatePlural(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	if (Z_TYPE_P(return_value) == IS_STRING) {
		PHPWRITE(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
	}
	zval_ptr_dtor(return_value);
}
/* }}} */

/* {{{ proto registerTemplateFunctions */
void azaleaRegisterTemplateFunctions()
{
	if (0 == AZALEA_G(renderDepth)++) {
		zend_register_functions(NULL, azalea_template_functions, NULL, MODULE_TEMPORARY);
		AZALEA_G(registeredTemplateFunctions) = 1;
	}
}
/* }}} */

/* {{{ proto unregisterTemplateFunctions */
void azaleaUnregisterTemplateFunctions(zend_bool forced)
{
	if (0 == --AZALEA_G(renderDepth) || forced) {
		zend_unregister_functions(azalea_template_functions, -1, NULL);
		AZALEA_G(registeredTemplateFunctions) = 0;
		AZALEA_G(renderDepth) = 0;
	}
}
/* }}} */
