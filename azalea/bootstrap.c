/*
 * azalea/bootstrap.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/loader.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"
#include "azalea/controller.h"
#include "azalea/request.h"
#include "azalea/response.h"
#include "azalea/exception.h"

#include "Zend/zend_exceptions.h"  // for zend_throw_exception
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*
#include "Zend/zend_smart_str.h"  // for smart_str
#include "ext/standard/url.h"  // for php_url_*
#include "ext/standard/php_var.h"  // for php_var_dump
#include "ext/standard/php_string.h"  // for php_trim
#include "ext/standard/php_filestat.h"  // for php_stat
#include "ext/session/php_session.h"  // for php_session_start
#include "ext/json/php_json.h"  // for php_json_encode
#include "main/SAPI.h"  // for sapi_header_op

zend_class_entry *azaleaBootstrapCe;

/* {{{ class Azalea\Bootstrap methods
 */
static zend_function_entry azalea_bootstrap_methods[] = {
	PHP_ME(azalea_bootstrap, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_bootstrap, getRoute, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_bootstrap, init, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_bootstrap, run, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_STARTUP_FUNCTION(bootstrap)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Bootstrap), azalea_bootstrap_methods);
	azaleaBootstrapCe = zend_register_internal_class(&ce TSRMLS_CC);
	azaleaBootstrapCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_bootstrap, __construct) {}
/* }}} */

/* {{{ proto Bootstrap init(mixed $config, string $environ = AZALEA_G(environ)) */
PHP_METHOD(azalea_bootstrap, init)
{
	zval *config = NULL, *field, *conf, *pData, iniValue;
	zend_string *environ = NULL, *iniName, *tstr, *docRoot = NULL, *baseUri = NULL, *uri = NULL;
	int module_number = 0;
	size_t len;
	char *cwd = NULL;
	double now;
	azalea_bootstrap_t *instance;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|zS", &config, &environ) == FAILURE) {
		return;
	}

	if (Z_TYPE(AZALEA_G(bootstrap)) != IS_UNDEF) {
		php_error_docref(NULL, E_ERROR, "Only one Azalea bootstrap can be initialized at a request");
		RETURN_FALSE;
	}

	// ---------- START ----------
	// print copyright
	sapi_header_line ctr = {0};
	ctr.line = PHP_AZALEA_COPYRIGHT_OUTPUT;
	ctr.line_len = sizeof(PHP_AZALEA_COPYRIGHT_OUTPUT) - 1;
	sapi_header_op(SAPI_HEADER_REPLACE, &ctr);

	// set timer
	now = azaleaGetMicrotime();
	AZALEA_G(timer) = now;

	// set environ
	if (environ && ZSTR_LEN(environ)) {
		zend_string_release(AZALEA_G(environ));
		AZALEA_G(environ) = zend_string_copy(environ);
	}

	// create output buffer
	if (php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS) == FAILURE) {
		php_error_docref(NULL, E_ERROR, "Failed to create output buffer");
		RETURN_FALSE;
	}

	// load config
	azaleaLoadConfig(config);

	// set timezone
	conf = azaleaConfigFind("timezone");
	if (Z_STRLEN_P(conf)) {
		iniName = zend_string_init(ZEND_STRL("date.timezone"), 0);
		zend_alter_ini_entry(iniName, Z_STR_P(conf), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// set error reporting while debug is true
	conf = azaleaConfigFind("debug");
	if (Z_TYPE_P(conf) == IS_TRUE) {
		EG(error_reporting) = E_ALL;
		iniName = zend_string_init(ZEND_STRL("display_errors"), 0);
		zend_alter_ini_entry_chars(iniName, ZEND_STRL("on"), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
	}

	// set docRoot / base_uri / uri
	if ((cwd = VCWD_GETCWD(NULL, PATH_MAX))) {
		len = strlen(cwd);
		// 确保 / 结尾
		docRoot = zend_string_init(cwd, len + 1, 0);
		ZSTR_VAL(docRoot)[len] = DEFAULT_SLASH;
		ZSTR_VAL(docRoot)[len + 1] = '\0';
	} else {
		docRoot = zend_string_init(ZEND_STRL("/"), 0);
	}
	AZALEA_G(docRoot) = docRoot;

	// load SERVER global variable
	if (PG(auto_globals_jit)) {
		tstr = zend_string_init(ZEND_STRL("_SERVER"), 0);
		zend_is_auto_global(tstr);
		zend_string_release(tstr);
	}

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("SCRIPT_NAME"))) &&
			Z_TYPE_P(field) == IS_STRING) {
		baseUri = zend_string_dup(Z_STR_P(field), 0);
		// dirname
		len = zend_dirname(ZSTR_VAL(baseUri), ZSTR_LEN(baseUri));
		tstr = baseUri;
		if (len > 1) {
			// 确保 / 结尾
			baseUri = zend_string_alloc(len + 1, 0);
			memcpy(ZSTR_VAL(baseUri), ZSTR_VAL(tstr), len);
			ZSTR_VAL(baseUri)[len] = DEFAULT_SLASH;
			ZSTR_VAL(baseUri)[len + 1] = '\0';
		} else {
			// empty
			baseUri = zend_string_init(ZEND_STRL("/"), 0);
		}
		zend_string_release(tstr);
	} else {
		// empty
		baseUri = zend_string_init(ZEND_STRL("/"), 0);
	}
	do {
		if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("PATH_INFO"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			uri = zend_string_copy(Z_STR_P(field));
			break;
		}
		if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("REQUEST_URI"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			if (strncasecmp(Z_STRVAL_P(field), "http://", sizeof("http://") - 1) &&
					strncasecmp(Z_STRVAL_P(field), "https://", sizeof("https://") - 1)) {
				// not http url
				char *pos = strstr(Z_STRVAL_P(field), "?");
				if (pos) {
					// found query
					uri = zend_string_init(Z_STRVAL_P(field), pos - Z_STRVAL_P(field), 0);
				} else {
					uri = zend_string_copy(Z_STR_P(field));
				}
			} else {
				php_url *urlInfo = php_url_parse(Z_STRVAL_P(field));
				if (urlInfo && urlInfo->path) {
					uri = zend_string_init(urlInfo->path, strlen(urlInfo->path), 0);
				}
				php_url_free(urlInfo);
			}
			// remove baseUri
			if (0 == strncasecmp(ZSTR_VAL(uri), ZSTR_VAL(baseUri), ZSTR_LEN(baseUri))) {
				tstr = uri;
				uri = zend_string_init(ZSTR_VAL(uri) + ZSTR_LEN(baseUri), ZSTR_LEN(uri) - ZSTR_LEN(baseUri), 0);
				zend_string_release(tstr);
			}
			break;
		}
		if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("ORIG_PATH_INFO"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			uri = zend_string_copy(Z_STR_P(field));
			// remove baseUri
			if (0 == strncasecmp(ZSTR_VAL(uri), ZSTR_VAL(baseUri), ZSTR_LEN(baseUri))) {
				tstr = uri;
				uri = zend_string_init(ZSTR_VAL(uri) + ZSTR_LEN(baseUri), ZSTR_LEN(uri) - ZSTR_LEN(baseUri), 0);
				zend_string_release(tstr);
			}
			break;
		}
		// for CLI mode
		if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("argv"))) &&
				Z_TYPE_P(field) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(field)) >= 2) {
			field = zend_hash_index_find(Z_ARRVAL_P(field), 1);
			uri = zend_string_copy(Z_STR_P(field));
			break;
		}
	} while (0);

	if (!baseUri) {
		baseUri = zend_string_init(ZEND_STRL("/"), 0);
	}
	AZALEA_G(baseUri) = baseUri;

	if (uri) {
		zend_string *t = uri;
		uri = php_trim(uri, ZEND_STRL("/"), 3);
		zend_string_release(t);
	} else {
		uri = ZSTR_EMPTY_ALLOC();
	}
	AZALEA_G(uri) = uri;

	// define AZALEA magic const
	module_number = AZALEA_G(moduleNumber);
	REGISTER_NS_STRINGL_CONSTANT(AZALEA_NS, "DOCROOT", ZSTR_VAL(docRoot), ZSTR_LEN(docRoot), CONST_CS);
	REGISTER_NS_STRINGL_CONSTANT(AZALEA_NS, "BASEPATH", ZSTR_VAL(baseUri), ZSTR_LEN(baseUri), CONST_CS);

	// set session if in session.environ list
	field = azaleaConfigSubFind("session", "env");
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(field), pData) {
		if (Z_TYPE_P(pData) == IS_STRING && 0 == strcasecmp(Z_STRVAL_P(pData), ZSTR_VAL(AZALEA_G(environ)))) {
			AZALEA_G(startSession) = 1;
			break;
		}
	} ZEND_HASH_FOREACH_END();
	if (AZALEA_G(startSession)) {
		// session.name
		iniName = zend_string_init(ZEND_STRL("session.name"), 0);
		zend_alter_ini_entry(iniName, Z_STR_P(azaleaConfigSubFind("session", "name")),
				PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
		// session.cookie_lifetime
		conf = azaleaConfigSubFind("session", "lifetime");
		ZVAL_COPY(&iniValue, conf);
		convert_to_string(&iniValue);
		iniName = zend_string_init(ZEND_STRL("session.cookie_lifetime"), 0);
		zend_alter_ini_entry(iniName, Z_STR(iniValue), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
		zval_ptr_dtor(&iniValue);
		// session.cookie_path
		conf = azaleaConfigSubFind("session", "path");
		ZVAL_COPY(&iniValue, conf);
		convert_to_string(&iniValue);
		if (!Z_STRLEN(iniValue)) {
			// use baseUri for default path
			zval_ptr_dtor(&iniValue);
			ZVAL_STR(&iniValue, baseUri);
		}
		iniName = zend_string_init(ZEND_STRL("session.cookie_path"), 0);
		zend_alter_ini_entry(iniName, Z_STR(iniValue), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		zend_string_release(iniName);
		zval_ptr_dtor(&iniValue);
		// session.cooke_domain
		conf = azaleaConfigSubFind("session", "domain");
		ZVAL_COPY(&iniValue, conf);
		convert_to_string(&iniValue);
		if (Z_STRLEN(iniValue)) {
			iniName = zend_string_init(ZEND_STRL("session.cookie_domain"), 0);
			zend_alter_ini_entry(iniName, Z_STR(iniValue), PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
			zend_string_release(iniName);
		}
		zval_ptr_dtor(&iniValue);
	}
	// ---------- END ----------

	// init bootstrap instance
	instance = &AZALEA_G(bootstrap);
	object_init_ex(instance, azaleaBootstrapCe);
	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto processContent */
static void processContent(zval *result)
{
	if (Z_TYPE_P(result) != IS_NULL) {
		if (Z_TYPE_P(result) == IS_ARRAY || Z_TYPE_P(result) == IS_OBJECT) {
			smart_str buf = {0};
			php_json_encode(&buf, result, 0);
			smart_str_0(&buf);
			PHPWRITE(ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
			smart_str_free(&buf);
		} else {
			convert_to_string(result);
			PHPWRITE(Z_STRVAL_P(result), Z_STRLEN_P(result));
		}
		zval_ptr_dtor(result);
	}
	ZVAL_TRUE(result);
}
/* }}} */

/* {{{ proto bool run(void) */
PHP_METHOD(azalea_bootstrap, run)
{
	zend_string *uri, *controllersPath, *modelsPath, *viewsPath, *appRoot = NULL, *tstr;
	zval *conf, *field, *paths;
	int module_number;

	// request uri
	uri = AZALEA_G(uri);
	// static router
	field = azaleaConfigFind("router");
	if (ZSTR_LEN(uri) > 0 && field && Z_TYPE_P(field) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(field)) > 0) {
		// check static router
		zend_string *key, *tstr;
		zval *pData;
		char *p;
		size_t len = ZSTR_LEN(uri);
		zend_bool matched = 0;
		do {
			tstr = zend_string_init(ZSTR_VAL(uri), len, 0);
			ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(field), key, pData) {
				if (0 == strcasecmp(ZSTR_VAL(key), ZSTR_VAL(tstr))) {
					// first match
					matched = 1;
					break;
				}
			} ZEND_HASH_FOREACH_END();
			zend_string_release(tstr);
			if (matched) {
				// update uri
				if (len == ZSTR_LEN(uri)) {
					uri = zend_string_copy(Z_STR_P(pData));
				} else {
					uri = strpprintf(0, "%s%s", Z_STRVAL_P(pData), ZSTR_VAL(uri) + len);
				}
				zend_string_release(AZALEA_G(uri));
				AZALEA_G(uri) = uri;
				break;
			}
			// check closest path length
			do {
				--len;
				p = ZSTR_VAL(uri) + len;
				if (*p == '/') {
					break;
				}
			} while (p > ZSTR_VAL(uri));
		} while (len);
	}

	// get paths
	paths = &AZALEA_G(paths);
	if (ZSTR_LEN(uri)) {
		zend_string *delim = zend_string_init(ZEND_STRL("/"), 0);
		php_explode(delim, uri, paths, ZEND_LONG_MAX);
		zend_string_release(delim);
	}
	conf = azaleaConfigSubFind("path", "controllers");
	controllersPath = zend_string_dup(Z_STR_P(conf), 0);
	conf = azaleaConfigSubFind("path", "models");
	modelsPath = zend_string_dup(Z_STR_P(conf), 0);
	conf = azaleaConfigSubFind("path", "views");
	viewsPath = zend_string_dup(Z_STR_P(conf), 0);
	conf = azaleaConfigSubFind("path", "basepath");
	if (conf && Z_TYPE_P(conf) == IS_STRING) {
		char realpath[MAXPATHLEN];
		if (VCWD_REALPATH(Z_STRVAL_P(conf), realpath)) {
			appRoot = zend_string_init(realpath, strlen(realpath), 0);
		}
	}
	if (!appRoot) {
		appRoot = zend_string_copy(AZALEA_G(docRoot));
	}
	if (ZSTR_VAL(appRoot)[ZSTR_LEN(appRoot) - 1] != DEFAULT_SLASH) {
		tstr = appRoot;
		appRoot = strpprintf(0, "%s%c", ZSTR_VAL(appRoot), DEFAULT_SLASH);
		zend_string_release(tstr);
	}
	if (ZSTR_VAL(controllersPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = controllersPath;
		controllersPath = strpprintf(0, "%s%s", ZSTR_VAL(appRoot), ZSTR_VAL(controllersPath));
		zend_string_release(tstr);
	}
	if (ZSTR_VAL(modelsPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = modelsPath;
		modelsPath = strpprintf(0, "%s%s", ZSTR_VAL(appRoot), ZSTR_VAL(modelsPath));
		zend_string_release(tstr);
	}
	if (ZSTR_VAL(viewsPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = viewsPath;
		viewsPath = strpprintf(0, "%s%s", ZSTR_VAL(appRoot), ZSTR_VAL(viewsPath));
		zend_string_release(tstr);
	}
	conf = azaleaConfigFind("theme");
	if (conf && Z_TYPE_P(conf) == IS_STRING && Z_STRLEN_P(conf)) {
		// add views subpath
		tstr = viewsPath;
		viewsPath = strpprintf(0, "%s%c%s", ZSTR_VAL(viewsPath), DEFAULT_SLASH, Z_STRVAL_P(conf));
		zend_string_release(tstr);
	}
	AZALEA_G(controllersPath) = controllersPath;
	AZALEA_G(modelsPath) = modelsPath;
	AZALEA_G(viewsPath) = viewsPath;

	// define AZALEA magic const
	module_number = AZALEA_G(moduleNumber);
	REGISTER_NS_STRINGL_CONSTANT(AZALEA_NS, "APPROOT", ZSTR_VAL(appRoot), ZSTR_LEN(appRoot), CONST_CS);
	zend_string_release(appRoot);

	// get folder / controller / action / arguments
	zend_ulong pathsOffset = 0;
	field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
	if (field) {
		zend_string *lcName, *folderPath;
		zval exists;
		lcName = zend_string_tolower(Z_STR_P(field));
		folderPath = strpprintf(0, "%s%c%s", ZSTR_VAL(controllersPath), DEFAULT_SLASH, ZSTR_VAL(lcName));
		php_stat(ZSTR_VAL(folderPath), (php_stat_len) ZSTR_LEN(folderPath), FS_IS_DIR, &exists);
		if (Z_TYPE(exists) == IS_TRUE) {
			++pathsOffset;
			AZALEA_G(folderName) = zend_string_copy(lcName);
		}
		zend_string_release(folderPath);
		zend_string_release(lcName);

		// controller
		field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
		if (field) {
			++pathsOffset;
			AZALEA_G(controllerName) = zend_string_tolower(Z_STR_P(field));

			// action
			field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
			if (field) {
				++pathsOffset;
				AZALEA_G(actionName) = zend_string_tolower(Z_STR_P(field));

				// arguments
				uint32_t num = zend_hash_num_elements(Z_ARRVAL_P(paths));
				if (num - pathsOffset > 0) {  // has arguments
					zval *args = &AZALEA_G(pathArgs);
					while ((field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset++))) {
						zend_hash_next_index_insert_new(Z_ARRVAL_P(args), field);
						zval_add_ref(field);  // release for AZALEA_G(pathArgs)
					}
				}
			}
		}
	}

	if (!AZALEA_G(controllerName)) {
		// default controller
		conf = azaleaConfigSubFind("dispatch", "default_controller");
		AZALEA_G(controllerName) = Z_STR_P(conf);
		zval_add_ref(conf);  // release for AZALEA_G(controllerName)
	}
	if (!AZALEA_G(actionName)) {
		// default controller
		conf = azaleaConfigSubFind("dispatch", "default_action");
		AZALEA_G(actionName) = Z_STR_P(conf);
		zval_add_ref(conf);  // release for AZALEA_G(actionName)
	}

	// session start
	if (AZALEA_G(startSession)) {
		php_session_start();
	}

	// start dispatch
	zval ret;
	azaleaDispatch(AZALEA_G(folderName), AZALEA_G(controllerName), AZALEA_G(actionName),
			&AZALEA_G(pathArgs), &ret);

	// try ... catch \Exception
	if (EG(exception) && instanceof_function(EG(exception)->ce, zend_ce_exception)) {
		php_output_clean();

		zval exception, errorArgs, *errorController, *errorAction;
		ZVAL_OBJ(&exception, EG(exception));
		EG(exception) = NULL;
		array_init(&errorArgs);
		add_next_index_zval(&errorArgs, &exception);

		errorController = azaleaConfigSubFind("dispatch", "error_controller");
		errorAction = azaleaConfigSubFind("dispatch", "error_action");
		azaleaDispatch(NULL, Z_STR_P(errorController), Z_STR_P(errorAction), &errorArgs, &ret);
		processContent(&ret);
		zval_ptr_dtor(&errorArgs);

		if (EG(exception)) {
			php_output_clean();

			// catch new exception from error controller action or view, ignore it
			zend_string *message;
			if (Z_TYPE_P(azaleaConfigFind("debug")) == IS_TRUE) {
				zval newException;
				ZVAL_OBJ(&newException, EG(exception));
				message = zval_get_string(zend_read_property(zend_ce_exception, &newException, ZEND_STRL("message"), 0, NULL));
			} else {
				message = zval_get_string(zend_read_property(zend_ce_exception, &exception, ZEND_STRL("message"), 0, NULL));
			}
			PHPWRITE(ZSTR_VAL(message), ZSTR_LEN(message));
		}
		zend_clear_exception();
		zend_bailout();
	} else {
		// process and output result content
		processContent(&ret);
	}

	// try to close output buffer
	if (!OG(active) || php_output_end() == FAILURE) {
		php_error_docref(NULL, E_ERROR, "Failed to close output buffer");
		RETURN_FALSE;
	}

	RETURN_ZVAL(&ret, 1, 0);
}
/* }}} */

/* {{{ proto string getRoute(void) */
PHP_METHOD(azalea_bootstrap, getRoute)
{
	array_init(return_value);
	if (AZALEA_G(folderName)) {
		add_assoc_str(return_value, "folder", zend_string_copy(AZALEA_G(folderName)));
	} else {
		add_assoc_null(return_value, "folder");
	}
	if (AZALEA_G(controllerName)) {
		add_assoc_str(return_value, "controller", zend_string_copy(AZALEA_G(controllerName)));
	} else {
		add_assoc_null(return_value, "controller");
	}
	if (AZALEA_G(actionName)) {
		add_assoc_str(return_value, "action", zend_string_copy(AZALEA_G(actionName)));
	} else {
		add_assoc_null(return_value, "action");
	}
	add_assoc_zval(return_value, "arguments", &AZALEA_G(pathArgs));
	zval_add_ref(&AZALEA_G(pathArgs));
}
/* }}} */

/* {{{ proto azaleaDispatch */
zend_bool azaleaDispatchEx(zend_string *folderName, zend_string *controllerName, zend_string *actionName, zend_bool isCallback, zval *pathArgs, zval *ret)
{
	zend_string *name, *lcName, *controllerClass, *controllerPath, *actionMethod = NULL, *tstr;
	zend_class_entry *ce;
	azalea_controller_t *instance = NULL, rv = {{0}};
	azalea_request_t *pReq;
	azalea_response_t *pRes;

	// controller name
	name = zend_string_init(ZSTR_VAL(controllerName), ZSTR_LEN(controllerName), 0);
	ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);  // ucfirst
	controllerClass = strpprintf(0, "%sController", ZSTR_VAL(name));
	zend_string_release(name);
	if (folderName) {
		name = zend_string_init(ZSTR_VAL(folderName), ZSTR_LEN(folderName), 0);
		ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);	// ucfirst
		tstr = controllerClass;
		controllerClass = strpprintf(0, "%s%s", ZSTR_VAL(name), ZSTR_VAL(controllerClass));
		zend_string_release(tstr);
		zend_string_release(name);
	}

	lcName = zend_string_tolower(controllerClass);
	if ((instance = zend_hash_find(Z_ARRVAL(AZALEA_G(instances)), lcName))) {
		ce = Z_OBJCE_P(instance);
	} else {
		// check controller class
		if (!(ce = zend_hash_find_ptr(EG(class_table), lcName))) {
			// class not exists, load controller file
			zval exists;
			if (folderName) {
				controllerPath = strpprintf(0, "%s%c%s%c%s.php", ZSTR_VAL(AZALEA_G(controllersPath)), DEFAULT_SLASH,
						ZSTR_VAL(folderName), DEFAULT_SLASH, ZSTR_VAL(controllerName));
			} else {
				controllerPath = strpprintf(0, "%s%c%s.php", ZSTR_VAL(AZALEA_G(controllersPath)), DEFAULT_SLASH,
						ZSTR_VAL(controllerName));
			}
			zend_bool error = 1;
			do {
				// check file exists
				php_stat(ZSTR_VAL(controllerPath), (php_stat_len) ZSTR_LEN(controllerPath), FS_IS_R, &exists);
				if (Z_TYPE(exists) == IS_FALSE) {
					tstr = strpprintf(0, "Controller file `%s` not found.", ZSTR_VAL(controllerPath));
					throw404(tstr);
					zend_string_release(tstr);
					break;
				}
				// require controller file
				int status = azaleaRequire(ZSTR_VAL(controllerPath), 1);
				if (!status) {
					tstr = strpprintf(0, "Controller file `%s` compile error.", ZSTR_VAL(controllerPath));
					throw404(tstr);
					zend_string_release(tstr);
					break;
				}
				// check controller class again
				if (!(ce = zend_hash_find_ptr(EG(class_table), lcName))) {
					tstr = strpprintf(0, "Controller class `%s` not found.", ZSTR_VAL(controllerClass));
					throw404(tstr);
					zend_string_release(tstr);
					break;
				}
				// check super class name
				if (!instanceof_function(ce, azaleaControllerCe)) {
					throw404Str(ZEND_STRL("Controller class must be an instance of " AZALEA_NS_NAME(Controller) "."));
					break;
				}
				error = 0;
			} while (0);
			zend_string_release(controllerPath);
			if (error) {
				zend_string_release(controllerClass);
				zend_string_release(lcName);
				ZVAL_FALSE(ret);
				return 0;
			}
		}

		// init controller instance
		instance = &rv;
		object_init_ex(instance, ce);
		if (!instance) {
			throw404Str(ZEND_STRL("Controller initialization is failed."));
			zend_string_release(controllerClass);
			zend_string_release(lcName);
			ZVAL_FALSE(ret);
			return 0;
		}

		// controller construct
		if (folderName) {
			zend_update_property_str(azaleaControllerCe, instance, ZEND_STRL("_folder"), folderName);
		} else {
			zend_update_property_null(azaleaControllerCe, instance, ZEND_STRL("_folder"));
		}
		zend_update_property_str(azaleaControllerCe, instance, ZEND_STRL("_controller"), controllerName);
		// request
		if (!(pReq = zend_hash_str_find(Z_ARRVAL(AZALEA_G(instances)), ZEND_STRL("_request")))) {
			azalea_request_t req = {{0}};
			pReq = &req;
			object_init_ex(pReq, azaleaRequestCe);
			add_assoc_zval_ex(&AZALEA_G(instances), ZEND_STRL("_request"), pReq);
		}
		zend_update_property(azaleaControllerCe, instance, ZEND_STRL("req"), pReq);
		// response
		{
			azalea_response_t res = {{0}};
			tstr = strpprintf(0, "_response_%s", ZSTR_VAL(lcName));
			pRes = &res;
			object_init_ex(pRes, azaleaResponseCe);
			zend_update_property(azaleaResponseCe, pRes, ZEND_STRL("_instance"), instance);
			add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(tstr), ZSTR_LEN(tstr), pRes);
			zend_string_release(tstr);
			zend_update_property(azaleaControllerCe, instance, ZEND_STRL("res"), pRes);
		}

		// call __init method
		if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__init"))) {
			zend_call_method_with_0_params(instance, ce, NULL, "__init", NULL);
		}

		// cache instance
		add_assoc_zval_ex(&AZALEA_G(instances), ZSTR_VAL(lcName), ZSTR_LEN(lcName), instance);
	}
	zend_string_release(controllerClass);
	zend_string_release(lcName);

	// dynamic router, call __router method
	name = zend_string_copy(actionName);
	if (!isCallback) {
		if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__router"))) {
			zval newRouter, arg, newArguments, *field;
			zend_string *newFolderName = NULL, *newControllerName = NULL;
			array_init(&arg);
			add_next_index_str(&arg, zend_string_copy(name));
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(pathArgs), field) {
				add_next_index_zval(&arg, field);
				zval_add_ref(field);
			} ZEND_HASH_FOREACH_END();
			ZVAL_NULL(&newRouter);
			zend_call_method_with_1_params(instance, ce, NULL, "__router", &newRouter, &arg);
			zval_ptr_dtor(&arg);
			if (Z_TYPE(newRouter) == IS_ARRAY) {
				if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("callback"))) && Z_TYPE_P(field) == IS_STRING) {
					actionMethod = zend_string_copy(Z_STR_P(field));
				} else if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("action"))) && Z_TYPE_P(field) == IS_STRING) {
					zend_string_release(name);
					name = zend_string_init(Z_STRVAL_P(field), Z_STRLEN_P(field), 0);
				}
				if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("arguments"))) && Z_TYPE_P(field) == IS_ARRAY) {
					zval_ptr_dtor(pathArgs);
					array_init(&newArguments);
					zend_hash_copy(Z_ARRVAL(newArguments), Z_ARRVAL_P(field), (copy_ctor_func_t) zval_add_ref);
					pathArgs = &newArguments;
				}
				// check folder + controller is current?
				if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("folder"))) && Z_TYPE_P(field) == IS_STRING) {
					newFolderName = Z_STR_P(field);
				} else if (folderName) {
					newFolderName = folderName;
				}
				if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("controller"))) && Z_TYPE_P(field) == IS_STRING) {
					newControllerName = Z_STR_P(field);
				} else {
					newControllerName = controllerName;
				}
				if (!!newFolderName != !!folderName || (newFolderName && folderName && strcasecmp(ZSTR_VAL(newFolderName), ZSTR_VAL(folderName))) ||
						strcasecmp(ZSTR_VAL(newControllerName), ZSTR_VAL(controllerName))) {
					// folder + controller is not current
					zend_bool r = azaleaDispatchEx(newFolderName, newControllerName, actionMethod ? actionMethod : name, actionMethod ? 1 : 0, pathArgs, ret);
					zval_ptr_dtor(&newRouter);
					if (actionMethod) {
						zend_string_release(actionMethod);
					}
					zend_string_release(name);
					return r;
				}
			}
			zval_ptr_dtor(&newRouter);
		}
		// action method name
		if (!actionMethod) {
			if (strcmp(ZSTR_VAL(AZALEA_G(environ)), "WEB")) {
				// not WEB
				actionMethod = strpprintf(0, "%s%s", ZSTR_VAL(name), ZSTR_VAL(AZALEA_G(environ)));
			} else {
				// WEB
				actionMethod = strpprintf(0, "%sAction", ZSTR_VAL(name));
			}
		}
		zend_string_release(name);
	} else {
		actionMethod = name;
	}

	// check action method
	lcName = zend_string_tolower(actionMethod);
	if (!(zend_hash_exists(&(ce->function_table), lcName))) {
		tstr = strpprintf(0, "Action method `%s` not found.", ZSTR_VAL(actionMethod));
		throw404(tstr);
		zend_string_release(tstr);
		zend_string_release(actionMethod);
		zend_string_release(lcName);
		ZVAL_FALSE(ret);
		return 0;
	}
	zend_string_release(actionMethod);

	// execute action
	zval functionName, *callArgs = NULL, *arg;
	uint32_t callArgsCount, current;
	callArgsCount = zend_hash_num_elements(Z_ARRVAL_P(pathArgs));
	if (callArgsCount) {
		callArgs = safe_emalloc(sizeof(zval), callArgsCount, 0);
		for (current = 0; current < callArgsCount; ++current) {
			arg = zend_hash_index_find(Z_ARRVAL_P(pathArgs), current);
			ZVAL_COPY_VALUE(&(callArgs[current]), arg);
		}
	}
	ZVAL_STR(&functionName, lcName);
	ZVAL_NULL(ret);
	call_user_function(&(ce->function_table), instance, &functionName, ret, callArgsCount, callArgs);
	if (callArgs) {
		efree(callArgs);
	}
	zval_ptr_dtor(&functionName);

	return 1;
}
/* }}} */
