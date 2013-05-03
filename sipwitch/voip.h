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
typedef unsigned sip_reg_t;
typedef	unsigned long sip_timeout_t;

sip_reg_t sip_create_registration(sip_context_t ctx, const char *uri, const char *s, const char *c, unsigned exp, osip_message_t **msg);
void sip_send_registration(sip_context_t ctx, sip_reg_t rid, osip_message_t *msg);
bool sip_release_registration(sip_context_t ctx, sip_reg_t rid);

void sip_add_authentication(sip_context_t ctx, const char *user, const char *secret, const char *realm, bool automatic = false);

void sip_default_action(sip_context_t ctx, sip_event_t ev);

sip_event_t sip_get_event(sip_context_t ctx, sip_timeout_t timeout);
void sip_release_event(sip_event_t ev);

#ifndef SESSION_EXPIRES
#define SESSION_EXPIRES "session-expires"
#endif

#ifndef ALLOW_EVENTS
#define ALLOW_EVENTS    "allow-events"
#endif
