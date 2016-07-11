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

#ifndef PHP_AZALEA_H
#define PHP_AZALEA_H

extern zend_module_entry azalea_module_entry;
#define phpext_azalea_ptr &azalea_module_entry

#define PHP_AZALEA_VERSION "0.2.0"

#define AZALEA_STARTUP(module)				ZEND_MODULE_STARTUP_N(azalea_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define AZALEA_STARTUP_FUNCTION(module)		ZEND_MINIT_FUNCTION(azalea_##module)
#define AZALEA_SHUTDOWN_FUNCTION(module)	ZEND_MSHUTDOWN_FUNCTION(azalea_##module)
#define AZALEA_SHUTDOWN(module)				ZEND_MODULE_SHUTDOWN_N(azalea_##module)(INIT_FUNC_ARGS_PASSTHRU)

#ifdef PHP_WIN32
#	define PHP_AZALEA_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_AZALEA_API __attribute__ ((visibility("default")))
#else
#	define PHP_AZALEA_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define azalea_bootstrap_t zval
#define azalea_config_t zval
#define azalea_controller_t zval
#define azalea_request_t zval
#define azalea_response_t zval
#define azalea_session_t zval
#define azalea_view_t zval
#define azalea_exception_t zval

ZEND_BEGIN_MODULE_GLOBALS(azalea)
	double request_time;
	zend_bool bootstrap;
	zend_string *environ;
	zend_string *directory;
	zend_string *uri;
	zend_string *baseUri;
	zend_string *ip;
	zend_string *host;
	zend_string *controllersPath;
	zend_string *modelsPath;
	zend_string *viewsPath;

	zend_string *folderName;
	zend_string *controllerName;
	zend_string *actionName;
	zval pathArgs;

	zval controllerInsts;
	zval modelInsts;
	azalea_request_t *requestInst;
	azalea_response_t *responseInst;
	azalea_session_t *sessionInst;

	zval config;
ZEND_END_MODULE_GLOBALS(azalea)

extern ZEND_DECLARE_MODULE_GLOBALS(azalea);

/* Always refer to the globals in your function as AZALEA_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define AZALEA_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(azalea, v)

#if defined(ZTS) && defined(COMPILE_DL_AZALEA)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_AZALEA_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
