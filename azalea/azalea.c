/*
 * azalea/azalea.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/config.h"
#include "azalea/exception.h"

#include "ext/date/php_date.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#elif defined(NETWARE)
#include <sys/timeval.h>
#include <sys/time.h>
#else
#include <sys/time.h>
#endif

#include "ext/standard/php_var.h"	// for php_var_dump function
#include "ext/standard/php_string.h"	// for php_trim function

/* {{{ azalea_functions[] */
const zend_function_entry azalea_functions[] = {
	ZEND_NS_NAMED_FE(AZALEA_NS, timer, ZEND_FN(azalea_timer), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, url, ZEND_FN(azalea_url), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, env, ZEND_FN(azalea_env), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, debug, ZEND_FN(azalea_debug), NULL)
	PHP_FE_END	/* Must be the last line in azalea_functions[] */
};
/* }}} */

/* {{{ proto azaleaUrl */
zend_string * azaleaUrl(zend_string *url, zend_bool includeHost, zend_bool forceHttps)
{
	zval *field;
	zend_string *hostname, *tstr, *returnUrl;

	if (!strncasecmp(ZSTR_VAL(url), ZEND_STRL("http://")) ||
			!strncasecmp(ZSTR_VAL(url), ZEND_STRL("https://"))) {
		return zend_string_copy(url);
	}

	// init AZALEA_G(host)
	if (!AZALEA_G(host)) {
		// check forceHttps
		if (forceHttps != 0) {
			// force https
			hostname = zend_string_init(ZEND_STRL("https://"), 0);
		} else {
			// auto detect
			if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTPS")))) {
				hostname = zend_string_init(ZEND_STRL("https://"), 0);
			} else {
				hostname = zend_string_init(ZEND_STRL("http://"), 0);
			}
		}
		field = azaleaConfigSubFindEx(ZEND_STRL("hostname"), NULL, 0);	// get from config
		if (!field) {
			// host not set, try to get from global
			field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_HOST"));
			if (!field) {
				tstr = strpprintf(0, "%slocalhost", ZSTR_VAL(hostname));
			} else {
				tstr = strpprintf(0, "%s%s", ZSTR_VAL(hostname), Z_STRVAL_P(field));
			}
		} else {
			tstr = strpprintf(0, "%s%s", ZSTR_VAL(hostname), Z_STRVAL_P(field));
		}
		zend_string_release(hostname);
		AZALEA_G(host) = tstr;
	}

	url = php_trim(url, ZEND_STRL("/"), 1);
	returnUrl = strpprintf(0, "%s%s%s", includeHost ? ZSTR_VAL(AZALEA_G(host)) : "",
			AZALEA_G(baseUri) ? ZSTR_VAL(AZALEA_G(baseUri)) : "/", ZSTR_VAL(url));
	zend_string_release(url);
	return returnUrl;
}
/* }}} */

/* {{{ azalea_timer */
PHP_FUNCTION(azalea_timer)
{
	double now = azaleaGetMicrotime();
	RETVAL_DOUBLE(now - AZALEA_G(timer));
	AZALEA_G(timer) = now;
}
/* }}} */

/* {{{ azalea_url */
PHP_FUNCTION(azalea_url)
{
	zend_string *url;
	zend_bool includeHost = 0, forceHttps = 0;
	zval *configForceHttps;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|bb", &url, &includeHost, &forceHttps) == FAILURE) {
		return;
	}

	if (!forceHttps) {
		configForceHttps = azaleaConfigSubFindEx(ZEND_STRL("https"), NULL, 0);
		if (configForceHttps && zend_is_true(configForceHttps)) {
			forceHttps = 1;
		}
	}

	RETURN_STR(azaleaUrl(url, includeHost, forceHttps));
}
/* }}} */

/* {{{ azalea_env */
PHP_FUNCTION(azalea_env)
{
	RETURN_STR_COPY(AZALEA_G(environ));
}
/* }}} */

/* {{{ proto azaleaDebugMode */
zend_bool azaleaDebugMode()
{
	zval *configDebug, *configDebugKey, *debugKeyField;

	configDebug = azaleaConfigSubFindEx(ZEND_STRL("debug"), NULL, 0);
	configDebugKey = azaleaConfigSubFindEx(ZEND_STRL("debug_key"), NULL, 0);
	debugKeyField = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_AZALEA_DEBUG_KEY"));

	if ((!configDebug || Z_TYPE_P(configDebug) != IS_TRUE) &&
			(!configDebugKey || !debugKeyField || Z_STRLEN_P(configDebugKey) == 0 ||
			strcmp(Z_STRVAL_P(configDebugKey), Z_STRVAL_P(debugKeyField)))) {
		return 0;
	}
	return 1;
}
/* }}} */

/* {{{ azalea_vardump */
PHP_FUNCTION(azalea_debug)
{
	zval *var;

	if (!azaleaDebugMode() || zend_parse_parameters(ZEND_NUM_ARGS(), "z", &var) == FAILURE) {
		return;
	}
	php_printf("<pre dir='ltr'>\n<small>%s:%d:</small>", zend_get_executed_filename(), zend_get_executed_lineno());
	php_var_dump(var, 0);
	php_printf("</pre>");
}
/* }}} */

/* {{{ proto azaleaGlobalsFind */
zval * azaleaGlobalsFind(uint type, zend_string *name)
{
	zval *carrier, *field;
	carrier = &PG(http_globals)[type];
	if (!name) {
		return carrier;
	}
	field = zend_hash_find(Z_ARRVAL_P(carrier), name);
	if (!field) {
		return NULL;
	}
	return field;
}
/* }}} */

/* {{{ proto azaleaGlobalsStrFind */
zval * azaleaGlobalsStrFind(uint type, char *name, size_t len)
{
	zval *carrier, *field;
	carrier = &PG(http_globals)[type];
	if (!name) {
		return carrier;
	}
	field = zend_hash_str_find(Z_ARRVAL_P(carrier), name, len);
	if (!field) {
		return NULL;
	}
	return field;
}
/* }}} */

/* {{{ proto azaleaGetMicrotime */
double azaleaGetMicrotime()
{
	struct timeval tp = {0};
	if (gettimeofday(&tp, NULL)) {
		return (double)time(0);
	}
	return (double)(tp.tv_sec + tp.tv_usec / 1000000.00);
}
/* }}} */

/** {{{ proto azaleaDeepCopy(zval *dst, zval *src) */
void azaleaDeepCopy(zval *dst, zval *src) {
	zval *pzval, *dstpzval, value;
	zend_ulong idx;
	zend_string *key;

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(src), idx, key, pzval) {
		if (key) {
			if (Z_TYPE_P(pzval) == IS_ARRAY) {
				array_init(&value);
				if ((dstpzval = zend_hash_find(Z_ARRVAL_P(dst), key)) && Z_TYPE_P(dstpzval) == IS_ARRAY) {
					azaleaDeepCopy(&value, dstpzval);
				}
				azaleaDeepCopy(&value, pzval);
			} else {
				ZVAL_DUP(&value, pzval);
			}
			zend_hash_update(Z_ARRVAL_P(dst), key, &value);
		} else {
			if (Z_TYPE_P(pzval) == IS_ARRAY) {
				array_init(&value);
				if ((dstpzval = zend_hash_index_find(Z_ARRVAL_P(dst), idx)) && Z_TYPE_P(dstpzval) == IS_ARRAY) {
					azaleaDeepCopy(&value, dstpzval);
				}
				azaleaDeepCopy(&value, pzval);
			} else {
				ZVAL_DUP(&value, pzval);
			}
			zend_hash_index_update(Z_ARRVAL_P(dst), idx, &value);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */
