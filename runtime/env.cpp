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
bool Env::tool_flag = false;

bool Env::init(shell_t *args)
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
    set("appaudio", _STR(str(prefix) + "/audio"));
    set("sounds", _STR(str(prefix) + "/sounds"));
    set("definitions", _STR(str(prefix) + "/scripts"));
    set("shell", "cmd.exe");
    set("voices", _STR(str(prefix) + "\\voices"));
    set("temp", "C:\\Program Files\\bayonne\\temp");

    prefix = "C:\\Program Files\\bayonne\\datafiles";

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
    set("scripts", DEFAULT_DATADIR "/bayonne");
    set("audio", DEFAULT_DATADIR "/bayonne");
    set("sounds", DEFAULT_DATADIR "/sounds");
    set("applications", DEFAULT_DATADIR "/bayonne");
    set("definitions", DEFAULT_DATADIR "/bayonne");
    set("shell", "/bin/sh");
    set("voices", DEFAULT_DATADIR "/phrasebook");
    set("temp", DEFAULT_VARPATH "/tmp/bayonne");
#endif

#ifdef  HAVE_PWD_H
    const char *home_prefix = NULL;
    struct passwd *pwd = getpwuid(getuid());
    umask(007);

    if(getuid() && pwd && pwd->pw_dir && *pwd->pw_dir == '/') {
        if(!eq(pwd->pw_shell, "/bin/false") && !eq(pwd->pw_dir, "/var/", 5) && !eq(pwd->pw_dir, "/srv/", 5)) {
            umask(077);
            daemon_flag = false;
        }
        else    // alternate media prefix possible if not root startup...
            prefix = strdup(pwd->pw_dir);
    }

    // if user mode (testing)...
    if(!daemon_flag && pwd)
        home_prefix = strdup(str(pwd->pw_dir) + "/.bayonne");

    // tool only uses ~/.bayonne if also actually exists...
    if(home_prefix && !fsys::isdir(home_prefix))
        home_prefix = NULL;

    if(home_prefix) {
        rundir = strdup(str("/tmp/bayonne-") + str(pwd->pw_name));
        prefix = home_prefix;

        set("config", _STR(str(pwd->pw_dir) + "/.bayonnerc"));
        set("configs", home_prefix);
        set("scripts", home_prefix);
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
#endif

    set("voice", "default");
    set("extension", ".au");
    set("prefix", prefix);
    set("rundir", rundir);
    set("plugins", plugins);

    return daemon_flag;
}

void Env::tool(shell_t *args)
{
    tool_flag = true;
    init(args);
}

const char *Env::config(const char *name)
{
    // if we have cached copy of config path, use it...
    const char *result = env(name);
    if(result)
        return result;

    const char *sysconfig = env("sysconfig");

    string_t filename = str(env("configs")) + str(name);

    if(sysconfig && !fsys::isfile(*filename))
        filename = str(sysconfig) + str(name);

    // cache and use requested config path...
    set(name, *filename);
    return env(name);
}

const char *Env::path(pathinfo_t& pi, const char *path, char *buffer, size_t size, bool writeflag)
{
    const char *ext = strrchr(path, '/');

    // file: and tool: can be used anywhere, but for toolmode only...
    if(case_eq(path, "file:", 5) || case_eq(path, "tool:")) {
        if(!tool_flag)
            return NULL;

        String::set(buffer, size, path + 5);
        return buffer;
    }

    // out: can be used to override for writing absolute path in tool mode...
    if(case_eq(path, "out:", 4)) {
        if(!tool_flag || !writeflag)
            return NULL;

        String::set(buffer, size, path + 4);
        return buffer;
    }

    // writeflag used to restrict what bayonne can modify.  We disable this
    // for various bayonne tools.
    if(tool_flag)
        writeflag = false;

    // all other xxx: paths cannot include subdirectories...
    if(ext && strchr(path + 2, ':'))
        return NULL;

    // xxx.ext is a local file, not phrase library, in tool mode...
    if(!ext && strchr(path, '.') && tool_flag && !strchr(path + 2, ':'))
        ext = path;

    if(ext) {
        ext = strchr(ext, '.');
        if(ext)
            ext = "";
        else
            ext = env("extension");

        if(*path != '/' && tool_flag)
            snprintf(buffer, size, "%s/%s%s",
                env("prefix"), path, ext);
        else
            snprintf(buffer, size, "%s%s", path, ext);

        return buffer;
    }

    const char *voice = pi.voices;
    if(!voice)
        voice = env("voice");

    ext = strchr(path, '.');

    if(ext)
        ext = "";
    else
        ext = env("extension");

    // tmp: to access bayonne temporary files...
    if(case_eq(path, "tmp:", 4) || case_eq(path, "temp:", 5)) {
        snprintf(buffer, size, "%s/%s%s",
            env("temp"), strchr(path, ':') + 1, ext);
        return buffer;
    }

    // we cannot write to system sounds directory...
    if(case_eq(path, "sound:", 6) || case_eq(path, "sounds:", 7)) {
        if(writeflag)
            return NULL;

        snprintf(buffer, size, "%s/%s%s",
            env("sounds"), strchr(path, ':') + 1, ext);
    }


    // lib: optional way to always specify use library
    else if(case_eq(path, "lib:", 4))
        path += 4;
    else if(strchr(path, ':'))
        return NULL;

    // normally cannot write to phrasebook library unless toolmode
    if(writeflag)
        return NULL;

    // can optionally force path through voice library setting
    if(strchr(voice, '/')) {
        snprintf(buffer, size, "%s/%s%s", voice, path, ext);
        return buffer;
    }

    if(pi.book) {
        snprintf(buffer, size, "%s%s%s/%s%s",
            env("voices"), pi->path(), voice, path, ext);

        return buffer;
    }
    return NULL;
}


