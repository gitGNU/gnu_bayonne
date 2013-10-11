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

/**
 * Basic registration operations.
 * This is the interface for common registration properties.  Drivers extend
 * the registration as needed for driver specific attributes.
 * @file bayonne/registry.h
 */

#ifndef _BAYONNE_REGISTRY_H_
#define _BAYONNE_REGISTRY_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/socket.h>
#endif

#ifndef _BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

#ifndef _BAYONNE_STATS_H_
#include <bayonne/stats.h>
#endif

#ifndef _BAYONNE_SERVER_H_
#include <bayonne/server.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

/**
 * A common registration base class.
 * This is used by drivers which manage server registrations, and may also
 * be used for spans, trunk groups, etc.  This offers a common base class
 * for generic properties.  Specific telephony servers have derived versions
 * that may have additional properties and support methods.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Registration : public LinkedObject, protected Env
{
protected:
    time_t activated;
    bool tracing;
    const char *id;
    const char *schema;
    statmap *stats;
    unsigned limit;

public:
    /**
     * Create a registration instance and add to registration list.
     * @param list we are adding registration to.
     * @param keys of /etc/bayonne.conf config entry used.
     * @param schema of registry.
     */
    Registration(LinkedObject **list, keydata *keys, const char *schema);

    /**
     * Release registration instance.  May cause deregistration from an
     * external call server.
     */
    virtual void release(void);

    /**
     * Return functional network interface of network based uri registrations.
     * @param uri to resolve a network interface for.
     * @param buffer to store resolved interface address.
     * @param size of buffer.
     */
    void getInterface(const char *uri, char *buffer, size_t size);

    /**
     * Get functional server or uri of a registration object.
     * @return uri of this registration object.
     */
    inline const char *getId(void)
        {return id;};

    /**
     * Get time that this registration became active.
     * @return activation time of this registration object.
     */
    inline time_t getActivated(void)
        {return activated;};

    /**
     * Convenience method to extract a host name from a network uri.
     * @param server network uri to parse.
     * @return host name (and port) portion of the uri.
     */
    const char *getHostid(const char *server);

    /**
     * Request a registration.  Uses call total and limit.
     * @return true if not past call limit.
     */
    bool attach(statmap::stat_t stat);

    /**
     * Detach a request.  Releases lock.
     */
    void detach(void);

    /**
     * Release a registration stat.
     * @param stat to release.
     */
    void release(statmap::stat_t stat);
};

END_NAMESPACE

#endif
