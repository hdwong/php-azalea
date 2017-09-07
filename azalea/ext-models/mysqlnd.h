/*
 * mysqlnd.h
 *
 * Created by Bun Wong on 17-9-3.
 */

#ifndef AZALEA_EXT_MODELS_MYSQLND_H_
#define AZALEA_EXT_MODELS_MYSQLND_H_

AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd);

PHP_METHOD(azalea_ext_model_mysqlnd, __init);
PHP_METHOD(azalea_ext_model_mysqlnd, escape);
PHP_METHOD(azalea_ext_model_mysqlnd, query);
PHP_METHOD(azalea_ext_model_mysqlnd, getQueries);
PHP_METHOD(azalea_ext_model_mysqlnd, getSqlBuilder);

PHP_METHOD(azalea_ext_model_mysqlnd_result, __construct);
PHP_METHOD(azalea_ext_model_mysqlnd_result, getSql);
PHP_METHOD(azalea_ext_model_mysqlnd_result, getError);
PHP_METHOD(azalea_ext_model_mysqlnd_result, getTimer);

PHP_METHOD(azalea_ext_model_mysqlnd_query, all);
PHP_METHOD(azalea_ext_model_mysqlnd_query, allWithKey);
PHP_METHOD(azalea_ext_model_mysqlnd_query, column);
PHP_METHOD(azalea_ext_model_mysqlnd_query, columnWithKey);
PHP_METHOD(azalea_ext_model_mysqlnd_query, row);
PHP_METHOD(azalea_ext_model_mysqlnd_query, field);
PHP_METHOD(azalea_ext_model_mysqlnd_query, fields);

PHP_METHOD(azalea_ext_model_mysqlnd_execute, insertId);
PHP_METHOD(azalea_ext_model_mysqlnd_execute, affected);
PHP_METHOD(azalea_ext_model_mysqlnd_execute, changed);

#ifdef WITH_SQLBUILDER
extern zend_class_entry * azaleaSqlBuilderGetCe(void);
extern zend_class_entry * azaleaSqlBuilderGetInterfaceCe(void);
extern void sqlBuilderEscapeEx(zval *return_value, zval *val, zend_bool escapeValue);
extern zend_string * sqlBuilderCompileBinds(zend_string *segment, zval *binds, zend_bool escapeValue);
#endif

extern zend_ulong mysqlnd_old_escape_string(char * newstr, const char * escapestr, size_t escapestr_len);

extern zend_class_entry *azalea_ext_model_mysqlnd_ce;

#endif /* AZALEA_EXT_MODELS_MYSQLND_H_ */
