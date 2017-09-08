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
zend_class_entry *mysqlResultCe;
zend_class_entry *mysqlQueryResultCe;
zend_class_entry *mysqlExecuteResultCe;

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

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_ext_model_mysqlnd_result_methods[] = {
//	PHP_ME(azalea_ext_model_mysqlnd_result, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
//	PHP_ME(azalea_ext_model_mysqlnd_result, getSql, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_result, getError, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_result, getTimer, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_ext_model_mysqlnd_query_methods[] = {
//	PHP_ME(azalea_ext_model_mysqlnd_query, all, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, allWithKey, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, column, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, columnWithKey, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, row, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, field, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_query, fields, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_ext_model_mysqlnd_execute_methods[] = {
//	PHP_ME(azalea_ext_model_mysqlnd_execute, insertId, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_execute, affected, NULL, ZEND_ACC_PUBLIC)
//	PHP_ME(azalea_ext_model_mysqlnd_execute, changed, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ 资源 dtor */
static int ldConnection;
ZEND_RSRC_DTOR_FUNC(rsrcConnectionDtor)
{
	if (res->ptr) {
		MYSQLND *mysql = (MYSQLND *) res->ptr;
		mysqlnd_close(mysql, MYSQLND_CLOSE_DISCONNECTED);
	}
}
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd)
{
	// 定义资源类型
	ldConnection = zend_register_list_destructors_ex(rsrcConnectionDtor, NULL, "Azalea\\Mysqlnd connection", module_number);

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
	mysqlResultCe = zend_register_internal_class(&resultCe);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_error"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_sql"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_timer"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlResultCe, ZEND_STRL("_result"), ZEND_ACC_PRIVATE);
	// MysqlQueryResult
	INIT_CLASS_ENTRY(queryResultCe, AZALEA_NS_NAME(MysqlndQueryResult), azalea_ext_model_mysqlnd_query_methods);
	mysqlQueryResultCe = zend_register_internal_class_ex(&queryResultCe, mysqlResultCe);
	// MysqlExecuteResult
	INIT_CLASS_ENTRY(executeResultCe, AZALEA_NS_NAME(MysqlndExecuteResult), azalea_ext_model_mysqlnd_execute_methods);
	mysqlExecuteResultCe = zend_register_internal_class_ex(&executeResultCe, mysqlResultCe);

	return SUCCESS;
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
		tstr = strpprintf(0, "Mysql connect error [%d, %s]", mysqlnd_errno(mysql), mysqlnd_error(mysql));
		throw500Str(ZSTR_VAL(tstr), ZSTR_LEN(tstr));
		zend_string_release(tstr);
		// free mysql structure
		mysqlnd_close(mysql, MYSQLND_CLOSE_DISCONNECTED);
		goto end;
	}
	mysqlnd_options(mysql, MYSQL_OPT_LOCAL_INFILE, (char *)&allowLocalInfile);
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

/* {{{ proto query */
PHP_METHOD(azalea_ext_model_mysqlnd, query)
{
	zend_string *sql;

#ifdef WITH_SQLBUILDER
	zval *binds = NULL, array;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|z", &sql, &binds) == FAILURE) {
		return;
	}

	if (binds && Z_TYPE_P(binds) > IS_ARRAY) {
		php_error_docref(NULL, E_WARNING, "expects parameter 2 to be array");
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

	php_printf(">%s<", ZSTR_VAL(sql));
//	mysqlQuery(getThis(), return_value, sql, throwsException);

	zend_string_release(sql);
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

//#endif
