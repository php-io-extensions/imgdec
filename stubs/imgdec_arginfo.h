/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b97ecfb7ef389ef2bfca5d0258d631b9ac7f6e2a */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_png, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_imgdec_jpeg arginfo_imgdec_png

#define arginfo_imgdec_tiff arginfo_imgdec_png

ZEND_FUNCTION(imgdec_png);
ZEND_FUNCTION(imgdec_jpeg);
ZEND_FUNCTION(imgdec_tiff);

static const zend_function_entry ext_functions[] = {
	ZEND_FE(imgdec_png, arginfo_imgdec_png)
	ZEND_FE(imgdec_jpeg, arginfo_imgdec_jpeg)
	ZEND_FE(imgdec_tiff, arginfo_imgdec_tiff)
	ZEND_FE_END
};

static void register_imgdec_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("IMGDEC_MAX_SIDE", IMGDEC_MAX_SIDE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_MAX_PIXELS", IMGDEC_MAX_PIXELS, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_CORRUPT", IMGDEC_CORRUPT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_UNSUPPORTED", IMGDEC_UNSUPPORTED, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_TOO_LARGE", IMGDEC_TOO_LARGE, CONST_PERSISTENT);
}

static zend_class_entry *register_class_ImgdecException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "ImgdecException", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_Exception, 0);

	return class_entry;
}
