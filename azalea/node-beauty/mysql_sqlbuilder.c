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
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, where, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_mysql_sqlbuilder, orWhere, NULL, ZEND_ACC_PUBLIC)
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
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, __construct)
{
	zval value;
	array_init(&value);
	zend_update_property(mysqlSqlBuilderCe, getThis(), ZEND_STRL("_where"), &value);
	zval_ptr_dtor(&value);
}
/* }}} */

static zend_string * mysqlGetType(zend_bool whereGroupPrefix, zval *pRec, zend_string *type)
{
	// check in whereGroupPrefix
	if (whereGroupPrefix == 0 || zend_hash_num_elements(Z_ARRVAL_P(pRec)) == 0) {
		type = zend_string_init(ZEND_STRL(""), 0);
	} else {
		// get type [AND, OR]
		if (type && 0 == strcasecmp("OR", ZSTR_VAL(type))) {
			type = zend_string_init(ZEND_STRL("OR "), 0);
		} else {
			type = zend_string_init(ZEND_STRL("AND "), 0);
		}
	}
	php_printf("[%s]", ZSTR_VAL(type));
	return type;
}

/* {{{ proto mysqlWhere */
static void mysqlWhere(zval *instance, zend_long recType, zval *conditions, zval *value, zend_string *type, zend_bool escape)
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
	zend_string *key, *op, *segment, *tstr;
	zval *pData;
	char *pOp;
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(conditions), key, pData) {
		// type
		type = mysqlGetType(Z_TYPE_P(pWhereGroupPrefix) == IS_TRUE, pRec, type);
		// field and OP
		key = php_trim(key, ZEND_STRL(" "), 3);
		pOp = strchr(ZSTR_VAL(key), ' ');
		if (pOp) {
			// get OP
			tstr = key;
			op = zend_string_init(pOp + 1, ZSTR_VAL(key) + ZSTR_LEN(key) - pOp - 1, 0);
			key = zend_string_init(ZSTR_VAL(key), pOp - ZSTR_VAL(key), 0);
			zend_string_release(tstr);
		} else {
			op = zend_string_init(ZEND_STRL("="), 0);
		}
		// build segment
		segment = strpprintf(0, "%s?? %s ?", ZSTR_VAL(type), ZSTR_VAL(op));
		zval binds;
		array_init(&binds);
		add_next_index_str(&binds, zend_string_copy(key));
		add_next_index_zval(&binds, pData);
		zval_add_ref(pData);
		tstr = mysqlCompileBinds(segment, &binds);  // where string
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
	zend_string *type = NULL;
	zend_bool escape = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|zSb", &conditions, &value, &type, &escape) == FAILURE) {
		return;
	}

	mysqlWhere(getThis(), RECKEY_WHERE, conditions, value, type, escape);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/* {{{ proto orWhere */
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orWhere)
{
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */
