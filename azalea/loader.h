/*
 * loader.h
 *
 * Created by Bun Wong on 16-8-10.
 */

#ifndef AZALEA_LOADER_H_
#define AZALEA_LOADER_H_

AZALEA_STARTUP_FUNCTION(loader);

PHP_METHOD(azalea_loader, load);

PHPAPI int azaleaRequire(char *path);

extern zend_class_entry *azalea_loader_ce;

#endif /* AZALEA_LOADER_H_ */
