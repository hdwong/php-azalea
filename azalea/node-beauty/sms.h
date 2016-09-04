/*
 * sms.h
 *
 * Created by Bun Wong on 16-9-4.
 */

#ifndef AZALEA_NODE_BEAUTY_SMS_H_
#define AZALEA_NODE_BEAUTY_SMS_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(sms);

PHP_METHOD(azalea_node_beauty_sms, __construct);
PHP_METHOD(azalea_node_beauty_sms, send);

extern zend_class_entry *azalea_node_beauty_sms_ce;

#endif /* AZALEA_NODE_BEAUTY_SMS_H_ */
