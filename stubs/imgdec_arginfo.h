/* Written by hand from imgdec.stub.php. Do not regenerate it. */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_info, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, bytes, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_decode_into, 0, 5, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, bytes, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, capacity, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, stride, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, order, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, frame, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_FUNCTION(imgdec_info);
ZEND_FUNCTION(imgdec_decode_into);

#ifdef IMGDEC_PROBES
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_probe_alloc, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, size, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_probe_read, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, size, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_imgdec_probe_free, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_FUNCTION(imgdec_probe_alloc);
ZEND_FUNCTION(imgdec_probe_read);
ZEND_FUNCTION(imgdec_probe_free);
#endif

static const zend_function_entry ext_functions[] = {
	ZEND_FE(imgdec_info, arginfo_imgdec_info)
	ZEND_FE(imgdec_decode_into, arginfo_imgdec_decode_into)
#ifdef IMGDEC_PROBES
	ZEND_FE(imgdec_probe_alloc, arginfo_imgdec_probe_alloc)
	ZEND_FE(imgdec_probe_read, arginfo_imgdec_probe_read)
	ZEND_FE(imgdec_probe_free, arginfo_imgdec_probe_free)
#endif
	ZEND_FE_END
};

static void register_imgdec_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("IMGDEC_MAX_SIDE", IMGDEC_MAX_SIDE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_MAX_PIXELS", IMGDEC_MAX_PIXELS, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_CORRUPT", IMGDEC_CORRUPT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_UNSUPPORTED", IMGDEC_UNSUPPORTED, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_TOO_LARGE", IMGDEC_TOO_LARGE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_RGBA", IMGDEC_RGBA, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IMGDEC_BGRA", IMGDEC_BGRA, CONST_PERSISTENT);
#ifdef IMGDEC_PROBES
	REGISTER_BOOL_CONSTANT("IMGDEC_PROBES", true, CONST_PERSISTENT);
#endif
}

static zend_class_entry *register_class_ImgdecException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "ImgdecException", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_Exception, 0);

	return class_entry;
}
