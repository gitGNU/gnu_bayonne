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
#include <ctype.h>

namespace bayonne {

srv::srv(const char *uri_id) :
uri(uri_id, driver::family, driver::protocol)
{
}

srv::srv() : uri() {}

voip::context_t srv::route(char *buf, size_t size, const char *uri)
{
    char host[256];
    const char *schema = "sip";
    const char *sid = uri;
    unsigned short port = uri::portid(uri);
    voip::context_t ctx = driver::out_context;

    if(!uri::hostid(host, sizeof(host), uri))
        return NULL;

    if(eq(uri, "sips:", 5)) {
        schema = "sips";
        ctx = driver::tls_context;
    }
    else if(eq(uri, "tcp:", 4)) {
        uri += 4;
        ctx = driver::tcp_context;
    }
	else if(eq(uri, "udp:", 4)) {
        uri += 4;
        ctx = driver::udp_context;
    }

    buf[0] = 0;
    char *cp = strrchr(host, '.');
    if(Socket::is_numeric(host) || !cp || eq(cp, ".local") || eq(cp, ".localdomain")) {
        if(!port) {
            if(eq(schema, "sips"))
                port = 5061;
            else
                port = 5060;
        }
        if(strchr(host, ':'))
            snprintf(buf, size, "%s:[%s]:%u", schema, host, port);
        else
            snprintf(buf, size, "%s:%s:%u", schema, host, port);
        sid = buf;
    }
    set(sid, driver::family, driver::protocol);
    if(!entry)
        return NULL;

    if(!Socket::query(entry, host, sizeof(host)))
        return NULL;

#ifdef  AF_INET6
    if(entry->sa_family == AF_INET6)
        snprintf(buf, size, "%s:[%s]:%u", schema, host, (unsigned)ntohs(((struct sockaddr_in6 *)(entry))->sin6_port) & 0xffff);
    else
#endif
        snprintf(buf, size, "%s:%s:%u", schema, host, (unsigned)ntohs(((struct sockaddr_in *)(entry))->sin_port) & 0xffff);
    return ctx;
}

} // end namespace
