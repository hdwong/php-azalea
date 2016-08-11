/*
 * azalea/request.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_REQUEST_H_
#define AZALEA_REQUEST_H_

AZALEA_STARTUP_FUNCTION(request);

PHP_METHOD(azalea_request, __construct);
PHP_METHOD(azalea_request, getUri);
PHP_METHOD(azalea_request, getRequestUri);
PHP_METHOD(azalea_request, getBaseUri);
PHP_METHOD(azalea_request, isPost);
PHP_METHOD(azalea_request, isAjax);
PHP_METHOD(azalea_request, getQuery);
PHP_METHOD(azalea_request, getPost);
PHP_METHOD(azalea_request, getCookie);

zend_string * azaleaGetBaseUri(void);
zend_string * azaleaGetUri(void);
zend_string * azaleaGetRequestUri(void);

extern zend_class_entry *azalea_request_ce;

#endif /* AZALEA_REQUEST_H_ */
