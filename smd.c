/*
  +----------------------------------------------------------------------+
  | Author: Andrew Krasichkov buglloc@yandex.ru                          |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "SAPI.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_smd.h"


ZEND_DECLARE_MODULE_GLOBALS(smd)


/* {{{ arginfo */

ZEND_BEGIN_ARG_INFO_EX(arginfo_smd_enable, 0, 0, 0)
    ZEND_ARG_INFO(0, enable)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smd_enabled, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smd_pump, 0, 0, 0)
ZEND_END_ARG_INFO()

/* }}} */

/* {{{ smd_functions[]
 *
 * Every user visible function must have an entry in smd_functions[].
 */
static const zend_function_entry smd_functions[] = {
    PHP_FE(smd_enable,          arginfo_smd_enable)
    PHP_FE(smd_enabled,         arginfo_smd_enabled)
    PHP_FE(smd_pump,            arginfo_smd_pump)
    PHP_FE_END
};
/* }}} */


/* {{{ smd_module_entry
 */
zend_module_entry smd_module_entry = {
    STANDARD_MODULE_HEADER,
    "smd",
    smd_functions,
    PHP_MINIT(smd), /* PHP_MINIT */
    NULL, /* PHP_MSHUTDOWN */
    PHP_RINIT(smd), /* PHP_RINIT */
    PHP_RSHUTDOWN(smd), /* PHP_RSHUTDOWN */
    PHP_MINFO(smd),
    SMD_VERSION,
    PHP_MODULE_GLOBALS(smd),
    NULL, /* PHP_GINIT ss*/
    NULL, /* PHP_GSHUTDOWN */
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_SMD
ZEND_GET_MODULE(smd)
#endif

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("smd.enable", "1", PHP_INI_ALL, OnUpdateBool, enable, zend_smd_globals, smd_globals)
PHP_INI_END()
/* }}} */

static struct smd_overridden_fucs /* {{{ */ {
    php_func array_merge;
} smd_origin_funcs;

#define SMD_TO_FUNC(m) (smd_origin_funcs.m)

static zend_uchar smd_get_type(const HashTable *ht) /* {{{ */ {
    zend_uchar ret = 0;
    zend_uchar mark = SMD_GET_MARK(ht);

    if (mark & _GET_MARK)
        ret = _GET;
    else if (mark & _POST_MARK)
        ret = _POST;
    else if (mark & _REQUEST_MARK)
        ret = _REQUEST;
    else if (mark & _COOKIE_MARK)
        ret = _COOKIE;

    return ret;
} /* }}} */

/* Ported from  Zend/zend_execute.c */

#ifndef CV_DEF_OF
#define CV_DEF_OF(i) (EX(func)->op_array.vars[i])
#endif

static zval *smd_get_zval_ptr_tmpvar(zend_execute_data *execute_data, uint32_t var, zend_free_op *should_free) /* {{{ */ {
    zval *ret = EX_VAR(var);

    if (should_free) {
        *should_free = ret;
    }
    ZVAL_DEREF(ret);

    return ret;
} /* }}} */

static zval *smd_get_zval_ptr_cv(zend_execute_data *execute_data, uint32_t var, int type, int force_ret) /* {{{ */ {
    zval *ret = EX_VAR(var);

    if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
        if (force_ret) {
            switch (type) {
                case BP_VAR_R:
                case BP_VAR_UNSET:
                    zend_error(E_NOTICE, "Undefined variable: %s", ZSTR_VAL(CV_DEF_OF(EX_VAR_TO_NUM(var))));
                case BP_VAR_IS:
                    ret = &EG(uninitialized_zval);
                    break;
                case BP_VAR_RW:
                    zend_error(E_NOTICE, "Undefined variable: %s", ZSTR_VAL(CV_DEF_OF(EX_VAR_TO_NUM(var))));
                case BP_VAR_W:
                    ZVAL_NULL(ret);
                    break;
            }
        } else {
            return NULL;
        }
    } else {
        ZVAL_DEREF(ret);
    }
    return ret;
} /* }}} */

static zval *smd_get_zval_ptr(zend_execute_data *execute_data, int op_type, znode_op op, zend_free_op *should_free, int type, int force_ret) /* {{{ */ {
    if (op_type & (IS_TMP_VAR|IS_VAR)) {
        return smd_get_zval_ptr_tmpvar(execute_data, op.var, should_free);
    } else {
        if (op_type == IS_CONST) {
            return EX_CONSTANT(op);
        } else if (op_type == IS_CV) {
            return smd_get_zval_ptr_cv(execute_data, op.var, type, force_ret);
        } else {
            return NULL;
        }
    }
} /* }}} */ 

/* End ported code */

static int smd_fetch_dim_handler(zend_execute_data *execute_data) /* {{{ */ {
    const zend_op *opline = execute_data->opline;
    zend_free_op free_op1, free_op2;
    zval *container, *key;
    zval string_key;
    zend_uchar mark;

    if (!(OP1_TYPE(opline) & (IS_VAR|IS_CV)))
        return ZEND_USER_OPCODE_DISPATCH;

    if (!(OP2_TYPE(opline) & (IS_VAR|IS_CV|IS_CONST)))
        return ZEND_USER_OPCODE_DISPATCH;

    container = smd_get_zval_ptr(execute_data, opline->op1_type, opline->op1, &free_op1, BP_VAR_R, 1);
    key = smd_get_zval_ptr(execute_data, opline->op2_type, opline->op2, &free_op2, BP_VAR_R, 1);

    if (
        container
        && IS_ARRAY == Z_TYPE_P(container)
        && SMD_CHECK_MARK(Z_ARRVAL_P(container))

        && key
        && IS_STRING == Z_TYPE_P(key)
        && Z_STRLEN_P(key)
        ) {
            ZVAL_STR_COPY(&string_key, Z_STR_P(key));
            if (Z_REFCOUNTED(string_key))
                Z_ADDREF(string_key);
            mark = smd_get_type(Z_ARRVAL_P(container));
            zend_hash_next_index_insert_new(&(SMD_G(superglobals)[mark]), &string_key);
    }

end:
    return ZEND_USER_OPCODE_DISPATCH; 
} /* }}} */

static void smd_mark_global(char *name, int len, zend_uchar mark) /* {{{ */ {
    zend_string * str = zend_string_init(name, len, 0);
    if (zend_is_auto_global(str)) {
        zval *zv = zend_hash_find(&EG(symbol_table), str);
        SMD_SET_MARK(Z_ARRVAL(*zv), mark);
    }

    efree(str);
} /* }}} */

static void smd_override_func(const char *name, php_func handler, php_func *stash) /* {{{ */ {
    zend_function *func;
    if ((func = zend_hash_str_find_ptr(CG(function_table), name, strlen(name))) != NULL) {
        if (stash) {
            *stash = func->internal_function.handler;
        }
        func->internal_function.handler = handler;
    }
} /* }}} */

static void smd_override_functions() /* {{{ */ {
    const char *f_array_merge = "array_merge";

    smd_override_func(f_array_merge, PHP_FN(smd_array_merge), &SMD_TO_FUNC(array_merge));

} /* }}} */

static void smd_register_handlers() /* {{{ */ {
    zend_set_user_opcode_handler(ZEND_ISSET_ISEMPTY_DIM_OBJ, smd_fetch_dim_handler);
    zend_set_user_opcode_handler(ZEND_FETCH_DIM_R, smd_fetch_dim_handler);
} /* }}} */

/* {{{ proto array array_merge(array arr1, array arr2 [, array ...])
   Merges elements from passed arrays into one array */
PHP_FUNCTION(smd_array_merge) {
    zval *args = NULL;
    int argc, i, mark = 0;

#ifndef FAST_ZPP
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "+", &args, &argc) == FAILURE) {
        return;
    }
#else
    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_VARIADIC('+', args, argc)
    ZEND_PARSE_PARAMETERS_END();
#endif

    for (i = 0; i < argc; i++) {
        zval *arg = args + i;

        ZVAL_DEREF(arg);
        if (Z_TYPE_P(arg) == IS_ARRAY && SMD_CHECK_MARK(Z_ARRVAL_P(arg))) {
            mark |= SMD_GET_MARK(Z_ARRVAL_P(arg));
        }
    }

    SMD_TO_FUNC(array_merge)(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    if (mark && IS_ARRAY == Z_TYPE_P(return_value)) {
        SMD_SET_MARK(Z_ARRVAL_P(return_value), mark);
    }
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(smd)
{
    REGISTER_INI_ENTRIES();

    if (!SMD_G(enable)) {
        return SUCCESS;
    }

    smd_override_functions();
    smd_register_handlers();

    return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(smd)
{
    if (SG(sapi_started) || !SMD_G(enable)) {
        return SUCCESS;
    }

    for (int i = 0; i < _SUPERGLOBALS_COUNT; ++i) {
        zend_hash_init(&SMD_G(superglobals)[i], 100, NULL, NULL, 1);
    }

    smd_mark_global(ZEND_STRL("_GET"), _GET_MARK);
    smd_mark_global(ZEND_STRL("_POST"), _POST_MARK);
    smd_mark_global(ZEND_STRL("_REQUEST"), _REQUEST_MARK);
    smd_mark_global(ZEND_STRL("_COOKIE"), _COOKIE_MARK);

    return SUCCESS;
} /* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(smd)
{
    if (!SMD_G(enable)) {
        return SUCCESS;
    }

    for (int i = 0; i < _SUPERGLOBALS_COUNT; ++i) {
        zend_hash_destroy(&SMD_G(superglobals)[i]);
    }

    return SUCCESS;
} /* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(smd)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "Show Me Dependency enabled", SMD_G(enable) ? "enabled": "disabled");
    php_info_print_table_row(2, "Version", SMD_VERSION);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
} /* }}} */


/* {{{ proto string smd_inited()
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(smd_enable)
{
    zend_bool enable = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &enable) == FAILURE) {
        RETURN_NULL();
    }

    SMD_G(enable) = enable;
    if (enable)
        RETURN_TRUE;
    RETURN_FALSE;
} /* }}} */

/* {{{ proto string smd_inited()
*/
PHP_FUNCTION(smd_enabled)
{
    if (SMD_G(enable))
        RETURN_TRUE;
    RETURN_FALSE;
} /* }}} */

/* {{{ proto array smd_pump()
 */
PHP_FUNCTION(smd_pump)
{
#define PUMP_KEYS(dst, type) \
    array_init(&dst); \
    zend_hash_copy(Z_ARRVAL(dst), \
        &SMD_G(superglobals)[type], (copy_ctor_func_t) zval_add_ref); \
    zend_hash_clean(&SMD_G(superglobals)[type]);

    zval get, post, request, cookie;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    array_init(return_value);

    PUMP_KEYS(get, _GET);
    PUMP_KEYS(post, _POST);
    PUMP_KEYS(request, _REQUEST);
    PUMP_KEYS(cookie, _COOKIE);

    zend_hash_str_add_new(Z_ARRVAL_P(return_value), "get", sizeof("get")-1, &get);
    zend_hash_str_add_new(Z_ARRVAL_P(return_value), "post", sizeof("post")-1, &post);
    zend_hash_str_add_new(Z_ARRVAL_P(return_value), "request", sizeof("request")-1, &request);
    zend_hash_str_add_new(Z_ARRVAL_P(return_value), "cookie", sizeof("cookie")-1, &cookie);
} /* }}} */
