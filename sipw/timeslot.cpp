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

namespace bayonne {

timeslot::timeslot() : Timeslot()
{
	timer = Timer::inf;
	ctx = NULL;
}

void timeslot::disarm()
{
	timer = Timer::inf;
}

void timeslot::arm(timeout_t timeout)
{
	timer = timeout;
	background::notify();
}

timeout_t timeslot::getExpires(time_t now)
{
	mutex.lock();
	expires = timer.get();

	// if triggered, reset...
	if(expires == 0) {
		timer = Timer::inf;
		return Timer::inf;
	}

	return expires;
}

void timeslot::allocate(long crn, statmap::stat_t stat, Registration *reg)
{
	// make sure no call session is "active" when we allocate...
	tid = did = -1;
	ctx = ((registration *)(reg))->getContext();
	if(!ctx)
		ctx = driver::out_context;
	Timeslot::allocate(crn, stat, reg);
}

int timeslot::incoming(voip::event_t sevent)
{
	Script *scr = NULL;
	registration *reg = (registration *)registry;
	const char *cp;
	osip_uri_t *to = NULL, *from = NULL;
	size_t len = 0;
	const char *entry = "@local";
	bool remote = false, diverted = false;
	char uri[256];
	const char *targets = reg->getTargets();
	const char *scrname = reg->getScript();
	const char *localnames = reg->getLocalnames();
	const char *caller = NULL, *dialed = NULL;
	osip_via_t *via = NULL;
	int from_port = 5060, via_port = 5060, to_port = 5060;

	tid = sevent->tid;
	did = sevent->did;
	setMapped('i', "incoming");
	arm(16000);

	if(!reg)
		return SIP_NOT_FOUND;

	if(sevent->request->to && sevent->request->to->url && sevent->request->to->url->username)
		to = sevent->request->to->url;

	if(sevent->request->from && sevent->request->from->url && sevent->request->from->url->username)
		from = sevent->request->from->url;

	if(osip_list_eol(OSIP2_LIST_PTR sevent->request->vias, 0) == 0) 
		via = (osip_via_t *)osip_list_get(OSIP2_LIST_PTR sevent->request->vias, 0);

	if(from && from->port && from->port[0])
		from_port = atoi(from->port);
	
	if(to && to->port && to->port[0])
		to_port = atoi(to->port);

	if(via && via->port && via->port[0])
		via_port = atoi(via->port);

	if(via && from) {
		remote = true;
		if(eq(from->host, via->host) && from_port == via_port)
			remote = false;
	}
	else if(from && to) {
		remote = true;
		if(eq(from->host, to->host) && from_port == to_port)
			remote = false;
	}
	
	if(remote && localnames && from && from->host && from->host[0]) {
		cp = strstr(localnames, from->host);
		if(cp)
			len = strlen(from->host);

		if(len && (isspace(cp[len]) || strchr(",;:", cp[len]) || !cp[len])) 
			remote = false;
	}

	if(remote)
		entry = "@remote";

	if(to && targets) {
		cp = strstr(targets, to->username);
		if(cp)
			len = strlen(to->username);

		if(len && (isspace(cp[len]) || strchr(",;:", cp[len]) || !cp[len])) {
			scrname = to->username;
			goto attach;
		}
	
		// detect diversion even if no diversion header....
		entry = "@divert";
		diverted = true;
	}

	// script= entry is our default script for the request uri...
	// special script names decline, none,reject, and busy are immediate error...

	if(!scrname || eq(scrname, "decline"))
		return SIP_DECLINE;

	if(eq(scrname, "none"))
		return SIP_NOT_FOUND;

	if(eq(scrname, "busy"))
		return SIP_BUSY_HERE;

	if(eq(scrname, "reject"))
		return SIP_FORBIDDEN;

attach:
	scr = Driver::getIncoming(scrname);
	if(!scr) {
		shell::log(shell::ERR, "%s: script not found", scrname);
		return SIP_NOT_FOUND;
	}

	// if we have a script, we setup default symbols, then try attach...

	Timeslot::initialize();

	if(remote && from->port && from->port[0]) {
		snprintf(uri, sizeof(uri), "%s:%s@%s:%s",
			from->scheme, from->username, from->host, from->port);
		setConst("caller", uri);
		caller = uri;
	}
	else if(remote) {
		snprintf(uri, sizeof(uri), "%s:%s@%s",
			from->scheme, from->username, from->host);
		setConst("caller", uri);
		caller = uri;
	}
	else if(from) {
		setConst("caller", from->username);
//		identity(from->username, "callerinfo");
		caller = from->username;
	}

	if(to) {
		setConst("dialed", to->username);
		dialed = to->username;
	}

	if(caller)
		String::set(mapped->source, sizeof(mapped->source), caller);
	if(dialed)
		String::set(mapped->target, sizeof(mapped->target), dialed);

	String::set(mapped->script, sizeof(mapped->script), scrname);

	if(diverted)
		mapped->type = mapped_t::DIVERT;
	else if(remote)
		mapped->type = mapped_t::REMOTE;
	else
		mapped->type = mapped_t::LOCAL;

	setConst("script", scrname);
	setConst("server", reg->getServer());
	
//	if(diverted && to)
//		identity(to->username);
//	else if(!remote && from)
//		identity(from->username);

	if(!attach(scr, entry)) {
		scr->release();
		shell::log(shell::ERR, "%s: cannot start %s", scrname, entry);
		return SIP_NOT_FOUND;
	}
	else	// getIncoming did extra retain to preserve until attached...
		scr->release();

	// we send ringback confirmation from script itself, hence script
	// can also reject (decline) the call, so no reply needed...

	shell::debug(3, "timeslot %d: running %s for %s", instance, scrname, entry); 
	setScripting();
	return 0;
}

void timeslot::drop(void)
{
	// if active connection, we terminate...
	if(connected) {	
		voip::release_call(ctx, cid, did);
		connected = false;
	}
	// if pending transaction, we decline...
	else if(tid != -1)
		voip::send_response_message(ctx, tid, SIP_DECLINE, NULL);
	tid = -1;
	// no hangup timer needed for exosip, it will give timed release...
	disarm();
}

void timeslot::disconnect(event_t *event)
{
	// calling party had disconnected, no need for us to also hangup...
	// no need to set release timer, either, since exosip does so automatically

	connected = false;
	tid = did = -1;			
	disarm();
	Timeslot::disconnect(event);
}

void timeslot::hangup(event_t *event)
{
	drop();
	disarm();
	Timeslot::disconnect(event);
}

void timeslot::release(event_t *event)
{
	// If all is correct we either had a disconnect event first since
	// the remote end terminated, or we explicitly dropped the call earlier.
	// one exceptional possibility is we setup a trying with a timer, and
	// the session timed out without further state updates, in which case
	// we should decline.
	drop();
	Timeslot::release(event);
}

void timeslot::disable(event_t *event)
{
	event->id = Timeslot::REJECT;
}

} // end namespace
