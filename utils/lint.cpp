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

#include <bayonne-config.h>
#include <bayonne.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

class lint : public Script::interp
{
public:
};

static Script::keyword_t keywords[] = {
    {NULL}};

static shell::flagopt helpflag('h',"--help", _TEXT("display this list"));
static shell::flagopt althelp('?', NULL, NULL);
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
    const char *fn;
    Script *lib = NULL;
    char buffer[256];
    const char *ext;
    size_t len;
    fsys_t dir;

    shell::bind("baylint");
    shell args(argc, argv);

    Env::tool(&args);

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

    if(!fsys::is_file(fn)) {
        fprintf(stderr, "*** baylint: %s: not found\n", fn);
        exit(-1);
    }

    if(!String::equal(ext, ".bcs") && !String::equal(ext, ".scr")) {
        fprintf(stderr, "*** %s: not a valid script file name\n", fn);
        exit(1);
    }

    Script::init();
    Script::assign(keywords);

    String::set(buffer, sizeof(buffer), Env::env("definitions"));
    fsys::open(dir, buffer, fsys::DIRECTORY);

    if(!is(dir))
        shell::log(shell::ERR, "cannot compile definitions from %s", buffer);
    else {
        shell::debug(2, "compiling definitions from %s", buffer);
        len = strlen(buffer);
        buffer[len++] = '/';

        while(is(dir) && fsys::read(dir, buffer + len, sizeof(buffer) - len) > 0) {
            char *ep = strrchr(buffer + len, '.');
            if(!ep)
                continue;
            if(!String::equal(ep, ".bcs"))
                continue;
            shell::log(shell::INFO, "compiling %s", buffer + len);
            lib = Script::compile(lib, buffer, NULL);
        }
        fsys::close(dir);
    }

    String::set(buffer, sizeof(buffer), Env::env("modules"));
    fsys::open(dir, buffer, fsys::DIRECTORY);

    if(!is(dir))
        shell::log(shell::ERR, "cannot use module definitions from %s", buffer);
    else {
        shell::debug(2, "using module definitions from %s", buffer);
        len = strlen(buffer);
        buffer[len++] = '/';

        while(is(dir) && fsys::read(dir, buffer + len, sizeof(buffer) - len) > 0) {
            char *ep = strrchr(buffer + len, '.');
            if(!ep)
                continue;
            if(!String::equal(ep, ".bcs"))
                continue;
            shell::log(shell::INFO, "module %s", buffer + len);
            lib = Script::compile(lib, buffer, NULL);
        }
        fsys::close(dir);
    }

    if(lib)
        ep = lib->getListing();

    while(is(ep)) {
        fprintf(stderr, "%s:%d: warning: %s\n", ep->filename, ep->errline, ep->errmsg);
        ep.next();
    }

    Script *img = Script::compile(NULL, fn, lib);

    if(!img) {
        fprintf(stderr, "*** baylint: %s: failed to compile\n", fn);
        exit(-1);
    }

    unsigned errors = img->getErrors();
    if(errors) {
        fprintf(stderr, "*** %d total errors in %s\n", errors, fn);
        ep = img->getListing();
        while(is(ep)) {
            fprintf(stderr, "%s:%d: error: %s\n", ep->filename, ep->errline, ep->errmsg);
            ep.next();
        }
        exit(2);
    }
    PROGRAM_EXIT(0);
}

