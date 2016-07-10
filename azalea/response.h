/*
 * azalea/response.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_RESPONSE_H_
#define AZALEA_RESPONSE_H_

AZALEA_STARTUP_FUNCTION(response);

PHP_METHOD(azalea_response, gotoUrl);
PHP_METHOD(azalea_response, gotoRoute);
PHP_METHOD(azalea_response, getBody);
PHP_METHOD(azalea_response, setBody);
PHP_METHOD(azalea_response, setCookie);

extern zend_class_entry *azalea_response_ce;

#endif /* AZALEA_RESPONSE_H_ */
