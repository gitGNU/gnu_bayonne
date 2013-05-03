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
    const char *schema = keys->get("protocol");
    char buffer[1024];
    char iface[180];
    osip_message_t *msg = NULL;

    if(!schema)
        schema = "sip";

    context = driver::out_context;
    if(eq_case(schema, "sipu") || eq_case("schema", "udp")) {
        schema = "sip";
        context = driver::udp_context;
    }
    else if(eq_case(schema, "sipt") || eq_case(schema, "tcp")) {
        schema = "sip";
        context = driver::tcp_context;
    }

    if(cp)
        expires = atoi(cp);
    else
        expires = expiration;

    userid = keys->get("userid");
    secret = keys->get("secret");
    digest = keys->get("digest");
    domain = keys->get("domain");
    method = keys->get("method");
    realm = keys->get("realm");
    active = false;
    rid = -1;

    if(!domain || !*domain)
        domain = "localdomain";

    if(!userid || !*userid)
        userid = keys->get("user");

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
        sip_add_authentication(context, userid, secret, realm);

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

    if(strchr(iface, ':'))
        snprintf(buffer, sizeof(buffer), "%s:%s@[%s]:%u", schema, uuid, iface, myport);
    else
        snprintf(buffer, sizeof(buffer), "%s:%s@%s:%u", schema, uuid, iface, myport);
    contact = Driver::dup(buffer);

    rid = sip_create_registration(context, uri, server, contact, expires, &msg);
    if(msg) {
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

        sip_send_registration(context, rid, msg);
        shell::debug(3, "registry id %d assigned to %s", rid, id);
    }
    else {
        shell::log(shell::ERR, "failed to register %s with %s", id, server);
        rid = -1;
    }
}

void registry::authenticate(const char *sip_realm)
{

    if(secret && userid && sip_realm) {
        shell::debug(3, "registry id %d authenticating to \"%s\"", rid, sip_realm);
        sip_add_authentication(context, userid, secret, sip_realm, true);
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

    shell::debug(3, "registry id %d released", rid);
    active = false;
}

void registry::shutdown(void)
{
    if(rid == -1)
        return;

    if(sip_release_registration(context, rid))
        shell::debug(3, "released registry id %d from %s", rid, server);
}
