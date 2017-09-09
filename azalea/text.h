/*
 * azalea/text.h
 *
 * Created by Bun Wong on 17-9-5.
 */

#ifndef AZALEA_TEXT_H_
#define AZALEA_TEXT_H_

AZALEA_STARTUP_FUNCTION(text);

PHP_METHOD(azalea_text, __construct);
PHP_METHOD(azalea_text, random);
PHP_METHOD(azalea_text, mask);

extern zend_class_entry *azaleaTextCe;

#endif /* AZALEA_TEXT_H_ */
