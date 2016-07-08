/*
 * azalea/bootstrap.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_BOOTSTRAP_H_
#define AZALEA_BOOTSTRAP_H_

AZALEA_STARTUP_FUNCTION(bootstrap);

PHP_METHOD(azalea_bootstrap, init);
PHP_METHOD(azalea_bootstrap, run);
PHP_METHOD(azalea_bootstrap, getBaseUri);
PHP_METHOD(azalea_bootstrap, getUri);
PHP_METHOD(azalea_bootstrap, getRequestUri);
PHP_METHOD(azalea_bootstrap, getRoute);

int azalea_require(char *path, size_t len);

#endif /* AZALEA_BOOTSTRAP_H_ */
