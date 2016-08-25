/*
 * mysql.h
 *
 * Created by Bun Wong on 16-8-24.
 */

#ifndef AZALEA_NODE_BEAUTY_MYSQL_H_
#define AZALEA_NODE_BEAUTY_MYSQL_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(mysql);

PHP_METHOD(azalea_node_beauty_mysql, __construct);
PHP_METHOD(azalea_node_beauty_mysql, escape);
PHP_METHOD(azalea_node_beauty_mysql, query);

PHP_METHOD(azalea_node_beauty_mysql_result, __construct);
PHP_METHOD(azalea_node_beauty_mysql_result, getSql);
PHP_METHOD(azalea_node_beauty_mysql_result, getTimer);

PHP_METHOD(azalea_node_beauty_mysql_query, all);
PHP_METHOD(azalea_node_beauty_mysql_query, allWithKey);
PHP_METHOD(azalea_node_beauty_mysql_query, column);
PHP_METHOD(azalea_node_beauty_mysql_query, columnWithKey);
PHP_METHOD(azalea_node_beauty_mysql_query, row);
PHP_METHOD(azalea_node_beauty_mysql_query, field);
PHP_METHOD(azalea_node_beauty_mysql_query, fields);

PHP_METHOD(azalea_node_beauty_mysql_execute, insertId);
PHP_METHOD(azalea_node_beauty_mysql_execute, affected);
PHP_METHOD(azalea_node_beauty_mysql_execute, changed);

extern zend_class_entry *azalea_node_beauty_mysql_ce;

#endif /* AZALEA_NODE_BEAUTY_MYSQL_H_ */
