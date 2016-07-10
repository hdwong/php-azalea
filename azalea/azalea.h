/*
 * azalea/azalea.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_AZALEA_H
#define AZALEA_AZALEA_H

double azaleaGetMicrotime();

PHP_FUNCTION(azalea_randomstring);
PHP_FUNCTION(azalea_timer);
PHP_FUNCTION(azalea_url);
PHP_FUNCTION(azalea_env);
PHP_FUNCTION(azalea_ip);

PHPAPI zend_string * azaleaUrl(zend_string *url, zend_bool includeHost);
PHPAPI zval * azaleaGlobalsFind(uint type, zend_string *name);
PHPAPI zval * azaleaGlobalsStrFind(uint type, char *name, size_t len);

#endif
