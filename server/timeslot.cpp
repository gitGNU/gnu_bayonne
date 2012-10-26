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

#define HASH_KEY_SIZE   97

static Mutex mlocker;
static unsigned tscount = 0;
static OrderedIndex list;
static OrderedIndex hash[HASH_KEY_SIZE];

Timeslot::Timeslot(Group *grp) : OrderedObject(&list), Script::interp(), Mutex()
{
    const char *cp = NULL;

    incoming = span = group = NULL;
    tsid = tscount++;
    inUse = false;

    handler = &Timeslot::idleHandler;
    tsmode = TS_UNCONNECTED;
    state = "initial";
    server::status[tsid] = '.';

    if(grp && grp->is_span()) {
        span = grp;
        cp = span->get("group");
    }

    if(cp)
        grp = Driver::getGroup(cp);
    if(grp)
        incoming = grp;
    else
        incoming = span;
}

void Timeslot::setHandler(handler_t proc, const char *name, char code)
{
    Event entry;

    shell::debug(4, "timeslot %d entering %s",
        tsid, name);

    handler = proc;
    state = name;
    server::status[tsid] = code;
    entry.id = ENTER_STATE;
    (this->*handler)(&entry);
}

bool Timeslot::idleHandler(Event *event)
{
    return true;
}

unsigned long Timeslot::getIdle(void)
{
    return 0l;
}

bool Timeslot::post(Event *msg)
{
    lock();

    bool rtn = (this->*handler)(msg);

    unlock();
    return rtn;
}

void Timeslot::startup(void)
{
    inUse = true;
}

void Timeslot::shutdown(void)
{
    setHandler(&Timeslot::idleHandler, "idle", '-');
    inUse = false;
    tsmode = TS_UNCONNECTED;
}

Timeslot *Timeslot::get(long cid)
{
    Timeslot *ts;
    mlocker.acquire();
    ts = (Timeslot *)hash[cid % HASH_KEY_SIZE].begin();
    while(ts != NULL) {
        if(ts->callid == cid)
            break;
        ts = (Timeslot *)ts->next;
    }
    mlocker.release();
    return ts;
}

void Timeslot::release(Timeslot *ts)
{
    mlocker.acquire();
    if(ts->inUse) {
        ts->shutdown();
        ts->delist(&hash[ts->callid % HASH_KEY_SIZE]);
        ts->enlist(&list);
        ++tscount;
    }
    mlocker.release();
}

bool Timeslot::request(Timeslot *ts, long cid)
{
    bool result = true;

    mlocker.acquire();
    if(!ts->inUse) {
        ts->callid = cid;
        ts->delist(&list);
        ts->enlist(&hash[cid % HASH_KEY_SIZE]);
        ts->startup();
        --tscount;
    }
    else
        result = false;
    mlocker.release();
    return result;
}

Timeslot *Timeslot::request(long cid)
{
    Timeslot *ts;

    mlocker.acquire();
    ts = (Timeslot *)list.begin();
    if(ts) {
        ts->callid = cid;
        ts->delist(&list);
        ts->enlist(&hash[cid % HASH_KEY_SIZE]);
        ts->startup();
        --tscount;
    }
    mlocker.release();
    return ts;
}

unsigned Timeslot::available(void)
{
    unsigned count;

    mlocker.acquire();
    count = tscount;
    mlocker.release();
    return count;
}