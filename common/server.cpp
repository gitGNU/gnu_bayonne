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
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static const char *replytarget = NULL;
static const char *plugins = NULL;
static const char *argv0;
static bool daemon_flag = true;
static bool running = true;
static int exit_code = 0;
static time_t uptime, periodic;
static const char *prefix;
static const char *rundir;

shell_t server::args(DEFAULT_PAGING);

static shell::groupopt options("Options");
static shell::flagopt helpflag('h',"--help",    _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::flagopt backflag('b', "--background", _TEXT("run in background"));
static shell::numericopt concurrency('c', "--concurrency", _TEXT("process concurrency"), "level");
static shell::flagopt foreflag('f', "--foreground", _TEXT("run in foreground"));
#ifdef  HAVE_PWD_H
static shell::stringopt group('g', "--group", _TEXT("use specified group permissions"), "groupid", "nobody");
#endif
static shell::stringopt loglevel('L', "--logging", _TEXT("set log level"), "level", "err");
static shell::stringopt loading('l', "--plugins", _TEXT("specify modules to load"), "names", "none");
static shell::counteropt priority('p', "--priority", _TEXT("set priority level"), "level");
static shell::flagopt restart('r', "--restartable", _TEXT("set to restartable process"));
#ifdef  HAVE_PWD_H
static shell::stringopt user('u', "--user", _TEXT("user to run as"), "userid", "nobody");
#endif
static shell::flagopt verbose('v', NULL, _TEXT("set verbosity, can be used multiple times"), false);
static shell::flagopt version(0, "--version", _TEXT("show version information"));
static shell::numericopt debuglevel('x', "--debug", _TEXT("set debug level directly"), "level", 0);

#ifndef _MSWINDOWS_

#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

static FILE *fifo = NULL;
static char fifopath[128] = "";

#ifndef OPEN_MAX
#define OPEN_MAX 20
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(status) ((unsigned)(status) >> 8)
#endif

#ifndef _PATH_TTY
#define _PATH_TTY   "/dev/tty"
#endif
#endif

static void usage(void)
{
#if defined(DEBUG)
    printf("%s\n", _TEXT("Usage: bayonne-daemon [debug] [options]"));
#else
    printf("%s\n", _TEXT("Usage: bayonne-daemon [options]"));
#endif
    printf("%s\n\n", _TEXT("Start bayonne service"));

    shell::help();
#if defined(DEBUG)
    printf("%s", _TEXT(
        "\nDebug Options:\n"
        "  --dbg            execute command in debugger\n"
        "  --memcheck       execute with valgrind memory check\n"
        "  --memleak        execute with valgrind leak detection\n"
        "\n"
    ));
#endif
    printf("\n%s\n", _TEXT("Report bugs to bayonne-devel@gnu.org"));
    exit(0);
}

static void versioninfo(void)
{
    printf("Bayonne " VERSION "\n"
        "Copyright (C) 2008,2009 David Sugar, Tycho Softworks\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

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

#ifndef _MSWINDOWS_
fd_t server::input(void)
{
    if(fifo)
        return fileno(fifo);
    return INVALID_HANDLE_VALUE;
}

static void cleanup(void)
{
    if(fifopath[0]) {
        ::remove(fifopath);
        char *cp = strrchr(fifopath, '/');
        String::set(cp, 10, "/pidfile");
        ::remove(fifopath);
        fifopath[0] = 0;
    }
}

size_t server::attach(void)
{
    String::set(fifopath, sizeof(fifopath), env("control"));
    remove(fifopath);
    if(mkfifo(fifopath, 0660)) {
        fifopath[0] = 0;
        return 0;
    }

    fifo = fopen(fifopath, "r+");
    if(fifo) {
        shell::exiting(&cleanup);
        return 512;
    }
    fifopath[0] = 0;
    return 0;
}

void server::release(void)
{
    signals::stop();
    cleanup();
    exit(exit_code);
}

char *server::receive(void)
{
    static char buf[512];
    char *cp;

    if(!fifo)
        return NULL;

    reply(NULL);

retry:
    if(fgets(buf, sizeof(buf), fifo) == NULL)
        buf[0] = 0;
    cp = String::strip(buf, " \t\r\n");
    if(*cp == '/') {
        if(strstr(cp, ".."))
            goto retry;

        if(!eq(cp, "/tmp/.reply.", 12))
            goto retry;
    }

    if(*cp == '/' || isdigit(*cp)) {
        replytarget = cp;
        while(*cp && !isspace(*cp))
            ++cp;
        *(cp++) = 0;
        while(isspace(*cp))
            ++cp;
    }
    return cp;
}

void server::reply(const char *msg)
{
    assert(msg == NULL || *msg != 0);

    pid_t pid;

    if(msg)
        shell::log(shell::ERR, "control failed; %s", msg);

    if(!replytarget)
        return;

    if(isdigit(*replytarget)) {
        pid = atoi(replytarget);
        if(msg)
            kill(pid, SIGUSR2);
        else
            kill(pid, SIGUSR1);
    }
    replytarget = NULL;
}

#else

static HANDLE hFifo = INVALID_HANDLE_VALUE;
static HANDLE hLoopback = INVALID_HANDLE_VALUE;
static HANDLE hEvent = INVALID_HANDLE_VALUE;
static OVERLAPPED ovFifo;

fd_t server::input(void)
{
    return hLoopback;
}

static void cleanup(void)
{
    if(hFifo != INVALID_HANDLE_VALUE) {
        CloseHandle(hFifo);
        CloseHandle(hLoopback);
        CloseHandle(hEvent);
        hFifo = hLoopback = hEvent = INVALID_HANDLE_VALUE;
    }

}

size_t server::attach(void)
{
    hFifo = CreateMailslot(env("control"), 0, MAILSLOT_WAIT_FOREVER, NULL);
    if(hFifo == INVALID_HANDLE_VALUE)
        return 0;

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hLoopback = CreateFile(env("control"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ovFifo.Offset = 0;
    ovFifo.OffsetHigh = 0;
    ovFifo.hEvent = hEvent;
    shell::exiting(&cleanup);
    return 464;
}

void server::reply(const char *msg)
{
    assert(msg == NULL || *msg != 0);

    if(msg)
        shell::log(shell::ERR, "control failed; %s", msg);

    replytarget = NULL;
}

char *server::receive(void)
{
    static char buf[464];
    BOOL result;
    DWORD msgresult;
    const char *lp;
    char *cp;

    if(hFifo == INVALID_HANDLE_VALUE)
        return NULL;

    reply(NULL);

retry:
    result = ReadFile(hFifo, buf, sizeof(buf) - 1, &msgresult, &ovFifo);
    if(!result && GetLastError() == ERROR_IO_PENDING) {
        int ret = WaitForSingleObject(ovFifo.hEvent, INFINITE);
        if(ret != WAIT_OBJECT_0)
            return NULL;
        result = GetOverlappedResult(hFifo, &ovFifo, &msgresult, TRUE);
    }

    if(!result || msgresult < 1)
        return NULL;

    buf[msgresult] = 0;
    cp = String::strip(buf, " \t\r\n");

    if(*cp == '\\') {
        if(strstr(cp, ".."))
            goto retry;

        if(!eq(cp, "\\\\.\\mailslot\\", 14))
            goto retry;
    }

    if(*cp == '\\' || isdigit(*cp)) {
        replytarget = cp;
        while(*cp && !isspace(*cp))
            ++cp;
        *(cp++) = 0;
        while(isspace(*cp))
            ++cp;
        lp = replytarget + strlen(replytarget) - 6;
        if(eq(lp, "_temp"))
            goto retry;
    }
    return cp;
}

void server::release(void)
{
    signals::stop();
    cleanup();
    exit(exit_code);
}

#endif

static bool isTimeslot(const char *cp)
{
    if(!cp || !*cp)
        return false;

    while(*cp) {
        if(*cp < '0' || *cp > '9')
            return false;
        ++cp;
    }
    return true;
}

bool server::period(long slice)
{
    assert(slice > 0);

    FILE *fp = NULL;
    char buf[256];
    time_t now, next;

    slice *= 60l;   // convert to minute intervals...
    time(&now);
    next = ((periodic / slice) + 1l) * slice;
    if(now < next)
        return false;

    next = (now / slice) * slice;

    fp = fopen(env("stats"), "a");
    if(fp) {
        DateTimeString dt(periodic);
        fprintf(fp, "%s %ld\n", buf, next - periodic);
    }
    statmap::period(fp);
    if(fp)
        fclose(fp);
    return true;
}

void server::snapshot(int pid)
{
    FILE *fp = NULL;
    char buf[256];
    time_t now;

    String::set(buf, sizeof(buf), env("snapshot"));
#ifndef _MSWINDOWS_
    if(pid > 0) {
        snprintf(buf, sizeof(buf), "/tmp/.bayonne.%d", pid);
        remove(buf);
    }
#endif
    fp = fopen(buf, "w");
    time(&now);
    fprintf(fp, "BAYONNE:\n");
    fprintf(fp, "  timeslots: %d\n", Driver::getCount());
    fprintf(fp, "  uptime:    %ld\n", now - uptime);
    if(fp)
        fclose(fp);
}

void server::printlog(const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    fsys_t log;
    va_list args;
    char buf[1024];
    int len;
    char *cp;

    va_start(args, fmt);

    log.open(env("logfile"), fsys::GROUP_PRIVATE, fsys::APPEND);

    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    len = strlen(buf);
    if(buf[len - 1] != '\n')
        buf[len++] = '\n';

    if(is(log)) {
        log.write(buf, strlen(buf));
        log.close();
    }
    cp = strchr(buf, '\n');
    if(cp)
        *cp = 0;

    shell::debug(2, "logfile: %s", buf);
    va_end(args);
}

bool server::system(const char *fmt, ...)
{
    assert(fmt != NULL);

    va_list args;
    char buf[256];

    va_start(args, fmt);
    if(fmt)
        vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef  _MSWINDOWS_
#else
    int max = sizeof(fd_set) * 8;
    pid_t pid = fork();
#ifdef  RLIMIT_NOFILE
    struct rlimit rlim;

    if(!getrlimit(RLIMIT_NOFILE, &rlim))
        max = rlim.rlim_max;
#endif
    if(pid) {
        waitpid(pid, NULL, 0);
        return true;
    }
    ::signal(SIGQUIT, SIG_DFL);
    ::signal(SIGINT, SIG_DFL);
    ::signal(SIGCHLD, SIG_DFL);
    ::signal(SIGPIPE, SIG_DFL);
    int fd = ::open("/dev/null", O_RDWR);
    dup2(fd, 0);
    dup2(fd, 2);
    dup2(fileno(fifo), 1);
    for(fd = 3; fd < max; ++fd)
        ::close(fd);
    pid = fork();
    if(pid > 0)
        ::exit(0);
    ::execlp("/bin/sh", "sh", "-c", buf, NULL);
    ::exit(127);
#endif
    return true;
}

bool server::control(const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    char buf[512];
    fd_t fd;
    int len;
    bool rtn = true;
    va_list args;

    va_start(args, fmt);
#ifdef  _MSWINDOWS_
    fd = CreateFile(env("control"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(fd == INVALID_HANDLE_VALUE)
        return false;

#else
    fd = ::open(env("control"), O_WRONLY | O_NONBLOCK);

    if(fd < 0)
        return false;
#endif

    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    len = strlen(buf);
    if(buf[len - 1] != '\n')
        buf[len++] = '\n';
#ifdef  _MSWINDOWS_
    if(!WriteFile(fd, buf, (DWORD)strlen(buf) + 1, NULL, NULL))
        rtn = false;
    if(fd != hLoopback)
        CloseHandle(fd);
#else
    if(::write(fd, buf, len) < len)
        rtn = false;
    ::close(fd);
#endif
    return rtn;
}

void server::parse(int argc, char **argv, const char *dname)
{
    const char *cp;

    argv0 = argv[0];
    time(&uptime);
    time(&periodic);

    shell::bind("bayonne");
    corefiles();

#if defined(DEBUG)
    if(eq(argv[1], "-gdb") || eq(argv[1], "--gdb") || eq(argv[1], "-dbg") || eq(argv[1], "--dbg")) {
        char *dbg[] = {(char *)"gdb", (char *)"--args", NULL};
        const char *cp = env("DEBUGGER");
        if(cp && *cp)
            dbg[0] = (char *)cp;
        args.restart(argv[0], &argv[2], dbg);
    }

    if(eq(argv[1], "-memcheck") || eq(argv[1], "--memcheck")) {
        char *mem[] = {(char *)"valgrind", (char *)"--tool=memcheck", NULL};
        args.restart(argv[0], &argv[2], mem);
    }

    if(eq(argv[1], "-memleak") || eq(argv[1], "--memleak")) {
        char *mem[] = {(char *)"valgrind",
            (char *)"--tool=memcheck", (char *)"--leak-check=yes", NULL};
        args.restart(argv[0], &argv[2], mem);
    }
#endif

#ifdef _MSWINDOWS_
    rundir = strdup(str(env("APPDATA")) + "/bayonne");
    prefix = "C:\\Program Files\\bayonne";
    plugins = "C:\\Program Files\\bayonne\\plugins";

    set("users", _STR(str(prefix) + "/users.ini"));
    set("config", _STR(str(prefix) + "/bayonne.ini"));
    set("configs", _STR(str(prefix) + "/config"));
    set("drivers", _STR(str(prefix) + "/" + dname + ".ini"));
    set("services", _STR(str(prefix) + "/services"));
    set("controls", rundir);
    set("control", "\\\\.\\mailslot\\bayonne_ctrl");
    set("snapshot", _STR(str(rundir) + "/snapshot.log"));
    set("boards", _STR(str(rundir) + "/boards.log"));
    set("spans", _STR(str(rundir) + "/spans.log"));
    set("logfiles", _STR(str(prefix) + "/logs"));
    set("logfile", _STR(str(prefix) + "/logs/bayonne.log"));
    set("calls", _STR(str(prefix) + "/logs/bayonne.calls"));
    set("stats", _STR(str(prefix) + "/logs/bayonne.stats"));
    set("prefix", rundir);
    set("scripts", _STR(str(prefix) + "/scripts"));
    set("shell", "cmd.exe");
    prefix = rundir;
#else
    prefix = DEFAULT_VARPATH "/lib/bayonne";
    rundir = DEFAULT_VARPATH "/run/bayonne";

    set("reply", "/tmp/.bayonne.");
    set("users", DEFAULT_CFGPATH "/bayonne/users.conf");
    set("config", DEFAULT_CFGPATH "/bayonne.conf");
    set("configs", DEFAULT_CFGPATH "/bayonne");
    set("drivers", _STR(str(DEFAULT_CFGPATH "/bayonne/") + dname + ".conf"));
    set("services", DEFAULT_SCRPATH);
    set("controls", DEFAULT_VARPATH "/run/bayonne");
    set("control", DEFAULT_VARPATH "/run/bayonne/control");
    set("snapshot", DEFAULT_VARPATH "/run/bayonne/snapshot");
    set("boards", DEFAULT_VARPATH "/run/bayonne/boards");
    set("spans", DEFAULT_VARPATH "/run/bayonne/spans");
    set("logfiles", DEFAULT_VARPATH "/log");
    set("logfile", DEFAULT_VARPATH "/log/bayonne.log");
    set("calls", DEFAULT_VARPATH "/log/bayonne.calls");
    set("stats", DEFAULT_VARPATH "/log/bayonne.stats");
    set("prefix", DEFAULT_VARPATH "/lib/bayonne");
    set("scripts", DEFAULT_DATADIR "/bayonne");
    set("shell", "/bin/sh");
#endif

#ifdef  HAVE_PWD_H
    struct passwd *pwd = getpwuid(getuid());
    umask(007);

    if(getuid() && pwd && pwd->pw_dir && *pwd->pw_dir == '/') {
        set("prefix", pwd->pw_dir);
        if(!eq(pwd->pw_shell, "/bin/false") && !eq(pwd->pw_dir, "/var/", 5) && !eq(pwd->pw_dir, "/srv/", 5)) {
            umask(077);
            daemon_flag = false;
        };
    }

    if(!getuid())
        plugins = DEFAULT_LIBPATH "/bayonne";

    if(!daemon_flag && pwd) {
        rundir = strdup(str("/tmp/bayonne-") + str(pwd->pw_name));
        prefix = strdup(str(pwd->pw_dir) + "/.bayonne");

        set("users", _STR(str(pwd->pw_dir) + "/.bayonne/users.conf"));
        set("config", _STR(str(pwd->pw_dir) + "/.bayonnerc"));
        set("configs", prefix);
        set("drivers", _STR(str(pwd->pw_dir) + "/.bayonne/" + dname + ".conf"));
        set("services", prefix);
        set("controls", rundir);
        set("control", _STR(str(rundir) + "/control"));
        set("snapshot", _STR(str(rundir) + "/snapshot"));
        set("boards", _STR(str(rundir) + "/boards"));
        set("spans", _STR(str(rundir) + "/spans"));
        set("logfiles", rundir);
        set("logfile", _STR(str(rundir) + "/logfile"));
        set("scripts", prefix);
        set("calls", _STR(str(rundir) + "/calls"));
        set("stats", _STR(str(rundir) + "/stats"));
        set("prefix", prefix);
        set("shell", pwd->pw_shell);
    }

#else
    if(argv[1])
        daemon_flag = false;
#endif

    // parse and check for help
    args.parse(argc, argv);
    if(is(helpflag) || is(althelp) || args.argc() > 0)
        usage();


#ifdef  HAVE_PWD_H
    cp = env("GROUP");
    if(cp && *cp && !is(group))
        group.set(cp);

    cp = env("USER");
    if(cp && *cp && !is(user))
        user.set(cp);
#endif

    cp = env("CONCURRENCY");
    if(cp && *cp && !is(concurrency))
        concurrency.set(atol(cp));

    cp = env("PRIORITY");
    if(cp && *cp && !is(priority))
        priority.set(atol(cp));

    cp = env("VERBOSE");
    if(cp && *cp && !is(loglevel))
        loglevel.set(strdup(cp));

    cp = env("LOGGING");
    if(cp && *cp && !is(loglevel))
        loglevel.set(strdup(cp));

    cp = env("MODULES");
    if(!cp)
        cp = env("PLUGINS");
    if(cp && *cp && !is(loading))
        loading.set(strdup(cp));

    if(is(version))
        versioninfo();

    if(*concurrency < 0)
        shell::errexit(1, "sipwitch: concurrency: %ld: %s\n",
            *concurrency, _TEXT("negative levels invalid"));
}

void server::startup(shell::mainproc_t proc, bool detached)
{
    static Script::keyword_t keywords[] = {
        {NULL}};

    if(*concurrency > 0)
        Thread::concurrency(*concurrency);

    shell::priority(*priority);

#ifdef  SCHED_RR
    if(*priority > 0)
        Thread::policy(SCHED_RR);
#endif

    // fore and background...

    if(is(backflag))
        daemon_flag = true;

    if(is(foreflag))
        daemon_flag = false;


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

    signals::setup();
    dir::create(rundir, fsys::GROUP_PUBLIC);
    dir::create(prefix, fsys::GROUP_PRIVATE);
    if(fsys::prefix(prefix))
        shell::errexit(3, "*** bayonne: %s: %s\n",
            prefix, _TEXT("data directory unavailable"));

    shell::loglevel_t level = (shell::loglevel_t)*verbose;

    shell::logmode_t logmode = shell::SYSTEM_LOG;
    if(daemon_flag) {
        if(!detached)
            args.detach(proc);
        logmode = shell::CONSOLE_LOG;
    }

    shell::log("bayonne", level, logmode);

    Script::init();
    Script::assign(keywords);

    load(*loading);


    if(!attach())
        shell::errexit(1, "*** bayonne: %s\n",
            _TEXT("no control file; exiting"));

    // drop root privilege
#ifdef  HAVE_PWD_H
    if(uid)
        setuid(uid);
#endif

    if(is(restart))
        args.restart();
}

void server::stop(void)
{
    running = false;
}

void server::dispatch(void)
{
    int argc;
    char *argv[65];
    char *cp, *tokens;
    static int exit_code = 0;
    Driver *driver = NULL;
    const char *err = NULL;
    Board *board;
    Timeslot *ts;
    Timeslot::event_t event;
    int pid;
    unsigned count = Driver::getCount();

    signals::start();
    DateTimeString dt;

    server::printlog("server starting %s", (const char *)dt);

    if(count)
        shell::log(shell::INFO, "%d timeslots active", count);
    else
        shell::log(shell::ERR, "no timeslots active");

    while(running && NULL != (cp = server::receive())) {
        shell::debug(9, "received request <%s>\n", cp);

        dt.set();

        if(replytarget && isdigit(*replytarget))
            pid = atoi(replytarget);
        else
            pid = 0;

        if(eq(cp, "stop") || eq(cp, "down") || eq(cp, "exit"))
            break;

        if(eq(cp, "reload")) {
            server::printlog("server reloading %s", (const char *)dt);
            Driver::reload();
            continue;
        }

        if(eq(cp, "check")) {
//          if(!check())
//              server::reply("check failed");
            continue;
        }

        if(eq(cp, "restart")) {
#ifdef  SIGABRT
            exit_code = SIGABRT;
#else
            exit_code = 7;
#endif
            break;
        }

        if(eq(cp, "snapshot")) {
            snapshot(pid);
            continue;
        }

        if(eq(cp, "abort")) {
            abort();
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
invalid:
                server::reply("missing or invalid argument");
                continue;
            }
            verbose = shell::loglevel_t(atoi(argv[1]));
            continue;
        }

        if(eq(argv[0], "period")) {
            if(argc != 2)
                goto invalid;
            if(server::period(atol(argv[1]))) {
                server::printlog("server period %s", (const char *)dt);
            }
            continue;
        }

        if(eq(argv[0], "concurrency")) {
            if(argc != 2)
                goto invalid;
            Thread::concurrency(atoi(argv[1]));
            continue;
        }

        if(eq(argv[0], "suspend")) {
            if(argc != 2)
                goto invalid;

            board = Driver::getBoard(atoi(argv[1]));
            err = NULL;
            if(board)
                err = board->suspend();
            else
                err = "unknown board id";
            if(err)
                server::reply(err);
            continue;
        }

        if(eq(argv[0], "resume")) {
            if(argc != 2)
                goto invalid;

            board = Driver::getBoard(atoi(argv[1]));
            err = NULL;
            if(board)
                err = board->resume();
            else
                err = "unknown board id";
            if(err)
                server::reply(err);
            continue;
        }

timeslot:
        if(eq(argv[0], "drop") || eq(argv[0], "hangup")) {
            if(argc != 2)
                goto invalid;

            if(!isTimeslot(argv[1]))
                goto notimeslot;

            event.id = Timeslot::HANGUP;
post:
            ts = Driver::get(atoi(argv[1]));
            err = NULL;
            if(ts) {
                ts->post(&event);
                if(event.id == Timeslot::REJECT)
                    err = "request rejected";
            }
            else
                err = "invalid timeslot";
            if(err)
                server::reply(err);
            continue;
        }

        if(eq(argv[0], "enable")) {
            if(argc != 2)
                goto invalid;

            if(!isTimeslot(argv[1]))
                goto notimeslot;

            event.id = Timeslot::ENABLE;
            goto post;
        }

        if(eq(argv[0], "disable")) {
            if(argc != 2)
                goto invalid;

            if(!isTimeslot(argv[1]))
                goto notimeslot;

            event.id = Timeslot::DISABLE;
            goto post;
        }

        // because we can have "span xx option" style commands...
        if(eq(argv[0], "timeslot")) {
            if(argc != 3)
                goto invalid;

            if(!isTimeslot(argv[1])) {
                err = "invalid timeslot";
                goto error;
            }

            if(
              eq(argv[2], "drop") ||
              eq(argv[2], "hangup") ||
              eq(argv[2], "enable") ||
              eq(argv[2], "disable")) {
                argc = 2;
                argv[0] = argv[2];
                argv[2] = NULL;
                goto timeslot;
            }
        }

        err = "unknown command";
        driver = Driver::get();
        if(driver)
            err = driver->dispatch(argv, pid);

error:
        Driver::release(driver);
        server::reply(err);
        continue;

notimeslot:
        err = "invalid timeslot";
        driver = Driver::get();
        if(driver)
            err = driver->dispatch(argv, pid);
        Driver::release(driver);
        server::reply(err);
    }
    dt.set();
    server::printlog("server shutdown, reason=%d %s\n", exit_code, (const char *)dt);
}

#ifdef  _MSWINDOWS_
#define DLL_SUFFIX  ".dll"
#define LIB_PREFIX  "_libs"
#else
#define LIB_PREFIX  ".libs"
#define DLL_SUFFIX  ".so"
#endif

void server::load(const char *list)
{
    char buffer[256];
    char path[256];
    char *tp = NULL;
    char *ep;
    const char *cp;
    fsys    module;
    dir_t   dir;
    size_t  el;

    if(!list)
        list = *loading;

    if(!list || !*list || eq(list, "none"))
        return;

    if(eq(list, "auto")) {
        String::set(path, sizeof(path), argv0);
        ep = strstr(path, LIB_PREFIX);
        if(ep) {
            *ep = 0;
            el = strlen(path);
            String::set(path + el, sizeof(path) - el, "../modules/" LIB_PREFIX);
        }
        else
            String::set(path, sizeof(path), DEFAULT_LIBPATH "/bayonne");
        el = strlen(path);
        dir.open(path);
        while(is(dir) && dir.read(buffer, sizeof(buffer)) > 0) {
            ep = strrchr(buffer, '.');
            if(!ep || !eq_case(ep, DLL_SUFFIX))
                continue;
            snprintf(path + el, sizeof(path) - el, "/%s", buffer);
            shell::log(shell::INFO, "loading %s", buffer);
            if(fsys::load(path))
                shell::log(shell::ERR, "failed loading %s", path);
        }
        dir.close();
    }
    else {
        String::set(buffer, sizeof(buffer), list);
        while(NULL != (cp = String::token(buffer, &tp, ", ;:\r\n"))) {
            String::set(path, sizeof(path), argv0);
            ep = strstr(path, LIB_PREFIX);
            if(ep) {
                *ep = 0;
                el = strlen(path);
                snprintf(path + el, sizeof(path) - el, "../modules/%s/%s%s",
                    LIB_PREFIX, cp, DLL_SUFFIX);
                if(fsys::is_file(path)) {
                    shell::log(shell::INFO, "loading %s" DLL_SUFFIX " locally", cp);
                    goto loading;
                }
            }
            snprintf(path, sizeof(path), "%s/%s%s",
                DEFAULT_LIBPATH "/bayonne", cp, DLL_SUFFIX);
            shell::log(shell::INFO, "loading %s " DLL_SUFFIX, cp);
loading:
            if(fsys::load(path))
                shell::log(shell::ERR, "failed loading %s", path);
        }
    }
}


