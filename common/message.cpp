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

static LinkedObject *msgs = NULL;
static bool cancelled = false;

static class msgthread : public JoinableThread, public Conditional
{
public:
    msgthread();

    void add(Message *msg);
    void stop(void);

private:
    void run(void);
} thread;

msgthread::msgthread() :
JoinableThread(), Conditional()
{
}

void msgthread::add(Message *msg)
{
    Conditional::lock();
    msg->enlist(&msgs);
    Conditional::signal();
    Conditional::unlock();
}

void msgthread::run(void)
{
    shell::log(DEBUG1, "starting message thread");

    for(;;) {
        Conditional::lock();
        if(cancelled) {
            Conditional::unlock();
            shell::log(DEBUG1, "stopping message thread");
            return; // exits thread...
        }
        if(msgs) {
            shell::log(DEBUG2, "delivering messages");
            Message::deliver();
        }
        Conditional::wait();
        Conditional::unlock();
    }
}

void msgthread::stop(void)
{
    Conditional::lock();
    cancelled = true;
    Conditional::signal();
    Conditional::unlock();
    join();
}

Message::Message(Timeslot *timeslot, Event *msg) :
LinkedObject()
{
    memcpy(&event, msg, sizeof(event));
    ts = timeslot;

    thread.add(this);
}

void Message::deliver(void)
{
    linked_pointer<Message> mp;

    while(is(mp)) {
        register Timeslot *ts = mp->ts;
        ts->lock();
        ts->post(&mp->event);
        ts->unlock();
        mp.next();
    }
    msgs = NULL;
}

void Message::start(int priority)
{
    thread.start();
}

void Message::stop(void)
{
    thread.stop();
}
