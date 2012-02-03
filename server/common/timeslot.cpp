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

Timeslot::Timeslot(unsigned port, Group *grp) :
OrderedObject(), Script::interp(), Mutex()
{
    const char *cp = NULL;

    incoming = span = group = NULL;

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

unsigned long Timeslot::getIdle(void)
{
    return 0l;
}

bool Timeslot::post(Event *msg)
{
    return false;
}

bool Timeslot::send(Event *msg)
{
    lock();

    bool rtn = post(msg);

    unlock();
    return rtn;
}

