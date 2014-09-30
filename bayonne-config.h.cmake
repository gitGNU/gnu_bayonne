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

#define DEFAULT_LIBPATH "${CMAKE_INSTALL_FULL_LIBDIR}"
#define DEFAULT_LIBEXEC "${CMAKE_INSTALL_FULL_LIBEXECDIR}"
#define DEFAULT_VARPATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}"
#define DEFAULT_CFGPATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}"
#define DEFAULT_SCRPATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}/bayonne.d"
#define DEFAULT_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}"
#define DEFAULT_PAGING  "${DEFAULT_PAGING}"

#cmakedefine HAVE_SYS_RESOURCE_H 1
#cmakedefine HAVE_RESOLV_H 1
#cmakedefine HAVE_PWD_H 1
#cmakedefine HAVE_SETRLIMIT 1
#cmakedefine HAVE_SETPGRP 1
#cmakedefine HAVE_GETUID 1
#cmakedefine HAVE_MKFIFO 1
#cmakedefine HAVE_SIGWAIT 1
#cmakedefine HAVE_SIGWAIT2 1
#cmakedefine HAVE_EXOSIP2 1
#cmakedefine HAVE_SYSTEMD 1
#cmakedefine HAVE_TLS 1

#define OSIP2_LIST_PTR  &
