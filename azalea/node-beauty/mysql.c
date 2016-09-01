/*
 * mysql.c
 *
 * Created by Bun Wong on 16-8-24.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/mysql.h"
#include "azalea/node-beauty/mysql_sqlbuilder.h"

#include "Zend/zend_hash.h"  // for php_array
#include "Zend/zend_exceptions.h"  // for zend_clear_exception
#include "Zend/zend_smart_str.h"  // for smart_str_*
#include "ext/standard/php_var.h"
#include "ext/standard/php_string.h"  // for php_strtr
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_mysql_ce;
zend_class_entry *mysqlResultCe;
zend_class_entry *mysqlQueryResultCe;
zend_class_entry *mysqlExecuteResultCe;

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_node_beauty_mysql_methods[] = {
	PHP_ME(azalea_node_beauty_mysql, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_mysql, __init, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_mysql, escape, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, query, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, getQueries, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, insert, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, replace, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, update, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, delete, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql, getSqlBuilder, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_node_beauty_mysql_result_methods[] = {
	PHP_ME(azalea_node_beauty_mysql_result, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_mysql_result, getSql, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_result, getError, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_result, getTimer, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_node_beauty_mysql_query_methods[] = {
	PHP_ME(azalea_node_beauty_mysql_query, all, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, allWithKey, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, column, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, columnWithKey, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, row, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, field, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_query, fields, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_node_beauty_mysql_execute_methods[] = {
	PHP_ME(azalea_node_beauty_mysql_execute, insertId, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_execute, affected, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_execute, changed, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(mysql)
{
	zend_class_entry ce, resultCe, queryResultCe, executeResultCe;

	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(MysqlModel), azalea_node_beauty_mysql_methods);
	azalea_node_beauty_mysql_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_mysql_ce->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azalea_node_beauty_mysql_ce, ZEND_STRL("_queries"), ZEND_ACC_PRIVATE);

	INIT_CLASS_ENTRY(resultCe, AZALEA_NS_NAME(MysqlResult), azalea_node_beauty_mysql_result_methods);
	mysqlResultCe = zend_register_internal_class(&resultCe);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_error"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_sql"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_timer"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_result"), ZEND_ACC_PRIVATE);

	INIT_CLASS_ENTRY(queryResultCe, AZALEA_NS_NAME(MysqlQueryResult), azalea_node_beauty_mysql_query_methods);
	mysqlQueryResultCe = zend_register_internal_class_ex(&queryResultCe, mysqlResultCe);

	INIT_CLASS_ENTRY(executeResultCe, AZALEA_NS_NAME(MysqlExecuteResult), azalea_node_beauty_mysql_execute_methods);
	mysqlExecuteResultCe = zend_register_internal_class_ex(&executeResultCe, mysqlResultCe);

	mysqlSqlBuilderStartup();

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_mysql, __construct) {}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_mysql, __init)
{
	zval queries;
	array_init(&queries);
	zend_update_property(azalea_node_beauty_mysql_ce, getThis(), ZEND_STRL("_queries"), &queries);
	zval_ptr_dtor(&queries);
}
/* }}} */

/* {{{ proto query */
PHP_METHOD(azalea_node_beauty_mysql_result, __construct) {}
/* }}} */

/* {{{ proto throwEmptyExecuteResult */
static zval * throwEmptyExecuteResult(const char *error, size_t len)
{
	zval rv = {{0}}, *instance = &rv;
	object_init_ex(instance, mysqlExecuteResultCe);
	if (error) {
		zend_update_property_stringl(mysqlResultCe, instance, ZEND_STRL("_error"), error, len);
	}
	return instance;
}
/* }}} */

/* {{{ proto mysqlEscapeStr */
zend_string * mysqlEscapeStr(zend_string *val)
{
	char *result, *pResult, *p = ZSTR_VAL(val);
	size_t len = 0;
	result = ecalloc(sizeof(char), ZSTR_LEN(val) * 2);
	pResult = result;
	while (*p) {
		if (*p == '\\' || *p == '"' || *p == '\'') {
			*pResult++ = '\\';
			*pResult++ = *p;
			len += 2;
		} else if (*p == '\0') {
			*pResult++ = '\\';
			*pResult++ = '0';
			len += 2;
		} else if (*p == '\r') {
			*pResult++ = '\\';
			*pResult++ = 'r';
			len += 2;
		} else if (*p == '\n') {
			*pResult++ = '\\';
			*pResult++ = 'n';
			len += 2;
		} else {
			*pResult++ = *p;
			++len;
		}
		++p;
	}
	zend_string *ret;
	if (len == ZSTR_LEN(val)) {
		ret = zend_string_copy(val);
	} else {
		ret = zend_string_init(result, len, 0);
	}
	efree(result);
	return ret;
}
/* }}} */

/* {{{ proto mysqlEscape */
void mysqlEscapeEx(zval *return_value, zval *val, zend_bool escapeValue)
{
	switch (Z_TYPE_P(val)) {
		case IS_ARRAY:
			array_init(return_value);
			zend_ulong h;
			zend_string *key;
			zval *pData;
			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(val), h, key, pData) {
				{
					zval ret;
					mysqlEscapeEx(&ret, pData, escapeValue);
					if (Z_TYPE(ret) == IS_STRING || Z_TYPE(ret) == IS_ARRAY || Z_TYPE(ret) == IS_NULL) {
						if (key) {
							key = mysqlEscapeStr(key);
							add_assoc_zval_ex(return_value, ZSTR_VAL(key), ZSTR_LEN(key), &ret);
							zend_string_release(key);
						} else {
							add_index_zval(return_value, h, &ret);
						}
					} else {
						php_error_docref(NULL, E_ERROR, "expects parameter 2 to be a valid value");
						RETURN_FALSE;
					}
				}
			} ZEND_HASH_FOREACH_END();
			return;
		case IS_STRING:
			if (escapeValue) {
				RETURN_STR(mysqlEscapeStr(Z_STR_P(val)));
			} else {
				RETURN_ZVAL(val, 1, 0);
			}
			return;
		case IS_LONG:
		case IS_DOUBLE:
			RETVAL_ZVAL(val, 1, 0);
			convert_to_string(return_value);
			return;
		case IS_TRUE:
			RETURN_STRINGL("1", 1);
		case IS_FALSE:
			RETURN_STRINGL("0", 1);
		case IS_NULL:
			RETURN_NULL();
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto mysqlKeyword */
zend_string * mysqlKeyword(zend_string *str)
{
	zend_string *ret;
	char *p = strchr(ZSTR_VAL(str), '.');
	if (p) {
		size_t len = p - ZSTR_VAL(str);
		if (len) {
			char *prefix = emalloc(len + 1);
			memcpy(prefix, ZSTR_VAL(str), len);
			*(prefix + len) = '\0';
			ret = strpprintf(0, "`%s`.`%s`", prefix, p + 1);
			efree(prefix);
		} else {
			ret = strpprintf(0, "`%s`", p + 1);
		}
	} else {
		ret = strpprintf(0, "`%s`", ZSTR_VAL(str));
	}
	return ret;
}
/* }}} */

/* {{{ proto mysqlCompileBinds */
zend_string * mysqlCompileBinds(zend_string *sql, zval *binds, zend_bool escapeValue)
{
	zval args;
	mysqlEscapeEx(&args, binds, escapeValue);
	if (Z_TYPE(args) == IS_FALSE) {
		zval_ptr_dtor(&args);
		return zend_string_init(ZSTR_VAL(sql), ZSTR_LEN(sql), 0);
	}
	char *p = ZSTR_VAL(sql), *pos;
	smart_str buf = {0};
	zend_ulong h;
	zend_string *key, *tstr;
	zval *pData, inString, *pInString = NULL;
	char *value;
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(args), h, key, pData) {
		pos = strchr(p, '?');
		if (pos) {
			// found
			smart_str_appendl(&buf, p, pos - p);
		} else {
			break;
		}
		if (*(pos + 1) == '?') {
			// keyword escape
			if (escapeValue) {
				tstr = mysqlKeyword(Z_STR_P(pData));
			} else {
				// escape keyword field
				zend_string *field = mysqlEscapeStr(Z_STR_P(pData));
				tstr = mysqlKeyword(field);
				zend_string_release(field);
			}
			p = pos + 2;
		} else {
			// value escape
			if (Z_TYPE_P(pData) == IS_ARRAY) {
				zend_string *delim;
				if (escapeValue) {
					delim = zend_string_init(ZEND_STRL("\",\""), 0);
				} else {
					delim = zend_string_init(ZEND_STRL(","), 0);
				}
				php_implode(delim, pData, &inString);
				zend_string_release(delim);
				value = Z_STRVAL(inString);
				pInString = &inString;
			} else if (Z_TYPE_P(pData) == IS_STRING) {
				value = Z_STRVAL_P(pData);
			} else {
				value = NULL;
			}
			if (key) {
				key = mysqlKeyword(key);
				if (pInString) {
					if (escapeValue) {
						tstr = strpprintf(0, "%s IN (\"%s\")", ZSTR_VAL(key), value);
					} else {
						tstr = strpprintf(0, "%s IN (%s)", ZSTR_VAL(key), value);
					}
				} else if (value) {
					if (escapeValue) {
						tstr = strpprintf(0, "%s = \"%s\"", ZSTR_VAL(key), value);
					} else {
						tstr = strpprintf(0, "%s = %s", ZSTR_VAL(key), value);
					}
				} else {
					tstr = strpprintf(0, "%s IS NULL", ZSTR_VAL(key));
				}
				zend_string_release(key);
			} else {
				if (pInString) {
					if (escapeValue) {
						tstr = strpprintf(0, "(\"%s\")", value);
					} else {
						tstr = strpprintf(0, "(%s)", value);
					}
				} else if (value) {
					if (escapeValue) {
						tstr = strpprintf(0, "\"%s\"", value);
					} else {
						tstr = zend_string_copy(Z_STR_P(pData));
					}
				} else {
					tstr = zend_string_init(ZEND_STRL("NULL"), 0);
				}
			}
			if (pInString) {
				zval_ptr_dtor(pInString);
				pInString = NULL;
			}
			p = pos + 1;
		}
		smart_str_append(&buf, tstr);
		zend_string_release(tstr);
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(&args);

	if (p - ZSTR_VAL(sql) < ZSTR_LEN(sql)) {
		smart_str_appendl(&buf, p, ZSTR_LEN(sql) - (p - ZSTR_VAL(sql)));
	}
	smart_str_0(&buf);
	zend_string *ret = zend_string_copy(buf.s);
	smart_str_free(&buf);
	return ret;
}
/* }}} */

/* {{{ proto mysqlCompileKeyValues */
static void mysqlCompileKeyValues(zval *ret, zval *set)
{
	zval escaped, newArray, *pSet = &escaped, keys, values, row, *pRow;
	// escape set
	mysqlEscape(pSet, set);
	// check first element if array for bulk-inserts
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(pSet));
	pRow = zend_hash_get_current_data(Z_ARRVAL_P(pSet));
	if (Z_TYPE_P(pRow) != IS_ARRAY) {
		array_init(&newArray);
		add_next_index_zval(&newArray, pSet);
		pSet = &newArray;
	}
	// foreach
	array_init(&keys);
	array_init(&values);
	zend_ulong i = 0;
	zend_string *key;
	zval *pData;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(pSet), pRow) {
		array_init(&row);
		if (Z_TYPE_P(pRow) == IS_ARRAY) {
			ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(pRow), key, pData) {
				if (!key) {
					continue;
				}
				if (0 == i) {
					// first row to save keys
					add_next_index_str(&keys, mysqlKeyword(key));
				}
				add_next_index_zval(&row, pData);
				zval_add_ref(pData);
			} ZEND_HASH_FOREACH_END();
		}
		zend_string *delim = zend_string_init(ZEND_STRL("\",\""), 0);
		zval value;
		php_implode(delim, &row, &value);
		add_next_index_str(&values, strpprintf(0, "(\"%s\")", Z_STRVAL(value)));
		zend_string_release(delim);
		zval_ptr_dtor(&value);
		zval_ptr_dtor(&row);
		++i;
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(pSet);

	// build ret
	array_init(ret);
	add_next_index_zval(ret, &keys);
	add_next_index_zval(ret, &values);
}
/* }}} */

/* {{{ proto escape */
PHP_METHOD(azalea_node_beauty_mysql, escape)
{
	zval *str, *escapeResult;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &str) == FAILURE) {
		return;
	}

	mysqlEscape(return_value, str);
}
/* }}} */

/* {{{ proto mysqlQuery */
void mysqlQuery(zval *serviceInstance, zval *return_value, zend_string *sql, zend_bool throwsException)
{
	zval ret, arg1, arg2, *value, rv = {{0}}, *instance = &rv;

	ZVAL_STRINGL(&arg1, "query", sizeof("query") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("sql"), zend_string_copy(sql));
	zend_call_method_with_2_params(serviceInstance, azalea_service_ce, NULL, "post", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);

	// has exception
	if (EG(exception)) {
		if (throwsException) {
			RETVAL_FALSE;
		} else {
			// catch and ignore exception
			zval ex, *error;
			ZVAL_OBJ(&ex, EG(exception));
			EG(exception) = NULL;
			object_init_ex(instance, mysqlResultCe);
			zend_update_property_str(mysqlResultCe, instance, ZEND_STRL("_sql"), sql);
			error = zend_read_property(zend_ce_exception, &ex, ZEND_STRL("message"), 1, NULL);
			if (error) {
				zend_update_property(mysqlResultCe, instance, ZEND_STRL("_error"), error);
			}
			RETVAL_ZVAL(instance, 0, 0);
			zval_ptr_dtor(&ex);
			zend_clear_exception();
		}
		zval_ptr_dtor(&arg2);
		zval_ptr_dtor(&ret);
		return;
	}

	if (Z_TYPE(ret) == IS_OBJECT && (value = zend_read_property(NULL, &ret, ZEND_STRL("result"), 1, NULL))) {
		if (Z_TYPE_P(value) == IS_ARRAY || Z_TYPE_P(value) == IS_OBJECT) {
			// record sql
			zval *queries = zend_read_property(azalea_node_beauty_mysql_ce, serviceInstance, ZEND_STRL("_queries"), 1, NULL);
			if (queries && Z_TYPE_P(queries) == IS_ARRAY) {
				add_next_index_str(queries, zend_string_copy(sql));
			}

			if (Z_TYPE_P(value) == IS_ARRAY) {
				// for DQL, return MysqlQueryResult object
				object_init_ex(instance, mysqlQueryResultCe);
			} else {
				// for DDL/DML, return MysqlExecuteResult object
				object_init_ex(instance, mysqlExecuteResultCe);
			}
			zend_update_property(mysqlResultCe, instance, ZEND_STRL("_result"), value);
			value = zend_read_property(NULL, &ret, ZEND_STRL("sql"), 1, NULL);
			zend_update_property_str(mysqlResultCe, instance, ZEND_STRL("_sql"),
					Z_TYPE_P(value) == IS_STRING ? Z_STR_P(value) : sql);
			value = zend_read_property(NULL, &ret, ZEND_STRL("timer"), 1, NULL);
			if (Z_TYPE_P(value) == IS_LONG) {
				zend_update_property(mysqlResultCe, instance, ZEND_STRL("_timer"), value);
			}
			RETVAL_ZVAL(instance, 0, 0);
		} else {
			// error
			RETVAL_FALSE;
		}
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&arg2);  // no need to release *sql
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto query */
PHP_METHOD(azalea_node_beauty_mysql, query)
{
	zend_string *sql;
	zval *binds = NULL;
	zend_bool throwsException = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|zb", &sql, &binds, &throwsException) == FAILURE) {
		return;
	}

	if (binds && Z_TYPE_P(binds) != IS_ARRAY && Z_TYPE_P(binds) != IS_NULL) {
		php_error_docref(NULL, E_WARNING, "expects parameter 2 to be array");
		binds = NULL;
	}
	if (binds && strchr(ZSTR_VAL(sql), '?') &&
			Z_TYPE_P(binds) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(binds)) > 0) {
		sql = mysqlCompileBinds(sql, binds, 1);
	} else {
		sql = zend_string_init(ZSTR_VAL(sql), ZSTR_LEN(sql), 0);
	}

	mysqlQuery(getThis(), return_value, sql, throwsException);

	zend_string_release(sql);
}
/* }}} */

/* {{{ proto getQueries */
PHP_METHOD(azalea_node_beauty_mysql, getQueries)
{
	zval *queries = zend_read_property(azalea_node_beauty_mysql_ce, getThis(), ZEND_STRL("_queries"), 1, NULL);
	if (queries && Z_TYPE_P(queries) == IS_ARRAY) {
		RETURN_ZVAL(queries, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto insert */
PHP_METHOD(azalea_node_beauty_mysql, insert)
{
	zend_string *tableName;
	zval *set, keyValues;
	zend_bool ignoreErrors = 0, duplicateKeyUpdate = 0;
	zend_bool throwsException = 1;
	zval *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sa|bbb", &tableName, &set, &ignoreErrors, &duplicateKeyUpdate, &throwsException) == FAILURE) {
		return;
	}

	// new ExecuteResult instance
	if (zend_hash_num_elements(Z_ARRVAL_P(set)) == 0) {
		// field set is empty
		RETURN_ZVAL(throwEmptyExecuteResult(ZEND_STRL("Field set is empty")), 0, 0);
	}
	mysqlCompileKeyValues(&keyValues, set);

	zval *keys, *values;
	keys = zend_hash_index_find(Z_ARRVAL(keyValues), 0);
	values = zend_hash_index_find(Z_ARRVAL(keyValues), 1);
	if (zend_hash_num_elements(Z_ARRVAL_P(keys)) == 0) {
		// fieldName is empty
		RETURN_ZVAL(throwEmptyExecuteResult(ZEND_STRL("Field name is empty")), 0, 0);
	}

	// build sql
	zend_string *sql, *delim, *strDuplicateKeyUpdate = NULL, *tstr;
	zval keysString, valuesString;

	tstr = mysqlEscapeStr(tableName);
	tableName = mysqlKeyword(tstr);
	zend_string_release(tstr);
	delim = zend_string_init(ZEND_STRL(","), 0);
	php_implode(delim, keys, &keysString);
	php_implode(delim, values, &valuesString);
	zend_string_release(delim);
	if (duplicateKeyUpdate) {
		smart_str buf = {0};
		smart_str_appendl_ex(&buf, ZEND_STRL(" ON DUPLICATE KEY UPDATE "), 0);
		zval *key;
		zend_ulong i = 0;
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(keys), key) {
			if (i++ > 0) {
				smart_str_appendc(&buf, ',');
			}
			tstr = strpprintf(0, "%s = VALUES(%s)", Z_STRVAL_P(key), Z_STRVAL_P(key));
			smart_str_append(&buf, tstr);
			zend_string_release(tstr);
		} ZEND_HASH_FOREACH_END();
		smart_str_0(&buf);
		strDuplicateKeyUpdate = zend_string_copy(buf.s);
		smart_str_free(&buf);
	}

	sql = strpprintf(0, "INSERT %sINTO %s (%s) VALUES %s%s", ignoreErrors ? "IGNORE " : "",
			ZSTR_VAL(tableName), Z_STRVAL(keysString), Z_STRVAL(valuesString),
			strDuplicateKeyUpdate ? ZSTR_VAL(strDuplicateKeyUpdate) : "");

	// execute query
	mysqlQuery(instance, return_value, sql, throwsException);

	zend_string_release(sql);
	zend_string_release(tableName);
	if (strDuplicateKeyUpdate) {
		zend_string_release(strDuplicateKeyUpdate);
	}
	zval_ptr_dtor(&keysString);
	zval_ptr_dtor(&valuesString);
	zval_ptr_dtor(&keyValues);
}
/* }}} */

/* {{{ proto replace */
PHP_METHOD(azalea_node_beauty_mysql, replace)
{
	zend_string *tableName;
	zval *set, keyValues;
	zend_bool throwsException = 1;
	zval *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sa|b", &tableName, &set, &throwsException) == FAILURE) {
		return;
	}

	// new ExecuteResult instance
	if (zend_hash_num_elements(Z_ARRVAL_P(set)) == 0) {
		// field set is empty
		RETURN_ZVAL(throwEmptyExecuteResult(ZEND_STRL("Field set is empty")), 0, 0);
	}
	mysqlCompileKeyValues(&keyValues, set);

	zval *keys, *values;
	keys = zend_hash_index_find(Z_ARRVAL(keyValues), 0);
	values = zend_hash_index_find(Z_ARRVAL(keyValues), 1);
	if (zend_hash_num_elements(Z_ARRVAL_P(keys)) == 0) {
		// fieldName is empty
		RETURN_ZVAL(throwEmptyExecuteResult(ZEND_STRL("Field name is empty")), 0, 0);
	}

	// build sql
	zend_string *sql, *delim, *tstr;
	zval keysString, valuesString;

	tstr = mysqlEscapeStr(tableName);
	tableName = mysqlKeyword(tstr);
	zend_string_release(tstr);
	delim = zend_string_init(ZEND_STRL(","), 0);
	php_implode(delim, keys, &keysString);
	php_implode(delim, values, &valuesString);
	zend_string_release(delim);

	sql = strpprintf(0, "REPLACE INTO %s (%s) VALUES %s", ZSTR_VAL(tableName), Z_STRVAL(keysString), Z_STRVAL(valuesString));

	// execute query
	mysqlQuery(instance, return_value, sql, throwsException);

	zend_string_release(sql);
	zend_string_release(tableName);
	zval_ptr_dtor(&keysString);
	zval_ptr_dtor(&valuesString);
	zval_ptr_dtor(&keyValues);
}
/* }}} */

/* {{{ proto mysqlCompileUpdateWhere */
static zend_string * mysqlCompileUpdateWhere(zval *instance, zval *where)
{
	if (!where || Z_TYPE_P(where) != IS_ARRAY) {
		return NULL;
	}
	// new and init mysqlSqlBuilder
	zval rv = {{0}}, *sqlBuilder = &rv;
	object_init_ex(sqlBuilder, mysqlSqlBuilderCe);
	if (zend_hash_str_exists(&(mysqlSqlBuilderCe->function_table), ZEND_STRL("__construct"))) {
		zend_call_method_with_1_params(&rv, mysqlSqlBuilderCe, NULL, "__construct", NULL, instance);
	}
	zend_string *type, *whereSql;
	type = zend_string_init(ZEND_STRL("AND"), 0);
	mysqlWhere(sqlBuilder, RECKEY_WHERE, where, NULL, type, 1);
	zend_string_release(type);
	whereSql = mysqlCompileWhere(sqlBuilder, RECKEY_WHERE);
	zval_ptr_dtor(sqlBuilder);
	if (!whereSql) {
		return NULL;
	}
	if (!ZSTR_LEN(whereSql)) {
		zend_string_release(whereSql);
		return NULL;
	}
	return whereSql;

}
/* }}} */

/* {{{ proto update */
PHP_METHOD(azalea_node_beauty_mysql, update)
{
	zend_string *tableName, *key, *tstr, *sql;
	zval *set, *where = NULL, *instance = getThis();
	zend_bool throwsException = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sa|zb", &tableName, &set, &where, &throwsException) == FAILURE) {
		return;
	}

	// new ExecuteResult instance
	if (zend_hash_num_elements(Z_ARRVAL_P(set)) == 0) {
		// field set is empty
		RETURN_ZVAL(throwEmptyExecuteResult(ZEND_STRL("Field set is empty")), 0, 0);
	}
	// compileSet
	zval setValues, setString, *pData;
	zend_string *delim;
	array_init(&setValues);
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(set), key, pData) {
		if (!key) {
			continue;
		}
		zval value;
		tstr = mysqlEscapeStr(key);
		key = mysqlKeyword(tstr);
		zend_string_release(tstr);
		if (Z_TYPE_P(pData) == IS_ARRAY) {
			// escape & value
			zval *escape, *pValue;
			if (!(pValue = zend_hash_str_find(Z_ARRVAL_P(pData), ZEND_STRL("value")))) {
				continue;
			}
			if ((escape = zend_hash_str_find(Z_ARRVAL_P(pData), ZEND_STRL("escape")))) {
				convert_to_boolean(escape);
			}
			mysqlEscapeEx(&value, pValue, !escape || Z_TYPE_P(escape) == IS_TRUE);
			tstr = strpprintf(0, "%s = %s", ZSTR_VAL(key), Z_STRVAL(value));
		} else {
			// string
			mysqlEscape(&value, pData);
			tstr = strpprintf(0, "%s = \"%s\"", ZSTR_VAL(key), Z_STRVAL(value));
		}
		add_next_index_str(&setValues, tstr);
		zend_string_release(key);
		zval_ptr_dtor(&value);
	} ZEND_HASH_FOREACH_END();
	delim = zend_string_init(ZEND_STRL(","), 0);
	php_implode(delim, &setValues, &setString);
	zend_string_release(delim);
	zval_ptr_dtor(&setValues);

	// get sql
	tstr = mysqlEscapeStr(tableName);
	tableName = mysqlKeyword(tstr);
	zend_string_release(tstr);
	tstr = mysqlCompileUpdateWhere(instance, where);
	if (tstr) {
		sql = strpprintf(0, "UPDATE %s SET %s WHERE %s", ZSTR_VAL(tableName), Z_STRVAL(setString), ZSTR_VAL(tstr));
		zend_string_release(tstr);
	} else {
		sql = strpprintf(0, "UPDATE %s SET %s", ZSTR_VAL(tableName), Z_STRVAL(setString));
	}
	zval_ptr_dtor(&setString);
	zend_string_release(tableName);

	mysqlQuery(instance, return_value, sql, throwsException);
	zend_string_release(sql);
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_node_beauty_mysql, delete)
{
	zend_string *tableName, *tstr, *sql;
	zval *where = NULL, *instance = getThis();
	zend_bool throwsException = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|zb", &tableName, &where, &throwsException) == FAILURE) {
		return;
	}

	// get sql
	tstr = mysqlEscapeStr(tableName);
	tableName = mysqlKeyword(tstr);
	zend_string_release(tstr);
	tstr = mysqlCompileUpdateWhere(instance, where);
	if (tstr) {
		sql = strpprintf(0, "DELETE FROM %s WHERE %s", ZSTR_VAL(tableName), ZSTR_VAL(tstr));
		zend_string_release(tstr);
	} else {
		sql = strpprintf(0, "DELETE FROM %s", ZSTR_VAL(tableName));
	}
	zend_string_release(tableName);

	mysqlQuery(instance, return_value, sql, throwsException);
	zend_string_release(sql);
}
/* }}} */

/* {{{ proto getSqlBuilder */
PHP_METHOD(azalea_node_beauty_mysql, getSqlBuilder)
{
	zval rv = {{0}};
	object_init_ex(&rv, mysqlSqlBuilderCe);
	// call __init method
	if (zend_hash_str_exists(&(mysqlSqlBuilderCe->function_table), ZEND_STRL("__construct"))) {
		zend_call_method_with_1_params(&rv, mysqlSqlBuilderCe, NULL, "__construct", NULL, getThis());
	}
	RETURN_ZVAL(&rv, 0, 0);
}
/* }}} */

/* ----- Azalea\MysqlResult ----- */
/* {{{ proto getSql */
PHP_METHOD(azalea_node_beauty_mysql_result, getSql)
{
	zval *value = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_sql"), 1, NULL);
	RETURN_ZVAL(value, 1, 0);
}
/* }}} */

/* {{{ proto getError */
PHP_METHOD(azalea_node_beauty_mysql_result, getError)
{
	zval *value = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_error"), 1, NULL);
	RETURN_ZVAL(value, 1, 0);
}
/* }}} */

/* {{{ proto getTimer */
PHP_METHOD(azalea_node_beauty_mysql_result, getTimer)
{
	zval *value = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_timer"), 1, NULL);
	if (Z_TYPE_P(value) == IS_LONG) {
		RETURN_LONG(Z_LVAL_P(value));
	}
	RETURN_NULL();
}
/* }}} */

/* ----- Azalea\MysqlQueryResult ----- */
/* {{{ proto all*/
PHP_METHOD(azalea_node_beauty_mysql_query, all)
{
	zval *result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) != IS_ARRAY) {
		array_init(return_value);
	} else {
		RETURN_ZVAL(result, 1, 0);
	}
}
/* }}} */

/* {{{ proto allWithKey*/
PHP_METHOD(azalea_node_beauty_mysql_query, allWithKey)
{
	zend_string *fieldName;
	zval *row, *result, *key;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &fieldName) == FAILURE) {
		return;
	}

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	array_init(return_value);
	if (Z_TYPE_P(result) != IS_ARRAY) {
		return;
	}
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(result), row) {
		key = zend_read_property(NULL, row, ZSTR_VAL(fieldName), ZSTR_LEN(fieldName), 1, NULL);
		if (Z_TYPE_P(key) == IS_STRING) {
			add_assoc_zval_ex(return_value, Z_STRVAL_P(key), Z_STRLEN_P(key), row);
			zval_add_ref(row);
		} else if (Z_TYPE_P(key) == IS_LONG) {
			add_index_zval(return_value, Z_LVAL_P(key), row);
			zval_add_ref(row);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

/* {{{ proto mysqlFindFieldName */
static zend_string * mysqlFindFieldName(zval *row, zval *index)
{
	zend_ulong h = 0;
	zend_bool found = 0;
	zend_string *keyValue;
	if (Z_TYPE_P(row) != IS_OBJECT) {
		return NULL;
	}
	ZEND_HASH_FOREACH_STR_KEY(Z_OBJPROP_P(row), keyValue) {
		if (!keyValue) {
			continue;
		}
		if ((Z_TYPE_P(index) == IS_LONG && Z_LVAL_P(index) == h++) ||
				(Z_TYPE_P(index) == IS_STRING && 0 == strcasecmp(ZSTR_VAL(keyValue), Z_STRVAL_P(index)))) {
			found = 1;
			break;
		}
	} ZEND_HASH_FOREACH_END();
	if (!found) {
		return NULL;
	}
	return zend_string_init(ZSTR_VAL(keyValue), ZSTR_LEN(keyValue), 0);
}
/* }}} */

/* {{{ proto column */
PHP_METHOD(azalea_node_beauty_mysql_query, column)
{
	zval fieldName, *index = NULL, *result, *row;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &index) == FAILURE) {
		return;
	}

	if (index == NULL) {
		index = &fieldName;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or numeric value");
		return;
	}

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	array_init(return_value);
	if (Z_TYPE_P(result) != IS_ARRAY || !(row = zend_hash_index_find(Z_ARRVAL_P(result), 0))) {
		return;
	}
	// find fieldName
	zend_string *keyValue = mysqlFindFieldName(row, index);
	if (!keyValue) {
		php_error_docref(NULL, E_NOTICE, "field index not found");
		return;
	}
	// foreach
	zval *value;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(result), row) {
		value = zend_read_property(NULL, row, ZSTR_VAL(keyValue), ZSTR_LEN(keyValue), 1, NULL);
		add_next_index_zval(return_value, value);
		zval_add_ref(value);
	} ZEND_HASH_FOREACH_END();
	zend_string_release(keyValue);
}
/* }}} */

/* {{{ proto columnWithKey */
PHP_METHOD(azalea_node_beauty_mysql_query, columnWithKey)
{
	zval fieldName, *index = NULL, *keyField, *result, *row;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|z", &keyField, &index) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(keyField) != IS_STRING && Z_TYPE_P(keyField) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or numeric value");
		return;
	}
	if (index == NULL) {
		index = &fieldName;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "expects parameter 2 to be string or numeric value");
		return;
	}

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	array_init(return_value);
	if (Z_TYPE_P(result) != IS_ARRAY || !(row = zend_hash_index_find(Z_ARRVAL_P(result), 0))) {
		return;
	}
	// find fieldName
	zend_string *key = mysqlFindFieldName(row, keyField);
	if (!key) {
		php_error_docref(NULL, E_NOTICE, "key not found");
		return;
	}
	zend_string *keyValue = mysqlFindFieldName(row, index);
	if (!keyValue) {
		php_error_docref(NULL, E_NOTICE, "field index not found");
		return;
	}
	// foreach
	zval *value;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(result), row) {
		keyField = zend_read_property(NULL, row, ZSTR_VAL(key), ZSTR_LEN(key), 1, NULL);
		value = zend_read_property(NULL, row, ZSTR_VAL(keyValue), ZSTR_LEN(keyValue), 1, NULL);
		if (Z_TYPE_P(keyField) == IS_STRING) {
			add_assoc_zval_ex(return_value, Z_STRVAL_P(keyField), Z_STRLEN_P(keyField), value);
			zval_add_ref(value);
		} else if (Z_TYPE_P(keyField) == IS_LONG) {
			add_index_zval(return_value, Z_LVAL_P(keyField), value);
			zval_add_ref(value);
		}
	} ZEND_HASH_FOREACH_END();
	zend_string_release(key);
	zend_string_release(keyValue);
}
/* }}} */

/* {{{ proto row */
PHP_METHOD(azalea_node_beauty_mysql_query, row)
{
	zend_long index = 0;
	zval *result, *row;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &index) == FAILURE) {
		return;
	}

	if (index < 0) {
		index = 0;
	}
	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) != IS_ARRAY || index >= zend_hash_num_elements(Z_ARRVAL_P(result)) ||
			!(row = zend_hash_index_find(Z_ARRVAL_P(result), index))) {
		RETURN_NULL();
	}
	RETURN_ZVAL(row, 1, 0);
}
/* }}} */

/* {{{ proto field */
PHP_METHOD(azalea_node_beauty_mysql_query, field)
{
	zval fieldName, *index = NULL, *result, *row;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &index) == FAILURE) {
		return;
	}

	if (index == NULL) {
		index = &fieldName;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or numeric value");
		return;
	}

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) != IS_ARRAY || !(row = zend_hash_index_find(Z_ARRVAL_P(result), 0))) {
		RETURN_FALSE;
	}
	// find fieldName
	zend_string *keyValue = mysqlFindFieldName(row, index);
	if (!keyValue) {
		php_error_docref(NULL, E_NOTICE, "field index not found");
		return;
	}
	// foreach
	zval *value = zend_read_property(NULL, row, ZSTR_VAL(keyValue), ZSTR_LEN(keyValue), 1, NULL);
	RETVAL_ZVAL(value, 1, 0);
	zend_string_release(keyValue);
}
/* }}} */

/* {{{ proto fields */
PHP_METHOD(azalea_node_beauty_mysql_query, fields)
{
	zend_string *key;
	zval *result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL), *row;
	if (Z_TYPE_P(result) != IS_ARRAY || !(row = zend_hash_index_find(Z_ARRVAL_P(result), 0))) {
		RETURN_FALSE;
	}
	array_init(return_value);
	if (Z_TYPE_P(row) != IS_OBJECT) {
		return;
	}
	ZEND_HASH_FOREACH_STR_KEY(Z_OBJPROP_P(row), key) {
		if (!key) {
			continue;
		}
		add_next_index_str(return_value, zend_string_copy(key));
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

/* ----- Azalea\MysqlExecuteResult ----- */
/* {{{ proto insertId */
PHP_METHOD(azalea_node_beauty_mysql_execute, insertId)
{
	zval *result, *insertId;

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) == IS_OBJECT) {
		insertId = zend_read_property(NULL, result, ZEND_STRL("insertId"), 1, NULL);
		if (Z_TYPE_P(insertId) == IS_LONG) {
			RETURN_LONG(Z_LVAL_P(insertId));
		}
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto affected */
PHP_METHOD(azalea_node_beauty_mysql_execute, affected)
{
	zval *result, *affected;

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) == IS_OBJECT) {
		affected = zend_read_property(NULL, result, ZEND_STRL("affectedRows"), 1, NULL);
		if (Z_TYPE_P(affected) == IS_LONG) {
			RETURN_LONG(Z_LVAL_P(affected));
		}
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto changed */
PHP_METHOD(azalea_node_beauty_mysql_execute, changed)
{
	zval *result, *changed;

	result = zend_read_property(mysqlResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) == IS_OBJECT) {
		changed = zend_read_property(NULL, result, ZEND_STRL("changedRows"), 1, NULL);
		if (Z_TYPE_P(changed) == IS_LONG) {
			RETURN_LONG(Z_LVAL_P(changed));
		}
	}
	RETURN_FALSE;
}
/* }}} */
