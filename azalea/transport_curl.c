/*
 * transport_curl.c
 *
 * Created by Bun Wong on 16-7-16.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/service.h"
#include "azalea/transport_curl.h"

#include "Zend/zend_smart_str.h"  // for smart_str_*
#include "ext/standard/php_http.h"  // for php_url_encode_hash_ex
#include "ext/standard/url.h"  // for php_url_encode_hash_ex
#include "ext/json/php_json.h"  // for php_json_decode
#include "ext/pcre/php_pcre.h"  // for php_pcre_replace

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
	long statusCode = 0;
	char *contentType = NULL;

	// init headers
	//	headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
	headers = curl_slist_append(headers, "Connection: Keep-Alive");
	headers = curl_slist_append(headers, "Keep-Alive: 300");
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
	curl_easy_setopt(cp, CURLOPT_CONNECTTIMEOUT, 1);
	curl_easy_setopt(cp, CURLOPT_TIMEOUT, 10);
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
				array_init(*arguments);
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
		curl_easy_setopt(cp, CURLOPT_PUT, 1);
	} else if (method == AZALEA_SERVICE_METHOD_DELETE) {
		curl_easy_setopt(cp, CURLOPT_CUSTOMREQUEST, "DELETE");
	}
	// url and writefunction
	curl_easy_setopt(cp, CURLOPT_URL, ZSTR_VAL(*url));
	curl_easy_setopt(cp, CURLOPT_WRITEFUNCTION, azaleaBufferWriter);
	curl_easy_setopt(cp, CURLOPT_WRITEDATA, &data);

	smart_str_alloc(&data, 4096, 0);
	ret = curl_easy_perform(cp);
	smart_str_0(&data);

	// TODO gzdecode if gzip response

	curl_easy_getinfo(cp, CURLINFO_HTTP_CODE, &statusCode);
	curl_easy_getinfo(cp, CURLINFO_CONTENT_TYPE, &contentType);
	if (contentType && (0 == strncasecmp(contentType, ZEND_STRL("application/json")))) {
		php_json_decode(result, ZSTR_VAL(data.s), ZSTR_LEN(data.s), 1, 0);
	} else {
		ZVAL_STR(result, data.s);
	}

	smart_str_free(&data);
	curl_slist_free_all(headers);

	return statusCode;
}

static size_t azaleaBufferWriter(char *p, size_t size, size_t nmemb, void *ctx)
{
	size_t len = size * nmemb;
	smart_str_appendl(&data, p, len);
	return len;
}
