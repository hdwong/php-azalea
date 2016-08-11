/*
 * azalea/template.c
 *
 * Created by Bun Wong on 16-7-23.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/view.h"
#include "azalea/template.h"

#include "ext/standard/html.h"  // for php_escape_html_entities
#include "main/SAPI.h"  // for SG

/* {{{ get_default_charset
 */
static char *get_default_charset(void) {
	if (PG(internal_encoding) && PG(internal_encoding)[0]) {
		return PG(internal_encoding);
	} else if (SG(default_charset) && SG(default_charset)[0] ) {
		return SG(default_charset);
	}
	return NULL;
}
/* }}} */

/* {{{ class Azalea\View template functions
 */
static zend_function_entry azalea_template_functions[] = {
	ZEND_NAMED_FE(p, ZEND_FN(azalea_template_printf), NULL)  // escape & printf
	ZEND_NAMED_FE(t, ZEND_FN(azalea_template_sprintf), NULL)  // escape & sprintf
	ZEND_NAMED_FE(url, ZEND_FN(azalea_url), NULL)  // url(url, includeHost)
	ZEND_NAMED_FE(conf, ZEND_MN(azalea_config_get), NULL)  // conf(key, default)
	ZEND_NAMED_FE(conf2, ZEND_MN(azalea_config_getSub), NULL)  // conf2(key, subkey, default)
	PHP_FE_END
};
/* }}} */

/* {{{ proto void p( string $string ) */
PHP_FUNCTION(azalea_template_printf)
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

/* {{{ proto string t( string $string ) */
PHP_FUNCTION(azalea_template_sprintf)
{
	zend_string *text;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &text) == FAILURE) {
		return;
	}
	text = php_escape_html_entities_ex((unsigned char *) ZSTR_VAL(text), ZSTR_LEN(text), 0, ENT_QUOTES, get_default_charset(), 1);
	RETVAL_STR(text);
}
/* }}} */

/* {{{ proto registerTemplateFunctions */
void azaleaRegisterTemplateFunctions()
{
	if (0 == AZALEA_G(renderLevel)++) {
		zend_register_functions(NULL, azalea_template_functions, NULL, MODULE_TEMPORARY);
	}
}
/* }}} */

/* {{{ proto unregisterTemplateFunctions */
void azaleaUnregisterTemplateFunctions()
{
	if (0 == --AZALEA_G(renderLevel)) {
		zend_unregister_functions(azalea_template_functions, -1, NULL);
	}
}
/* }}} */

