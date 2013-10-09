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

static struct
{
    bool running;
    bool signalled;
    timeout_t slice;
    Background *thread;
} bk = {false, false, 500, NULL};

Background::Background(size_t stack) : DetachedThread(stack), Conditional()
{
    bk.thread = this;
}

Background::~Background()
{
    shutdown();
}

void Background::shutdown(void)
{
    if(bk.running) {
        bk.running = false;

        bk.thread->notify();

        while(bk.signalled) {
            Thread::sleep(10);
        }
    }
}

void Background::notify(void)
{
    if(!bk.thread)
        return;

    bk.thread->Conditional::lock();
    bk.signalled = true;
    bk.thread->Conditional::signal();
    bk.thread->Conditional::unlock();
}

timeout_t Background::schedule(void)
{
    return bk.slice;
}

void Background::schedule(timeout_t slice, int priority)
{
    bk.slice = slice;

    if(!bk.thread || bk.running)
        return;

    bk.thread->start(priority);
}

void Background::automatic(void)
{
}

// disable object delete in exit...
void Background::exit(void)
{
}

void Background::run(void)
{
    timeout_t timeout, current;
    Timeslot *ts;
    unsigned tsid;
    time_t now;
    Timer expires;

    shell::debug(1, "starting background thread");
    expires = Timer::inf;
    bk.running = true;

    for(;;) {
        Conditional::lock();
        if(!bk.running) {
            Conditional::unlock();
            shell::debug(1, "stopping background thread");
            bk.signalled = false;
            return; // exit thread...
        }
        timeout = expires.get();
        if(!bk.signalled && timeout) {
            if(timeout > bk.slice)
                timeout = bk.slice;
            Conditional::wait(timeout);
        }
        timeout = expires.get();
        if(bk.signalled || !timeout) {
            bk.signalled = false;
            Conditional::unlock();
            tsid = Driver::getCount();
            timeout = Timer::inf;
            time(&now);
            while(tsid) {
                ts = Driver::get(--tsid);
                current = ts->getExpires(now);
                if(current && current < timeout)
                    timeout = current;
                ts->expire();
            }
            expires = timeout;
        }
        else {
            bk.signalled = false;
            Conditional::unlock();
        }
        automatic();
    }
}

