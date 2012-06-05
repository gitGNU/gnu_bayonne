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

#include "../config.h"
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>
#include <ctype.h>

#ifdef  HAVE_MATH_H
#include <math.h>
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define MAP_HASH_SIZE   197
#define MAP_PAGE_COUNT  255
#define MAP_PAGE_SIZE (sizeof(void *[MAP_PAGE_COUNT]))
#define MAP_PAGE_FIX (MAP_PAGE_SIZE / MAP_PAGE_COUNT)

static unsigned char *page = NULL;
static unsigned used = MAP_PAGE_SIZE;
static Tonegen::key_t *hash[MAP_HASH_SIZE];

static unsigned key(const char *id)
{
    unsigned val = 0;
    while(*id)
        val = (val << 1) ^ (*(id++) & 0x1f);

    return val % MAP_HASH_SIZE;
}

static void *map(unsigned len)
{
    unsigned char *pos;
    unsigned fix = len % MAP_PAGE_FIX;

    if(fix)
        len += MAP_PAGE_FIX - fix;

    if(used + len > MAP_PAGE_SIZE) {
        page = (unsigned char *)(new void *[MAP_PAGE_COUNT]);
        used = 0;
    }

    pos = page + used;
    used += len;
    return pos;
}

Tonegen::Tonegen(timeout_t duration, rate_t r)
{
    rate = r;
    df1 = df2 = 0;
    samples = (duration *(long)rate) / 1000;
    frame = new sample_t[samples];
    silencer = true;
    complete = false;
    def = NULL;

    reset();
}

Tonegen::Tonegen(key_t *k, level_t l, timeout_t duration)
{
    rate = rate8khz;
    df1 = df2 = 0;
    samples = (duration *(long)rate) / 1000;
    frame = new sample_t[samples];
    silencer = true;
    complete = false;

    reset();

    tone = k;

    if(!tone) {
        complete = true;
        return;
    }

    framing = duration;
    def = tone->first;
    complete = false;
    remaining = silent = count = 0;
    level = l;
}

Tonegen::Tonegen(unsigned freq, level_t l, timeout_t duration, rate_t r)
{
    rate = r;
    df1 = (freq * M_PI * 2) / (long)rate;
    df2 = (freq * M_PI * 2) / (long)rate;
    p1 = 0, p2 = 0;
    samples = (duration * (long)rate) / 1000;
    m1 = l / 2;
    m2 = l / 2;
    silencer = false;
    complete = false;
    def = NULL;

    frame = new sample_t[samples];
}

Tonegen::Tonegen(unsigned f1, unsigned f2, level_t l1, level_t l2, timeout_t duration, rate_t r)
{
    rate = r;
    df1 = (f1 * M_PI * 2) / (long)r;
    df2 = (f2 * M_PI * 2) / (long)r;
    p1 = 0, p2 = 0;
    samples = (duration * (long)r) / 1000;
    m1 = l1 / 2;
    m2 = l2 / 2;
    silencer = false;
    complete = false;
    def = NULL;

    frame = new sample_t[samples];
}

unsigned Tonegen::getFrames(linear_t buffer, unsigned pages)
{
    unsigned count = 0;
    linear_t save = frame;

    while(count < pages) {
        frame = buffer;
        buffer += samples;
        if(!getFrame())
            break;
        ++count;
    }

    if(count && count < pages)
        memset(buffer, 0, samples * (pages - count) * 2);

    frame = save;
    return count;
}

void Tonegen::silence(void)
{
    silencer = true;
}

void Tonegen::reset(void)
{
    m1 = m2 = 0;
    p1 = p2 = 0;
}

void Tonegen::single(unsigned freq, level_t l)
{
    df1 = (freq * M_PI * 2) / (long)rate;
    df2 = (freq * M_PI * 2) / (long)rate;
    m1 = l / 2;
    m2 = l / 2;
    silencer = false;
    complete = false;
    def = NULL;
}

void Tonegen::dual(unsigned f1, unsigned f2, level_t l1, level_t l2)
{
    df1 = (f1 * M_PI * 2) / (long)rate;
    df2 = (f2 * M_PI * 2) / (long)rate;
    m1 = l1 / 2;
    m2 = l2 / 2;
    silencer = false;
    complete = false;
    def = NULL;
}

Audio::linear_t Tonegen::getFrame(void)
{
    unsigned count = samples;
    linear_t data = frame;

    if(complete)
        return NULL;

    if(def) {
        if(count >= def->count && !remaining && !silent) {
            def = def->next;
            count = 0;
            if(!def) {
                complete = true;
                return NULL;
            }
        }

        if(!remaining && !silent) {
            if(count && !def->duration)
                goto get;

            if(def->f2)
                dual(def->f1, def->f2, level, level);
            else
                single(def->f1, level);
            ++count;
            remaining = def->duration / framing;
            if(def->silence)
                silent = (def->duration + def->silence) / framing - remaining;
            else
                silent = 0;
        }

        if(!remaining && m1 && silent)
            reset();

        if(remaining)
            --remaining;
        else if(silent)
            --silent;
    }

get:

    if(is_silent() && !p1 && !p2) {
        if(!p1 && !p2) {
            memset(frame, 0, samples * 2);
            return frame;
        }
    }
    else if(silencer)
    {
        while(count--) {
            if(p1 <= 0 && df1 >= p1) {
                p1 = 0;
                df1 = 0;
                m1 = 0;
            }
            if(p1 >= 0 && -df1 >= p1) {
                p1 = 0;
                df1 = 0;
                m1 = 0;
            }
            if(p2 <= 0 && df2 >= p1) {
                p2 = 0;
                df2 = 0;
                m2 = 0;
            }
            if(p2 >= 0 && -df2 >= p1) {
                p2 = 0;
                df2 = 0;
                m2 = 0;
            }

            if(!m1 && !m2) {
                *(data++) = 0;
                continue;
            }

                    *(data++) = (level_t)(sin(p1) * (double)m1) +
                            (level_t)(sin(p2) * (double)m2);

            p1 += df1;
            p2 += df2;
        }
    }
    else {
        while(count--) {
                    *(data++) = (level_t)(sin(p1) * (double)m1) +
                (level_t)(sin(p2) * (double)m2);

                    p1 += df1;
                    p2 += df2;
        }
    }

    return frame;
}

Tonegen::~Tonegen()
{
    cleanup();
}

void Tonegen::cleanup(void)
{
    if(frame) {
        delete[] frame;
        frame = NULL;
    }
}

bool Tonegen::is_silent(void)
{
    if(!m1 && !m2)
        return true;

    return false;
}

bool Tonegen::load(const char *l)
{
    char buffer[256];
    char locale[256];
    char *loclist[128], *cp, *ep, *name;
    char *lists[64];
    char **field, *freq, *fdur, *fcount;
    def_t *def, *first, *again, *last, *final = NULL;
    key_t *tk;
    unsigned count, i, k;
    unsigned lcount = 0;
    FILE *fp;
    char namebuf[65];
    bool loaded = false;

    fp = fopen(Env::path("configs") + "/tones.conf", "r");
    if(!fp)
        return false;

    memset(&hash, 0, sizeof(hash));

    for(;;)
    {
        if(!fgets(buffer, sizeof(buffer) - 1, fp) || feof(fp))
            break;
        cp = buffer;
        while(isspace(*cp))
            ++cp;

        if(*cp == '[') {
            if(loaded)
                break;
            strcpy(locale, buffer);
            cp = locale;
            lcount = 0;
            cp = strtok(cp, "[]|\r\n");
            while(cp) {
                if(*cp && !l)
                    loclist[lcount++] = cp;
                else if(*cp && !stricmp(cp, l))
                {
                    loclist[lcount++] = cp;
                    loaded = true;
                }
                cp = strtok(NULL, "[]|\r\n");
            }
            continue;
        }

        if(!isalpha(*cp) || !lcount)
            continue;

        ep = strchr(cp, '=');
        if(!ep)
            continue;
        *(ep++) = 0;
        name = strtok(cp, " \t");
        cp = strchr(ep, ';');
        if(cp)
            *cp = 0;
        cp = strchr(ep, '#');
        if(cp)
            *cp = 0;
        cp = strtok(ep, ",");
        count = 0;
        while(cp) {
            while(isspace(*cp))
                ++cp;

            lists[count++] = cp;
            cp = strtok(NULL, ",");
        }
        if(!count)
            continue;

        field = &lists[0];
        first = last = again = NULL;
        while(count--) {
            freq = strtok(*field, " \t\r\n");
            fdur = strtok(NULL, " \t\r\n");
            fcount = strtok(NULL, " \t\r\n");

            if(!freq)
                goto skip;

            freq = strtok(freq, " \r\r\n");

            if(isalpha(*freq)) {
                tk = find(freq, loclist[0]);
                if(tk) {
                    if(!first)
                        first = tk->first;
                    else {
                        final = tk->last;
                        again = tk->first;
                    }
                }
                break;
            }

            def = (def_t *)map(sizeof(def_t));
            memset(def, 0, sizeof(def_t));
            if(!first)
                first = def;
            else
                last->next = def;

            last = final = def;

            def->next = NULL;

            if(!fdur || !atol(fdur)) {
                again = def;
                count = 0;
            }
            else if((!fcount || !atoi(fcount)) && !count)
                again = first;

            cp = strtok(freq, " \t\r\n");
            ep = cp;
            while(isdigit(*ep))
                ++ep;
            def->f1 = atoi(cp);
            if(*ep)
                def->f2 = atoi(++ep);
            else
                def->f2 = 0;

            if(!fcount)
                fcount = (char *)"1";

            def->count = atoi(fcount);
            if(!def->count)
                ++def->count;

            if(!fdur)
                goto skip;

            cp = strtok(fdur, " \t\r\n");
            ep = cp;
            while(isdigit(*ep))
                ++ep;
            def->duration = atol(cp);
            if(*ep)
                def->silence = atol(++ep);
            else
                def->silence = 0;

skip:
            ++field;
        }
        if(last)
            last->next = again;
        field = &loclist[0];
        i = lcount;
        while(i--) {
            snprintf(namebuf, sizeof(namebuf), "%s.%s",
                *(field++), name);
            tk = (key_t *)map((unsigned)sizeof(key_t) + strlen(namebuf));
            strcpy(tk->id, namebuf);
            tk->first = first;
            tk->last = final;
            k = key(namebuf);
            tk->next = hash[k];
            hash[k] = tk;
        }
    }

    fclose(fp);

    if(page)
        return true;
    return false;
}

Tonegen::key_t *Tonegen::find(const char *id, const char *locale)
{
    unsigned k;
    key_t *tk;
    char namebuf[65];

    if(!locale)
        locale="us";

    snprintf(namebuf, sizeof(namebuf), "%s.%s", locale, id);
    k = key(namebuf);
    tk = hash[k];

    while(tk) {
        if(!stricmp(namebuf, tk->id))
            break;
        tk = tk->next;
    }
    return tk;
}



