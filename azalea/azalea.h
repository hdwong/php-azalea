/*
 * azalea/azalea.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_AZALEA_H
#define AZALEA_AZALEA_H

extern const zend_function_entry azalea_functions[];

PHP_FUNCTION(azalea_timer);
PHP_FUNCTION(azalea_url);
PHP_FUNCTION(azalea_env);
PHP_FUNCTION(azalea_ip);
PHP_FUNCTION(azalea_randomString);
PHP_FUNCTION(azalea_maskString);

double azaleaGetMicrotime();
zend_string * azaleaUrl(zend_string *url, zend_bool includeHost);
zend_string * azaleaRequestIp();
zval * azaleaGlobalsFind(uint type, zend_string *name);
zval * azaleaGlobalsStrFind(uint type, char *name, size_t len);
int azaleaSetHeaderStr(char *line, size_t len);
int azaleaSetHeaderStrWithCode(char *line, size_t len, zend_long httpCode);

#endif
