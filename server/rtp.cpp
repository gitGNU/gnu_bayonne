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

#ifdef  HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#ifdef  HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

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

typedef struct {
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char event:8;
    unsigned char ending:1;
    unsigned char reserved:1;
    unsigned char volume:6;
    uint16_t duration;
#else
    unsigned char event:8;
    unsigned char volume:6;
    unsigned char ending:1;
    unsigned char reserved:1;
    uint16_t duration;
#endif
} rfc2833_t;

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

void RTPTimeslot::send(void *address, size_t len)
{
    rtp_header_t *sending = (rtp_header_t*)rtp_sending;
    uint16_t sequence = ntohs(sending->sequence);
    ssize_t result;
#ifdef  HAVE_SYS_UIO_H
    struct msghdr msg;
    struct iovec iov[2];
    iov[0].iov_base = sending;
    iov[0].iov_len = 12;
    iov[1].iov_base = address;
    iov[1].iov_len = len;

    msg.msg_name = (void*)&rtp_contact;
    msg.msg_namelen = rtp_addrlen;
    msg.msg_iov = &iov[0];
    msg.msg_iovlen = 2;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    result = ::sendmsg(rtp, &msg, 0);
#else
    memcpy(rtp_sending + 12, address, len);
    result = ::write(rtp, rtp_sending, len + 12);
#endif
    if(result >= 12)
        sending->sequence = htons(++sequence);
}

void RTPTimeslot::run(void)
{
    Timer syncup;
    char buf[16];
    unsigned silence = 0;
    rtp_header_t *sending = (rtp_header_t*)rtp_sending;
    rtp_header_t *rfc2833 = (rtp_header_t*)rtp_rfc2833;
    rtp_header_t *index[3] = {NULL, NULL, NULL};
    rtp_header_t *receive;
    unsigned rtp_count = 0, rtp_index = 0;
    struct sockaddr_storage origin;
    socklen_t olen;
    ssize_t len;
    unsigned pos, last;
    bool rtp_in, rtcp_in;

    snprintf(buf, sizeof(buf), "%u", rtp_port);
    rtp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);
    snprintf(buf, sizeof(buf), "%u", rtp_port + 1);
    rtcp = Socket::create(rtp_address, buf, rtp_family, SOCK_DGRAM);

    Random::fill((uint8_t *)&sending->sequence, 2);
    Random::fill((uint8_t *)&sending->sources[0], 4);

    sending->timestamp = 0;
    sending->cc = rfc2833->cc = 0;
    sending->marker = rfc2833->marker = 0;
    sending->padding = rfc2833->padding = 0;
    sending->version = rfc2833->version = 2;

    rfc2833->sources[0] = sending->sources[0];
    prior_rfc2833 = 1;  // impossible value, guarantees no initial match

    syncup.set(rtp_slice);
    for(;;) {
        if(!rtp_slice) {
            ::close(rtp);
            ::close(rtcp);
            return;
        }

#ifdef  HAVE_SYS_POLL_H
        struct pollfd pfd[2];
        pfd[0].fd = rtp;
        pfd[1].fd = rtcp;
        pfd[0].events = pfd[1].events = POLLIN | POLLRDNORM;
        pfd[0].revents = pfd[1].revents = 0;
        poll(pfd, 2, *syncup);
        rtp_in = pfd[0].revents & POLLRDNORM;
        rtcp_in = pfd[1].revents & POLLRDNORM;
#else
        fd_set inp;
        struct timeval timeout;
        int maxfd = rtp + 1;
        if((SOCKET)maxfd <= rtcp)
            maxfd = rtcp + 1;
        FD_ZERO(&inp);
        FD_SET(rtp, &inp);
        FD_SET(rtcp, &inp);
        timeout.tv_sec = 0;
        timeout.tv_usec = *syncup * 1000l;
        select(maxfd, &inp, NULL, NULL, &timeout);
        rtp_in = rtcp_in = false;
        if(FD_ISSET(rtp, &inp))
            rtp_in = true;
        if(FD_ISSET(rtcp, &inp))
            rtcp_in = true;
#endif

        // see if input...
        if(rtp_in) {
            receive = (rtp_header_t *)&rtp_receive[rtp_index];
            olen = sizeof(struct sockaddr_storage);
            len = recvfrom(rtp, (char *)receive,
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
                if(ntohl(receive->timestamp) < ntohl(index[pos]->timestamp))
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
            memcpy(&rtp_contact, &origin, olen);
            rtp_addrlen = olen;
            silence = 0;
        }

        // see if rtcp...
        if(rtcp_in) {

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

void RTPTimeslot::create2833(unsigned event)
{
    rtp_header_t *sending = (rtp_header_t*)rtp_sending;
    rtp_header_t *rfc2833 = (rtp_header_t*)rtp_rfc2833;
    rfc2833_t *data = (rfc2833_t *)rtp_rfc2833 + 12;

    rfc2833->timestamp = sending->timestamp;
    data->duration = 0;
    data->ending = 0;
    data->reserved = 0;
    data->event = event;
    send2833(false);
}

void RTPTimeslot::send2833(bool end)
{
    rtp_header_t *sending = (rtp_header_t*)rtp_sending;
    rtp_header_t *rfc2833 = (rtp_header_t*)rtp_rfc2833;
    rfc2833_t *data = (rfc2833_t *)rtp_rfc2833 + 12;
    uint16_t sequence = ntohs(sending->sequence);
    uint16_t duration = ntohs(data->duration);

    rfc2833->sequence = htons(sequence++);
    sending->sequence = htons(sequence);

    if(!data->ending) {
        duration += rtp_samples;
        data->duration = htons(duration);
    }

    data->ending = end;
    ::sendto(rtp, (const char *)rfc2833, 16, 0, (struct sockaddr *)&rtp_contact, rtp_addrlen);
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

