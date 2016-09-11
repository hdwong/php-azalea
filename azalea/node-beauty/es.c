/*
 * es.c
 *
 * Created by Bun Wong on 16-9-11.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/es.h"

#include "Zend/zend_smart_str.h"  // for smart_str_*
#include "ext/standard/php_string.h"  // for php_addcslashes
#include "ext/json/php_json.h"  // for php_json_*
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*
#include "ext/standard/php_var.h"

zend_class_entry *azalea_node_beauty_es_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_node_beauty_es_methods[] = {
	PHP_ME(azalea_node_beauty_es, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_es, __init, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_es, escape, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_es, ping, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_es, query, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_es, index, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_es, delete, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_es, commit, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(es)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(ElasticSearchModel), azalea_node_beauty_es_methods);
	azalea_node_beauty_es_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_es_ce->ce_flags |= ZEND_ACC_FINAL;
	zend_declare_property_null(azalea_node_beauty_es_ce, ZEND_STRL("_indexes"), ZEND_ACC_PRIVATE);
	zend_declare_property_null(azalea_node_beauty_es_ce, ZEND_STRL("_deletes"), ZEND_ACC_PRIVATE);
	zend_declare_property_long(azalea_node_beauty_es_ce, ZEND_STRL("_waiting"), 0, ZEND_ACC_PRIVATE);

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_es, __construct) {}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_es, __init)
{
	zval indexes, deletes;

	array_init(&indexes);
	zend_update_property(azalea_node_beauty_es_ce, getThis(), ZEND_STRL("_indexes"), &indexes);
	zval_ptr_dtor(&indexes);
	array_init(&deletes);
	zend_update_property(azalea_node_beauty_es_ce, getThis(), ZEND_STRL("_deletes"), &deletes);
	zval_ptr_dtor(&deletes);
}
/* }}} */

/* {{{ proto escape */
PHP_METHOD(azalea_node_beauty_es, escape)
{
	zend_string *str;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &str) == FAILURE) {
		return;
	}

	RETURN_STR(php_addcslashes(str, 0, ZEND_STRL("*-+[]!\\/\"~^(){}:")));
}
/* }}} */

/* {{{ proto ping */
PHP_METHOD(azalea_node_beauty_es, ping)
{
	zval ret, arg1;

	ZVAL_STRINGL(&arg1, "ping", sizeof("ping") - 1);
	zend_call_method_with_1_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1);
	zval_ptr_dtor(&arg1);
	RETURN_ZVAL(&ret, 0, 0);
}
/* }}} */

/* {{{ proto query */
PHP_METHOD(azalea_node_beauty_es, query)
{
	zend_string *type, *queryBody;
	zval *query = NULL, ret, arg1, arg2;
	zend_long size = 10, page = 1, offset;
	smart_str buf = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|all", &type, &query, &size, &page) == FAILURE) {
		return;
	}

	if (size < 1) {
		size = 1;
	}
	if (page < 1) {
		page = 1;
	}
	offset = size * (page - 1);
	// json querybody
	if (query) {
		php_json_encode(&buf, query, 0);
		smart_str_0(&buf);
		queryBody = zend_string_copy(buf.s);
		smart_str_free(&buf);
	} else {
		// json an empty object
		queryBody = zend_string_init(ZEND_STRL("{}"), 0);
	}

	ZVAL_STRINGL(&arg1, "document", sizeof("document") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("type"), zend_string_copy(type));
	add_assoc_str_ex(&arg2, ZEND_STRL("query"), queryBody);
	add_assoc_long_ex(&arg2, ZEND_STRL("size"), size);
	add_assoc_long_ex(&arg2, ZEND_STRL("from"), offset);
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "get", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETURN_ZVAL(&ret, 0, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto commit */
static void esCommit(zval *instance)
{
	zend_string *key;
	zval *indexes, *deletes, *waitingCount, *pData, arg1, arg2;
	smart_str buf = {0};

	waitingCount = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_waiting"), 1, NULL);
	indexes = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_indexes"), 1, NULL);
	deletes = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_deletes"), 1, NULL);

	ZVAL_STRINGL(&arg1, "document", sizeof("document") - 1);
	// index
	if (zend_hash_num_elements(Z_ARRVAL_P(indexes))) {
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(indexes), key, pData) {
			if (key) {
				// index a document
				array_init(&arg2);
				add_assoc_str_ex(&arg2, ZEND_STRL("type"), zend_string_copy(key));
				php_json_encode(&buf, pData, 0);
				smart_str_0(&buf);
				add_assoc_str_ex(&arg2, ZEND_STRL("data"), zend_string_copy(buf.s));
				smart_str_free(&buf);
				zend_call_method_with_2_params(instance, azalea_service_ce, NULL, "post", NULL, &arg1, &arg2);
				zval_ptr_dtor(&arg2);
			}
		} ZEND_HASH_FOREACH_END();
		zval_ptr_dtor(indexes);
		array_init(indexes);
	}
	// delete
	if (zend_hash_num_elements(Z_ARRVAL_P(deletes))) {
		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(deletes), key, pData) {
			if (key) {
				// index a document
				array_init(&arg2);
				add_assoc_str_ex(&arg2, ZEND_STRL("type"), zend_string_copy(key));
				php_json_encode(&buf, pData, 0);
				smart_str_0(&buf);
				add_assoc_str_ex(&arg2, ZEND_STRL("data"), zend_string_copy(buf.s));
				smart_str_free(&buf);
				zend_call_method_with_2_params(instance, azalea_service_ce, NULL, "delete", NULL, &arg1, &arg2);
				zval_ptr_dtor(&arg2);
			}
		} ZEND_HASH_FOREACH_END();
		zval_ptr_dtor(deletes);
		array_init(deletes);
	}
	zval_ptr_dtor(&arg1);
	ZVAL_LONG(waitingCount, 0);
}
/* }}} */

/* {{{ proto index */
PHP_METHOD(azalea_node_beauty_es, index)
{
	zend_string *type, *docId, *key;
	zval *values, *instance = getThis(), *indexes, *waitingCount, typeIndexes, *pTypeIndexes, doc, *pData;
	zend_bool autoCommit = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SSa|b", &type, &docId, &values, &autoCommit) == FAILURE) {
		return;
	}

	waitingCount = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_waiting"), 1, NULL);
	indexes = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_indexes"), 1, NULL);
	pTypeIndexes = zend_hash_find(Z_ARRVAL_P(indexes), type);
	if (!pTypeIndexes) {
		pTypeIndexes = &typeIndexes;
		array_init(pTypeIndexes);
		add_assoc_zval_ex(indexes, ZSTR_VAL(type), ZSTR_LEN(type), pTypeIndexes);
	}
	// init doc
	array_init(&doc);
	add_assoc_str_ex(&doc, ZEND_STRL("id"), zend_string_copy(docId));
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(values), key, pData) {
		if (key && Z_TYPE_P(pData) > IS_NULL && Z_TYPE_P(pData) <= IS_STRING) {
			// value is scalar
			add_assoc_zval_ex(&doc, ZSTR_VAL(key), ZSTR_LEN(key), pData);
			zval_add_ref(pData);
		}
	} ZEND_HASH_FOREACH_END();
	add_next_index_zval(pTypeIndexes, &doc);
	// auto commit
	if (autoCommit || ++Z_LVAL_P(waitingCount) >= ES_AUTO_COMMIT) {
		esCommit(instance);
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto delete */
PHP_METHOD(azalea_node_beauty_es, delete)
{
	zend_string *type, *docId;
	zval *instance = getThis(), *deletes, *waitingCount, typeDeletes, *pTypeDeletes;
	zend_bool autoCommit = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS|b", &type, &docId, &autoCommit) == FAILURE) {
		return;
	}

	waitingCount = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_waiting"), 1, NULL);
	deletes = zend_read_property(azalea_node_beauty_es_ce, instance, ZEND_STRL("_deletes"), 1, NULL);
	pTypeDeletes = zend_hash_find(Z_ARRVAL_P(deletes), type);
	if (!pTypeDeletes) {
		pTypeDeletes = &typeDeletes;
		array_init(pTypeDeletes);
		add_assoc_zval_ex(deletes, ZSTR_VAL(type), ZSTR_LEN(type), pTypeDeletes);
	}
	add_next_index_str(pTypeDeletes, zend_string_copy(docId));
	// auto commit
	if (autoCommit || ++Z_LVAL_P(waitingCount) >= ES_AUTO_COMMIT) {
		esCommit(instance);
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto commit */
PHP_METHOD(azalea_node_beauty_es, commit)
{
	esCommit(getThis());

	RETURN_TRUE;
}
/* }}} */
