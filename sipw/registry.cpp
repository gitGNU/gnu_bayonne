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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

registration::registration(Registration *root, keydata *keys, unsigned expiration, unsigned myport) :
Registration(root, keys, "sip:")
{
    const char *cp = keys->get("expires");
    const char *auth = keys->get("authorize");
    char buffer[256];
    char iface[180];

    if(auth)
        authid = memcopy(auth);
    else
        authid = uuid;

    if(cp)
        expires = atoi(cp);
    else
        expires = expiration;

    userid = memcopy(keys->get("userid"));
    secret = memcopy(keys->get("secret"));
    script = memcopy(keys->get("script"));
    domain = memcopy(keys->get("domain"));
    targets = memcopy(keys->get("targets"));
    localnames = memcopy(keys->get("localnames"));
    rid = -1;

    if(!script)
        script = id;

    if(!userid)
        userid = memcopy(keys->get("user"));

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
        snprintf(buffer, sizeof(buffer), "%s%s:%u", schema, cp, port);
    else
        snprintf(buffer, sizeof(buffer), "%s%s", schema, cp);

    server = memcopy(buffer);

    if(!domain)
        domain = getHostid(server);

    if(userid) {
        snprintf(buffer, sizeof(buffer), "%s%s@%s", schema, userid, domain);
        uri = memcopy(buffer);
    }
    else {
        snprintf(buffer, sizeof(buffer), "%s%s", schema, domain);
        uri = memcopy(buffer);
    }

    getInterface(server, iface, sizeof(iface));
    Random::uuid(uuid);

    if(strchr(iface, ':'))
        snprintf(buffer, sizeof(buffer), "%s%s@[%s]:%u", schema, uuid, iface, myport);
    else
        snprintf(buffer, sizeof(buffer), "%s%s@%s:%u", schema, uuid, iface, myport);
    contact = memcopy(buffer);
}

void registration::reload(keydata *keys)
{
    char buffer[256];

    const char *changed_userid = keys->get("userid");
    const char *changed_secret = keys->get("secret");
    const char *changed_script = keys->get("script");
    const char *changed_domain = keys->get("domain");
    const char *changed_targets = keys->get("targets");
    const char *changed_authid = keys->get("authorize");
    const char *changed_localnames = keys->get("localnames");
    bool newuri = false;

    if(!changed_domain)
        changed_domain = keys->get("domain");

    if(!changed_userid)
        changed_userid = keys->get("user");

    if(!changed_script)
        changed_script = id;

    if(changed_authid && !eq(changed_authid, authid))
        authid = memcopy(changed_authid);
    else if(!changed_authid)
        authid = uuid;

    if(changed_domain && !eq(changed_domain, domain)) {
        domain = memcopy(changed_domain);
        newuri = true;
    }

    if(changed_userid && !eq(changed_userid, userid)) {
        newuri = true;
        userid = memcopy(changed_userid);
    }

    if(changed_secret && !eq(changed_secret, secret))
        secret = memcopy(changed_secret);

    if(changed_script && !eq(changed_script, script))
        script = memcopy(changed_script);

    if(changed_targets && (!targets || !eq(changed_targets, targets)))
        targets = memcopy(changed_targets);
    else if(!changed_targets)
        targets = NULL;

    if(changed_localnames && (!localnames || !eq(changed_localnames, localnames)))
        localnames = memcopy(changed_localnames);
    else if(!changed_localnames)
        localnames = NULL;

    if(newuri) {
        snprintf(buffer, sizeof(buffer), "%s%s@%s", schema, userid, domain);
        uri = memcopy(buffer);
    }

    Registration::reload(keys);
    refresh();
}

bool registration::refresh(void)
{
    voip::msg_t msg = NULL;

    if(rid != -1)
        return true;

    context = driver::out_context;
    if(-1 != (rid = voip::make_registry_request(context, uri, server, contact, expires, &msg))) {
        osip_message_set_supported(msg, "100rel");
        osip_message_set_header(msg, "Event", "Registration");
        osip_message_set_header(msg, "Allow-Events", "presence");
        voip::send_registry_request(context, rid, msg);
    }
    else
        rid = -1;

    shell::debug(3, "registry id %d assigned to %s", rid, id);

    if(rid == -1)
        return false;

    return true;
}

void registration::release(void)
{
    if(rid == -1)
        return;

    Registration::release();

    voip::release_registry(context, rid);
    shell::debug(3, "registry id %d released", rid);
    rid = -1;
}

void registration::confirm(void)
{
    if(rid == -1 || activated != 0)
        return;

    Registration::activate();
    shell::debug(3, "registry id %d activated", rid);
}

void registration::failed(void)
{
    if(rid == -1)
        return;

    Registration::release();
    shell::debug(3, "registry id %d failed", rid);
}

void registration::cancel(void)
{
    if(rid == -1)
        return;

    Registration::release();
    shell::debug(3, "registry id %d terminated", rid);
    rid = -1;
}

void registration::authenticate(const char *realm)
{
    voip::add_authentication(context, userid, secret, realm);
}
