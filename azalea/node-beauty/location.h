/*
 * location.h
 *
 * Created by Bun Wong on 16-8-23.
 */

#ifndef AZALEA_NODE_BEAUTY_LOCATION_H_
#define AZALEA_NODE_BEAUTY_LOCATION_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(location);

PHP_METHOD(azalea_node_beauty_location, __construct);
PHP_METHOD(azalea_node_beauty_location, get);
PHP_METHOD(azalea_node_beauty_location, children);
PHP_METHOD(azalea_node_beauty_location, ip);

extern zend_class_entry *azalea_node_beauty_location_ce;

#endif /* AZALEA_NODE_BEAUTY_LOCATION_H_ */
