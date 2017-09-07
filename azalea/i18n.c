/*
 * azalea/i18n.c
 *
 * Created by Bun Wong on 17-9-5.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/i18n.h"

#include "ext/standard/php_string.h"
#include "ext/standard/php_var.h"
#include "ext/json/php_json.h"  // for php_json_decode

zend_class_entry *azaleaI18nCe;

/* {{{ class Azalea\I18n methods */
static zend_function_entry azalea_i18n_methods[] = {
	PHP_ME(azalea_i18n, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_i18n, getLocale, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_i18n, setLocale, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_i18n, addTranslationFile, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_i18n, translate, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_i18n, translatePlural, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(i18n)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(I18n), azalea_i18n_methods);
	azaleaI18nCe = zend_register_internal_class(&ce);
	azaleaI18nCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_i18n, __construct) {}
/* }}} */

/* {{{ proto setLocale */
PHP_METHOD(azalea_i18n, getLocale)
{
	RETURN_STR_COPY(AZALEA_G(locale));
}
/* }}} */

/* {{{ proto setLocale */
PHP_METHOD(azalea_i18n, setLocale)
{
	zend_string *locale;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &locale) == FAILURE) {
		return;
	}

	zend_string_release(AZALEA_G(locale));
	AZALEA_G(locale) = zend_string_copy(locale);
}
/* }}} */

static void azaleaI18nAddToTranslation(zval *translation, zend_string *textDomain, zval *result)
{
	zend_string *key;
	zval *pData, *pTextDomain;

	if (textDomain == NULL) {
		// 根 domain 赋值
		// 获取默认 textDomain
		pTextDomain = zend_hash_find(Z_ARRVAL_P(translation), ZSTR_EMPTY_ALLOC());
		if (!pTextDomain) {
			zval dummy;
			pTextDomain = &dummy;
			array_init(pTextDomain);
			zend_hash_add(Z_ARRVAL_P(translation), ZSTR_EMPTY_ALLOC(), pTextDomain);
		}
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(result), key, pData) {
			if (!key) {
				continue;
			}
			if (Z_TYPE_P(pData) == IS_STRING) {
				if (0 == strcmp("_plural", ZSTR_VAL(key))) {
					// 复数设置
					zend_hash_add(Z_ARRVAL_P(translation), key, pData);
					zval_add_ref(pData);
					continue;
				}
				// 其它字符串加入默认 textDomain
				zend_hash_add(Z_ARRVAL_P(pTextDomain), key, pData);
				zval_add_ref(pData);
			} else if (Z_TYPE_P(pData) == IS_ARRAY) {
				azaleaI18nAddToTranslation(translation, key, pData);
			}
		} ZEND_HASH_FOREACH_END();
	} else {
		pTextDomain = zend_hash_find(Z_ARRVAL_P(translation), textDomain);
		if (!pTextDomain) {
			zval dummy;
			pTextDomain = &dummy;
			array_init(pTextDomain);
			zend_hash_add(Z_ARRVAL_P(translation), textDomain, pTextDomain);
		}
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(result), key, pData) {
			if (!key || Z_TYPE_P(pData) != IS_STRING) {
				continue;
			}
			zend_hash_add(Z_ARRVAL_P(pTextDomain), key, pData);
			zval_add_ref(pData);
		} ZEND_HASH_FOREACH_END();
	}
}

static zend_bool azaleaI18nLoadFile(zend_string *filename, zend_string *textDomain, zend_string *locale)
{
	php_stream *stream;
	zend_long maxlen = (ssize_t) PHP_STREAM_COPY_ALL;
	zval *translation, *pTextDomain, jsonResult;
	zend_string *contents;
	zend_bool returnValue = 0;

	translation = zend_hash_find(Z_ARRVAL(AZALEA_G(translations)), locale);
	if (!translation) {
		// 初始化成数组
		zval dummy;
		translation = &dummy;
		array_init(translation);
		zend_hash_add(Z_ARRVAL(AZALEA_G(translations)), locale, translation);
	}

	// 打开本地化翻译文件
	stream = php_stream_open_wrapper(ZSTR_VAL(filename), "rb", REPORT_ERRORS, NULL);
	if (!stream) {
		return 0;
	}
	if ((contents = php_stream_copy_to_mem(stream, maxlen, 0)) != NULL) {
		JSON_G(error_code) = PHP_JSON_ERROR_NONE;
		php_json_decode(&jsonResult, ZSTR_VAL(contents), ZSTR_LEN(contents), 1, 0);
		zend_string_release(contents);
		if (JSON_G(error_code) == PHP_JSON_ERROR_NONE && Z_TYPE(jsonResult) == IS_ARRAY) {
			// json 解析成功
			azaleaI18nAddToTranslation(translation, textDomain, &jsonResult);
			returnValue = 1;
		} else {
			php_error_docref(NULL, E_WARNING, "Load translation file `%s` is failed", ZSTR_VAL(filename));
		}
		zval_ptr_dtor(&jsonResult);
	}
	php_stream_close(stream);
	return returnValue;
}

static zend_bool azaleaI18nInit(zval **translation, zend_string *locale)
{
	// 检查当前 locale 是否已初始化到 AZALEA_G(translations)
	*translation = zend_hash_find(Z_ARRVAL(AZALEA_G(translations)), locale);
	if (!*translation) {
		// 未初始化则 azaleaI18nLoadFile
		if (AZALEA_G(appRoot)) {
			zend_string *filename;
			// 默认的本地化翻译文件名
			filename = strpprintf(0, "%slangs%c%s.json", ZSTR_VAL(AZALEA_G(appRoot)), DEFAULT_SLASH, ZSTR_VAL(locale));
			if (!(azaleaI18nLoadFile(filename, NULL, locale))) {	// 第一次加载 locale 文件 textDomain 传入 NULL 表示解析到根 domain
				zend_string_release(filename);
				return 0;
			}
			zend_string_release(filename);
		}
	} else {
		return 1;
	}
	// 重新获取
	*translation = zend_hash_find(Z_ARRVAL(AZALEA_G(translations)), locale);
	if (!*translation || Z_TYPE_P(*translation) != IS_ARRAY) {
		php_error_docref(NULL, E_ERROR, "Failed to get translation");
		return 0;
	}
	return 1;
}

static void azaleaI18nTranslateMessage(zval *return_value, zend_string *message, zval *values, zval *translation, zend_string *textDomain, zend_string *locale)
{
	zval *pTextDomain, *pMessage;

	if (translation && Z_TYPE_P(translation) == IS_ARRAY) {
		// 检查是否已存在 textDomain
		pTextDomain = zend_hash_find(Z_ARRVAL_P(translation), textDomain);
		if (pTextDomain && Z_TYPE_P(pTextDomain) == IS_ARRAY) {
			// 检查是否找到本地化
			if ((pMessage = zend_hash_find(Z_ARRVAL_P(pTextDomain), message)) &&
					Z_TYPE_P(pMessage) == IS_STRING) {
				message = Z_STR_P(pMessage);
			}
		}
	}

	// 进行展位符替换
	if (values && Z_TYPE_P(values) == IS_ARRAY) {
		// 调用 strtr 方法来替换
		zval callFuncName, callFuncParams[2], *pData;
		zend_string *key;

		ZVAL_STRINGL(&callFuncName, "strtr", sizeof("strtr") - 1);
		ZVAL_STR(callFuncParams, message);
		array_init(&callFuncParams[1]);
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(values), key, pData) {
			if (!key || (Z_TYPE_P(pData) != IS_LONG &&
					Z_TYPE(pData) != IS_DOUBLE && Z_TYPE_P(pData) != IS_STRING)) {
				continue;
			}
			key = strpprintf(0, ":%s", ZSTR_VAL(key));
			zend_hash_add(Z_ARRVAL(callFuncParams[1]), key, pData);
			zend_string_release(key);
		} ZEND_HASH_FOREACH_END();

		call_user_function(EG(function_table), NULL, &callFuncName, return_value, 2, callFuncParams);
		zval_ptr_dtor(&callFuncParams[1]);
		zval_ptr_dtor(&callFuncName);
	} else {
		RETURN_STR_COPY(message);
	}
}

/* {{{ proto addTranslationFile */
PHP_METHOD(azalea_i18n, addTranslationFile)
{
	zend_string *filename, *textDomain = NULL, *locale = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|SS", &filename, &textDomain, &locale) == FAILURE) {
		return;
	}
	if (textDomain == NULL) {
		textDomain = ZSTR_EMPTY_ALLOC();
	}
	if (locale == NULL) {
		locale = AZALEA_G(locale);
	}
}
/* }}} */

/* {{{ proto translate */
PHP_METHOD(azalea_i18n, translate)
{
	azaleaI18nTranslate(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

void azaleaI18nTranslate(INTERNAL_FUNCTION_PARAMETERS)
{
	zend_string *message, *textDomain = NULL, *locale = NULL;
	zval *translation, *values = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|zSS", &message, &values, &textDomain, &locale) == FAILURE) {
		return;
	}
	if (textDomain == NULL) {
		textDomain = ZSTR_EMPTY_ALLOC();
	}
	if (locale == NULL) {
		locale = AZALEA_G(locale);
	}
	if (!azaleaI18nInit(&translation, locale)) {
		translation = NULL;
	}

	azaleaI18nTranslateMessage(return_value, message, values, translation, textDomain, locale);
}
/* }}} */

static zend_bool azaleaI18nCheckPlural(zval *translation, zval *values, zend_string *locale)
{
	zval *plural = NULL, *pNum;
	zend_long num;
	char *pPlural;

	if (!translation || Z_TYPE_P(translation) != IS_ARRAY ||
			!values || Z_TYPE_P(values) != IS_ARRAY) {
		return 0;
	}
	if (!(pNum = zend_hash_str_find(Z_ARRVAL_P(values), ZEND_STRL("num"))) ||
			(Z_TYPE_P(pNum) != IS_LONG && Z_TYPE_P(pNum) != IS_DOUBLE && Z_TYPE_P(pNum) != IS_STRING)) {
		//	找不到数量设置
		return 0;
	}
	if (!(plural = zend_hash_str_find(Z_ARRVAL_P(translation), ZEND_STRL("_plural"))) ||
			Z_TYPE_P(pNum) != IS_STRING) {
		// 找不到复数设置
		if (0 == strcmp(ZSTR_VAL(locale), ZSTR_VAL(AG(stringEn)))) {
			// 默认 en_US 时为 "1" 模式
			pPlural = "1";
		} else {
			return 0;
		}
	} else {
		pPlural = Z_STRVAL_P(plural);
	}
	// values 必须存在 num 键，并使用 num 来作为判断条件
	switch (Z_TYPE_P(pNum)) {
		case IS_LONG:
			num = Z_LVAL_P(pNum);
			break;
		case IS_DOUBLE:
			num = (zend_long) Z_DVAL_P(pNum);
			break;
		case IS_STRING:
			num = ZEND_STRTOL(Z_STRVAL_P(pNum), NULL, 10);
			break;
	}
	// "1" 模式 只有 1 是单数，其它是复数
	// "0" 模式 只有 0 是单数，其它是复数
	// "01" 模式 0 或 1 是单数，其它是复数
	if (0 == strcmp(pPlural, "1") && num == 1) {
		return 0;
	} else if (0 == strcmp(pPlural, "0")&& num == 0) {
		return 0;
	} else if (0 == strcmp(pPlural, "01") && (num == 0 || num == 1)) {
		return 0;
	}
	return 1;
}

/* {{{ proto translatePlural */
PHP_METHOD(azalea_i18n, translatePlural)
{
	azaleaI18nTranslatePlural(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

void azaleaI18nTranslatePlural(INTERNAL_FUNCTION_PARAMETERS)
{
	zend_string *messageSingular, *messagePlural, *textDomain = NULL, *locale = NULL;
	zval *translation, *values = NULL;
	zend_bool isPlural;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "SS|zSS", &messageSingular, &messagePlural, &values, &textDomain, &locale) == FAILURE) {
		return;
	}
	if (textDomain == NULL) {
		textDomain = ZSTR_EMPTY_ALLOC();
	}
	if (locale == NULL) {
		locale = AZALEA_G(locale);
	}
	if (!azaleaI18nInit(&translation, locale)) {
		translation = NULL;
	}

	// 检查当前 locale 配置中 _plural 的单数和复数条件，决定使用 messageSingular 或者 messagePlural
	isPlural = azaleaI18nCheckPlural(translation, values, locale);
	azaleaI18nTranslateMessage(return_value, isPlural ? messagePlural : messageSingular, values, translation, textDomain, locale);
}
/* }}} */
