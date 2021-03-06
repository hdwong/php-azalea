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
PHP_METHOD(azalea_request, getIp);
PHP_METHOD(azalea_request, getQuery);
PHP_METHOD(azalea_request, getQueryTrim);
PHP_METHOD(azalea_request, getPost);
PHP_METHOD(azalea_request, getPostTrim);
PHP_METHOD(azalea_request, getCookie);
PHP_METHOD(azalea_request, getHeader);
PHP_METHOD(azalea_request, isMobile);
PHP_METHOD(azalea_request, isWechat);
PHP_METHOD(azalea_request, isQq);
PHP_METHOD(azalea_request, isIosDevice);
PHP_METHOD(azalea_request, isAndroidDevice);

zend_string * azaleaGetBaseUri(void);
zend_string * azaleaGetUri(void);
zend_string * azaleaGetRequestUri(void);
azalea_request_t * azaleaGetRequest(void);

extern zend_class_entry *azaleaRequestCe;

#endif /* AZALEA_REQUEST_H_ */
