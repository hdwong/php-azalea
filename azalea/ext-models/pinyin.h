/*
 * pinyin.h
 *
 * Created by Bun Wong on 16-9-16.
 */

#ifndef AZALEA_EXT_MODELS_PINYIN_H_
#define AZALEA_EXT_MODELS_PINYIN_H_

AZALEA_EXT_MODEL_STARTUP_FUNCTION(pinyin);

PHP_METHOD(azalea_ext_model_pinyin, first);
PHP_METHOD(azalea_ext_model_pinyin, token);

extern zend_class_entry *azalea_ext_model_pinyin_ce;

#endif /* AZALEA_EXT_MODELS_PINYIN_H_ */
