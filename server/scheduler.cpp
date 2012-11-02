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

static unsigned tod(const char *text)
{
    unsigned hour, minute;
    const char *sep = strchr(text, ':');

    if(!sep)
        return 0;

    hour = atoi(text);
    minute = atoi(++sep);

    if(tolower(sep[2]) == 'a' && hour == 12)
        hour = 0;
    else if(tolower(sep[2]) == 'p' && hour < 12)
        hour += 12;

    if(hour == 24 && !minute)
        hour = 0;

    if(hour > 23 || minute > 60)
        return 0;

    return hour * 60 + minute;
}

static unsigned months(const char *text)
{
    static const char *table[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};

    unsigned index = 0;
    while(index < 12) {
        if(eq_case(text, table[index++], 3))
            return index;
    }
    return 0;
}

static unsigned days(const char *text)
{
    static const char *table[] = {"su", "mo", "tu", "we", "th", "fr", "sa"};

    unsigned index = 0;
    while(index < 7) {
        if(eq_case(text, table[index++], 2))
            return index;
    }
    return 0;
}



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
    unsigned current = (unsigned)(now.hour() * 60 + now.minute());
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
        if(selected_dow[0] && !sp->dow[0])
            goto next;

        // if uses days, and not selected day, we skip...
        if(sp->dow[0] && !sp->dow[now.dow() + 1])
            goto next;

        // if we already have a month selection, skip
        if(selected_month && !sp->month)
            goto next;

        // if wrong month, skip...
        if(sp->month && sp->month != now.month())
            goto next;

        // if specific date and not matched, skip...
        if(sp->month && sp->day != now.day())
            goto next;

        // if we already have a year selection, skip
        if(selected_year && !sp->year)
            goto next;

        // if year, match
        if(sp->year && sp->year != (unsigned)now.year())
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

void Scheduler::load(Script *image, const char *path)
{
    assert(image != NULL && path != NULL && *path != 0);

    stringbuf<512> buffer;
    charfile sf(path, "r");
    char *group, *event, *token, *script;
    char *tokens;
    Scheduler *entry;
    unsigned year, day, month, fdow, ldow, start, end;
    bool hours;

    if(!is(sf)) {
        shell::log(shell::ERR, "cannot load schedule from %s", path);
        return;
    }

    shell::debug(2, "loading schedule from %s", path);

    while(sf.readline(buffer)) {
        year = day = month = fdow = ldow = 0;
        hours = false;
        start = 0;
        end = (24 * 60) - 1;
        script = tokens = NULL;
        group = String::token(buffer.c_mem(), &tokens, " \t", "{}\'\'\"\"");
        if(!group || !event)
            continue;

        if(*group == '#')
            continue;

        event = strrchr(group, '/');
        if(event)
            *(event++) = 0;
        else
            event = (char *)"*";

        if(eq(group, "*") || !*group)
            group = NULL;
        else
            group = image->dup(group);

        if(eq(event, "*"))
            event = NULL;
        else
            event = image->dup(event);

        while(NULL != (token = String::token(NULL, &tokens, " \t", "{}\'\'\"\""))) {
            // if script, get it and done
            if(*token == '@') {
                script = image->dup(token);
                break;
            }
            if(atoi(token) > 2000) {
                year = atoi(token);
                continue;
            }
            if(atoi(token)) {
                day = atoi(token);
                continue;
            }
            if((month = months(token)) > 0)
                continue;

            if((fdow = days(token)) > 0) {
                token = strchr(token, '-');
                if(!token) {
                    ldow = fdow;
                    continue;
                }
                ldow = days(++token);
            }

            if(strchr(token, ':')) {
                if(hours) {
                    end = tod(token);
                    if(!end)
                        end = (24 * 60) - 1;
                    else
                        --end;
                }
                else
                    start = tod(token);
                continue;
            }
        }
        if(!script)
            continue;
        caddr_t mp = (caddr_t)image->alloc(sizeof(Scheduler));
        entry = new(mp) Scheduler(&image->scheduler);
        entry->script = script;
        entry->group = group;
        entry->event = event;
        entry->start = start;
        entry->end = end;
        if(month && day) {
            entry->day = day;
            entry->month = month;
        }
        if(fdow && ldow) {
            entry->dow[0] = true;
            while(fdow <= ldow) {
                entry->dow[fdow++] = true;
            }
        }
    }
}
