/*
 * azalea/azalea.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_AZALEA_H
#define AZALEA_AZALEA_H

AZALEA_STARTUP_FUNCTION(azalea);

extern const zend_function_entry azalea_functions[];

PHP_FUNCTION(azalea_timer);
PHP_FUNCTION(azalea_url);
PHP_FUNCTION(azalea_env);
PHP_FUNCTION(azalea_ip);
PHP_FUNCTION(azalea_randomString);
PHP_FUNCTION(azalea_maskString);

PHPAPI double azaleaGetMicrotime();
PHPAPI zend_string * azaleaUrl(zend_string *url, zend_bool includeHost);
PHPAPI zval * azaleaGlobalsFind(uint type, zend_string *name);
PHPAPI zval * azaleaGlobalsStrFind(uint type, char *name, size_t len);
PHPAPI void azaleaSetHeaderStr(char *line, size_t len, zend_long httpCode);
#define azaleaSetHeader(string, httpCode) azaleaSetHeaderStr((string)->val, (string)->len, httpCode)
PHPAPI int azaleaRequire(char *path, size_t len);
PHPAPI void azaleaLoadModel(INTERNAL_FUNCTION_PARAMETERS, zval *from);

#endif
