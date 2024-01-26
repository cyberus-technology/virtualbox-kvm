#
# configure.in
#
# See the LICENSE file for the license associated with this file.

AC_INIT([libtpms],[0.9.6])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR(Makefile.am)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign 1.6 subdir-objects])
AM_SILENT_RULES([yes])

LIBTPMS_VER_MAJOR=`echo $PACKAGE_VERSION | awk -F. '{print $1}'`
LIBTPMS_VER_MINOR=`echo $PACKAGE_VERSION | awk -F. '{print $2}'`
LIBTPMS_VER_MICRO=`echo $PACKAGE_VERSION | awk -F. '{print $3}'`
LIBTPMS_VERSION=$PACKAGE_VERSION
LIBTPMS_VERSION_INFO=`expr $LIBTPMS_VER_MAJOR + $LIBTPMS_VER_MINOR`:$LIBTPMS_VER_MICRO:$LIBTPMS_VER_MINOR

AC_SUBST([LIBTPMS_VER_MAJOR])
AC_SUBST([LIBTPMS_VER_MINOR])
AC_SUBST([LIBTPMS_VER_MICRO])
AC_SUBST([LIBTPMS_VERSION])
AC_SUBST([LIBTPMS_VERSION_INFO])

DEBUG=""
AC_MSG_CHECKING([for debug-enabled build])
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug],[create a debug build]),
  [if test "$enableval" = "yes"; then
     DEBUG="yes"
     AC_MSG_RESULT([yes])
   else
     DEBUG="no"
     AC_MSG_RESULT([no])
   fi],
  [DEBUG="no",
   AC_MSG_RESULT([no])])

if test "$DEBUG" = "yes"; then
	CFLAGS="$CFLAGS -O0 -g -DDEBUG"
fi

debug_defines=
if test "$DEBUG" = "yes"; then
	debug_defines="-DTPM_DEBUG"
	# Enable the following only if ABSOLUTELY necessary
	# volatile state will be written and behavior changes
	#"-DTPM_VOLATILE_STORE"
fi
AC_SUBST(DEBUG_DEFINES, $debug_defines)

# AX_CHECK_LINK_FLAG needs autoconf 2.64 or later
have_version_script="no"
m4_if(
      m4_version_compare(
                         m4_defn([AC_AUTOCONF_VERSION]),
                         [2.64]),
      -1,
      [],
      [AX_CHECK_LINK_FLAG([-Wl,--version-script=$srcdir/src/test.syms],
                          [have_version_script="yes"],
                          [])]
)

AM_CONDITIONAL([HAVE_VERSION_SCRIPT], [test "x$have_version_script" = "xyes"])

AC_ARG_WITH([tpm1],
    AS_HELP_STRING([--with-tpm1], [build libtpms with TPM 1.2 support]), [], [with_tpm1=yes])
AM_CONDITIONAL([WITH_TPM1], [test "x$with_tpm1" != "xno"])
AS_IF([test "x$with_tpm1" != "xno"], [
    AC_DEFINE([WITH_TPM1], [1], [With TPM 1.2 support])
    AC_SUBST([WITH_TPM1], [1])
])

AC_ARG_WITH([tpm2],
	AS_HELP_STRING([--with-tpm2],[build libtpms with TPM2 support]),
	[],
	[with_tpm2=yes]
)
AS_IF([test "x$with_tpm2" = xyes], [
	AC_MSG_RESULT([Building with TPM2 support])
	AC_DEFINE_UNQUOTED([WITH_TPM2], 1, [whether to support TPM2])
	AC_SUBST([WITH_TPM2], [1])
	AM_CONDITIONAL(WITH_TPM2, true)
	cryptolib=openssl
], [
	AM_CONDITIONAL(WITH_TPM2, false)
	cryptolib=freebl
])

AC_SUBST(cryptolib, $cryptolib)

AC_ARG_WITH([openssl],
            AS_HELP_STRING([--with-openssl],[build libtpms with openssl library]),
              [AC_CHECK_LIB(crypto,
                            [AES_set_encrypt_key],
                            [],
                            AC_MSG_ERROR(Faulty openssl crypto library))
               AC_CHECK_HEADERS([openssl/aes.h],[],
                            AC_MSG_ERROR(Is openssl-devel/libssl-dev installed?))
               AC_MSG_RESULT([Building with openssl crypto library])
               cryptolib=openssl
              ]
)

case "$cryptolib" in
freebl)
	AM_CONDITIONAL(LIBTPMS_USE_FREEBL, true)
	AM_CONDITIONAL(LIBTPMS_USE_OPENSSL, false)
        AC_DEFINE([USE_FREEBL_CRYPTO_LIBRARY],
                  [1],
                  [use freebl crypto library])

	CFLAGS_save=$CFLAGS

	AC_CHECK_HEADERS([gmp.h],[],
			 AC_MSG_ERROR(gmp-devel/libgmp-dev is bad))

	CFLAGS="$(nspr-config --cflags)"
	if test $? -ne 0; then
		AC_MSG_ERROR(Could not find nspr-config. Is nspr-devel/libnspr4-dev installed?)
	fi
	CPPFLAGS=$CFLAGS
	AC_CHECK_HEADERS([plbase64.h],[],
			 AC_MSG_ERROR(You must install nspr-devel/libnspr4-dev))

	CFLAGS="$(nss-config --cflags) $CFLAGS"
	if test $? -ne 0; then
		AC_MSG_ERROR(Could not find nss-config. Is nss-devel/libnss3-dev installed?)
	fi
	CPPFLAGS="$CPPFLAGS $CFLAGS"
	AC_CHECK_HEADERS([sslerr.h],[],
			 AC_MSG_ERROR(nss-devel/libnss3-dev is bad))

	# Check for missing headers
	AC_CHECK_HEADERS([blapi.h],[],
			 AC_MSG_ERROR(nss-softokn-freebl-devel/libnss3-dev is missing blapi.h))
	# Check for missing freebl library or missing library functions
	LIBS_save="$LIBS"
	LIBS="$(nss-config --libs) $(nspr-config --libs)"
	AC_SEARCH_LIBS([AES_CreateContext], [freebl],[],
             AC_MSG_ERROR("Could not find AES_CreateContext(). Is nss-softokn-freebl-devel/libnss3-dev installed?"),
             [])
	LIBS="$LIBS_save"
	CFLAGS="$CFLAGS_save $CFLAGS"
	enable_use_openssl_functions=no
	;;
openssl)
	AM_CONDITIONAL(LIBTPMS_USE_FREEBL, false)
	AM_CONDITIONAL(LIBTPMS_USE_OPENSSL, true)
        AC_DEFINE([USE_OPENSSL_CRYPTO_LIBRARY],
                  [1],
                  [use openssl crypto library])
	LIBCRYPTO_EXTRA_CFLAGS="-DOPENSSL_SUPPRESS_DEPRECATED"
	AC_SUBST([LIBCRYPTO_EXTRA_CFLAGS])
	;;
esac

use_openssl_functions_for=""
use_openssl_functions_symmetric=0
use_openssl_functions_ec=0
use_openssl_functions_ecdsa=0
use_openssl_functions_rsa=0
AC_ARG_ENABLE(use-openssl-functions,
	AS_HELP_STRING([--disable-use-openssl-functions],
		       [Use TPM 2 crypot code rather than OpenSSL crypto functions]),
)
AS_IF([test "x$enable_use_openssl_functions" != "xno"], [
	if test "x$cryptolib" != "xopenssl"; then
		AC_MSG_ERROR([OpenSSL crypto function usage requires openssl as crypto library])
	fi
	LIBS_save=$LIBS
	# Check for symmetric key crypto functions
	not_found=0
	AC_CHECK_LIB([crypto], [EVP_CIPHER_CTX_new],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_EncryptInit_ex],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_aes_128_cbc],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_des_ede3_cbc],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_camellia_128_cbc],, not_found=1)
	AC_CHECK_LIB([crypto], [DES_random_key],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_CIPHER_CTX_iv],, not_found=1)
	if test "x$not_found" = "x0"; then
		use_openssl_functions_symmetric=1
		use_openssl_functions_for="symmetric (AES, TDES) "
	fi
	# Check for EC crypto support
	not_found=0
	AC_CHECK_LIB([crypto], [EC_KEY_set_group],, not_found=1)
	AC_CHECK_LIB([crypto], [EC_KEY_generate_key],, not_found=1)
	AC_CHECK_LIB([crypto], [EC_KEY_get0_private_key],, not_found=1)
	if test "x$not_found" = "x0"; then
		use_openssl_functions_ec=1
		use_openssl_functions_for="${use_openssl_functions_for}general elliptic curve (EC) "
	fi
	# Check for ECDSA crypto support
	not_found=0
	AC_CHECK_LIB([crypto], [ECDSA_SIG_new],, not_found=1)
	AC_CHECK_LIB([crypto], [ECDSA_SIG_set0],, not_found=1)
	AC_CHECK_LIB([crypto], [ECDSA_do_verify],, not_found=1)
	AC_CHECK_LIB([crypto], [ECDSA_do_sign],, not_found=1)
	AC_CHECK_LIB([crypto], [EC_KEY_set_group],, not_found=1)
	if test "x$not_found" = "x0"; then
		use_openssl_functions_ecdsa=1
		use_openssl_functions_for="${use_openssl_functions_for}elliptic curve (ECDSA) "
	fi
	# Check for RSA crypto functions
	not_found=0
	AC_CHECK_LIB([crypto], [RSA_set0_key],, not_found=1)
	AC_CHECK_LIB([crypto], [RSA_set0_factors],, not_found=1)
	AC_CHECK_LIB([crypto], [RSA_set0_crt_params],, not_found=1)
	AC_CHECK_LIB([crypto], [RSA_generate_key_ex],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_CTX_new],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_assign],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_encrypt_init],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_encrypt],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_decrypt_init],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_decrypt],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_sign_init],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_sign],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_verify_init],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_PKEY_verify],, not_found=1)
	AC_CHECK_LIB([crypto], [EVP_get_digestbyname],, not_found=1)
	# OpenSSL 3.0 turned some #defines into functions
	AX_CHECK_DEFINE([<openssl/rsa.h>], [EVP_PKEY_CTX_set0_rsa_oaep_label],,
	    AC_CHECK_LIB([crypto], [EVP_PKEY_CTX_set0_rsa_oaep_label],, not_found=1)
	)
	AX_CHECK_DEFINE([<openssl/rsa.h>], [EVP_PKEY_CTX_set_rsa_padding],,
	    AC_CHECK_LIB([crypto], [EVP_PKEY_CTX_set_rsa_padding],, not_found=1)
	)
	AX_CHECK_DEFINE([<openssl/rsa.h>], [EVP_PKEY_CTX_set_rsa_oaep_md],,
	    AC_CHECK_LIB([crypto], [EVP_PKEY_CTX_set_rsa_oaep_md],, not_found=1)
	)
	AX_CHECK_DEFINE([<openssl/evp.h>], [EVP_PKEY_CTX_set_signature_md],,
	    AC_CHECK_LIB([crypto], [EVP_PKEY_CTX_set_signature_md],, not_found=1)
	)
	if test "x$not_found" = "x0"; then
		use_openssl_functions_rsa=1
		use_openssl_functions_for="${use_openssl_functions_for}RSA "
	fi
	LIBS=$LIBS_save
])
CFLAGS="$CFLAGS -DUSE_OPENSSL_FUNCTIONS_SYMMETRIC=$use_openssl_functions_symmetric"
CFLAGS="$CFLAGS -DUSE_OPENSSL_FUNCTIONS_EC=$use_openssl_functions_ec"
CFLAGS="$CFLAGS -DUSE_OPENSSL_FUNCTIONS_ECDSA=$use_openssl_functions_ecdsa"
CFLAGS="$CFLAGS -DUSE_OPENSSL_FUNCTIONS_RSA=$use_openssl_functions_rsa"

AC_ARG_ENABLE([sanitizers], AS_HELP_STRING([--enable-sanitizers], [Enable address sanitizing]),
    [SANITIZERS="-fsanitize=address,undefined"], [])
AC_ARG_ENABLE([fuzzer], AS_HELP_STRING([--enable-fuzzer], [Enable fuzzer]),
    [FUZZER="$SANITIZERS -fsanitize=fuzzer"
     AM_CONDITIONAL(WITH_FUZZER, true)],
    [AM_CONDITIONAL(WITH_FUZZER, false)])
AC_SUBST([SANITIZERS])
AC_SUBST([FUZZER])

AM_CONDITIONAL([WITH_FUZZING_ENGINE], [test "x$LIB_FUZZING_ENGINE" != "x"])
AC_SUBST([LIB_FUZZING_ENGINE])

AC_ARG_ENABLE([test-coverage],
  AS_HELP_STRING([--enable-test-coverage], [Enable test coverage flags]),
  [COVERAGE_CFLAGS="-fprofile-arcs -ftest-coverage" COVERAGE_LDFLAGS="-fprofile-arcs"])

LT_INIT
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
LT_INIT

#AM_GNU_GETTEXT_VERSION([0.15])
#AM_GNU_GETTEXT([external])

AC_C_CONST
AC_C_INLINE

AC_TYPE_SIZE_T

AC_CHECK_LIB(c, clock_gettime, LIBRT_LIBS="", LIBRT_LIBS="-lrt")
AC_SUBST([LIBRT_LIBS])

AC_ARG_ENABLE([hardening],
  AS_HELP_STRING([--disable-hardening], [Disable hardening flags]))

if test "x$enable_hardening" != "xno"; then
	# Some versions of gcc fail with -Wstack-protector,
	# some with -Wstack-protector-strong enabled
	if ! $CC -fstack-protector-strong $srcdir/include/libtpms/tpm_error.h 2>/dev/null; then
		if $CC -fstack-protector $srcdir/include/libtpms/tpm_error.h 2>/dev/null; then
			HARDENING_CFLAGS="-fstack-protector "
		fi
	else
		HARDENING_CFLAGS="-fstack-protector-strong "
	fi

	dnl Only support -D_FORTIFY_SOURCE=2 and have higher levels passed in by user
	dnl since they may create more overhead
	if $CC $CFLAGS -Werror -D_FORTIFY_SOURCE=2 $srcdir/include/libtpms/tpm_library.h 2>/dev/null; then
		HARDENING_CFLAGS="$HARDENING_CFLAGS -D_FORTIFY_SOURCE=2"
	fi
	dnl Check linker for 'relro' and 'now'
	save_CFLAGS="$CFLAGS"
	CFLAGS="-Wl,-z,relro -Werror"
	AC_MSG_CHECKING([whether linker supports -Wl,-z,relro])
	AC_LINK_IFELSE(
		[AC_LANG_SOURCE([[int main() { return 0; }]])],
		[HARDENING_LDFLAGS="$HARDENING_LDFLAGS -Wl,-z,relro"
		 AC_MSG_RESULT(yes)],
		[AC_MSG_RESULT(no)]
	)
	CFLAGS="-Wl,-z,now -Werror"
	AC_MSG_CHECKING([whether linker supports -Wl,-z,now])
	AC_LINK_IFELSE(
		[AC_LANG_SOURCE([[int main() { return 0; }]])],
		[HARDENING_LDFLAGS="$HARDENING_LDFLAGS -Wl,-z,now"
		 AC_MSG_RESULT(yes)],
		[AC_MSG_RESULT(no)]
	)
	CFLAGS="$save_CFLAGS"
	AC_SUBST([HARDENING_CFLAGS])
	AC_SUBST([HARDENING_LDFLAGS])
fi

AM_CFLAGS="$CFLAGS $COVERAGE_CFLAGS -Wall -Werror -Wreturn-type -Wsign-compare -Wno-self-assign -Wmissing-prototypes"
AM_CFLAGS="$AM_CFLAGS"
AM_LDFLAGS="$LDFLAGS $COVERAGE_LDFLAGS"

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES(Makefile                   \
		dist/libtpms.spec          \
		include/Makefile           \
		include/libtpms/Makefile   \
		include/libtpms/tpm_library.h \
		man/Makefile               \
		man/man3/Makefile          \
		src/Makefile               \
		libtpms.pc                 \
		tests/Makefile)
PKG_INSTALLDIR()
AC_OUTPUT

if test -z "$enable_debug" ; then
    enable_debug="no"
fi
if test -z "$with_tpm2"; then
    with_tpm2=no
fi

echo
echo "AM_CFLAGS=$AM_CFLAGS"
echo "HARDENING_CFLAGS=$HARDENING_CFLAGS"
echo "HARDENING_LDFLAGS=$HARDENING_LDFLAGS"
echo "AM_LDFLAGS=$AM_LDFLAGS"
echo
echo "Version to build        : $PACKAGE_VERSION"
echo "Crypto library          : $cryptolib"
echo "Debug build             : $enable_debug"
echo "With TPM1.2 support     : $with_tpm1"
echo "With TPM2 support       : $with_tpm2"
echo "HAVE_VERSION_SCRIPT     : $have_version_script"
echo "Use openssl crypto for  : $use_openssl_functions_for"
echo
echo
