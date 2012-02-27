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

static Mutex locking;
static unsigned tscount = 0;
static OrderedIndex list;

Timeslot::Timeslot(Group *grp) : OrderedObject(&list), Script::interp(), Mutex()
{
    const char *cp = NULL;

    incoming = span = group = NULL;
    tsid = tscount++;
    inUse = false;

    setIdle();

    if(grp && group->is_span()) {
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

void Timeslot::setIdle(void)
{
    step = STEP_IDLE;
    handler = &Timeslot::idleHandler;
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
    shell::debug(4, "timeslot %d step %d; event=%d\n",
        tsid, step, msg->id);

    return (this->*handler)(msg);
}

bool Timeslot::send(Event *msg)
{
    lock();

    bool rtn = post(msg);

    unlock();
    return rtn;
}

void Timeslot::startup(void)
{
    inUse = true;
}

void Timeslot::shutdown(void)
{
    inUse = false;
}

void Timeslot::release(Timeslot *ts)
{
    locking.acquire();
    if(ts->inUse) {
        ts->shutdown();
        ts->enlist(&list);
        ++tscount;
    }
    locking.release();
}

bool Timeslot::request(Timeslot *ts)
{
    bool result = true;

    locking.acquire();
    if(!ts->inUse) {
        ts->delist(&list);
        ts->startup();
        --tscount;
    }
    else
        result = false;
    locking.release();
    return result;
}

Timeslot *Timeslot::request(void)
{
    Timeslot *ts;

    locking.acquire();
    ts = (Timeslot *)list.begin();
    if(ts) {
        ts->delist(&list);
        ts->startup();
        --tscount;
    }
    locking.release();
    return ts;
}

unsigned Timeslot::available(void)
{
    unsigned count;

    locking.acquire();
    count = tscount;
    locking.release();
    return count;
}
