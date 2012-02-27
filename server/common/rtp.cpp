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
#include <sys/poll.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

RTPTimeslot::RTPTimeslot(const char *address, unsigned short port, int family) :
Timeslot(NULL), JoinableThread()
{
    rtp_address = Driver::dup(address);
    rtp_family = family;
    rtp_port = port;
    rtp_slice = 0;
}

void RTPTimeslot::run(void)
{
    Timer syncup;
    struct pollfd pfd[2];
    char buf[16];

    snprintf(buf, sizeof(buf), "%u", rtp_port);
    rtp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);
    snprintf(buf, sizeof(buf), "%u", rtp_port + 1);
    rtcp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);

    syncup.set(rtp_slice);
    for(;;) {
        if(!rtp_slice) {
            ::close(rtp);
            ::close(rtcp);
            return;
        }

        pfd[0].fd = rtp;
        pfd[1].fd = rtcp;
        pfd[0].events = pfd[1].events = POLLIN | POLLRDNORM;
        pfd[0].revents = pfd[1].revents = 0;
        poll(pfd, 2, *syncup);
        if(*syncup == 0) {
            syncup += rtp_slice;
        }
    }
}

void RTPTimeslot::shutdown(void)
{
    lock();
    join();
    unlock();
}

