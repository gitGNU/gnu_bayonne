// Copyright (C) 2008-2011 David Sugar, Tycho Softworks.
//
// This file is part of GNU Bayonne.
//
// GNU Bayonne is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// GNU Bayonne is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Bayonne.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>

#ifdef  HAVE_PWD_H
#include <pwd.h>
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

shell_t *Env::sys;
bool Env::daemon_flag = true;

void Env::init(shell_t *args)
{
    sys = args;

#ifdef _MSWINDOWS_
    const char *rundir = strdup(str(env("APPDATA")) + "/bayonne");
    const char *prefix = "C:\\Program Files\\bayonne";
    const char *plugins = "C:\\Program Files\\bayonne\\plugins";

    set("config", _STR(str(prefix) + "/bayonne.ini"));
    set("configs", _STR(str(prefix) + "\\config\\"));
    set("services", _STR(str(prefix) + "/services"));
    set("controls", rundir);
    set("control", "\\\\.\\mailslot\\bayonne_ctrl");
    set("snapshot", _STR(str(rundir) + "/snapshot.log"));
    set("boards", _STR(str(rundir) + "/boards.log"));
    set("spans", _STR(str(rundir) + "/spans.log"));
    set("logfiles", _STR(str(prefix) + "/logs"));
    set("logfile", _STR(str(prefix) + "/logs/bayonne.log"));
    set("calls", _STR(str(prefix) + "/logs/bayonne.calls"));
    set("stats", _STR(str(prefix) + "/logs/bayonne.stats"));
    set("scripts", _STR(str(prefix) + "/scripts"));
    set("shell", "cmd.exe");

    prefix = "C:\\Program Files\\bayonne\\media";

#else
    const char *prefix = DEFAULT_VARPATH "/lib/bayonne";
    const char *rundir = DEFAULT_VARPATH "/run/bayonne";
    const char *plugins = DEFAULT_LIBPATH "/bayonne";

    set("reply", "/tmp/.bayonne.");
    set("config", DEFAULT_CFGPATH "/bayonne.conf");
    set("configs", DEFAULT_CFGPATH "/bayonne/");
    set("services", DEFAULT_SCRPATH);
    set("controls", DEFAULT_VARPATH "/run/bayonne");
    set("control", DEFAULT_VARPATH "/run/bayonne/control");
    set("snapshot", DEFAULT_VARPATH "/run/bayonne/snapshot");
    set("boards", DEFAULT_VARPATH "/run/bayonne/boards");
    set("spans", DEFAULT_VARPATH "/run/bayonne/spans");
    set("logfiles", DEFAULT_VARPATH "/log");
    set("logfile", DEFAULT_VARPATH "/log/bayonne.log");
    set("calls", DEFAULT_VARPATH "/log/bayonne.calls");
    set("stats", DEFAULT_VARPATH "/log/bayonne.stats");
    set("scripts", DEFAULT_CFGPATH "/bayonne.d");
    set("shell", "/bin/sh");
#endif

#ifdef  HAVE_PWD_H
    struct passwd *pwd = getpwuid(getuid());
    umask(007);

    if(getuid() && pwd && pwd->pw_dir && *pwd->pw_dir == '/') {
        prefix = strdup(pwd->pw_dir);
        if(!eq(pwd->pw_shell, "/bin/false") && !eq(pwd->pw_dir, "/var/", 5) && !eq(pwd->pw_dir, "/srv/", 5)) {
            umask(077);
            daemon_flag = false;
        };
    }

    if(!daemon_flag && pwd) {
        rundir = strdup(str("/tmp/bayonne-") + str(pwd->pw_name));
        prefix = strdup(str(pwd->pw_dir) + "/.bayonne");

        set("config", _STR(str(pwd->pw_dir) + "/.bayonnerc"));
        set("configs", _STR(str(pwd->pw_dir) + "/.bayonne"));
        set("sysconfigs", _STR(str(DEFAULT_CFGPATH) + "/bayonne"));
        set("services", prefix);
        set("controls", rundir);
        set("control", _STR(str(rundir) + "/control"));
        set("snapshot", _STR(str(rundir) + "/snapshot"));
        set("boards", _STR(str(rundir) + "/boards"));
        set("spans", _STR(str(rundir) + "/spans"));
        set("logfiles", rundir);
        set("logfile", _STR(str(rundir) + "/logfile"));
        set("calls", _STR(str(rundir) + "/calls"));
        set("stats", _STR(str(rundir) + "/stats"));
        set("prefix", prefix);
        set("shell", pwd->pw_shell);
    }

#else
    if(argv[1])
        daemon_flag = false;
#endif

    set("prefix", prefix);
    set("rundir", rundir);
    set("plugins", plugins);
}

const char *Env::config(const char *name)
{
    // if we have cached copy of config path, use it...
    const char *result = env(name);
    if(result)
        return result;

    const char *altconfig = env("altconfig");
    const char *sysconfig = env("sysconfig");

    string_t filename = str(env("configs")) + str(name);

    if(sysconfig && !fsys::isfile(*filename))
        filename = str(sysconfig) + str(name);

    if(altconfig && !fsys::isfile(*filename))
        filename = str(altconfig) + str(name);

    // cache and use requested config path...
    set(name, *filename);
    return env(name);
}


