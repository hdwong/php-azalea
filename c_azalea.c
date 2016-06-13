//
//  c_azalea.c
//  
//
//  Created by Bun Wong on 16-6-13.
//
//

#include "php.h"
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

#include "c_azalea.h"

/* {{{ class Azalea
 */
zend_class_entry *azalea_ce_azalea;

static zend_function_entry azalea_methods[] = {
    PHP_ME(Azalea, timer, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Azalea, now, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Azalea, randomString, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Azalea, ipAddress, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    PHP_ME(Azalea, gotoUrl, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

void azalea_init_azalea(TSRMLS_D)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Azalea", azalea_methods);
    azalea_ce_azalea = zend_register_internal_class(&ce TSRMLS_CC);
}

double getMicrotime()
{
    struct timeval tp = {0};
    if (gettimeofday(&tp, NULL)) {
        return 0;
    }
    return (double)(tp.tv_sec + tp.tv_usec / MICRO_IN_SEC);
}

/* {{{ proto double timer(void) */
PHP_METHOD(Azalea, timer)
{
    static double start = 0;
    double m = getMicrotime();
    if (m == 0) {
        RETURN_FALSE;
    }
    double diff = 0;
    if (start) {
       diff = m - start;
    }
    start = m;
    RETURN_DOUBLE(diff);
}
/* }}} */

/* {{{ proto long now(void) */
PHP_METHOD(Azalea, now)
{
    zval *carrier = NULL, *ret;
    carrier = &PG(http_globals)[TRACK_VARS_SERVER];
    if (!carrier ||
            NULL == (ret = zend_hash_str_find(Z_ARRVAL_P(carrier), ZEND_STRL("REQUEST_TIME")))) {
        RETURN_LONG((zend_long)time(NULL));
    }
    RETURN_LONG(Z_LVAL_P(ret));
}
/* }}} */

/* {{{ proto string randomString(long len, string mode) */
PHP_METHOD(Azalea, randomString)
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

/* {{{ proto string ipAddress(void) */
PHP_METHOD(Azalea, ipAddress)
{
    // TODO
}
/* }}} */

/* {{{ proto void gotoUrl(string url) */
PHP_METHOD(Azalea, gotoUrl)
{
    // TODO
}
/* }}} */

/* }}} */