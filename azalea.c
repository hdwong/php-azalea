/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/bootstrap.h"
#include "azalea/config.h"
#include "azalea/controller.h"
#include "azalea/request.h"
#include "azalea/response.h"
#include "azalea/session.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/view.h"
#include "azalea/exception.h"

ZEND_DECLARE_MODULE_GLOBALS(azalea);

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(azalea)
{
    REGISTER_NS_STRINGL_CONSTANT(AZALEA_NS, "VERSION", PHP_AZALEA_VERSION, sizeof(PHP_AZALEA_VERSION) - 1, CONST_CS | CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT(AZALEA_NS, "E404", -404, CONST_CS | CONST_PERSISTENT);
    REGISTER_NS_LONG_CONSTANT(AZALEA_NS, "E500", -500, CONST_CS | CONST_PERSISTENT);

    AZALEA_STARTUP(bootstrap);
    AZALEA_STARTUP(config);
    AZALEA_STARTUP(controller);
    AZALEA_STARTUP(request);
    AZALEA_STARTUP(response);
    AZALEA_STARTUP(session);
//    AZALEA_STARTUP(model);
//    AZALEA_STARTUP(service);
//    AZALEA_STARTUP(view);
//    AZALEA_STARTUP(exception);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(azalea)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(azalea)
{
	double now = azaleaGetMicrotime();
	zval *server;

	REGISTER_NS_LONG_CONSTANT(AZALEA_NS, "TIME", (long) now, CONST_CS);
	AZALEA_G(request_time) = now;
	AZALEA_G(environ) = zend_string_init(ZEND_STRL("WEB"), 0);
	AZALEA_G(bootstrap) = 0;
	AZALEA_G(directory) = NULL;
	AZALEA_G(uri) = NULL;
	AZALEA_G(baseUri) = NULL;
	AZALEA_G(ip) = NULL;
	AZALEA_G(host) = NULL;
	AZALEA_G(controllersPath) = NULL;
	AZALEA_G(modelsPath) = NULL;
	AZALEA_G(viewsPath) = NULL;

	AZALEA_G(folderName) = NULL;
	AZALEA_G(controllerName) = NULL;
	AZALEA_G(actionName) = NULL;
	array_init(&AZALEA_G(pathArgs));

	array_init(&AZALEA_G(controllerInsts));
	array_init(&AZALEA_G(modelInsts));
	AZALEA_G(requestInst) = NULL;
	AZALEA_G(responseInst) = NULL;
	AZALEA_G(sessionInst) = NULL;

	array_init(&AZALEA_G(config));

#if defined(COMPILE_DL_AZALEA) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(azalea)
{
	if (AZALEA_G(environ)) {
		zend_string_release(AZALEA_G(environ));
	}
	if (AZALEA_G(directory)) {
		zend_string_release(AZALEA_G(directory));
	}
	if (AZALEA_G(uri)) {
		zend_string_release(AZALEA_G(uri));
	}
	if (AZALEA_G(baseUri)) {
		zend_string_release(AZALEA_G(baseUri));
	}
	if (AZALEA_G(ip)) {
		zend_string_release(AZALEA_G(ip));
	}
	if (AZALEA_G(host)) {
		zend_string_release(AZALEA_G(host));
	}
	if (AZALEA_G(controllersPath)) {
		zend_string_release(AZALEA_G(controllersPath));
	}
	if (AZALEA_G(modelsPath)) {
		zend_string_release(AZALEA_G(modelsPath));
	}
	if (AZALEA_G(viewsPath)) {
		zend_string_release(AZALEA_G(viewsPath));
	}

	if (AZALEA_G(folderName)) {
		zend_string_release(AZALEA_G(folderName));
	}
	if (AZALEA_G(controllerName)) {
		zend_string_release(AZALEA_G(controllerName));
	}
	if (AZALEA_G(actionName)) {
		zend_string_release(AZALEA_G(actionName));
	}
	zval_ptr_dtor(&AZALEA_G(pathArgs));

	zval_ptr_dtor(&AZALEA_G(controllerInsts));
	zval_ptr_dtor(&AZALEA_G(modelInsts));
	if (AZALEA_G(requestInst)) {
		zval_ptr_dtor(AZALEA_G(requestInst));
	}
	if (AZALEA_G(responseInst)) {
		zval_ptr_dtor(AZALEA_G(responseInst));
	}
	if (AZALEA_G(sessionInst)) {
		zval_ptr_dtor(AZALEA_G(sessionInst));
	}

	zval_ptr_dtor(&AZALEA_G(config));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(azalea)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "azalea support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ azalea_functions[]
 *
 * Every user visible function must have an entry in azalea_functions[].
 */
const zend_function_entry azalea_functions[] = {
	ZEND_NS_NAMED_FE(AZALEA_NS, randomString, ZEND_FN(azalea_randomstring), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, timer, ZEND_FN(azalea_timer), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, url, ZEND_FN(azalea_url), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, env, ZEND_FN(azalea_env), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, ip, ZEND_FN(azalea_ip), NULL)
	PHP_FE_END	/* Must be the last line in azalea_functions[] */
};
/* }}} */

/* {{{ azalea_module_entry
 */
zend_module_entry azalea_module_entry = {
	STANDARD_MODULE_HEADER,
	"azalea",
	azalea_functions,
	PHP_MINIT(azalea),
	PHP_MSHUTDOWN(azalea),
	PHP_RINIT(azalea),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(azalea),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(azalea),
	PHP_AZALEA_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_AZALEA
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(azalea)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
