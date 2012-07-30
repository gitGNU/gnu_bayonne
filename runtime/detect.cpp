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

#ifdef  HAVE_MATH_H
#include <math.h>
#endif

#ifdef  HAVE_STDINT_H
#include <stdint.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

//
//   DTMF Receiver module, part of:
//      BSD Telephony Of Mexico "Zapata" Telecom Library, version 1.10  12/9/01
//
//   Part of the "Zapata" Computer Telephony Technology.
//
//   See http://www.bsdtelephony.com.mx
//
//  The technologies, software, hardware, designs, drawings, scheumatics, board
//  layouts and/or artwork, concepts, methodologies (including the use of all
//  of these, and that which is derived from the use of all of these), all other
//  intellectual properties contained herein, and all intellectual property
//  rights have been and shall continue to be expressly for the benefit of all
//  mankind, and are perpetually placed in the public domain, and may be used,
//  copied, and/or modified by anyone, in any manner, for any legal purpose,
//  without restriction.
//
// tone_detect.c - General telephony tone detection, and specific
//                      detection of DTMF.
//
//        Copyright (C) 2001  Steve Underwood <steveu@coppice.org>
//
//        Despite my general liking of the GPL, I place this code in the
//        public domain for the benefit of all mankind - even the slimy
//        ones who might try to proprietize my work and use it to my
//        detriment.
//

/* Basic DTMF specs:
 *
 * Minimum tone on = 40ms
 * Minimum tone off = 50ms
 * Maximum digit rate = 10 per second
 * Normal twist <= 8dB accepted
 * Reverse twist <= 4dB accepted
 * S/N >= 15dB will detect OK
 * Attenuation <= 26dB will detect OK
 * Frequency tolerance +- 1.5% will detect, +-3.5% will reject
 */

#define SAMPLE_RATE     8000.0

#define DTMF_THRESHOLD      8.0e7
#define FAX_THRESHOLD       8.0e7
#define FAX_2ND_HARMONIC    2.0     /* 4dB */
#define DTMF_NORMAL_TWIST   8.0     /* 8dB */
#define DTMF_REVERSE_TWIST  4.0     /* 4dB normal */
#define DTMF_RELATIVE_PEAK_ROW  6.3     /* 8dB */
#define DTMF_RELATIVE_PEAK_COL  6.3     /* 8dB */
#define DTMF_2ND_HARMONIC_ROW   2.5     /* 4dB normal */
#define DTMF_2ND_HARMONIC_COL   63.1    /* 18dB */

DTMFDetect::DTMFDetect()
{
    int i;
    float theta;
    static float dtmf_row[] = { 697.0, 770.0, 852.0, 941.0 };
    static float dtmf_col[] = { 1209.0, 1336.0, 1477.0, 1633.0 };
    static float fax_freq = 1100.0;

    state = (dtmf_detect_state_t *)malloc(sizeof(dtmf_detect_state_t));
    memset(state, 0, sizeof(state));

    for(i = 0; i < 4; i++)
    {
        theta = (float)(2.0 * M_PI * (dtmf_row[i] / SAMPLE_RATE));
        dtmf_detect_row[i].fac = (float)(2.0 * cos(theta));

        theta = (float)(2.0 * M_PI * (dtmf_col[i] / SAMPLE_RATE));
        dtmf_detect_col[i].fac = (float)(2.0 * cos(theta));

        theta = (float)(2.0 * M_PI * (dtmf_row[i] * 2.0 / SAMPLE_RATE));
        dtmf_detect_row_2nd[i].fac = (float)(2.0 * cos(theta));

        theta = (float)(2.0 * M_PI * (dtmf_col[i] * 2.0 / SAMPLE_RATE));
        dtmf_detect_col_2nd[i].fac = (float)(2.0 * cos(theta));

        goertzelInit(&state->row_out[i], &dtmf_detect_row[i]);
        goertzelInit(&state->col_out[i], &dtmf_detect_col[i]);
        goertzelInit(&state->row_out2nd[i], &dtmf_detect_row_2nd[i]);
        goertzelInit(&state->col_out2nd[i], &dtmf_detect_col_2nd[i]);

        state->energy = 0.0;
    }

    // Same for the fax detector
    theta = (float)(2.0 * M_PI * (fax_freq / SAMPLE_RATE));
    fax_detect.fac = (float)(2.0 * cos(theta));
    goertzelInit(&state->fax_tone, &fax_detect);

    // Same for the fax detector 2nd harmonic
    theta = (float)(2.0 * M_PI * (fax_freq / SAMPLE_RATE));
    fax_detect_2nd.fac = (float)(2.0 * cos(theta));
    goertzelInit(&state->fax_tone2nd, &fax_detect_2nd);

    state->current_digits = 0;
    state->current_sample = 0;
    state->detected_digits = 0;
    state->lost_digits = 0;
    state->digits[0] = '\0';
    state->mhit = 0;
}

DTMFDetect::~DTMFDetect()
{
    if(state) {
        free(state);
        state = NULL;
    }
    return;
}

void DTMFDetect::goertzelInit(goertzel_state_t *s, tone_detection_descriptor_t *t)
{
    s->v2 = s->v3 = 0.0;
    s->fac = t->fac;
}

void DTMFDetect::goertzelUpdate(goertzel_state_t *s,
         sample_t x[],
         int samples)
{
    int i;
    float v1;

    for (i = 0;  i < samples;  i++)
    {
        v1 = s->v2;
        s->v2 = s->v3;
        s->v3 = s->fac*s->v2 - v1 + x[i];
    }
}

float DTMFDetect::goertzelResult (goertzel_state_t *s)
{
    return s->v3 * s->v3 + s->v2 * s->v2 - s->v2 *s->v3 *s->fac;
}

int DTMFDetect::putSamples(linear_t amp, int samples)
{
    static char dtmf_positions[] = "123A" "456B" "789C" "*0#D";
    float row_energy[4];
    float col_energy[4];
    float fax_energy;
    float fax_energy_2nd;
    float famp;
    float v1;
    int i;
    int j;
    int sample;
    int best_row;
    int best_col;
    int hit;
    int limit;

    hit = 0;
    for (sample = 0;  sample < samples;  sample = limit)
    {
        // 102 is optimised to meet the DTMF specs.
        if ((samples - sample) >= (102 - state->current_sample))
            limit = sample + (102 - state->current_sample);
        else
            limit = samples;

        // The following unrolled loop takes only 35% (rough estimate) of the
        // time of a rolled loop on the machine on which it was developed
        for(j = sample;  j < limit;  j++)
        {
            famp = amp[j];
            state->energy += famp*famp;

            // With GCC 2.95, the following unrolled code seems to take about 35%
            // (rough estimate) as long as a neat little 0-3 loop
            v1 = state->row_out[0].v2;
            state->row_out[0].v2 = state->row_out[0].v3;
            state->row_out[0].v3 = state->row_out[0].fac*state->row_out[0].v2 - v1 + famp;

            v1 = state->col_out[0].v2;
            state->col_out[0].v2 = state->col_out[0].v3;
            state->col_out[0].v3 = state->col_out[0].fac*state->col_out[0].v2 - v1 + famp;

            v1 = state->row_out[1].v2;
            state->row_out[1].v2 = state->row_out[1].v3;
            state->row_out[1].v3 = state->row_out[1].fac*state->row_out[1].v2 - v1 + famp;

            v1 = state->col_out[1].v2;
            state->col_out[1].v2 = state->col_out[1].v3;
            state->col_out[1].v3 = state->col_out[1].fac*state->col_out[1].v2 - v1 + famp;

            v1 = state->row_out[2].v2;
            state->row_out[2].v2 = state->row_out[2].v3;
            state->row_out[2].v3 = state->row_out[2].fac*state->row_out[2].v2 - v1 + famp;

            v1 = state->col_out[2].v2;
            state->col_out[2].v2 = state->col_out[2].v3;
            state->col_out[2].v3 = state->col_out[2].fac*state->col_out[2].v2 - v1 + famp;

            v1 = state->row_out[3].v2;
            state->row_out[3].v2 = state->row_out[3].v3;
            state->row_out[3].v3 = state->row_out[3].fac*state->row_out[3].v2 - v1 + famp;

            v1 = state->col_out[3].v2;
            state->col_out[3].v2 = state->col_out[3].v3;
            state->col_out[3].v3 = state->col_out[3].fac*state->col_out[3].v2 - v1 + famp;

            v1 = state->col_out2nd[0].v2;
            state->col_out2nd[0].v2 = state->col_out2nd[0].v3;
            state->col_out2nd[0].v3 = state->col_out2nd[0].fac*state->col_out2nd[0].v2 - v1 + famp;

            v1 = state->row_out2nd[0].v2;
            state->row_out2nd[0].v2 = state->row_out2nd[0].v3;
            state->row_out2nd[0].v3 = state->row_out2nd[0].fac*state->row_out2nd[0].v2 - v1 + famp;

            v1 = state->col_out2nd[1].v2;
            state->col_out2nd[1].v2 = state->col_out2nd[1].v3;
            state->col_out2nd[1].v3 = state->col_out2nd[1].fac*state->col_out2nd[1].v2 - v1 + famp;

            v1 = state->row_out2nd[1].v2;
            state->row_out2nd[1].v2 = state->row_out2nd[1].v3;
            state->row_out2nd[1].v3 = state->row_out2nd[1].fac*state->row_out2nd[1].v2 - v1 + famp;

            v1 = state->col_out2nd[2].v2;
            state->col_out2nd[2].v2 = state->col_out2nd[2].v3;
            state->col_out2nd[2].v3 = state->col_out2nd[2].fac*state->col_out2nd[2].v2 - v1 + famp;

            v1 = state->row_out2nd[2].v2;
            state->row_out2nd[2].v2 = state->row_out2nd[2].v3;
            state->row_out2nd[2].v3 = state->row_out2nd[2].fac*state->row_out2nd[2].v2 - v1 + famp;

            v1 = state->col_out2nd[3].v2;
            state->col_out2nd[3].v2 = state->col_out2nd[3].v3;
            state->col_out2nd[3].v3 = state->col_out2nd[3].fac*state->col_out2nd[3].v2 - v1 + famp;

            v1 = state->row_out2nd[3].v2;
            state->row_out2nd[3].v2 = state->row_out2nd[3].v3;
            state->row_out2nd[3].v3 = state->row_out2nd[3].fac*state->row_out2nd[3].v2 - v1 + famp;

            v1 = state->fax_tone.v2;
            state->fax_tone.v2 = state->fax_tone.v3;
            state->fax_tone.v3 = state->fax_tone.fac*state->fax_tone.v2 - v1 + famp;

            v1 = state->fax_tone.v2;
            state->fax_tone2nd.v2 = state->fax_tone2nd.v3;
            state->fax_tone2nd.v3 = state->fax_tone2nd.fac*state->fax_tone2nd.v2 - v1 + famp;
        }
        state->current_sample += (limit - sample);
        if(state->current_sample < 102)
            continue;

        fax_energy = goertzelResult(&state->fax_tone);

        // We are at the end of a DTMF detection block
        // Find the peak row and the peak column
        row_energy[0] = goertzelResult (&state->row_out[0]);
        col_energy[0] = goertzelResult (&state->col_out[0]);

        for(best_row = best_col = 0, i = 1;  i < 4;  i++)
        {
            row_energy[i] = goertzelResult (&state->row_out[i]);
            if(row_energy[i] > row_energy[best_row])
                 best_row = i;
            col_energy[i] = goertzelResult (&state->col_out[i]);
            if(col_energy[i] > col_energy[best_col])
                best_col = i;
        }
        hit = 0;

        // Basic signal level test and the twist test
        if(row_energy[best_row] >= DTMF_THRESHOLD &&
            col_energy[best_col] >= DTMF_THRESHOLD &&
            col_energy[best_col] < row_energy[best_row] * DTMF_REVERSE_TWIST &&
            col_energy[best_col] * DTMF_NORMAL_TWIST > row_energy[best_row])
        {
            // Relative peak test
            for(i = 0;  i < 4;  i++)
            {
                if ((i != best_col &&
                    col_energy[i]*DTMF_RELATIVE_PEAK_COL > col_energy[best_col]) ||
                    (i != best_row && row_energy[i]*DTMF_RELATIVE_PEAK_ROW > row_energy[best_row]))
                    break;
            }
            // ... and second harmonic test
            if(i >= 4 &&
                (row_energy[best_row] + col_energy[best_col]) > 42.0*state->energy &&
                goertzelResult (&state->col_out2nd[best_col])*DTMF_2ND_HARMONIC_COL < col_energy[best_col] &&
                goertzelResult (&state->row_out2nd[best_row])*DTMF_2ND_HARMONIC_ROW < row_energy[best_row])
            {
                hit = dtmf_positions[(best_row << 2) + best_col];
                // Look for two successive similar results
                // The logic in the next test is:
                //   We need two successive identical clean detects, with
                //   something different preceeding it. This can work with
                //   back to back differing digits. More importantly, it
                //   can work with nasty phones that give a very wobbly start
                //   to a digit.
                if (hit == state->hit3  &&  state->hit3 != state->hit2) {
                    state->mhit = hit;
                    state->digit_hits[(best_row << 2) + best_col]++;
                    state->detected_digits++;
                    if (state->current_digits < 128) {
                        state->digits[state->current_digits++] = hit;
                        state->digits[state->current_digits] = '\0';
                    }
                    else {
                        state->lost_digits++;
                    }
                }
            }
        }

        if (!hit && (fax_energy >= FAX_THRESHOLD) && (fax_energy > state->energy * 21.0)) {
            fax_energy_2nd = goertzelResult(&state->fax_tone2nd);
            if (fax_energy_2nd * FAX_2ND_HARMONIC < fax_energy) {
                // XXX Probably need better checking than just this the energy
                hit = 'f';
                state->fax_hits++;
            } /* Don't reset fax hits counter */
        } else {
            if (state->fax_hits > 5) {
                state->mhit = 'f';
                state->detected_digits++;
                if (state->current_digits < 128) {
                    state->digits[state->current_digits++] = hit;
                    state->digits[state->current_digits] = '\0';
                }
                else
                    state->lost_digits++;
            }
            state->fax_hits = 0;
        }
        state->hit1 = state->hit2;
        state->hit2 = state->hit3;
        state->hit3 = hit;
        // Reinitialise the detector for the next block
        for (i = 0;  i < 4;  i++)
        {
            goertzelInit (&state->row_out[i], &dtmf_detect_row[i]);
            goertzelInit (&state->col_out[i], &dtmf_detect_col[i]);
            goertzelInit (&state->row_out2nd[i], &dtmf_detect_row_2nd[i]);
            goertzelInit (&state->col_out2nd[i], &dtmf_detect_col_2nd[i]);
        }
        goertzelInit (&state->fax_tone, &fax_detect);
        goertzelInit (&state->fax_tone2nd, &fax_detect_2nd);
        state->energy = 0.0;
        state->current_sample = 0;
    }
    if ((!state->mhit) || (state->mhit != hit)) {
        state->mhit = 0;
        return(0);
    }
    return (hit);
}

int DTMFDetect::getResult(char *buf, int max)
{
    if (max > state->current_digits)
        max = state->current_digits;
    if (max > 0) {
        memcpy (buf, state->digits, max);
        memmove (state->digits, state->digits + max, state->current_digits - max);
        state->current_digits -= max;
    }
    buf[max] = '\0';
    return  max;
}


