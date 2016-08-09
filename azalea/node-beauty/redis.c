/*
 * redis.c
 *
 * Created by Bun Wong on 16-8-9.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/service.h"
#include "azalea/exception.h"
#include "azalea/node-beauty/redis.h"

zend_class_entry *azalea_node_beauty_redis_ce;

/* {{{ class RedisModel methods
 */
static zend_function_entry azalea_node_beauty_redis_methods[] = {
	PHP_ME(azalea_node_beauty_redis, keys, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, get, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, set, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, delete, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, incr, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, clean, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_node_beauty_redis, command, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION
 */
AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(redis)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(RedisModel), azalea_node_beauty_redis_methods);
	azalea_node_beauty_redis_ce = zend_register_internal_class_ex(&ce, azalea_service_ce);
	azalea_node_beauty_redis_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, keys)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, get)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, set)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, delete)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, incr)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, clean)
{
}
/* }}} */

/* {{{ proto keys */
PHP_METHOD(azalea_node_beauty_redis, command)
{
}
/* }}} */
