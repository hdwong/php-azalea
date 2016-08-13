/*
 * transport_curl.h
 *
 * Created by Bun Wong on 16-7-16.
 */

#ifndef AZALEA_TRANSPORT_CURL_H_
#define AZALEA_TRANSPORT_CURL_H_

static size_t azaleaBufferWriter(char *, size_t, size_t, void *);
static int php_zlib_decode(const char *in_buf, size_t in_len, char **out_buf, size_t *out_len, int encoding, size_t max_len);

void * azaleaCurlOpen();
zend_long azaleaCurlExec(void *cp, zend_long method, zend_string **serviceUrl, zval **arguments, zval *headers, zval *result);
int azaleaCurlClose(void *cp);

#endif /* AZALEA_TRANSPORT_CURL_H_ */
