/*
 * azalea/template.h
 *
 * Created by Bun Wong on 16-7-23.
 */

#ifndef AZALEA_TEMPLATE_H_
#define AZALEA_TEMPLATE_H_

PHP_FUNCTION(azalea_template_print);
PHP_FUNCTION(azalea_template_return);
PHP_FUNCTION(azalea_template_translate);

void azaleaRegisterTemplateFunctions();
void azaleaUnregisterTemplateFunctions(zend_bool forced);

#endif /* AZALEA_TEMPLATE_H_ */
