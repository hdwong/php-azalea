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
#include "ext/mbstring/mbstring.h"
#include "Zend/zend_smart_str.h"	// for smart_str

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
	zend_long len, len2 = 0, i, j, number;
	zend_string *mode = NULL;
	static char *base = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";	// length 62
	static char *code = "23456789abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ";	// length 56
	char *p = base, *pMode, *p2 = NULL;
	size_t l = 62, l2 = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|S", &len, &mode) == FAILURE) {
		return;
	}
	if (len < 1 || len > 1024) {
		php_error_docref(NULL, E_WARNING, "String length must > 0 and <= 1024");
		RETURN_FALSE;
	}

	if (mode) {
		pMode = ZSTR_VAL(mode);
		if (0 == strcmp(pMode, "9")) {
			// [1-9][0-9]+
			p2 = base;	// [0-9]
			l2 = 10;
			len2 = len - 1;
			p += 1;	// [1-9]
			l = 9;
			len = 1;
		} else if (0 == strcmp(pMode, "10") || 0 == strcasecmp(pMode, "n")) {
			// [0-9]
			l = 10;
		} else if (0 == strcmp(pMode, "16")) {
			// [0-9a-f]
			l = 16;
		} else if (0 == strcasecmp(pMode, "c")) {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (0 == strcasecmp(pMode, "ln")) {
			// [0-9a-z]
			l = 36;
		} else if (0 == strcasecmp(pMode, "un")) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (0 == strcasecmp(pMode, "l")) {
			// [a-z]
			p += 10;
			l = 26;
		} else if (0 == strcasecmp(pMode, "u")) {
			// [A-Z]
			p += 36;
			l = 26;
		} else if (0 == strcasecmp(pMode, "code")) {
			// 增加用于生成验证码的模式 (避免 1 I l 0 O o 影响识别的字符)
			p = code;
			l = 56;
		}
		// TODO 考虑增加产生不重复字符的模式
	}
	char result[len + len2];
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}
	l -= 1;	// for RAND_RANGE
	l2 -= 1;	// for RAND_RANGE
	// 生成前半部分
	for (i = 0, j = 0; i < len; ++i, ++j) {
		number = (zend_long) php_mt_rand() >> 1;
#if PHP_VERSION_ID >= 70300
		RAND_RANGE_BADSCALING(number, 0, l, PHP_MT_RAND_MAX);
#else
		RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
#endif
		result[j] = *(p + number);
	}
	// 生成后半部分
	for (i = 0; i < len2; ++i, ++j) {
		number = (zend_long) php_mt_rand() >> 1;
#if PHP_VERSION_ID >= 70300
		RAND_RANGE_BADSCALING(number, 0, l2, PHP_MT_RAND_MAX);
#else
		RAND_RANGE(number, 0, l2, PHP_MT_RAND_MAX);
#endif
		result[j] = *(p2 + number);
	}
	RETURN_STRINGL(result, len + len2);
}
/* }}} */

static zend_string * _getMaskString(zend_string *string, zend_string *maskString, zend_long minLength)
{
	zend_long stringLength, mbStringLength, len, maskLength;
	zend_string *pResult;
	mbfl_string str, *prefix;
	smart_str buf = {0};

	stringLength = ZSTR_LEN(string);	// 内存长度
	// 获取字符串 mbfl 结构
	mbfl_string_init(&str);
	str.val = (unsigned char *) ZSTR_VAL(string);
	str.len = (uint32_t) stringLength;
#if PHP_VERSION_ID >= 70300
	str.encoding = mbfl_name2encoding("UTF8");
#else
	str.no_encoding = mbfl_name2no_encoding("UTF8");
#endif
	mbStringLength = mbfl_strlen(&str);	// mb 长度
	maskLength = mbStringLength > minLength ? mbStringLength : minLength;	// 掩码长度

	if (stringLength) {
		// 获取前缀长度
		zend_long prefixLength = (int) floor(mbStringLength / 2);
		if (prefixLength == 0) {
			prefixLength = 1;
		}
		// 截取前缀字符串
		mbfl_string dummy;
		prefix = mbfl_substr(&str, &dummy, 0, prefixLength);
		smart_str_appendl_ex(&buf, (char *) prefix->val, prefix->len, 0);
		mbfl_string_clear(prefix);
		// 更新掩码长度
		maskLength -= prefixLength;
	}
	// 生成掩码
	do {
		zend_long len = maskLength > ZSTR_LEN(maskString) ? ZSTR_LEN(maskString) : maskLength;	// 取小值
		smart_str_appendl_ex(&buf, ZSTR_VAL(maskString), len, 0);
		maskLength -= ZSTR_LEN(maskString);
	} while (maskLength > 0);
	smart_str_0(&buf);	// 字符串结尾
	pResult = zend_string_dup(buf.s, 0);
	smart_str_free(&buf);

	return pResult;
}

/* {{{ proto mask */
PHP_METHOD(azalea_text, mask)
{
	zend_long minLength = 3, type = 0;	// type:0 默认模式, type:1 电子邮箱模式
	zend_string *string, *mode = NULL, *maskString = NULL, *pResult, *newString, *tstr;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|SSl", &string, &mode, &maskString, &minLength) == FAILURE) {
		return;
	}

	// 模式
	if (mode && 0 == strcasecmp(ZSTR_VAL(mode), "email")) {
		type = 1;
	}
	// 默认掩码
	if (!maskString) {
		maskString = zend_string_init(ZEND_STRL("*"), 0);
	} else {
		zend_string_addref(maskString);
	}
	if (type == 0) {
		// 默认模式
		RETVAL_STR(_getMaskString(string, maskString, minLength));
	} else if (type == 1) {
		// 电子邮箱模式
		char *suffix = strrchr(ZSTR_VAL(string), '@');
		if (!suffix) {
			// 找不到 @ 后缀，默认处理
			RETVAL_STR(_getMaskString(string, maskString, minLength));
		} else {
			newString = zend_string_init(ZSTR_VAL(string), ZSTR_LEN(string) - strlen(suffix), 0);
			tstr = _getMaskString(newString, maskString, minLength);
			RETVAL_STR(strpprintf(0, "%s%s", ZSTR_VAL(tstr), suffix));
			zend_string_release(newString);
			zend_string_release(tstr);
		}
	}
	zend_string_release(maskString);
}
/* }}} */
