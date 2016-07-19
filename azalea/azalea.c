/*
 * azalea/azalea.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "azalea.h"
#include "php_azalea.h"

#include "azalea/namespace.h"
#include "azalea/config.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"

#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

#include "ext/date/php_date.h"
#include "ext/standard/php_rand.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#elif defined(NETWARE)
#include <sys/timeval.h>
#include <sys/time.h>
#else
#include <sys/time.h>
#endif
#define MICRO_IN_SEC 1000000.00

#include "ext/standard/php_var.h"	// for php_var_dump function
#include "ext/standard/php_string.h"  // for php_trim function
#include "main/SAPI.h"  // for sapi_header_op

/* {{{ azalea_randomstring
 */
PHP_FUNCTION(azalea_randomstring)
{
	long len;
	zend_string *mode = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|S", &len, &mode) == FAILURE) {
		return;
	}
	if (len < 1) {
		php_error_docref(NULL, E_WARNING, "String length is smaller than 1");
		RETURN_FALSE;
	}

	static char *base = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char *p = base;
	size_t l = 62;

	if (mode) {
		if (strncmp(ZSTR_VAL(mode), ZEND_STRL("10")) == 0 || strncasecmp(ZSTR_VAL(mode), ZEND_STRL("n")) == 0) {
			// [0-9]
			l = 10;
		} else if (strncmp(ZSTR_VAL(mode), ZEND_STRL("16")) == 0) {
			// [0-9a-f]
			l = 16;
		} else if (strncasecmp(ZSTR_VAL(mode), ZEND_STRL("c")) == 0) {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (strncasecmp(ZSTR_VAL(mode), ZEND_STRL("ln")) == 0) {
			// [0-9a-z]
			l = 36;
		} else if (strncasecmp(ZSTR_VAL(mode), ZEND_STRL("un")) == 0) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (strncasecmp(ZSTR_VAL(mode), ZEND_STRL("l")) == 0) {
			// [a-z]
			p += 10;
			l = 26;
		} else if (strncasecmp(ZSTR_VAL(mode), ZEND_STRL("u")) == 0) {
			// [A-Z]
			p += 36;
			l = 26;
		}
	}
	char result[len];
	long i;
	zend_long number;
	l -= 1; // for RAND_RANGE
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}
	for (i = 0; i < len; ++i) {
		number = (zend_long) php_mt_rand() >> 1;
		RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
		result[i] = *(p + number);
	}
	RETURN_STRINGL(result, len);
}
/* }}} */

/* {{{ proto azaleaUrl */
PHPAPI zend_string * azaleaUrl(zend_string *url, zend_bool includeHost)
{
	// init AZALEA_G(host)
	if (!AZALEA_G(host)) {
		zval *server, *field;
		zend_string *hostname, *tstr;

		server = &PG(http_globals)[TRACK_VARS_SERVER];
		if (server && Z_TYPE_P(server) == IS_ARRAY &&
				zend_hash_str_exists(Z_ARRVAL_P(server), ZEND_STRL("HTTPS"))) {
			hostname = zend_string_init(ZEND_STRL("https://"), 0);
		} else {
			hostname = zend_string_init(ZEND_STRL("http://"), 0);
		}
		field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_HOST"));
		if (!field) {
			// host not found, try to get from config
			field = azaleaConfigFind("hostname");
			if (!field) {
				tstr = strpprintf(0, "%slocalhost", ZSTR_VAL(hostname));
			} else {
				tstr = strpprintf(0, "%s%s", ZSTR_VAL(hostname), Z_STRVAL_P(field));
			}
		} else {
			tstr = strpprintf(0, "%s%s", ZSTR_VAL(hostname), Z_STRVAL_P(field));
		}
		zend_string_release(hostname);
		AZALEA_G(host) = tstr;
	}

	return strpprintf(0, "%s%s%s", includeHost ? ZSTR_VAL(AZALEA_G(host)) : "",
			AZALEA_G(baseUri) ? ZSTR_VAL(AZALEA_G(baseUri)) : "/", ZSTR_VAL(url));
}
/* }}} */

/* {{{ azalea_timer
 */
PHP_FUNCTION(azalea_timer)
{
	double now = azaleaGetMicrotime();
	RETVAL_DOUBLE(now - AZALEA_G(request_time));
	AZALEA_G(request_time) = now;
}
/* }}} */

/* {{{ azalea_url
 */
PHP_FUNCTION(azalea_url)
{
	zend_string *url;
	zend_bool includeHost = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &url, &includeHost) == FAILURE) {
		return;
	}

	RETURN_STR(azaleaUrl(url, includeHost));
}
/* }}} */

/* {{{ azalea_env
 */
PHP_FUNCTION(azalea_env)
{
	RETURN_STR(zend_string_copy(AZALEA_G(environ)));
}
/* }}} */

/* {{{ azalea_getmodel
 */
PHP_FUNCTION(azalea_ip)
{
	if (!AZALEA_G(ip)) {
		zval *server, *field;
		zend_string *ip = NULL;

		server = &PG(http_globals)[TRACK_VARS_SERVER];
		if (Z_TYPE_P(server) == IS_ARRAY) {
			if ((field= zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_CLIENT_IP"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			} else if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_X_FORWARDED_FOR"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			} else if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("REMOTE_ADDR"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			}
		}
		if (ip) {
			AZALEA_G(ip) = ip;
		} else {
			AZALEA_G(ip) = zend_string_init(ZEND_STRL("0.0.0.0"), 0);
		}
	}
	RETURN_STR(zend_string_copy(AZALEA_G(ip)));
}
/* }}} */

/* {{{ proto azaleaRequestFind */
PHPAPI zval * azaleaGlobalsFind(uint type, zend_string *name)
{
	zval *carrier, *field;
	carrier = &PG(http_globals)[type];
	if (!name) {
		return carrier;
	}
	field = zend_hash_find(Z_ARRVAL_P(carrier), name);
	if (!field) {
		return NULL;
	}
	return field;
}
/* }}} */

/* {{{ proto azaleaGlobalsStrFind */
PHPAPI zval * azaleaGlobalsStrFind(uint type, char *name, size_t len)
{
	zval *carrier, *field;
	carrier = &PG(http_globals)[type];
	if (!name) {
		return carrier;
	}
	field = zend_hash_str_find(Z_ARRVAL_P(carrier), name, len);
	if (!field) {
		return NULL;
	}
	return field;
}
/* }}} */

/* {{{ proto azaleaSetHeaderStr */
PHPAPI void azaleaSetHeaderStr(char *line, size_t len, zend_long httpCode)
{
	sapi_header_line ctr = {0};
	ctr.line = line;
	ctr.line_len = len;
	ctr.response_code = httpCode;
	sapi_header_op(SAPI_HEADER_REPLACE, &ctr);
}
/* }}} */

/** {{{ int azaleaRequiree(char *path, size_t len)
*/
PHPAPI int azaleaRequire(char *path, size_t len)
{
	zend_file_handle file_handle;
	zend_op_array *op_array;
	char realpath[MAXPATHLEN];

	if (!VCWD_REALPATH(path, realpath)) {
		return 0;
	}

	file_handle.filename = path;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
	file_handle.handle.fp = NULL;
	op_array = zend_compile_file(&file_handle, ZEND_REQUIRE);

	if (op_array && file_handle.handle.stream.handle) {
		zval dummy;
		ZVAL_NULL(&dummy);
		if (!file_handle.opened_path) {
			file_handle.opened_path = zend_string_init(path, len, 0);
		}
		zend_hash_add(&EG(included_files), file_handle.opened_path, &dummy);
	}
	zend_destroy_file_handle(&file_handle);

	if (op_array) {
		zval result;
		ZVAL_UNDEF(&result);
		zend_execute(op_array, &result);
		destroy_op_array(op_array);
		efree(op_array);
		if (!EG(exception)) {
			zval_ptr_dtor(&result);
		}
		return 1;
	}
	return 0;
}
/* }}} */

/* {{{ proto azaleaGetMicrotime */
PHPAPI double azaleaGetMicrotime()
{
	struct timeval tp = {0};
	if (gettimeofday(&tp, NULL)) {
		return 0;
	}
	return (double)(tp.tv_sec + tp.tv_usec / MICRO_IN_SEC);
}
/* }}} */

/** {{{ int azaleaLoadModel(zend_execute_data *execute_data, zval *return_value, zval *instance)
*/
PHPAPI void azaleaLoadModel(INTERNAL_FUNCTION_PARAMETERS, zval *from)
{
	zend_string *modelName, *lcName, *name, *modelClass, *tstr;
	zend_class_entry *ce;
	azalea_model_t *instance = NULL, rv = {{0}};

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &modelName) == FAILURE) {
		return;
	}

	lcName = zend_string_tolower(modelName);
	name = zend_string_init(ZSTR_VAL(lcName), ZSTR_LEN(lcName), 0);
	ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);  // ucfirst
	modelClass = strpprintf(0, "%sModel", ZSTR_VAL(name));
	zend_string_release(name);

	name = zend_string_tolower(modelClass);
	instance = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), name);
	if (instance) {
		ce = Z_OBJCE_P(instance);
	} else {
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
		int status = azaleaRequire(Z_STRVAL(modelPath), Z_STRLEN(modelPath));
		if (!status) {
			tstr = strpprintf(0, "Model file `%s` compile error.", Z_STRVAL(modelPath));
			throw404(tstr);
			zend_string_release(tstr);
			RETURN_FALSE;
		}
		// check model class exists
		if (!(ce = zend_hash_find_ptr(EG(class_table), name))) {
			tstr = strpprintf(0, "Model class `%s` not found.", ZSTR_VAL(modelClass));
			throw404(tstr);
			zend_string_release(tstr);
			RETURN_FALSE;
		}
		// check super class name
		if (!instanceof_function(ce, azalea_model_ce)) {
			throw404Str(ZEND_STRL("Model class must be an instance of " AZALEA_NS_NAME(Model) "."));
			RETURN_FALSE;
		}
		zval_ptr_dtor(&modelPath);

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
			if (!(field = zend_read_property(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), 1, NULL)) ||
					Z_TYPE_P(field) != IS_STRING) {
				// get serviceUrl from config
				if (!(field = azaleaConfigSubFind("service", "url")) || Z_TYPE_P(field) != IS_STRING) {
					throw404Str(ZEND_STRL("Service url not set."));
					RETURN_FALSE;
				}
				zend_string *serviceUrl, *t;
				t = zend_string_dup(Z_STR_P(field), 0);
				tstr = php_trim(t, ZEND_STRL("/"), 2);
				zend_string_release(t);
				serviceUrl = strpprintf(0, "%s/%s", ZSTR_VAL(tstr), ZSTR_VAL(lcName));
				zend_update_property_str(azalea_service_ce, instance, ZEND_STRL("serviceUrl"), serviceUrl);
				zend_string_release(tstr);
				zend_string_release(serviceUrl);
			}
		}

		// call __init method
		if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__init"))) {
			zend_call_method_with_0_params(instance, ce, NULL, "__init", NULL);
		}

		// cache instance
		add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(name), ZSTR_LEN(name), instance);
	}
	zend_string_release(lcName);
	zend_string_release(modelClass);
	zend_string_release(name);

	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */
