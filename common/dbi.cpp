// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
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

#include "common.h"

namespace bayonne {

class __LOCAL dbithread : public DetachedThread, public Conditional, protected Env
{
public:
    dbithread();

    inline void lock(void)
        {Conditional::lock();}

    inline void unlock(void)
        {Conditional::unlock();}

    inline void signal(void)
        {Conditional::signal();}

private:
    void exit(void);
    void run(void);
};

static LinkedObject *freelist = NULL;
static LinkedObject *runlist = NULL;
static dbi *runlast = NULL;
static Mutex private_locking;
static dbithread run;
static bool running = false;
static bool cdr = false;

dbithread::dbithread() : DetachedThread(), Conditional()
{
}

void dbithread::exit(void)
{
}

void dbithread::run(void)
{
    running = true;
    linked_pointer<dbi> cp;
    LinkedObject *next;
    FILE *fp = NULL;

    shell::log(shell::DEBUG0, "starting dbi thread");

    for(;;) {
        Conditional::lock();
        if(!running) {
            Conditional::unlock();
            shell::log(shell::DEBUG0, "stopped dbi thread");
            return;
        }
        if(!runlist)
            Conditional::wait();
        else
            Thread::yield();
        cp = runlist;
        if(runlist && cdr)
            fp = fopen(env("calls"), "a");
        cdr = false;
        runlist = runlast = NULL;
        Conditional::unlock();
        while(is(cp)) {
            next = cp->getNext();
            if(fp)
                Driver::query(fp, *cp);
            // optional tuples in strdup'd memory...
            if(cp->optional)
                free(cp->optional);
            private_locking.lock();
            cp->enlist(&freelist);
            private_locking.release();
            cp = next;
        }
        if(fp)
            fclose(fp);
    }
}

void dbi::post(dbi *rec)
{
    run.lock();
    rec->Next = NULL;
    if(runlast)
        runlast->Next = rec;
    else
        runlist = runlast = rec;
    if(rec->type == STOP)
        cdr = true;
    run.signal();
    run.unlock();
}

dbi *dbi::get(void)
{
    dbi *rec;

    private_locking.lock();
    if(freelist) {
        rec = (dbi *)freelist;
        freelist = rec->Next;
        private_locking.release();
        rec->uuid[0] = 0;
        rec->source[0] = 0;
        rec->target[0] = 0;
        rec->script[0] = 0;
        rec->reason[0] = 0;
        rec->optional = NULL;
        rec->timeslot = rec->sequence = 0;
        rec->starting = 0;
        rec->duration = 0;
        return rec;
    }
    private_locking.release();
    return (dbi *)(memget(sizeof(dbi)));
}

void dbi::start(void)
{
    run.start();
}

void dbi::stop(void)
{
    run.lock();
    running = false;
    run.signal();
    run.unlock();
}

} // end namespace
