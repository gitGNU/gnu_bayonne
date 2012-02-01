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

static keyfile keyserver(BAYONNE_CFGPATH "/server.conf");
static keyfile keydriver(BAYONNE_CFGPATH "/driver.conf");
static keyfile keyspans(BAYONNE_CFGPATH "/spans.conf");
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
    tsCount = tsUsed = tsSpan = active = down = 0;
    name = id;

    keygroup.load(strdup(str(BAYONNE_CFGPATH) + str(registry) + str(".conf")));

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

keydata *Driver::getSystem(void)
{
    return keyserver.get("system");
}

keydata *Driver::getRegistry(void)
{
    return keyserver.get("registry");
}

int Driver::start(void)
{
    return 0;
}

int Driver::setup(void)
{
    return -1;
}

void Driver::stop(void)
{
}

int Driver::startup(void)
{
    keydata *pri = keyserver.get("threads");
    const char *cp;

    if(pri)
        cp = pri->get("message");
    if(!cp)
        cp = "-1";

    Message::start(atoi(cp));
    return instance->start();
}

void Driver::shutdown(void)
{
    Message::stop();
    instance->stop();
}

int Driver::init(void)
{
    return instance->setup();
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

Timeslot *Driver::access(unsigned index)
{
    if(index >= instance->tsUsed)
        return NULL;

    Timeslot *ts = instance->tsIndex[index];
    ts->lock();
    return ts;
}

// we usually select by longest idle, but this can be overriden...

Timeslot *Driver::request(Group *group)
{
    enum {
        FIRST, LAST, IDLE
    } mode = IDLE;

    const char *cp = NULL;
    unsigned first = 0;
    unsigned index = instance->tsUsed;
    unsigned long idle = 0;
    Timeslot *ts, *prior = NULL;

    if(group && group->tsCount) {
        first = group->tsFirst;
        index = group->tsFirst + group->tsCount;
        cp = group->keys->get("select");
    }

    if(!cp)
        cp = "longest";

    if(case_eq(cp, "first"))
        mode = FIRST;
    else if(case_eq(cp, "last"))
        mode = LAST;
    else if(case_eq(cp, "idle") || case_eq(cp, "longest"))
        mode = IDLE;

    switch(mode) {
    case FIRST:
        while(first < index) {
            ts = instance->tsIndex[index++];
            ts->lock();
            if(ts->getIdle())
                return ts;
            ts->unlock();
        }
        return NULL;
    default:
        while(index > first) {
            ts = instance->tsIndex[--index];
            ts->lock();
            if(ts->getIdle() > idle) {
                if(mode == LAST)
                    return ts;
                if(prior)
                    prior->unlock();
                idle = ts->getIdle();
                prior = ts;
            }
            else
                ts->unlock();
        }
    }
    return prior;
}

void Driver::release(Timeslot *timeslot)
{
    if(timeslot)
        timeslot->unlock();
}

