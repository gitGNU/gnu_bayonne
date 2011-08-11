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

#include <ucommon/ucommon.h>
#include <bayonne.h>
#include <config.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

class lint : public Script::interp
{
public:
};

static Script::keyword_t keywords[] = {
    {NULL}};

static const char *defs[128];
static const char *clist = " \t,;:";
unsigned defcount = 0;

static shell::flagopt helpflag('h',"--help", _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
static shell::stringopt driver('d', "--driver", _TEXT("driver lint definitions to use"), "name", NULL);
static shell::stringopt modules('m', "--modules", _TEXT("names of optional modules we added"), "list", NULL);
static shell::flagopt ver('v', "--version", _TEXT("version of application"));

static void version(void)
{
    printf("Bayonne Lint " VERSION "\n"
        "Copyright (C) 2009 David Sugar, Tycho Softworks\n"
        "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

static void usage(void)
{
    printf("%s\n", _TEXT("usage: baylint [options] scriptfile\n"
        "Options:"));
    shell::help();
    printf("\n%s\n", _TEXT("Report bugs to dyfet@gnu.org"));
    exit(0);
}

PROGRAM_MAIN(argc, argv)
{
    linked_pointer<Script::error> ep;
    const char *modname = NULL;
    const char *fn, *dn;
    Script *lib = NULL;
    unsigned def = 0;
    char buffer[256];
    char *tokens = NULL;
    const char *ext;

    shell::bind("baylint");
    shell args(argc, argv);

    Env::init(&args);

    const char *lintfiles = Env::env("definitions");

    if(is(helpflag) || is(althelp))
        usage();

    if(is(ver))
        version();

    argv = args.argv();
    fn = *(argv++);

    if(!fn) {
        fprintf(stderr, "*** baylint: no filename specified\n");
        exit(-1);
    }

    ext = strrchr(fn, '.');

    if(!fsys::isfile(fn)) {
        fprintf(stderr, "*** baylint: %s: not found\n", fn);
        exit(-1);
    }

    if((is(driver) || is(modules)) && !fsys::isdir(lintfiles)) {
        fprintf(stderr, "*** baylint: %s: invalid data directory\n", lintfiles);
        exit(-1);
    }

    Script::init();
    Script::assign(keywords);

    if(is(driver))
        defs[defcount++] = *driver;

    if(modules)
        modname = String::token((char *)*modules, &tokens, clist);

    while(modname) {
        defs[defcount++] = modname;
        modname = String::token(NULL, &tokens, clist);
    }

    while(def < defcount) {
        dn = defs[def++];
        if(!strchr(dn, '/') && !strchr(dn, '.')) {
            snprintf(buffer, sizeof(buffer), "%s/%s.lint", lintfiles, dn);
            dn = buffer;
        }
        if(!fsys::isfile(dn))
            continue;
        lib = Script::merge(dn, lib);
        ep = lib->getListing();
        while(is(ep)) {
            fprintf(stderr, "%s:%d: warning: %s\n", lib->getFilename(), ep->errline, ep->errmsg);
            ep.next();
        }
    }

    Script *img = Script::compile(fn, lib);

    if(!img) {
        fprintf(stderr, "*** baylint: %s: failed to compile\n", fn);
        exit(-1);
    }

    unsigned errors = img->getErrors();

    if(!String::equal(ext, ".ics") && !String::equal(ext, ".ocs") && !String::equal(ext, ".dcs")) {
        fprintf(stderr, "*** %s: not a valid script file name\n", fn);
        exit(1);
    }

    if(errors) {
        fprintf(stderr, "*** %d total errors in %s\n", errors, fn);
        ep = img->getListing();
        while(is(ep)) {
            fprintf(stderr, "%s:%d: error: %s\n", img->getFilename(), ep->errline, ep->errmsg);
            ep.next();
        }
        exit(2);
    }
    PROGRAM_EXIT(0);
}

