/*
 * message.h
 *
 * Created by Bun Wong on 17-12-11.
 */

#ifndef AZALEA_MESSAGE_H_
#define AZALEA_MESSAGE_H_

AZALEA_STARTUP_FUNCTION(message);

PHP_METHOD(azalea_message, __construct);
PHP_METHOD(azalea_message, has);
PHP_METHOD(azalea_message, get);
PHP_METHOD(azalea_message, clean);
PHP_METHOD(azalea_message, add);
PHP_METHOD(azalea_message, addInfo);
PHP_METHOD(azalea_message, addWarning);
PHP_METHOD(azalea_message, addSuccess);
PHP_METHOD(azalea_message, addError);

extern zend_class_entry *azaleaMessageCe;


#endif /* AZALEA_MESSAGE_H_ */
