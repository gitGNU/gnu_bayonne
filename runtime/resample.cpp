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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

Audio::resample::resample(Audio::rate_t div, Audio::rate_t mul)
{
    bool common = true;
    while(common) {
        common = false;

        while(!(mul & 0x01) && !(div & 0x01)) {
            mul = (Audio::rate_t)(mul >> 1);
            div = (Audio::rate_t)(div >> 1);
            common = true;
        }

        while(!(mul % 3) && !(div % 3)) {
            mul = (Audio::rate_t)(mul / 3);
            div = (Audio::rate_t)(div / 3);
            common = true;
        }

        while(!(mul % 5) && !(div %5)) {
            mul = (rate_t)(mul / 5);
            div = (rate_t)(div / 5);
            common = true;
        }
    }


    mfact = (unsigned)mul;
    dfact = (unsigned)div;

    max = mfact;
    if(dfact > mfact)
        max = dfact;

    ++max;
    buffer = new Audio::sample_t[max];
    ppos = gpos = 0;
    memset(buffer, 0, max * 2);
    last = 0;
}

Audio::resample::~resample()
{
    delete[] buffer;
}

size_t Audio::resample::estimate(size_t count)
{
    count *= mfact;
    count += (mfact - 1);
    return (count / dfact) + 1;
}

size_t Audio::resample::process(linear_t from, linear_t dest, size_t count)
{
    size_t saved = 0;
    sample_t diff, current;
    unsigned pos;
    unsigned dpos;

    while(count--) {
        current = *(from++);
        diff = (current - last) / mfact;
        pos = mfact;
        while(pos--) {
            last += diff;
            buffer[ppos++] = current;
            if(ppos >= max)
                ppos = 0;

            if(gpos < ppos)
                dpos = ppos - gpos;
            else
                dpos = max - (gpos - ppos);
            if(dpos >= dfact) {
                *(dest++) = buffer[gpos];
                ++saved;
                gpos += dfact;
                if(gpos >= max)
                    gpos = gpos - max;
            }
        }
        last = current;
    }
    return saved;
}
