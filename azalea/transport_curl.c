/*
 * transport_curl.c
 *
 * Created by Bun Wong on 16-7-16.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/config.h"
#include "azalea/service.h"
#include "azalea/transport_curl.h"

#include "Zend/zend_smart_str.h"  // for smart_str_*
#include "ext/standard/php_http.h"  // for php_url_encode_hash_ex
#include "ext/standard/url.h"  // for php_url_encode_hash_ex
#include "ext/json/php_json.h"  // for php_json_decode
#include "ext/pcre/php_pcre.h"  // for php_pcre_replace
#include "azalea/php_zlib.h"  // for php_zlib_decode

#include <curl/curl.h>
#include <curl/easy.h>

static smart_str data = {0};

void * azaleaCurlOpen()
{
	CURL *cp = NULL;
	cp = curl_easy_init();
	if (!cp) {
		php_error_docref(NULL, E_ERROR, "start curl failed");
		return NULL;
	}

	return cp;
}

int azaleaCurlClose(void *cp)
{
	if (cp) {
		curl_easy_cleanup(cp);
	}

	return 1;
}


long azaleaCurlExec(void *cp, long method, zend_string **url, zval **arguments, zval *result)
{
	CURLcode ret;
	struct curl_slist *headers = NULL;
	long statusCode = 0, timeout = 15, connectTimeout = 2;
	char *contentType = NULL;
	double downloadLength = 0;
	zval *conf;

	// init headers
	headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
	headers = curl_slist_append(headers, "Connection: Keep-Alive");
	headers = curl_slist_append(headers, "Keep-Alive: 300");
	// token
	if ((conf = azaleaConfigSubFind("service", "token")) && Z_TYPE_P(conf) == IS_STRING && Z_STRLEN_P(conf)) {
		// check url is equal with config.service.url
		zval *serviceUrl = azaleaConfigSubFind("service", "url");
		if (serviceUrl && Z_TYPE_P(serviceUrl) == IS_STRING && Z_STRLEN_P(serviceUrl) &&
				(0 == strncasecmp(ZSTR_VAL(*url), Z_STRVAL_P(serviceUrl), Z_STRLEN_P(serviceUrl)))) {
			zend_string *tstr;
			tstr = strpprintf(0, "token: %s", Z_STRVAL_P(conf));
			headers = curl_slist_append(headers, ZSTR_VAL(tstr));
			zend_string_release(tstr);
		}
	}
	// timeout and connectTimeout
	if ((conf = azaleaConfigSubFind("service", "timeout")) && Z_TYPE_P(conf) == IS_LONG && Z_LVAL_P(conf) > 0) {
		timeout = Z_LVAL_P(conf);
	}
	if ((conf = azaleaConfigSubFind("service", "connecttimeout")) && Z_TYPE_P(conf) == IS_LONG && Z_LVAL_P(conf) > 0) {
		connectTimeout = Z_LVAL_P(conf);
	}
	curl_easy_setopt(cp, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(cp, CURLOPT_NETRC, 0);
	curl_easy_setopt(cp, CURLOPT_HEADER, 0);
	curl_easy_setopt(cp, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(cp, CURLOPT_FOLLOWLOCATION, 0);
	curl_easy_setopt(cp, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(cp, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(cp, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(cp, CURLOPT_DNS_USE_GLOBAL_CACHE, 1);
	curl_easy_setopt(cp, CURLOPT_DNS_CACHE_TIMEOUT, 300);
	curl_easy_setopt(cp, CURLOPT_TCP_NODELAY, 0);
	curl_easy_setopt(cp, CURLOPT_CONNECTTIMEOUT, connectTimeout);
	curl_easy_setopt(cp, CURLOPT_TIMEOUT, timeout);
	// method and query
	if (*arguments && Z_TYPE_P(*arguments) == IS_ARRAY) {
		smart_str formstr = {0};
		if (php_url_encode_hash_ex(Z_ARRVAL_P(*arguments), &formstr, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, PHP_QUERY_RFC1738) != FAILURE) {
			// preg_replace('/%5B[0-9]+%5D/simU', '', http_build_query($params)
			zend_string *regex = zend_string_init(ZEND_STRL("/%5B[0-9]+%5D/simU"), 0);
			zval rep;
			ZVAL_EMPTY_STRING(&rep);
			zend_string *query = php_pcre_replace(regex, formstr.s, ZSTR_VAL(formstr.s), ZSTR_LEN(formstr.s), &rep, 0, -1, NULL);
			zend_string_release(regex);

			if (method == AZALEA_SERVICE_METHOD_GET) {
				// add query
				zend_string *newUrl;
				if (strstr(ZSTR_VAL(*url), "?")) {
					// has query yet
					newUrl = strpprintf(0, "%s&%s", ZSTR_VAL(*url), ZSTR_VAL(query));
				} else {
					newUrl = strpprintf(0, "%s?%s", ZSTR_VAL(*url), ZSTR_VAL(query));
				}
				zend_string_release(*url);
				*url = newUrl;
				zval_ptr_dtor(*arguments);
				*arguments = NULL;
			} else if (method == AZALEA_SERVICE_METHOD_POST || method == AZALEA_SERVICE_METHOD_PUT ||
					method == AZALEA_SERVICE_METHOD_DELETE) {
				// copy from ext/standard/interface.c line 2619
				/* with curl 7.17.0 and later, we can use COPYPOSTFIELDS, but we have to provide size before */
				curl_easy_setopt(cp, CURLOPT_POSTFIELDSIZE, ZSTR_LEN(query));
				curl_easy_setopt(cp, CURLOPT_COPYPOSTFIELDS, ZSTR_VAL(query));
			}
			zend_string_release(query);
		}
		smart_str_free(&formstr);
	}
	if (method == AZALEA_SERVICE_METHOD_GET) {
		curl_easy_setopt(cp, CURLOPT_HTTPGET, 1);
	} else if (method == AZALEA_SERVICE_METHOD_POST) {
		curl_easy_setopt(cp, CURLOPT_POST, 1);
	} else if (method == AZALEA_SERVICE_METHOD_PUT) {
		curl_easy_setopt(cp, CURLOPT_CUSTOMREQUEST, "PUT");
	} else if (method == AZALEA_SERVICE_METHOD_DELETE) {
		curl_easy_setopt(cp, CURLOPT_CUSTOMREQUEST, "DELETE");
	}
	// url and writefunction
	curl_easy_setopt(cp, CURLOPT_URL, ZSTR_VAL(*url));
	curl_easy_setopt(cp, CURLOPT_WRITEFUNCTION, azaleaBufferWriter);
	curl_easy_setopt(cp, CURLOPT_WRITEDATA, &data);

	// exec
	smart_str_alloc(&data, 4096, 0);
	ret = curl_easy_perform(cp);
	smart_str_0(&data);
	curl_slist_free_all(headers);

	if (ret != CURLE_OK) {
		// request is fail
		// TODO log?
		smart_str_free(&data);
		return 0;
	}

	curl_easy_getinfo(cp, CURLINFO_HTTP_CODE, &statusCode);
	curl_easy_getinfo(cp, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadLength);
	curl_easy_getinfo(cp, CURLINFO_CONTENT_TYPE, &contentType);
	//  gzdecode if gzip response
	if ((long) downloadLength == -1) {
		// gzip
		char *buf;
		size_t bufLen;
		if (SUCCESS != php_zlib_decode(ZSTR_VAL(data.s), ZSTR_LEN(data.s), &buf, &bufLen, PHP_ZLIB_ENCODING_GZIP, 0)) {
			smart_str_free(&data);
			return -1;
		}
		zend_string_release(data.s);
		data.s = zend_string_init(buf, bufLen, 0);
		efree(buf);
	}

	if (contentType && (0 == strncasecmp(contentType, ZEND_STRL("application/json")))) {
		php_json_decode(result, ZSTR_VAL(data.s), ZSTR_LEN(data.s), 1, 0);
	} else {
		ZVAL_STR(result, data.s);
	}

	smart_str_free(&data);

	return statusCode;
}

static size_t azaleaBufferWriter(char *p, size_t size, size_t nmemb, void *ctx)
{
	size_t len = size * nmemb;
	smart_str_appendl(&data, p, len);
	return len;
}

// ----- COPY FROM zlib.c -----
/* {{{ Memory management wrappers */
static voidpf php_zlib_alloc(voidpf opaque, uInt items, uInt size)
{
	return (voidpf)safe_emalloc(items, size, 0);
}

static void php_zlib_free(voidpf opaque, voidpf address)
{
	efree((void*)address);
}
/* }}} */

/* {{{ php_zlib_inflate_rounds() */
static inline int php_zlib_inflate_rounds(z_stream *Z, size_t max, char **buf, size_t *len)
{
	int status, round = 0;
	php_zlib_buffer buffer = {NULL, NULL, 0, 0, 0};

	*buf = NULL;
	*len = 0;

	buffer.size = (max && (max < Z->avail_in)) ? max : Z->avail_in;

	do {
		if ((max && (max <= buffer.used)) || !(buffer.aptr = erealloc_recoverable(buffer.data, buffer.size))) {
			status = Z_MEM_ERROR;
		} else {
			buffer.data = buffer.aptr;
			Z->avail_out = buffer.free = buffer.size - buffer.used;
			Z->next_out = (Bytef *) buffer.data + buffer.used;
#if 0
			fprintf(stderr, "\n%3d: %3d PRIOR: size=%7lu,\tfree=%7lu,\tused=%7lu,\tavail_in=%7lu,\tavail_out=%7lu\n", round, status, buffer.size, buffer.free, buffer.used, Z->avail_in, Z->avail_out);
#endif
			status = inflate(Z, Z_NO_FLUSH);

			buffer.used += buffer.free - Z->avail_out;
			buffer.free = Z->avail_out;
#if 0
			fprintf(stderr, "%3d: %3d AFTER: size=%7lu,\tfree=%7lu,\tused=%7lu,\tavail_in=%7lu,\tavail_out=%7lu\n", round, status, buffer.size, buffer.free, buffer.used, Z->avail_in, Z->avail_out);
#endif
			buffer.size += (buffer.size >> 3) + 1;
		}
	} while ((Z_BUF_ERROR == status || (Z_OK == status && Z->avail_in)) && ++round < 100);

	if (status == Z_STREAM_END) {
		buffer.data = erealloc(buffer.data, buffer.used + 1);
		buffer.data[buffer.used] = '\0';
		*buf = buffer.data;
		*len = buffer.used;
	} else {
		if (buffer.data) {
			efree(buffer.data);
		}
		/* HACK: See zlib/examples/zpipe.c inf() function for explanation. */
		/* This works as long as this function is not used for streaming. Required to catch very short invalid data. */
		status = (status == Z_OK) ? Z_DATA_ERROR : status;
	}
	return status;
}
/* }}} */

/* {{{ php_zlib_decode() */
static int php_zlib_decode(const char *in_buf, size_t in_len, char **out_buf, size_t *out_len, int encoding, size_t max_len)
{
	int status = Z_DATA_ERROR;
	z_stream Z;

	memset(&Z, 0, sizeof(z_stream));
	Z.zalloc = php_zlib_alloc;
	Z.zfree = php_zlib_free;

	if (in_len) {
retry_raw_inflate:
		status = inflateInit2(&Z, encoding);
		if (Z_OK == status) {
			Z.next_in = (Bytef *) in_buf;
			Z.avail_in = in_len + 1; /* NOTE: data must be zero terminated */

			switch (status = php_zlib_inflate_rounds(&Z, max_len, out_buf, out_len)) {
				case Z_STREAM_END:
					inflateEnd(&Z);
					return SUCCESS;

				case Z_DATA_ERROR:
					/* raw deflated data? */
					if (PHP_ZLIB_ENCODING_ANY == encoding) {
						inflateEnd(&Z);
						encoding = PHP_ZLIB_ENCODING_RAW;
						goto retry_raw_inflate;
					}
			}
			inflateEnd(&Z);
		}
	}

	*out_buf = NULL;
	*out_len = 0;

	php_error_docref(NULL, E_WARNING, "%s", zError(status));
	return FAILURE;
}
/* }}} */
