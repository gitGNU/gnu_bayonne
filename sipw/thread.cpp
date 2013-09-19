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

static bool shutdown_flag = false;
static unsigned active_count = 0;
static unsigned startup_count = 0;
static unsigned shutdown_count = 0;

#if 0
static char *remove_quotes(char *c)
{
    assert(c != NULL);

    char *o = c;
    char *d = c;
    if(*c != '\"')
        return c;

    ++c;

    while(*c)
        *(d++) = *(c++);

    *(--d) = 0;
    return o;
}
#endif

thread::thread(sip::context_t source, size_t size, const char *type) : DetachedThread(size)
{
    context = source;
    instance = type;
}

const char *thread::eid(eXosip_event_type ev)
{
    switch(ev) {
#ifndef EXOSIP_API4
    case EXOSIP_REGISTRATION_REFRESHED:
#endif
    case EXOSIP_REGISTRATION_SUCCESS:
        return "register";
    case EXOSIP_CALL_INVITE:
        return "invite";
    case EXOSIP_CALL_REINVITE:
        return "reinvite";
#ifndef EXOSIP_API4
    case EXOSIP_CALL_TIMEOUT:
        return "timeout";
#endif
    case EXOSIP_CALL_NOANSWER:
    case EXOSIP_SUBSCRIPTION_NOANSWER:
    case EXOSIP_NOTIFICATION_NOANSWER:
        return "noanswer";
    case EXOSIP_MESSAGE_PROCEEDING:
    case EXOSIP_NOTIFICATION_PROCEEDING:
    case EXOSIP_CALL_MESSAGE_PROCEEDING:
    case EXOSIP_SUBSCRIPTION_PROCEEDING:
    case EXOSIP_CALL_PROCEEDING:
        return "proceed";
    case EXOSIP_CALL_RINGING:
        return "ring";
    case EXOSIP_MESSAGE_ANSWERED:
    case EXOSIP_CALL_ANSWERED:
    case EXOSIP_CALL_MESSAGE_ANSWERED:
    case EXOSIP_SUBSCRIPTION_ANSWERED:
    case EXOSIP_NOTIFICATION_ANSWERED:
        return "answer";
    case EXOSIP_SUBSCRIPTION_REDIRECTED:
    case EXOSIP_NOTIFICATION_REDIRECTED:
    case EXOSIP_CALL_MESSAGE_REDIRECTED:
    case EXOSIP_CALL_REDIRECTED:
    case EXOSIP_MESSAGE_REDIRECTED:
        return "redirect";
#ifndef EXOSIP_API4
    case EXOSIP_REGISTRATION_TERMINATED:
#endif
    case EXOSIP_REGISTRATION_FAILURE:
        return "noreg";
    case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
    case EXOSIP_NOTIFICATION_REQUESTFAILURE:
    case EXOSIP_CALL_REQUESTFAILURE:
    case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
    case EXOSIP_MESSAGE_REQUESTFAILURE:
        return "failed";
    case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
    case EXOSIP_NOTIFICATION_SERVERFAILURE:
    case EXOSIP_CALL_SERVERFAILURE:
    case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
    case EXOSIP_MESSAGE_SERVERFAILURE:
        return "server";
    case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
    case EXOSIP_NOTIFICATION_GLOBALFAILURE:
    case EXOSIP_CALL_GLOBALFAILURE:
    case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
    case EXOSIP_MESSAGE_GLOBALFAILURE:
        return "global";
    case EXOSIP_CALL_ACK:
        return "ack";
    case EXOSIP_CALL_CLOSED:
    case EXOSIP_CALL_RELEASED:
        return "bye";
    case EXOSIP_CALL_CANCELLED:
        return "cancel";
    case EXOSIP_MESSAGE_NEW:
    case EXOSIP_CALL_MESSAGE_NEW:
    case EXOSIP_IN_SUBSCRIPTION_NEW:
        return "new";
    case EXOSIP_SUBSCRIPTION_NOTIFY:
        return "notify";
    default:
        break;
    }
    return "unknown";
}

void thread::run(void)
{
     ++startup_count;
    shell::log(DEBUG1, "starting event thread %s", instance);

    for(;;) {
        if(!shutdown_flag)
            sevent = sip::get_event(context, 1000);

        if(shutdown_flag) {
            shell::log(DEBUG1, "stopping event thread %s", instance);
            ++shutdown_count;
            return; // exits event thread...
        }

        if(!sevent)
            continue;

        ++active_count;
        shell::debug(2, "sip: event %s(%d); cid=%d, did=%d, instance=%s",
            eid(sevent->type), sevent->type, sevent->cid, sevent->did, instance);

        switch(sevent->type) {
        case EXOSIP_REGISTRATION_SUCCESS:
#ifndef EXOSIP_API4
        case EXOSIP_REGISTRATION_REFRESHED:
#endif
            reg = driver::locate(sevent->rid);
            if(reg)
                reg->activate();
            break;
#ifndef EXOSIP_API4
        case EXOSIP_REGISTRATION_TERMINATED:
            reg = driver::locate(sevent->rid);
            if(reg)
                reg->release();
            break;
#endif
        case EXOSIP_REGISTRATION_FAILURE:
            reg = driver::locate(sevent->rid);
            if(!reg)
                break;
            if(sevent->response && sevent->response->status_code == 401) {
                char *sip_realm = NULL;
                osip_proxy_authenticate_t *prx_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR sevent->response->proxy_authenticates, 0);
                osip_www_authenticate_t *www_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR sevent->response->www_authenticates,0);
                if(prx_auth)
                    sip_realm = osip_proxy_authenticate_get_realm(prx_auth);
                else if(www_auth)
                    sip_realm = osip_www_authenticate_get_realm(www_auth);
                sip_realm = String::unquote(sip_realm, "\"\"");
                reg->authenticate(sip_realm);
            }
            else
                reg->release();
            break;
        default:
            sip::default_action(context, sevent);
        }

        sip::release_event(sevent);
        --active_count;
    }
}

void thread::shutdown(void)
{
    shutdown_flag = true;
    while(active_count)
        Thread::sleep(50);
    while(shutdown_count < startup_count)
        Thread::sleep(50);
}

