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
RELEASE = -version-info $(LT_VERSION)
AM_CXXFLAGS = -I$(top_srcdir)/inc @BAYONNE_FLAGS@
EXTRA_DIST =

lib_LTLIBRARIES = libbayonne.la 
noinst_HEADERS = common.h

libbayonne_la_LDFLAGS = @BAYONNE_LIBS@ $(RELEASE) 
libbayonne_la_SOURCES = server.cpp driver.cpp registry.cpp timeslot.cpp \
	thread.cpp segment.cpp stats.cpp dbi.cpp psignals.cpp uri.cpp

