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
#include <ccscript.h>
#include <bayonne/bayonne.h>
#include <bayonne-config.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

class lint : public Script::interp
{
public:
};

static Script::keyword_t keywords[] = {
	{NULL}};

static char *driver = NULL;
static char *modules = NULL;
static const char *lintfiles = DEFAULT_DATADIR;
static const char *defs[128];
static const char *clist = " \t,;:";
unsigned defcount = 0;

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
	printf("usage: baylint [options] scriptfile\n"
		"Options:\n"
		"  --driver <name>         Driver lint definitions to use\n"
		"  --modules <list>        Names of optional modules we added\n"
		"  --lint <dir>            Alternate lint database directory\n"
		"  --definition <script>   Alternate definition script\n"
	);		
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

	while(NULL != *(++argv)) {
		if(String::equal(*argv, "--")) {
			++argv;
			break;
		}

		if(String::equal(*argv, "--", 2))
			++*argv;

		if(String::equal(*argv, "-version"))
			version();

		if(String::equal(*argv, "-l") || String::equal(*argv, "-lint")) {
			if(NULL == *(++argv)) {
				fprintf(stderr, "*** baylint: data directory missing\n");
				exit(-1);
			}
			lintfiles = strdup(*argv);
			continue;
		}

		if(String::equal(*argv, "-lint=", 6)) {
			lintfiles = strdup(*argv + 6);
			continue;
		}

		if(String::equal(*argv, "-d") || String::equal(*argv, "-driver")) {
			if(NULL == *(++argv)) {
				fprintf(stderr, "*** baylint: driver name missing\n");
				exit(-1);
			}
			driver = strdup(*argv);
			continue;
		}

		if(String::equal(*argv, "-driver=", 8)) {
			driver = strdup(*argv + 8);
			continue;
		}

		if(String::equal(*argv, "-m") || String::equal(*argv, "-modules")) {
			if(NULL == *(++argv)) {
				fprintf(stderr, "*** baylint: module name missing\n");
				exit(-1);
			}
			modules = strdup(*argv);
			continue;
		}

		if(String::equal(*argv, "-modules=", 9)) {
			driver = strdup(*argv + 9);
			continue;
		}

		if(String::equal(*argv, "-?") || String::equal(*argv, "-h") || String::equal(*argv, "-help"))
			usage();

		if(**argv == '-') {
			fprintf(stderr, "*** baylint: %s: unknown option\n", *argv);
			exit(-1);
		}

		break;
	}

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

	if((driver || modules) && !fsys::is_dir(lintfiles)) {
		fprintf(stderr, "*** baylint: %s: invalid data directory\n", lintfiles);
		exit(-1);
	}

	Script::init();
	Script::assign(keywords);

	if(driver)
		defs[defcount++] = driver;

	if(modules) 
		modname = String::token(modules, &tokens, clist); 

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
		if(!fsys::is_file(dn))
			continue;
		lib = Script::compile(lib, dn);
		ep = lib->getListing();
		while(is(ep)) {
			fprintf(stderr, "%s:%d: warning: %s\n", lib->getFilename(), ep->errline, ep->errmsg);
			ep.next();
		}
	}

	Script *img = Script::compile(NULL, fn, lib);

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

