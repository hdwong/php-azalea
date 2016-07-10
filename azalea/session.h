/*
 * azalea/session.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_SESSION_H_
#define AZALEA_SESSION_H_

AZALEA_STARTUP_FUNCTION(session);

PHP_METHOD(azalea_session, get);
PHP_METHOD(azalea_session, set);
PHP_METHOD(azalea_session, clean);

extern zend_class_entry *azalea_session_ce;

#endif /* AZALEA_SESSION_H_ */
