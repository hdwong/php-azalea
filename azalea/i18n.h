/*
 * azalea/i18n.h
 *
 * Created by Bun Wong on 17-9-5.
 */

#ifndef AZALEA_I18N_H_
#define AZALEA_I18N_H_

AZALEA_STARTUP_FUNCTION(i18n);

PHP_METHOD(azalea_i18n, __construct);
PHP_METHOD(azalea_i18n, getLocale);
PHP_METHOD(azalea_i18n, setLocale);
PHP_METHOD(azalea_i18n, addTranslationFile);
PHP_METHOD(azalea_i18n, translate);
PHP_METHOD(azalea_i18n, translatePlural);

void azaleaI18nTranslate(zval *return_value, zend_string *message, zval *values, zend_string *textDomain, zend_string *locale);
zend_bool azaleaI18nTranslateZval(zval *return_value, zval *zval);
void azaleaI18nTranslateFunction(INTERNAL_FUNCTION_PARAMETERS);
void azaleaI18nTranslatePluralFunction(INTERNAL_FUNCTION_PARAMETERS);

extern zend_class_entry *azaleaI18nCe;

#endif /* AZALEA_I18N_H_ */
