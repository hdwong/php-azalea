/*
 * azalea/model.c
 *
 * Created by Bun Wong on 16-7-15.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/loader.h"
#include "azalea/config.h"
#include "azalea/request.h"
#include "azalea/model.h"
#include "azalea/exception.h"

#include "Zend/zend_smart_str.h"	// for smart_str_*
#include "ext/standard/php_string.h"	// for php_trim function
#include "ext/standard/php_filestat.h"	// for php_stat
#include "ext/standard/php_var.h"	// for php_var_serialize
#include "Zend/zend_interfaces.h"	// for zend_call_method_with_*

#ifdef WITH_PINYIN
#include "azalea/ext-models/pinyin.h"
#endif
#ifdef WITH_MYSQLND
#include "azalea/ext-models/mysqlnd.h"
#endif

zend_class_entry *azaleaModelCe;

/* {{{ class Azalea\Model methods
 */
static zend_function_entry azalea_model_methods[] = {
	PHP_ME(azalea_model, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_model, getRequest, NULL, ZEND_ACC_PROTECTED | ZEND_ACC_FINAL)
	PHP_ME(azalea_model, loadModel, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC | ZEND_ACC_FINAL)
	PHP_ME(azalea_model, getModel, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC | ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(model)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Model), azalea_model_methods);
	azaleaModelCe = zend_register_internal_class(&ce);
	azaleaModelCe->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	zend_declare_property_null(azaleaModelCe, ZEND_STRL("_modelname"), ZEND_ACC_PRIVATE);

#ifdef WITH_PINYIN
	AZALEA_EXT_MODEL_STARTUP(pinyin);
#endif
#ifdef WITH_MYSQLND
	AZALEA_EXT_MODEL_STARTUP(mysqlnd);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_model, __construct) {}
/* }}} */

/* {{{ proto getRequest */
PHP_METHOD(azalea_model, getRequest)
{
	azalea_request_t *pReq = azaleaGetRequest();
	RETURN_ZVAL(pReq, 1, 0);
}
/* }}} */

/* {{{ proto loadModel */
PHP_METHOD(azalea_model, loadModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_model, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/** {{{ int azaleaLoadModel(zend_execute_data *execute_data, zval *return_value) */
void azaleaLoadModel(INTERNAL_FUNCTION_PARAMETERS)
{
	zval *models;
	zend_string *filename, *lcName, *tstr;
	int argc, offset = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "+", &models, &argc) == FAILURE) {
		return;
	}

	do {
		if (Z_TYPE_P(models + offset) != IS_STRING) {
			continue;
		}
		zval exists;
		lcName = zend_string_init(Z_STRVAL_P(models + offset), Z_STRLEN_P(models + offset), 0);
		zend_str_tolower(ZSTR_VAL(lcName), ZSTR_LEN(lcName));
		// load model file
		filename = strpprintf(0, "%s%c%s.php", ZSTR_VAL(AZALEA_G(modelsPath)), DEFAULT_SLASH, ZSTR_VAL(lcName));
		zend_string_release(lcName);
		// check file exists
		php_stat(ZSTR_VAL(filename), (php_stat_len) ZSTR_LEN(filename), FS_IS_R, &exists);
		if (Z_TYPE(exists) == IS_FALSE) {
			tstr = strpprintf(0, "Model file `%s` not found.", ZSTR_VAL(filename));
			throw404(tstr);
			zend_string_release(tstr);
			zend_string_release(filename);
			RETURN_FALSE;
		}
		// require model file
		if (!azaleaRequire(ZSTR_VAL(filename), 1)) {
			// 保持 Exception throws
			zend_string_release(filename);
			RETURN_FALSE;
		}
		zend_string_release(filename);
	} while (++offset < argc);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto azaleaGetExtModelClassEntry */
static zend_class_entry * azaleaModelGetExtModelClassEntry(zend_string *name)
{
#ifdef WITH_PINYIN
	if (0 == strcmp("pinyin", ZSTR_VAL(name))) {
		return azalea_ext_model_pinyin_ce;
	}
#endif
#ifdef WITH_MYSQLND
	if (0 == strcmp("mysqlnd", ZSTR_VAL(name))) {
		return azalea_ext_model_mysqlnd_ce;
	}
#endif
	return NULL;
}
/* }}} */

/** {{{ int azaleaGetModel(zend_execute_data *execute_data, zval *return_value) */
void azaleaGetModel(INTERNAL_FUNCTION_PARAMETERS)
{
	zend_string *modelName;
	zend_string *lcName, *nodeBeautyLcName, *extModelLcName, *lcClassName, *cacheId, *modelClass, *tstr;
	zend_class_entry *ce = NULL;
	azalea_model_t *instance = NULL, rv = {{0}};
	zval *conf, *field, *arg1 = NULL;
	zend_bool hasError = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|z", &modelName, &arg1) == FAILURE) {
		return;
	}

	lcName = zend_string_tolower(modelName);
//	lcName = zend_string_init(ZSTR_VAL(modelName), ZSTR_LEN(modelName), 0);
//	zend_str_tolower(ZSTR_VAL(lcName), ZSTR_LEN(lcName));
	modelClass = strpprintf(0, "%sModel", ZSTR_VAL(lcName));
	ZSTR_VAL(modelClass)[0] = toupper(ZSTR_VAL(modelClass)[0]); // ucfirst

	lcClassName = zend_string_tolower(modelClass);
	if (arg1) {	// 生成 Model Cache Id
		php_serialize_data_t var_hash;
		smart_str buf = {0};
		PHP_VAR_SERIALIZE_INIT(var_hash);
		php_var_serialize(&buf, arg1, &var_hash);
		PHP_VAR_SERIALIZE_DESTROY(var_hash);
		cacheId = strpprintf(0, "%s-%s", ZSTR_VAL(lcClassName), ZSTR_VAL(buf.s));
		smart_str_free(&buf);
	} else {
		cacheId = zend_string_copy(lcClassName);
	}

	instance = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), cacheId);
	if (instance) {
		ce = Z_OBJCE_P(instance);
	} else {
		do {
			// try to load extend models
			conf = azaleaConfigSubFindEx(ZEND_STRL("ext-model"), NULL, 0);
			if (conf && Z_TYPE_P(conf) == IS_ARRAY && (conf = zend_hash_find(Z_ARRVAL_P(conf), lcName)) &&
					Z_TYPE_P(conf) == IS_STRING) {
				if (!is_numeric_string(Z_STRVAL_P(conf), Z_STRLEN_P(conf), NULL, NULL, 0)) {
					extModelLcName = zend_string_tolower(Z_STR_P(conf));
				} else {
					extModelLcName = zend_string_copy(lcName);
				}
				ce = azaleaModelGetExtModelClassEntry(extModelLcName);
				zend_string_release(extModelLcName);
			}
			if (!ce) {
				// load local model file
				zval modelPath, exists;
				ZVAL_STR(&modelPath, zend_string_dup(AZALEA_G(modelsPath), 0));
				tstr = Z_STR(modelPath);
				Z_STR(modelPath) = strpprintf(0, "%s%c%s.php", ZSTR_VAL(tstr), DEFAULT_SLASH, ZSTR_VAL(lcName));
				zend_string_release(tstr);
				// check file exists
				php_stat(Z_STRVAL(modelPath), (php_stat_len) Z_STRLEN(modelPath), FS_IS_R, &exists);
				if (Z_TYPE(exists) == IS_FALSE) {
					tstr = strpprintf(0, "Model file `%s` not found.", Z_STRVAL(modelPath));
					throw404(tstr);
					zend_string_release(tstr);
					hasError = 1;
					break;
				}
				// require model file
				if (!azaleaRequire(Z_STRVAL(modelPath), 1)) {
					// 保持 Exception throws
					zval_ptr_dtor(&modelPath);
					hasError = 1;
					break;
				}
				zval_ptr_dtor(&modelPath);
				// check model class exists
				if (!(ce = zend_hash_find_ptr(EG(class_table), lcClassName))) {
					tstr = strpprintf(0, "Model class `%s` not found.", ZSTR_VAL(modelClass));
					throw404(tstr);
					zend_string_release(tstr);
					hasError = 1;
					break;
				}
			}
			// check super class name
			if (!instanceof_function(ce, azaleaModelCe)) {
				throw404Str(ZEND_STRL("Model class must be an instance of " AZALEA_NS_NAME(Model) "."));
				hasError = 1;
				break;
			}
			// init controller instance
			instance = &rv;
			object_init_ex(instance, ce);
			if (!instance) {
				throw404Str(ZEND_STRL("Model initialization is failed."));
				hasError = 1;
				break;
			}
			// save lcName to property
			zend_update_property_str(azaleaModelCe, instance, ZEND_STRL("_modelname"), lcName);
			// call __init method
			if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__init"))) {
				zend_call_method(instance, ce, NULL, ZEND_STRL("__init"), NULL, arg1 ? 1 : 0, arg1, NULL);
			}
			if (!EG(exception)) {
				// 没有异常则加入缓存
				add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(cacheId), ZSTR_LEN(cacheId), instance);
				// zval_add_ref(instance); 不需要增加计数
			}
		} while (0);
	}
	zend_string_release(lcName);
	zend_string_release(modelClass);
	zend_string_release(lcClassName);
	zend_string_release(cacheId);
	if (hasError) {
		// has error and RETURN_FALSE
		RETURN_FALSE;
	}
	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */
