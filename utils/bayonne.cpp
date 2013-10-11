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

#include "bayonne/bayonne.h"
#ifndef	_MSWINDOWS_
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>
#endif
#include <bayonne-config.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static void version(void)
{
	printf("Bayonne " VERSION "\n"
        "Copyright (C) 2008,2009 David Sugar, Tycho Softworks\n"
		"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

static void capture(void)
{
	char buffer[512];
	FILE *fp;

	snprintf(buffer, sizeof(buffer), "/tmp/.bayonne.%d", getpid());
	fp = fopen(buffer, "r");
	remove(buffer);
	while(fp && fgets(buffer, sizeof(buffer), fp) != NULL)
		fputs(buffer, stdout);
	if(fp)
		fclose(fp);
}

#ifndef	_MSWINDOWS_
static void runfiles(char **argv)
{
	char buffer[256];
	struct passwd *pwd = getpwuid(getuid());
	FILE *fp;

	if(argv[1]) {
		fprintf(stderr, "*** bayonne: %s: no arguments\n", *argv);
		exit(-1);
	}

	fp = fopen(DEFAULT_VARPATH "/run/bayonne/%s", *argv);
	if(!fp) {
		snprintf(buffer, sizeof(buffer), "/tmp/bayonne-%s/%s", pwd->pw_name, *argv);
		fp = fopen(buffer, "r");
	}

	if(!fp) {
		printf("none\n");
		exit(0);
	}
	
	while(fgets(buffer, sizeof(buffer), fp) != NULL && !feof(fp))
		puts(buffer);

	fclose(fp);
	exit(0);
}
#endif

static void command(char **argv, unsigned timeout)
{
	char buffer[512];
	size_t len;
	fd_t fd;

#ifdef	_MSWINDOWS_
	snprintf(buffer, sizeof(buffer), "\\\\.\\mailslot\\bayonne_ctrl");
	fd = CreateFile(buffer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);	
#else
	sigset_t sigs;
	int signo;
	struct passwd *pwd = getpwuid(getuid());

	sigemptyset(&sigs);
	sigaddset(&sigs, SIGUSR1);
	sigaddset(&sigs, SIGUSR2);
	sigaddset(&sigs, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &sigs, NULL);

	fd = ::open(DEFAULT_VARPATH "/run/bayonne/control", O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		snprintf(buffer, sizeof(buffer), "/tmp/bayonne-%s/control", pwd->pw_name);
		fd = ::open(buffer, O_WRONLY | O_NONBLOCK);
	}
#endif
	if(fd == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "*** bayonne: driver offline\n");
		exit(2);
	}

#ifndef	_MSWINDOWS_
	if(timeout)
		snprintf(buffer, sizeof(buffer), "%d", getpid());
	else
#endif
		buffer[0] = 0;

	while(*argv) {
		len = strlen(buffer);
		snprintf(buffer + len, sizeof(buffer) - len - 1, " %s", *(argv++));
	}

#ifdef	_MSWINDOWS_
	if(!WriteFile(fd, buffer, (DWORD)strlen(buffer) + 1, NULL, NULL)) {
		fprintf(stderr, "*** bayonne: control failed\n");
		exit(4);
	}
#else
	len = strlen(buffer);
	buffer[len++] = '\n';
	buffer[len] = 0;
	if(::write(fd, buffer, len) < (ssize_t)len) {
		fprintf(stderr, "*** bayonne: control failed\n");
		exit(4);
	}
	
	if(!timeout)
		exit(0);

	alarm(timeout);
#ifdef	HAVE_SIGWAIT2
	sigwait(&sigs, &signo);
#else
	signo = sigwait(&sigs);
#endif
	if(signo == SIGUSR1) {
		capture();
		exit(0);
	}
	if(signo == SIGALRM) {
		fprintf(stderr, "*** bayonne: driver timed out\n");
		exit(1);
	}
	fprintf(stderr, "*** bayonne: request failed\n");
	exit(3); 
#endif
}

static void usage(void)
{
	printf("usage: bayonne command\n"
		"Commands:\n"
        "  abort                   Driver forced abort\n"
		"  boards                  Dump board configuration\n"
		"  check                   Driver deadlock check\n"
        "  concurrency <level>     Driver concurrency level\n"
        "  disable <resource>      Disable a timeslot or span\n"
		"  down                    Shut down server\n"
        "  enable <resource>       Enable a disabled timeslot or span\n"
		"  hangup <resource>       Hangup an active timeslot or span\n"
		"  history                 Dump recent errlog history records\n"
		"  period <interval>       Collect periodic statistics\n"
		"  pstats                  Dump periodic statistics\n"
        "  release <registry>      Release registration entry\n"
		"  reload                  Reload configuration\n"
        "  restart                 Driver daemon restart\n"
		"  resume <board>          Resume suspended board\n"
        "  snapshot                Driver snapshot\n"
        "  spans                   Dump span configuration\n"
        "  stats                   Dump server statistics\n"
        "  suspend <board>         Suspend an active board\n"
        "  verbose <level>         Driver loggin verbose level\n"
	);		
	exit(0);
}

static void single(char **argv, int timeout)
{
	if(argv[1]) {
		fprintf(stderr, "*** bayonne: %s: too many arguments\n", *argv);
		exit(-1);
	}
	command(argv, timeout);
}

static void resource(char **argv, int timeout)
{
	if(!argv[1]) {
		fprintf(stderr, "*** bayonne: %s: resource missing\n", *argv);
		exit(-1);
	}
	if(argv[2]) {
		fprintf(stderr, "*** bayonne: %s: only one resource\n", *argv);
		exit(-1);
	}
	command(argv, timeout);
}

/*
static void registry(char **argv, int timeout)
{
	if(!argv[1]) {
		fprintf(stderr, "*** bayonne: %s: registry uri missing\n", *argv);
		exit(-1);
	}
	if(argv[2]) {
		fprintf(stderr, "*** bayonne: %s: only one registry uri\n", *argv);
		exit(-1);
	}
	command(argv, timeout);
}
*/

static void level(char **argv, int timeout)
{
	if(!argv[1]) {
		fprintf(stderr, "*** bayonne: %s: level missing\n", *argv);
		exit(-1);
	}
	if(argv[2]) {
		fprintf(stderr, "*** bayonne: %s: too many arguments\n", *argv);
		exit(-1);
	}
	command(argv, timeout);
}

static void pstats(char **argv)
{
	char text[80];

	if(argv[1]) {
		fprintf(stderr, "*** bayonne: pstats: no arguments used\n");
		exit(-1);
	}
	mapped_view<statmap> sta(STAT_MAP);
	unsigned count = sta.count();
	unsigned index = 0;
	const volatile statmap *map;
	
	if(!count) {
		fprintf(stderr, "*** bayonne: driver offline\n");
		exit(-1);
	}
	while(index < count) {
		map = (const volatile statmap *)sta(index++);
	
		if(map->type == statmap::UNUSED)
			break;
		
		if(map->type == statmap::BOARD) 
			snprintf(text, sizeof(text), "board/%-6s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::SPAN)
			snprintf(text, sizeof(text), "span/%-7s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::REGISTRY && map->timeslots)
			snprintf(text, sizeof(text), "net/%-8s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::REGISTRY)
			snprintf(text, sizeof(text), "net/%-8s -    ", map->id);
		else
			snprintf(text, sizeof(text), "%-12s %05hu", "system", map->timeslots);

		for(unsigned entry = 0; entry < 2; ++entry) {
			size_t len = strlen(text);
			snprintf(text + len, sizeof(text) - len, " %07lu %05hu %05hu",
				map->stats[entry].pperiod,
				map->stats[entry].min, 
				map->stats[entry].max);
		}
		printf("%s\n", text);
	}
	exit(0);
}

static void stats(char **argv)
{
	char text[80];
	time_t now;

	if(argv[1]) {
		fprintf(stderr, "*** bayonne: stats: no arguments used\n");
		exit(-1);
	}
	mapped_view<statmap> sta(STAT_MAP);
	unsigned count = sta.count();
	unsigned index = 0;
	const statmap *map;
	unsigned current;
	statmap buffer;
	
	if(!count) {
		fprintf(stderr, "*** bayonne: driver offline\n");
		exit(-1);
	}
	time(&now);
	while(index < count) {
		map = const_cast<const statmap *>(sta(index++));
	
		if(map->type == statmap::UNUSED)
			break;

		do {
			memcpy(&buffer, map, sizeof(buffer));
		} while(memcmp(&buffer, map, sizeof(buffer)));
		map = &buffer;
		
		if(map->type == statmap::BOARD) 
			snprintf(text, sizeof(text), "board/%-6s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::SPAN)
			snprintf(text, sizeof(text), "span/%-7s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::REGISTRY && map->timeslots)
			snprintf(text, sizeof(text), "net/%-8s %05hu", map->id, map->timeslots);
		else if(map->type == statmap::REGISTRY)
			snprintf(text, sizeof(text), "net/%-8s -    ", map->id);
		else
			snprintf(text, sizeof(text), "%-12s %05hu", "system", map->timeslots);

		for(unsigned entry = 0; entry < 2; ++entry) {
			size_t len = strlen(text);
			snprintf(text + len, sizeof(text) - len, " %09lu %05hu %05hu",
				map->stats[entry].total,
				map->stats[entry].current,
				map->stats[entry].peak);
		}
		current = map->stats[0].current + map->stats[1].current;
		if(current)
			printf("%s 0s\n", text);
		else if(!map->lastcall)
			printf("%s -\n", text);
		else if(now - map->lastcall > (3600l * 99l))
			printf("%s %ld%c\n", text, (now - map->lastcall) / (3600l * 24l), 'd');
		else if(now - map->lastcall > (60l * 120l))
			printf("%s %ld%c\n", text, (now - map->lastcall) / 3600l, 'h');
		else if(now - map->lastcall > 120l)
			printf("%s %ld%c\n", text, (now - map->lastcall) / 60l, 'm');
		else
			printf("%s %ld%c\n", text, now - map->lastcall, 's');
	}
	exit(0);
}

static void timeslots(char **argv)
{
	unsigned active = 0;
	char text[60];

	if(argv[1]) {
		fprintf(stderr, "*** bayonne: timeslots: no arguments\n");
		exit(-1);
	}
	mapped_view<Timeslot::mapped_t>	tsm(TIMESLOT_MAP);
	unsigned count = tsm.count();
	unsigned index = 0;
	const volatile Timeslot::mapped_t *map;
	
	if(!count) {
		fprintf(stderr, "*** bayonne: driver offline\n");
		exit(-1);
	}
	while(index < count) {
		map = (const volatile Timeslot::mapped_t *)tsm(index++);
		switch(map->state[0]) {
		default:
			String::set(text, sizeof(text), (const char *)map->source);
		}
		if(map->started) {
			++active;	
			printf("%4d %-12s %s\n",
				index - 1, map->state + 1, map->source);
		}
	}
	printf("%d of %d timeslots active\n", active, count);
	exit(0);
}

static void status(char **argv)
{
	if(argv[1]) {
		fprintf(stderr, "*** bayonne: status: no arguments\n");
		exit(-1);
	}
	mapped_view<Timeslot::mapped_t>	tsm(TIMESLOT_MAP);
	unsigned count = tsm.count();
	unsigned index = 0;
	const volatile Timeslot::mapped_t *map;
	
	if(!count) {
		fprintf(stderr, "*** bayonne: driver offline\n");
		exit(-1);
	}
	while(index < count) {
		map = (const volatile Timeslot::mapped_t *)tsm(index++);
		putc(map->state[0], stdout);
	}
	printf("\n");
	exit(0);
}

static void period(char **argv)
{
	if(!argv[1]) {
		fprintf(stderr, "*** bayonne: period: interval missing\n");
		exit(-1);
	}
	if(argv[2]) {
		fprintf(stderr, "*** bayonne: period: too many arguments\n");
		exit(-1);
	}
	command(argv, 10);
}

PROGRAM_MAIN(argc, argv)
{
	if(argc < 2)
		usage();

	++argv;
	if(String::equal(*argv, "version") || String::equal(*argv, "-version") || String::equal(*argv, "--version"))
		version();
	else if(String::equal(*argv, "help") || String::equal(*argv, "-help") || String::equal(*argv, "--help"))
		usage();
#ifndef _MSWINDOWS_
	else if(String::equal(*argv, "boards") || String::equal(*argv, "spans"))
		runfiles(argv);
#endif
	else if(String::equal(*argv, "reload") || String::equal(*argv, "check") || String::equal(*argv, "snapshot") || String::equal(*argv, "suspend") || String::equal(*argv, "resume"))
		single(argv, 30);
	else if(String::equal(*argv, "history")) {
		if(argc == 2)
			single(argv, 30);
		else
			level(argv, 10);
	}
	else if(String::equal(*argv, "down") || String::equal(*argv, "restart") || String::equal(*argv, "abort"))
		single(argv, 0);
	else if(String::equal(*argv, "verbose") || String::equal(*argv, "concurrency"))
		level(argv, 10);
	else if(String::equal(*argv, "enable") || String::equal(*argv, "disable") || String::equal(*argv, "drop") || String::equal(*argv, "hangup"))
		resource(argv, 10);
	else if(String::equal(*argv, "status"))
		status(argv);
	else if(String::equal(*argv, "timeslots"))
		timeslots(argv);
	else if(String::equal(*argv, "stats"))
		stats(argv);
	else if(String::equal(*argv, "period"))
		period(argv);
	else if(String::equal(*argv, "pstats"))
		pstats(argv);
	fprintf(stderr, "*** bayonne: %s: unknown command or option\n", argv[0]);
	PROGRAM_EXIT(1);
}

