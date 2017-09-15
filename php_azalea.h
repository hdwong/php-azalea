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

#define PHP_AZALEA_VERSION "2.0.0"
#define PHP_AZALEA_COPYRIGHT_OUTPUT "X-Framework: Azalea/"PHP_AZALEA_VERSION

#define AZALEA_STARTUP(module)				ZEND_MODULE_STARTUP_N(azalea_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define AZALEA_STARTUP_FUNCTION(module)		ZEND_MINIT_FUNCTION(azalea_##module)
#define AZALEA_SHUTDOWN_FUNCTION(module)	ZEND_MSHUTDOWN_FUNCTION(azalea_##module)
#define AZALEA_SHUTDOWN(module)				ZEND_MODULE_SHUTDOWN_N(azalea_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define AZALEA_EXT_MODEL_STARTUP(module)	ZEND_MODULE_STARTUP_N(azalea_ext_model_##module)(INIT_FUNC_ARGS_PASSTHRU)
#define AZALEA_EXT_MODEL_STARTUP_FUNCTION(module)	ZEND_MINIT_FUNCTION(azalea_ext_model_##module)

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
#define azalea_model_t zval
#define azalea_view_t zval
#define azalea_exception_t zval

ZEND_BEGIN_MODULE_GLOBALS(azalea)
	double timer;					// 计时器
	zend_ulong renderDepth;			// 渲染嵌套层数
	zend_string *environ;			// 运行环境
	zend_string *locale;			// 语言区域
	azalea_bootstrap_t bootstrap;	// Azalea\Bootstrap 实例变量
	zend_bool registeredTemplateFunctions;	// 是否已注册模板函数
	zend_bool startSession;			// 是否开启回话
	zval instances;					// 实例缓存变量
	zval config;					// 配置项变量
	zval translations;				// 翻译字符变量
	zval viewTagFunctionsUser;		// 视图标签变量 (用户定义)

	zend_string *docRoot;			// 入口文件根目录
	zend_string *appRoot;			// 系统文件根目录
	zend_string *uri;
	zend_string *baseUri;
	zend_string *ip;
	zend_string *host;
	zend_string *controllersPath;
	zend_string *modelsPath;
	zend_string *viewsPath;

	zval paths;
	zend_string *folderName;
	zend_string *controllerName;
	zend_string *actionName;
	zval pathArgs;
ZEND_END_MODULE_GLOBALS(azalea)
extern ZEND_DECLARE_MODULE_GLOBALS(azalea);
#define AZALEA_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(azalea, v)

ZEND_BEGIN_MODULE_GLOBALS(azalea_internal)
	int moduleNumber;			// PHP 模块序号
	zend_string *stringWeb;		// 字符串 "WEB"
	zend_string *stringEn;		// 字符串 "en_US"
	zend_string *stringSlash;	// 字符串 "/"
	zend_array *viewTagFunctions;	// 视图标签变量
ZEND_END_MODULE_GLOBALS(azalea_internal)
extern ZEND_DECLARE_MODULE_GLOBALS(azalea_internal);
#define AG(v) ZEND_MODULE_GLOBALS_ACCESSOR(azalea_internal, v)

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
