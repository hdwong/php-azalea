/*
 * redis.h
 *
 * Created by Bun Wong on 16-8-9.
 */

#ifndef AZALEA_NODE_BEAUTY_NODE_BEAUTY_REDIS_H_
#define AZALEA_NODE_BEAUTY_NODE_BEAUTY_REDIS_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(redis);

PHP_METHOD(azalea_node_beauty_redis, __construct);
PHP_METHOD(azalea_node_beauty_redis, keys);
PHP_METHOD(azalea_node_beauty_redis, get);
PHP_METHOD(azalea_node_beauty_redis, set);
PHP_METHOD(azalea_node_beauty_redis, delete);
PHP_METHOD(azalea_node_beauty_redis, incr);
PHP_METHOD(azalea_node_beauty_redis, clean);
PHP_METHOD(azalea_node_beauty_redis, command);

extern zend_class_entry *azalea_node_beauty_redis_ce;

#endif /* AZALEA_NODE_BEAUTY_NODE_BEAUTY_REDIS_H_ */
