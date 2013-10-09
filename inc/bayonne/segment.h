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

/**
 * Basic segment objects.
 * A segment is something, like a telephony board or a isdn span, that
 * can control a collection (range) of Bayonne timeslots.
 * @file bayonne/segment.h
 */

#ifndef _BAYONNE_SEGMENT_H_
#define	_BAYONNE_SEGMENT_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef	_BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

#ifndef	_BAYONNE_STATS_H_
#include <bayonne/stats.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

/**
 * Common segment base class for boards and spans.
 * This is used to collect a group (range) of timeslots.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Segment
{
protected:
	bool live;
	unsigned instance;
	unsigned first, count;
	const char *description;
	const char *script;
	statmap *stats;

	Segment(unsigned id);

public:
	virtual const char *suspend(void);

	virtual const char *resume(void);

	void assign(statmap::stat_t stat);

	void release(statmap::stat_t stat);

	inline unsigned getCount(void) const
		{return count;};

	inline unsigned getFirst(void) const
		{return first;};

	inline unsigned getInstance(void) const
		{return instance;};

	inline bool getLive(void) const
		{return live;};

	inline const char *getDescription(void) const
		{return description;};

	inline const char *getScript(void) const
		{return script;};
};		

class __EXPORT Board : public Segment
{
protected:
	unsigned spans;

public:
	const char *suspend(void);

	const char *resume(void);

	Board();	

	inline unsigned getTimeslots(void)
		{return count;};

	inline unsigned getSpans(void)
		{return spans;};

	static void allocate(void);
};

class __EXPORT Span : public Segment
{
public:
	Span();

	void hangup(void);
};

END_NAMESPACE

#endif
