/*
 * azalea/azalea.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "azalea.h"
#include "php_azalea.h"

#include "azalea/config.h"

#include "ext/date/php_date.h"
#include "ext/standard/php_rand.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#elif defined(NETWARE)
#include <sys/timeval.h>
#include <sys/time.h>
#else
#include <sys/time.h>
#endif
#define MICRO_IN_SEC 1000000.00

#include "ext/standard/php_var.h"	// for php_var_dump function
#include "ext/standard/php_string.h"  // for php_trim function

/* {{{ azalea_randomstring
 */
PHP_FUNCTION(azalea_randomstring)
{
	long len;
	zend_string *mode;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|S", &len, &mode) == FAILURE) {
		return;
	}
	if (len < 1) {
		php_error_docref(NULL, E_WARNING, "String length is smaller than 1");
		RETURN_FALSE;
	}

	static char *base = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char *p = base;
	size_t l = 62;

	if (mode) {
		if (strcmp(ZSTR_VAL(mode), "10") == 0 || ZSTR_VAL(mode)[0] == 'n') {
			// [0-9]
			l = 10;
		} else if (strcmp(ZSTR_VAL(mode), "16") == 0) {
			// [0-9a-f]
			l = 16;
		} else if (ZSTR_VAL(mode)[0] == 'c') {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (strcmp(ZSTR_VAL(mode), "ln") == 0) {
			// [0-9a-z]
			l = 36;
		} else if (strcmp(ZSTR_VAL(mode), "un") == 0) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (ZSTR_VAL(mode)[0] == 'l') {
			// [a-z]
			p += 10;
			l = 26;
		} else if (ZSTR_VAL(mode)[0] == 'u') {
			// [A-Z]
			p += 36;
			l = 26;
		}
	}

	char result[len + 1];
	result[len] = '\0';
	php_uint32 number;
	l -= 1; // for RAND_RANGE
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}
	for (long i = 0; i < len; ++i) {
		number = php_mt_rand() >> 1;
		RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
		result[i] = *(p + number);
	}
	RETURN_STRING(result);
}
/* }}} */

/* {{{ proto azaleaUrl */
PHPAPI zend_string * azaleaUrl(zend_string *url, zend_bool includeHost)
{
	// init AZALEA_G(host)
	if (!AZALEA_G(host)) {
		zval hostname, *server, *field;

		server = &PG(http_globals)[TRACK_VARS_SERVER];
		if (server && Z_TYPE_P(server) == IS_ARRAY &&
				zend_hash_str_exists(Z_ARRVAL_P(server), ZEND_STRL("HTTPS"))) {
			ZVAL_STRING(&hostname, "https://");
		} else {
			ZVAL_STRING(&hostname, "http://");
		}
		field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_HOST"));
		if (!field) {
			// host not found, try to get from config
			field = azaleaGetConfig("hostname");
			if (!field) {
				zval t;
				ZVAL_STRING(&t, "localhost");
				concat_function(&hostname, &hostname, &t);
				zval_ptr_dtor(&t);
			} else {
				concat_function(&hostname, &hostname, field);
			}
		} else {
			concat_function(&hostname, &hostname, field);
		}
		AZALEA_G(host) = Z_STR(hostname);
	}

	zval ret, t;
	ZVAL_EMPTY_STRING(&ret);
	if (includeHost) {
		ZVAL_STR(&t, AZALEA_G(host));
		concat_function(&ret, &ret, &t);
	}
	ZVAL_STR(&t, AZALEA_G(baseUri));
	concat_function(&ret, &ret, &t);
	ZVAL_STR(&t, url);
	concat_function(&ret, &ret, &t);
	zval_ptr_dtor(&t);

	return Z_STR(ret);
}
/* }}} */

/* {{{ azalea_url
 */
PHP_FUNCTION(azalea_url)
{
	zend_string *url;
	zend_bool includeHost = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &url, &includeHost) == FAILURE) {
		return;
	}

	RETURN_STR(azaleaUrl(url, includeHost));
}
/* }}} */

double azaleaGetMicrotime()
{
    struct timeval tp = {0};
    if (gettimeofday(&tp, NULL)) {
        return 0;
    }
    return (double)(tp.tv_sec + tp.tv_usec / MICRO_IN_SEC);
}

/* {{{ azalea_timer
 */
PHP_FUNCTION(azalea_timer)
{
	double now = azaleaGetMicrotime();
	RETVAL_DOUBLE(now - AZALEA_G(request_time));
	AZALEA_G(request_time) = now;
}
/* }}} */

/* {{{ azalea_env
 */
PHP_FUNCTION(azalea_env)
{
	RETURN_STR(zend_string_copy(AZALEA_G(environ)));
}
/* }}} */

/* {{{ azalea_getmodel
 */
PHP_FUNCTION(azalea_ip)
{
	if (!AZALEA_G(ip)) {
		zval *server, *field;
		zend_string *ip = NULL;

		server = &PG(http_globals)[TRACK_VARS_SERVER];
		if (Z_TYPE_P(server) == IS_ARRAY) {
			if ((field= zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_CLIENT_IP"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			} else if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_X_FORWARDED_FOR"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			} else if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("REMOTE_ADDR"))) &&
					Z_TYPE_P(field) == IS_STRING) {
				ip = Z_STR_P(field);
			}
		}
		if (ip) {
			AZALEA_G(ip) = ip;
		} else {
			AZALEA_G(ip) = zend_string_init(ZEND_STRL("0.0.0.0"), 0);
		}
	}
	RETURN_STR(zend_string_copy(AZALEA_G(ip)));
}
/* }}} */

/* {{{ proto azaleaRequestFind */
PHPAPI zval * azaleaGlobalsFind(uint type, zend_string *name)
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
PHPAPI zval * azaleaGlobalsStrFind(uint type, char *name, size_t len)
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
