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

#include "driver.h"

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

static bool shutdown_flag = false;
static unsigned startup_count = 0;
static unsigned shutdown_count = 0;
static media *instance;

bool media::symmetric = false;
size_t media::buffer = 2048;
unsigned media::jitter = RTP_DEFAULT_JITTER_TIME;
const char *media::address = "0.0.0.0";

media::media(size_t size) : DetachedThread(size)
{
    waiting = session_set_new();
    pending = session_set_new();
    sessions = 0;
    instance = this;
}

void media::run(void)
{
    ++startup_count;
    shell::log(DEBUG1, "starting media thread");
    
    while(!shutdown_flag) {
        lock.acquire();
        session_set_copy(waiting, pending);
        if(sessions > 0) { 
            lock.release();
            events = session_set_select(waiting, NULL, NULL);
        }
        else {
            lock.release();
            events = 0;
            Thread::sleep(50);
        }
    }
    shell::log(DEBUG1, "stopping media thread");
    ++shutdown_count;
}

void media::shutdown(void)
{
    shutdown_flag = true;
    ortp_exit();
    while(shutdown_count < startup_count)
        Thread::sleep(50);
}

void media::attach(timeslot *ts, const char *connect, unsigned port)
{
    if(ts->session)
        return;

    RtpSession *s = rtp_session_new(RTP_SESSION_SENDRECV);
    
    rtp_session_set_scheduling_mode(s, 1);
    rtp_session_set_blocking_mode(s, 0);

#ifdef  ORTP_API1
    rtp_session_set_local_addr(s, address, ts->media_port);
#else
    rtp_session_set_local_addr(s, address, ts->media_port, ts->media_port + 1);
#endif
    rtp_session_set_remote_addr(s, connect, port);

    if(symmetric)
        rtp_session_set_symmetric_rtp(s, TRUE);
    else
        rtp_session_set_symmetric_rtp(s, FALSE);

    if(jitter) {
        rtp_session_enable_adaptive_jitter_compensation(s, TRUE);
        rtp_session_set_jitter_compensation(s, jitter);
    }
    else
        rtp_session_enable_adaptive_jitter_compensation(s, FALSE);

    rtp_session_set_recv_buf_size(s, buffer); // default is 64k...
    rtp_session_set_connected_mode(s, TRUE);

    instance->lock.acquire();
    ++instance->sessions;
    session_set_set(instance->pending, s);
    instance->lock.release();
    ts->session = s;
}

void media::release(timeslot *ts)
{
    if(!ts->session)
        return;

    instance->lock.acquire();
    --instance->sessions;
    session_set_clr(instance->pending, ts->session);
    instance->lock.release();

    rtp_session_destroy(ts->session);
	ts->session = NULL;
}

