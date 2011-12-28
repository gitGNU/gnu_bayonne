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

#if defined(HAVE_SETRLIMIT) && defined(DEBUG)
#include <sys/time.h>
#include <sys/resource.h>

static shell::flagopt helpflag('h',"--help",    _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::stringopt lang('L', "--lang", _TEXT("specify language"), "language", "C");
static shell::stringopt prefix('P', "--prefix", _TEXT("specify alternate prefix path"), "path", NULL);
static shell::stringopt suffix('S', "--suffix", _TEXT("audio extension"), ".ext", ".au");
static shell::stringopt voice('V', "--voice", _TEXT("specify voice library"), "name", "default");
static shell::stringopt phrasebook('B', "--phrasebook", _TEXT("specify phrasebook directory"), "path", NULL);

static void corefiles(void)
{
    struct rlimit core;

    assert(getrlimit(RLIMIT_CORE, &core) == 0);
#ifdef  MAX_CORE_SOFT
    core.rlim_cur = MAX_CORE_SOFT;
#else
    core.rlim_cur = RLIM_INFINITY;
#endif
#ifdef  MAX_CORE_HARD
    core.rlim_max = MAX_CORE_HARD;
#else
    core.rlim_max = RLIM_INFINITY;
#endif
    assert(setrlimit(RLIMIT_CORE, &core) == 0);
}
#else
static void corefiles(void)
{
}
#endif

static void init(int argc, char **argv, shell::mainproc_t svc = NULL)
{
    const char *cp;

    shell::bind("bayonne");

    shell args(argc, argv);

    Env::init(&args);

    bool detached = false;

    if(svc)
        detached = Driver::getDetached();

    corefiles();

    // reset paths from config and env ...

    keydata *paths = Driver::getPaths();
    if(paths) {
        cp = paths->get("phrasebook");
        if(!cp)
            cp = paths->get("voices");

        if(cp && *cp)
            Env::set("voices", cp);
    }

#ifndef _MSWINDOWS_
    // apply env overrides...
    // these may come from the defaults file...

    cp = getenv("VOICES");
    if(cp && *cp)
        Env::set("voices", cp);

#endif

    // apply switch options if used...

    if(is(phrasebook))
        Env::set("voices", *phrasebook);


}

static SERVICE_MAIN(main, argc, argv)
{
//  signals::service("bayonne");
    init(argc, argv);
}

PROGRAM_MAIN(argc, argv)
{
    init(argc, argv, &service_main);
    PROGRAM_EXIT(0);
}


