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

#include "common.h"

namespace bayonne {

#ifdef HAVE_SIGWAIT

psignals psignals::thread;

psignals::psignals() :
JoinableThread()
{
    shutdown = started = false;
}

psignals::~psignals()
{
    if(!shutdown)
        cancel();
}

void psignals::cancel(void)
{
    if(started) {
        shutdown = true;
#ifdef  __FreeBSD__
        raise(SIGINT);
#endif
        pthread_kill(tid, SIGALRM);
        join();
    }
}

void psignals::run(void)
{
    int signo;
    unsigned period = 900;

    started = true;
    shell::log(shell::DEBUG0, "starting signal handler");

    for(;;) {
        alarm(period);
#ifdef  HAVE_SIGWAIT2
        sigwait(&sigs, &signo);
#else
        signo = sigwait(&sigs);
#endif
        alarm(0);
        if(shutdown)
            return;

        shell::log(shell::DEBUG0, "received signal %d", signo);

        switch(signo) {
        case SIGALRM:
            server::control("timer");
            break;
        case SIGINT:
        case SIGTERM:
            server::control("down");
            break;
        case SIGUSR1:
            server::control("snapshot");
            break;
        case SIGHUP:
            server::control("reload");
            break;
        default:
            break;
        }
    }
    shell::log(shell::DEBUG0, "stopping signal handler");
}

void psignals::service(const char *name)
{
}

void psignals::setup(void)
{
    sigemptyset(&thread.sigs);
    sigaddset(&thread.sigs, SIGALRM);
    sigaddset(&thread.sigs, SIGHUP);
    sigaddset(&thread.sigs, SIGINT);
    sigaddset(&thread.sigs, SIGTERM);
    sigaddset(&thread.sigs, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &thread.sigs, NULL);

    signal(SIGPIPE, SIG_IGN);
}

void psignals::start(void)
{
    thread.background();
}

void psignals::stop(void)
{
    thread.cancel();
}

#elif defined(WIN32)

static SERVICE_STATUS_HANDLE hStatus = 0;
static SERVICE_STATUS status;

static void WINAPI handler(DWORD sigint)
{
    switch(sigint) {
    case 128:
        // control::request("reload");
        return;
    case 129:
        // control::request("snapshot");
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwWin32ExitCode = 0;
        status.dwCheckPoint = 0;
        status.dwWaitHint = 6000;
        SetServiceStatus(hStatus, &status);
        // control::request("down");
        break;
    default:
        break;
    }
}

void psignals::service(const char *name)
{
    memset(&status, 0, sizeof(SERVICE_STATUS));
    status.dwServiceType = SERVICE_WIN32;
    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
    hStatus = ::RegisterServiceCtrlHandler(name, &handler);
}

void psignals::setup(void)
{
}

void psignals::start(void)
{
    if(!hStatus)
        return;

    status.dwCurrentState = SERVICE_RUNNING;
    ::SetServiceStatus(hStatus, &status);
}

void psignals::stop(void)
{
    if(!hStatus)
        return;

    status.dwCurrentState = SERVICE_STOPPED;
    ::SetServiceStatus(hStatus, &status);
}

#else

void psignals::service(const char *name)
{
}

void psignals::setup(void)
{
}

void psignals::start(void)
{
}

void psignals::stop(void)
{
}

#endif

} // end namespace

