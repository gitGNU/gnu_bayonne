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
EXTRA_DIST = bayonne.log bayonne.defaults bayonne.init \
	bayonne.cron bayonne.conf users.conf bayonne.service

logrotatedir=$(sysconfdir)/logrotate.d
systemddir=$(prefix)/lib/systemd/system

sysconf_DATA = bayonne.conf
conf_DATA = users.conf

confdir = $(sysconfdir)/bayonne

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(logrotatedir)/bayonne
	$(INSTALL_DATA) bayonne.log $(DESTDIR)/$(logrotatedir)/bayonne
	if test -d /etc/sysconfig ; then \
		$(mkinstalldirs) $(DESTDIR)/$(sysconfdir)/sysconfig ; \
		$(INSTALL_DATA) bayonne.defaults $(DESTDIR)/$(sysconfdir)/sysconfig/bayonne ; \
	else \
		$(mkinstalldirs) $(DESTDIR)/$(sysconfdir)/default ; \
		$(INSTALL_DATA) bayonne.defaults $(DESTDIR)/$(sysconfdir)/default/bayonne ; \
	fi
	$(mkinstalldirs) $(DESTDIR)/@INITRDDIR@
	$(mkinstalldirs) $(DESTDIR)/$(sysconfdir)/cron.hourly
	$(INSTALL) -m 0755 bayonne.cron $(DESTDIR)/$(sysconfdir)/cron.hourly/bayonne
	if test "@INITRDDIR@" != "none" ; then $(mkinstalldirs) $(DESTDIR)/@INITRDDIR@ ; fi
	if test "@INITRDDIR@" != "none" ; then $(INSTALL) -m 0755 bayonne.init $(DESTDIR)/@INITRDDIR@/bayonne ; fi
	if test "@INITRDDIR@" = "none" ; then $(mkinstalldirs) $(DESTDIR)/$(systemddir) ; fi
	if test "@INITRDDIR@" = "none" ; then $(INSTALL) -m 0644 bayonne.service $(DESTDIR)/$(systemddir)/sipwitch.service ; fi


