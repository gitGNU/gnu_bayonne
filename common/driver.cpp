// Copyright (C) 2010 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "server.h"

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static keyfile keyserver(DEFAULT_CFGPATH "/bayonne/server.conf");
static keyfile keydriver(DEFAULT_CFGPATH "/bayonne/driver.conf");

Driver *Driver::instance = NULL;

Driver::Driver()
{
    assert(instance == NULL);

    instance = this;
    detached = true;

#ifndef _MSWINDOWS_
    const char *home = NULL;

    if(getuid())
        home = getenv("HOME");

    if(home)
        home = strdup(str(home) + str("/.bayonnerc"));

    if(home) {
        keyserver.load(home);
        keydriver.load(home);
    }
#endif
}

keydata *Driver::getPaths(void)
{
    return keyserver.get("paths");
}

