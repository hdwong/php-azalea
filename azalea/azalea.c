/*
 * azalea/azalea.c
 *
 * Created by Bun Wong on 16-6-18.
 */

#include "php.h"
#include "azalea.h"
#include "php_azalea.h"

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

/* {{{ azalea_randomstring
 */
PHP_FUNCTION(azalea_randomstring)
{
	long len;
	char *mode = NULL;
	size_t mode_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l|s", &len, &mode, &mode_len) == FAILURE) {
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
		if (strcmp(mode, "10") == 0) {
			// [0-9]
			l = 10;
		} else if (strcmp(mode, "16") == 0) {
			// [0-9a-f]
			l = 16;
		} else if (*mode == 'c') {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (strcmp(mode, "ln") == 0) {
			// [0-9a-z]
			l = 36;
		} else if (strcmp(mode, "un") == 0) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (*mode == 'l') {
			// [a-z]
			p += 10;
			l = 26;
		} else if (*mode == 'u') {
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

/* {{{ azalea_url
 */
PHP_FUNCTION(azalea_url)
{
	RETURN_TRUE;
}
/* }}} */

double getMicrotime()
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
	double now = getMicrotime(), diff = now - AZALEA_G(request_time);
	AZALEA_G(request_time) = now;
	RETURN_DOUBLE(diff);
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
	zend_string *p = NULL;
	zval *server, *zv;
	server = zend_hash_str_find(&EG(symbol_table), "_SERVER", sizeof("_SERVER") - 1);
	if (Z_TYPE_P(server) == IS_ARRAY) {
		if ((zv = zend_hash_str_find(Z_ARRVAL_P(server), "HTTP_CLIENT_IP", sizeof("HTTP_CLIENT_IP") - 1)) &&
					Z_TYPE_P(zv) == IS_STRING) {
			p = Z_STR_P(zv);
		} else if ((zv = zend_hash_str_find(Z_ARRVAL_P(server), "HTTP_X_FORWARDED_FOR", sizeof("HTTP_X_FORWARDED_FOR") - 1)) &&
					Z_TYPE_P(zv) == IS_STRING) {
			p = Z_STR_P(zv);
		} else if ((zv = zend_hash_str_find(Z_ARRVAL_P(server), "REMOTE_ADDR", sizeof("REMOTE_ADDR") - 1)) &&
					Z_TYPE_P(zv) == IS_STRING) {
			p = Z_STR_P(zv);
		}
	}
	RETURN_STR(zend_string_copy(p));
}
/* }}} */
