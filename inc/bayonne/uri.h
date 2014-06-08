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

#ifndef _BAYONNE_URL_H_
#define _BAYONNE_URL_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/socket.h>
#endif

#ifndef __BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

namespace bayonne {

class __EXPORT uri : protected Socket::address
{
public:
    class address
    {
    public:
	struct sockaddr_storage addr;
	uint16_t weight, priority;
    };

protected:
    address *srvlist;
    struct sockaddr *entry;
    uint16_t pri;
    unsigned count;

public:
    uri(const char *id, int family = AF_INET, int protocol = IPPROTO_UDP);
    uri();
    ~uri();

    void set(const char *uri, int family = AF_INET, int protocol = IPPROTO_UDP);

    void clear(void);

    inline struct sockaddr *operator*() const
        {return entry;}

    inline operator bool() const
	    {return entry != NULL;}

    inline bool operator!() const
	    {return entry == NULL;}

    struct sockaddr *next(void);

    static bool create(char *buf, size_t size, const char *server, const char *user = NULL);
    static bool dialed(char *buf, size_t size, const char *to, const char *id);
    static bool server(char *buf, size_t size, const char *uri);
    static bool hostid(char *buf, size_t size, const char *uri);
    static bool userid(char *buf, size_t size, const char *uri);
    static unsigned short portid(const char *uri);
};

} // end namespace

#endif
