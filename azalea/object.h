/*
 * azalea/object.h
 *
 * Created by Bun Wong on 17-11-27.
 */

#ifndef AZALEA_OBJECT_H_
#define AZALEA_OBJECT_H_

AZALEA_STARTUP_FUNCTION(object);

PHP_METHOD(azalea_object, loadModel);
PHP_METHOD(azalea_object, getModel);
PHP_METHOD(azalea_object, getInstance);

extern zend_class_entry *azaleaObjectCe;

#endif /* AZALEA_OBJECT_H_ */
