/*
 * azalea/view.h
 *
 * Created by Bun Wong on 16-6-28.
 */

#ifndef AZALEA_VIEW_H_
#define AZALEA_VIEW_H_

AZALEA_STARTUP_FUNCTION(view);

PHP_METHOD(azalea_view, __construct);
PHP_METHOD(azalea_view, render);
PHP_METHOD(azalea_view, assign);
PHP_METHOD(azalea_view, append);

extern zend_class_entry *azalea_view_ce;

#endif /* AZALEA_VIEW_H_ */
