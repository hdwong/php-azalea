/*
 * azalea/exception.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_EXCEPTION_H_
#define AZALEA_EXCEPTION_H_

AZALEA_STARTUP_FUNCTION(exception);

PHP_METHOD(azalea_exception404, getUri);
PHP_METHOD(azalea_exception404, getRoute);
PHP_METHOD(azalea_exception500, getServiceInfo);

PHPAPI void throw404Str(const char *message, size_t len);
#define throw404(message) throw404Str((message)->val, (message)->len)

PHPAPI void throw500Str(const char *message, size_t len, const char *method, const char *serviceUrl, zval *arguments);

extern zend_class_entry *azalea_exception_ce;
extern zend_class_entry *azalea_exception404_ce;
extern zend_class_entry *azalea_exception500_ce;

#endif /* AZALEA_EXCEPTION_H_ */
