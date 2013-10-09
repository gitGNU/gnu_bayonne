// Copyright (C) 2009 David Sugar, Tycho Softworks.
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

/**
 * Basic server call detail record.
 * This provides an interface for creating call detail and database query
 * objects which can be processed through plugins.  The dbi system has it's
 * own thread context and buffering, so dbi queries and updates can be
 * dispatched without delaying calls processing.
 * @file bayonne/dbi.h
 */

#ifndef _BAYONNE_DBI_H_
#define _BAYONNE_DBI_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef _UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef _BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

#ifndef _BAYONNE_SERVER_H_
#include <bayonne/server.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class __EXPORT dbi : public LinkedObject, protected Env
{
public:
    enum {START, STOP, QUERY, UPDATE, APPEND, INDEX} type;
    char uuid[48];          // call detail session id
    char source[64];        // table name for query
    char target[64];        // table name for update, sym name for query
    char script[64];
    char reason[16];
    char *optional;         // strdup optional data & dbi tuples
    unsigned timeslot, sequence;
    time_t starting;        // start of db query...
    unsigned long duration; // expiration for db queries

    // get a dbi instance to fill from free list or memory...
    static dbi *get(void);

    // post dbi query and return to free...
    static void post(dbi *data);

    // start thread...
    static void start(void);

    // stop subsystem
    static void stop(void);
};

END_NAMESPACE

#endif
