// Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
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

#include <bayonne-config.h>
#include <ucommon/ucommon.h>
#include <ccscript.h>
#include <ucommon/export.h>
#include <bayonne/bayonne.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

Registration::Registration(Registration *root, keydata *keys, const char *sid) :
LinkedObject()
{
    const char *cp = keys->get("limit");

    id = keys->get();

    if(cp)
        limit = atoi(cp);
    else
        limit = 0;

    stats = statmap::getRegistry(id, limit);
    activated = 0;
    id = memcopy(id);
    schema = sid;
    Next = root;
}

void Registration::release(void)
{
    activated = 0;
}

void Registration::activate(void)
{
    time(&activated);
}

bool Registration::refresh(void)
{
    return false;
}

void Registration::reload(keydata *keys)
{
    const char *cp = keys->get("limit");
    if(cp)
        limit = atoi(cp);
    else
        limit = 0;
    if(stats)
        stats->timeslots = limit;
}

const char *Registration::getHostid(const char *id)
{
    const char *cp = strrchr(id, '@');
    if(cp)
        return ++cp;

    if(!schema)
        return id;

    size_t len = strlen(schema);
    if(String::equal(schema, id, len))
        id += len;

    return id;
}

void Registration::getInterface(const char *uri, char *buffer, size_t size)
{
    struct sockaddr_storage iface;
    struct sockaddr *address;
    Socket::address resolver;
    char *cp;

    uri = getHostid(uri);

    if(*uri == '[') {
        String::set(buffer, size, ++uri);
        cp = strchr(buffer, ']');
        if(cp)
            *(cp) = 0;
    }
    else {
        String::set(buffer, size, uri);
        cp = strchr(buffer, ':');
        if(cp)
            *(cp) = 0;
    }
    resolver.set(buffer, 6000);
    address = resolver.getAddr();
    if(address == NULL || Socket::via((struct sockaddr *)&iface, address) != 0) {
        String::set(buffer, sizeof(buffer), "localhost");
        return;
    }
    resolver.clear();
    Socket::query((struct sockaddr *)&iface, buffer, size);
}

bool Registration::attach(statmap::stat_t stat)
{
    bool rtn = true;

    if(!stats)
        return true;

    Mutex::protect(stats);
    if(!limit || limit > stats->active())
        stats->update(stat);
    else
        rtn = false;
    Mutex::release(stats);
    return rtn;
}

void Registration::release(statmap::stat_t stat)
{
    if(stats)
        stats->release(stat);
}

