#ifndef PHP_IMGDEC_H
#define PHP_IMGDEC_H

extern zend_module_entry imgdec_module_entry;
#define phpext_imgdec_ptr &imgdec_module_entry

#define PHP_IMGDEC_VERSION "0.10.0"

#if defined(ZTS) && defined(COMPILE_DL_IMGDEC)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif
