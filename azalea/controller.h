/*
 * azalea/controller.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_CONTROLLER_H_
#define AZALEA_CONTROLLER_H_

AZALEA_STARTUP_FUNCTION(controller);

PHP_METHOD(azalea_controller, __construct);
PHP_METHOD(azalea_controller, getSession);
PHP_METHOD(azalea_controller, loadModel);
PHP_METHOD(azalea_controller, getModel);
PHP_METHOD(azalea_controller, notFound);

void azaleaControllerInit(zval *this, zend_class_entry *ce, zend_string *folderName, zend_string *controllerName);

extern zend_class_entry *azaleaControllerCe;

#endif /* AZALEA_CONTROLLER_H_ */
