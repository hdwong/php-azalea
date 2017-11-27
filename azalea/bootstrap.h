/*
 * azalea/bootstrap.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_BOOTSTRAP_H_
#define AZALEA_BOOTSTRAP_H_

AZALEA_STARTUP_FUNCTION(bootstrap);

PHP_METHOD(azalea_bootstrap, __construct);
PHP_METHOD(azalea_bootstrap, init);
PHP_METHOD(azalea_bootstrap, run);
PHP_METHOD(azalea_bootstrap, getRouter);

#define azaleaDispatch(f,c,a,p,r) azaleaDispatchEx(f,c,a,0,p,r);
zend_bool azaleaDispatchEx(zend_string *folderName, zend_string *controllerName, zend_string *actionName, zend_bool isCallback, zval *pathArgs, zval *ret);

#endif /* AZALEA_BOOTSTRAP_H_ */
