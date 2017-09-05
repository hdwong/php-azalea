/*
 * azalea/config.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_CONFIG_H_
#define AZALEA_CONFIG_H_

AZALEA_STARTUP_FUNCTION(config);

PHP_METHOD(azalea_config, get);
PHP_METHOD(azalea_config, getAll);

void azaleaLoadConfig(zval *);
#define azaleaConfigFind(key) azaleaConfigSubFindEx(key, strlen(key), NULL, 0)
#define azaleaConfigSubFind(key, subKey) azaleaConfigSubFindEx(key, strlen(key), subKey, strlen(subKey))
zval * azaleaConfigSubFindEx(const char *key, size_t lenKey, const char *subKey, size_t lenSubKey);

extern zend_class_entry *azalea_config_ce;

#endif /* AZALEA_CONFIG_H_ */
