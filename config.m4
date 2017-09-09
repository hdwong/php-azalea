dnl $Id$
dnl config.m4 for extension azalea

PHP_ARG_ENABLE(azalea, whether to enable azalea support,
[  --enable-azalea         Enable azalea support], yes)

PHP_ARG_ENABLE(i18n, whether to enable Azalea\I18n support,
[  --enable-i18n           Enable Azalea\I18n support], yes)

PHP_ARG_WITH(service, for Azalea\ServiceModel support,
[  --with-service          Include Azalea\ServiceModel support. It depends on
                          the PHP curl extension.], no, no)

PHP_ARG_WITH(mysqlnd, for Azalea\MysqlndModel support,
[  --with-mysqlnd          Include Azalea\MysqlndModel support. It depends on
                          the PHP mysqlnd extension.], no, no)

PHP_ARG_WITH(sqlbuilder, for Azalea\SqlBuilder support,
[  --with-sqlbuilder       Include Azalea\SqlBuilder support. It depends on
                          the azalea_sqlbuilder extension.], no, no)

if test "$PHP_AZALEA" != "no"; then
    AC_DEFINE([WITH_PINYIN], 1, [Whether pinyin is enabled])

    if test "$PHP_I18N" != "no"; then
        AC_DEFINE([WITH_I18N], 1, [Whether Azalea\I18n is enabled])
    fi
    
    if test "$PHP_SERVICE" != "no"; then
        PHP_ADD_EXTENSION_DEP(azalea, curl, true)
        AC_DEFINE([WITH_SERVICE], 1, [Whether Azalea\ServiceModel is enabled])
    fi
    
    if test "$PHP_MYSQLND" != "no"; then
        PHP_ADD_EXTENSION_DEP(azalea, mysqlnd, true)
        AC_DEFINE([WITH_MYSQLND], 1, [Whether mysqlnd is enabled])
    fi
    
    if test "$PHP_SQLBUILDER" != "no"; then
        PHP_ADD_EXTENSION_DEP(azalea, azalea_sqlbuilder, true)
        AC_DEFINE([WITH_SQLBUILDER], 1, [Whether Azalea\SqlBuilder is enabled])
    fi
    
    PHP_NEW_EXTENSION(azalea, \
        azalea.c \
        azalea/azalea.c \
        azalea/loader.c \
        azalea/bootstrap.c \
        azalea/config.c \
        azalea/controller.c \
        azalea/request.c \
        azalea/response.c \
        azalea/session.c \
        azalea/model.c \
        azalea/view.c \
        azalea/template.c \
        azalea/text.c \
        azalea/i18n.c \
        azalea/exception.c \
        azalea/ext-models/pinyin.c \
        azalea/ext-models/mysqlnd.c \
    , $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
