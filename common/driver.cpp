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
static keyfile keyspans(DEFAULT_CFGPATH "/bayonne/spans.conf");
static keyfile keygroup;
static Group *groups = NULL, *spans = NULL;
static unsigned spanid = 0;

Driver *Driver::instance = NULL;

Group::Group(const char *name) :
LinkedObject((LinkedObject **)&groups)
{
    keys = keygroup.get(name);
    id = strdup(name);
    tsFirst = tsCount;
    span = (unsigned)-1;

    if(!keys)
        return;

    const char *cp = keys->get("first");
    if(cp)
        tsFirst = atoi(cp);

    cp = keys->get("count");
    if(cp)
        tsCount = atoi(cp);
}

Group::Group(unsigned count) :
LinkedObject((LinkedObject **)&spans)
{
    char buf[32];
    unsigned ts = Driver::instance->tsSpan;

    snprintf(buf, sizeof(buf), "%d", spanid);

    keys = keyspans.get(id);
    if(keys)
        id = keys->get("name");
    else
        id = NULL;

    if(!id)
        id = strdup(buf);

    span = spanid;
    tsFirst = ts;
    tsCount = count;
    Driver::instance->tsSpan += count;
    ++spanid;
}

Driver::Driver(const char *id, const char *registry)
{
    assert(instance == NULL);

    instance = this;
    detached = true;
    tsCount = tsUsed = tsSpan = active = down = 0;
    name = id;

    keygroup.load(strdup(str(DEFAULT_CFGPATH "/bayonne/") + str(registry) + str(".conf")));

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

    keys = keydriver.get(id);
    if(!keys)
        keys = keyserver.get("driver");
}

keydata *Driver::getKeys(const char *gid)
{
    return keygroup.get(gid);
}

keydata *Driver::getPaths(void)
{
    return keyserver.get("paths");
}

int Driver::start(void)
{
    return 0;
}

void Driver::stop(void)
{
}

Timeslot *Driver::index(unsigned timeslot)
{
    return NULL;
}

Timeslot *Driver::request(void)
{
    return NULL;
}

int Driver::startup(void)
{
    return instance->start();
}

void Driver::shutdown(void)
{
    instance->stop();
}

Timeslot *Driver::getTimeslot(unsigned timeslot)
{
    if(timeslot >= instance->tsUsed)
        return NULL;

    return instance->index(timeslot);
}

Group *Driver::getSpan(unsigned sid)
{
    linked_pointer<Group> gp = spans;

    while(is(gp)) {
        if(gp->span == sid)
            return *gp;
        gp.next();
    }
    return NULL;
}

Group *Driver::getSpan(const char *id)
{
    linked_pointer<Group> gp = spans;
    unsigned sid = (unsigned)-1;

    if(isdigit(*id))
        sid = atoi(id);

    while(is(gp)) {
        if(eq(gp->id, id))
            return *gp;
        if(gp->span == sid)
            return *gp;
        gp.next();
    }
    return NULL;
}

Group *Driver::getGroup(const char *id)
{
    linked_pointer<Group> gp = spans;

    while(is(gp)) {
        if(eq(gp->id, id))
            return *gp;
        gp.next();
    }
    return NULL;
}

