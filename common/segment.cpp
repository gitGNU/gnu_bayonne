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

#include "common.h"

namespace bayonne {

static unsigned boards = 0;
static unsigned spans = 0;

Segment::Segment(unsigned id)
{
    instance = id;
    first = count = 0;
    live = true;
    description = "none";
    script = NULL;
    stats = NULL;
}

const char *Segment::suspend(void)
{
    Timeslot::event_t event;
    Timeslot *ts;
    unsigned tsc = count;
    unsigned tsid = first;

    if(!live)
        return "already suspended";

    while(tsc--) {
        ts = Driver::get(tsid++);
        if(!ts)
            continue;

        event.id = Timeslot::DISABLE;
        ts->post(&event);
        if(event.id == Timeslot::REJECT)
            return "cannot suspend";
    }
    live = false;
    return NULL;
}

const char *Segment::resume(void)
{
    Timeslot::event_t event;
    Timeslot *ts;
    unsigned tsc = count;
    unsigned tsid = first;

    if(live)
        return "already resumed";

    while(tsc--) {
        ts = Driver::get(tsid++);
        if(!ts)
            continue;

        event.id = Timeslot::ENABLE;
        ts->post(&event);
        if(event.id == Timeslot::REJECT)
            return "cannot resume";
    }
    live = true;
    return NULL;
}

void Segment::assign(statmap::stat_t stat)
{
    if(stats)
        stats->assign(stat);
}

void Segment::release(statmap::stat_t stat)
{
    if(stats)
        stats->release(stat);
}

Board::Board() : Segment(boards++)
{
    spans = 0;
}

void Board::allocate(void)
{
    unsigned index = 0;

    while(index < boards) {
        Board *board = Driver::getBoard(index);
        board->stats = statmap::getBoard(index++);
    }
}

const char *Board::suspend(void)
{
    unsigned spid = first;
    unsigned spc = spans;
    Span *span;

    if(!live)
        return "already suspended";

    if(!spc)
        return Segment::suspend();

    while(spc--) {
        span = Driver::getSpan(spid++);
        if(!span)
            continue;

        if(!span->suspend())
            return "cannot suspend spans";
    }
    live = false;
    return NULL;
}

const char *Board::resume(void)
{
    unsigned spid = first;
    unsigned spc = spans;
    Span *span;

    if(live)
        return "already resumed";

    if(!spc)
        return Segment::resume();

    while(spc--) {
        span = Driver::getSpan(spid++);
        if(!span)
            continue;

        if(!span->resume())
            return "cannot resume spans";
    }

    live = true;
    return NULL;
}

Span::Span() : Segment(spans++)
{
    stats = statmap::getSpan(instance);
}

void Span::hangup(void)
{
    Timeslot::event_t event;
    Timeslot *ts;
    unsigned tsc = count;
    unsigned tsid = first;

    while(tsc--) {
        ts = Driver::get(tsid++);
        if(!ts)
            continue;

        event.id = Timeslot::HANGUP;
        ts->post(&event);
    }
}

} // end namespace
