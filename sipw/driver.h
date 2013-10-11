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

#include <bayonne/voip.h>
#include <ucommon/secure.h>
#include <bayonne-config.h>

#ifndef SESSION_EXPIRES
#define SESSION_EXPIRES "session-expires"
#endif

#ifndef ALLOW_EVENTS
#define ALLOW_EVENTS    "allow-events"
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class __LOCAL registration : public Registration
{
protected:
    registration *fwd;          // forwarding on replacement...
    unsigned expires;
    const char *userid;
    const char *secret;
    const char *server;
    const char *script;         // default script...
    const char *uri, *contact;
    const char *targets;        // To: destinations allowed...
    const char *localnames;     // hostnames we recognize as local
    char uuid[38];
    voip::context_t context;
    voip::reg_t rid;

public:
    registration(LinkedObject **list, keydata *keys, unsigned expiration, unsigned port);

    void release(void);
    void cancel(void);
    void failed(void);
    void confirm(void);
    void authenticate(const char *realm);

    inline voip::context_t getContext(void)
        {return context;}

    inline int getRegistry(void)
        {return rid;}

    inline const char *getUUID(void)
        {return uuid;}

    inline const char *getScript(void)
        {return script;}

    inline const char *getServer(void)
        {return server;}

    inline const char *getTargets(void)
        {return targets;};

    inline const char *getLocalnames(void)
        {return localnames;}
};

class __LOCAL timeslot : public Timeslot
{
public:
    timeslot();

    int incoming(voip::event_t sevent);

    timeout_t getExpires(time_t now);

private:
    voip::context_t ctx;
    voip::did_t did;
    voip::tid_t tid;
    Timer timer;

    void disarm(void);
    void arm(timeout_t timeout);
    void drop(void);
    void allocate(long cid, statmap::stat_t stat, Registration *reg);
    void disconnect(event_t *event);
    void hangup(event_t *event);
    void release(event_t *event);
    void disable(event_t *event);
};

class __LOCAL driver : public Driver
{
public:
    driver();

    Driver *create(void);
    void update(void);
    const char *dispatch(char **argv, int pid);

    static voip::context_t out_context;   // default output context
    static voip::context_t udp_context;
    static voip::context_t tcp_context;
    static voip::context_t tls_context;
    static int family, protocol;

    static registration *contact(const char *uuid);
    static registration *locate(int regid);
    static registration *locate(const char *id);
    static const char *activate(keydata *keys);
    static const char *realm(void);
    static void start(void);
    static void stop(void);
};

class __LOCAL thread : public DetachedThread
{
private:
    const char *instance;
    voip::context_t context;
    voip::event_t sevent;
    registration *registry;
    char buffer[256];

    thread();

    void options(void);
    void invite(void);
    void run(void);

public:
    thread(voip::context_t source, size_t stack, const char *type);

    static void activate(int priority, size_t stack);
    static void shutdown(void);
};

class __LOCAL background : public Background
{
public:
    background(size_t stack);

private:
    void automatic(void);
};

class __LOCAL srv : public uri
{
public:
    srv(const char *id);
    srv();

    voip::context_t route(char *buf, size_t size, const char *uri);
};

END_NAMESPACE
