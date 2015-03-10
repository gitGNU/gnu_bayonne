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
EXTRA_DIST = autogen.sh *.pc.in bayonne.spec.in bayonne.spec \
	DRIVERS bayonne-config.in bayonne-config.1 docs/script.tex \
	bayonne-config.h.cmake CMakeLists.txt cmake-abi.sh PKGBUILD.in
SUBDIRS = common config utils @BUILD_SUBDIRS@ inc
DIST_SUBDIRS = common config utils sipw vpb inc 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbayonne.pc

scriptdir = $(bindir)
script_SCRIPTS = bayonne-config

man_MANS = bayonne-config.1

doc:
	cd docs ; pdflatex script.tex
	cd docs ; latex2html -nosubdir -split 0 script.tex
	cd docs ; pdflatex script.tex
	cd docs ; latex2html -nosubdir -split 0 script.tex
	cd docs ; lynx --dump script.html >script.text

