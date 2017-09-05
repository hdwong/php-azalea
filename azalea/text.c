/*
 * azalea/text.c
 *
 * Created by Bun Wong on 17-9-5.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/text.h"

#include "ext/standard/php_rand.h"

zend_class_entry *azaleaTextCe;

/* {{{ class Azalea\Text methods */
static zend_function_entry azalea_text_methods[] = {
	PHP_ME(azalea_text, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_text, random, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_text, mask, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(text)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Text), azalea_text_methods);
	azaleaTextCe = zend_register_internal_class(&ce);
	azaleaTextCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_text, __construct) {}
/* }}} */

/* {{{ proto random */
PHP_METHOD(azalea_text, random)
{
	zend_long len, i, number;
	zend_string *mode = NULL;
	static char *base = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";	// length 62
	static char *code = "23456789abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";	// length 56
	char *p = base, *pMode;
	size_t l = 62;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|S", &len, &mode) == FAILURE) {
		return;
	}
	if (len < 1) {
		php_error_docref(NULL, E_WARNING, "String length is smaller than 1");
		RETURN_FALSE;
	}

	if (mode) {
		pMode = ZSTR_VAL(mode);
		if (strncmp(pMode, "9", 1) == 0) {
			// [1-9]
			p += 1;
			l = 9;
		} else if (strncmp(pMode, "10", 2) == 0 || strncasecmp(pMode, "n", 1) == 0) {
			// [0-9]
			l = 10;
		} else if (strncmp(pMode, "16", 2) == 0) {
			// [0-9a-f]
			l = 16;
		} else if (strncasecmp(pMode, "code", 4) == 0) {
			// 增加用于生成验证码的模式 (避免 1 I l 0 O o 影响识别的字符)
			p = code;
			l = 56;
		} else if (strncasecmp(pMode, "c", 1) == 0) {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (strncasecmp(pMode, "ln", 2) == 0) {
			// [0-9a-z]
			l = 36;
		} else if (strncasecmp(pMode, "un", 2) == 0) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (strncasecmp(pMode, "l", 1) == 0) {
			// [a-z]
			p += 10;
			l = 26;
		} else if (strncasecmp(pMode, "u", 1) == 0) {
			// [A-Z]
			p += 36;
			l = 26;
		}
		// TODO 考虑增加产生不重复字符的模式
	}
	char result[len];
	l -= 1; // for RAND_RANGE
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}
	for (i = 0; i < len; ++i) {
		number = (zend_long) php_mt_rand() >> 1;
		RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
		result[i] = *(p + number);
	}
	RETURN_STRINGL(result, len);
}
/* }}} */

/* {{{ proto mask */
PHP_METHOD(azalea_text, mask)
{
	zend_string *string;
	zend_bool isEmail = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &string, &isEmail) == FAILURE) {
		return;
	}
	string = zend_string_init(ZSTR_VAL(string), ZSTR_LEN(string), 0);
	if (ZSTR_LEN(string) > 1) {
		if (!isEmail) {
			// normal string
		} else {
			// email
		}
	}
	RETURN_STR(string);
}
/* }}} */
