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
#include <ucommon/secure.h>
#include <sys/poll.h>

#ifdef  HAVE_ENDIAN_H
#include <endian.h>
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER 1234
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

#pragma pack(1)
typedef struct {
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char version:2;
    unsigned char padding:1;
    unsigned char extension:1;
    unsigned char cc:4;
    unsigned char marker:1;
    unsigned char payload:7;
#else
    unsigned char cc:4;
    unsigned char extension:1;
    unsigned char padding:1;
    unsigned char version:2;
    unsigned char payload:7;
    unsigned char marker:1;
#endif
    uint16_t sequence;
    uint32_t timestamp;
    uint32_t sources[1];
} rtp_header_t;
#pragma pack()

void *rtp_data(rtp_header_t *header)
{
    char *data = (char *)header;
    unsigned len = 12 + (header->cc * 4);
    return data + len;
}

RTPTimeslot::RTPTimeslot(const char *address, unsigned short port, int family) :
Timeslot(NULL), JoinableThread()
{
    rtp_address = Driver::dup(address);
    rtp_family = family;
    rtp_port = port;
    rtp_slice = 0;
    rtp_priority = 1;
    rtp_samples = 160;
}

void RTPTimeslot::run(void)
{
    Timer syncup;
    struct pollfd pfd[2];
    char buf[16];
    unsigned silence = 0;
    rtp_header_t *sending = (rtp_header_t*)rtp_sending;
    rtp_header_t *index[3] = {NULL, NULL, NULL};
    rtp_header_t *receive;
    unsigned rtp_count = 0, rtp_index = 0;
    struct sockaddr_storage origin;
    socklen_t olen;
    ssize_t len;
    unsigned pos, last;

    snprintf(buf, sizeof(buf), "%u", rtp_port);
    rtp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);
    snprintf(buf, sizeof(buf), "%u", rtp_port + 1);
    rtcp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);

    Random::fill((uint8_t *)&sending->sequence, 2);
    Random::fill((uint8_t *)&sending->sources[0], 4);

    sending->timestamp = 0;
    sending->cc = 0;
    sending->marker = 0;
    sending->padding = 0;
    sending->version = 2;

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

        // see if input...
        if(pfd[0].revents & POLLRDNORM) {
            receive = (rtp_header_t *)&rtp_receive[rtp_index];
            olen = sizeof(struct sockaddr_storage);
            len = recvfrom(rtp, (void *)receive,
                480 + 72, 0, (struct sockaddr *)&origin, &olen);
            // if error or some kind of keep-alive, we ignore...
            if(len < 12)
                continue;

            // confirm origin and source id...

            // process first packet in jitter buffer if already 3 pending...
            pos = last = 0;
            if(rtp_count == RTP_BUFFER_SIZE) {
                last = rtp_count;
                while(++last < RTP_BUFFER_SIZE)
                    index[last - 1] = index[last];
                rtp_count = --last;
            }

            // check where in existing list we are...
            while(pos < rtp_count) {
                if(ntohs(receive->sequence) < ntohs(index[pos]->sequence))
                    break;
                ++pos;
            }

            // reorder by sequence if needed...
            last = rtp_count;
            while(last-- > pos)
                index[last + 1] = index[last];

            index[pos] = receive;
            ++rtp_count;
            if(++rtp_index >= RTP_BUFFER_SIZE)
                rtp_index = 0;
            silence = 0;
        }

        // see if rtcp...
        if(pfd[1].revents & POLLRDNORM) {

        }

        if(*syncup == 0) {
            // if no input during last two timeslices, we have input reset
            if(silence > 1) {

                silence = 0;
            }
            // process timeout...

            // update state
            ++silence;
            syncup += rtp_slice;
            sending->timestamp = htonl(ntohl(sending->timestamp) + rtp_samples);
        }
    }
}

void RTPTimeslot::startup(void)
{
    if(inUse)
        return;

    shell::debug(4, "starting timeslot %d", tsid);
    start(rtp_priority);
    Timeslot::startup();
}

void RTPTimeslot::shutdown(void)
{
    if(!inUse)
        return;

    shell::debug(4, "stopping timeslot %d", tsid);
    lock();
    join();
    unlock();
    Timeslot::shutdown();
}

