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
 * Some common threads.
 * This offers a generic interface to support threads drivers may commonly
 * use.  At the moment, this is mostly the background thread that is used
 * for timing purposes, but other common thread objects may be added in
 * the future.
 * @file bayonne/thread.h
 */

#ifndef _BAYONNE_THREAD_H_
#define	_BAYONNE_THREAD_H_

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

#ifndef	_BAYONNE_REGISTRY_H_
#include <bayonne/registry.h>
#endif

#ifndef	_BAYONNE_TIMESLOT_H_
#include <bayonne/timeslot.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class __EXPORT Background : public DetachedThread, public Conditional
{
public:
	Background(size_t stack);
	~Background();

	virtual void automatic(void);

	static void schedule(timeout_t slice, int priority = 0);

	static timeout_t schedule(void);

	static void shutdown(void);

	static void notify(void);

private:
	/**
	 * Overriden to disables object delete on thread exit...
	 */
	void exit(void);

	void run(void);
};

END_NAMESPACE

#endif
