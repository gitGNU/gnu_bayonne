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
 * Bayonne server management.
 * This offers a portable abstract interface class for server & ipc related
 * services that may be used by bayonne telephony servers.  This includes
 * management of the deamon environment, executing child processes, and basic
 * IPC services.  Command-line argument processing and basic server operations
 * are also included.  IPC services are offered through control ports which are
 * implimented as fifo's on Posix platforms, and as mailslots on w32.
 * @file bayonne/server.h
 */

#ifndef _BAYONNE_SERVER_H_
#define _BAYONNE_SERVER_H_

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

namespace bayonne {

/**
 * A container for all server related functions.
 * This is used to hold static member functions that are related to process
 * management.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT server
{
private:
    inline static void set(const char *id, const char *value)
        {args.setsym(id, value);}

    static size_t attach(void);

protected:
    friend class Env;

    static shell_t args;

public:
    /**
     * Server startup.  Called from main().
     */
    static void startup(shell::mainproc_t proc = NULL, bool detached = false);

    static void parse(int argc, char **argv, const char *dname);

    /**
     * Get file handle of control port, used for child tgi processes.
     * @return file handle of control.
     */
    static fd_t input(void);

    /**
     * Print/produce log message.
     * @param fmt string.
     */
    static void printlog(const char *fmt, ...) __PRINTF(1, 2);

    /**
     * Perform server control operation.  Often used by plugins.
     * @param fmt of server control message.
     * @return true if successful, false if error.
     */
    static bool control(const char *fmt, ...) __PRINTF(1, 2);

    /**
     * Receive control message.  Mostly internal use.
     * @return control message received.
     */
    static char *receive(void);

    /**
     * Reply to a control message.  Often used in derived driver dispatch
     * handler.
     * @param text of reply error message or NULL if no error.
     */
    static void reply(const char *text);

    /**
     * Enter server control message dispatch loop from main() thread.  Exits
     * when server is being shutdown.
     * @param pid used for reply.
     */
    static void dispatch(void);

    /**
     * Process stop, mostly internal use.
     */
    static void stop(void);

    /**
     * Process release, mostly internal use.
     */
    static void release(void);

    /**
     * Execute a system process such as a shell script.  Returns when
     * process is started.  The process is detached from the server.
     * @return false if fails to create process.
     * @param fmt of shell command request.
     */
    static bool system(const char *fmt, ...) __PRINTF(1, 2);

    /**
     * Create a snapshot file.
     * @param pid used in reply.
     */
    static void snapshot(int pid);

    /**
     * Extract periodic stats.
     * @param slice time of collection.
     */
    static bool period(long slice);

    /**
     * Load plugins into memory.
     * @param plugins to load, uses default if not specified.
     */
    static void load(const char *plugins = NULL);

    inline static const char *env(const char *id)
        {return args.getsym(id);}

    inline static String path(const char *id)
        {return (String)(args.getsym(id));}
};

class __EXPORT Env
{
public:
    inline static const char *env(const char *id)
        {return server::args.getsym(id);}

    inline static String path(const char *id)
        {return (String)(server::args.getsym(id));}

    static inline caddr_t memget(size_t size)
        {return (caddr_t)server::args.zalloc(size);}

    static inline char *memcopy(const char *str)
        {return server::args.dup(str);}
};

} // end namespace

#endif
