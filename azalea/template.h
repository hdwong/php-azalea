/*
 * azalea/template.h
 *
 * Created by Bun Wong on 16-7-23.
 */

#ifndef AZALEA_TEMPLATE_H_
#define AZALEA_TEMPLATE_H_

PHP_FUNCTION(azalea_template_printf);
PHP_FUNCTION(azalea_template_sprintf);

PHPAPI void azaleaRegisterTemplateFunctions();
PHPAPI void azaleaUnregisterTemplateFunctions();

#endif /* AZALEA_TEMPLATE_H_ */
