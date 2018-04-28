/*
 * message.c
 *
 * Created by Bun Wong on 17-12-11.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/message.h"
#ifdef WITH_I18N
#include "azalea/i18n.h"
#endif

#include "ext/standard/php_var.h"
#include "ext/session/php_session.h"	// for php_*_session_var

zend_class_entry *azaleaMessageCe;

/* {{{ class Azalea\Message methods */
static zend_function_entry azalea_message_methods[] = {
	PHP_ME(azalea_message, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_message, has, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, get, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, clean, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, add, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addInfo, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addWarning, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addSuccess, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addError, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#ifdef WITH_I18N
	PHP_ME(azalea_message, addT, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addInfoT, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addWarningT, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addSuccessT, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(azalea_message, addErrorT, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#endif
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_STARTUP_FUNCTION(message)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(Message), azalea_message_methods);
	azaleaMessageCe = zend_register_internal_class(&ce);
	azaleaMessageCe->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_message, __construct) {}
/* }}} */

static zval * azaleaMessageFindSpace(zend_string *space)
{
	zend_array *session;
	zval *pMessages;

	// IF_SESSION_VARS()
	if (!Z_ISREF_P(&PS(http_session_vars)) || Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) != IS_ARRAY) {
		return NULL;
	}
	// find _messages
	session = Z_ARRVAL_P(Z_REFVAL(PS(http_session_vars)));
	if (!(pMessages = zend_hash_str_find(session, ZEND_STRL("_messages")))) {
		return NULL;
	}
	// find space
	if (space == NULL) {
		space = ZSTR_EMPTY_ALLOC();
	}
	return zend_hash_find(Z_ARRVAL_P(pMessages), space);
}

/* {{{ proto has */
PHP_METHOD(azalea_message, has)
{
	zval *pSpace;
	zend_string *space = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|S", &space) == FAILURE) {
		return;
	}

	if (!(pSpace = azaleaMessageFindSpace(space)) || Z_TYPE_P(pSpace) != IS_ARRAY) {
		RETURN_FALSE;
	}
	if (zend_hash_num_elements(Z_ARRVAL_P(pSpace))) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto get */
PHP_METHOD(azalea_message, get)
{
	zval *pSpace;
	zend_string *space = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|S", &space) == FAILURE) {
		return;
	}

	array_init(return_value);
	if (!(pSpace = azaleaMessageFindSpace(space)) || Z_TYPE_P(pSpace) != IS_ARRAY) {
		return;
	}
	azaleaDeepCopy(return_value, pSpace);
	zend_hash_clean(Z_ARRVAL_P(pSpace));
}
/* }}} */

/* {{{ proto clean */
PHP_METHOD(azalea_message, clean)
{
	zend_array *session;
	zval *pMessages;

	// IF_SESSION_VARS()
	if (!Z_ISREF_P(&PS(http_session_vars)) || Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) != IS_ARRAY) {
		RETURN_FALSE;
	}
	// find _messages
	session = Z_ARRVAL_P(Z_REFVAL(PS(http_session_vars)));
	if ((pMessages = zend_hash_str_find(session, ZEND_STRL("_messages")))) {
		zend_hash_clean(Z_ARRVAL_P(pMessages));
	}
}
/* }}} */

static void azaleaMessageAdd(zend_string *message, zend_string *level, zend_string *space, zval *return_value)
{
	zend_array *session;
	zval *pMessages, *pSpace, dummy;

	// IF_SESSION_VARS()
	if (!Z_ISREF_P(&PS(http_session_vars)) || Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) != IS_ARRAY) {
		RETURN_FALSE;
	}

	// find _messages
	session = Z_ARRVAL_P(Z_REFVAL(PS(http_session_vars)));
	if (!(pMessages = zend_hash_str_find(session, ZEND_STRL("_messages")))) {
		array_init(&dummy);
		pMessages = zend_hash_str_add(session, "_messages", sizeof ("_messages") - 1,  &dummy);
	}
	// find space
	if (space == NULL) {
		space = ZSTR_EMPTY_ALLOC();
	}
	if (!(pSpace = zend_hash_find(Z_ARRVAL_P(pMessages), space))) {
		array_init(&dummy);
		pSpace = zend_hash_add(Z_ARRVAL_P(pMessages), space, &dummy);
	}
	// add message
	array_init(&dummy);
	add_assoc_str_ex(&dummy, ZEND_STRL("level"), zend_string_copy(level));
	add_assoc_str_ex(&dummy, ZEND_STRL("message"), zend_string_copy(message));
	if (SUCCESS == add_next_index_zval(pSpace, &dummy)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}

/* {{{ proto add */
PHP_METHOD(azalea_message, add)
{
	zend_string *message, *level, *space = NULL;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "SS|S", &message, &level, &space) == FAILURE) {
		return;
	}

	azaleaMessageAdd(message, level, space, return_value);
}
/* }}} */

/* {{{ proto addInfo */
PHP_METHOD(azalea_message, addInfo)
{
	zend_string *message, *level, *space = NULL;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|S", &message, &space) == FAILURE) {
		return;
	}

	level = zend_string_init(ZEND_STRL("info"), 0);
	azaleaMessageAdd(message, level, space, return_value);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addWarning */
PHP_METHOD(azalea_message, addWarning)
{
	zend_string *message, *level, *space = NULL;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|S", &message, &space) == FAILURE) {
		return;
	}

	level = zend_string_init(ZEND_STRL("warning"), 0);
	azaleaMessageAdd(message, level, space, return_value);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addSuccess */
PHP_METHOD(azalea_message, addSuccess)
{
	zend_string *message, *level, *space = NULL;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|S", &message, &space) == FAILURE) {
		return;
	}

	level = zend_string_init(ZEND_STRL("success"), 0);
	azaleaMessageAdd(message, level, space, return_value);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addError */
PHP_METHOD(azalea_message, addError)
{
	zend_string *message, *level, *space = NULL;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|S", &message, &space) == FAILURE) {
		return;
	}

	level = zend_string_init(ZEND_STRL("error"), 0);
	azaleaMessageAdd(message, level, space, return_value);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addT */
PHP_METHOD(azalea_message, addT)
{
	zval *value, dummy;
	zend_string *message, *level, *space = NULL, *tstr;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "zS|S", &value, &level, &space) == FAILURE) {
		return;
	}

	if (!azaleaI18nTranslateZval(&dummy, value)) {
		RETURN_FALSE;
	}

	message = Z_STR(dummy);
	azaleaMessageAdd(message, level, space, return_value);
	zval_ptr_dtor(&dummy);
}
/* }}} */

/* {{{ proto addInfoT */
PHP_METHOD(azalea_message, addInfoT)
{
	zval *value, dummy;
	zend_string *message, *level, *space = NULL, *tstr;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|S", &value, &space) == FAILURE) {
		return;
	}

	if (!azaleaI18nTranslateZval(&dummy, value)) {
		RETURN_FALSE;
	}

	level = zend_string_init(ZEND_STRL("info"), 0);
	message = Z_STR(dummy);
	azaleaMessageAdd(message, level, space, return_value);
	zval_ptr_dtor(&dummy);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addWarningT */
PHP_METHOD(azalea_message, addWarningT)
{
	zval *value, dummy;
	zend_string *message, *level, *space = NULL, *tstr;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|S", &value, &space) == FAILURE) {
		return;
	}

	if (!azaleaI18nTranslateZval(&dummy, value)) {
		RETURN_FALSE;
	}

	level = zend_string_init(ZEND_STRL("warning"), 0);
	message = Z_STR(dummy);
	azaleaMessageAdd(message, level, space, return_value);
	zval_ptr_dtor(&dummy);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addSuccessT */
PHP_METHOD(azalea_message, addSuccessT)
{
	zval *value, dummy;
	zend_string *message, *level, *space = NULL, *tstr;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|S", &value, &space) == FAILURE) {
		return;
	}

	if (!azaleaI18nTranslateZval(&dummy, value)) {
		RETURN_FALSE;
	}

	level = zend_string_init(ZEND_STRL("success"), 0);
	message = Z_STR(dummy);
	azaleaMessageAdd(message, level, space, return_value);
	zval_ptr_dtor(&dummy);
	zend_string_release(level);
}
/* }}} */

/* {{{ proto addError */
PHP_METHOD(azalea_message, addErrorT)
{
	zval *value, dummy;
	zend_string *message, *level, *space = NULL, *tstr;
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z|S", &value, &space) == FAILURE) {
		return;
	}

	if (!azaleaI18nTranslateZval(&dummy, value)) {
		RETURN_FALSE;
	}

	level = zend_string_init(ZEND_STRL("error"), 0);
	message = Z_STR(dummy);
	azaleaMessageAdd(message, level, space, return_value);
	zval_ptr_dtor(&dummy);
	zend_string_release(level);
}
/* }}} */
