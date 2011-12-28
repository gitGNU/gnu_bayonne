# Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AUTOMAKE_OPTIONS = no-dependencies dist-shar dist-zip
EXTRA_DIST = CMakeLists.txt config.h.cmake docs/script.tex \
	*.pc.in *-config.in *-config.1 autogen.sh ChangeLog
SUBDIRS = inc runtime common server utils test 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = bayonne.pc

scriptdir = $(bindir)
script_SCRIPTS = bayonne-config

man_MANS = bayonne-config.1

doc:
	cd docs ; pdflatex script.tex
	cd docs ; latex2html -nosubdir -split 0 script.tex
	cd docs ; pdflatex script.tex
	cd docs ; latex2html -nosubdir -split 0 script.tex
	cd docs ; lynx --dump script.html >script.text

