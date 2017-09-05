/*
 * mysqlnd.h
 *
 * Created by Bun Wong on 17-9-3.
 */

#ifndef AZALEA_EXT_MODELS_MYSQLND_H_
#define AZALEA_EXT_MODELS_MYSQLND_H_

AZALEA_EXT_MODEL_STARTUP_FUNCTION(mysqlnd);

PHP_METHOD(azalea_ext_model_mysqlnd, __construct);

extern void mysqlnd_library_init(void);

extern zend_class_entry *azalea_ext_model_mysqlnd_ce;

#endif /* AZALEA_EXT_MODELS_MYSQLND_H_ */
