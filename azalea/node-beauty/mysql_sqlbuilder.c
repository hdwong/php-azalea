/*
 * mysql_sqlbuilder.c
 *
 * Created by Bun Wong on 16-8-27.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/node-beauty/mysql.h"
#include "azalea/node-beauty/mysql_sqlbuilder.h"

#include "ext/standard/php_var.h"
#include "ext/standard/php_string.h"  // for php_trim

zend_class_entry *mysqlSqlBuilderCe;

/* {{{ class MysqlModel methods */
static zend_function_entry azalea_node_beauty_mysql_sqlbuilder_methods[] = {
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, __toString, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, where, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orWhere, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, having, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orHaving, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, whereGroupStart, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orWhereGroupStart, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, notWhereGroupStart, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orNotWhereGroupStart, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, whereGroupEnd, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, select, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, distinct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, from, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, join, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, limit, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, limitPage, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orderBy, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, groupBy, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, getSql, NULL, ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ proto Startup */
void mysqlSqlBuilderStartup()
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(MysqlSqlBuilder), azalea_node_beauty_mysql_sqlbuilder_methods);
	mysqlSqlBuilderCe = zend_register_internal_class(&ce);
	mysqlSqlBuilderCe->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_where"), ZEND_ACC_PRIVATE);
	zend_declare_property_bool(mysqlSqlBuilderCe, ZEND_STRL("_whereGroupPrefix"), 0, ZEND_ACC_PRIVATE);
	zend_declare_property_long(mysqlSqlBuilderCe, ZEND_STRL("_whereGroupDepth"), 0, ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_select"), ZEND_ACC_PRIVATE);
	zend_declare_property_bool(mysqlSqlBuilderCe, ZEND_STRL("_distinct"), 0, ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_from"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_join"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_orderBy"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_groupBy"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(mysqlSqlBuilderCe, ZEND_STRL("_limit"), ZEND_ACC_PRIVATE);
	zend_declare_property_long(mysqlSqlBuilderCe, ZEND_STRL("_offset"), 0, ZEND_ACC_PRIVATE);
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, __construct)
{
	zval value;
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_where"), &value);
	zval_ptr_dtor(&value);
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_select"), &value);
	zval_ptr_dtor(&value);
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_from"), &value);
	zval_ptr_dtor(&value);
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_join"), &value);
	zval_ptr_dtor(&value);
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_orderBy"), &value);
	zval_ptr_dtor(&value);
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_groupBy"), &value);
	zval_ptr_dtor(&value);
}
/* }}} */

/* {{{ proto mysqlGetType */
static zend_string * mysqlGetType(zend_bool whereGroupPrefix, zval *pRec, const char *pType)
{
	zend_string *type;
	// check in whereGroupPrefix
	if (whereGroupPrefix == 0 || zend_hash_num_elements(Z_ARRVAL_P(pRec)) == 0) {
		type = ZSTR_EMPTY_ALLOC();
	} else {
		// get type [AND, OR]
		if (type && 0 == strcasecmp("OR", pType)) {
			type = zend_string_init(ZEND_STRL("OR "), 0);
		} else if (type && 0 == strcasecmp("NOT", pType)) {
			type = zend_string_init(ZEND_STRL("NOT "), 0);
		} else if (type && 0 == strcasecmp("OR NOT", pType)) {
			type = zend_string_init(ZEND_STRL("OR NOT "), 0);
		} else {
			type = zend_string_init(ZEND_STRL("AND "), 0);
		}
	}
	return type;
}
/* }}} */

/* {{{ proto mysqlWhere */
static void mysqlWhere(zval *instance, zend_long recType, zval *conditions, zval *value, const char *pType, zend_bool escapeValue)
{
	zval *where, *pRec, *pWhereGroupPrefix;
	where = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_where"), 1, NULL);
	pRec = zend_hash_index_find(Z_ARRVAL_P(where), recType);
	pWhereGroupPrefix = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_whereGroupPrefix"), 1, NULL);

	// init rec property
	if (!pRec) {
		zval rec;
		pRec = &rec;
		array_init(pRec);
		zend_hash_index_add(Z_ARRVAL_P(where), recType, pRec);
	}
	// init conditions to an array
	if (Z_TYPE_P(conditions) != IS_ARRAY && Z_TYPE_P(conditions) != IS_STRING) {
		return;
	}
	if (Z_TYPE_P(conditions) != IS_ARRAY) {
		zval cond;
		array_init(&cond);
		add_assoc_zval_ex(&cond, Z_STRVAL_P(conditions), Z_STRLEN_P(conditions), value);
		zval_add_ref(value);
		conditions = &cond;
	} else {
		zval_add_ref(conditions);
	}

	// foreach
	zend_string *type, *key, *op, *segment, *tstr;
	zval *pData;
	char *pOp;
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(conditions), key, pData) {
		if (!key) {
			continue;
		}
		op = NULL;
		// type
		type = mysqlGetType(Z_TYPE_P(pWhereGroupPrefix) == IS_TRUE, pRec, pType);
		// field and OP
		key = php_trim(key, ZEND_STRL(" "), 3);
		pOp = strchr(ZSTR_VAL(key), ' ');
		if (pOp) {
			// get OP
			tstr = key;
			op = zend_string_init(pOp + 1, ZSTR_VAL(key) + ZSTR_LEN(key) - pOp - 1, 0);
			key = zend_string_init(ZSTR_VAL(key), pOp - ZSTR_VAL(key), 0);
			zend_string_release(tstr);
			// check OP
			if (strcmp(ZSTR_VAL(op), ">=") &&
					strcmp(ZSTR_VAL(op), "<=") &&
					strcmp(ZSTR_VAL(op), "<>") &&
					strcmp(ZSTR_VAL(op), "!=") &&
					strcmp(ZSTR_VAL(op), "=") &&
					strcmp(ZSTR_VAL(op), "<=>") &&
					strcmp(ZSTR_VAL(op), ">") &&
					strcmp(ZSTR_VAL(op), "<") &&
					strcasecmp(ZSTR_VAL(op), "IS") &&
					strcasecmp(ZSTR_VAL(op), "IS NOT") &&
					strcasecmp(ZSTR_VAL(op), "NOT IN") &&
					strcasecmp(ZSTR_VAL(op), "IN") &&
					strcasecmp(ZSTR_VAL(op), "LIKE") &&
					strcasecmp(ZSTR_VAL(op), "NOT LIKE")) {
				zend_string_release(op);
				op = NULL;
			} else {
				tstr = op;
				op = php_string_toupper(op);
				zend_string_release(tstr);
			}
		}
		if (!op) {
			if (Z_TYPE_P(pData) == IS_NULL) {
				op = zend_string_init(ZEND_STRL("IS"), 0);
			} else if (Z_TYPE_P(pData) == IS_ARRAY) {
				op = zend_string_init(ZEND_STRL("IN"), 0);
			} else {
				op = zend_string_init(ZEND_STRL("="), 0);
			}
		}
		// build segment
		segment = strpprintf(0, "%s?? %s ?", ZSTR_VAL(type), ZSTR_VAL(op));
		zval binds;
		array_init(&binds);
		add_next_index_str(&binds, zend_string_copy(key));
		add_next_index_zval(&binds, pData);
		zval_add_ref(pData);
		tstr = mysqlCompileBinds(segment, &binds, escapeValue);  // where string
		zval_ptr_dtor(&binds);

		add_next_index_str(pRec, tstr);

		ZVAL_TRUE(pWhereGroupPrefix);
		zend_string_release(segment);
		zend_string_release(op);
		zend_string_release(key);
		zend_string_release(type);
	} ZEND_HASH_FOREACH_END();

	zval_ptr_dtor(conditions);
}
/* }}} */

/* {{{ proto where */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, where)
{
	zval *conditions, *value;
	zend_bool escapeValue = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|zb", &conditions, &value, &escapeValue) == FAILURE) {
		return;
	}

	mysqlWhere(getThis(), RECKEY_WHERE, conditions, value, "AND", escapeValue);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto orWhere */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orWhere)
{
	zval *conditions, *value;
	zend_bool escapeValue = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|zb", &conditions, &value, &escapeValue) == FAILURE) {
		return;
	}

	mysqlWhere(getThis(), RECKEY_WHERE, conditions, value, "OR", escapeValue);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto having */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, having)
{
	zval *conditions, *value;
	zend_bool escapeValue = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|zb", &conditions, &value, &escapeValue) == FAILURE) {
		return;
	}

	mysqlWhere(getThis(), RECKEY_HAVING, conditions, value, "AND", escapeValue);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto orHaving */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orHaving)
{
	zval *conditions, *value;
	zend_bool escapeValue = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|zb", &conditions, &value, &escapeValue) == FAILURE) {
		return;
	}

	mysqlWhere(getThis(), RECKEY_HAVING, conditions, value, "OR", escapeValue);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto mysqlWhereGroupStart */
static void mysqlWhereGroupStart(zval *instance, const char *pType)
{
	zend_long recType = RECKEY_WHERE;
	zval *where, *pRec, *pWhereGroupPrefix, *pWhereGroupDepth;
	where = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_where"), 1, NULL);
	pRec = zend_hash_index_find(Z_ARRVAL_P(where), recType);
	pWhereGroupPrefix = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_whereGroupPrefix"), 1, NULL);
	pWhereGroupDepth = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_WhereGroupDepth"), 1, NULL);

	// init rec property
	if (!pRec) {
		zval rec;
		pRec = &rec;
		array_init(pRec);
		zend_hash_index_add(Z_ARRVAL_P(where), recType, pRec);
	}

	zend_string *type, *tstr;
	type = mysqlGetType(Z_TYPE_P(pWhereGroupPrefix) == IS_TRUE, pRec, pType);
	tstr = strpprintf(0, "%s(", ZSTR_VAL(type));

	add_next_index_str(pRec, tstr);

	ZVAL_FALSE(pWhereGroupPrefix);
	++Z_LVAL_P(pWhereGroupDepth);
	zend_string_release(type);
}
/* }}} */

/* {{{ proto mysqlWhereGroupEnd */
static void mysqlWhereGroupEnd(zval *instance)
{
	zend_long recType = RECKEY_WHERE;
	zval *where, *pRec, *pWhereGroupPrefix, *pWhereGroupDepth;
	where = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_where"), 1, NULL);
	pRec = zend_hash_index_find(Z_ARRVAL_P(where), recType);
	pWhereGroupPrefix = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_whereGroupPrefix"), 1, NULL);
	pWhereGroupDepth = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_WhereGroupDepth"), 1, NULL);

	if (Z_LVAL_P(pWhereGroupDepth) == 0) {
		return;
	}
	// init rec property
	if (!pRec) {
		zval rec;
		pRec = &rec;
		array_init(pRec);
		zend_hash_index_add(Z_ARRVAL_P(where), recType, pRec);
	}
	zend_string *tstr;
	tstr = zend_string_init(ZEND_STRL(")"), 0);

	add_next_index_str(pRec, tstr);

	ZVAL_TRUE(pWhereGroupPrefix);
	--Z_LVAL_P(pWhereGroupDepth);
}
/* }}} */

/* {{{ proto whereGroupStart */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, whereGroupStart)
{
	mysqlWhereGroupStart(getThis(), "AND");
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto orWhereGroupStart */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orWhereGroupStart)
{
	mysqlWhereGroupStart(getThis(), "OR");
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto notWhereGroupStart */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, notWhereGroupStart)
{
	mysqlWhereGroupStart(getThis(), "NOT");
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto orNotWhereGroupStart */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orNotWhereGroupStart)
{
	mysqlWhereGroupStart(getThis(), "OR NOT");
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto whereGroupEnd */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, whereGroupEnd)
{
	mysqlWhereGroupEnd(getThis());
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

static zend_string * mysqlCompileWhere(zval *instance, zend_long recType)
{
	zval *where, *pRec, *pWhereGroupPrefix, *pWhereGroupDepth;
	where = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_where"), 1, NULL);
	pRec = zend_hash_index_find(Z_ARRVAL_P(where), recType);
	pWhereGroupPrefix = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_whereGroupPrefix"), 1, NULL);
	pWhereGroupDepth = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_WhereGroupDepth"), 1, NULL);

	if (!pRec) {
		return ZSTR_EMPTY_ALLOC();
	}
	while (Z_LVAL_P(pWhereGroupDepth) > 0) {
		mysqlWhereGroupEnd(instance);
	}
	zend_string *delim = zend_string_init(ZEND_STRL("\n"), 0), *ret;
	zval wh;
	php_implode(delim, pRec, &wh);
	zend_string_release(delim);
	ret = zend_string_copy(Z_STR(wh));
	zval_ptr_dtor(&wh);
	// reset after compiled
	zval_ptr_dtor(pRec);
	array_init(pRec);
	ZVAL_FALSE(pWhereGroupPrefix);
	return ret;
}

/* {{{ proto select */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, select)
{
	zend_string *select;
	zend_bool escapeValue = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &select, &escapeValue) == FAILURE) {
		return;
	}

	zval selects, *ret, *pSelect, *instance = getThis(), *pData;
	zend_string *delim, *tstr;
	array_init(&selects);
	delim = zend_string_init(ZEND_STRL(","), 0);
	php_explode(delim, select, &selects, ZEND_LONG_MAX);
	zend_string_release(delim);

	// escape if need be
	if (escapeValue) {
		zval t;
		mysqlEscape(&t, &selects);
		zval_ptr_dtor(&selects);
		ret = &t;
	} else {
		ret = &selects;
	}

	pSelect = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_select"), 1, NULL);
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(ret), pData) {
		tstr = php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3);
		add_next_index_str(pSelect, tstr);
	} ZEND_HASH_FOREACH_END();

	zval_ptr_dtor(ret);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto distinct */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, distinct)
{
	zval *pDistinct, *instance = getThis();
	zend_bool distinct = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &distinct) == FAILURE) {
		return;
	}

	pDistinct = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_distinct"), 1, NULL);

	ZVAL_BOOL(pDistinct, distinct);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto from */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, from)
{
	zval *from, *pFrom, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &from) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(from) != IS_ARRAY && Z_TYPE_P(from) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or array");
		return;
	}

	// form table name
	if (Z_TYPE_P(from) == IS_STRING) {
		zend_string *delim = zend_string_init(ZEND_STRL(","), 0);
		zval froms;
		array_init(&froms);
		php_explode(delim, Z_STR_P(from), &froms, ZEND_LONG_MAX);
		zend_string_release(delim);
		from = &froms;
	} else {
		zval_add_ref(from);
	}
	// foreach
	zend_ulong h;
	zend_string *key, *tableName, *value;
	zval *pData;
	pFrom = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_from"), 1, NULL);
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(from), h, key, pData) {
		if (Z_TYPE_P(pData) != IS_STRING) {
			continue;
		}
		if (key) {
			key = php_trim(key, ZEND_STRL(" "), 3);
			tableName = php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3);
			value = strpprintf(0, "`%s` AS `%s`", ZSTR_VAL(tableName), ZSTR_VAL(key));
			add_next_index_str(pFrom, value);
			zend_string_release(key);
			zend_string_release(tableName);
		} else {
			tableName = php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3);
			value = strpprintf(0, "`%s`", ZSTR_VAL(tableName));
			add_next_index_str(pFrom, value);
			zend_string_release(tableName);
		}
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(from);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto join */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, join)
{
	zval *join, *pJoin, *instance = getThis();
	zend_string *condition, *type = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zS|S", &join, &condition, &type) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(join) != IS_ARRAY && Z_TYPE_P(join) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or array");
		return;
	}

	// codition
	if (strchr(ZSTR_VAL(condition), '=')) {
		condition = strpprintf(0, "ON (%s)", ZSTR_VAL(condition));
	} else {
		condition = strpprintf(0, "USING (%s)", ZSTR_VAL(condition));
	}
	// join type
	if (type && 0 == strcasecmp(ZSTR_VAL(type), "left")) {
		type = zend_string_init(ZEND_STRL("LEFT JOIN"), 0);
	} else if (type && 0 == strcasecmp(ZSTR_VAL(type), "right")) {
		type = zend_string_init(ZEND_STRL("RIGHT JOIN"), 0);
	} else {
		type = zend_string_init(ZEND_STRL("INNER JOIN"), 0);
	}
	// join table name
	if (Z_TYPE_P(join) == IS_STRING) {
		zend_string *delim = zend_string_init(ZEND_STRL(","), 0);
		zval joins;
		array_init(&joins);
		php_explode(delim, Z_STR_P(join), &joins, ZEND_LONG_MAX);
		zend_string_release(delim);
		join = &joins;
	} else {
		zval_add_ref(join);
	}
	// foreach
	zend_ulong h;
	zend_string *key, *tableName, *value;
	zval *pData;
	pJoin = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_join"), 1, NULL);
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(join), h, key, pData) {
		if (Z_TYPE_P(pData) != IS_STRING) {
			continue;
		}
		if (key) {
			key = php_trim(key, ZEND_STRL(" "), 3);
			tableName = php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3);
			value = strpprintf(0, "%s `%s` AS `%s` %s", ZSTR_VAL(type), ZSTR_VAL(tableName), ZSTR_VAL(key), ZSTR_VAL(condition));
			add_next_index_str(pJoin, value);
			zend_string_release(key);
			zend_string_release(tableName);
		} else {
			tableName = php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3);
			value = strpprintf(0, "%s `%s` %s", ZSTR_VAL(type), ZSTR_VAL(tableName), ZSTR_VAL(condition));
			add_next_index_str(pJoin, value);
			zend_string_release(tableName);
		}
	} ZEND_HASH_FOREACH_END();
	zend_string_release(condition);
	zend_string_release(type);
	zval_ptr_dtor(join);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto limit */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, limit)
{
	zend_long limit, offset = 0;
	zval *pValue, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|l", &limit, &offset) == FAILURE) {
		return;
	}

	if (limit >= 0) {
		pValue = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_limit"), 1, NULL);
		ZVAL_LONG(pValue, limit);
		if (offset >= 0) {
			pValue = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_offset"), 1, NULL);
			ZVAL_LONG(pValue, offset);
		}
	}

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto limitPage */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, limitPage)
{
	zend_long limit, page = 1;
	zval *pValue, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|l", &limit, &page) == FAILURE) {
		return;
	}

	if (limit >= 0) {
		pValue = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_limit"), 1, NULL);
		ZVAL_LONG(pValue, limit);
		if (page >= 1) {
			zend_long offset = limit * (page - 1);
			pValue = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_offset"), 1, NULL);
			ZVAL_LONG(pValue, offset);
		}
	}

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto orderBy */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orderBy)
{
	zval *orderBy, *pOrderBy, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &orderBy) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(orderBy) != IS_ARRAY && Z_TYPE_P(orderBy) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or array");
		return;
	}

	// orderBy field name
	if (Z_TYPE_P(orderBy) == IS_STRING) {
		zend_string *delim = zend_string_init(ZEND_STRL(","), 0);
		zval orderBys;
		array_init(&orderBys);
		php_explode(delim, Z_STR_P(orderBy), &orderBys, ZEND_LONG_MAX);
		zend_string_release(delim);
		orderBy = &orderBys;
	} else {
		zval_add_ref(orderBy);
	}
	// foreach
	zend_ulong h;
	zval *pData;
	pOrderBy = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_orderBy"), 1, NULL);
	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(orderBy), h, pData) {
		if (Z_TYPE_P(pData) != IS_STRING) {
			continue;
		}
		add_next_index_str(pOrderBy, php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3));
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(orderBy);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto groupBy */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, groupBy)
{
	zval *groupBy, *pGroupBy, *instance = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &groupBy) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(groupBy) != IS_ARRAY && Z_TYPE_P(groupBy) != IS_STRING) {
		php_error_docref(NULL, E_ERROR, "expects parameter 1 to be string or array");
		return;
	}

	// orderBy field name
	if (Z_TYPE_P(groupBy) == IS_STRING) {
		zend_string *delim = zend_string_init(ZEND_STRL(","), 0);
		zval orderBys;
		array_init(&orderBys);
		php_explode(delim, Z_STR_P(groupBy), &orderBys, ZEND_LONG_MAX);
		zend_string_release(delim);
		groupBy = &orderBys;
	} else {
		zval_add_ref(groupBy);
	}
	// foreach
	zend_ulong h;
	zval *pData;
	pGroupBy = zend_read_property(mysqlSqlBuilderCe, instance, ZEND_STRL("_groupBy"), 1, NULL);
	ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(groupBy), h, pData) {
		if (Z_TYPE_P(pData) != IS_STRING) {
			continue;
		}
		add_next_index_str(pGroupBy, php_trim(Z_STR_P(pData), ZEND_STRL(" "), 3));
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(groupBy);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

static zend_string * mysqlGetSql(zval *instance)
{
	smart_str buf = {0};
	smart_str_appendl(&buf, ZEND_STRL("SELECT "));


	zend_string *ret = zend_string_copy(buf.s);
	smart_str_free(&buf);
	return ret;
}

/* {{{ proto getSql */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, getSql)
{
	RETURN_STR(mysqlGetSql(getThis()));
}
/* }}} */

/* {{{ proto __toString */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, __toString)
{
	RETURN_STR(mysqlGetSql(getThis()));
}
/* }}} */






