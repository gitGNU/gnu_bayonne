# Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


AC_INIT([bayonne],[3.0.0])
AC_CONFIG_SRCDIR([autogen.sh])

LT_VERSION="3:0:0"
USES_UCOMMON_REQUIRED="6.6.0"
USES_CCSCRIPT_REQUIRED="5.0.0"
USES_CCAUDIO2_REQUIRED="2.1.0"
BUILD_SUBDIRS=""

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_SYSTEM
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXXCPP
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE([dist-zip])
AM_CONFIG_HEADER(bayonne-config.h)

AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE

if test -z "$PKG_CONFIG_PATH" ; then
    case "$prefix" in
    NONE|/usr/local|/usr)
        PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
        ;;
    *)
        PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
        ;;
    esac
else
    case "$prefix" in
    NONE|/usr/local|/usr)
        PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
        ;;
    *)
        PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"
        ;;
    esac
fi

export PKG_CONFIG_PATH

INCLUDE_FLAGS=""
PKG_BAYONNE_LIBS=""
PKG_BAYONNE_FLAGS=""
BAYONNE_LIBS=""
BAYONNE_FLAGS=""

test -z "$ac_with_malloc" && ac_with_malloc=""
test -z "$plugindir" && plugindir='${libdir}'
test -z "$scriptdir" && scriptdir='${datadir}/bayonne'

AC_DEFUN([AC_SUBST_DIR], [
    ifelse($2,,,$1="[$]$2")
    result="***"
    prior="A"
    while test "$prior" != "$result" ; do
        prior=`(echo "[$]$1")`
        $1=`(
        test "x$prefix" = xNONE && prefix="$ac_default_prefix"
        test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
        eval echo \""[$]$1"\"
        )`
        result=`(echo "[$]$1")`
    done
    AC_SUBST($1)
])

AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],[compile for debugging]))
if test -z "$enable_debug" ; then
    enable_debug="no"
elif test $enable_debug = "yes" ; then
    CXXFLAGS="${CXXFLAGS} -g -DDEBUG"
fi

AC_CACHE_CHECK(whether sigwait has 2 arguments,
    ac_cv_libc_sigwait2,
    AC_TRY_COMPILE([
        #define _GNU_SOURCE
        #define _POSIX_PTHREAD_SEMANTICS
        #include <stdio.h>
        #include <signal.h>],
        [sigset_t sigs; int signo; sigwait(&sigs, &signo);],
        ac_cv_libc_sigwait2=yes,
        ac_cv_libc_sigwait2=no
    )
)
if test "$ac_cv_libc_sigwait2" = "yes" ; then
    AC_DEFINE(HAVE_SIGWAIT2, [1], [2 argument form])
fi

# lets check init path
AC_ARG_WITH(initrddir,
    AC_HELP_STRING([--with-initrddir=path], [specify /etc/init.d path]),[
        INITRDDIR="$with_initrddir"
])

AC_ARG_WITH(paging,
    AC_HELP_STRING([--with-paging=size], [specify paging size]),
        [default_paging="$with_paging"])

if test -z "$default_paging" ; then
    default_paging=4096 ; fi

# lets check init path
AC_ARG_WITH(initrddir,
    AC_HELP_STRING([--with-initrddir=path], [specify /etc/init.d path]),[
        INITRDDIR="$with_initrddir"
])

# pkgconfig

AC_ARG_WITH(pkg-config,
    AC_HELP_STRING([--with-pkg-config],[enable support for pkg-config]),[
    PKG_CHECK_MODULES(UCOMMON, ucommon >= $USES_UCOMMON_REQUIRED)
    PKG_CHECK_MODULES(CCSCRIPT, ccscript >= $USES_CCSCRIPT_REQUIRED)
    PKG_CHECK_MODULES(CCAUDIO2, ccaudio2 >= $USES_CCAUDIO2_REQUIRED)
],[
    AC_PATH_PROG([UCOMMON],[ucommon-config],[none])
    if test "$UCOMMON" = "none" ; then
        AC_ERROR("required ucommon library missing")
    fi
    AC_PATH_PROG([CCSCRIPT],[ccscript-config],[none])
    if test "$CCSCRIPT" = "none" ; then
        AC_ERROR("required ccscript library missing")
    fi

    AC_PATH_PROG([CCAUDIO2],[ccaudio2-config],[none])
    if test "$CCAUDIO2" = "none" ; then
        AC_ERROR("required ccaudio2 library missing")
    fi

    UCOMMON_CFLAGS=`$UCOMMON --cflags`
    UCOMMON_LIBS=`$UCOMMON --libs`
    CCSCRIPT_CFLAGS=`$CCSCRIPT --cflags`
    CCSCRIPT_LIBS=`$CCSCRIPT --libs`
    CCAUDIO2_CFLAGS=`$CCAUDIO2 --cflags`
    CCAUDIO2_LIBS=`$CCAUDIO2 --libs`
])

AC_CHECK_HEADERS(sys/resource.h pwd.h)
AC_CHECK_FUNCS(setrlimit setpgrp setrlimit getuid mkfifo sigwait)

AC_CHECK_HEADER(resolv.h,[
    PKG_BAYONNE_LIBS="$PKG_BAYONNE_LIBS -lresolv"
    AC_DEFINE(HAVE_RESOLV_H, [1], [resolv header])
],[])

AC_LANG_CPLUSPLUS
AC_CHECK_HEADER(vpbapi.h, [has_vpbapi="yes"],[has_vpbapi="no"])

PKG_CHECK_MODULES(EXOSIP2, eXosip2 >= 3.0.0, [
    has_eXosip2="yes"
    AC_DEFINE(EXOSIP2_OPTION_SEND_101, 1, [new option 6])
    AC_DEFINE(EXOSIP2_OPTION_RPORT, 1, [new option 7])
],[
    AC_ARG_WITH(libeXosip2,
        AC_HELP_STRING([--with-libeXosip2=xxx],[specify eXosip2 library]),
            [EXOSIP2_LIBS="$with_libeXosip2"],[EXOSIP2_LIBS="-leXosip2"])

    AC_CHECK_HEADER(eXosip2/eXosip.h, [
        has_eXosip2="yes"
        AC_EGREP_HEADER(EXOSIP_OPT_DONT_SEND_101, eXosip2/eX_setup.h,[
            AC_DEFINE(EXOSIP2_OPTION_SEND_101, 1, [new option 6])
        ])
        AC_EGREP_HEADER(EXOSIP_OPT_USE_RPORT, eXosip2/eX_setup.h,[
            AC_DEFINE(EXOSIP2_OPTION_RPORT, 1, [new option 7])
        ])
    ],[
        has_eXosip2="no"
    ])
])

if test x"$has_eXosip2" = "xyes" ; then
    BUILD_SUBDIRS="$BUILD_SUBDIRS sipw"

    PKG_CHECK_MODULES(LIBOSIP2, libosip2 >= 3.0.0)
    AC_DEFINE(HAVE_EXOSIP2, [1], [has exosip2 header])
    AC_DEFINE(OSIP2_LIST_PTR, [&], [define pointer mode])
    EXOSIP2_LIBS="$EXOSIP2_LIBS $LIBOSIP2_LIBS"
    EXOSIP2_FLAGS="$EXOSIP2_CFLAGS $LIBOSIP2_CFLAGS"

    case "$target" in
    *mingw*)
        EXOSIP2_LIBS="$EXOSIP2_LIBS -liphlpapi -ldnsapi"
        ;;
    esac
fi

PKG_CHECK_MODULES(SYSTEMD, libsystemd-daemon >= 44, [
    AC_DEFINE(HAVE_SYSTEMD, 1, [systemd daemon support])
    PKG_BAYONNE_LIBS="$PKG_BAYONNE_LIBS $CCSCRIPT_LIBS $CCAUDIO2_LIBS $UCOMMON_LIBS $SYSTEMD_LIBS"
],[SYSTEMD_LIBS=""])

AC_CHECK_LIB(msvcrt, fopen, [
    PKG_BAYONNE_LIBS="$PKG_BAYONNE_LIBS -ladvapi32"
])

if test x"$has_vpbapi" = "xyes" ; then
    BUILD_SUBDIRS="$BUILD_SUBDIRS vpb" ; fi

if test -z "$BUILD_SUBDIRS" ; then
    AC_ERROR([no buildable targets.  Stopped])
    exit 1
fi

BAYONNE_FLAGS="$PKG_BAYONNE_FLAGS $EXOSIP2_FLAGS $CCAUDIO2_FLAGS $CCSCRIPT_CFLAGS $UCOMMON_CFLAGS"
BAYONNE_LIBS="$PKG_BAYONNE_LIBS $EXOSIP2_LIBS $CCAUDIO2_LIBS $CCSCRIPT_LIBS $UCOMMON_LIBS $ac_with_malloc $UCOMMON_CLINK"

if test "$localstatedir" == '${prefix}/var' ; then
    localstatedir="/var" ; fi

if test "$sysconfdir" == '${prefix}/etc' ; then
    sysconfdir="/etc"
fi

DLOPEN=""
if test "$enable_shared" = "no" ; then
    MODFLAGS="-static"
else
    MODFLAGS="-shared -avoid-version"
fi

AC_SUBST_DIR(default_scrpath, scriptdir)
AC_SUBST_DIR(default_cfgpath, sysconfdir)
AC_SUBST_DIR(default_varpath, localstatedir)
AC_SUBST_DIR(default_incpath, includedir)
AC_SUBST_DIR(default_libexec, libexecdir)
AC_SUBST_DIR(default_libpath, plugindir)
AC_SUBST_DIR(default_datadir, datadir)

if test -z "$INITRDDIR" ; then
    INITRDDIR="$default_cfgpath/init.d" ; fi

if test -z "$INITRDDIR" ; then
    INITRDDIR="$default_cfgpath/init.d" ; fi

case "$BAYONNE_FLAGS -I/usr/include" in
*-I${default_incpath}*)
    ;;
*)
    PKG_BAYONNE_FLAGS="$PKG_BAYONNE_FLAGS -I$default_incpath"
    ;;
esac

AC_DEFINE_UNQUOTED(DEFAULT_SCRPATH, "$default_scrpath", [script path])
AC_DEFINE_UNQUOTED(DEFAULT_CFGPATH, "$default_cfgpath", [config path])
AC_DEFINE_UNQUOTED(DEFAULT_VARPATH, "$default_varpath", [run path])
AC_DEFINE_UNQUOTED(DEFAULT_LIBPATH, "$default_libpath", [lib path])
AC_DEFINE_UNQUOTED(DEFAULT_LIBEXEC, "$default_libexec", [script path])
AC_DEFINE_UNQUOTED(DEFAULT_DATADIR, "$default_datadir", [data path])
AC_DEFINE_UNQUOTED(DEFAULT_PAGING, $default_paging, [default page size])

AC_SUBST(plugindir)
AC_SUBST(scriptdir)
AC_SUBST(USES_UCOMMON_REQUIRED)
AC_SUBST(USES_CCSCRIPT_REQUIRED)
AC_SUBST(USES_CCAUDIO2_REQUIRED)
AC_SUBST(INITRDDIR)
AC_SUBST(BUILD_SUBDIRS)
AC_SUBST(PKG_BAYONNE_FLAGS)
AC_SUBST(PKG_BAYONNE_LIBS)
AC_SUBST(INCLUDE_FLAGS)
AC_SUBST(BAYONNE_FLAGS)
AC_SUBST(BAYONNE_LIBS)
AC_SUBST(EXOSIP2_LIBS)
AC_SUBST(EXOSIP2_FLAGS)
AC_SUBST(LT_VERSION)
AC_SUBST(CXXFLAGS)
AC_SUBST(MODFLAGS)
AC_SUBST(DLOPEN)
AC_OUTPUT(Makefile common/Makefile utils/Makefile config/Makefile
sipw/Makefile vpb/Makefile PKGBUILD
inc/Makefile inc/bayonne/Makefile libbayonne.pc bayonne.spec bayonne-config)

