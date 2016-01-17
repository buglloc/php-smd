/*
  +----------------------------------------------------------------------+
  | Author: Andrew Krasichkov buglloc@yandex.ru                          |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_SMD_H
#define PHP_SMD_H

extern zend_module_entry smd_module_entry;
#define phpext_smd_ptr &smd_module_entry

#ifdef PHP_WIN32
#   define PHP_TEST_HELPERS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_TEST_HELPERS_API __attribute__ ((visibility("default")))
#else
#   define PHP_TEST_HELPERS_API
#endif

ZEND_BEGIN_MODULE_GLOBALS(smd)
    zend_bool enable;
    zend_bool autostart;
    HashTable superglobals[4];
ZEND_END_MODULE_GLOBALS(smd)

#ifdef ZTS
#define SMD_G(v) TSRMG(smd_globals_id, zend_smd_globals *, v)
#else
#define SMD_G(v) (smd_globals.v)
#endif

#define SMD_VERSION "0.1"
#define SMD_FULL_NAME "Show Me Dependency"

#define _GET     0
#define _POST    1
#define _REQUEST 2
#define _COOKIE  3

#define _SUPERGLOBALS_COUNT 4

/* Make sure this value is not used by Zend or any other extension!
   See aray flags in Zend/zend_types.h */
#define GC_FLAGS_PADDING  4
#define SMD_TYPE_TO_MARK(type) (1 << (GC_FLAGS_PADDING + type))

#define _GET_MARK        SMD_TYPE_TO_MARK(_GET)
#define _POST_MARK       SMD_TYPE_TO_MARK(_POST)
#define _REQUEST_MARK    SMD_TYPE_TO_MARK(_REQUEST)
#define _COOKIE_MARK     SMD_TYPE_TO_MARK(_COOKIE)

#define SMD_SET_MARK(array, mark)	\
	(GC_FLAGS((array)) |= mark)
#define SMD_GET_MARK(array) \
	( \
		GC_FLAGS((array)) & ( \
			_GET_MARK|_POST_MARK|_REQUEST_MARK|_COOKIE_MARK \
		) \
	)
#define SMD_CHECK_MARK(array) \
	( SMD_GET_MARK(array) != 0 )

#define OP1_TYPE(opline)	(opline->op1_type)
#define OP2_TYPE(opline)	(opline->op2_type)

typedef void (*php_func)(INTERNAL_FUNCTION_PARAMETERS);

/* Module handlers */
PHP_MINIT_FUNCTION(smd);
PHP_RINIT_FUNCTION(smd);
PHP_RSHUTDOWN_FUNCTION(smd);
PHP_MINFO_FUNCTION(smd);
PHP_GINIT_FUNCTION(smd);

/* Userspace functions */
PHP_FUNCTION(smd_enable);
PHP_FUNCTION(smd_enabled);
PHP_FUNCTION(smd_pump);

/* Overrided functions */
PHP_FUNCTION(smd_array_merge);

#endif  /* PHP_SMD_H */
