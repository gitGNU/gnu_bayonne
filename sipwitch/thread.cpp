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

thread::thread(sip::context_t source, size_t size) : DetachedThread(size)
{
    context = source;
}

void thread::run(void)
{
    instance = ++startup_count;
    shell::log(DEBUG1, "starting event thread %d", instance);

    for(;;) {
        if(!shutdown_flag)
            sevent = sip::get_event(context, 1000);

        if(shutdown_flag) {
            shell::log(DEBUG1, "stopping event thread %d", instance);
            ++shutdown_count;
            return; // exits event thread...
        }

        if(!sevent)
            continue;

        ++active_count;
        shell::debug(2, "sip: event %d; cid=%d, did=%d, instance=%d",
            sevent->type, sevent->cid, sevent->did, instance);

        switch(sevent->type) {
        case EXOSIP_REGISTRATION_SUCCESS:
        case EXOSIP_REGISTRATION_REFRESHED:
            reg = driver::locate(sevent->rid);
            if(reg)
                reg->activate();
            break;
        case EXOSIP_REGISTRATION_TERMINATED:
            reg = driver::locate(sevent->rid);
            if(reg)
                reg->release();
            break;
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

