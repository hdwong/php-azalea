/*
 * azalea/config.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_CONFIG_H_
#define AZALEA_CONFIG_H_

zval * loadConfig(zval *);
zval * getConfig(const char *key);

AZALEA_STARTUP_FUNCTION(config);

PHP_METHOD(azalea_config, all);
PHP_METHOD(azalea_config, get);
PHP_METHOD(azalea_config, set);

extern zend_class_entry *azalea_config_ce;

#endif /* AZALEA_CONFIG_H_ */
