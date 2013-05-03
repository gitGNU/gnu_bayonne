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

bool make_request_message(sip_context_t ctx, osip_message_t **msg, const char *method, const char *to, const char *from, const char *route)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_message_build_request(ctx, msg, method, to, from, route);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }
    return true;
}

bool make_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_message_build_answer(ctx, tid, status, msg);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }
    return true;
}

void send_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message *msg)
{
    if(!msg)
        eXosip_lock(ctx);
    eXosip_message_send_answer(ctx, tid, status, msg);
    eXosip_unlock(ctx);
}

bool make_answer_invite(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_call_build_answer(ctx, tid, status, msg);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }
    return true;
}

void send_answer_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message *msg)
{
    if(!msg)
        eXosip_lock(ctx);
    eXosip_call_send_answer(ctx, tid, status, msg);
    eXosip_unlock(ctx);
}

void send_request_message(sip_context_t ctx, osip_message_t *msg)
{
    if(!msg)
        return;

    eXosip_message_send_request(ctx, msg);
    eXosip_unlock(ctx);
}

void sip_release_call(sip_context_t ctx, sip_call_t cid, sip_dlg_t did)
{
    eXosip_lock(ctx);
    eXosip_call_terminate(ctx, cid, did);
    eXosip_unlock(ctx);
}

bool make_dialog_requeast(sip_context_t ctx, sip_dlg_t did, const char *method, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_call_build_request(ctx, did, method, msg);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }

    return true;
}

bool make_dialog_notify(sip_context_t ctx, sip_dlg_t did, int status, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_call_build_notify(ctx, did, status, msg);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }

    return true;
}

bool make_dialog_update(sip_context_t ctx, sip_dlg_t did, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock(ctx);
    eXosip_call_build_update(ctx, did, msg);
    if(!*msg) {
        eXosip_unlock(ctx);
        return false;
    }

    return true;
}

void send_dialog_message(sip_context_t ctx, sip_dlg_t did, osip_message_t *msg)
{
    if(!msg)
        return;

    eXosip_call_send_request(ctx, did, msg);
    eXosip_unlock(ctx);
}

sip_reg_t make_registry_request(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg) 
{
    *msg = NULL;
    eXosip_lock(ctx);
    sip_reg_t rid = eXosip_register_build_initial_register(ctx, uri, s, c, exp, msg);
    if(!*msg)
        eXosip_unlock(ctx);
    return rid;
}

void send_registry_message(sip_context_t c, sip_reg_t r, osip_message_t *msg) 
{
    if(!msg)
	    return;
    eXosip_register_send_register(c, r, msg);
    eXosip_unlock(c);
}

bool sip_release_registry(sip_context_t ctx, sip_reg_t rid)
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

bool make_request_message(sip_context_t ctx, osip_message_t **msg, const char *method, const char *to, const char *from, const char *route)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock();
    eXosip_message_build_request(msg, method, to, from, route);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }
    return true;
}

bool make_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock();
    eXosip_message_build_answer(tid, status, msg);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }
    return true;
}

void send_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message *msg)
{
    if(!msg)
        eXosip_lock();
    eXosip_message_send_answer(tid, status, msg);
    eXosip_unlock();
}

bool make_answer_invite(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg)
{
    if(!msg)
        return false;
    *msg = NULL;
    eXosip_lock();
    eXosip_call_build_answer(tid, status, msg);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }
    return true;
}

void send_answer_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message *msg)
{
    if(!msg)
        eXosip_lock();
    eXosip_call_send_answer(tid, status, msg);
    eXosip_unlock();
}


void send_request_message(sip_context_t ctx, osip_message_t *msg)
{
    if(!msg)
        return;

    eXosip_message_send_request(msg);
    eXosip_unlock();
}

sip_reg_t make_registry_request(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg) 
{
    if(!msg)
        return -1;

    *msg = NULL;
    eXosip_lock();
    sip_reg_t rid = eXosip_register_build_initial_register(uri, s, c, exp, msg);
    if(!msg)
        eXosip_unlock();
    return rid;
}

void send_registry_message(sip_context_t c, sip_reg_t r, osip_message_t *msg) 
{
    if(!msg)
        return;
    eXosip_register_send_register(r, msg);
    eXosip_unlock();
}

bool sip_release_registry(sip_context_t ctx, sip_reg_t rid)
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

void sip_release_call(sip_context_t ctx, sip_call_t cid, sip_dlg_t did)
{
    eXosip_lock();
    eXosip_call_terminate(cid, did);
    eXosip_unlock();
}

bool make_dialog_request(sip_context_t ctx, sip_dlg_t did, const char *method, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock();
    eXosip_call_build_request(did, method, msg);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }

    return true;
}

bool make_dialog_notify(sip_context_t ctx, sip_dlg_t did, int status, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock();
    eXosip_call_build_notify(did, status, msg);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }

    return true;
}

bool make_dialog_update(sip_context_t ctx, sip_dlg_t did, osip_message_t **msg)
{
    if(!msg)
        return false;

    *msg = NULL;
    eXosip_lock();
    eXosip_call_build_update(did, msg);
    if(!*msg) {
        eXosip_unlock();
        return false;
    }

    return true;
}

void send_dialog_message(sip_context_t ctx, sip_dlg_t did, osip_message_t *msg)
{
    if(!msg)
        return;

    eXosip_call_send_request(did, msg);
    eXosip_unlock();
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

