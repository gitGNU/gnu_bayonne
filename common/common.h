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
#include <ucommon/secure.h>
#include <ccscript.h>
#include <ucommon/export.h>
#include <bayonne/bayonne.h>

/**
 * Internal stuff, not seen in external interfaces
 * @file common.h
 */

namespace bayonne {

#ifdef  HAVE_SIGWAIT
class __EXPORT psignals : private JoinableThread
#else
class __EXPORT psignals
#endif
{
private:
#ifdef  HAVE_SIGWAIT
    bool shutdown;
    bool started;

    sigset_t sigs;

    void run(void);
    void cancel(void);

    psignals();
    ~psignals();

    static psignals thread;
#endif
public:
    static void service(const char *name);
    static void setup(void);
    static void start(void);
    static void stop(void);
};

} // end namespace

