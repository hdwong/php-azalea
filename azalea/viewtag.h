/*
 * azalea/viewtag.h
 *
 * Created by Bun Wong on 19-9-12.
 */

#ifndef AZALEA_VIEWTAG_H_
#define AZALEA_VIEWTAG_H_

AZALEA_STARTUP_FUNCTION(viewtag);

PHP_FUNCTION(azalea_viewtag_js);
PHP_FUNCTION(azalea_viewtag_css);

void azaleaViewtagCallFunction(INTERNAL_FUNCTION_PARAMETERS);

#endif /* AZALEA_VIEWTAG_H_ */
