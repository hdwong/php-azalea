/*
 * mysql_sqlbuilder.h
 *
 * Created by Bun Wong on 16-8-27.
 */

#ifndef AZALEA_NODE_BEAUTY_MYSQL_SQLBUILDER_H_
#define AZALEA_NODE_BEAUTY_MYSQL_SQLBUILDER_H_

#define RECKEY_WHERE  0  // where
#define RECKEY_HAVING 1  // having

void mysqlSqlBuilderStartup();

PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, __construct);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, where);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orWhere);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, whereGroupStart);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orWhereGroupStart);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, notWhereGroupStart);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, orNotWhereGroupStart);
PHP_METHOD(azalea_node_beauty_mysql_sqlbuilder, whereGroupEnd);

extern zend_class_entry *mysqlSqlBuilderCe;

#endif /* AZALEA_NODE_BEAUTY_MYSQL_SQLBUILDER_H_ */
