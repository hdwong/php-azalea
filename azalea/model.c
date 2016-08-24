/*
 * azalea/model.c
 *
 * Created by Bun Wong on 16-7-15.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/loader.h"
#include "azalea/config.h"
#include "azalea/request.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"

#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*
#include "ext/standard/php_string.h"  // for php_trim function
#include "ext/standard/php_filestat.h"	// for php_stat

zend_class_entry *azalea_model_ce;

/* {{{ class Azalea\Model methods
 */
static zend_function_entry azalea_model_methods[] = {
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
	azalea_model_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_model_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	return SUCCESS;
}
/* }}} */

/* {{{ proto getRequest */
PHP_METHOD(azalea_model, getRequest)
{
	object_init_ex(return_value, azalea_request_ce);
}
/* }}} */

/* {{{ proto loadModel */
PHP_METHOD(azalea_model, loadModel)
{
	azaleaLoadModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */

/* {{{ proto getModel */
PHP_METHOD(azalea_model, getModel)
{
	azaleaGetModel(INTERNAL_FUNCTION_PARAM_PASSTHRU, getThis());
}
/* }}} */

/** {{{ int azaleaLoadModel(zend_execute_data *execute_data, zval *return_value, zval *instance) */
void azaleaLoadModel(INTERNAL_FUNCTION_PARAMETERS, zval *form)
{
	zval *models;
	int argc, offset = 0;
	zend_bool hasError = 1;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "+", &models, &argc) == FAILURE) {
		return;
	}

	zend_string *filename, *tstr;
	do {
		if (Z_TYPE_P(models + offset) != IS_STRING) {
			continue;
		}
		zval exists;
		zend_string *lcName = zend_string_init(Z_STRVAL_P(models + offset), Z_STRLEN_P(models + offset), 0);
		zend_str_tolower(ZSTR_VAL(lcName), ZSTR_LEN(lcName));
		// load model file
		filename = strpprintf(0, "%s%c%s.php", ZSTR_VAL(AZALEA_G(modelsPath)), DEFAULT_SLASH, ZSTR_VAL(lcName));
		zend_string_release(lcName);
		// check file exists
		php_stat(ZSTR_VAL(filename), (php_stat_len) ZSTR_LEN(filename), FS_IS_R, &exists);
		if (Z_TYPE(exists) == IS_FALSE) {
			tstr = strpprintf(0, "Model file `%s` not found.", ZSTR_VAL(filename));
			throw404(tstr);
			break;
		}
		// require model file
		if (!azaleaRequire(ZSTR_VAL(filename), 1)) {
			tstr = strpprintf(0, "Model file `%s` compile error.", ZSTR_VAL(filename));
			throw404(tstr);
			break;
		}
		zend_string_release(filename);
		hasError = 0;
	} while (++offset < argc);

	if (hasError) {
		zend_string_release(filename);
		zend_string_release(tstr);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/** {{{ int azaleaGetModel(zend_execute_data *execute_data, zval *return_value, zval *instance) */
void azaleaGetModel(INTERNAL_FUNCTION_PARAMETERS, zval *from)
{
	zend_string *modelName;
	zend_string *name, *lcName, *lcClassName, *modelClass, *tstr;
	zend_class_entry *ce;
	azalea_model_t *instance = NULL, rv = {{0}};

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &modelName) == FAILURE) {
		return;
	}

	lcName = zend_string_init(ZSTR_VAL(modelName), ZSTR_LEN(modelName), 0);
	zend_str_tolower(ZSTR_VAL(lcName), ZSTR_LEN(lcName));
	name = zend_string_dup(lcName, 0);
	ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);  // ucfirst
	modelClass = strpprintf(0, "%sModel", ZSTR_VAL(name));
	zend_string_release(name);

	lcClassName = zend_string_tolower(modelClass);
	instance = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), lcClassName);
	if (instance) {
		ce = Z_OBJCE_P(instance);
	} else {
		ce = azaleaServiceGetNodeBeautyClassEntry(lcName);  // try to load node-beauty models
		do {
			// load from extension
			if (ce) {
				zval *conf;
				if ((conf = azaleaConfigSubFind("node-beauty", ZSTR_VAL(lcName)))) {
					convert_to_boolean(conf);
					if (Z_TYPE_P(conf) == IS_TRUE) {
						break;
					}
				}
			}
			// load model file
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
				RETURN_FALSE;
			}
			// require model file
			if (!azaleaRequire(Z_STRVAL(modelPath), 1)) {
				tstr = strpprintf(0, "Model file `%s` compile error.", Z_STRVAL(modelPath));
				throw404(tstr);
				zend_string_release(tstr);
				RETURN_FALSE;
			}
			zval_ptr_dtor(&modelPath);
			// check model class exists
			if (!(ce = zend_hash_find_ptr(EG(class_table), lcClassName))) {
				tstr = strpprintf(0, "Model class `%s` not found.", ZSTR_VAL(modelClass));
				throw404(tstr);
				zend_string_release(tstr);
				RETURN_FALSE;
			}
		} while (0);
		// check super class name
		if (!instanceof_function(ce, azalea_model_ce)) {
			throw404Str(ZEND_STRL("Model class must be an instance of " AZALEA_NS_NAME(Model) "."));
			RETURN_FALSE;
		}
		// init controller instance
		instance = &rv;
		object_init_ex(instance, ce);
		if (!instance) {
			throw404Str(ZEND_STRL("Model initialization is failed."));
			RETURN_FALSE;
		}
		// service model construct
		if (instanceof_function(ce, azalea_service_ce)) {
			zval *field;
			if ((field = zend_read_property(azalea_service_ce, instance, ZEND_STRL("service"), 1, NULL)) &&
					Z_TYPE_P(field) == IS_STRING) {
				zend_string_release(lcName);
				lcName = zend_string_copy(Z_STR_P(field));
			}
			if (!(field = zend_read_property(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), 0, NULL)) ||
					Z_TYPE_P(field) != IS_STRING) {
				// get serviceUrl from config
				if (!(field = azaleaConfigSubFind("service", "url")) || Z_TYPE_P(field) != IS_STRING) {
					throw404Str(ZEND_STRL("Service url not set."));
					RETURN_FALSE;
				}
				zend_string *serviceUrl, *t;
				t = zend_string_dup(Z_STR_P(field), 0);
				tstr = php_trim(t, ZEND_STRL("/"), 2);
				serviceUrl = strpprintf(0, "%s/%s", ZSTR_VAL(tstr), ZSTR_VAL(lcName));
				zend_update_property_str(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), serviceUrl);
				zend_string_release(t);
				zend_string_release(tstr);
				zend_string_release(serviceUrl);
			}
		}
		// call __init method
		if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__init"))) {
			zend_call_method_with_0_params(instance, ce, NULL, "__init", NULL);
		}
		// cache instance
		add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(lcClassName), ZSTR_LEN(lcClassName), instance);
	}
	zend_string_release(lcName);
	zend_string_release(modelClass);
	zend_string_release(lcClassName);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */
