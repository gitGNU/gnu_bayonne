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

#include "driver.h"

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

static bool shutdown_flag = false;
static unsigned startup_count = 0;
static unsigned shutdown_count = 0;

media::media(size_t size) : DetachedThread(size)
{
    waiting = session_set_new();
    sessions = 0;
}

void media::run(void)
{
    instance = ++startup_count;
    shell::log(DEBUG1, "starting media thread %d", instance);

    while(!shutdown_flag) {
        if(sessions > 0) 
            events = session_set_select(waiting, NULL, NULL);
        else {
            events = 0;
            Thread::sleep(50);
        }
    }
    shell::log(DEBUG1, "stopping media thread %d", instance);
    ++shutdown_count;
}

void media::shutdown(void)
{
    shutdown_flag = true;
    ortp_exit();
    while(shutdown_count < startup_count)
        Thread::sleep(50);
}

