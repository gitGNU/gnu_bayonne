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

#ifdef  HAVE_RULI_H
extern "C" {
#include <ruli.h>
}
#endif

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

bool uri::dialed(char *buf, size_t size, const char *to, const char *id)
{
    assert(buf != NULL);
    assert(to != NULL);
    assert(size > 0);

    buf[0] = 0;
    const char *schema = "sip:";

    if(eq(to, "sips:", 5)) {
        schema = "sips:";
        to += 5;
    }
    else if(eq(to, "sip:", 4)) {
        to += 4;
    }
    else if(eq(id, "sips:", 5))
        schema = "sips:";

    if(strchr(to, '@')) {
        snprintf(buf, size, "%s%s", schema, to);
        return true;
    }

    const char *host = strchr(id, '@');
    if(host)
        ++host;
    else {
        if(eq(id, "sips:", 5))
            id += 5;
        else if(eq(id, "sip:", 4))
            id += 4;
        host = id;
    }
    snprintf(buf, size, "%s%s@%s", schema, to, host);
    return true;
} 

bool uri::create(char *buf, size_t size, const char *server, const char *userid)
{
    assert(buf != NULL);
    assert(size > 0);

    buf[0] = 0;
    const char *schema="sip:";
    
    if(eq(server, "sips:", 5)) {
        schema="sips:";
        server += 5;
    }
    else if(eq(server, "sip:", 4)) {
        server += 4;
    }

    const char *sp = strchr(server, '@');
    if(!sp && userid) {
        snprintf(buf, size, "%s%s@%s", schema, userid, server);
        return true;
    }

    snprintf(buf, size, "%s%s", schema, server);
    return true;
}
    
bool uri::server(char *buf, size_t size, const char *uri)
{
    assert(buf != NULL);
    assert(size > 0);

    buf[0] = 0;
    const char *schema="sip:";
    
    if(eq(uri, "sips:", 5)) {
        schema="sips:";
        uri += 5;
    }
    else if(eq(uri, "sip:", 4)) {
        uri += 4;
    }

    const char *sp = strchr(uri, '@');
    if(sp)
        uri = ++sp;

    snprintf(buf, size, "%s%s", schema, uri);
    return true;
}

bool uri::userid(char *buf, size_t size, const char *uri)
{
    assert(buf != NULL);
    assert(size > 0);

	buf[0] = 0;
	char *ep;

	if(!uri)
		return false;

    if(eq(uri, "sip:", 4))
        uri += 4;
    else if(eq(uri, "sips:", 5))
        uri += 5;

	if(!strchr(uri, '@'))
		return false;

    String::set(buf, size, uri);
    ep = strchr(buf, '@');
    if(ep)
        *ep = 0;
    return true;
}

unsigned short uri::portid(const char *uri)
{
    const char *pp = NULL;
    const char *fp = NULL;

    if(eq(uri, "sips:", 5))      
        uri += 5;
    else if(eq(uri, "sip:", 4)) {
        uri += 4;
    }

    if(*uri == '[') {
        pp = strchr(uri, ']');
        if(pp)
            pp = strchr(pp, ':');
    }
    else {
        pp = strrchr(uri, ':');
        fp = strchr(uri, ':');
        if(fp != pp)
            pp = NULL;
    }
    if(pp)
        return atoi(++pp);
    else
        return 0;
}

bool uri::route(char *buf, size_t size, const char *uri, int family, int protocol)
{
    char host[256];
    if(!uri::hostid(host, sizeof(host), uri))
        return false;

    buf[0] = 0;
    char *cp = strrchr(host, '.');
    if(Socket::is_numeric(host) || portid(uri) || !cp || eq(cp, ".local") || eq(cp, ".localdomain"))
        return uri::server(buf, size, uri);

#ifdef  HAVE_RULI_H
    uri::address addr(uri, family, protocol);
	struct addrinfo *ap = *addr;
    if(!ap)
        return false;
    if(!Socket::query(ap->ai_addr, host, sizeof(host)))
        return false;
#ifdef	AF_INET6
	if(ap->ai_family == AF_INET6)
		snprintf(buf, size, "sip:[%s]:%u", host, (unsigned)ntohs(((struct sockaddr_in6 *)(ap->ai_addr))->sin6_port) & 0xffff);
	else
#endif
		snprintf(buf, size, "sip:%s:%u", host, (unsigned)ntohs(((struct sockaddr_in *)(ap->ai_addr))->sin_port) & 0xffff);
    return true;
#else
    return uri::server(buf, size, uri);
#endif
}

bool uri::hostid(char *buf, size_t size, const char *uri)
{
    assert(buf != NULL);
    assert(size > 0);

    buf[0] = 0;
    
    if(eq(uri, "sips:", 5))      
        uri += 5;
    else if(eq(uri, "sip:", 4)) {
        uri += 4;
    }

    char *sp = (char *)strchr(uri, '@');
    if(sp)
        uri = ++sp;

    if(*uri == '[') {
        String::set(buf, size, ++uri);
        sp = strchr(buf, ']');
        if(sp)
            (*sp++) = 0;
    }
    else {
        String::set(buf, size, uri);
        sp = strrchr(buf, ':');
        if(sp && sp == strchr(buf, ':'))
            *sp = 0;
    }
    return true;
}

