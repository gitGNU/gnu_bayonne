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

static LinkedObject *callbacks = NULL;

Registration *volatile Driver::registrations = NULL;
caddr_t Driver::timeslots = NULL;
caddr_t Driver::boards = NULL;
caddr_t Driver::spans = NULL;
unsigned Driver::board_count = 0;
unsigned Driver::board_alloc = 0;
unsigned Driver::span_count = 0;
unsigned Driver::span_alloc = 0;
unsigned Driver::ts_alloc = 0;
unsigned Driver::ts_count = 0;
OrderedIndex Driver::idle;
Driver *Driver::active = NULL;
condlock_t Driver::locking;
timeout_t Driver::stepping = 50;
statmap *Driver::stats = NULL;
const char *Driver::encoding = "generic";

#ifdef  _MSWINDOWS_
#define CONFIG_EXTENSION    ".ini"
#else
#define CONFIG_EXTENSION    ".conf"
#endif

Driver::callback::callback() :
LinkedObject(&callbacks)
{
}

void Driver::callback::reload(Driver *driver)
{
}

void Driver::callback::start(void)
{
}

void Driver::callback::stop(void)
{
}

bool Driver::callback::query(dbi *data)
{
    return false;
}

void Driver::callback::errlog(shell::loglevel_t level, const char *text)
{
}

Driver::image::image(Script *img, LinkedObject **root, const char *name) :
LinkedObject(root)
{
    id = name;
    ptr = img;
}

Driver::Driver(const char *dname) :
keyfile()
{
    Script *img = NULL, *def = NULL;
    char dirpath[256];
    size_t len;
    dir_t dir;
    const char *cp = env("config");

    shell::debug(2, "reloading config from %s", cp);
    server::load();

    snprintf(dirpath, sizeof(dirpath), "%s/%s" CONFIG_EXTENSION, cp, dname);
    if(fsys::is_file(dirpath)) {
        shell::debug(2, "reloading registrations from %s", dirpath);
        regfile.load(dirpath);
    }

    image_services = NULL;

    String::set(dirpath, sizeof(dirpath), env("scripts"));
    dir.open(dirpath);
    if(!is(dir)) {
        shell::log(shell::ERR, "cannot compile definitions from %s", dirpath);
        goto noscr;
    }

    len = strlen(dirpath);
    shell::debug(2, "compiling definitions from %s", dirpath);
    dirpath[len++] = '/';

    while(is(dir) && dir.read(dirpath + len, sizeof(dirpath) - len) > 0) {
        char *ep = strrchr(dirpath + len, '.');
        if(!ep)
            continue;
        if(!String::equal(ep, ".def"))
            continue;
        shell::log(shell::INFO, "compiling %s", dirpath + len);
        def = Script::compile(def, dirpath, NULL);
    }
    dir.close();

noscr:
    String::set(dirpath, sizeof(dirpath), env("services"));
    dir.open(dirpath);
    if(!is(dir)) {
        shell::log(shell::ERR, "cannot compile from %s", dirpath);
        return;
    }

    len = strlen(dirpath);
    shell::debug(2, "compiling services from %s", dirpath);
    dirpath[len++] = '/';

    image_definitions = def;

    while(is(dir) && dir.read(dirpath + len, sizeof(dirpath) - len) > 0) {
        char *ep = strrchr(dirpath + len, '.');
        caddr_t mp;
        if(!ep)
            continue;
        if(!String::equal(ep, ".scr"))
            continue;
        shell::log(shell::INFO, "compiling %s", dirpath + len);
        img = Script::compile(NULL, dirpath, *image_definitions);
        if(!img) {
            shell::log(shell::ERR, "%s: failed", dirpath + len);
            continue;
        }

        if(errors(img)) {
            delete img;
            continue;
        }

        *(ep++) = 0;
        mp = (caddr_t)zalloc(sizeof(image));
        new(mp) image(img, &image_services, dup(dirpath + len));
    }
    dir.close();
}

Driver::~Driver()
{
    linked_pointer<image> img;

    img = image_services;
    while(is(img)) {
        img->ptr = NULL;
        img.next();
    }
    image_definitions = NULL;
}

unsigned Driver::errors(Script *image)
{
    assert(image != NULL);
    linked_pointer<Script::error> ep = image->getListing();
    while(is(ep)) {
        shell::log(shell::ERR, "%s(%d): %s",
            image->getFilename(), ep->errline, ep->errmsg);
        ep.next();
    }
    return image->getErrors();
}

Timeslot *Driver::get(unsigned tsid)
{
    if(tsid >= ts_count)
        return NULL;

    return reinterpret_cast<Timeslot *>(&timeslots[ts_alloc * tsid]);
}

Board *Driver::getBoard(unsigned bdid)
{
    if(bdid >= board_count)
        return NULL;

    return reinterpret_cast<Board *>(&boards[board_alloc * bdid]);
}

Span *Driver::getSpan(unsigned span)
{
    if(span >= span_count)
        return NULL;

    return reinterpret_cast<Span *>(&spans[span_alloc * span]);
}

Driver *Driver::get(void)
{
    Driver *driver;
    locking.access();
    driver = active;
    if(!driver)
        locking.release();
    return driver;
}

void Driver::reload(void)
{
    linked_pointer<Registration> rp = registrations;

    locking.access();
    Driver *driver = active->create();
    locking.release();

    while(is(rp)) {
        keydata *keys = driver->regfile.get(rp->getId());
        if(keys)
            rp->reload(keys);
        else
            rp->release();
        rp.next();
    }

    commit(driver);
}

void Driver::update(void)
{
    linked_pointer<keydata::keyvalue> kv;
    keydata *keys = keyfile::get("script");

    if(keys)
        kv = keys->begin();

    while(is(kv)) {
        if(String::equal(kv->id, "stacking"))
            Script::stacking = atoi(kv->value);
        else if(String::equal(kv->id, "decimals"))
            Script::decimals = atoi(kv->value);
        else if(String::equal(kv->id, "stepping"))
            Script::stepping = atoi(kv->value);
        else if(String::equal(kv->id, "paging"))
            Script::paging = atol(kv->value);
        else if(String::equal(kv->id, "symbols"))
            Script::sizing = atoi(kv->value);
        else if(String::equal(kv->id, "indexing"))
            Script::indexing = atoi(kv->value);
        kv.next();
    }
}

void Driver::commit(Driver *driver)
{
    linked_pointer<Driver::callback> cb = callbacks;
    Driver *orig;

    driver->update();

    while(is(cb)) {
        cb->reload(driver);
        cb.next();
    }

    locking.modify();
    orig = active;
    active = driver;
    locking.commit();

    if(orig) {
        Thread::sleep(1000);
        delete orig;
    }
}

Script *Driver::getOutgoing(const char *name)
{
    Driver *driver = get();
    linked_pointer<image> ip;
    Script *scr = NULL;

    if(!driver)
        return NULL;

    ip = driver->image_services;
    while(is(ip)) {
        if(String::equal(ip->id, name)) {
            ip->ptr->retain();
            scr = ip->ptr.get();
            break;
        }
        ip.next();
    }
    release(driver);
    if(Script::find(scr, "@outgoing"))
        return scr;
    scr->release();
    return NULL;
}

Script *Driver::getIncoming(const char *name)
{
    Driver *driver = get();
    linked_pointer<image> ip;
    Script *scr = NULL;

    if(!driver)
        return NULL;

    ip = driver->image_services;
    while(is(ip)) {
        if(String::equal(ip->id, name)) {
            ip->ptr->retain();
            scr = ip->ptr.get();
            break;
        }
        ip.next();
    }
    release(driver);
    if(Script::find(scr, "@incoming"))
        return scr;
    scr->release();
    return NULL;
}

void Driver::release(keydata *keys)
{
    if(keys)
        locking.release();
}

void Driver::release(Script *img)
{
    if(img)
        img->release();
}

void Driver::release(Driver *driver)
{
    if(driver) {
        driver->regfile.release();
        locking.release();
    }
}

const char *Driver::dispatch(char **argv, int pid)
{
    linked_pointer<Registration> rp;

    if(String::equal(argv[0], "release")) {
        rp = registrations;
        while(is(rp)) {
            if(String::equal(rp->getId(), argv[1])) {
                rp->release();
                return NULL;
            }
            rp.next();
        }
        return "unknown resource";
    }

    if(String::equal(argv[0], "drop") || String::equal(argv[0], "hangup") || String::equal(argv[0], "enable") || String::equal(argv[0], "disable"))
        return "unknown resource";

    if(String::equal(argv[0], "refresh")) {
        if(argv[1] == NULL || argv[2] != NULL)
            goto invalid;

        rp = registrations;
        while(is(rp)) {
            if(String::equal(rp->getId(), argv[1])) {
                if(rp->refresh())
                    return NULL;
                return "refresh failed";
            }
            rp.next();
        }
        return "unknown resource";
    }

    return "unknown command";

invalid:
    return "missing or invalid argument";
}

void Driver::errlog(shell::loglevel_t level, const char *text)
{
    linked_pointer<Driver::callback> cb = callbacks;

    while(is(cb)) {
        cb->errlog(level, text);
        cb.next();
    }
}

void Driver::query(FILE *fp, dbi *data)
{
    DateTimeString dt(data->starting);
    const char *optional = "-";
    const char *buf = (const char *)dt;

    if(data->optional)
            optional = data->optional;

    switch(data->type) {
    case dbi::STOP:
        shell::debug(1, "call %u:%u %s %s %ld %s %s %s %s",
            data->timeslot, data->sequence, data->reason, buf, data->duration,
            data->source, data->target, data->script, optional);
        break;
    case dbi::QUERY:
        shell::debug(3, "query %u:%u (%s -> %s) %s\n",
            data->timeslot, data->sequence,
            data->source, data->target, optional);
        break;
    case dbi::UPDATE:
        shell::debug(3, "update %u:%u (%s) %s\n",
            data->timeslot, data->sequence,
            data->target, optional);
        break;
    case dbi::APPEND:
        shell::debug(3, "append %u:%u (%s) %s\n",
            data->timeslot, data->sequence,
            data->target, optional);
        break;
    default:
        break;
    }

    linked_pointer<Driver::callback> cb = callbacks;

    while(is(cb)) {
        if(cb->query(data))
            break;
        cb.next();
    }

    if(!fp || data->type != dbi::STOP)
        return;

    fprintf(fp, "%u:%u %s %s %ld %s %s %s %s\n",
        data->timeslot, data->sequence, data->reason, buf, data->duration,
        data->source, data->target, data->script, optional);
}

void Driver::start(void)
{
    linked_pointer<Driver::callback> cb = callbacks;

    dbi::start();

    while(is(cb)) {
        cb->start();
        cb.next();
    }

#ifndef _MSWINDOWS_
    unsigned pos;
    Segment *seg;
    FILE *fp;

    remove(env("boards"));

    if(board_count) {
        fp = fopen(env("boards"), "w");
        pos = 0;
        while(fp && pos < board_count) {
            seg = getBoard(pos++);
            if(seg) {
                fprintf(fp, "%03d %04d %04d %s\n",
                    seg->getInstance(), seg->getFirst(), seg->getCount(), seg->getDescription());
            }
        }
        if(fp)
            fclose(fp);
    }

    remove(env("spans"));

    if(span_count) {
        fp = fopen(env("spans"), "w");
        pos = 0;
        while(fp && pos < span_count) {
            seg = getSpan(pos++);
            if(seg) {
                fprintf(fp, "%03d %04d %04d %s\n",
                    seg->getInstance(), seg->getFirst(), seg->getCount(), seg->getDescription());
            }
        }
        if(fp)
            fclose(fp);
    }

#endif
}

void Driver::stop(void)
{
    linked_pointer<Driver::callback> cb = callbacks;

    unsigned ts_index = 0;
    Timeslot *ts;

    while(ts_index < ts_count) {
        ts = get(ts_index++);
        Timeslot::event_t event = {Timeslot::SHUTDOWN};
        ts->post(&event);
    }

    while(is(cb)) {
        cb->stop();
        cb.next();
    }

    dbi::stop();
}

void Driver::assign(statmap::stat_t stat)
{
    if(stats)
        stats->assign(stat);
}

void Driver::release(statmap::stat_t stat)
{
    if(stats)
        stats->release(stat);
}
