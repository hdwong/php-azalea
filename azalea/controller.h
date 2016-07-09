/*
 * azalea/controller.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_CONTROLLER_H_
#define AZALEA_CONTROLLER_H_

AZALEA_STARTUP_FUNCTION(controller);

PHP_METHOD(azalea_controller, getRequest);
PHP_METHOD(azalea_controller, getResponse);

extern zend_class_entry *azalea_controller_ce;

#endif /* AZALEA_CONTROLLER_H_ */
