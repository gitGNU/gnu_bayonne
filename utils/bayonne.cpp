// Copyright (C) 2008-2010 David Sugar, Tycho Softworks.
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
#include <bayonne.h>
#ifdef _MSWINDOWS_
#include <windows.h>
#include <io.h>
#else
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static void capture(void)
{
#ifndef _MSWINDOWS_
    char buffer[512];
    FILE *fp;

    snprintf(buffer, sizeof(buffer), "/tmp/.bayonne.%ld", (long)getpid());
    fp = fopen(buffer, "r");
    remove(buffer);
    while(fp && fgets(buffer, sizeof(buffer), fp) != NULL)
        fputs(buffer, stdout);
    if(fp)
        fclose(fp);
#endif
}

static void command(char **argv, unsigned timeout)
{
    char buffer[512];
    size_t len;
    fd_t fd;

#ifdef  _MSWINDOWS_
    snprintf(buffer, sizeof(buffer), "\\\\.\\mailslot\\bayonne_ctrl");
    fd = CreateFile(buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    sigset_t sigs;
    int signo;
    struct passwd *pwd = getpwuid(getuid());

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGUSR1);
    sigaddset(&sigs, SIGUSR2);
    sigaddset(&sigs, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    fd = ::open(DEFAULT_VARPATH "/run/bayonne/control", O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        if(!pwd)
            shell::errexit(4, "*** bayonne: events: invalid login\n");

        snprintf(buffer, sizeof(buffer), "/tmp/bayonne-%s/control", pwd->pw_name);
        fd = ::open(buffer, O_WRONLY | O_NONBLOCK);
    }
#endif

    if(fd == INVALID_HANDLE_VALUE)
        shell::errexit(10, "*** bayonne: command: offline\n");

#ifndef _MSWINDOWS_
    if(timeout)
        snprintf(buffer, sizeof(buffer), "%ld", (long)getpid());
    else
#endif
        buffer[0] = 0;

    while(*argv) {
        len = strlen(buffer);
        snprintf(buffer + len, sizeof(buffer) - len - 1, " %s", *(argv++));
    }

#ifdef  _MSWINDOWS_
    if(!WriteFile(fd, buffer, (DWORD)strlen(buffer) + 1, NULL, NULL))
        shell::errexit(11, "*** bayonne: control failed\n");
#else
    len = strlen(buffer);
    buffer[len++] = '\n';
    buffer[len] = 0;

    if(::write(fd, buffer, len) < (int)len)
        shell::errexit(11, "*** bayonne: control failed\n");

    if(!timeout)
        exit(0);

    alarm(timeout);
#ifdef  HAVE_SIGWAIT2
    sigwait(&sigs, &signo);
#else
    signo = sigwait(&sigs);
#endif
    if(signo == SIGUSR1) {
        capture();
        exit(0);
    }
    if(signo == SIGALRM)
        shell::errexit(12, "*** bayonne: command: timed out\n");

    shell::errexit(20, "*** bayonne: command: request failed\n");
#endif
}

static void version(void)
{
    printf("Bayonne " VERSION "\n"
        "Copyright (C) 2007,2008,2009 David Sugar, Tycho Softworks\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

static void usage(void)
{
    printf("usage: bayonne command\n"
        "Commands:\n"
        "  abort                    Force daemon abort\n"
        "  check                    Server deadlock check\n"
        "  concurrency <level>      Server concurrency level\n"
        "  down                     Shut down server\n"
        "  reload                   Reload configuration\n"
        "  restart                  Server restart\n"
        "  snapshot                 Server snapshot\n"
        "  status                   Dump status string\n"
        "  verbose <level>          Server verbose logging level\n"
    );

    printf("Report bugs to sipwitch-devel@gnu.org\n");
    exit(0);
}

static void single(char **argv, int timeout)
{
    if(argv[1])
        shell::errexit(1, "*** bayonne: %s: too many arguments\n", *argv);

    command(argv, timeout);
}

static void level(char **argv, int timeout)
{
    if(!argv[1])
        shell::errexit(1, "*** bayonne: %s: level missing\n", *argv);

    if(argv[2])
        shell::errexit(1, "*** bayonne: %s: too many arguments\n", *argv);

    command(argv, timeout);
}

PROGRAM_MAIN(argc, argv)
{
    if(argc < 2)
        usage();

    ++argv;
    if(eq(*argv, "version") || eq(*argv, "-version") || eq(*argv, "--version"))
        version();
    else if(eq(*argv, "help") || eq(*argv, "-help") || eq(*argv, "--help"))
        usage();
    else if(eq(*argv, "reload") || eq(*argv, "check") || eq(*argv, "snapshot") || eq(*argv, "dump"))
        single(argv, 30);
    else if(eq(*argv, "down") || eq(*argv, "restart") || eq(*argv, "abort"))
        single(argv, 0);
    else if(eq(*argv, "verbose") || eq(*argv, "concurrency"))
        level(argv, 10);

    if(!argv[1])
        shell::errexit(1, "use: bayonne command [arguments...]\n");
    else
        shell::errexit(1, "*** bayonne: %s: unknown command or option\n", argv[0]);
    PROGRAM_EXIT(1);
}

