/*
 * azalea/bootstrap.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"
#include "azalea/controller.h"
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

zend_class_entry *azalea_bootstrap_ce;

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
	azalea_bootstrap_ce = zend_register_internal_class(&ce TSRMLS_CC);
	azalea_bootstrap_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_bootstrap, __construct) {}
/* }}} */

/* {{{ proto Bootstrap init(mixed $config, string $environ = AZALEA_G(environ)) */
PHP_METHOD(azalea_bootstrap, init)
{
	zval *config = NULL;
	zend_string *environ = NULL;

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
	double now;
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

	zval *server, *field, *conf, iniValue;
	zend_string *iniName;

	// load SERVER global variable
	if (PG(auto_globals_jit)) {
		zend_string *tstr = zend_string_init(ZEND_STRL("_SERVER"), 0);
		zend_is_auto_global(tstr);
		zend_string_release(tstr);
	}
	server = &PG(http_globals)[TRACK_VARS_SERVER];

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

	// set directory / base_uri / uri
	zend_string *directory = NULL, *baseUri = NULL, *uri = NULL;
	if (server && Z_TYPE_P(server) == IS_ARRAY) {
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("SCRIPT_FILENAME"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			directory = zend_string_dup(Z_STR_P(field), 0);
			// dirname
			ZSTR_LEN(directory) = zend_dirname(ZSTR_VAL(directory), ZSTR_LEN(directory));
		}
		if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("SCRIPT_NAME"))) &&
				Z_TYPE_P(field) == IS_STRING) {
			baseUri = zend_string_dup(Z_STR_P(field), 0);
			// dirname
			size_t len;
			len = ZSTR_LEN(baseUri) = zend_dirname(ZSTR_VAL(baseUri), ZSTR_LEN(baseUri));
			if (len > 1) {
				// add '/'
				zend_string *t = zend_string_alloc(len + 1, 0);
				memcpy(ZSTR_VAL(t), ZSTR_VAL(baseUri), len);
				ZSTR_VAL(t)[len] = DEFAULT_SLASH;
				ZSTR_VAL(t)[len + 1] = '\0';
				zend_string_release(baseUri);
				baseUri = t;
			}
		}
		do {
			if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("PATH_INFO"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				uri = zend_string_copy(Z_STR_P(field));
				break;
			}
			if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("REQUEST_URI"))) &&
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
				break;
			}
			if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("ORIG_PATH_INFO"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				uri = zend_string_copy(Z_STR_P(field));
				break;
			}
			// for CLI mode
			if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("argv"))) &&
					Z_TYPE_P(field) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(field)) >= 2) {
				field = zend_hash_index_find(Z_ARRVAL_P(field), 1);
				uri = zend_string_copy(Z_STR_P(field));
				break;
			}
		} while (0);
	}
	if (!directory) {
		directory = zend_string_init(ZEND_STRL("/"), 0);
	}
	AZALEA_G(directory) = directory;
	if (!baseUri) {
		baseUri = zend_string_init(ZEND_STRL("/"), 0);
	}
	AZALEA_G(baseUri) = baseUri;
	if (uri) {
		zend_string *t = uri;
		uri = php_trim(uri, ZEND_STRL("/"), 3);
		zend_string_release(t);
	} else {
		uri = zend_string_init(ZEND_STRL(""), 0);
	}
	AZALEA_G(uri) = uri;

	// set session
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

	// ---------- END ----------

	// init bootstrap instance
	azalea_bootstrap_t *instance = &AZALEA_G(bootstrap);
	object_init_ex(instance, azalea_bootstrap_ce);
	RETURN_ZVAL(instance, 1, 0);
}
/* }}} */

/* {{{ proto bool run(void) */
PHP_METHOD(azalea_bootstrap, run)
{
	zend_string *uri, *controllersPath, *modelsPath, *viewsPath, *basePath, *tstr;
	zval *conf, *field, *paths;
	zend_ulong pathsOffset = 0;

	// request uri & get paths
	uri = AZALEA_G(uri);
	paths = &AZALEA_G(paths);
	if (ZSTR_LEN(uri)) {
		zend_string *delim = zend_string_init(ZEND_STRL("/"), 0);
		php_explode(delim, uri, paths, ZEND_LONG_MAX);
		zend_string_release(delim);
	}
	conf = azaleaConfigSubFind("path", "controllers");
	controllersPath = zend_string_init(Z_STRVAL_P(conf), Z_STRLEN_P(conf), 0);
	conf = azaleaConfigSubFind("path", "models");
	modelsPath = zend_string_init(Z_STRVAL_P(conf), Z_STRLEN_P(conf), 0);
	conf = azaleaConfigSubFind("path", "views");
	viewsPath = zend_string_init(Z_STRVAL_P(conf), Z_STRLEN_P(conf), 0);
	conf = azaleaConfigSubFind("path", "basepath");
	basePath = zend_string_copy(conf && Z_TYPE_P(conf) == IS_STRING ? Z_STR_P(conf) : AZALEA_G(directory));
	if (ZSTR_VAL(basePath)[ZSTR_LEN(basePath) - 1] != DEFAULT_SLASH) {
		tstr = strpprintf(0, "%s%c", ZSTR_VAL(basePath), DEFAULT_SLASH);
		zend_string_release(basePath);
		basePath = tstr;
	}
	if (ZSTR_VAL(controllersPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = strpprintf(0, "%s%s", ZSTR_VAL(basePath), ZSTR_VAL(controllersPath));
		zend_string_release(controllersPath);
		controllersPath = tstr;
	}
	if (ZSTR_VAL(modelsPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = strpprintf(0, "%s%s", ZSTR_VAL(basePath), ZSTR_VAL(modelsPath));
		zend_string_release(modelsPath);
		modelsPath = tstr;
	}
	if (ZSTR_VAL(viewsPath)[0] != DEFAULT_SLASH) {
		// relative path
		tstr = strpprintf(0, "%s%s", ZSTR_VAL(basePath), ZSTR_VAL(viewsPath));
		zend_string_release(viewsPath);
		viewsPath = tstr;
	}
	conf = azaleaConfigFind("theme");
	if (conf && Z_TYPE_P(conf) == IS_STRING && Z_STRLEN_P(conf)) {
		// add views subpath
		tstr = strpprintf(0, "%s%c%s", ZSTR_VAL(viewsPath), DEFAULT_SLASH, Z_STRVAL_P(conf));
		zend_string_release(viewsPath);
		viewsPath = tstr;
	}
	zend_string_release(basePath);
	AZALEA_G(controllersPath) = zend_string_dup(controllersPath, 0);
	AZALEA_G(modelsPath) = zend_string_dup(modelsPath, 0);
	AZALEA_G(viewsPath) = zend_string_dup(viewsPath, 0);

	// get folder / controller / action / arguments
	field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
	if (field) {
		zend_string *lc, *folderPath;
		zval exists;
		lc = zend_string_tolower(Z_STR_P(field));
		folderPath = strpprintf(0, "%s%c%s", ZSTR_VAL(controllersPath), DEFAULT_SLASH, ZSTR_VAL(lc));
		php_stat(ZSTR_VAL(folderPath), (php_stat_len) ZSTR_LEN(folderPath), FS_IS_DIR, &exists);
		zend_string_release(folderPath);
		if (Z_TYPE(exists) == IS_TRUE) {
			++pathsOffset;
			AZALEA_G(folderName) = zend_string_dup(lc, 0);
		}
		zend_string_release(lc);

		// controller
		field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
		if (field) {
			++pathsOffset;
			if (AZALEA_G(controllerName)) {
				zend_string_release(AZALEA_G(controllerName));
			}
			AZALEA_G(controllerName) = zend_string_tolower(Z_STR_P(field));

			// action
			field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset);
			if (field) {
				++pathsOffset;
				if (AZALEA_G(actionName)) {
					zend_string_release(AZALEA_G(actionName));
				}
				AZALEA_G(actionName) = zend_string_tolower(Z_STR_P(field));

				// arguments
				uint32_t num = zend_hash_num_elements(Z_ARRVAL_P(paths));
				if (num - pathsOffset > 0) {  // has arguments
					zval *args = &AZALEA_G(pathArgs);
					zval_ptr_dtor(args);
					array_init_size(args, num - pathsOffset);
					while ((field = zend_hash_index_find(Z_ARRVAL_P(paths), pathsOffset++))) {
						field = zend_hash_next_index_insert_new(Z_ARRVAL_P(args), field);
						zval_add_ref(field);
					}
				}
			}
		}
	}
	zend_string_release(controllersPath);
	zend_string_release(modelsPath);
	zend_string_release(viewsPath);
	if (!AZALEA_G(controllerName)) {
		// default controller
		conf = azaleaConfigSubFind("dispatch", "default_controller");
		AZALEA_G(controllerName) = Z_STR_P(conf);
		zval_add_ref(conf);
	}
	if (!AZALEA_G(actionName)) {
		// default controller
		conf = azaleaConfigSubFind("dispatch", "default_action");
		AZALEA_G(actionName) = Z_STR_P(conf);
		zval_add_ref(conf);
	}

	// session start
	php_session_start();

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

/* {{{ proto azaleaDispatch */
PHPAPI zend_bool azaleaDispatch(zend_string *folderName, zend_string *controllerName, zend_string *actionName, zval *pathArgs, zval *ret)
{
	zend_string *name, *lcName, *controllerClass, *actionMethod, *tstr;
	zend_class_entry *ce;
	azalea_controller_t *instance = NULL, rv = {{0}};

	// controller name
	name = zend_string_dup(controllerName, 0);
	ZSTR_VAL(name)[0] = toupper(ZSTR_VAL(name)[0]);  // ucfirst
	controllerClass = strpprintf(0, "%sController", ZSTR_VAL(name));
	zend_string_release(name);
	if (folderName) {
		name = zend_string_dup(folderName, 0);
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
			zend_string *controllerPath;
			controllerPath = zend_string_copy(AZALEA_G(controllersPath));
			if (folderName) {
				tstr = controllerPath;
				controllerPath = strpprintf(0, "%s%c%s", ZSTR_VAL(controllerPath), DEFAULT_SLASH, ZSTR_VAL(folderName));
				zend_string_release(tstr);
			}
			tstr = controllerPath;
			controllerPath = strpprintf(0, "%s%c%s.php", ZSTR_VAL(controllerPath), DEFAULT_SLASH, ZSTR_VAL(controllerName));
			zend_string_release(tstr);
			bool error = 1;
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
				int status = azaleaRequire(ZSTR_VAL(controllerPath), ZSTR_LEN(controllerPath));
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
				if (!instanceof_function(ce, azalea_controller_ce)) {
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
			ZVAL_FALSE(ret);
			return 0;
		}

		// controller construct
		if (folderName) {
			zend_update_property_string(azalea_controller_ce, instance, ZEND_STRL("_folderName"), ZSTR_VAL(folderName));
		} else {
			zend_update_property_null(azalea_controller_ce, instance, ZEND_STRL("_folderName"));
		}
		zend_update_property_str(azalea_controller_ce, instance, ZEND_STRL("_controllerName"), controllerName);

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
	if (zend_hash_str_exists(&(ce->function_table), ZEND_STRL("__router"))) {
		zval newRouter, arg, *field;
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
			// only process action and arguments
			if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("action")))) {
				zend_string_release(name);
				name = zend_string_init(Z_STRVAL_P(field), Z_STRLEN_P(field), 0);
			}
			if ((field = zend_hash_str_find(Z_ARRVAL(newRouter), ZEND_STRL("arguments"))) && Z_TYPE_P(field) == IS_ARRAY) {
				zval_ptr_dtor(pathArgs);
				array_init(pathArgs);
				zend_hash_copy(Z_ARRVAL_P(pathArgs), Z_ARRVAL_P(field), (copy_ctor_func_t)zval_add_ref);
			}
		}
		zval_ptr_dtor(&newRouter);
	}

	// action method name
	if (strcmp(ZSTR_VAL(AZALEA_G(environ)), "WEB")) {
		// not WEB
		actionMethod = strpprintf(0, "%s%s", ZSTR_VAL(name), ZSTR_VAL(AZALEA_G(environ)));
	} else {
		// WEB
		actionMethod = strpprintf(0, "%sAction", ZSTR_VAL(name));
	}
	zend_string_release(name);

	// check action method
	lcName = zend_string_tolower(actionMethod);
	if (!(zend_hash_exists(&(ce->function_table), lcName))) {
		tstr = strpprintf(0, "Action method `%s` not found.", ZSTR_VAL(actionMethod));
		throw404(tstr);
		zend_string_release(tstr);
		zend_string_release(actionMethod);
		ZVAL_FALSE(ret);
		return 0;
	}
	zend_string_release(actionMethod);

	// execute action
	zval functionName, *callArgs = NULL;
	uint32_t callArgsCount, current;
	callArgsCount = zend_hash_num_elements(Z_ARRVAL_P(pathArgs));
	if (callArgsCount) {
		zval *arg;
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
