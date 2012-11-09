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

static Script *image = NULL;
static Mutex imglock;
static const char *groupname;

Driver *Driver::instance = NULL;

Driver::Driver(const char *id, const char *registry)
{
    assert(instance == NULL);

    instance = this;
    tsCount = tsUsed = tsSpan = active = down = 0;
    name = id;
    definitions = NULL;
    groupname = registry;
    autotimer = 5000;
    keyserver.load(BAYONNE_CFGPATH "/server.conf");
    keyserver.load(BAYONNE_CFGPATH "/driver.conf");
    keygroup.load(strdup(str(BAYONNE_CFGPATH) + str(registry) + str(".conf")));

#ifndef _MSWINDOWS_
    const char *home = NULL;

    if(getuid())
        home = getenv("BAYONNERC");

    if(getuid() && !home) {
        home = getenv("HOME");

        if(home)
            home = strdup(str(home) + str("/.bayonnerc"));
    }

    if(home && fsys::is_file(home)) {
        setenv("BAYONNERC", home, 1);
        keyserver.load(home);
        keydriver.load(home);
    }
    else
        setenv("BAYONNERC", BAYONNE_CFGPATH "/server.conf", 1);
#endif

    keys = keydriver.get(id);
    if(!keys)
        keys = keyserver.get("driver");
}

keydata *Driver::getKeys(const char *gid)
{
    return instance->keygroup.get(gid);
}

keydata *Driver::getPaths(void)
{
    return instance->keyserver.get("paths");
}

keydata *Driver::getSystem(void)
{
    return instance->keyserver.get("system");
}

int Driver::start(void)
{
    return 0;
}

void Driver::stop(void)
{
}

void Driver::automatic(void)
{
}

Script *Driver::load(void)
{
    char dirpath[256];
    size_t len;
    dir_t dir;
    Script *img = NULL;

    String::set(dirpath, sizeof(dirpath), env("scripts"));
    dir::open(dir, dirpath);

    if(!is(dir)) {
        shell::log(shell::ERR, "cannot load scripts from %s", dirpath);
        return NULL;
    }

    shell::debug(2, "loading scripts from %s", dirpath);
    len = strlen(dirpath);
    dirpath[len++] = '/';

    while(is(dir) && dir::read(dir, dirpath + len, sizeof(dirpath) - len) > 0) {
        char *ep = strrchr(dirpath + len, '.');
        if(!ep)
            continue;
        if(!String::equal(ep, ".bcs"))
            continue;
        shell::log(shell::INFO, "loading %s", dirpath + len);
        img = Script::compile(img, dirpath, definitions);
    }
    dir::close(dir);

    if(!img)
        return NULL;

    Scheduler::load(img, path("configs") + "/scheduler.conf");

    return img;
}

void Driver::compile(void)
{
    char dirpath[256];
    size_t len;
    dir_t dir;

    String::set(dirpath, sizeof(dirpath), env("definitions"));
    dir::open(dir, dirpath);

    if(!is(dir)) {
        shell::log(shell::ERR, "cannot compile definitions from %s", dirpath);
        return;
    }

    shell::debug(2, "compiling definitions from %s", dirpath);
    len = strlen(dirpath);
    dirpath[len++] = '/';

    while(is(dir) && dir::read(dir, dirpath + len, sizeof(dirpath) - len) > 0) {
        char *ep = strrchr(dirpath + len, '.');
        if(!ep)
            continue;
        if(!String::equal(ep, ".bcs"))
            continue;
        shell::log(shell::INFO, "compiling %s", dirpath + len);
        definitions = Script::compile(definitions, dirpath, NULL);
    }
    dir::close(dir);
}

int Driver::startup(void)
{
    keydata *pri = instance->keyserver.get("threads");
    const char *cp = NULL;

    if(pri)
        cp = pri->get("message");

    if(!cp)
        cp = "-1";

    instance->compile();        // compile definitions
    reload();                   // first load of image...
    Message::start(atoi(cp));
    return instance->start();
}

void Driver::shutdown(void)
{
    linked_pointer<Group> gp;

    gp = Group::groups;

    while(is(gp)) {
        gp->shutdown();
        gp.next();
    }

    Message::stop();
    instance->stop();

    gp = Group::spans;

    while(is(gp)) {
        gp->shutdown();
        gp.next();
    }
}

void Driver::reload(void)
{
    Script *img = instance->load();
    if(!img)
        return;
    imglock.acquire();
    // disconnect current image, deletes if no attachments...
    if(image)
        image->release();
    image=img;
    image->retain();
    imglock.release();
}

Group *Driver::getSpan(unsigned sid)
{
    linked_pointer<Group> gp = Group::spans;

    while(is(gp)) {
        if(gp->span == sid)
            return *gp;
        gp.next();
    }
    return NULL;
}

Group *Driver::getSpan(const char *id)
{
    linked_pointer<Group> gp = Group::spans;
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
    linked_pointer<Group> gp = Group::groups;

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

    if(eq(cp, "first"))
        mode = FIRST;
    else if(eq(cp, "last"))
        mode = LAST;
    else if(eq(cp, "idle") || eq(cp, "longest"))
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

Script *Driver::getImage(void)
{
    Script *img;
    imglock.acquire();
    img = image;
    if(image)
        image->retain();
    imglock.release();
    return img;
}

void Driver::release(Timeslot *timeslot)
{
    if(timeslot)
        timeslot->unlock();
}

void Driver::release(Script *image)
{
    if(image) {
        imglock.acquire();
        image->release();
        imglock.release();
    }
}

void Driver::snapshot(void)
{
    FILE *fp = Control::output("snapshot");

    if(!fp) {
        shell::log(shell::ERR, "%s\n",
            _TEXT("snapshot; cannot access file"));
        return;
    }

    shell::log(DEBUG1, "%s\n", _TEXT("snapshot started"));

    linked_pointer<Group> gp = Group::groups;
    if(is(gp))
        fprintf(fp, "%s:\n", groupname);

    while(is(gp)) {
        gp->snapshot(fp);
        gp.next();
    }

    gp = Group::spans;
    if(is(gp))
        fprintf(fp, "spans:\n");

    while(is(gp)) {
        gp->snapshot(fp);
        gp.next();
    }

    fclose(fp);
    shell::log(DEBUG1, "%s\n", _TEXT("snapshot completed"));
}

const char *Driver::dup(const char *text)
{
    return instance->memalloc::dup(text);
}

void *Driver::alloc(size_t size)
{
    return instance->memalloc::alloc(size);
}

