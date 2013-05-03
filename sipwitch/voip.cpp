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

#include "voip.h"

static const char *agent = "eXosip";
static int family = AF_INET;

#ifdef	EXOSIP_API4

void sip_add_authentication(sip_context_t ctx, const char *user, const char *secret, const char *realm, bool automatic) 
{
    eXosip_lock(ctx);
    eXosip_add_authentication_info(ctx, user, user, secret, NULL, realm);
    if(automatic)
        eXosip_automatic_action(ctx);
    eXosip_unlock(ctx);
}

sip_reg_t sip_create_registration(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg) 
{
    *msg = NULL;
    eXosip_lock(ctx);
    sip_reg_t rid = eXosip_register_build_initial_register(ctx, uri, s, c, exp, msg);
    if(!msg)
        eXosip_unlock(ctx);
    return rid;
}

void sip_send_registration(sip_context_t c, sip_reg_t r, osip_message_t *msg) 
{
    if(!msg)
	    return;
    eXosip_register_send_register(c, r, msg);
    eXosip_unlock(c);
}

bool sip_release_registration(sip_context_t ctx, sip_reg_t rid)
{
    bool rtn = false;
    osip_message_t *msg = NULL;
    eXosip_lock(ctx);
    eXosip_register_build_register(ctx, rid, 0, &msg);
    if(msg) {
        eXosip_register_send_register(ctx, rid, msg);
        rtn = true;
    }
    eXosip_unlock(ctx);
    return rtn;
}

void sip_default_action(sip_context_t ctx, sip_event_t ev)
{
    eXosip_lock(ctx);
    eXosip_default_action(ctx, ev);
    eXosip_unlock(ctx);
}

void sip_automatic_action(sip_context_t ctx)
{
    eXosip_lock(ctx);
    eXosip_automatic_action(ctx);
    eXosip_unlock(ctx);
}

sip_event_t sip_get_event(sip_context_t ctx, sip_timeout_t timeout)
{
    unsigned s = timeout / 1000l;
    unsigned ms = timeout % 1000l;
    return eXosip_event_wait(ctx, s, ms);
}

bool sip_listen(sip_context_t ctx, int proto, const char *addr, unsigned port, bool tls)
{
    int tlsmode = 0;

    if(!ctx)
        return false;

#ifdef  AF_INET6
    if(family == AF_INET6 && !addr)
        addr = "::0";
#endif
    if(!addr)
        addr = "*";

    // port always even...
    port = port & 0xfffe;
    if(tls) {
        tlsmode = 1;
        ++port;	// tls always next odd port...
    }

    if(eXosip_listen_addr(ctx, proto, addr, port, family, tlsmode))
        return false;

    eXosip_set_user_agent(ctx, agent);

    return true;
}

#else

void sip_add_authentication(sip_context_t ctx, const char *user, const char *secret, const char *realm, bool automatic) 
{
    eXosip_lock();
    eXosip_add_authentication_info(user, user, secret, NULL, realm);
    if(automatic)
        eXosip_automatic_action();
    eXosip_unlock();
}

sip_reg_t sip_create_registration(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg) 
{
    *msg = NULL;
    eXosip_lock();
    sip_reg_t rid = eXosip_register_build_initial_register(uri, s, c, exp, msg);
    if(!msg)
        eXosip_unlock();
    return rid;
}

void sip_send_registration(sip_context_t c, sip_reg_t r, osip_message_t *msg) 
{
    if(!msg)
        return;
    eXosip_register_send_register(r, msg);
    eXosip_unlock();
}

bool sip_release_registration(sip_context_t ctx, sip_reg_t rid)
{
    bool rtn = false;
    osip_message_t *msg = NULL;
    eXosip_lock();
    eXosip_register_build_register(rid, 0, &msg);
    if(msg) {
        eXosip_register_send_register(rid, msg);
        rtn = true;
    }
    eXosip_unlock();
    return rtn;
}

void sip_default_action(sip_context_t ctx, sip_event_t ev)
{
    eXosip_lock();
    eXosip_default_action(ev);
    eXosip_unlock();
}

void sip_automatic_action(sip_context_t ctx)
{
    eXosip_lock();
    eXosip_automatic_action();
    eXosip_unlock();
}

sip_event_t sip_get_event(sip_context_t ctx, sip_timeout_t timeout)
{
    unsigned s = timeout / 1000l;
    unsigned ms = timeout % 1000l;
    return eXosip_event_wait(s, ms);
}

bool sip_listen(sip_context_t ctx, int proto, const char *addr, unsigned port, bool tls)
{
    int tlsmode = 0;

#ifdef  AF_INET6
    if(family == AF_INET6 && !addr)
        addr = "::0";
#endif
    if(!addr)
        addr = "*";

    // port always even...
    port = port & 0xfffe;
    if(tls) {
        tlsmode = 1;
        ++port;	// tls always next odd port...
    }

    if(eXosip_listen_addr(proto, addr, port, family, tlsmode))
        return false;

    eXosip_set_user_agent(agent);
    return true;
}

#endif

void sip_release_event(sip_event_t ev)
{
    if(ev)
        eXosip_event_free(ev);
}

void sip_setup(const char *a, int f)
{
    agent = a;
    family = f;

#ifdef  AF_INET6
    if(family == AF_INET6)
        eXosip_enable_ipv6(1);
#endif
}

