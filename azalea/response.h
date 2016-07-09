/*
 * azalea/response.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_RESPONSE_H_
#define AZALEA_RESPONSE_H_

AZALEA_STARTUP_FUNCTION(response);

PHP_METHOD(azalea_request, gotoUrl);
PHP_METHOD(azalea_request, gotoRoute);
PHP_METHOD(azalea_request, getBody);
PHP_METHOD(azalea_request, setBody);
PHP_METHOD(azalea_request, setCookie);

extern zend_class_entry *azalea_response_ce;

#endif /* AZALEA_RESPONSE_H_ */
