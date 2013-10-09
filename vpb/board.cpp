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

static unsigned tsid = 0;
static unsigned spcount = 0;

board::board() : Board()
{
	first = tsid;
	count = vpb_get_ports_per_card(instance);
	vpb_get_card_info(instance, &cardinfo);

	// perhaps we will get spans by model and/or port count for vpb...
	// no clear/entirely clean way to do so at startup...

	shell::log(shell::INFO, "board %d %s; timeslots=%d, spans=%d",
		instance + 1, cardinfo.model, count, spans);
	tsid += count;

	// if a span card, then we track range by spans, not timeslots...

	if(spans) {
		first = spcount;
		spcount += spans;
	}
}

// used to get final timeslot count for driver...
unsigned board::timeslots(void)
{
	return tsid;
}

// used to get final span count for driver...
unsigned board::allspans(void)
{
	return spcount;
}
