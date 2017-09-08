/*
 * mysqlnd.c
 *
 * Created by Bun Wong on 17-9-3.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#ifdef WITH_MYSQLND

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/model.h"
#include "azalea/exception.h"
#include "azalea/ext-models/mysqlnd.h"

#include "ext/standard/php_var.h"
#include "Zend/zend_interfaces.h"	// for zend_call_method_with_*
#include "ext/mysqlnd/mysqlnd.h"	// for mysqlnd

zend_class_entry *azalea_ext_model_mysqlnd_ce;
zend_class_entry *mysqlndResultCe;
zend_class_entry *mysqlndQueryResultCe;
zend_class_entry *mysqlndExecuteResultCe;

#ifdef WITH_SQLBUILDER
zend_class_entry *sqlBuilderCe;
zend_class_entry *sqlBuilderInterfaceCe;
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysqlnd_query, 0, 0, 1)
    ZEND_ARG_INFO(0, sql)
ZEND_END_ARG_INFO()

/* {{{ class MysqlndModel methods */
static zend_function_entry azalea_ext_model_mysqlnd_methods[] = {
	PHP_ME(azalea_ext_model_mysqlnd, __init, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(azalea_ext_model_mysqlnd, escape, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd, query, arginfo_mysqlnd_query, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd, getQueries, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd, getSqlBuilder, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlndResult methods */
static zend_function_entry azalea_ext_model_mysqlnd_result_methods[] = {
	PHP_ME(azalea_ext_model_mysqlnd_result, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_ext_model_mysqlnd_result, getSql, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_result, getTimer, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlndQueryResult methods */
static zend_function_entry azalea_ext_model_mysqlnd_query_methods[] = {
	PHP_ME(azalea_ext_model_mysqlnd_query, count, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, all, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, allWithKey, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, column, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, columnWithKey, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, row, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, field, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_query, fields, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlndExecuteResult methods */
static zend_function_entry azalea_ext_model_mysqlnd_execute_methods[] = {
	PHP_ME(azalea_ext_model_mysqlnd_execute, insertId, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_mysqlnd_execute, affectedRows, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ 资源 dtor */
// Connection
static int ldConnection;
ZEND_RSRC_DTOR_FUNC(rsrcConnectionDtor)
{
	if (res->ptr) {
		MYSQLND *mysql = (MYSQLND *) res->ptr;
		mysqlnd_close(mysql, MYSQLND_CLOSE_DISCONNECTED);
	}
}
// Query result
static int ldQueryResult;
ZEND_RSRC_DTOR_FUNC(rsrcQueryResultDtor)
{
	if (res->ptr) {
		MYSQLND_RES *result = (MYSQLND_RES *) res->ptr;
		mysqlnd_free_result(result, 0);
	}
}
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd)
{
	// 定义资源类型
	ldConnection = zend_register_list_destructors_ex(rsrcConnectionDtor, NULL, AZALEA_NS_NAME(MysqlndConnectionRes), module_number);
	ldQueryResult = zend_register_list_destructors_ex(rsrcQueryResultDtor, NULL, AZALEA_NS_NAME(MysqlndQueryResultRes), module_number);

#ifdef WITH_SQLBUILDER
	// 获取 sqlBuilder ce
	sqlBuilderCe = azaleaSqlBuilderGetCe();
	sqlBuilderInterfaceCe = azaleaSqlBuilderGetInterfaceCe();
#endif

	zend_class_entry ce, resultCe, queryResultCe, executeResultCe;
	// model
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(MysqlndModel), azalea_ext_model_mysqlnd_methods);
	azalea_ext_model_mysqlnd_ce = zend_register_internal_class_ex(&ce, azaleaModelCe);
#ifdef WITH_SQLBUILDER
	if (sqlBuilderInterfaceCe) {
		zend_class_implements(azalea_ext_model_mysqlnd_ce, 1, sqlBuilderInterfaceCe);
	}
#endif
	azalea_ext_model_mysqlnd_ce->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azalea_ext_model_mysqlnd_ce, ZEND_STRL("_connection"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_ext_model_mysqlnd_ce, ZEND_STRL("_queries"), ZEND_ACC_PRIVATE);
	// MysqlResult
	INIT_CLASS_ENTRY(resultCe, AZALEA_NS_NAME(MysqlndResult), azalea_ext_model_mysqlnd_result_methods);
	mysqlndResultCe = zend_register_internal_class(&resultCe);
	mysqlndResultCe->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	zend_declare_property_null(mysqlndResultCe, ZEND_STRL("_sql"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlndResultCe, ZEND_STRL("_timer"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlndResultCe, ZEND_STRL("_result"), ZEND_ACC_PRIVATE);
	// MysqlQueryResult
	INIT_CLASS_ENTRY(queryResultCe, AZALEA_NS_NAME(MysqlndQueryResult), azalea_ext_model_mysqlnd_query_methods);
	mysqlndQueryResultCe = zend_register_internal_class_ex(&queryResultCe, mysqlndResultCe);
	// MysqlExecuteResult
	INIT_CLASS_ENTRY(executeResultCe, AZALEA_NS_NAME(MysqlndExecuteResult), azalea_ext_model_mysqlnd_execute_methods);
	mysqlndExecuteResultCe = zend_register_internal_class_ex(&executeResultCe, mysqlndResultCe);

	return SUCCESS;
}
/* }}} */

/* {{{ throwMysqlError */
static void azaleaMysqndThrowError(MYSQLND *mysql, const char *str)
{
	zend_string *message;
	message = strpprintf(0, "%s [%d, %s]", str, mysqlnd_errno(mysql), mysqlnd_error(mysql));
	throw500Str(ZSTR_VAL(message), ZSTR_LEN(message));
	zend_string_release(message);
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_ext_model_mysqlnd, __construct) {}
PHP_METHOD(azalea_ext_model_mysqlnd, __init)
{
	zval dummy, rsrc, *this = getThis(), *mysqlConfig, *lcName;
	zval *pHost, *pPort, *pUsername, *pPasswd, *pDbname, *pCharset;
	zend_string *host, *username, *passwd, *dbname, *charset, *tstr;
	zend_long port, flags = 0, allowLocalInfile = 1;
	zend_bool persistent = 0;
	MYSQLND *mysql;

	// init _queries array
	array_init(&dummy);
	zend_update_property(azalea_ext_model_mysqlnd_ce, this, ZEND_STRL("_queries"), &dummy);
	zval_ptr_dtor(&dummy);

	// init mysql connection
	if ((lcName = zend_read_property(azaleaModelCe, this, ZEND_STRL("_modelname"), 1, NULL)) &&
			(mysqlConfig = azaleaConfigSubFindEx(Z_STRVAL_P(lcName), Z_STRLEN_P(lcName), NULL, 0))) {
		pHost = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("host"));
		pPort = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("port"));
		pUsername = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("user"));
		pPasswd = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("passwd"));
		pDbname = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("name"));
		pCharset = zend_hash_str_find(Z_ARRVAL_P(mysqlConfig), ZEND_STRL("charset"));
		host = pHost ? zend_string_copy(Z_STR_P(pHost)) : zend_string_init(ZEND_STRL("127.0.0.1"), 0);
		port = pPort ? ZEND_STRTOL(Z_STRVAL_P(pPort), NULL, 10) : 3306;
		username = pUsername ? zend_string_copy(Z_STR_P(pUsername)) : zend_string_init(ZEND_STRL("root"), 0);
		passwd = pPasswd ? zend_string_copy(Z_STR_P(pPasswd)) : ZSTR_EMPTY_ALLOC();
		dbname = pDbname ? zend_string_copy(Z_STR_P(pDbname)) : zend_string_init(ZEND_STRL("test"), 0);
		charset = pCharset ? zend_string_copy(Z_STR_P(pCharset)) : zend_string_init(ZEND_STRL("utf8"), 0);
		flags |= CLIENT_MULTI_RESULTS;
	} else {
		// 找不到设置或未初始化连接
		if (!lcName) {
			php_error_docref(NULL, E_ERROR, "Model name not found");
		} else {
			php_error_docref(NULL, E_ERROR, "Mysql config `[%s]` not found", Z_STRVAL_P(lcName));
		}
		return;
	}
	if (!(mysql = mysqlnd_init(MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA, persistent))) {
		throw500Str(ZEND_STRL("Mysqlnd init error"));
		goto end;
	}
	if (mysqlnd_connect(mysql, ZSTR_VAL(host), ZSTR_VAL(username), ZSTR_VAL(passwd),
			ZSTR_LEN(passwd), ZSTR_VAL(dbname), ZSTR_LEN(dbname), port, NULL, flags,
			MYSQLND_CLIENT_KNOWS_RSET_COPY_DATA) == NULL) {
		// throws exception
		azaleaMysqndThrowError(mysql, "Mysql connect error");
		// free mysql structure
		mysqlnd_close(mysql, MYSQLND_CLOSE_DISCONNECTED);
		goto end;
	}
	mysqlnd_options(mysql, MYSQL_OPT_LOCAL_INFILE, (char *)&allowLocalInfile);
	mysqlnd_set_server_option(mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	mysqlnd_set_character_set(mysql, ZSTR_VAL(charset));
	// 定义为资源属性
	ZVAL_RES(&rsrc, zend_register_resource(mysql, ldConnection));
	zend_update_property(azalea_ext_model_mysqlnd_ce, this, ZEND_STRL("_connection"), &rsrc);
	zval_ptr_dtor(&rsrc);

end:
	zend_string_release(host);
	zend_string_release(username);
	zend_string_release(passwd);
	zend_string_release(dbname);
	zend_string_release(charset);
}
/* }}} */

/* {{{ proto escape */
PHP_METHOD(azalea_ext_model_mysqlnd, escape)
{
#ifdef WITH_SQLBUILDER
	zval *value;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z", &value) == FAILURE) {
		return;
	}

	sqlBuilderEscapeEx(return_value, value, 1);
#else
	zend_string *value;
	char *result;
	size_t len, lenResult;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &value) == FAILURE) {
		return;
	}

	len = ZSTR_LEN(value);
	result = emalloc(len * 2);	// 最多为原字符串2倍长度
	lenResult = mysqlnd_old_escape_string(result, ZSTR_VAL(value), len);

	if (lenResult == len) {
		RETVAL_STR_COPY(value);
	} else {
		RETVAL_STRINGL(result, lenResult);
	}
	efree(result);
#endif
}
/* }}} */

/* {{{ proto azaleaMysqlndInitExecuteResult */
static void azaleaMysqlndInitExecuteResult(zval *return_value, MYSQLND *mysql, zend_string *sql, double timeSpent)
{
	zval result;
	uint64_t rc;
	zend_long affectedRows;

	array_init(&result);
	// affected
	rc = mysqlnd_affected_rows(mysql);
	if (rc == (uint64_t) -1) {
		affectedRows = -1;
	} else {
		affectedRows = (zend_long) rc;
	}
	add_assoc_long_ex(&result, ZEND_STRL("affectedRows"), affectedRows);
	// insertId
	add_assoc_long_ex(&result, ZEND_STRL("insertId"), (zend_long) mysqlnd_insert_id(mysql));
	// init object
	object_init_ex(return_value, mysqlndExecuteResultCe);
	zend_update_property_double(mysqlndResultCe, return_value, ZEND_STRL("_timer"), timeSpent);
	zend_update_property_str(mysqlndResultCe, return_value, ZEND_STRL("_sql"), sql);
	zend_update_property(mysqlndResultCe, return_value, ZEND_STRL("_result"), &result);
	zval_ptr_dtor(&result);
}
/* }}} */

/* {{{ proto azaleaMysqlndInitQueryResult */
static void azaleaMysqlndInitQueryResult(zval *return_value, MYSQLND *mysql, zend_string *sql, double timeSpent)
{
	zval result;

	ZVAL_RES(&result, zend_register_resource(mysqlnd_store_result(mysql), ldQueryResult));

	// init object
	object_init_ex(return_value, mysqlndQueryResultCe);
	zend_update_property_double(mysqlndResultCe, return_value, ZEND_STRL("_timer"), timeSpent);
	zend_update_property_str(mysqlndResultCe, return_value, ZEND_STRL("_sql"), sql);
	zend_update_property(mysqlndResultCe, return_value, ZEND_STRL("_result"), &result);
	zval_ptr_dtor(&result);
}
/* }}} */

/* {{{ proto query */
PHP_METHOD(azalea_ext_model_mysqlnd, query)
{
	zval *this = getThis(), *queries;
	zend_string *sql;
	MYSQLND *mysql;
	double timeQueryStart, timeSpent;

	AZALEA_MYSQLND_FETCH_RESOURCE_CONN(mysql, this);

#ifdef WITH_SQLBUILDER
	zval *binds = NULL, array;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|z", &sql, &binds) == FAILURE) {
		return;
	}

	if (binds && Z_TYPE_P(binds) > IS_ARRAY) {
		php_error_docref(NULL, E_WARNING, "Expects parameter 2 to be array");
		binds = NULL;
	}
	// convert binds to array
	if (binds && Z_TYPE_P(binds) != IS_ARRAY) {
		array_init(&array);
		add_next_index_zval(&array, binds);
		zval_add_ref(binds);
		binds = &array;
	} else if (binds) {
		zval_add_ref(binds);
	}

	if (binds && strchr(ZSTR_VAL(sql), '?') && Z_TYPE_P(binds) == IS_ARRAY &&
			zend_hash_num_elements(Z_ARRVAL_P(binds)) > 0) {
		sql = sqlBuilderCompileBinds(sql, binds, 1);
	} else {
		zend_string_addref(sql);
	}
	if (binds) {
		zval_ptr_dtor(binds);
	}
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &sql) == FAILURE) {
		return;
	}

	zend_string_addref(sql);
#endif

	if (!ZSTR_LEN(sql)) {
		zend_string_release(sql);
		php_error_docref(NULL, E_WARNING, "Empty query");
		RETURN_FALSE;
	}
	// 执行查询
	timeQueryStart = azaleaGetMicrotime();
	if (mysqlnd_query(mysql, ZSTR_VAL(sql), ZSTR_LEN(sql))) {
		azaleaMysqndThrowError(mysql, "Mysql query error");
		zend_string_release(sql);
		RETURN_FALSE;
	}
	timeSpent = azaleaGetMicrotime() - timeQueryStart;	// 查询消耗的时间
	queries = zend_read_property(azalea_ext_model_mysqlnd_ce, this, ZEND_STRL("_queries"), 1, NULL);
	add_next_index_str(queries, sql);	// sql 加入了 queries 数组，就不需要再 release 了

	// 检查结果
	if (!mysqlnd_field_count(mysql)) {
		// 无结果集, 生成 MysqlndExecuteResult
		azaleaMysqlndInitExecuteResult(return_value, mysql, sql, timeSpent);
	} else {
		// 查询结果集, 生成 MysqlndQueryResult
		azaleaMysqlndInitQueryResult(return_value, mysql, sql, timeSpent);
	}
}
/* }}} */

/* {{{ proto getQueries */
PHP_METHOD(azalea_ext_model_mysqlnd, getQueries)
{
	zval *queries;

	queries = zend_read_property(azalea_ext_model_mysqlnd_ce, getThis(), ZEND_STRL("_queries"), 1, NULL);
	if (queries && Z_TYPE_P(queries) == IS_ARRAY) {
		RETURN_ZVAL(queries, 1, 0);
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto getSqlBuilder */
PHP_METHOD(azalea_ext_model_mysqlnd, getSqlBuilder)
{
#ifdef WITH_SQLBUILDER
	zval rv = {{0}};
	if (sqlBuilderCe) {
		object_init_ex(&rv, sqlBuilderCe);
		// call __init method
		if (zend_hash_str_exists(&(sqlBuilderCe->function_table), ZEND_STRL("__construct"))) {
			zend_call_method_with_1_params(&rv, sqlBuilderCe, NULL, "__construct", NULL, getThis());
		}
		RETURN_ZVAL(&rv, 0, 0);
	}
	RETURN_NULL();
#else
	RETURN_NULL();
#endif
}
/* }}} */

/* ----- Azalea\MysqlndResult ----- */
/* {{{ proto getSql */
PHP_METHOD(azalea_ext_model_mysqlnd_result, __construct) {}
/* }}} */

/* {{{ proto getSql */
PHP_METHOD(azalea_ext_model_mysqlnd_result, getSql)
{
	zval *value = zend_read_property(mysqlndResultCe, getThis(), ZEND_STRL("_sql"), 1, NULL);
	if (value && Z_TYPE_P(value) == IS_STRING) {
		RETURN_ZVAL(value, 1, 0);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto getTimer */
PHP_METHOD(azalea_ext_model_mysqlnd_result, getTimer)
{
	zval *value = zend_read_property(mysqlndResultCe, getThis(), ZEND_STRL("_timer"), 1, NULL);
	if (value && Z_TYPE_P(value) == IS_DOUBLE) {
		RETURN_DOUBLE(Z_DVAL_P(value));
	}
	RETURN_FALSE;
}
/* }}} */

/* ----- Azalea\MysqlndQueryResult ----- */
/* {{{ proto all*/
PHP_METHOD(azalea_ext_model_mysqlnd_query, count)
{
	MYSQLND_RES *result;

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	MYSQLND_RETURN_LONG(mysqlnd_num_rows(result));
}
/* }}} */

/* {{{ proto azaleaMysqlndFindName */
static zend_string * azaleaMysqlndFindFieldName(MYSQLND_RES *result, zval *index)
{
	int i;
	for (i = 0; i < mysqlnd_num_fields(result); ++i) {
		const MYSQLND_FIELD *field = mysqlnd_fetch_field_direct(result, i);
		if ((Z_TYPE_P(index) == IS_LONG && Z_LVAL_P(index) == i) ||
				(Z_TYPE_P(index) == IS_STRING && 0 == strcasecmp(field->name, Z_STRVAL_P(index)))) {
			return field->sname;
		}
	}
	return NULL;
}
/* }}} */

/* {{{ proto azaleaMysqlndFetchRow */
static zend_bool azaleaMysqlndFetchRow(zval *return_value, MYSQLND_RES *result, zend_class_entry *ce, zend_bool isObject)
{
	zval dataset;
	mysqlnd_fetch_into(result, MYSQLND_FETCH_ASSOC, &dataset, MYSQLND_MYSQL);
	if (Z_TYPE(dataset) != IS_ARRAY) {
		return 0;
	}
	if (ce && isObject) {
		object_and_properties_init(return_value, ce, NULL);
		if (!ce->default_properties_count && !ce->__set) {
			Z_OBJ_P(return_value)->properties = Z_ARR(dataset);
		} else {
			zend_merge_properties(return_value, Z_ARRVAL(dataset));
			zval_ptr_dtor(&dataset);
		}
	} else {
		ZVAL_ZVAL(return_value, &dataset, 0, 0);
	}
	return 1;
}
/* }}} */

/* {{{ proto all */
PHP_METHOD(azalea_ext_model_mysqlnd_query, all)
{
	MYSQLND_RES *result;
	zend_class_entry *ce;
	zend_bool hasNext;

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	ce = zend_standard_class_def;
	array_init(return_value);	// make sure return_value is an array
	do {
		zval dummy;
		if ((hasNext = azaleaMysqlndFetchRow(&dummy, result, ce, 1))) {
			add_next_index_zval(return_value, &dummy);
		}
	} while (hasNext);
}
/* }}} */

/* {{{ proto allWithKey */
PHP_METHOD(azalea_ext_model_mysqlnd_query, allWithKey)
{
	zval *key;
	MYSQLND_RES *result;
	zend_string *keyField;
	zend_class_entry *ce;
	zend_bool hasNext;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &key) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(key) != IS_STRING && Z_TYPE_P(key) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "Expects parameter 1 to be string or numeric value");
		return;
	}

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	ce = zend_standard_class_def;
	keyField = azaleaMysqlndFindFieldName(result, key);
	if (!keyField) {
		// 找不到字段
		php_error_docref(NULL, E_NOTICE, "Key not found");
		RETURN_FALSE;
	}
	array_init(return_value);
	do {
		zval dummy;
		if ((hasNext = azaleaMysqlndFetchRow(&dummy, result, ce, 1))) {
			key = zend_read_property(ce, &dummy, ZSTR_VAL(keyField), ZSTR_LEN(keyField), 1, NULL);
			add_assoc_zval_ex(return_value, Z_STRVAL_P(key), Z_STRLEN_P(key), &dummy);
		}
	} while (hasNext);
}
/* }}} */

/* {{{ proto column */
PHP_METHOD(azalea_ext_model_mysqlnd_query, column)
{
	zval dummy, *value, *index = NULL;
	MYSQLND_RES *result;
	zend_string *keyValue;
	zend_bool hasNext;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &index) == FAILURE) {
		return;
	}
	if (index == NULL) {
		index = &dummy;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "Expects parameter 1 to be string or numeric value");
		return;
	}

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	keyValue = azaleaMysqlndFindFieldName(result, index);
	if (!keyValue) {
		// 找不到字段
		php_error_docref(NULL, E_NOTICE, "Field index not found");
		RETURN_FALSE;
	}
	array_init(return_value);	// make sure return_value is an array
	do {
		zval dummy;
		if ((hasNext = azaleaMysqlndFetchRow(&dummy, result, NULL, 0))) {
			value = zend_hash_find(Z_ARRVAL(dummy), keyValue);
			add_next_index_zval(return_value, value);
			zval_ptr_dtor(&dummy);
		}
	} while (hasNext);
}
/* }}} */

/* {{{ proto columnWithKey */
PHP_METHOD(azalea_ext_model_mysqlnd_query, columnWithKey)
{
	zval dummy, *key, *value, *index = NULL;
	MYSQLND_RES *result;
	zend_string *keyField, *keyValue;
	zend_bool hasNext;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|z", &key, &index) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(key) != IS_STRING && Z_TYPE_P(key) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "Expects parameter 1 to be string or numeric value");
		return;
	}
	if (index == NULL) {
		index = &dummy;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "Expects parameter 2 to be string or numeric value");
		return;
	}

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	keyField = azaleaMysqlndFindFieldName(result, key);
	if (!keyField) {
		// 找不到字段
		php_error_docref(NULL, E_NOTICE, "Key not found");
		RETURN_FALSE;
	}
	keyValue = azaleaMysqlndFindFieldName(result, index);
	if (!keyValue) {
		// 找不到字段
		php_error_docref(NULL, E_NOTICE, "Field index not found");
		RETURN_FALSE;
	}
	array_init(return_value);	// make sure return_value is an array
	do {
		zval dummy;
		if ((hasNext = azaleaMysqlndFetchRow(&dummy, result, NULL, 0))) {
			key = zend_hash_find(Z_ARRVAL(dummy), keyField);
			value = zend_hash_find(Z_ARRVAL(dummy), keyValue);
			add_assoc_zval_ex(return_value, Z_STRVAL_P(key), Z_STRLEN_P(key), value);
			zval_ptr_dtor(&dummy);
		}
	} while (hasNext);
}
/* }}} */

/* {{{ proto row */
PHP_METHOD(azalea_ext_model_mysqlnd_query, row)
{
	MYSQLND_RES *result;
	zend_class_entry *ce;

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	ce = zend_standard_class_def;
	{
		zval dummy;
		if ((azaleaMysqlndFetchRow(&dummy, result, ce, 1))) {
			RETVAL_ZVAL(&dummy, 1, 0);
			zval_ptr_dtor(&dummy);
		}
	}
}
/* }}} */

/* {{{ proto field */
PHP_METHOD(azalea_ext_model_mysqlnd_query, field)
{
	zval dummy, *value, *index = NULL;
	MYSQLND_RES *result;
	zend_string *keyValue;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &index) == FAILURE) {
		return;
	}
	if (index == NULL) {
		index = &dummy;
		ZVAL_LONG(index, 0);
	} else if (Z_TYPE_P(index) != IS_STRING && Z_TYPE_P(index) != IS_LONG) {
		php_error_docref(NULL, E_ERROR, "Expects parameter 1 to be string or numeric value");
		return;
	}

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	keyValue = azaleaMysqlndFindFieldName(result, index);
	if (!keyValue) {
		// 找不到字段
		php_error_docref(NULL, E_NOTICE, "Field index not found");
		RETURN_FALSE;
	}
	{
		zval dummy;
		if ((azaleaMysqlndFetchRow(&dummy, result, NULL, 0))) {
			value = zend_hash_find(Z_ARRVAL(dummy), keyValue);
			RETVAL_ZVAL(value, 1, 0);
			zval_ptr_dtor(&dummy);
		}
	}
}
/* }}} */

/* {{{ proto fields */
PHP_METHOD(azalea_ext_model_mysqlnd_query, fields)
{
	MYSQLND_RES *result;
	int i;

	AZALEA_MYSQLND_FETCH_RESOURCE_QR(result, getThis());
	array_init(return_value);
	for (i = 0; i < mysqlnd_num_fields(result); ++i) {
		const MYSQLND_FIELD *field = mysqlnd_fetch_field_direct(result, i);
		add_index_str(return_value, i, zend_string_copy(field->sname));
	}
}

/* ----- Azalea\MysqlndExecuteResult ----- */
/* {{{ proto insertId */
PHP_METHOD(azalea_ext_model_mysqlnd_execute, insertId)
{
	zval *result, *insertId;

	result = zend_read_property(mysqlndResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) == IS_ARRAY) {
		insertId = zend_hash_str_find(Z_ARRVAL_P(result), ZEND_STRL("insertId"));
		if (insertId && Z_TYPE_P(insertId) == IS_LONG) {
			MYSQLND_RETURN_LONG(Z_LVAL_P(insertId));
		}
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto affectedRows */
PHP_METHOD(azalea_ext_model_mysqlnd_execute, affectedRows)
{
	zval *result, *affectedRows;

	result = zend_read_property(mysqlndResultCe, getThis(), ZEND_STRL("_result"), 1, NULL);
	if (Z_TYPE_P(result) == IS_ARRAY) {
		affectedRows = zend_hash_str_find(Z_ARRVAL_P(result), ZEND_STRL("affectedRows"));
		if (affectedRows && Z_TYPE_P(affectedRows) == IS_LONG) {
			MYSQLND_RETURN_LONG(Z_LVAL_P(affectedRows));
		}
	}
	RETURN_FALSE;
}
/* }}} */
//#endif
