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

zend_class_entry *azaleaI18nCe;

/* {{{ class Azalea\I18n methods */
static zend_function_entry azalea_i18n_methods[] = {
	PHP_ME(azalea_i18n, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
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
PHP_METHOD(azalea_i18n, setLocale)
{
	zend_string *locale;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &locale) == FAILURE) {
		return;
	}
}
/* }}} */

static void azaleaI18nLoadFile(zend_string *filename, zend_string *textDomain, zend_string *locale)
{
	return NULL
}

static zend_string * azaleaI18nTranslate(zend_string *message, zval *values, zend_string *textDomain, zend_string *locale)
{
	return NULL;
}

/* {{{ proto addTranslationFile */
PHP_METHOD(azalea_i18n, addTranslationFile)
{
	zend_string *filename, *textDomain = NULL, *locale = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|SS", &filename, &textDomain, &locale) == FAILURE) {
		return;
	}
}
/* }}} */

/* {{{ proto translate */
PHP_METHOD(azalea_i18n, translate)
{
	zend_string *message, *textDomain = NULL, *locale = NULL;
	zval *values;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sz|SS", &message, &values, &textDomain, &locale) == FAILURE) {
		return;
	}

	// TODO 检查当前 locale 是否已经存在于 AZALEA_G(translations)，不存在则 azaleaI18nLoadFile

	// TODO 存在 textDomain.message 则替换，否则用回传入的 message，最后进行类似 vsprintf(message, values)
}
/* }}} */

/* {{{ proto translatePlural */
PHP_METHOD(azalea_i18n, translatePlural)
{
	zend_string *messageSingular, *messagePlural, *textDomain = NULL, *locale = NULL;
	zval *values;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "SSz|SS", &messageSingular, &messagePlural, &values, &textDomain, &locale) == FAILURE) {
		return;
	}

	// TODO 检查当前 locale 配置中 _plural 的单数和复数条件，决定使用 messageSingular 或者 messagePlural

	// TODO values 必须存在 :num 键，否则默认 :num = 1，并使用 :num 来作为判断条件
}
/* }}} */
