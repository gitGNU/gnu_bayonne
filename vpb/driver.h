// Copyright (C) 2009 David Sugar, Tycho Softworks.
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

#include <bayonne/bayonne.h>
#include <vpbapi.h>
#include <bayonne-config.h>

#ifdef	HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class __LOCAL board : public Board
{
public:
	struct VPB_CARD_INFO cardinfo;

	board();

	static unsigned timeslots(void);
	static unsigned allspans(void);
};

class __LOCAL timeslot : public Timeslot
{
public:
	timeslot();

	timeout_t getExpires(time_t now);

private:
	int handle;
	void *timer;
	bool inTimer;
	float ingain, outgain;
	Board *board;
	Span *span;

	void disarm(void);
	void arm(timeout_t timeout);
	void drop(void);
	void allocate(long cid);
	void disconnect(event_t *event);
	void hangup(event_t *event);
	void release(event_t *event);
	void disable(event_t *event);
	void enable(event_t *event);
};

class __LOCAL driver : public Driver
{
public:
	driver();
	
	Driver *create(void);
	void update(void);
	const char *dispatch(char **argv, int pid);

	static void start(void);
	static void stop(void);
};

END_NAMESPACE
