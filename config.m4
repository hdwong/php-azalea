dnl $Id$
dnl config.m4 for extension azalea

PHP_ARG_ENABLE(azalea, whether to enable azalea support,
[  --enable-azalea           Enable azalea support])

if test "$PHP_AZALEA" != "no"; then
  PHP_NEW_EXTENSION(azalea, azalea.c c_azalea.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
