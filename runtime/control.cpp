// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
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

#include "../config.h"
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static const char *replytarget = NULL;

#ifndef _MSWINDOWS_

#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <limits.h>
#include <pwd.h>

static FILE *fifo = NULL;
static char fifopath[128] = "";

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

size_t Control::create(void)
{
    String::set(fifopath, sizeof(fifopath), env("control"));
    remove(fifopath);
    if(mkfifo(fifopath, 0660)) {
        fifopath[0] = 0;
        return 0;
    }
    else
        shell::exiting(&cleanup);

    fifo = fopen(fifopath, "r+");
    if(fifo)
        return 512;
    fifopath[0] = 0;
    return 0;
}

void Control::release(void)
{
    shell::log(shell::INFO, "shutdown");
    cleanup();
}

char *Control::receive(void)
{
    static char buf[512];
    char *cp;

    if(!fifo)
        return NULL;

    reply(NULL);

retry:
    buf[0] = 0;
    if(fgets(buf, sizeof(buf), fifo) == NULL) {
        buf[0] = 0;
        Thread::sleep(100); // throttle if dead...
    }
    cp = String::strip(buf, " \t\r\n");
    if(*cp == '/') {
        if(strstr(cp, ".."))
            goto retry;

        if(strncmp(cp, "/tmp/.reply.", 12))
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

#else

static HANDLE hFifo = INVALID_HANDLE_VALUE;
static HANDLE hLoopback = INVALID_HANDLE_VALUE;
static HANDLE hEvent = INVALID_HANDLE_VALUE;
static OVERLAPPED ovFifo;

static void cleanup(void)
{
    if(hFifo != INVALID_HANDLE_VALUE) {
        CloseHandle(hFifo);
        CloseHandle(hLoopback);
        CloseHandle(hEvent);
        hFifo = hLoopback = hEvent = INVALID_HANDLE_VALUE;
    }
}

size_t Control::create(void)
{
    char buf[64];

    String::set(buf, sizeof(buf), env("control"));
    hFifo = CreateMailslot(buf, 0, MAILSLOT_WAIT_FOREVER, NULL);
    if(hFifo == INVALID_HANDLE_VALUE)
        return 0;

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hLoopback = CreateFile(buf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ovFifo.Offset = 0;
    ovFifo.OffsetHigh = 0;
    ovFifo.hEvent = hEvent;
    shell::exiting(&cleanup);
    return 464;
}

char *Control::receive(void)
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

        if(strncmp(cp, "\\\\.\\mailslot\\", 14))
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
        if(stricmp(lp, "_temp"))
            goto retry;
    }
    return cp;
}

void Control::release(void)
{
    shell::log(shell::INFO, "shutdown");
    cleanup();
}

#endif

void Control::reply(const char *msg)
{
    assert(msg == NULL || *msg != 0);

    char *sid;
    fsys fd;
    char buffer[256];

    if(msg)
        shell::log(shell::ERR, "control failed; %s", msg);

    if(!replytarget)
        return;

    if(isdigit(*replytarget)) {
#ifndef _MSWINDOWS_
        pid_t pid = atoi(replytarget);
        if(msg)
            kill(pid, SIGUSR2);
        else
            kill(pid, SIGUSR1);
#endif
    }
    else {
        sid = (char *)strchr(replytarget, ';');
        if(sid)
            *(sid++) = 0;

        else
            sid = (char *)"-";
        if(msg)
            snprintf(buffer, sizeof(buffer), "%s msg %s\n", sid, msg);
        else
            snprintf(buffer, sizeof(buffer), "%s ok\n", sid);
        fd.open(replytarget, fsys::ACCESS_WRONLY);
        if(is(fd)) {
            fd.write(buffer, strlen(buffer));
            fd.close();
        }
    }
    replytarget = NULL;
}

bool Control::send(const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    char buf[512];
    fd_t fd;
    int len;
    bool rtn = true;
    va_list vargs;

    va_start(vargs, fmt);
#ifdef  _MSWINDOWS_
    fd = CreateFile(env("control"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(fd == INVALID_HANDLE_VALUE)
        return false;

#else
    fd = ::open(env("control"), O_WRONLY | O_NONBLOCK);
    if(fd < 0)
        return false;
#endif

    vsnprintf(buf, sizeof(buf) - 1, fmt, vargs);
    va_end(vargs);
    len = strlen(buf);
    if(buf[len - 1] != '\n')
        buf[len++] = '\n';
#ifdef  _MSWINDOWS_
    if(!WriteFile(fd, buf, (DWORD)strlen(buf) + 1, NULL, NULL))
        rtn = false;
    if(fd != hLoopback)
        CloseHandle(fd);
#else
    buf[len] = 0;
    if(::write(fd, buf, len) < len)
        rtn = false;
    ::close(fd);
#endif
    return rtn;
}

FILE *Control::output(const char *id)
{
#ifdef  _MSWINDOWS_
    if(!id)
        return NULL;

    return fopen(_STR(str(env("controls")) + "/" + id + ".out"), "w");
#else
    if(replytarget && isdigit(*replytarget))
        return fopen(str(env("reply")) + str((Unsigned)atol(replytarget)), "w");
    if(!id)
        return NULL;
    return fopen(_STR(str(env("controls")) + "/" + id), "w");
#endif
}

void Control::log(const char *fmt, ...)
{
    assert(fmt != NULL && *fmt != 0);

    fsys_t log;
    va_list args;
    char buf[1024];
    int len;
    char *cp;

    va_start(args, fmt);

    fsys::create(log, env("logfile"), fsys::ACCESS_APPEND, 0660);

    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    len = strlen(buf);
    if(buf[len - 1] != '\n')
        buf[len++] = '\n';

    if(is(log)) {
        fsys::write(log, buf, strlen(buf));
        fsys::close(log);
    }
    cp = strchr(buf, '\n');
    if(cp)
        *cp = 0;

    shell::debug(2, "logfile: %s", buf);
    va_end(args);
}


