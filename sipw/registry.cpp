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
    const char *schema = NULL;
    char buffer[1024];
    char iface[180];
    sip::msg_t msg = NULL;
    context = driver::out_context;
    const char *identity = keys->get("identity");
    const char *sid = identity;

    active = false;
    rid = -1;

    if(!identity)
        identity = keys->get("uri");

    if(!identity) {
        shell::log(shell::ERR, "failed to register %s; no uri identity", id);
        return;
    }
    
    if(eq(identity, "udp:", 4)) {
        identity += 4;
        schema = "sip";
        context = driver::udp_context;
    }
    else if(eq(identity, "tcp:", 4)) {
        identity += 4;
        schema = "sip";
        context = driver::tcp_context;
    }
    else if(eq(identity, "sips:", 5)) {
        schema = "sips";
        context = driver::tls_context;
        identity += 5;
    }
    else if(!eq(identity, "sip:", 4))
        schema = "sip";

    if(schema) {
        snprintf(buffer, sizeof(buffer), "%s:%s", schema, identity);
        uri = driver::dup(buffer);
    }
    else {
        uri = identity;
        schema = "sip";
    }

    if(cp)
        expires = atoi(cp);
    else
        expires = expiration;

    sip::uri_userid(buffer, sizeof(buffer), uri);
    userid = driver::dup(buffer);

    server = keys->get("server");
    if(!server)
        server = keys->get("proxy");
    if(!server)
        server = keys->get("route");

    if(!server)
        server = sid;

    context = srv::route(buffer, sizeof(buffer), server);
    server = driver::dup(buffer);

    secret = keys->get("secret");
    digest = keys->get("digest");
    method = keys->get("method");
    realm = keys->get("realm");
    
    // inactive contexts ignored...
    if(!context) {
        shell::log(shell::ERR, "failed to register %s; no context for %s", id, schema);
        return;
    }

    if(!method || !*method)
        method = "md5";

    if(!digest && secret && userid && realm) {
        digest_t tmp = method;
        if(tmp.puts(str(userid) + ":" + realm + ":" + secret))
            digest = strdup(*tmp);
        else
            shell::errexit(6, "*** bayonne: digest: unsupported computation");
    }

    if(secret && userid && realm)
        sip::add_authentication(context, userid, secret, realm);

    getInterface(server, iface, sizeof(iface));
    Random::uuid(uuid);

    if(strchr(iface, ':'))
        snprintf(buffer, sizeof(buffer), "%s:%s@[%s]:%u", schema, uuid, iface, myport);
    else
        snprintf(buffer, sizeof(buffer), "%s:%s@%s:%u", schema, uuid, iface, myport);
    contact = Driver::dup(buffer);

    if(-1 != (rid = sip::make_registry_request(context, uri, server, contact, expires, &msg))) {
        osip_message_set_supported(msg, "100rel");
        osip_message_set_header(msg, "Event", "Registration");
        osip_message_set_header(msg, "Allow-Events", "presence");

        if(digest) {
            stringbuf<64> response;
            stringbuf<64> once;
            char nounce[64];
            snprintf(buffer, sizeof(buffer), "%s:%s", "REGISTER", uri);
            Random::uuid(nounce);
            digest_t auth = "md5";
            auth.puts(nounce);
            once = *auth;
            auth = method;
            auth.puts(buffer);
            response = *auth;
            snprintf(buffer, sizeof(buffer), "%s:%s:%s", digest, *once, *response);
            auth.reset();
            auth.puts(buffer);
            response = *auth;
            snprintf(buffer, sizeof(buffer),
                "Digest username=\"%s\""
                ",realm=\"%s\""
                ",uri=\"%s\""
                ",response=\"%s\""
                ",nonce=\"%s\""
                ",algorithm=%s"
                ,userid, realm, uri, *response, *once, method);
            osip_message_set_header(msg, AUTHORIZATION, buffer);
        }

        sip::send_registry_request(context, rid, msg);
        shell::debug(3, "registry id %d assigned to %s", rid, id);
    }
    else {
        shell::log(shell::ERR, "failed to register %s with %s", id, server);
        rid = -1;
    }
}

void registry::authenticate(const char *sip_realm)
{
    if(rid == -1)
        return;

    if(secret && userid && sip_realm) {
        shell::debug(3, "registry id %d authenticating to \"%s\"", rid, sip_realm);
        sip::add_authentication(context, userid, secret, sip_realm, true);
    }
}

void registry::activate(void)
{
    if(rid == -1 || active)
        return;

    shell::debug(3, "registry id %d activated", rid);
    active = true;
}

void registry::release(void)
{
    if(rid == -1 || !active)
        return;

    if(sip::release_registry(context, rid))
        shell::debug(3, "registry id %d released", rid);
    active = false;
    rid = -1;
}

