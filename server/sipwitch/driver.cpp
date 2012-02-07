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

static class driver : public Driver
{
public:
    driver();

    int start(void);

} _driver_;

driver::driver() : Driver("sip", "registry")
{
}

int driver::start(void)
{
    linked_pointer<keydata> kp = getRegistry();
    const char *cp = keys->get("port");
    unsigned port = 5060, expires = 300;
    int protocol = IPPROTO_UDP;
    int family = AF_INET;
    int tlsmode = 0;
    int socktype = SOCK_DGRAM;
    int send101 = 1;

    if(cp)
        port = atoi(cp);

    cp = keys->get("expires");
    if(cp)
        expires = atoi(cp);

    const char *iface = keys->get("interface");
    if(iface) {
#ifdef  AF_INET6
        if(strchr(iface, ':'))
            family = AF_INET6;
#endif
        if(eq(iface, ":::") || eq(iface, "::0") || eq(iface, "*") || iface[0] == 0)
            iface = NULL;
    }

    const char *transport = keys->get("transport");
    if(transport && eq(transport, "tcp")) {
        socktype = SOCK_STREAM;
        protocol = IPPROTO_TCP;
    }
    else if(transport && eq(transport, "tls")) {
        socktype = SOCK_STREAM;
        protocol = IPPROTO_TCP;
        tlsmode = 1;
    }

    const char *agent = keys->get("agent");
    if(!agent)
        agent = "bayonne-" VERSION "/exosip2";


    const char *addr = iface;

    eXosip_init();
#ifdef  AF_INET6
    if(family == AF_INET6) {
        eXosip_enable_ipv6(1);
        if(!iface)
            addr = "::0";
    }
#endif
    if(!addr)
        addr = "*";

    Socket::family(family);
    if(eXosip_listen_addr(protocol, iface, port, family, tlsmode))
        shell::log(shell::FAIL, "driver cannot bind %s, port=%d", addr, port);
    else
        shell::log(shell::NOTIFY, "driver binding %s, port=%d", addr, port);

    osip_trace_initialize_syslog(TRACE_LEVEL0, (char *)"bayonne");
    eXosip_set_user_agent(agent);

#ifdef  EXOSIP2_OPTION_SEND_101
    eXosip_set_option(EXOSIP_OPT_DONT_SEND_101, &send101);
#endif

    if(is(kp)) {
        new registry(*kp, port, expires);
        return 0;
    }

    keys = getGroups();
    while(is(kp)) {
        new registry(*kp, port, expires);
        kp.next();
    }
    return 0;
}
