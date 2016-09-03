/*
 * upyun.h
 *
 * Created by Bun Wong on 16-9-3.
 */

#ifndef AZALEA_NODE_BEAUTY_UPYUN_H_
#define AZALEA_NODE_BEAUTY_UPYUN_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(upyun);

PHP_METHOD(azalea_node_beauty_upyun, __construct);
PHP_METHOD(azalea_node_beauty_upyun, write);
PHP_METHOD(azalea_node_beauty_upyun, upload);
PHP_METHOD(azalea_node_beauty_upyun, copyUrl);
PHP_METHOD(azalea_node_beauty_upyun, remove);

extern zend_class_entry *azalea_node_beauty_upyun_ce;


#endif /* AZALEA_NODE_BEAUTY_UPYUN_H_ */
