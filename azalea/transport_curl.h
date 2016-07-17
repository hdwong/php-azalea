/*
 * transport_curl.h
 *
 * Created by Bun Wong on 16-7-16.
 */

#ifndef AZALEA_TRANSPORT_CURL_H_
#define AZALEA_TRANSPORT_CURL_H_

static size_t azaleaBufferWriter(char *, size_t, size_t, void *);

void * azaleaCurlOpen();
int azaleaCurlClose(void *cp);
long azaleaCurlExec(void *cp, long method, zend_string **serviceUrl, zval **arguments, zval *result);

#endif /* AZALEA_TRANSPORT_CURL_H_ */
