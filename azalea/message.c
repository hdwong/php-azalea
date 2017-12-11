/*
 * message.c
 *
 * Created by Bun Wong on 17-12-11.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/message.h"

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
	if (!Z_ISREF_P(&PS(http_session_vars)) || !Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) == IS_ARRAY) {
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
	if (!Z_ISREF_P(&PS(http_session_vars)) || !Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) == IS_ARRAY) {
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
	if (!Z_ISREF_P(&PS(http_session_vars)) || !Z_TYPE_P(Z_REFVAL(PS(http_session_vars))) == IS_ARRAY) {
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
