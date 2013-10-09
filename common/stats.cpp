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

static unsigned count = 0, used = 0;

static class __LOCAL sta : public mapped_array<statmap>
{
public:
    sta();
    ~sta();

    void init(void);
} shm;

sta::sta() : mapped_array<statmap>()
{
}

sta::~sta()
{
    release();
    remove(STAT_MAP);
}

void sta::init(void)
{
    remove(STAT_MAP);
    create(STAT_MAP, count);
    initialize();
}

statmap *statmap::create(unsigned total)
{
    count = ++total;

    shm.init();

    statmap *node = shm(used++);
    node->type = SYSTEM;
    node->timeslots = Driver::getCount();
    Board::allocate();  // assign board stats if there are boards...
    return node;
}

statmap *statmap::getBoard(unsigned id)
{
    Board *board = Driver::getBoard(id);

    if(!count || used >= count)
        return NULL;

    statmap *node = shm(used++);
    snprintf(node->id, sizeof(node->id), "%d", id);
    node->type = BOARD;
    if(board)
        node->timeslots = board->getCount();
    return node;
}

statmap *statmap::getSpan(unsigned id)
{
    Span *span = Driver::getSpan(id);
    if(!count || used >= count)
        return NULL;

    statmap *node = shm(used++);
    snprintf(node->id, sizeof(node->id), "%d", id);
    node->type = SPAN;
    if(span)
        node->timeslots = span->getCount();
    return node;
}

statmap *statmap::getRegistry(const char *id, unsigned timeslots)
{
    if(!count || used >= count)
        return NULL;

    statmap *node = shm(used++);
    snprintf(node->id, sizeof(node->id), "%s", id);
    node->type = REGISTRY;
    node->timeslots = timeslots;
    return node;
}

unsigned statmap::active(void) const
{
    return stats[0].current + stats[1].current;
}

void statmap::assign(stat_t entry)
{
    Mutex::protect(this);
    update(entry);
    Mutex::release(this);
}

void statmap::update(stat_t entry)
{
    ++stats[entry].period;
    ++stats[entry].total;
    ++stats[entry].current;
    if(stats[entry].current > stats[entry].peak)
        stats[entry].peak = stats[entry].current;
    if(stats[entry].current > stats[entry].max)
        stats[entry].max = stats[entry].current;
}

void statmap::release(stat_t entry)
{
    Mutex::protect(this);
    if(active() == 1)
        time(&lastcall);
    --stats[entry].current;
    if(stats[entry].current < stats[entry].min)
        stats[entry].min = stats[entry].current;
    Mutex::release(this);
}

void statmap::period(FILE *fp)
{
    unsigned pos = 0;
    char text[80];
    size_t len;
    time_t last;

    while(pos < count) {
        statmap *node = shm(pos++);
        if(node->type == UNUSED)
            continue;

        if(fp) {
            if(node->type == statmap::BOARD)
                snprintf(text, sizeof(text), " board/%-6s", node->id);
            else if(node->type == statmap::SPAN)
                snprintf(text, sizeof(text), " span/%-7s", node->id);
            else if(node->type == statmap::REGISTRY)
                snprintf(text, sizeof(text), " net/%-8s", node->id);
            else
                snprintf(text, sizeof(text), " system      ");
            len = strlen(text);
        }
        else
            len = 0;

        Mutex::protect(node);
        for(unsigned entry = 0; entry < 2; ++entry) {
            if(fp) {
                snprintf(text + len, sizeof(text) - len, " %09lu %05hu %05hu",
                node->stats[entry].period, node->stats[entry].min, node->stats[entry].max);
                len = strlen(text);
            }
            node->stats[entry].pperiod = node->stats[entry].period;
            node->stats[entry].pmax = node->stats[entry].max;
            node->stats[entry].pmin = node->stats[entry].min;
            node->stats[entry].max = node->stats[entry].min = node->stats[entry].current;
            node->stats[entry].period = 0;
        }
        last = node->lastcall;
        Mutex::release(node);
        if(fp)
            fprintf(fp, "%s %ld\n", text, last);
    }
}

