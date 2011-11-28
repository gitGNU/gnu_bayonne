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
AM_CXXFLAGS = -I$(top_srcdir)/inc @BAYONNE_FLAGS@ $(CHECKFLAGS)
INCLUDES = -I$(top_srcdir)/inc
LDADD = ../runtime/libbayonne.la @BAYONNE_LIBS@
EXTRA_DIST = *.1 CMakeLists.txt

man_MANS = audiotool.1 tonetool.1 phrasebook.1 baylint.1
bin_PROGRAMS = phrasebook tonetool baylint audiotool

phrasebook_SOURCES = phrasebook.cpp

audiotool_SOURCES = audiotool.cpp

tonetool_SOURCES = tonetool.cpp

baylint_SOURCES = baylint.cpp
