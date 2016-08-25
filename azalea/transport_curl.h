/*
 * transport_curl.h
 *
 * Created by Bun Wong on 16-7-16.
 */

#ifndef AZALEA_TRANSPORT_CURL_H_
#define AZALEA_TRANSPORT_CURL_H_

void * azaleaCurlOpen();
zend_long azaleaCurlExec(void *cp, zend_long method, zend_string **serviceUrl, zval **arguments, zval *headers, zval *result);
int azaleaCurlClose(void *cp);

#endif /* AZALEA_TRANSPORT_CURL_H_ */
