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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

Audio::Info::Info()
{
    clear();
}

void Audio::Info::clear(void)
{
    memset(this, 0, sizeof(Info));
}

void Audio::Info::setRate(rate_t r)
{
    rate = getRate(encoding, r);
    set();
}

void Audio::Info::setFraming(timeout_t timeout)
{
    set();

    framing = getFraming(encoding);

    if(!timeout)
        return;

    if(framing) {
        timeout = (timeout / framing);
        if(!timeout)
            timeout = framing;
        else
            timeout = timeout * framing;
    }

    switch(timeout) {
    case 10:
    case 15:
    case 20:
    case 30:
    case 40:
        break;
    default:
        timeout = 20;
    }

    framing = timeout;
    framecount = (rate * framing) / 1000l;
    framesize = (unsigned)toBytes(encoding, framecount);
}

void Audio::Info::set(void)
{
    switch(encoding) {
    case mp1Audio:
        framecount = 384;
        framesize = (12 * bitrate / rate) * 4 + headersize + padding;
        return;
    case mp2Audio:
    case mp3Audio:
        framecount = 1152;
        framesize = (144 * bitrate / rate) + headersize + padding;
        return;
    default:
        break;
    }
    if(!framesize)
        framesize = getFrame(encoding);

    if(!framecount)
        framecount = getCount(encoding);

    if(!rate)
        rate = getRate(encoding);

    if(!bitrate && rate && framesize && framecount)
        bitrate = ((long)framesize * 8l * rate) / (long)framecount;
}

