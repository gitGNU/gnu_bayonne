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

#pragma GCC diagnostic ignored "-Wvariadic-macros"
#undef  HAVE_CONFIG_H

#ifdef  WIN32
#undef  alloca
#endif

#include <eXosip2/eXosip.h>
#include <ortp/ortp.h>

#if defined(EXOSIP_OPT_BASE_OPTION) && !defined(EXOSIP_OPT_DONT_SEND_101)
#define EXOSIP_API4
#endif

#ifdef  EXOSIP_API4
typedef eXosip_t    *sip_context_t;
inline  void sip_lock(sip_context_t ctx) {eXosip_lock(ctx);}
inline  void sip_release(sip_context_t ctx) {eXosip_unlock(ctx);}
#else
typedef void        *sip_context_t;
inline  void sip_lock(sip_context_t ctx) {eXosip_lock();}
inline  void sip_release(sip_context_t ctx) {eXosip_unlock();}
#endif

typedef eXosip_event_t  *sip_event_t;
typedef int sip_reg_t;		// registration id
typedef	int	sip_tran_t;		// transaction id
typedef	int sip_dlg_t;		// dialog id
typedef	int	sip_call_t;		// call id
typedef	unsigned long sip_timeout_t;

bool make_request_message(sip_context_t ctx, osip_message_t **msg, const char *method, const char *to, const char *from, const char *route = NULL);
bool make_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg);
void send_request_message(sip_context_t ctx, osip_message_t *msg);
void send_response_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t *msg = NULL);

bool make_answer_invite(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t **msg);
void send_answer_message(sip_context_t ctx, sip_tran_t tid, int status, osip_message_t *msg = NULL);

void sip_release_call(sip_context_t ctx, sip_call_t cid, sip_dlg_t did);

bool make_dialog_request(sip_context_t ctx, sip_dlg_t did, const char *method, osip_message_t **msg);
bool make_dialog_notify(sip_context_t ctx, sip_dlg_t did, int status, osip_message_t **msg);
bool make_dialog_update(sip_context_t ctx, sip_dlg_t did, osip_message_t **msg);
void send_dialog_message(sip_context_t ctx, sip_dlg_t did, osip_message_t *msg);

sip_reg_t make_registry_request(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg);
void send_registry_message(sip_context_t ctx, sip_reg_t rid, osip_message_t *msg);
bool sip_release_registry(sip_context_t ctx, sip_reg_t rid);

void sip_add_authentication(sip_context_t ctx, const char *user, const char *secret, const char *realm, bool automatic = false);

void sip_default_action(sip_context_t ctx, sip_event_t ev);
void sip_automatic_action(sip_context_t ctx);

sip_event_t sip_get_event(sip_context_t ctx, sip_timeout_t timeout);
void sip_release_event(sip_event_t ev);

void sip_setup(const char *agent, int family = AF_INET);
bool sip_listen(sip_context_t ctx, int proto = IPPROTO_UDP, const char *iface = NULL, unsigned port = 5060, bool tls = false);

#ifndef SESSION_EXPIRES
#define SESSION_EXPIRES "session-expires"
#endif

#ifndef ALLOW_EVENTS
#define ALLOW_EVENTS    "allow-events"
#endif
