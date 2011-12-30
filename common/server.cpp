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
#endif

static shell::flagopt helpflag('h',"--help",    _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::flagopt background('b', "--background", _TEXT("run as daemon"));
static shell::flagopt altback('d', NULL, NULL);
#ifdef  DEBUG
static shell::flagopt dump('D', "--dump", _TEXT("dump configuration"));
#endif
static shell::flagopt foreground('f', "--foreground", _TEXT("run in foreground"));
static shell::stringopt lang('L', "--lang", _TEXT("specify language"), "language", "C");
static shell::stringopt phrasebook('B', "--phrasebook", _TEXT("specify phrasebook directory"), "path", NULL);
static shell::stringopt prefix('P', "--prefix", _TEXT("specify alternate prefix path"), "path", NULL);
static shell::stringopt scripts('A', "--scripts", _TEXT("specify script directory"), "path", NULL);
static shell::stringopt suffix('S', "--suffix", _TEXT("audio extension"), ".ext", ".au");
static shell::stringopt voice('V', "--voice", _TEXT("specify voice library"), "name", "default");
static shell::flagopt version(0, "--version", _TEXT("show version information"));


#if defined(HAVE_SETRLIMIT) && defined(DEBUG)
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

#ifdef  DEBUG
static void dumpconfig(void)
{
    printf("configs = %s\n", BAYONNE_CFGPATH);
    printf("defines = %s\n", Env::get("definitions"));
    printf("libexec = %s\n", BAYONNE_LIBEXEC);
    printf("scripts = %s\n", Env::get("scripts"));
    printf("voices = %s\n", Env::get("voices"));
    exit(0);
}
#endif

static void usage(void)
{
    printf("%s\n", _TEXT("Usage: bayonne-server [options]"));
    printf("%s\n\n", _TEXT("Start bayonne service"));
    printf("%s\n", _TEXT("Options:"));
    shell::help();
    printf("\n%s\n", _TEXT("Report bugs to bayonne-devel@gnu.org"));
    exit(0);
}

static void versioninfo(void)
{
    printf("Bayonne " VERSION "\n%s", _TEXT(
        "Copyright (C) 2007,2008,2009 David Sugar, Tycho Softworks\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n"));
    exit(0);
}

static void init(int argc, char **argv, shell::mainproc_t svc = NULL)
{
    const char *cp;

    shell::bind("bayonne");

    shell args(argc, argv);

    bool detached = Env::init(&args);

    if(!svc)
        detached = false;
    else if(detached)
        detached = Driver::getDetached();

    if((is(background) || is(altback)) && svc)
        detached = true;
    else if(is(foreground))
        detached = false;

    corefiles();

    if(is(helpflag) || is(althelp) || args.argc() > 0)
        usage();

    if(is(version))
        versioninfo();

    // reset paths from config and env ...

    keydata *paths = Driver::getPaths();
    if(paths) {
        cp = paths->get("phrasebook");
        if(!cp)
            cp = paths->get("voices");

        if(cp && *cp)
            Env::set("voices", cp);

        cp = paths->get("scripts");
        if(cp && *cp)
            Env::set("scripts", cp);
    }

#ifndef _MSWINDOWS_
    // apply env overrides...
    // these may come from the defaults file...

    cp = getenv("VOICES");
    if(cp && *cp)
        Env::set("voices", cp);

    cp = getenv("SCRIPTS");
    if(cp && *cp)
        Env::set("scripts", cp);

#endif

    // apply switch options if used...

    if(is(phrasebook))
        Env::set("voices", *phrasebook);

    if(is(scripts))
        Env::set("scripts", *scripts);

#ifdef  DEBUG
    if(is(dump))
        dumpconfig();
#endif
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


