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

#ifdef  HAVE_PWD_H
#include <pwd.h>
#include <grp.h>
#endif

static shell::flagopt helpflag('h',"--help",    _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::flagopt background('b', "--background", _TEXT("run as daemon"));
static shell::flagopt altback('d', NULL, NULL);
static shell::numericopt concurrency('c', "--concurrency", _TEXT("process concurrency"), "level");
#ifdef  DEBUG
static shell::flagopt dump('C', "--dump", _TEXT("dump configuration"));
#endif
static shell::flagopt foreground('f', "--foreground", _TEXT("run in foreground"));
static shell::stringopt lang('L', "--lang", _TEXT("specify language"), "language", "C");
static shell::stringopt phrasebook('B', "--phrasebook", _TEXT("specify phrasebook directory"), "path", NULL);
static shell::stringopt prefix('P', "--prefix", _TEXT("specify alternate prefix path"), "path", NULL);
static shell::stringopt scripts('A', "--scripts", _TEXT("specify script directory"), "path", NULL);
static shell::stringopt definitions('I', "--definitions", _TEXT("specify script definitions"), "path", NULL);
static shell::stringopt prompts('D', "--prompts", _TEXT("specify prompt directory"), "path", NULL);
static shell::stringopt suffix('S', "--suffix", _TEXT("audio extension"), ".ext", ".au");
static shell::stringopt voice('V', "--voice", _TEXT("specify voice library"), "name", "default");
static shell::flagopt version(0, "--version", _TEXT("show version information"));

#ifdef  HAVE_PWD_H
static shell::stringopt group('g', "--group", _TEXT("use specified group permissions"), "groupid", "nobody");
#endif
static shell::stringopt loglevel('l', "--logging", _TEXT("set log level"), "level", "err");
static shell::counteropt priority('p', "--priority", _TEXT("set priority level"), "level");
static shell::flagopt restart('r', "--restartable", _TEXT("set to restartable process"));
#ifdef  HAVE_PWD_H
static shell::stringopt user('u', "--user", _TEXT("user to run as"), "userid", "nobody");
#endif
static shell::flagopt verbose('v', NULL, _TEXT("set verbosity, can be used multiple times"), false);
static shell::numericopt debuglevel('x', "--debug", _TEXT("set debug level directly"), "level", 0);

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
#ifdef _MSWINDOWS_
    printf("config = %s\n", BAYONNE_CFGPATH "/server.conf");
#else
    printf("config = %s\n", getenv("BAYONNERC"));
#endif
    printf("configs = %s\n", *Env::path("configs"));
    printf("controls = %s\n", *Env::path("controls"));
    printf("defines = %s\n", *Env::path("definitions"));
    printf("libexec = %s\n", *Env::path("libexec"));
    printf("scripts = %s\n", *Env::path("scripts"));
    printf("voices = %s\n", *Env::path("voices"));
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

static const char *userpath(const char *path)
{
    if(!path)
        return NULL;

#ifndef _MSWINDOWS_
    if(eq(path, "~/", 2) && getuid()) {
        const char *home = getenv("HOME");
        if(home)
            return strdup(str(home) + ++path);
    }
#endif
    return path;
}

static void dispatch(int slots)
{
    int argc;
    char *argv[65];
    char *cp, *tokens;
    DateTimeString dt;
    const char *err;

    Control::log("server starting %s", (const char *)dt);

    if(slots > 0)
        shell::log(shell::INFO, "started with %d timeslots active", slots);
    else
        shell::log(shell::ERR, "started with no timeslots active");

    while(NULL != (cp = Control::receive())) {
        shell::debug(9, "received request <%s>\n", cp);

        dt.set();

        if(eq(cp, "stop") || eq(cp, "down") || eq(cp, "exit"))
            break;

        if(eq(cp, "abort")) {
            abort();
            continue;
        }

        if(eq(cp, "reload")) {
            Driver::reload();
            continue;
        }

        if(eq(cp, "snapshot")) {
            Driver::snapshot();
            continue;
        }

        argc = 0;
        tokens = NULL;
        while(argc < 64 && NULL != (cp = const_cast<char *>(String::token(cp, &tokens, " \t", "{}")))) {
            argv[argc++] = cp;
        }
        argv[argc] = NULL;
        if(argc < 1)
            continue;

        if(eq(argv[0], "verbose")) {
            if(argc != 2) {
// invalid:
                Control::reply("missing or invalid argument");
                continue;
            }
            verbose = shell::loglevel_t(atoi(argv[1]));
            continue;
        }

        err = "unknown command";

//error:
        Control::reply(err);
    }

    dt.set();
    Control::log("server shutdown %s\n", (const char *)dt);
}

void server::start(int argc, char **argv, shell::mainproc_t svc)
{
    const char *cp;

    shell::bind("bayonne");
    shell args(argc, argv);

    bool detached = Env::init(&args);

    if(!svc)
        detached = false;

    if((is(background) || is(altback)) && svc)
        detached = true;
    else if(is(foreground))
        detached = false;

    corefiles();

    if(is(helpflag) || is(althelp) || args.argc() > 0)
        usage();

    if(is(version))
        versioninfo();

#ifdef  HAVE_PWD_H
    cp = getenv("GROUP");
    if(cp && *cp && !is(group))
        group.set(cp);

    cp = getenv("USER");
    if(cp && *cp && !is(user))
        user.set(cp);
#endif

    // reset paths from config and env ...

    keydata *paths = Driver::getPaths();
    if(paths) {
        cp = userpath(paths->get("phrasebook"));
        if(!cp)
            cp = userpath(paths->get("voices"));

        if(cp && *cp && fsys::isdir(cp))
            Env::set("voices", cp);

        cp = userpath(paths->get("scripts"));
        if(cp && *cp && fsys::isdir(cp))
            Env::set("scripts", cp);

        cp = userpath(paths->get("prompts"));
        if(cp && *cp && fsys::isdir(cp))
            Env::set("prompts", cp);

        cp = userpath(paths->get("definitions"));
        if(cp && *cp && fsys::isdir(cp))
            Env::set("definitions", cp);

        cp = userpath(paths->get("configs"));
        if(cp && *cp && fsys::isdir(cp))
            Env::set("configs", cp);

        cp = userpath(paths->get("libexec"));
        if(cp && *cp && fsys::isdir(cp))
            Env::set("libexec", cp);
    }

    paths = Driver::getSystem();
    if(paths) {
        cp = paths->get("concurrency");
        if(cp && *cp && !is(concurrency));
            concurrency.set(atol(cp));

        cp = paths->get("priority");
        if(cp && *cp && !is(priority))
            priority.set(atol(cp));

        cp = paths->get("logging");
        if(cp && *cp && !is(loglevel))
            loglevel.set(strdup(cp));
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


    cp = getenv("CONCURRENCY");
    if(cp && *cp && !is(concurrency))
        concurrency.set(atol(cp));

    cp = getenv("PRIORITY");
    if(cp && *cp && !is(priority))
        priority.set(atol(cp));

    cp = getenv("LOGGING");
    if(cp && *cp && !is(loglevel))
        loglevel.set(strdup(cp));

#endif

    // apply switch options if used...

    if(is(phrasebook))
        Env::set("voices", *phrasebook);

    if(is(scripts))
        Env::set("scripts", *scripts);

    if(is(definitions))
        Env::set("definitions", *definitions);

    if(is(prompts))
        Env::set("prompts", *prompts);

    if(is(prefix))
        Env::set("prefix", *prefix);

#ifdef  DEBUG
    if(is(dump))
        dumpconfig();
#endif

    if(*concurrency < 0)
        shell::errexit(1, "bayonne: concurrency: %ld: %s\n",
            *concurrency, _TEXT("negative levels invalid"));

    if(*concurrency > 0)
        Thread::concurrency(*concurrency);

    shell::priority(*priority);

#ifdef  SCHED_RR
    if(*priority > 0)
        Thread::policy(SCHED_RR);
#endif

    if(is(verbose))
        verbose.set(*verbose + (unsigned)shell::INFO);
    else {
        if(atoi(*loglevel) > 0)
            verbose.set(atoi(*loglevel));
        else if(eq(*loglevel, "0") || eq(*loglevel, "no", 2) || eq(*loglevel, "fail", 4))
            verbose.set((unsigned)shell::FAIL);
        else if(eq(*loglevel, "err", 3))
            verbose.set((unsigned)shell::ERR);
        else if(eq(*loglevel, "warn", 4))
            verbose.set((unsigned)shell::WARN);
        else if(eq(*loglevel, "noti", 4))
            verbose.set((unsigned)shell::NOTIFY);
        else if(eq(*loglevel, "info"))
            verbose.set((unsigned)shell::INFO);
        else if(eq(*loglevel, "debug", 5))
            verbose.set((unsigned)shell::DEBUG0 + atoi(*loglevel + 5));
    }

    if(is(debuglevel))
        verbose.set((unsigned)shell::DEBUG0 + *debuglevel);

#ifdef  HAVE_PWD_H
    struct passwd *pwd = NULL;
    struct group *grp = NULL;

    // if root user, then see if we change permissions...

    if(!getuid()) {
        if(is(user)) {
            if(atoi(*user))
                pwd = getpwuid(atoi(*user));
            else
                pwd = getpwnam(*user);
            if(!pwd)
                shell::errexit(2, "*** apennine: %s: %s\n", *user,
                    _TEXT("unknown or invalid user id"));
        }
    }

    if(is(group)) {
        if(atoi(*group))
            grp = getgrgid(atoi(*group));
        else
            grp = getgrnam(*group);
        if(!grp)
            shell::errexit(2, "*** bayonne: %s: %s\n", *group,
                _TEXT("unknown or invalid group id"));
    }

    if(grp) {
        umask(007);
        setgid(grp->gr_gid);
    }

    int uid = 0;
    if(pwd) {
        umask(007);
        if(!grp)
            setgid(pwd->pw_gid);
        uid = pwd->pw_uid;
    }

    endgrent();
    endpwent();
#endif
    const char *vardir = Env::get("prefix");
    const char *rundir = Env::get("controls");

    signals::setup();
    fsys::createDir(rundir, 0770);
    fsys::createDir(vardir, 0770);
    if(fsys::changeDir(vardir))
        shell::errexit(3, "*** bayonne: %s: %s\n",
            vardir, _TEXT("data directory unavailable"));

    shell::loglevel_t level = (shell::loglevel_t)*verbose;

    shell::logmode_t logmode = shell::SYSTEM_LOG;
    if(detached) {
        args.detach(svc);
        logmode = shell::CONSOLE_LOG;
    }

    shell::log("bayonne", level, logmode);

    Script::init();
//  Script::assign(keywords);

    // load plugins?

    if(!Control::create())
        shell::errexit(1, "*** bayonne: %s\n",
            _TEXT("no control file; exiting"));

    // drop root privilege
#ifdef  HAVE_PWD_H
    if(uid)
        setuid(uid);
#endif

    if(is(restart))
        args.restart();

    // create threads, etc...
    signals::start();
    notify::start();

    dispatch(Driver::startup());
    Driver::shutdown();

    notify::stop();
    signals::stop();
}

