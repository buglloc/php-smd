PHP_ARG_ENABLE(smd, whether to enable smd support,
[  --enable-smd   Enable smd support])

PHP_ARG_WITH(smd-sanitize, whether to enable AddressSanitizer for smd,
[  --with-smd-sanitize Build smd with AddressSanitizer support], no, no)

if test "$PHP_SMD" != "no"; then
  if test "$PHP_SMD_SANITIZE" != "no"; then
    EXTRA_LDFLAGS="-lasan"
  EXTRA_CFLAGS="-fsanitize=address -fno-omit-frame-pointer"
  PHP_SUBST(EXTRA_LDFLAGS)
    PHP_SUBST(EXTRA_CFLAGS)
  fi

  PHP_NEW_EXTENSION(smd, smd.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi