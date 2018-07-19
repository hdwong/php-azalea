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
PHP_FUNCTION(azalea_randomString);
PHP_FUNCTION(azalea_maskString);
PHP_FUNCTION(azalea_debug);

double azaleaGetMicrotime();
zend_string * azaleaUrl(zend_string *url, zend_bool includeHost, zend_bool forceHttps);
zval * azaleaGlobalsFind(uint type, zend_string *name);
zval * azaleaGlobalsStrFind(uint type, char *name, size_t len);
zend_bool azaleaDebugMode();
void azaleaDeepCopy(zval *dst, zval *src);

#endif
