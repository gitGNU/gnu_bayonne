# Copyright (C) 2009 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

MAINTAINERCLEANFILES = Makefile.in Makefile
AM_CXXFLAGS = -I$(top_srcdir)/inc @BAYONNE_FLAGS@
EXTRA_DIST = bayonne.8

bin_PROGRAMS = bayonne baylint

bayonne_SOURCES = bayonne.cpp
bayonne_LDADD = @UCOMMON_LIBS@

baylint_SOURCES = baylint.cpp
baylint_LDADD = ../common/libbayonne.la @BAYONNE_LIBS@

man_MANS = bayonne.8

