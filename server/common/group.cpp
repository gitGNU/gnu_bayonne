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

static unsigned spanid = 0;
static keyfile keyspans(BAYONNE_CFGPATH "/spans.conf");

Group *Group::groups = NULL;
Group *Group::spans = NULL;

static const char *getscr(keydata *keys)
{
    char buf[65];

    if(!keys)
        return NULL;

    const char *cp = keys->get("script");
    if(cp && *cp == '@')
        return cp;

    if(!cp)
        return NULL;

    snprintf(buf, sizeof(buf), "@%s", cp);
    keys->set("script", buf);
    return keys->get("script");
}

Group::Group(keydata *keyset) :
LinkedObject((LinkedObject **)&groups)
{
    keys = keyset;
    id = keys->get();
    tsFirst = tsCount = 0;
    tsUsed = 0;
    span = (unsigned)-1;
    contact = NULL;

    if(!keys)
        return;

    const char *cp = keys->get("first");
    if(cp)
        tsFirst = atoi(cp);

    cp = keys->get("count");
    if(!cp)
        cp = keys->get("limit");

    if(cp)
        tsCount = atoi(cp);

    script = getscr(keys);
}

Group::Group(unsigned count) :
LinkedObject((LinkedObject **)&spans)
{
    char buf[32];
    unsigned ts = Driver::instance->tsSpan;

    snprintf(buf, sizeof(buf), "%d", spanid);

    contact = NULL;
    keys = keyspans.get(id);
    if(keys)
        id = keys->get("name");
    else
        id = NULL;

    if(!id)
        id = strdup(buf);

    script = getscr(keys);
    span = spanid;
    tsFirst = ts;
    tsCount = count;
    tsUsed = 0;
    Driver::instance->tsSpan += count;
    ++spanid;
}

bool Group::assign(Timeslot *ts)
{
    lock.acquire();
    if(tsUsed >= tsCount) {
        lock.release();
        return false;
    }

    ++tsUsed;
    lock.release();

    if(ts->span && ts->span != this)
        ts->span->assign(ts);

    ts->group = this;
    return true;
}

void Group::release(Timeslot *ts)
{
    if(ts->group != this)
        return;

    lock.acquire();
    if(tsUsed)
        --tsUsed;
    lock.release();

    if(ts->span && ts->span != this)
        ts->span->release(ts);
    else
        ts->group = NULL;
}

void Group::snapshot(FILE *fp)
{
    lock.acquire();
    if(is_span()) {
        fprintf(fp, "\t#%04d %04d-%04d %04d used\n",
            spanid, tsFirst, tsFirst + tsCount - 1, tsUsed);
    }
    else if(tsCount)
        fprintf(fp, "\t%-16s %04d-%04d %04d used\n",
            id, tsFirst, tsFirst + tsCount - 1, tsUsed);
    else
        fprintf(fp, "\t%-16s %04d used\n", id, tsUsed);
    lock.release();
}

Registry::Registry(keydata *keyset) :
Group(keyset)
{
    tsFirst = (unsigned)-1;
}

void Registry::snapshot(FILE *fp)
{
    lock.acquire();
    if(tsCount)
        fprintf(fp, "\t%-16s %04d used of %04d\n", id, tsUsed, tsCount);
    else
        fprintf(fp, "\t%-16s %04d used\n", id, tsUsed);
    lock.release();
}

