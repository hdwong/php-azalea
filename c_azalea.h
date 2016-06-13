//
//  azalea.h
//  
//
//  Created by Bun Wong on 16-6-13.
//
//

#ifndef AZALEA_H
#define AZALEA_H

void azalea_init_azalea(TSRMLS_D);

PHP_METHOD(Azalea, timer);
PHP_METHOD(Azalea, now);
PHP_METHOD(Azalea, randomString);
PHP_METHOD(Azalea, ipAddress);
PHP_METHOD(Azalea, gotoUrl);

#endif
