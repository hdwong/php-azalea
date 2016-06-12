/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_rand.h"
#include "php_azalea.h"

/* If you declare any globals in php_azalea.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(azalea)
*/

/* True global resources - no need for thread safety here */
static int le_azalea;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("azalea.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_azalea_globals, azalea_globals)
    STD_PHP_INI_ENTRY("azalea.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_azalea_globals, azalea_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto long now(void) */
PHP_FUNCTION(now)
{
    RETURN_LONG(1);
}
/* }}} */

/* {{{ proto string randomString(long len, string mode) */
PHP_FUNCTION(randomString)
{
    long len;
	char *mode = NULL;
	size_t mode_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|s", &len, &mode, &mode_len) == FAILURE) {
		return;
	}
    if (len < 1) {
        php_error_docref(NULL, E_WARNING, "String length is smaller than 1");
        RETURN_FALSE;
    }

    static char *base = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *p = base;
    size_t l = 62;

    if (mode) {
        if (strcmp(mode, "10") == 0) {
            // [0-9a-f]
            l = 16;
        } else if (*mode == 'c') {
            // [a-zA-Z]
            p += 10;
            l = 52;
        } else if (strcmp(mode, "ln") == 0 || strcmp(mode, "un") == 0) {
            // [0-9a-z] || [0-9A-Z]
            l = 36;
        } else if (*mode == 'l' || *mode == 'u') {
            // [a-z] || [A-Z]
            p += 10;
            l = 26;
        }
    }

    char result[len + 1];
    result[len] = '\0';
    php_uint32 number;
    l -= 1; // for RAND_RANGE
    bool upper = mode && *mode == 'u';
    if (!BG(mt_rand_is_seeded)) {
        php_mt_srand(GENERATE_SEED());
    }
    for (long i = 0; i < len; ++i) {
        number = php_mt_rand() >> 1;
        RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
        result[i] = upper ? toupper(*(p + number)) : *(p + number);
    }
    RETURN_STRING(result);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_azalea_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_azalea_init_globals(zend_azalea_globals *azalea_globals)
{
	azalea_globals->global_value = 0;
	azalea_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(azalea)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(azalea)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(azalea)
{
#if defined(COMPILE_DL_AZALEA) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(azalea)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(azalea)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "azalea support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ azalea_functions[]
 *
 * Every user visible function must have an entry in azalea_functions[].
 */
const zend_function_entry azalea_functions[] = {
	PHP_FE(now,	NULL)
    PHP_FE(randomString, NULL)
	PHP_FE_END	/* Must be the last line in azalea_functions[] */
};
/* }}} */

/* {{{ azalea_module_entry
 */
zend_module_entry azalea_module_entry = {
	STANDARD_MODULE_HEADER,
	"azalea",
	azalea_functions,
	PHP_MINIT(azalea),
	PHP_MSHUTDOWN(azalea),
	PHP_RINIT(azalea),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(azalea),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(azalea),
	PHP_AZALEA_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_AZALEA
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(azalea)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
