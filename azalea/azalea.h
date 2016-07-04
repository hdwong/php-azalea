/*
 * azalea/azalea.h
 *
 * Created by Bun Wong on 16-6-18.
 */

#ifndef AZALEA_AZALEA_H
#define AZALEA_AZALEA_H

double getMicrotime();

PHP_FUNCTION(azalea_randomstring);
PHP_FUNCTION(azalea_timer);
PHP_FUNCTION(azalea_url);
PHP_FUNCTION(azalea_env);
PHP_FUNCTION(azalea_ip);

#endif
