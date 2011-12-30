/* Copyright (C) 2010 David Sugar, Tycho Softworks

   This file is free software; as a special exception the author gives
   unlimited permission to copy and/or distribute it, with or without
   modifications, as long as this notice is preserved.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#cmakedefine VERSION "${VERSION}"
#cmakedefine PACKAGE "${PROJECT_NAME}"

#define DEFAULT_LIBPATH "${DEFAULT_LIBPATH}"
#define DEFAULT_VARPATH "${DEFAULT_VARPATH}"
#define DEFAULT_DATADIR "${DEFAULT_DATADIR}"
#define BAYONNE_LIBEXEC "${BAYONNE_LIBEXEC}"
#define BAYONNE_CFGPATH "${BAYONNE_CFGPATH}"
#define DEFAULT_PAGING  "${DEFAULT_PAGING}"

#cmakedefine HAVE_SYS_RESOURCE_H 1
#cmakedefine HAVE_PWD_H 1
#cmakedefine HAVE_ENDIAN_H 1
#cmakedefine HAVE_SPEEX_SPEEX_H 1
#cmakedefine HAVE_GSM_H 1
#cmakedefine HAVE_GSM_GSM_H 1
#cmakedefine HAVE_MATH_H 1
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_TERMIOS_H 1
#cmakedefine HAVE_SYS_IOCTL_H 1
#cmakedefine HAVE_IOCTL_H 1
#cmakedefine HAVE_SETRLIMIT 1
#cmakedefine HAVE_SETPGRP 1
#cmakedefine HAVE_GETUID 1
#cmakedefine HAVE_MKFIFO 1
#cmakedefine HAVE_SIGWAIT 1
#cmakedefine HAVE_SIGWAIT2 1
#cmakedefine HAVE_TLS 1

#define OSIP2_LIST_PTR  &
