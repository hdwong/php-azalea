/*
 * loader.h
 *
 * Created by Bun Wong on 16-8-10.
 */

#ifndef AZALEA_LOADER_H_
#define AZALEA_LOADER_H_

AZALEA_STARTUP_FUNCTION(loader);

PHP_METHOD(azalea_loader, load);

int azaleaRequire(char *path, zend_bool once);

extern zend_class_entry *azaleaLoaderCe;

#endif /* AZALEA_LOADER_H_ */
