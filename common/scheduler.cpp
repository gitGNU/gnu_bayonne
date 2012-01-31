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

Scheduler::Scheduler(LinkedObject **root) :
LinkedObject(root)
{
    script = group = event = NULL;
    year = month = day = 0;
    dow[0] = false;
    start = 0;
    end = (24 * 60) - 1;
}

const char *Scheduler::select(Script *image, const char *group, const char *event)
{
    linked_pointer<Scheduler> sp = image->scheduler;
    DateTime now;
    unsigned current = (unsigned)(now.getHour() * 60 + now.getMinute());
    const char *selected_group = NULL;
    const char *selected_event = NULL;
    const char *selected_script = NULL;
    unsigned selected_year = 0, selected_month = 0;
    bool selected_dow[8];

    while(is(sp)) {
        // if outside of time range, we skip...
        if(sp->start > sp->end) {
            if(current > sp->end && current < sp->start)
                goto next;
        }
        else {
            if(current < sp->start || current > sp->end)
                goto next;
        }
        // if we have dow and new one doesnt, we skip
        if(selected_dow[7] && !sp->dow[7])
            goto next;

        // if uses days, and not selected day, we skip...
        if(sp->dow[7] && !sp->dow[now.getDayOfWeek()])
            goto next;

        // if we already have a month selection, skip
        if(selected_month && !sp->month)
            goto next;

        // if wrong month, skip...
        if(sp->month && sp->month != now.getMonth())
            goto next;

        // if specific date and not matched, skip...
        if(sp->month && sp->day != now.getDay())
            goto next;

        // if we already have a year selection, skip
        if(selected_year && !sp->year)
            goto next;

        // if year, match
        if(sp->year && sp->year != (unsigned)now.getYear())
            goto next;

        // see if event already selected
        if(selected_event && !sp->event)
            goto next;

        // if has event and not our event, skip
        if(sp->event && !eq(sp->event, event))
            goto next;

        // see if group already selected
        if(selected_group && !sp->group)
            goto next;

        // see if group matches
        if(sp->group && !eq(sp->group, group))
            goto next;

        selected_group = sp->group;
        selected_event = sp->event;
        selected_year = sp->year;
        selected_month = sp->month;
        selected_script = sp->script;
        memcpy(selected_dow, sp->dow, sizeof(selected_dow));

next:
        sp.next();
    }
    return selected_script;
}
