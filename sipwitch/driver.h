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

#include <../server/server.h>
#include <ucommon/secure.h>
#include "voip.h"

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class __LOCAL registry : public Registry
{
public:
    int rid;
    unsigned expires;
    const char *contact;
    const char *userid;
    const char *secret;
    const char *digest;
    const char *server;
    const char *domain;
    const char *method;
    const char *realm;
    const char *uri;
    char uuid[38];
    bool active;
    sip_context_t context;

    registry(keydata *keyset, unsigned port = 5060, unsigned expiration = 120);

    void authenticate(const char *realm);

    void activate(void);

    void release(void);

    void shutdown(void);
};

class __LOCAL thread : public DetachedThread
{
private:
    unsigned instance;
    sip_context_t    context;
    sip_event_t      sevent;
    registry    *reg;

    void run(void);

public:
    thread(sip_context_t source, size_t size);

    static void shutdown();
};

class __LOCAL timeslot : public Timeslot
{
private:
    virtual void shutdown(void);

public:
    RtpSession *session;
    unsigned media_port;

    timeslot(unsigned port);
};

class __LOCAL media : public DetachedThread
{
private:
    friend class driver;

    static unsigned jitter;
    static size_t buffer;
    static const char *address;
    static bool symmetric;

    SessionSet *waiting, *pending;
    unsigned sessions;
    int events;
    Mutex lock;

    void run(void);

public:
    media(size_t size);

    static void shutdown();
    static void attach(timeslot *ts, const char *host, unsigned port);
    static void release(timeslot *ts);
};

class driver : public Driver
{
public:
    friend class registry;

    driver();

    int start(void);

    void stop(void);

    static sip_context_t out_context;   // default output context
    static sip_context_t udp_context;
    static sip_context_t tcp_context;
    static sip_context_t tls_context;

    static registry *locate(int rid);

};

END_NAMESPACE

