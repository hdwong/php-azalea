/*
 * azalea/service.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_SERVICE_H_
#define AZALEA_SERVICE_H_

#define AZALEA_SERVICE_METHOD_GET    1
#define AZALEA_SERVICE_METHOD_POST   2
#define AZALEA_SERVICE_METHOD_PUT    3
#define AZALEA_SERVICE_METHOD_DELETE 4

AZALEA_STARTUP_FUNCTION(service);

PHP_METHOD(azalea_service, get);
PHP_METHOD(azalea_service, post);
PHP_METHOD(azalea_service, put);
PHP_METHOD(azalea_service, delete);
PHP_METHOD(azalea_service, request);

zend_class_entry * azaleaServiceGetNodeBeautyClassEntry(zend_string *name);

extern zend_class_entry *azalea_service_ce;

#endif /* AZALEA_SERVICE_H_ */
