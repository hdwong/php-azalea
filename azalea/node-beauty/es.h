/*
 * es.h
 *
 * Created by Bun Wong on 16-9-11.
 */

#ifndef AZALEA_NODE_BEAUTY_ES_H_
#define AZALEA_NODE_BEAUTY_ES_H_

#define ES_AUTO_COMMIT 20

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(es);

PHP_METHOD(azalea_node_beauty_es, __construct);
PHP_METHOD(azalea_node_beauty_es, __init);
PHP_METHOD(azalea_node_beauty_es, escape);
PHP_METHOD(azalea_node_beauty_es, ping);
PHP_METHOD(azalea_node_beauty_es, query);
PHP_METHOD(azalea_node_beauty_es, index);
PHP_METHOD(azalea_node_beauty_es, delete);
PHP_METHOD(azalea_node_beauty_es, commit);

extern zend_class_entry *azalea_node_beauty_es_ce;

#endif /* AZALEA_NODE_BEAUTY_ES_H_ */
