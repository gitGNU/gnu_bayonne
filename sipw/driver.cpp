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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

voip::context_t driver::tcp_context = NULL;
voip::context_t driver::udp_context = NULL;
voip::context_t driver::tls_context = NULL;
voip::context_t driver::out_context = NULL;
int driver::family = AF_INET;
int driver::protocol = IPPROTO_UDP;

static const char *sip_realm = NULL;
static const char *sip_schema = "sip:";
static unsigned expires = 300;
static const char *iface = NULL;
static bool started = false;
static unsigned registries = 40;
static unsigned short port = 5060;

static shell::groupopt driveropts("Driver Options");
#ifdef  AF_INET6
static shell::flagopt ipv6('6', "--ipv6", "default to ipv6");
#endif
static shell::numericopt portopt(0, "--port", "port to bind (5060)", "port", 5060);
static shell::stringopt realmopt(0, "--realm", "realm of server", "string", NULL);
static shell::flagopt tcp(0, "--tcp", "default to sip over tcp");
static shell::numericopt slots('t', "--timeslots", "number of timeslots to allocate", "ports", 16);

driver::driver() :
Driver("registry")
{
    udp_context = tcp_context = tls_context = out_context = NULL;
}

Driver *driver::create(void)
{
    return new driver();
}

registration *driver::contact(const char *uuid)
{
    assert(uuid != NULL && *uuid != 0);

    linked_pointer<registration> rp = registrations;

    while(is(rp)) {
        if(eq(rp->getUUID(), uuid))
            return *rp;
        rp.next();
    }
    return NULL;
}

registration *driver::locate(const char *id)
{
    assert(id != NULL && *id != 0);

    linked_pointer<registration> rp = registrations;

    while(is(rp)) {
        if(eq(rp->getId(), id))
            return *rp;
        rp.next();
    }
    return NULL;
}

registration *driver::locate(int rid)
{
    assert(rid != -1);

    linked_pointer<registration> rp = registrations;

    while(is(rp)) {
        if(rp->getRegistry() == rid)
            return *rp;
        rp.next();
    }
    shell::log(shell::WARN, "unknown registry id %d requested", rid);
    return NULL;
}

const char *driver::realm(void)
{
    if(is(realmopt))
        return *realmopt;

    if(sip_realm && *sip_realm)
        return sip_realm;

    return NULL;
}

void driver::update(void)
{
    keydata *keys = keyfile::get("sip");
    const char *new_realm = NULL;

    if(!keys)
        keys = keyfile::get("sips");

    linked_pointer<keydata::keyvalue> kv;
    if(keys)
        kv = keys->begin();

    while(is(kv)) {
        if(eq(kv->id, "timing"))
            background::schedule(atol(kv->value));
        else if(eq(kv->id, "stepping"))
            stepping = atol(kv->value);
        else if(eq(kv->id, "realm")) {
            if(eq(kv->value, sip_realm))
                new_realm = sip_realm;
            else
                new_realm = memcopy(kv->value);
        }
        kv.next();
    }

    sip_realm = new_realm;
    Driver::update();
}

void driver::start(void)
{
    const char *agent = "bayonne-" VERSION "/exosip2";
    const char *err = NULL, *id;
    char buffer[256];
    size_t len;
    size_t stack = 0;
    unsigned priority = 1;

    ts_count = 16;      // default if not modified...
    ts_alloc = sizeof(timeslot);

    Driver *drv = Driver::get();
    keydata *keys = drv->keyfile::get("sip");
    linked_pointer<keydata::keyvalue> kv;

    if(keys)
        kv = keys->begin();

    while(is(kv)) {
        if(eq(kv->id, "iface")) {
            iface = kv->value;
#ifdef  AF_INET6
            if(strchr(iface, ':'))
                family = AF_INET6;
#endif
            if(eq(iface, ":::") || eq(iface, "::0") || eq(iface, "*") || iface[0] == 0)
                iface = NULL;
            else
                iface = memcopy(iface);
        } else if(eq(kv->id, "protocol") || eq(kv->id, "transport")) {
            if(eq(kv->value, "tcp"))
                protocol = IPPROTO_TCP;                
        } else if(eq(kv->id, "agent"))
            agent = memcopy(kv->value);
        else if(eq(kv->id, "port"))
            port = atoi(kv->value);
        else if(eq(kv->id, "stack"))
            stack = atoi(kv->value) * 1024;
        else if(eq(kv->id, "expires"))
            expires = atoi(kv->value);
        else if(eq(kv->id, "priority"))
            priority = atoi(kv->value);
        else if(eq(kv->id, "sessions"))
            ts_count = atoi(kv->value);
        else if(eq(kv->id, "timeslots"))
            ts_count = atoi(kv->value);
        else if(eq(kv->id, "registries"))
            registries = atoi(kv->value);
        else if(eq(kv->id, "realm"))
            sip_realm = memcopy(kv->value);
        kv.next();
    }

    if(is(slots))
        ts_count = *slots;

    if(is(portopt))
        port = *portopt;

    if(is(tcp))
        protocol = IPPROTO_TCP;

#ifdef  AF_INET6
    if(is(ipv6))
        family = AF_INET6;
#endif

    stats = statmap::create(registries);
    len = strlen(buffer);
    snprintf(buffer + len, sizeof(buffer) - len, ":%u", port);


    Socket::query(family);

    if(protocol == IPPROTO_TCP) {
        voip::create(&tcp_context, agent, family);
        voip::create(&udp_context, agent, family);
        out_context = tcp_context;
    }
    else {
        voip::create(&udp_context, agent, family);
        voip::create(&tcp_context, agent, family);
        out_context = udp_context;
    }

#ifdef  HAVE_OPENSSL
    voip::create(&tls_context, agent, family);
#endif

    if(udp_context) {
        if(!voip::listen(udp_context, IPPROTO_UDP, iface, port))
            shell::log(shell::FAIL, "cannot listen port %u for udp", port);
        else
            shell::log(shell::NOTIFY, "listening port %u for udp", port);
    }

    if(tcp_context) {
        if(!voip::listen(tcp_context, IPPROTO_TCP, iface, port))
            shell::log(shell::FAIL, "cannot listen port %u for tcp", port);
        else
            shell::log(shell::NOTIFY, "listening port %u for tcp", port);
    }

    if(tls_context) {
        if(!voip::listen(tls_context, IPPROTO_TCP, iface, port, true))
            shell::log(shell::FAIL, "cannot listen port %u for tls", port + 1);
        else
            shell::log(shell::NOTIFY, "listening port %u for tls", port + 1);
    }

    osip_trace_initialize_syslog(TRACE_LEVEL0, (char *)"bayonne");

    timeslots = (caddr_t)new timeslot[ts_count];

    Driver::start();
    thread::activate(priority, stack);

    started = true;
    len = strlen(sip_schema);

    linked_pointer<keydata> kp = drv->registry();

    keydata *regkeys = drv->keyfile::get("registry");
    if(regkeys) {
        err = activate(regkeys);
        if(err)
            shell::log(shell::ERR, "registering registry, %s", err);
    }

    while(is(kp)) {
        err = NULL;
        id = kp->get();
        err = activate(*kp);
        if(err)
            shell::log(shell::ERR, "registering %s, %s", id, err);
        ++kp;
    }

    Driver::release(drv);
}

void driver::stop(void)
{
    linked_pointer<registration> rp = registrations;

    while(is(rp)) {
        rp->release();
        rp.next();
    }

    thread::shutdown();
    Driver::stop();
}

const char *driver::dispatch(char **argv, int pid)
{
    return Driver::dispatch(argv, pid);
}

const char *driver::activate(keydata *keys)
{
    caddr_t mp = memget(sizeof(registration));
    registration *reg = new(mp) registration(&registrations, keys, expires, port);
    shell::log(shell::INFO, "registering with %s", reg->getServer());

    if(reg->getRegistry() != -1)
        return NULL;

    return "failed registration";
}

static void init(int argc, char **argv, bool detached, shell::mainproc_t svc = NULL)
{
    static Script::keyword_t keywords[] = {
        {NULL}};

    server::parse(argc, argv, "sip");
    server::startup(svc, detached);

    Script::assign(keywords);   // bind local driver keywords if any...

    Driver::commit(new driver());

    driver::start();
    server::dispatch();
    driver::stop();

    server::release();
}

static SERVICE_MAIN(main, argc, argv)
{
    signals::service("bayonne");
    init(argc, argv, true);
}

PROGRAM_MAIN(argc, argv)
{
    init(argc, argv, false, &service_main);
    PROGRAM_EXIT(0);
}

