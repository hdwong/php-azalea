/*
 * azalea/config.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_CONFIG_H_
#define AZALEA_CONFIG_H_

AZALEA_STARTUP_FUNCTION(config);
AZALEA_SHUTDOWN_FUNCTION(config);

PHP_METHOD(azalea_config, get);
PHP_METHOD(azalea_config, getSub);
PHP_METHOD(azalea_config, getAll);

PHPAPI void azaleaLoadConfig(zval *);
#define azaleaConfigFind(key) azaleaConfigSubFind(key, NULL)
PHPAPI zval * azaleaConfigSubFind(const char *key, const char *subKey);

extern zend_class_entry *azalea_config_ce;

#endif /* AZALEA_CONFIG_H_ */
