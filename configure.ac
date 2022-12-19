AC_PREREQ(2.65)

# For releases bump this version here,
# but also remember to bump the lib version as instructed below
# Don't forget to check also GIR_VERSION
m4_define([dee_major], [1])
m4_define([dee_minor], [2])
m4_define([dee_micro], [7])
m4_define([dee_api],
          [dee_major.dee_minor])
m4_define([dee_version],
          [dee_major.dee_minor.dee_micro])

AC_INIT([dee],[dee_version],[https://bugs.launchpad.net/dee])
AC_CONFIG_SRCDIR([src/dee.h])
AC_CONFIG_MACRO_DIR([build/autotools])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.9])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_PATH_PYTHON

###############################################
### pygobject overrides directory detection ###
###############################################

AC_ARG_WITH([pygi_overrides_dir],
            AC_HELP_STRING([--with-pygi-overrides-dir], [Path to pygobject overrides directory]))

AC_MSG_CHECKING(for pygobject overrides directory)
if test "x$with_pygi_overrides_dir" = "x" ; then
        overrides_dir="`$PYTHON -c 'import gi; print(gi._overridesdir)' 2>/dev/null`"
	# fallback if the previous failed
	if test "x$overrides_dir" = "x" ; then
		overrides_dir="${pyexecdir}/gi/overrides"
	fi
else
        overrides_dir="$with_pygi_overrides_dir"
fi

PYGI_OVERRIDES_DIR="$overrides_dir"
AC_SUBST(PYGI_OVERRIDES_DIR)
AC_MSG_RESULT($PYGI_OVERRIDES_DIR)

DEE_MAJOR_VERSION=dee_major
DEE_MINOR_VERSION=dee_minor
DEE_MICRO_VERSION=dee_micro
DEE_VERSION=dee_version
AC_SUBST(DEE_MAJOR_VERSION)
AC_SUBST(DEE_MINOR_VERSION)
AC_SUBST(DEE_MICRO_VERSION)
AC_SUBST(DEE_VERSION)

m4_define([gir_version],
          [dee_major.0])
GIR_VERSION=gir_version
AC_SUBST(GIR_VERSION)

# Before making a release, the DEE_LT_VERSION string should be updated.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
DEE_LT_CURRENT=6
DEE_LT_REV=1
DEE_LT_AGE=2
DEE_LT_VERSION="$DEE_LT_CURRENT:$DEE_LT_REV:$DEE_LT_AGE"
DEE_LT_LDFLAGS="-version-info $DEE_LT_VERSION -export-symbols-regex '^dee_.*'"

AC_SUBST(DEE_LT_VERSION)
AC_SUBST(DEE_LT_LDFLAGS)

dnl ===========================================================================

# Checks for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
LT_INIT

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures and compiler charecteristics
AC_C_CONST

# Checks for library functions
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([memset munmap strcasecmp strdup])

PKG_CHECK_MODULES(DEE,
                  glib-2.0     >= 2.32
                  gthread-2.0  >= 2.32
                  gobject-2.0  >= 2.32
                  gio-2.0      >= 2.32
                  gio-unix-2.0 >= 2.32
                 )
AC_SUBST(DEE_CFLAGS)
AC_SUBST(DEE_LIBS)

dnl ===========================================================================

if test "x$GCC" = "xyes"; then
  GCC_FLAGS="-g -Wall"
fi
AC_SUBST(GCC_FLAGS)

dnl = use strict compiler flags only on development releases ==================
m4_define([maintainer_flags_default], [m4_if(m4_eval(dee_micro  % 2), [1], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AS_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>@],[Use strict compiler flags @<:@default=no@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])

MAINTAINER_CFLAGS=""
AS_IF([test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
      [
        MAINTAINER_CFLAGS="-Werror -Wall -Wcast-align -Wno-uninitialized -Wempty-body -Wformat-security -Winit-self -Wno-error=deprecated-declarations"
      ]
)

AC_SUBST(MAINTAINER_CFLAGS)

dnl = GObject Introspection Check =============================================
GOBJECT_INTROSPECTION_CHECK([0.10.2])

dnl = GTK Doc Check ===========================================================
GTK_DOC_CHECK([1.8])

dnl = Check if build tests ====================================================
AC_ARG_ENABLE([tests],
              AS_HELP_STRING([--enable-tests=@<:@no/yes@:>@],[build tests suite @<:@default=yes@:>@]),,
              [enable_tests=yes])

AM_CONDITIONAL([WANT_TESTS], [test "x$enable_tests" != "xno"])

###########################
# gcov coverage reporting
###########################
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

dnl = Check for GLib Test Extensions (GTX) ====================================
AC_ARG_ENABLE([extended-tests],
              AS_HELP_STRING([--enable-extended-tests=@<:@no/yes@:>@],[build extended test suite (requires libgtx from lp:gtx) @<:@default=no@:>@]),,
              [enable_extended_tests=no])

if test "x$enable_extended_tests" = "xyes"; then
	AM_COND_IF([WANT_TESTS],,[
		AC_MSG_ERROR([extended tests require tests support, please --enable-tests])
	])
	AC_DEFINE(HAVE_GTX, 1, [Define to 1 if we have GLib Test Extensions (libgtx from lp:gtx)])
	
	PKG_CHECK_MODULES(GTX, [ gtx >= 0.2.2 ])
	AC_SUBST(GTX_CFLAGS)
	AC_SUBST(GTX_LIBS)
fi

AM_CONDITIONAL(HAVE_GTX, test "$enable_extended_tests" = "yes")

dnl = Check if we should compile with trace logging ===========================
AC_ARG_ENABLE([trace-log],
              AS_HELP_STRING([--enable-trace-log=@<:@no/yes@:>@],[build with very verbose logging @<:@default=no@:>@]),,
              [enable_trace_log=no])

if test "x$enable_trace_log" = "xyes"; then
	AC_DEFINE(ENABLE_TRACE_LOG, 1, [build with vert verbose logging])
fi

AM_CONDITIONAL(ENABLE_TRACE_LOG, test "$enable_trace_log" = "yes")

dnl ===========================================================================

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
AC_SUBST(GLIB_MKENUMS)

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_SUBST(GLIB_GENMARSHAL)

dnl ===========================================================================

AC_PATH_PROG([VALA_API_GEN], [vapigen])
AM_CONDITIONAL([HAVE_VAPIGEN], [test "x$VALA_API_GEN" != "x"])

dnl = Check for ICU ====================================
AC_ARG_ENABLE([icu],
              AS_HELP_STRING([--enable-icu=@<:@no/yes@:>@],[build with advanced unicode text handling (requires ICU >= 4.6) @<:@default=yes@:>@]),,
              [enable_icu=yes])

if test "x$enable_icu" = "xyes"; then
	AC_PREPROC_IFELSE([AC_LANG_PROGRAM([[
	#include "unicode/uvernum.h"
	#if U_ICU_VERSION_MAJOR_NUM < 4
	#error Dee ICU requires at least ICU v4.6
	#elif U_ICU_VERSION_MAJOR_NUM == 4 && U_ICU_VERSION_MINOR_NUM < 6
	#error Dee ICU requires at least ICU v4.6
	#endif]], [[]])],
	[icu_available=yes],
	[icu_available=no])

	if test "x$icu_available" = "xyes"; then
		AC_DEFINE(HAVE_ICU, 1, [Define to 1 if we have ICU])
		ICU_CFLAGS="$(icu-config --cflags)"
		ICU_LIBS="$(icu-config --ldflags-libsonly)"
		AC_SUBST(ICU_CFLAGS)
		AC_SUBST(ICU_LIBS)
		AC_OUTPUT([dee-icu-1.0.pc])
	else
		AC_MSG_ERROR([Dee ICU support requires ICU >= 4.6])
	fi
fi

AM_CONDITIONAL(HAVE_ICU, test "x$enable_icu" = "xyes")

dnl ===========================================================================
AC_OUTPUT([
  Makefile
  build/Makefile
  build/autotools/Makefile
  src/Makefile
  bindings/Makefile
  bindings/python/Makefile
  doc/Makefile
  doc/reference/Makefile
  doc/reference/dee-1.0/Makefile
  dee-1.0.pc
  tests/Makefile
  tools/Makefile
  examples/Makefile
  vapi/Makefile
]) 

dnl Output the results
AC_MSG_NOTICE([

  dee $VERSION
  ----------------

  Prefix         : ${prefix}

  ICU support    : ${enable_icu}

  Documentation  : ${enable_gtk_doc}
  Introspection  : ${enable_introspection}
  PyGi Overrides : ${overrides_dir}

  Tests          : ${enable_tests}
  Extended Tests : ${enable_extended_tests}
  Coverage       : ${use_gcov}
  Verbose logging: ${enable_trace_log}

  Extra CFlags   : ${CPPFLAGS} $MAINTAINER_CFLAGS
  Extra ValaFlags: ${CPPFLAGS} $MAINTAINER_VALAFLAGS
])
