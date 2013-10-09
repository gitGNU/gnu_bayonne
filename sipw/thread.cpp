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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static bool shutdown_flag = false;
static unsigned shutdown_count = 0;
static unsigned startup_count = 0;
static unsigned active_count = 0;

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

static const char *eid(eXosip_event_type ev)
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

thread::thread(voip::context_t source, size_t stack, const char *type) :
DetachedThread(stack)
{
    context = source;
    instance = type;
}

void thread::activate(int priority, size_t stack) 
{
	timeout_t timing = background::schedule();
	unsigned count = 0;
	thread *t;

	new background(stack);
	background::schedule(timing, 0);	

	if(driver::udp_context) {
		++count;
        t = new thread(driver::udp_context, stack, "udp");
        t->start(priority);
    }

	if(driver::tcp_context) {
		++count;
        t = new thread(driver::tcp_context, stack, "tcp");
        t->start(priority);
    }

	if(driver::tls_context) {
		++count;
        t = new thread(driver::tls_context, stack, "tls");
        t->start(priority);
    }

	while(startup_count < count)
		Thread::sleep(50);
}

void thread::shutdown(void)
{
	shutdown_flag = true;
	Background::shutdown();
	while(active_count)
		Thread::sleep(50);
    
    voip::release(driver::tcp_context);
    voip::release(driver::udp_context);
    voip::release(driver::tls_context);
	while(shutdown_count < startup_count)
		Thread::sleep(50);
}

void thread::run(void)
{
	registration *reg;
	Timeslot *ts;
	Timeslot::event_t event;

	++startup_count;
	shell::debug(1, "starting thread %s", instance);

	for(;;) {
		assert(instance > 0);
		
		if(!shutdown_flag)
			sevent = voip::get_event(context, background::schedule());

		if(shutdown_flag) {
			shell::debug(1, "stopping thread %s", instance);
			++shutdown_count;
			return; // exit thread...
		}

		if(!sevent)
			continue;

		++active_count;
        shell::debug(2, "sip: event %s(%d); cid=%d, did=%d, instance=%s",
            eid(sevent->type), sevent->type, sevent->cid, sevent->did, instance);

		switch(sevent->type) {
		case EXOSIP_REGISTRATION_FAILURE:
			reg = driver::locate(sevent->rid);
			if(sevent->response && sevent->response->status_code == 401) {
				char *sip_realm = NULL;
				osip_proxy_authenticate_t *prx_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR sevent->response->proxy_authenticates, 0);
				osip_www_authenticate_t *www_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR sevent->response->www_authenticates,0);
				if(prx_auth)
					sip_realm = osip_proxy_authenticate_get_realm(prx_auth);
				else if(www_auth)
					sip_realm = osip_www_authenticate_get_realm(www_auth);
				sip_realm = String::unquote(sip_realm, "\"\"");
				if(reg)
					reg->authenticate(sip_realm);
			}
			else if(reg)
				reg->failed();
			break;
#ifndef  EXOSIP_API4
		case EXOSIP_REGISTRATION_TERMINATED:
			reg = driver::locate(sevent->rid);
			if(reg)
				reg->cancel();
			break;

		case EXOSIP_REGISTRATION_REFRESHED:
#endif
		case EXOSIP_REGISTRATION_SUCCESS:
			reg = driver::locate(sevent->rid);
			if(reg)
				reg->confirm();
			break;
		case EXOSIP_CALL_INVITE:
			if(!sevent->request || sevent->cid < 1) {
				shell::log(shell::WARN, "invalid invite received");
				break;
			}
			invite();
			break;
		case EXOSIP_CALL_CLOSED:
			ts = Timeslot::get(sevent->cid);
			if(ts) {
				event.id = Timeslot::DROP;
				ts->post(&event);
			}
			break;
		case EXOSIP_CALL_RELEASED:
			ts = Timeslot::get(sevent->cid);
			if(ts) {
				event.id = Timeslot::RELEASE;
				ts->post(&event);
			}
			break; 
		default:
			shell::log(shell::WARN, "unsupported message %d", sevent->type);
		}
		
        voip::release_event(sevent);
		--active_count;
	}
}

void thread::invite(void)
{
	timeslot *ts = NULL;
	const char *realm = driver::realm();
	int error = SIP_UNDECIPHERABLE;
	const char *uuid;
	osip_authorization_t *auth = NULL;
	voip::msg_t reply = NULL;
	char nonce[32];
	time_t now;

	error = SIP_UNDECIPHERABLE;
	if(!sevent->request || !sevent->request->to || !sevent->request->from || !sevent->request->req_uri)
		goto reply;

	error = SIP_ADDRESS_INCOMPLETE;
	uuid = sevent->request->req_uri->username;
	if(!uuid || !*uuid)
		goto reply;

	registry = driver::contact(uuid);
	error = SIP_NOT_FOUND;
	if(!registry)
		goto reply;

	// first we allocate the timeslot stat while checking the registry limit...
	error = SIP_TEMPORARILY_UNAVAILABLE;
	if(!registry->Registration::attach(statmap::INCOMING))
		goto reply;

	ts = static_cast<timeslot*>(Timeslot::assign(registry, statmap::INCOMING, sevent->cid));
	if(!ts) {
		// release registry stat allocation if no timeslots since stat was used...
		registry->Registration::release(statmap::INCOMING);
		goto reply;
	}

	error = SIP_FORBIDDEN;
	if(realm) {

		if(!sevent->request || osip_message_get_authorization(sevent->request, 0, &auth) != 0 || !auth || !auth->username || !auth->response) 
			goto challenge;

		remove_quotes(auth->username);
		remove_quotes(auth->uri);
		remove_quotes(auth->nonce);
		remove_quotes(auth->response);

		if(!eq(auth->username, registry->getAuth()))
			goto reply;

		// CHECK MD5 SUMS!!!
	}
	error = ts->incoming(sevent);

reply:
	if(error) {
		shell::debug(1, "rejecting invite; error=%d", error);
        voip::send_answer_response(context, sevent->tid, error, NULL);
	}
	return;

challenge:
	shell::debug(1, "challenge required");

	time(&now);
	snprintf(nonce, sizeof(nonce), "%08lx", now);
	snprintf(buffer, sizeof(buffer),
		"Digest realm=\"%s\", nonce=\"%s\", algorithm=MD5",	realm, nonce);
    if(voip::make_answer_response(context, sevent->tid, SIP_UNAUTHORIZED, &reply)) {
		osip_message_set_header(reply, ALLOW, "INVITE, ACK, CANCEL, BYE, REFER, OPTIONS, NOTIFY, SUBSCRIBE, PRACK, MESSAGE, INFO");
		osip_message_set_header(reply, ALLOW_EVENTS, "talk, hold, refer");
		osip_message_set_header(reply, WWW_AUTHENTICATE, buffer);
        voip::send_answer_response(context, sevent->tid, SIP_UNAUTHORIZED, reply);
	}
}

background::background(size_t stack) : Background(stack)
{
}

void background::automatic(void)
{
    if(driver::udp_context)
        voip::automatic_action(driver::udp_context);
    if(driver::tcp_context)
        voip::automatic_action(driver::tcp_context);
    if(driver::tls_context)
        voip::automatic_action(driver::tls_context);
}

		
