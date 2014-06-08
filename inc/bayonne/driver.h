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
 * Core driver management interface.
 * This is the core class for building a Bayonne driver instance.  This
 * includes management of the driver's timeslots, and server operations
 * which might be derived into driver specific implimentations.  Management
 * of server components through callbacks also happens here.
 * @file bayonne/driver.h
 */

#ifndef _BAYONNE_DRIVER_H_
#define _BAYONNE_DRIVER_H_

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

#ifndef _BAYONNE_SERVER_H_
#include <bayonne/server.h>
#endif

#ifndef _BAYONNE_STATS_H_
#include <bayonne/stats.h>
#endif

#ifndef _BAYONNE_DBI_H_
#include <bayonne/dbi.h>
#endif

#ifndef _BAYONNE_TIMESLOT_H_
#include <bayonne/timeslot.h>
#endif

#ifndef _BAYONNE_SEGMENT_H_
#include <bayonne/segment.h>
#endif

#ifndef BAYONNE_REGISTRY_H_
#include <bayonne/registry.h>
#endif

namespace bayonne {

/**
 * A common base class for all telephony drivers.
 * A driver is an adaption of Bayonne to a specific telephony API.  The
 * driver manages all timeslot and special resources associated with the
 * telephony API being used.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Driver : public keyfile, protected Env
{
private:
    keyfile regfile;

public:
    /**
     * Callback class for supporting plugins and services.
     * Services are derived from the Driver callback class, as this class
     * manages their startup and shutdown.  Plugins typically have their
     * own threads which are created and destroyed based on driver startup.
     * This is part of the driver since the management of service threads
     * occurs as a subset of the driver process initialization.
     *
     * The driver also holds the currently active configuration for the
     * running server, as parsed from /etc/bayonne.conf or ~/.bayonnerc.  A
     * new driver instance is created when a server config reload request
     * is made, and then replaces the last active instance.  This allows
     * for runtime dynamic reconfiguration of most server properties.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT callback : public LinkedObject
    {
    protected:
        friend class Driver;

        /**
         * Initialize callback instance and add to linked list of callbacks.
         */
        callback();

        /**
         * Reload method called when the server reloads configure.
         * @param driver instance that holds new config keys.
         */
        virtual void reload(Driver *driver);

        /**
         * Start method for initializing callback service when Bayonne is
         * starting up.
         */
        virtual void start(void);

        /**
         * Stop method for shutting down the callback service when Bayonne
         * is being shut down.
         */
        virtual void stop(void);

        /**
         * DBI operations for plugins.
         * @param db record being processed.
         */
        virtual bool query(dbi *db);

        /**
         * Notify generic alarm and logging events.
         * @param level of alarm.
         * @param text of alarm message.
         */
        virtual void errlog(shell::loglevel_t level, const char *text);
    };

protected:
    /**
     * Used to hold instance of compiled Bayonne script.  These instances
     * are created during server startup and reload, and can be used to
     * dynamically change application scripting while live.  Script images
     * are reference counted, so existing calls will complete using a prior
     * script if the scripting is changed mid-call before an old script image
     * is released from memory.  The current implimentation of GNU Bayonnne
     * can allow a single script to be recompiled and replaced live without
     * having to reload the server and all scripts together.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT image : public LinkedObject
    {
    public:
        /**
         * Construct a compiled script instance on an image chain.
         * @param img to construct.
         * @param root of image chain to add to.
         * @param name identity of this script image.
         */
        image(Script *img, LinkedObject **root, const char *name);

        object_pointer<Script> ptr;
        const char *id;
        const char *path;
    };

    object_pointer<Script> image_definitions;
    LinkedObject *image_services;
    LinkedObject *activations;      // dynamic registry list...

    /**
     * Construct driver instance singleton.
     * This holds the core driver for a Bayonne server.  The name is used
     * for parsing a definition call script, name.dcs, which may be driver
     * specific since drivers can extend the core library scripting language
     * for access to telephony API specific features and functions.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    Driver(const char *dname);

    /**
     * Destroy driver during server shutdown.
     */
    virtual ~Driver();

    static LinkedObject *registrations;
    static condlock_t locking;
    static Driver *active;
    static caddr_t timeslots;
    static caddr_t boards;
    static caddr_t spans;
    static unsigned board_count;
    static unsigned board_alloc;
    static unsigned span_count;
    static unsigned span_alloc;
    static unsigned ts_count;
    static unsigned ts_alloc;
    static OrderedIndex idle;
    static timeout_t stepping;
    static statmap *stats;
    static const char *encoding;

    // these are protected because they are called from derived classes
    // start and stop static members, not externally..

    /**
     * Start the driver and any service plugin callbacks.
     */
    static void start(void);

    /**
     * Stop the driver and any service plugin callbacks.
     */
    static void stop(void);

public:
    /**
     * Extends fifo control commands for driver specific features.  This can
     * be used for adding span management functions for ISDN/SS7 driver
     * adaptions, or access to other telephony API specific features in
     * Bayonne derived servers.
     * @param parsed fifo command options.
     * @param pid of process to reply to.
     * @return NULL if successful, else error message.
     */
    virtual const char *dispatch(char **argv, int pid);

    /**
     * Used to create singleton driver as derived class.  This is used during
     * reload to create a new configuration instance while live, as well as
     * to create the initial driver instance at server startup.  The object is
     * returned as the Driver base class, but really is the derived and
     * telephony API specific instance.  It is an abstract method and must
     * be implimented for Bayonne to work.
     * @return a new derived driver instance cast back as Driver base class.
     */
    virtual Driver *create(void) = 0;

    /**
     * Called during runtime reload to reconfigure the derived driver.
     * @param first key of registration list.
     */
    virtual void update(void);

    /**
     * Get the current active driver instance and active configuration.
     * @return driver instance that is active.
     */
    static Driver *get(void);

    /**
     * Release an older driver instance when a reload request happens.
     * @param driver instance to release.
     */
    static void release(Driver *driver);

    /**
     * Release a keydata based lock.
     * @param keydata instance to release for.
     */
    static void release(keydata *keys);

    /**
     * Release a script image.
     * @param img to release.
     */
    static void release(Script *img);

    /**
     * Get incoming script by name.
     * @param name of incoming script.
     * @return image or NULL if not found.
     */
    static Script *getIncoming(const char *name);

    /**
     * Get outgoing script by name.
     * @param name of outgoing script.
     * @return image or NULL if not found.
     */
    static Script *getOutgoing(const char *name);

    /**
     * Commit a new configuration as the "active" configuration.
     * @param driver instance committed.
     * @param first registry key to use.
     */
    static void commit(Driver *driver);

    /**
     * Perform server reload operation.
     */
    static void reload(void);

    /**
     * Unwind and produce compiler error messages for a compiled script image.
     * @param img to get error compile-time messages from.
     * @return count of errors for this or 0 if none.
     */
    static unsigned errors(Script *img);

    /**
     * Get a timeslot object for this driver.
     * @param index of timeslot to request (0-n).
     * @return NULL if no such timeslot, else base class object reference.
     */
    static Timeslot *get(unsigned index);

    /**
     * Get a board object for this driver.
     * @param index of board to request (0-n).
     * @return NULL if no such timeslot, else base class object reference.
     */
    static Board *getBoard(unsigned index);

    /**
     * Get a span object for this driver.
     * @param index of board to request (0-n).
     * @return NULL if no such timeslot, else base class object reference.
     */
    static Span *getSpan(unsigned index);

    /**
     * Get count of timeslots associated with the driver.
     * @return timeslot count.
     */
    inline static unsigned getCount(void)
        {return ts_count;}

    inline static unsigned getBoards(void)
        {return board_count;}

    inline static unsigned getSpans(void)
        {return span_count;}

    /**
     * Get driver script stepping rate.
     */
    inline static timeout_t getStepping(void)
        {return stepping;}

    /**
     * Assign driver stat.
     * @param type of stat to assign.
     */
    static void assign(statmap::stat_t stat);

    /**
     * Release driver stat.
     * @param type of stat to release.
     */
    static void release(statmap::stat_t stat);

    /**
     * Dispatch a dbi event through plugins.
     * @param logfile to write for generic call log data.
     * @param data for query or call detail logging.
     */
    static void query(FILE *logfile, dbi *data);

    /**
     * Dispatch generic logging events through plugins.
     * @param level of error event.
     * @param text of error event.
     */
    static void errlog(shell::loglevel_t level, const char *text);

    inline keydata *registry(void)
        {return regfile.begin();}
};

} // end namespace

#endif
