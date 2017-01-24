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
#include "ext/standard/php_rand.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#elif defined(NETWARE)
#include <sys/timeval.h>
#include <sys/time.h>
#else
#include <sys/time.h>
#endif

#include "ext/standard/php_var.h"	// for php_var_dump function
#include "ext/standard/php_string.h"  // for php_trim function
#include "main/SAPI.h"  // for sapi_header_op

/* {{{ azalea_functions[] */
const zend_function_entry azalea_functions[] = {
	ZEND_NS_NAMED_FE(AZALEA_NS, timer, ZEND_FN(azalea_timer), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, url, ZEND_FN(azalea_url), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, env, ZEND_FN(azalea_env), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, ip, ZEND_FN(azalea_ip), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, randomString, ZEND_FN(azalea_randomString), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, maskString, ZEND_FN(azalea_maskString), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, debug, ZEND_FN(azalea_debug), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, isMobile, ZEND_FN(azalea_isMobile), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, isWechat, ZEND_FN(azalea_isWechat), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, isQq, ZEND_FN(azalea_isQq), NULL)
	ZEND_NS_NAMED_FE(AZALEA_NS, isIos, ZEND_FN(azalea_isIos), NULL)
	PHP_FE_END	/* Must be the last line in azalea_functions[] */
};
/* }}} */

/* {{{ proto azalea_randomString */
PHP_FUNCTION(azalea_randomString)
{
	zend_long len;
	zend_string *mode = NULL;

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
		if (strncmp(ZSTR_VAL(mode), "10", 2) == 0 || strncasecmp(ZSTR_VAL(mode), "n", 1) == 0) {
			// [0-9]
			l = 10;
		} else if (strncmp(ZSTR_VAL(mode), "16", 2) == 0) {
			// [0-9a-f]
			l = 16;
		} else if (strncasecmp(ZSTR_VAL(mode), "c", 1) == 0) {
			// [a-zA-Z]
			p += 10;
			l = 52;
		} else if (strncasecmp(ZSTR_VAL(mode), "ln", 2) == 0) {
			// [0-9a-z]
			l = 36;
		} else if (strncasecmp(ZSTR_VAL(mode), "un", 2) == 0) {
			// [0-9A-Z]
			p += 36;
			l = 36;
		} else if (strncasecmp(ZSTR_VAL(mode), "l", 1) == 0) {
			// [a-z]
			p += 10;
			l = 26;
		} else if (strncasecmp(ZSTR_VAL(mode), "u", 1) == 0) {
			// [A-Z]
			p += 36;
			l = 26;
		}
	}
	char result[len];
	zend_long i, number;
	l -= 1; // for RAND_RANGE
	if (!BG(mt_rand_is_seeded)) {
		php_mt_srand(GENERATE_SEED());
	}
	for (i = 0; i < len; ++i) {
		number = (zend_long) php_mt_rand() >> 1;
		RAND_RANGE(number, 0, l, PHP_MT_RAND_MAX);
		result[i] = *(p + number);
	}
	RETURN_STRINGL(result, len);
}
/* }}} */

/* {{{ proto azalea_maskString */
PHP_FUNCTION(azalea_maskString)
{
	zend_string *string;
	zend_bool isEmail = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &string, &isEmail) == FAILURE) {
		return;
	}
	// copy for change
	string = zend_string_init(ZSTR_VAL(string), ZSTR_LEN(string), 0);
	if (ZSTR_LEN(string) > 1) {
		if (!isEmail) {
			// normal string
		} else {
			// email
		}
	}
	RETURN_STR(string);
}
/* }}} */

/* {{{ proto azaleaUrl */
zend_string * azaleaUrl(zend_string *url, zend_bool includeHost)
{
	// init AZALEA_G(host)
	if (!AZALEA_G(host)) {
		zval *server, *field;
		zend_string *hostname, *tstr;

		server = &PG(http_globals)[TRACK_VARS_SERVER];
		if (server && Z_TYPE_P(server) == IS_ARRAY &&
				zend_hash_str_exists(Z_ARRVAL_P(server), ZEND_STRL("HTTPS"))) {
			hostname = zend_string_init(ZEND_STRL("https://"), 0);
		} else {
			hostname = zend_string_init(ZEND_STRL("http://"), 0);
		}
		field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_HOST"));
		if (!field) {
			// host not found, try to get from config
			field = azaleaConfigFind("hostname");
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

	zend_string *returnUrl;
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
	zend_bool includeHost = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|b", &url, &includeHost) == FAILURE) {
		return;
	}

	RETURN_STR(azaleaUrl(url, includeHost));
}
/* }}} */

/* {{{ azalea_env */
PHP_FUNCTION(azalea_env)
{
	RETURN_STR(zend_string_copy(AZALEA_G(environ)));
}
/* }}} */

/* {{{ proto azaleaRequestIp */
zend_string * azaleaRequestIp()
{
	if (AZALEA_G(ip)) {
		return AZALEA_G(ip);
	}
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
	if (!ip) {
		ip = zend_string_init(ZEND_STRL("0.0.0.0"), 0);
	}
	AZALEA_G(ip) = ip;
	return ip;
}
/* }}} */

/* {{{ azalea_ip */
PHP_FUNCTION(azalea_ip)
{
	RETURN_STR_COPY(azaleaRequestIp());
}
/* }}} */

/* {{{ proto azaleaDebugMode */
zend_bool azaleaDebugMode()
{
	zval *configDebug, *configDebugKey, *debugKeyField;

	configDebug = azaleaConfigFind("debug");
	configDebugKey = azaleaConfigFind("debug_key");
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
	php_var_dump(var, 0);
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

/* {{{ proto azaleaSetHeaderStr */
int azaleaSetHeaderStr(char *line, size_t len)
{
	sapi_header_line ctr = {0};
	ctr.line = line;
	ctr.line_len = len;
	return sapi_header_op(SAPI_HEADER_REPLACE, &ctr) == SUCCESS;
}
/* }}} */

/* {{{ proto azaleaSetHeaderStrWithCode */
int azaleaSetHeaderStrWithCode(char *line, size_t len, zend_long httpCode)
{
	sapi_header_line ctr = {0};
	ctr.line = line;
	ctr.line_len = len;
	ctr.response_code = httpCode;
	return sapi_header_op(SAPI_HEADER_REPLACE, &ctr) == SUCCESS;
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

/* {{{ azalea_isMobile */
PHP_FUNCTION(azalea_isMobile)
{
	zval *server, *field;
	zend_string *tstr;

	server = &PG(http_globals)[TRACK_VARS_SERVER];
	if (!server || Z_TYPE_P(server) != IS_ARRAY) {
		RETURN_FALSE;
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_USER_AGENT")))) {
		static const char * uastrs[] = {
			"nokia", "sony", "ericsson", "mot",
			"samsung", "htc", "sgh", "lg", "sharp", "sie-",
			"philips", "panasonic", "alcatel", "lenovo", "iphone",
			"ipod", "ipad", "blackberry", "meizu", "android",
			"netfront", "symbian", "ucweb", "windowsce", "palm",
			"operamini", "operamobi", "openwave", "nexusone", "cldc",
			"midp", "wap", "mobile", "micromessenger", "qqbrowser"
		};
		int i;
		tstr = php_string_tolower(Z_STR_P(field));
		for (i = (sizeof(uastrs) / sizeof(uastrs[0]) - 1); i >= 0; i--) {
			if (php_memnstr(ZSTR_VAL(tstr), uastrs[i], strlen(uastrs[i]), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
				zend_string_release(tstr);
				RETURN_TRUE;
			}
		}
		zend_string_release(tstr);
	}
	if (zend_hash_str_exists(Z_ARRVAL_P(server), ZEND_STRL("HTTP_X_WAP_PROFILE"))) {
		RETURN_TRUE;
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_VIA")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		// find "wap"
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("wap"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	if ((field = zend_hash_str_find(Z_ARRVAL_P(server), ZEND_STRL("HTTP_ACCEPT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		// find "vnd.wap.wml"
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("vnd.wap.wml"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ azalea_isMobile */
PHP_FUNCTION(azalea_isWechat)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("micromessenger"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ azalea_isQq */
PHP_FUNCTION(azalea_isQq)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("qqbrowser"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ azalea_isIos */
PHP_FUNCTION(azalea_isIos)
{
	zval *field;
	zend_string *tstr;

	if ((field = azaleaGlobalsStrFind(TRACK_VARS_SERVER, ZEND_STRL("HTTP_USER_AGENT")))) {
		tstr = php_string_tolower(Z_STR_P(field));
		if (php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("iphone"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr)) ||
				php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("ipod"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr)) ||
				php_memnstr(ZSTR_VAL(tstr), ZEND_STRL("ipad"), ZSTR_VAL(tstr) + ZSTR_LEN(tstr))) {
			zend_string_release(tstr);
			RETURN_TRUE;
		}
		zend_string_release(tstr);
	}
	RETURN_FALSE;
}
/* }}} */
