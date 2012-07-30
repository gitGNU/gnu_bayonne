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

#include <bayonne-config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

DTMFDialer::DTMFDialer(const char *d, level_t l, timeout_t duration, timeout_t timer) :
Tonegen(duration)
{
    dtmfframes = (timer / duration);
    framing = duration;
    level = l;

    if(timer % duration)
        ++dtmfframes;

    digits = d;
    remaining = 0;
    reset();

    complete = true;
    if(digits && *digits)
        complete = false;
}

DTMFDialer::~DTMFDialer()
{
    Tonegen::cleanup();
}

Audio::linear_t DTMFDialer::getFrame(void)
{
    for(;;)
    {
        if(remaining) {
            --remaining;
            return Tonegen::getFrame();
        }

        if(!digits || !*digits) {
            complete = true;
            return NULL;
        }

        if(!is_silent()) {
            remaining = dtmfframes;
            reset();
            continue;
        }

        switch(*(digits++)) {
        case '!':
        case 'f':
        case 'F':
            return NULL;

        case '1':
            dual(697, 1209, level, level);
            remaining = dtmfframes;
            break;

        case '2':
            dual(697, 1336, level, level);
            remaining = dtmfframes;
            break;

        case '3':
            dual(697, 1477, level, level);
            remaining = dtmfframes;
            break;

        case 'A':
        case 'a':
            dual(697, 1633, level, level);
            remaining = dtmfframes;
            break;

        case '4':
            dual(770, 1209, level, level);
            remaining = dtmfframes;
            break;

        case '5':
            dual(770, 1336, level, level);
            remaining = dtmfframes;
            break;

        case '6':
            dual(770, 1477, level, level);
            remaining = dtmfframes;
            break;

        case '7':
            dual(852, 1209, level, level);
            remaining = dtmfframes;
            break;

        case '8':
            dual(852, 1336, level, level);
            remaining = dtmfframes;
            break;

        case '9':
            dual(852, 1477, level, level);
            remaining = dtmfframes;
            break;

        case 'C':
        case 'c':
            dual(852, 1633, level, level);
            remaining = dtmfframes;
            break;

        case '*':
            dual(941, 1209, level, level);
            remaining = dtmfframes;
            break;

        case '0':
            dual(941, 1336, level, level);
            remaining = dtmfframes;
            break;

        case '#':
            dual(941, 1477, level, level);
            remaining = dtmfframes;
            break;

        case 'D':
        case 'd':
            dual(941, 1633, level, level);
            remaining = dtmfframes;
            break;

        case 's':
        case 'S':
        case ',':
            remaining = 1000 / framing;
            reset();
            break;
        case '.':
            remaining = dtmfframes * 2;
            reset();
            break;
        }
    }
}

MFDialer::MFDialer(const char *d, level_t l, timeout_t duration, timeout_t timer) :
Tonegen(duration)
{
    mfframes = (timer / duration);
    framing = duration;
    level = l;
    kflag = false;

    if(timer % duration)
        ++mfframes;

    digits = d;
    remaining = 0;
    reset();

    complete = true;
    if(digits && *digits)
        complete = false;
}

MFDialer::~MFDialer()
{
    Tonegen::cleanup();
}

Audio::linear_t MFDialer::getFrame(void)
{
    for(;;)
    {
        if(remaining) {
            --remaining;
            return Tonegen::getFrame();
        }

        if(!digits || !*digits) {
            complete = true;
            return NULL;
        }

        if(!is_silent()) {
            if(kflag)
                remaining = 100 / framing;
            else
                remaining = mfframes;
            kflag = false;
            reset();
            continue;
        }

        switch(*(digits++)) {
        case '!':
        case 'f':
        case 'F':
            return NULL;

        case '1':
            dual(700, 900, level, level);
            remaining = mfframes;
            break;

        case '2':
            dual(700, 1100, level, level);
            remaining = mfframes;
            break;

        case '3':
            dual(900, 1100, level, level);
            remaining = mfframes;
            break;

        case '4':
            dual(700, 1300, level, level);
            remaining = mfframes;
            break;

        case '5':
            dual(900, 1300, level, level);
            remaining = mfframes;
            break;

        case '6':
            dual(1100, 1300, level, level);
            remaining = mfframes;
            break;

        case '7':
            dual(700, 1500, level, level);
            remaining = mfframes;
            break;

        case '8':
            dual(900, 1500, level, level);
            remaining = mfframes;
            break;

        case '9':
            dual(1100, 1500, level, level);
            remaining = mfframes;
            break;

        case 'K':
        case 'k':
        case '#':
            dual(1100, 1700, level, level);
            remaining = 100 / framing;
            kflag = true;
            break;

        case '0':
            dual(1300, 1500, level, level);
            remaining = mfframes;
            break;

        case 'S':
        case 's':
        case '*':
            dual(1500, 1700, level, level);
            remaining = mfframes;
            break;

        case 'B':
        case 'b':
            single(2600, level);
            kflag = true;
            remaining = 1000 / framing;
            break;

        case ',':
            remaining = 1000 / framing;
            reset();
            break;
        case '.':
            remaining = mfframes * 2;
            reset();
            break;
        }
    }
}

