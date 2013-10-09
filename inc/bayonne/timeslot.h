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
 * Basic timeslot operation.
 * This is the generic timeslot interface of a Bayonne server.  This includes
 * the core Bayonne state machine and script engine binding.
 * @file bayonne/timeslot.h
 */

#ifndef _BAYONNE_TIMESLOT_H_
#define _BAYONNE_TIMESLOT_H_

#ifndef _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef _UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef _BAYONNE_NAMESPACE_H_
#include <bayonne/namespace.h>
#endif

#ifndef _BAYONNE_STATS_H_
#include <bayonne/stats.h>
#endif

#ifndef _BAYONNE_REGISTRY_H_
#include <bayonne/registry.h>
#endif

#ifndef _BAYONNE_SEGMENT_H_
#include <bayonne/segment.h>
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

#define TIMESLOT_MAP    "bayonne.tsm"

/**
 * Common timeslot base class for a Bayonne driver.
 * A given server has a number of timeslots, each of which handles a single
 * telephone call session.  The timeslot base class is derived and implimented
 * in the telephony API specific process as needed.  The core timeslot
 * includes state machine operations and generic telephony events which are
 * passed into it.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Timeslot : public LinkedObject, public Script::interp, protected Env
{
public:
    // events
    enum {REJECT = 0, SHUTDOWN, TIMEOUT, DROP, HANGUP, RELEASE, ENABLE, DISABLE};

    typedef struct {
        enum {NONE, DIALED, LOCAL, REMOTE, DIVERT, RECALL} type;
        time_t started;
        char state[16];
        char source[64];
        char target[64];
        char script[64];
    } mapped_t;

    typedef struct {
        unsigned id;
        union {
            int dialog;
            unsigned sequence;
            void *data;
        };
    } event_t;

    typedef void (Timeslot::*handler_t)(event_t *event);

protected:
    long cid;
    statmap::stat_t stats;
    mapped_t *mapped;
    timeout_t expires;
    handler_t handler;
    unsigned instance;
    unsigned sequence;
    unsigned transaction;
    unsigned rings;
    char *voice;
    char *digits;
    const char *reason;
    char *optional;         // optional strdup tuplets for dbi record
    bool connected, answered, tracing, traceflag;
    Registration *registry;
    Board *board;
    Span *span;
    Mutex mutex;

    /**
     * release event handler.
     * @param event message to process.
     */
    void releaseHandler(event_t *event);

    /**
     * idle event handler.
     * @param event message to process.
     */
    void idleHandler(event_t *event);

    /**
     * scripting step handler.
     * @param event message to process.
     */
    void scriptHandler(event_t *event);

    /**
     * Handle offline events.
     * @param event message to process.
     */
    void offlineHandler(event_t *event);

    /**
     * Contruct common base class.  This maps shared memory segment.
     */
    Timeslot();

    /**
     * Common initialization of channel.
     */
    void initialize(void);

    /**
     * Timeslot shutdown, for final disconnect.
     * @param event message of original event.
     */
    virtual void shutdown(event_t *event);

    /**
     * Release timeslot in response to an event.
     * @param event message of original event.
     */
    virtual void release(event_t *event);

    /**
     * Remote end has disconnected the call.
     * @param event message of original event.
     */
    virtual void disconnect(event_t *event);

    /**
     * We are hanging up the call.
     * @param event message of original event.
     */
    virtual void hangup(event_t *event);


    /**
     * Disable a timeslot (set to offline mode...).  Derived methods may
     * toggle span channel or port status, etc.
     * @param event message of original event.
     */
    virtual void disable(event_t *event);

    /**
     * Processing of timeslot attributes at assignment time.  This may
     * be modified or extended in a derived class.
     * @param cid of timeslot.
     * @param stat id (incoming or outgoing) we are allocating for.
     * @param registry the call is assigned under or NULL if none.
     */
    virtual void allocate(long cid, statmap::stat_t stat, Registration *registry = NULL);

    /**
     * Enable a disabled timeslot (return to idle list...).  Derived
     * method may re-enable port, etc.
     * @param event message of original event.
     */
    virtual void enable(event_t *event);

    /**
     * Process driver unique timeslot events while call is "live".
     * @param event message of original event.
     */
    virtual void running(event_t *event);

    /**
     * Disarm timeslot timer.
     */
    virtual void disarm(void) = 0;

    /**
     * Set timeslot timer.
     * @param timeout in milliseconds.
     */
    virtual void arm(timeout_t timeout) = 0;

    /**
     * Set channel state back to idle.
     */
    void setIdle(void);

    /**
     * Set scripting state and execute a step.
     */
    void setScripting(void);

public:
    /**
     * Assign an available timeslot by logical call id.  This is used by
     * pure network drivers.
     * @param registry we are using for this timeslot.
     * @param stat id of timeslot allocation.
     * @param cid of network session.
     * @return timeslot if available, NULL if all used.
     */
    static Timeslot *assign(Registration *registry, statmap::stat_t stat, long cid);

    /**
     * Assign physical channel timeslots.  A range is used, and the first
     * free timeslot in the range is found.  To select a single timeslot,
     * the ending timeslot can be the same as the starting.  Ranges are used
     * to extract a free channel in an isdn span or similar groupings.
     * @param stat id of timeslot allocation.
     * @param starting timeslot to search.
     * @param ending timslot to search.
     * @param cid of the call session (isdn crn...).
     * @return timeslot if available, NULL if none free in range.
     */
    static Timeslot *assign(statmap::stat_t stat, unsigned starting, unsigned ending, long cid);

    /**
     * Assign physical channel timeslots.  A range is used, and the first
     * free timeslot in the range is found.  To select a single timeslot,
     * the ending timeslot can be the same as the starting.  Ranges are used
     * to extract a free channel in an isdn span or similar groupings.
     * @param stat id of timeslot allocation.
     * @param starting timeslot to search.
     * @param ending timslot to search.
     * @return timeslot if available, NULL if none free in range.
     */
    static Timeslot *assign(statmap::stat_t stat, unsigned starting, unsigned ending);

    /**
     * Return sequence identity of timeslot.  Used to make each telephone
     * call "unique".
     * @return timeslot call sequence.
     */
    inline unsigned getSequence(void)
        {return sequence;}

    /**
     * Get instance (index) of timeslot.  Each timeslot has a unique index.
     * @return index of timeslot.
     */
    inline unsigned getInstance(void)
        {return instance;}

    /**
     * Get transaction id of timeslot.  This is used to give each tgi process
     * a unique transaction id.
     * @return transaction id currently in effect.
     */
    inline unsigned getTransaction(void)
        {return transaction;}

    /**
     * Get the registration entry associated with the timeslot.
     * @return associated registration entry or NULL if none.
     */
    inline Registration *getRegistry(void)
        {return registry;};

    /**
     * Get the board entry associated with the timeslot.
     * @return associated board entry or NULL if none.
     */
    inline Board *getBoard(void)
        {return board;};

    /**
     * Get the span entry associated with the timeslot.
     * @return associated span entry or NULL if none.
     */
    inline Span *getSpan(void)
        {return span;};

    /**
     * Set fields in shared memory segment.  Used by state handlers.
     * @param id code for Bayonne timeslot management protocol.
     * @param long name of state.
     */
    void setMapped(const char id, const char *state = NULL);

    /**
     * Post generic Bayonne event into a timeslot.
     * @param event message.
     */
    void post(event_t *event);

    /**
     * Get timeslot timeout expiration.  Returns 0 if expired.  May also be
     * used to manage other session timers.  This function holds a mutex
     * lock, and must be released by calling expire().
     * @param time_t of current time, so we do not have to call time().
     * @return timeout remaining or 0 if expired.
     */
    virtual timeout_t getExpires(time_t now);

    /**
     * Expiration handler to call after calling getExpires.  If the timer has
     * expired, it sends a TIMEOUT event into the state machine.  Releases
     * lock held by getExpires.
     */
    void expire(void);

    /**
     * Lookup a timeslot by logical call identifier.
     * @param cid to search for.
     * @return timeslot for this call identifier or NULL if none.
     */
    static Timeslot *get(long cid);
};

END_NAMESPACE

#endif
