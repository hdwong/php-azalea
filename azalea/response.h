/*
 * azalea/response.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_RESPONSE_H_
#define AZALEA_RESPONSE_H_

AZALEA_STARTUP_FUNCTION(response);

PHP_METHOD(azalea_response, __construct);
PHP_METHOD(azalea_response, gotoUrl);
PHP_METHOD(azalea_response, reload);
PHP_METHOD(azalea_response, gotoRoute);
PHP_METHOD(azalea_response, setHeader);
PHP_METHOD(azalea_response, getBody);
PHP_METHOD(azalea_response, setBody);
PHP_METHOD(azalea_response, setCookie);

azalea_response_t * azaleaGetResponse(azalea_controller_t *controller);

extern zend_class_entry *azaleaResponseCe;

#endif /* AZALEA_RESPONSE_H_ */
