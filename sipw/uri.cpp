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
#include <ucommon/secure.h>

#ifdef  HAVE_RESOLV_H
extern "C" {
#include <resolv.h>
}

#if PACKETSZ > 1024
#define MAXPACKET PACKETSZ
#else
#define MAXPACKET 1024
#endif 

typedef union {
        HEADER hdr;
        char buf[MAXPACKET];
} query;

#ifndef T_SRV 
#define T_SRV           33
#endif

#endif

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

uri::address::address(const char *uri, int family, int protocol) : Socket::address()
{
#ifdef  _MSWINDOWS_
    Socket::init();
#endif

    int port = portid(uri);
    char host[256], svc[10];
    struct addrinfo hint;

#if defined(HAVE_RESOLV_H)
    bool nosrv = false;
#endif

    srv = NULL;
    entry = NULL;
	count = 0;

    String::set(svc, sizeof(svc), "sip");

    if(port) {
#ifdef  HAVE_RESOLV_H
        nosrv = true;
#endif
        snprintf(svc, sizeof(svc), "%d", port);
    }
    else if(eq(uri, "sips:", 5)) {
        protocol = IPPROTO_TCP;
        String::set(svc, sizeof(svc), "sips");
    }
    else if(eq(uri, "tcp:", 4)) {
        protocol = IPPROTO_TCP;
        uri += 4;    
    }
    else if(eq(uri, "udp:", 4)) {
        protocol = IPPROTO_UDP;
        uri += 4;    
    }

    hostid(host, sizeof(host), uri);
    memset(&hint, 0, sizeof(hint));

    hint.ai_socktype = 0;
    hint.ai_protocol = protocol;

    if(hint.ai_protocol == IPPROTO_UDP)
        hint.ai_socktype = SOCK_DGRAM;
    else
        hint.ai_socktype = SOCK_STREAM;

#ifdef  PF_UNSPEC
    hint.ai_flags = AI_PASSIVE;
#endif

    if(Socket::is_numeric(host)) {
        hint.ai_flags |= AI_NUMERICHOST;
#ifdef  HAVE_RESOLV_H
        nosrv = true;
#endif

        if(strchr(host, ':')) {
#ifdef  AF_INET6
            family = AF_INET6;
#else
            return NULL;
#endif
        }
        else
            family = AF_INET;
    }

    hint.ai_family = family;

#if defined(AF_INET6) && defined(AI_V4MAPPED)
    if(hint.ai_family == AF_INET6)
        hint.ai_flags |= AI_V4MAPPED;
#endif
#ifdef  AI_NUMERICSERV
    if(atoi(svc) > 0)
        hint.ai_flags |= AI_NUMERICSERV;
#endif

#ifdef  HAVE_RESOLV_H
    int result;
    HEADER *hp;
    char hbuf[256];
    uint16_t acount, qcount;
    unsigned char *mp, *ep, *cp;
    uint16_t type, weight, priority, hport, dlen;

    if(nosrv)
        goto nosrv;

    query reply;
    char zone[256];
    
    if(hint.ai_protocol == IPPROTO_TCP)
        snprintf(zone, sizeof(zone), "_%s._tcp.%s", svc, host);
    else
        snprintf(zone, sizeof(zone), "_%s._udp.%s", svc, host);

    result = res_query(zone, C_IN, T_SRV, (unsigned char *)&reply, sizeof(reply));
    if(result < (int)sizeof(HEADER))
        goto nosrv;

    hp = (HEADER *)&reply;
    acount = ntohs(hp->ancount);
    qcount = ntohs(hp->qdcount);
    mp = (unsigned char *)&reply;
    ep = (unsigned char *)&reply + result;
    cp = (unsigned char *)&reply + sizeof(HEADER);

    if(!acount)
        goto nosrv;

    srv = new srvaddrinfo[acount];
    while(qcount-- > 0 && cp < ep) {
        result = dn_expand(mp, ep, cp, hbuf, sizeof(hbuf));
        if(result < 0)
            goto nosrv;
        cp += result + QFIXEDSZ;
    }

    while(acount-- > 0 && cp < ep) {
        result = dn_expand(mp, ep, cp, hbuf, sizeof(hbuf));
        if(result < 0)
            goto nosrv;
        
        cp += result;

        type = ntohs(*((uint16_t *)cp));
        cp += sizeof(uint16_t);

		// class
		cp += sizeof(uint16_t);

        // ttl
        cp += sizeof(uint32_t);

        dlen = ntohs(*((uint16_t *)cp));
        cp += sizeof(uint16_t);
        
		if(type != T_SRV) {
	        cp += dlen;
			continue;
		}
		
        priority = ntohs(*((uint16_t *)cp));
		cp += sizeof(uint16_t);
	
		weight = ntohs(*((uint16_t *)cp));
		cp += sizeof(uint16_t);

		hport = ntohs(*((uint16_t *)cp));
		cp += sizeof(uint16_t);

		result = dn_expand(mp, ep, cp, hbuf, sizeof(hbuf));
		if(result < 0)
			break;

        Socket::address resolv(hbuf, hport);
        struct sockaddr *sp = resolv.getAddr();

        if(sp) {
            uint16_t rand;
            srvaddrinfo *current = NULL;

            Random::fill((unsigned char *)&rand, sizeof(rand));
            rand &= 0x7fff;
            if(weight)
                weight = (1 + rand) % ( 10000 * weight);
        
            srv[count].weight = weight;
            srv[count].priority = priority;
            Socket::store(&srv[count].addr, sp);
            if(!current || priority < current->priority || weight > current->weight) {
                current = &srv[count];
                entry = (struct sockaddr *)&srv[count].addr;
                pri = priority;
            }
            ++count;
        }
		cp += result;
    }
	
	return;
nosrv:
    if(srv) {
        delete[] srv;
        srv = NULL;
    }
#endif

    if(eq(svc, "sips"))
        String::set(svc, sizeof(svc), "5061");
    else if(eq(svc, "sip"))
        String::set(svc, sizeof(svc), "5060");
    getaddrinfo(host, svc, &hint, &list);
	struct addrinfo *ap = list;
    count = 0;

    if(ap)
        entry = ap->ai_addr;
	while(ap) {
		++count;
		ap = ap->ai_next;
	}
}

uri::address::~address()
{
    if(srv) {
        delete[] srv;
        srv = NULL;
    }

    if(list) {
        freeaddrinfo(list);
        list = NULL;
    } 
}       

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
    else if(eq(uri, "sip:", 4))
        uri += 4;
    else if(eq(uri, "tcp:", 4))
        uri += 4;
    else if(eq(uri, "udp:", 4))
        uri += 4;

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
    const char *schema = "sip";

    if(!uri::hostid(host, sizeof(host), uri))
        return false;

    if(eq(uri, "sips:", 5)) {
        schema = "sips";
        protocol = IPPROTO_TCP;
    }
    else if(eq(uri, "tcp:", 4)) {
        uri += 4;
        protocol = IPPROTO_TCP;
    }
    else if(eq(uri, "udp:", 4)) {
        uri += 4;
        protocol = IPPROTO_UDP;
    }

    buf[0] = 0;
    char *cp = strrchr(host, '.');
    if(Socket::is_numeric(host) || !cp || eq(cp, ".local") || eq(cp, ".localdomain"))
        return uri::server(buf, size, uri);

    uri::address addr(uri, family, protocol);
	struct sockaddr *ap = *addr;
    if(!ap)
        return false;
    if(!Socket::query(ap, host, sizeof(host)))
        return false;
#ifdef	AF_INET6
	if(ap->sa_family == AF_INET6)
		snprintf(buf, size, "%s:[%s]:%u", schema, host, (unsigned)ntohs(((struct sockaddr_in6 *)(ap))->sin6_port) & 0xffff);
	else
#endif
		snprintf(buf, size, "%s:%s:%u", schema, host, (unsigned)ntohs(((struct sockaddr_in *)(ap))->sin_port) & 0xffff);
    return true;
}

bool uri::hostid(char *buf, size_t size, const char *uri)
{
    assert(buf != NULL);
    assert(size > 0);

    buf[0] = 0;
    
    if(eq(uri, "sips:", 5))      
        uri += 5;
    else if(eq(uri, "sip:", 4))
        uri += 4;
    else if(eq(uri, "udp:", 4))
        uri += 4;
    else if(eq(uri, "tcp:", 4))
        uri += 4;

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

