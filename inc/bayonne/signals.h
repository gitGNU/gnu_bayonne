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

/**
 * Common signal handling.
 * @file bayonne/signals.h
 */

#ifndef _BAYONNE_SIGNALS_H_
#define _BAYONNE_SIGNALS_H_

#ifndef _BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

#ifdef  HAVE_SIGWAIT
class __EXPORT signals : private JoinableThread
#else
class __EXPORT signals
#endif
{
private:
#ifdef  HAVE_SIGWAIT
    bool shutdown;
    bool started;

    sigset_t sigs;

    void run(void);
    void cancel(void);

    signals();
    ~signals();

    static signals thread;
#endif
public:
    static void service(const char *name);
    static void setup(void);
    static void start(void);
    static void stop(void);
};

END_NAMESPACE

#endif
