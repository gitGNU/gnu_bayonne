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

sip::context_t driver::tcp_context = NULL;
sip::context_t driver::udp_context = NULL;
sip::context_t driver::tls_context = NULL;
sip::context_t driver::out_context = NULL;

driver::driver() : Driver("sip", "registry")
{
    autotimer = 500;
    udp_context = tcp_context = tls_context = out_context = NULL;
}

int driver::start(void)
{
    linked_pointer<keydata> kp = keyserver.get("registry");
    const char *cp = NULL, *iface = NULL, *transport = NULL, *agent = NULL;
    unsigned sip_port = 5060, expires = 300, timeslots = 16;
    unsigned rtp_port = 0;
    bool tcp = false;
    int family = AF_INET;
    size_t stack = 0;
    unsigned priority = 0, mpri = 1;

    if(keys)
        cp = keys->get("port");
    else
        cp = NULL;

    if(cp)
        sip_port = atoi(cp);

    if(keys)
        cp = keys->get("jitter");
    else
        cp = NULL;

    if(cp)
        media::jitter = atoi(cp);


    if(keys)
        cp = keys->get("buffer");
    else
        cp = NULL;

    if(cp)
        media::buffer = atoi(cp) * 1024l;

    if(keys)
        cp = keys->get("expires");
    else
        cp = NULL;

    if(cp)
        expires = atoi(cp);

    if(keys)
        cp = keys->get("stack");
    else
        cp = NULL;

    if(cp)
        stack = atoi(cp);

    if(keys)
        cp = keys->get("priority");
    else
        cp = NULL;

    if(cp)
        priority = atoi(cp);

    if(keys)
        cp = keys->get("media");
    else
        cp = NULL;

    if(cp)
        mpri = atoi(cp);

    if(keys)
        iface = keys->get("interface");

    if(iface) {
#ifdef  AF_INET6
        if(strchr(iface, ':'))
            family = AF_INET6;
#endif
        if(eq(iface, "::*")) 
            iface = "::0";
        else if(eq(iface, "*"))
            iface = "0.0.0.0";
        media::address = iface;
    }

    if(keys)
        transport = keys->get("transport");
    if(keys && !transport)
        transport = keys->get("protocol");

    if(transport && eq(transport, "tcp"))
        tcp = true;

    if(keys)
        agent = keys->get("agent");

    if(!agent)
        agent = "bayonne-" VERSION "/exosip2";

    sip_port = sip_port & 0xfffe;

    ortp_init();
    ortp_scheduler_init();

    ortp_set_log_level_mask(ORTP_WARNING|ORTP_ERROR);
    ortp_set_log_file(stdout);

#if UCOMMON_ABI > 5
    Socket::query(family);
#else
    Socket::family(family);
#endif
    if(tcp) {
        sip::create(&tcp_context, agent, family);
        sip::create(&udp_context, agent, family);
        out_context = tcp_context;
    }
    else {
        sip::create(&udp_context, agent, family);
        sip::create(&tcp_context, agent, family);
        out_context = udp_context;
    }

#ifdef  HAVE_OPENSSL
    sip::create(&tls_context, agent, family);
#endif

    if(udp_context) {
        if(!sip::listen(udp_context, IPPROTO_UDP, iface, sip_port))
            shell::log(shell::FAIL, "cannot listen port %u for udp", sip_port);
        else
            shell::log(shell::NOTIFY, "listening port %u for udp", sip_port);
    }

    if(tcp_context) {
        if(!sip::listen(tcp_context, IPPROTO_TCP, iface, sip_port))
            shell::log(shell::FAIL, "cannot listen port %u for tcp", sip_port);
        else
            shell::log(shell::NOTIFY, "listening port %u for tcp", sip_port);
    }

    if(tls_context) {
        if(!sip::listen(tls_context, IPPROTO_TCP, iface, sip_port, true))
            shell::log(shell::FAIL, "cannot listen port %u for tls", sip_port + 1);
        else
            shell::log(shell::NOTIFY, "listening port %u for tls", sip_port + 1);
    }

    osip_trace_initialize_syslog(TRACE_LEVEL0, (char *)"bayonne");

    // create only one media thread for now...
    media *m = new media(stack * 1024l);
    m->start(mpri);

    thread *t;

    if(udp_context) {
        t = new thread(udp_context, stack *1024l);
        t->start(priority);
    }

    if(tcp_context) {
        t = new thread(tcp_context, stack *1024l);
        t->start(priority);
    }

    if(tls_context) {
        t = new thread(tls_context, stack *1024l);
        t->start(priority);
    }

    if(is(kp)) {
        new registry(*kp, sip_port, expires);
        return 0;
    }

    kp = keygroup.begin();
    while(is(kp)) {
        new registry(*kp, sip_port, expires);
        kp.next();
    }

    if(keys)
        cp = keys->get("timeslots");
    else
        cp = NULL;

    if(cp)
        timeslots = atoi(cp);

    if(keys)
        cp = keys->get("rtp");
    else
        cp = NULL;

    if(cp)
        rtp_port = atoi(cp);

    if(!rtp_port)
        rtp_port = ((sip_port / 2) * 2) + 2;

    if(keys)
        cp = keys->get("symmetric");
    else
        cp = NULL;

    if(cp && (toupper(*cp) == 'Y' || toupper(*cp) == 'T' || atoi(cp) > 0))
        media::symmetric = true;
    
    tsIndex = new Timeslot *[timeslots];
    while(timeslots--) {
        timeslot *ts = new timeslot(rtp_port);
        tsIndex[tsCount++] = (Timeslot *)ts;
        rtp_port += 2;
    }

    return tsCount;
}

void driver::stop(void)
{
    sip::release(tcp_context);
    sip::release(udp_context);
    sip::release(tls_context);

    thread::shutdown();

    unsigned index = 0;
    while(index < tsCount) {
        Timeslot *ts = tsIndex[index++];
        ts->shutdown();
    }

    media::shutdown();
}

registry *driver::locate(sip::reg_t rid)
{
    linked_pointer<registry>  rp = Group::begin();

    while(is(rp)) {
        if(rp->rid == rid)
            return *rp;
        rp.next();
    }
    return NULL;
}

static SERVICE_MAIN(main, argc, argv)
{
    signals::service("bayonne");
    server::start(argc, argv);
}

PROGRAM_MAIN(argc, argv)
{
    new driver;
    server::start(argc, argv, &service_main);
    PROGRAM_EXIT(0);
}


