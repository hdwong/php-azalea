/*
 * azalea/exception.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_EXCEPTION_H_
#define AZALEA_EXCEPTION_H_

AZALEA_STARTUP_FUNCTION(exception);

PHP_METHOD(azalea_exception404, getUri);
PHP_METHOD(azalea_exception404, getRouter);

#define throw404(message) \
	if (message) throw404Str((message)->val, (message)->len); \
	else throw404Str("", 0);
void throw404Str(const char *message, size_t len);
void throw500Str(const char *message, size_t len);

extern zend_class_entry *azaleaExceptionCe;
extern zend_class_entry *azaleaException404Ce;
extern zend_class_entry *azaleaException500Ce;

#endif /* AZALEA_EXCEPTION_H_ */
