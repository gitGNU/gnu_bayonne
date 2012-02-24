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

#include "driver.h"

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

static bool shutdown_flag = false;
static unsigned active_count = 0;
static unsigned startup_count = 0;
static unsigned shutdown_count = 0;

static char *remove_quotes(char *c)
{
    assert(c != NULL);

    char *o = c;
    char *d = c;
    if(*c != '\"')
        return c;

    ++c;

    while(*c)
        *(d++) = *(c++);

    *(--d) = 0;
    return o;
}

thread::thread(size_t size) : DetachedThread(size)
{
}

void thread::run(void)
{
    instance = ++startup_count;
    shell::log(DEBUG1, "starting event thread %d", instance);

    for(;;) {
        if(!shutdown_flag)
            sevent = eXosip_event_wait(1, 0);

        if(shutdown_flag) {
            shell::log(DEBUG1, "stopping event thread %d", instance);
            ++shutdown_count;
            return; // exits event thread...
        }

        if(!sevent)
            continue;

        ++active_count;
        shell::debug(2, "sip: event %d; cid=%d, did=%d, instance=%d",
            sevent->type, sevent->cid, sevent->did, instance);

        switch(sevent->type) {
        default:
            eXosip_lock();
            eXosip_default_action(sevent);
            eXosip_unlock();
        }

        eXosip_event_free(sevent);
        --active_count;
    }
}

void thread::shutdown(void)
{
    shutdown_flag = true;
    while(active_count)
        Thread::sleep(50);
    eXosip_quit();
    while(shutdown_count < startup_count)
        Thread::sleep(50);
}

