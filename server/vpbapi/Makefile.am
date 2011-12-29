# Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc @BAYONNE_FLAGS@ @EXOSIP2_FLAGS@
LDADD = ../../common/libserver.a ../../runtime/libbayonne.la -lvpb @BAYONNE_LIBS@

sbin_PROGRAMS = bayonne-server-vpbapi
noinst_HEADERS = driver.h

bayonne_server_exosip2_SOURCES = driver.cpp

