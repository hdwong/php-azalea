/*
 * upyun.c
 *
 * Created by Bun Wong on 16-9-3.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/upyun.h"

#include "ext/standard/php_var.h"
#include "ext/standard/base64.h"  // for php_base64_encode
#include "Zend/zend_interfaces.h"  // for zend_call_method_with_*

zend_class_entry *azalea_node_beauty_upyun_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_node_beauty_upyun_methods[] = {
	PHP_ME(azalea_node_beauty_upyun, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_node_beauty_upyun, write, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_upyun, upload, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_upyun, copyUrl, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_upyun, remove, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(upyun)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(UpyunModel), azalea_node_beauty_upyun_methods);
	azalea_node_beauty_upyun_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_upyun_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_node_beauty_upyun, __construct) {}
/* }}} */

static void upyunWrite(zval *instance, zval *ret, zend_string *type, zend_string *filename, const char *buf, size_t len)
{
	zval arg1, arg2;
	zend_string *content;

	content = php_base64_encode((unsigned char *) buf, len);

	ZVAL_STRINGL(&arg1, "file", sizeof("file") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("type"), zend_string_init(ZSTR_VAL(type), ZSTR_LEN(type), 0));
	add_assoc_str_ex(&arg2, ZEND_STRL("filename"), zend_string_init(ZSTR_VAL(filename), ZSTR_LEN(filename), 0));
	add_assoc_str_ex(&arg2, ZEND_STRL("content"), content);
	zend_call_method_with_2_params(instance, azalea_service_ce, NULL, "post", ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);
}

/* {{{ proto write */
PHP_METHOD(azalea_node_beauty_upyun, write)
{
	zend_string *type, *filename, *content;
	zval ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SSS", &type, &filename, &content) == FAILURE) {
		return;
	}

	upyunWrite(getThis(), &ret, type, filename, ZSTR_VAL(content), ZSTR_LEN(content));
	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto upload */
PHP_METHOD(azalea_node_beauty_upyun, upload)
{
	zend_string *type, *filename, *extname = NULL, *content;
	zval ret;
	php_stream *stream;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS|S", &type, &filename, &extname) == FAILURE) {
		return;
	}

	// load file
	stream = php_stream_open_wrapper_ex(ZSTR_VAL(filename), "rb", REPORT_ERRORS, NULL, NULL);
	if (!stream) {
		php_error_docref(NULL, E_WARNING, "Unable to open '%s' for reading", ZSTR_VAL(filename));
		RETURN_FALSE;
	}
	if (NULL == (content = php_stream_copy_to_mem(stream, PHP_STREAM_COPY_ALL, 0))) {
		php_stream_close(stream);
		php_error_docref(NULL, E_WARNING, "Read '%s' error", ZSTR_VAL(filename));
		RETURN_FALSE;
	}
	php_stream_close(stream);

	if (extname) {
		filename = strpprintf(0, "%s.%s", ZSTR_VAL(filename), ZSTR_VAL(extname));
	} else {
		filename = zend_string_init(ZSTR_VAL(filename), ZSTR_LEN(filename), 0);
	}
	upyunWrite(getThis(), &ret, type, filename, ZSTR_VAL(content), ZSTR_LEN(content));
	zend_string_release(content);
	zend_string_release(filename);
	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto copyUrl */
PHP_METHOD(azalea_node_beauty_upyun, copyUrl)
{
	zend_string *type, *url;
	zval ret, arg1, arg2, *pFile;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "SS", &type, &url) == FAILURE) {
		return;
	}

	ZVAL_STRINGL(&arg1, "copy", sizeof("copy") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("type"), zend_string_init(ZSTR_VAL(type), ZSTR_LEN(type), 0));
	add_assoc_str_ex(&arg2, ZEND_STRL("url"), zend_string_init(ZSTR_VAL(url), ZSTR_LEN(url), 0));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "post", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);

	if (Z_TYPE(ret) == IS_OBJECT) {
		RETVAL_ZVAL(&ret, 1, 0);
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */

/* {{{ proto remove */
PHP_METHOD(azalea_node_beauty_upyun, remove)
{
	zend_string *filename;
	zval ret, arg1, arg2, *pFile;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &filename) == FAILURE) {
		return;
	}

	ZVAL_STRINGL(&arg1, "file", sizeof("file") - 1);
	array_init(&arg2);
	add_assoc_str_ex(&arg2, ZEND_STRL("filename"), zend_string_init(ZSTR_VAL(filename), ZSTR_LEN(filename), 0));
	zend_call_method_with_2_params(getThis(), azalea_service_ce, NULL, "delete", &ret, &arg1, &arg2);
	zval_ptr_dtor(&arg1);
	zval_ptr_dtor(&arg2);

	if (Z_TYPE(ret) == IS_TRUE) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
	zval_ptr_dtor(&ret);
}
/* }}} */
