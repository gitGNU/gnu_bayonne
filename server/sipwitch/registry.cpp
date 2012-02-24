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

registry::registry(keydata *keyset, unsigned myport, unsigned expiration) :
Registry(keyset)
{
    const char *cp = keys->get("expires");
    const char *auth = keys->get("authorize");
    const char *schema = keys->get("protocol");
    char buffer[256];
    char iface[180];
    osip_message_t *msg = NULL;

    if(!schema)
        schema = "sip";

    if(!auth)
        authid = uuid;

    if(cp)
        expires = atoi(cp);
    else
        expires = expiration;

    userid = keys->get("userid");
    secret = keys->get("secret");
    domain = keys->get("domain");
    rid = -1;

    if(!userid)
        userid = keys->get("user");

    if(!secret)
        authid = NULL;

    unsigned port = 0;
    cp = keys->get("port");
    if(cp)
        port = atoi(cp);

    cp = keys->get("server");
    if(!cp)
        cp = id;

    if(port)
        snprintf(buffer, sizeof(buffer), "%s:%s:%u", schema, cp, port);
    else
        snprintf(buffer, sizeof(buffer), "%s:%s", schema, cp);

    server = Driver::dup(buffer);

    if(!domain)
        domain = getHostid(server);

    if(userid) {
        snprintf(buffer, sizeof(buffer), "%s:%s@%s", schema, userid, domain);
        uri = Driver::dup(buffer);
    }
    else {
        snprintf(buffer, sizeof(buffer), "%s:%s", schema, domain);
        uri = Driver::dup(buffer);
    }

    getInterface(server, iface, sizeof(iface));
    Random::uuid(uuid);
    target = uuid;

    if(strchr(iface, ':'))
        snprintf(buffer, sizeof(buffer), "%s:%s@[%s]:%u", schema, uuid, iface, myport);
    else
        snprintf(buffer, sizeof(buffer), "%s:%s@%s:%u", schema, uuid, iface, myport);
    contact = Driver::dup(buffer);

    eXosip_lock();
    rid = eXosip_register_build_initial_register(uri, server, contact, expires, &msg);
    if(msg) {
        osip_message_set_supported(msg, "100rel");
        osip_message_set_header(msg, "Event", "Registration");
        osip_message_set_header(msg, "Allow-Events", "presence");
        eXosip_register_send_register(rid, msg);
        shell::debug(3, "registry id %d assigned to %s", rid, id);
    }
    else {
        shell::log(shell::ERR, "failed to register %s with %s", id, server);
        rid = -1;
    }

    eXosip_unlock();
}

void registry::shutdown()
{
    osip_message_t *msg = NULL;

    if(rid == -1)
        return;

    eXosip_lock();
    eXosip_register_build_register(rid, 0, &msg);
    if(msg) {
        shell::debug(3, "released registry id %d from %s", rid, server);
        eXosip_register_send_register(rid, msg);
    }
    eXosip_unlock();
}
