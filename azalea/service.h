/*
 * azalea/service.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_SERVICE_H_
#define AZALEA_SERVICE_H_

AZALEA_STARTUP_FUNCTION(service);

PHP_METHOD(azalea_service, getModel);

extern zend_class_entry *azalea_service_ce;

#endif /* AZALEA_SERVICE_H_ */
