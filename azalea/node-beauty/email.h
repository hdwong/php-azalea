/*
 * email.h
 *
 * Created by Bun Wong on 16-9-3.
 */

#ifndef AZALEA_NODE_BEAUTY_EMAIL_H_
#define AZALEA_NODE_BEAUTY_EMAIL_H_

AZALEA_NODE_BEAUTY_STARTUP_FUNCTION(email);

PHP_METHOD(azalea_node_beauty_email, __construct);
PHP_METHOD(azalea_node_beauty_email, send);

extern zend_class_entry *azalea_node_beauty_email_ce;

#endif /* AZALEA_NODE_BEAUTY_EMAIL_H_ */
