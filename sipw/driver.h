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
    unsigned expires;
    const char *contact;
    const char *userid;
    const char *secret;
    const char *digest;
    const char *server;
    const char *method;
    const char *realm;
    const char *uri;
    char uuid[38];
    bool active;

    sip::context_t context;
    sip::reg_t rid;

    registry(keydata *keyset, unsigned port = 5060, unsigned expiration = 120);

    void authenticate(const char *realm);

    void activate(void);

    void release(void);
};

class __LOCAL thread : public DetachedThread
{
private:
    const char *instance;
    sip::context_t    context;
    sip::event_t      sevent;
    registry    *reg;

    void run(void);

public:
    thread(sip::context_t source, size_t size, const char *type);

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

    static sip::context_t out_context;   // default output context
    static sip::context_t udp_context;
    static sip::context_t tcp_context;
    static sip::context_t tls_context;

    static registry *locate(sip::reg_t rid);

};

class __LOCAL uri 
{
public:
	class address : protected Socket::address
	{
	protected:
		class srvaddrinfo
		{
		public:
		    struct sockaddr_storage addr;
		    uint16_t weight, priority;
		};

		class srvaddrinfo *srv;
		struct sockaddr *entry;
		uint16_t pri;
		unsigned count;

	public:
		address(const char *uri, int family, int protocol);
		~address();

		inline struct sockaddr *operator*() const
			{return entry;};
	};

	static bool create(char *buf, size_t size, const char *server, const char *user = NULL);
	static bool dialed(char *buf, size_t size, const char *to, const char *id);
	static bool server(char *buf, size_t size, const char *uri);
	static bool hostid(char *buf, size_t size, const char *uri);
	static bool userid(char *buf, size_t size, const char *uri);
	static bool route(char *buf, size_t size, const char *uri, int family, int protocol);
	static unsigned short portid(const char *uri);
};


END_NAMESPACE

