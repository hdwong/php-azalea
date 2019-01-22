/*
 * mysqlnd.h
 *
 * Created by Bun Wong on 17-9-3.
 */

#ifndef AZALEA_EXT_MODELS_MYSQLND_H_
#define AZALEA_EXT_MODELS_MYSQLND_H_

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#define AZALEA_MYSQLND_FETCH_RESOURCE_CONN(__ptr, __this) \
{ \
	zval *res = zend_read_property(azalea_ext_model_mysqlnd_ce, __this, "_connection", sizeof("_connection") - 1, 1, NULL); \
	if (!res || Z_TYPE_P(res) != IS_RESOURCE || !(__ptr = (MYSQLND *)Z_RES_VAL_P(res))) { \
		php_error_docref(NULL, E_ERROR, "Invalid Mysql connect"); \
	} \
}
#define AZALEA_MYSQLND_FETCH_RESOURCE_QR(__ptr, __this) \
{ \
	zval *res = zend_read_property(mysqlndResultCe, __this, "_result", sizeof("_result") - 1, 1, NULL); \
	if (!res || Z_TYPE_P(res) != IS_RESOURCE || !(__ptr = (MYSQLND_RES *)Z_RES_VAL_P(res))) { \
		php_error_docref(NULL, E_ERROR, "Invalid Mysql query result"); \
	} \
}
#define MYSQLND_RETURN_LONG(__val) \
{ \
	if ((__val) < ZEND_LONG_MAX) { \
		RETURN_LONG((zend_long) (__val)); \
	} else { \
		RETURN_STR(strpprintf(0, "%llu", (__val))); \
	} \
}

AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd);

PHP_METHOD(azalea_ext_model_mysqlnd, __init);
PHP_METHOD(azalea_ext_model_mysqlnd, escape);
PHP_METHOD(azalea_ext_model_mysqlnd, escapeLike);
PHP_METHOD(azalea_ext_model_mysqlnd, query);
PHP_METHOD(azalea_ext_model_mysqlnd, getQueries);
PHP_METHOD(azalea_ext_model_mysqlnd, getSqlBuilder);

PHP_METHOD(azalea_ext_model_mysqlnd_result, __construct);
PHP_METHOD(azalea_ext_model_mysqlnd_result, getSql);
PHP_METHOD(azalea_ext_model_mysqlnd_result, getTimer);

PHP_METHOD(azalea_ext_model_mysqlnd_query, count);
PHP_METHOD(azalea_ext_model_mysqlnd_query, all);
PHP_METHOD(azalea_ext_model_mysqlnd_query, allWithKey);
PHP_METHOD(azalea_ext_model_mysqlnd_query, column);
PHP_METHOD(azalea_ext_model_mysqlnd_query, columnWithKey);
PHP_METHOD(azalea_ext_model_mysqlnd_query, row);
PHP_METHOD(azalea_ext_model_mysqlnd_query, field);
PHP_METHOD(azalea_ext_model_mysqlnd_query, fields);

PHP_METHOD(azalea_ext_model_mysqlnd_execute, insertId);
PHP_METHOD(azalea_ext_model_mysqlnd_execute, affectedRows);

#ifdef WITH_SQLBUILDER
extern zend_class_entry * azaleaSqlBuilderGetCe(void);
extern zend_class_entry * azaleaSqlBuilderGetInterfaceCe(void);
extern void sqlBuilderEscapeEx(zval *return_value, zval *val, zend_bool escapeValue, zend_bool escapeLike);
extern zend_string * sqlBuilderCompileBinds(zend_string *segment, zval *binds, zend_bool escapeValue);
#endif

extern zend_ulong mysqlnd_old_escape_string(char * newstr, const char * escapestr, size_t escapestr_len);

extern zend_class_entry *azalea_ext_model_mysqlnd_ce;

#endif /* AZALEA_EXT_MODELS_MYSQLND_H_ */
