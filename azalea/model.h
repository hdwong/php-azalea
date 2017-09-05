/*
 * azalea/model.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_MODEL_H_
#define AZALEA_MODEL_H_

AZALEA_STARTUP_FUNCTION(model);

PHP_METHOD(azalea_model, getRequest);
PHP_METHOD(azalea_model, loadModel);
PHP_METHOD(azalea_model, getModel);

void azaleaLoadModel(INTERNAL_FUNCTION_PARAMETERS, zval *from);
void azaleaGetModel(INTERNAL_FUNCTION_PARAMETERS, zval *from);

extern zend_class_entry *azaleaModelCe;

#endif /* AZALEA_MODEL_H_ */
