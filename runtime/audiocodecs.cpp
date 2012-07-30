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

#ifdef  HAVE_MATH_H
#include <math.h>
#endif

extern "C" {
    #if defined(HAVE_GSM_GSM_H)
    #include <gsm/gsm.h>
    #elif defined(HAVE_GSM_H)
    #include <gsm.h>
    #endif
}

#ifdef  HAVE_SPEEX_SPEEX_H
#include <speex/speex.h>
#endif

#include <ucommon/export.h>
#include <bayonne.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static LinkedObject *first = NULL;

AudioCodec::AudioCodec(const char *n, encoding_t e) :
LinkedObject(&first)
{
    encoding = e;
    name = n;
    first = this;

    info.clear();
    info.format = raw;
    info.encoding = e;
}

AudioCodec::AudioCodec()
{
    name = NULL;

    info.clear();
    info.format = raw;
}

void AudioCodec::release(AudioCodec *codec)
{
    if(!codec->name)
        delete codec;
}

AudioCodec *AudioCodec::begin(void)
{
    return (AudioCodec *)first;
}

AudioCodec *AudioCodec::get(encoding_t e, const char *format)
{
    linked_pointer<AudioCodec> codec = first;

    while(is(codec)) {
        if(e == codec->encoding)
            break;
        codec.next();
    }

    if(is(codec) && format)
        return codec->getByFormat(format);

    return *codec;
}

AudioCodec *AudioCodec::get(info_t& info)
{
    linked_pointer<AudioCodec> codec = first;

    while(is(codec)) {
        if(info.encoding == codec->encoding)
            break;
        codec.next();
    }

    if(is(codec))
        return codec->getByInfo(info);

    return *codec;
}

bool AudioCodec::is_silent(level_t hint, void *data, unsigned samples)
{
    level_t power = impulse(data, samples);

    if(power < 0)
        return true;

    if(power > hint)
        return false;

    return true;
}

Audio::level_t AudioCodec::impulse(void *data, unsigned samples)
{
    unsigned long sum = 0;
    linear_t ldata = new sample_t[samples];
    linear_t lptr = ldata;
    long count = decode(ldata, data, samples);

    samples = count;
    while(samples--) {
        if(*ldata < 0)
            sum -= *(ldata++);
        else
            sum += *(ldata++);
    }

    delete[] lptr;
    return (level_t)(sum / count);
}

Audio::level_t AudioCodec::peak(void *data, unsigned samples)
{
    level_t max = 0, value;
    linear_t ldata = new sample_t[samples];
    linear_t lptr = ldata;
    long count = decode(ldata, data, samples);

    samples = count;
    while(samples--) {
        value = *(ldata++);
        if(value < 0)
            value = -value;
        if(value > max)
            max = value;
    }

    delete[] lptr;
    return max;
}

unsigned AudioCodec::getEstimated(void)
{
    return info.framesize;
}

unsigned AudioCodec::getRequired(void)
{
    return info.framecount;
}

unsigned AudioCodec::encodeBuffered(linear_t buffer, encoded_t source, unsigned samples)
{
    return encode(buffer, source, samples);
}

unsigned AudioCodec::decodeBuffered(linear_t buffer, encoded_t source, unsigned bytes)
{
    return decode(buffer, source, toSamples(info, bytes));
}

unsigned AudioCodec::getPacket(encoded_t packet, encoded_t data, unsigned bytes)
{
    if(bytes != info.framesize)
        return 0;

    memcpy(packet, data, bytes);
    return bytes;
}

// g711 codecs initialized in static linkage...

static class __LOCAL g711u : public AudioCodec {
public:
    g711u();

    unsigned encode(linear_t buffer, void *source, unsigned lsamples);
    unsigned decode(linear_t buffer, void *dest, unsigned lsamples);
    level_t impulse(void *buffer, unsigned samples);
    level_t peak(void *buffer, unsigned samples);

} g711u;

static class __LOCAL g711a : public AudioCodec {
public:
    g711a();

    unsigned encode(linear_t buffer, void *source, unsigned lsamples);
    unsigned decode(linear_t buffer, void *dest, unsigned lsamples);
    level_t impulse(void *buffer, unsigned samples);
    level_t peak(void *buffer, unsigned samples);

} g711a;

g711u::g711u() : AudioCodec("g.711", mulawAudio)
{
    info.framesize = 1;
    info.framecount = 1;
    info.rate = 8000;
    info.bitrate = 64000;
    info.annotation = (char *)"mu-law";
}

g711a::g711a() : AudioCodec("g.711", alawAudio)
{
    info.framesize = 1;
    info.framecount = 1;
    info.bitrate = 64000;
    info.rate = 8000;
    info.annotation = (char *)"a-law";
}

static unsigned ullevels[128] =
{
            32124,   31100,   30076,   29052,   28028,
    27004,   25980,   24956,   23932,   22908,   21884,   20860,
    19836,   18812,   17788,   16764,   15996,   15484,   14972,
    14460,   13948,   13436,   12924,   12412,   11900,   11388,
    10876,   10364,    9852,    9340,    8828,    8316,    7932,
    7676,    7420,    7164,    6908,    6652,    6396,    6140,
    5884,    5628,    5372,    5116,    4860,    4604,    4348,
    4092,    3900,    3772,    3644,    3516,    3388,    3260,
    3132,    3004,    2876,    2748,    2620,    2492,    2364,
    2236,    2108,    1980,    1884,    1820,    1756,    1692,
    1628,    1564,    1500,    1436,    1372,    1308,    1244,
    1180,    1116,    1052,     988,     924,     876,     844,
    812,     780,     748,     716,     684,     652,     620,
    588,     556,     524,     492,     460,     428,     396,
    372,     356,     340,     324,     308,     292,     276,
    260,     244,     228,     212,     196,     180,     164,
    148,     132,     120,     112,     104,      96,      88,
    80,      72,      64,      56,      48,      40,      32,
    24,      16,       8,       0
};

Audio::level_t g711u::impulse(void *data, unsigned samples)
{
    unsigned long count = samples;
    unsigned long sum = 0;

    if(!samples)
        samples = count = 160;

    unsigned char *dp = (unsigned char *)data;

    while(samples--)
        sum += (ullevels[*(dp++) & 0x7f]);

    return (level_t)(sum / count);
}

Audio::level_t g711u::peak(void *data, unsigned samples)
{
    unsigned long count = samples;
    level_t max = 0, value;

    if(!samples)
        samples = count = 160;

    unsigned char *dp = (unsigned char *)data;

    while(samples--) {
        value = ullevels[*(dp++) & 0x7f];
        if(value > max)
            max = value;
    }
    return max;
}

unsigned g711u::encode(linear_t buffer, void *dest, unsigned lsamples)
{
    static int ulaw[256] = {
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

    register sample_t sample;
    int sign, exponent, mantissa, retval;
    register unsigned char *d = (unsigned char *)dest;
    unsigned count;

    count = lsamples;

    while(lsamples--) {
        sample = *(buffer++);
                sign = (sample >> 8) & 0x80;
            if(sign != 0) sample = -sample;
            sample += 0x84;
            exponent = ulaw[(sample >> 7) & 0xff];
            mantissa = (sample >> (exponent + 3)) & 0x0f;
            retval = ~(sign | (exponent << 4) | mantissa);
            if(!retval)
            retval = 0x02;
        *(d++) = (unsigned char)retval;
    }
    return count;
}

unsigned g711u::decode(linear_t buffer, void *source, unsigned lsamples)
{
    register unsigned char *src = (unsigned char *)source;
    unsigned count;

    count = lsamples;

    static sample_t values[256] =
    {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0
    };

    while(lsamples--)
        *(buffer++) = values[*(src++)];

    return count;
}

#define AMI_MASK    0x55

unsigned g711a::encode(linear_t buffer, void *dest, unsigned lsamples)
{
    int mask, seg, pcm_val;
    unsigned count;
    unsigned char *d = (unsigned char *)dest;

    static int seg_end[] = {
        0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

    count = lsamples;

    while(lsamples--) {
        pcm_val = *(buffer++);
        if(pcm_val >= 0)
            mask = AMI_MASK | 0x80;
        else {
            mask = AMI_MASK;
            pcm_val = -pcm_val;
        }
        for(seg = 0; seg < 8; seg++)
        {
            if(pcm_val <= seg_end[seg])
                break;
        }
        *(d++) = ((seg << 4) | ((pcm_val >> ((seg)  ?  (seg + 3)  :  4)) & 0x0F)) ^ mask;
    }
    return count;
}

static unsigned allevels[128] =
{
    5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
    7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
    2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
    3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
    22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
    30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
    11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
    15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
    344,    328,    376,    360,    280,    264,    312,    296,
    472,    456,    504,    488,    408,    392,    440,    424,
    88,     72,    120,    104,     24,      8,     56,     40,
    216,    200,    248,    232,    152,    136,    184,    168,
    1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
    1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
    688,    656,    752,    720,    560,    528,    624,    592,
    944,    912,   1008,    976,    816,    784,    880,    848
};

Audio::level_t g711a::impulse(void *data, unsigned samples)
{
    unsigned long count = samples;
    unsigned long sum = 0;

    if(!samples)
        samples = count = 160;



    unsigned char *dp = (unsigned char *)data;

    while(samples--)
        sum += (allevels[*(dp++) & 0x7f]);

    return (level_t)(sum / count);
}

Audio::level_t g711a::peak(void *data, unsigned samples)
{
    unsigned long count = samples;
    level_t max = 0, value;

    if(!samples)
        samples = count = 160;

    unsigned char *dp = (unsigned char *)data;

    while(samples--) {
        value = allevels[*(dp++) & 0x7f];
        if(value > max)
            max = value;
    }
    return max;
}

unsigned g711a::decode(linear_t buffer, void *source, unsigned lsamples)
{
    register unsigned char *src = (unsigned char *)source;
    unsigned count;

    static sample_t values[256] =
    {
        -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
        -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
        -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
        -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
       -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
       -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
       -11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
       -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
         -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
         -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
          -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
         -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
        -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
        -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
         -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
         -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
         5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
         7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
         2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
         3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
        22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
        30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
        11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
        15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
          344,    328,    376,    360,    280,    264,    312,    296,
          472,    456,    504,    488,    408,    392,    440,    424,
           88,     72,    120,    104,     24,      8,     56,     40,
          216,    200,    248,    232,    152,    136,    184,    168,
         1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
         1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
          688,    656,    752,    720,    560,    528,    624,    592,
          944,    912,   1008,    976,    816,    784,    880,    848
    };

    count = lsamples;

    while(lsamples--)
        *(buffer++) = values[*(src++)];

    return count;
}

// adpcm codec

static short power2[15] = {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80,
            0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000};

typedef struct state {
    long yl;
    short yu;
    short dms;
    short dml;
    short ap;
    short a[2];
    short b[6];
    short pk[2];
    short dq[6];
    short sr[2];
    char td;
}   state_t;

static int quan(
    int     val,
    short       *table,
    int     size)
{
    int     i;

    for (i = 0; i < size; i++)
        if (val < *table++)
            break;
    return (i);
}

static int quantize(
    int     d,  /* Raw difference signal sample */
    int     y,  /* Step size multiplier */
    short       *table, /* quantization table */
    int     size)   /* table size of short integers */
{
    short       dqm;    /* Magnitude of 'd' */
    short       exp;    /* Integer part of base 2 log of 'd' */
    short       mant;   /* Fractional part of base 2 log */
    short       dl; /* Log of magnitude of 'd' */
    short       dln;    /* Step size scale factor normalized log */
    int     i;

    /*
     * LOG
     *
     * Compute base 2 log of 'd', and store in 'dl'.
     */
    dqm = abs(d);
    exp = quan(dqm >> 1, power2, 15);
    mant = ((dqm << 7) >> exp) & 0x7F;  /* Fractional portion. */
    dl = (exp << 7) + mant;

    /*
     * SUBTB
     *
     * "Divide" by step size multiplier.
     */
    dln = dl - (y >> 2);

    /*
     * QUAN
     *
     * Obtain codword i for 'd'.
     */
    i = quan(dln, table, size);
    if (d < 0)          /* take 1's complement of i */
        return ((size << 1) + 1 - i);
    else if (i == 0)        /* take 1's complement of 0 */
        return ((size << 1) + 1); /* new in 1988 */
    else
        return (i);
}

static int fmult(
    int     an,
    int     srn)
{
    short       anmag, anexp, anmant;
    short       wanexp, wanmant;
    short       retval;

    anmag = (an > 0) ? an : ((-an) & 0x1FFF);
    anexp = quan(anmag, power2, 15) - 6;
    anmant = (anmag == 0) ? 32 :
        (anexp >= 0) ? anmag >> anexp : anmag << -anexp;
    wanexp = anexp + ((srn >> 6) & 0xF) - 13;

    wanmant = (anmant * (srn & 077) + 0x30) >> 4;
    retval = (wanexp >= 0) ? ((wanmant << wanexp) & 0x7FFF) :
        (wanmant >> -wanexp);

    return (((an ^ srn) < 0) ? -retval : retval);
}

static int reconstruct(
    int     sign,   /* 0 for non-negative value */
    int     dqln,   /* G.72x codeword */
    int     y)  /* Step size multiplier */
{
    short       dql;    /* Log of 'dq' magnitude */
    short       dex;    /* Integer part of log */
    short       dqt;
    short       dq; /* Reconstructed difference signal sample */

    dql = dqln + (y >> 2);  /* ADDA */

    if (dql < 0) {
        return ((sign) ? -0x8000 : 0);
    } else {        /* ANTILOG */
        dex = (dql >> 7) & 15;
        dqt = 128 + (dql & 127);
        dq = (dqt << 7) >> (14 - dex);
        return ((sign) ? (dq - 0x8000) : dq);
    }
}

static void update(
    int     code_size,  /* distinguish 723_40 with others */
    int     y,      /* quantizer step size */
    int     wi,     /* scale factor multiplier */
    int     fi,     /* for long/short term energies */
    int     dq,     /* quantized prediction difference */
    int     sr,     /* reconstructed signal */
    int     dqsez,      /* difference from 2-pole predictor */
    state_t *state_ptr)     /* coder state pointer */
{
    int     cnt;
    short       mag, exp;   /* Adaptive predictor, FLOAT A */
    short       a2p = 0;        /* LIMC */
    short       a1ul;       /* UPA1 */
    short       pks1;       /* UPA2 */
    short       fa1;
    char        tr;     /* tone/transition detector */
    short       ylint, thr2, dqthr;
    short       ylfrac, thr1;
    short       pk0;

    pk0 = (dqsez < 0) ? 1 : 0;  /* needed in updating predictor poles */

    mag = dq & 0x7FFF;      /* prediction difference magnitude */
    /* TRANS */
    ylint = (short)(state_ptr->yl >> 15);   /* exponent part of yl */
    ylfrac = (state_ptr->yl >> 10) & 0x1F;  /* fractional part of yl */
    thr1 = (32 + ylfrac) << ylint;      /* threshold */
    thr2 = (short)((ylint > 9) ? 31 << 10 : thr1);  /* limit thr2 to 31 << 10 */
    dqthr = (thr2 + (thr2 >> 1)) >> 1;  /* dqthr = 0.75 * thr2 */
    if (state_ptr->td == 0)     /* signal supposed voice */
        tr = 0;
    else if (mag <= dqthr)      /* supposed data, but small mag */
        tr = 0;         /* treated as voice */
    else                /* signal is data (modem) */
        tr = 1;

    /*
     * Quantizer scale factor adaptation.
     */

    /* FUNCTW & FILTD & DELAY */
    /* update non-steady state step size multiplier */
    state_ptr->yu = y + ((wi - y) >> 5);

    /* LIMB */
    if (state_ptr->yu < 544)    /* 544 <= yu <= 5120 */
        state_ptr->yu = 544;
    else if (state_ptr->yu > 5120)
        state_ptr->yu = 5120;

    /* FILTE & DELAY */
    /* update steady state step size multiplier */
    state_ptr->yl += state_ptr->yu + ((-state_ptr->yl) >> 6);

    /*
     * Adaptive predictor coefficients.
     */
    if (tr == 1) {          /* reset a's and b's for modem signal */
        state_ptr->a[0] = 0;
        state_ptr->a[1] = 0;
        state_ptr->b[0] = 0;
        state_ptr->b[1] = 0;
        state_ptr->b[2] = 0;
        state_ptr->b[3] = 0;
        state_ptr->b[4] = 0;
        state_ptr->b[5] = 0;
    } else {            /* update a's and b's */
        pks1 = pk0 ^ state_ptr->pk[0];      /* UPA2 */

        /* update predictor pole a[1] */
        a2p = state_ptr->a[1] - (state_ptr->a[1] >> 7);
        if (dqsez != 0) {
            fa1 = (pks1) ? state_ptr->a[0] : -state_ptr->a[0];
            if (fa1 < -8191)    /* a2p = function of fa1 */
                a2p -= 0x100;
            else if (fa1 > 8191)
                a2p += 0xFF;
            else
                a2p += fa1 >> 5;

            if (pk0 ^ state_ptr->pk[1])
                /* LIMC */
                if (a2p <= -12160)
                    a2p = -12288;
                else if (a2p >= 12416)
                    a2p = 12288;
                else
                    a2p -= 0x80;
            else if (a2p <= -12416)
                a2p = -12288;
            else if (a2p >= 12160)
                a2p = 12288;
            else
                a2p += 0x80;
        }

        /* TRIGB & DELAY */
        state_ptr->a[1] = a2p;

        /* UPA1 */
        /* update predictor pole a[0] */
        state_ptr->a[0] -= state_ptr->a[0] >> 8;
        if (dqsez != 0) {
            if (pks1 == 0)
                state_ptr->a[0] += 192;
            else
                state_ptr->a[0] -= 192;
        }

        /* LIMD */
        a1ul = 15360 - a2p;
        if (state_ptr->a[0] < -a1ul)
            state_ptr->a[0] = -a1ul;
        else if (state_ptr->a[0] > a1ul)
            state_ptr->a[0] = a1ul;

        /* UPB : update predictor zeros b[6] */
        for (cnt = 0; cnt < 6; cnt++) {
            if (code_size == 5)     /* for 40Kbps G.723 */
                state_ptr->b[cnt] -= state_ptr->b[cnt] >> 9;
            else            /* for G.721 and 24Kbps G.723 */
                state_ptr->b[cnt] -= state_ptr->b[cnt] >> 8;
            if (dq & 0x7FFF) {          /* XOR */
                if ((dq ^ state_ptr->dq[cnt]) >= 0)
                    state_ptr->b[cnt] += 128;
                else
                    state_ptr->b[cnt] -= 128;
            }
        }
    }

    for (cnt = 5; cnt > 0; cnt--)
        state_ptr->dq[cnt] = state_ptr->dq[cnt-1];
    /* FLOAT A : convert dq[0] to 4-bit exp, 6-bit mantissa f.p. */
    if (mag == 0) {
        state_ptr->dq[0] = (dq >= 0) ? 0x20 : 0xFC20;
    } else {
        exp = quan(mag, power2, 15);
        state_ptr->dq[0] = (dq >= 0) ?
            (exp << 6) + ((mag << 6) >> exp) :
            (exp << 6) + ((mag << 6) >> exp) - 0x400;
    }

    state_ptr->sr[1] = state_ptr->sr[0];
    /* FLOAT B : convert sr to 4-bit exp., 6-bit mantissa f.p. */
    if (sr == 0) {
        state_ptr->sr[0] = 0x20;
    } else if (sr > 0) {
        exp = quan(sr, power2, 15);
        state_ptr->sr[0] = (exp << 6) + ((sr << 6) >> exp);
    } else if (sr > -32768) {
        mag = -sr;
        exp = quan(mag, power2, 15);
        state_ptr->sr[0] =  (exp << 6) + ((mag << 6) >> exp) - 0x400;
    } else
        state_ptr->sr[0] = (short)0xFC20;

    /* DELAY A */
    state_ptr->pk[1] = state_ptr->pk[0];
    state_ptr->pk[0] = pk0;

    /* TONE */
    if (tr == 1)        /* this sample has been treated as data */
        state_ptr->td = 0;  /* next one will be treated as voice */
    else if (a2p < -11776)  /* small sample-to-sample correlation */
        state_ptr->td = 1;  /* signal may be data */
    else                /* signal is voice */
        state_ptr->td = 0;

    /*
     * Adaptation speed control.
     */
    state_ptr->dms += (fi - state_ptr->dms) >> 5;       /* FILTA */
    state_ptr->dml += (((fi << 2) - state_ptr->dml) >> 7);  /* FILTB */

    if (tr == 1)
        state_ptr->ap = 256;
    else if (y < 1536)                  /* SUBTC */
        state_ptr->ap += (0x200 - state_ptr->ap) >> 4;
    else if (state_ptr->td == 1)
        state_ptr->ap += (0x200 - state_ptr->ap) >> 4;
    else if (abs((state_ptr->dms << 2) - state_ptr->dml) >=
        (state_ptr->dml >> 3))
        state_ptr->ap += (0x200 - state_ptr->ap) >> 4;
    else
        state_ptr->ap += (-state_ptr->ap) >> 4;
}

static int predictor_zero(
    state_t *state_ptr)
{
    int     i;
    int     sezi;

    sezi = fmult(state_ptr->b[0] >> 2, state_ptr->dq[0]);
    for (i = 1; i < 6; i++)         /* ACCUM */
        sezi += fmult(state_ptr->b[i] >> 2, state_ptr->dq[i]);
    return (sezi);
}

static int predictor_pole(
    state_t *state_ptr)
{
    return (fmult(state_ptr->a[1] >> 2, state_ptr->sr[1]) +
        fmult(state_ptr->a[0] >> 2, state_ptr->sr[0]));
}

static int step_size(
    state_t *state_ptr)
{
    int     y;
    int     dif;
    int     al;

    if (state_ptr->ap >= 256)
        return (state_ptr->yu);
    else {
        y = state_ptr->yl >> 6;
        dif = state_ptr->yu - y;
        al = state_ptr->ap >> 2;
        if (dif > 0)
            y += (dif * al) >> 6;
        else if (dif < 0)
            y += (dif * al + 0x3F) >> 6;
        return (y);
    }
}

static class __LOCAL g721Codec : private AudioCodec
{
private:
    static short    _dqlntab[16];
    static short    _witab[16];
    static short    _fitab[16];
    static short qtab_721[7];

    state_t encode_state, decode_state;

    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned decode(linear_t buffer, void *from, unsigned lsamples);
    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    short coder(state_t *state, int nib);
    unsigned char encoder(short sl, state_t *state);

public:
    g721Codec(const char *id, encoding_t e);
    g721Codec();
    ~g721Codec();
} g723_4("adpcm", Audio::g721ADPCM);

static class __LOCAL g723_3Codec : private AudioCodec
{
private:
    static short    _dqlntab[8];
    static short    _witab[8];
    static short    _fitab[8];
    static short qtab_723_24[3];

    state_t encode_state, decode_state;

    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned decode(linear_t buffer, void *from, unsigned lsamples);
    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    short coder(state_t *state, int nib);
    unsigned char encoder(short sl, state_t *state);

public:
    g723_3Codec(const char *id, encoding_t e);
    g723_3Codec();
    ~g723_3Codec();
} g723_3("g.723", Audio::g723_3bit);

static class __LOCAL g723_5Codec : private AudioCodec
{
private:
    static short    _dqlntab[32];
    static short    _witab[32];
    static short    _fitab[32];
    static short qtab_723_40[15];

    state_t encode_state, decode_state;

    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned decode(linear_t buffer, void *from, unsigned lsamples);
    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    short coder(state_t *state, int nib);
    unsigned char encoder(short sl, state_t *state);

public:
    g723_5Codec(const char *id, encoding_t e);
    g723_5Codec();
    ~g723_5Codec();
} g723_5("g.723", Audio::g723_5bit);

class __LOCAL g723_2Codec : public AudioCodec
{
private:
    static short    _dqlntab[4];
    static short    _witab[4];
    static short    _fitab[4];
    static short qtab_723_16[1];

    state_t encode_state, decode_state;

public:
    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned decode(linear_t buffer, void *from, unsigned lsamples);
    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    short coder(state_t *state, int nib);
    unsigned char encoder(short sl, state_t *state);

    g723_2Codec(const char *id, encoding_t e);
    g723_2Codec();
    ~g723_2Codec();
} g723_2("g.723", Audio::g723_2bit);

short g723_2Codec::_dqlntab[4] = { 116, 365, 365, 116};
short g723_2Codec::_witab[4] = {-704, 14048, 14048, -704};
short g723_2Codec::_fitab[4] = {0, 0xE00, 0xE00, 0};
short g723_2Codec::qtab_723_16[1] = {261};

short g723_3Codec::_dqlntab[8] = {-2048, 135, 273, 373, 373, 273, 135, -2048};
short g723_3Codec::_witab[8] = {-128, 960, 4384, 18624, 18624, 4384, 960, -128};
short g723_3Codec::_fitab[8] = {0, 0x200, 0x400, 0xE00, 0xE00, 0x400, 0x200, 0};
short g723_3Codec::qtab_723_24[3] = {8, 218, 331};

short g723_5Codec::_dqlntab[32] = {-2048, -66, 28, 104, 169, 224, 274, 318,
                358, 395, 429, 459, 488, 514, 539, 566,
                566, 539, 514, 488, 459, 429, 395, 358,
                318, 274, 224, 169, 104, 28, -66, -2048};

short g723_5Codec::_witab[32] = {448, 448, 768, 1248, 1280, 1312, 1856, 3200,
            4512, 5728, 7008, 8960, 11456, 14080, 16928, 22272,
            22272, 16928, 14080, 11456, 8960, 7008, 5728, 4512,
            3200, 1856, 1312, 1280, 1248, 768, 448, 448};

short g723_5Codec::_fitab[32] = {0, 0, 0, 0, 0, 0x200, 0x200, 0x200,
            0x200, 0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xC00,
            0xC00, 0xC00, 0xA00, 0x800, 0x600, 0x400, 0x200, 0x200,
            0x200, 0x200, 0x200, 0, 0, 0, 0, 0};

short g723_5Codec::qtab_723_40[15] = {-122, -16, 68, 139, 198, 250, 298, 339,
                378, 413, 445, 475, 502, 528, 553};


short g721Codec::_dqlntab[16] = {-2048, 4, 135, 213, 273, 323, 373, 425,
                425, 373, 323, 273, 213, 135, 4, -2048};
short g721Codec::_witab[16] = {-12, 18, 41, 64, 112, 198, 355, 1122,
                1122, 355, 198, 112, 64, 41, 18, -12};
short g721Codec::_fitab[16] = {0, 0, 0, 0x200, 0x200, 0x200, 0x600, 0xE00,
                0xE00, 0x600, 0x200, 0x200, 0x200, 0, 0, 0};
short g721Codec::qtab_721[7] = {-124, 80, 178, 246, 300, 349, 400};

g723_3Codec::g723_3Codec() : AudioCodec()
{
    unsigned pos;

    info.framesize = 3;
    info.framecount = 8;
    info.bitrate = 24000;
    info.encoding = g723_3bit;
    info.annotation = (char *)"g.723/3";
    info.rate = 8000;
    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    encode_state.yl = decode_state.yl = 34816;
    encode_state.yu = decode_state.yu = 544;
    encode_state.sr[0] = encode_state.sr[1] = decode_state.sr[0] = decode_state.sr[1] = 32;

    for(pos = 0; pos < 6; ++pos)
        encode_state.dq[pos] = decode_state.dq[pos] = 32;
}

g723_3Codec::g723_3Codec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    info.framesize = 3;
    info.framecount = 8;
    info.bitrate = 24000;
    info.rate = 8000;
    info.annotation = (char *)"g.723/3";
}

g723_3Codec::~g723_3Codec()
{}


unsigned char g723_3Codec::encoder(short sl, state_t *state_ptr)
{
    short sezi, se, sez, sei;
    short d, sr, y, dqsez, dq, i;

    sl >>= 2;

    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    d = sl - se;                    /* d = estimation diff. */

    /* quantize prediction difference d */
    y = step_size(state_ptr);       /* quantizer step size */
    i = quantize(d, y, qtab_723_24, 3);     /* i = ADPCM code */
    dq = reconstruct(i & 4, _dqlntab[i], y); /* quantized diff. */

    sr = (dq < 0) ? se - (dq & 0x3FFF) : se + dq; /* reconstructed signal */
    dqsez = sr + sez - se;          /* pole prediction diff. */

    update(3, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);
    return (unsigned char)(i);
}

short g723_3Codec::coder(state_t *state_ptr, int i)
{
    short sezi, sei, sez, se;
    short y, sr, dq, dqsez;

    i &= 0x07;                      /* mask to get proper bits */
    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    y = step_size(state_ptr);       /* adaptive quantizer step size */
    dq = reconstruct(i & 0x04, _dqlntab[i], y); /* unquantize pred diff */

    sr = (dq < 0) ? (se - (dq & 0x3FFF)) : (se + dq); /* reconst. signal */

    dqsez = sr - se + sez;                  /* pole prediction diff. */

    update(3, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);

    return sr << 2;
}

unsigned g723_3Codec::encode(linear_t buffer, void *coded, unsigned lsamples)
{
    unsigned count = (lsamples / 8);
    encoded_t dest = (encoded_t)coded;
    unsigned i, data, byte, bits;

    while(count--) {
        bits = 0;
        data = 0;
        for(i = 0; i < 8; ++i)
        {
            byte = encoder(*(buffer++), &encode_state);
            data |= (byte << bits);
            bits += 3;
            if(bits >= 8) {
                *(dest++) = (data & 0xff);
                bits -= 8;
                data >>= 8;
            }
        }
    }
    return (lsamples / 8) * 8;
}

unsigned g723_3Codec::decode(linear_t buffer, void *from, unsigned lsamples)
{
    encoded_t src = (encoded_t)from;
    unsigned count = (lsamples / 8) * 8;
    unsigned char byte, nib;
    unsigned bits = 0, data = 0;

    while(count--) {
        if(bits < 3) {
            byte = *(src++);
            data |= (byte << bits);
            bits += 8;
        }
        nib = data & 0x07;
        data >>= 3;
        bits -= 3;
        *(buffer++) = coder(&decode_state, nib);
    }
    return (lsamples / 8) * 8;
}

AudioCodec *g723_3Codec::getByInfo(info_t& info)
{
    return (AudioCodec *)new g723_3Codec();
}

AudioCodec *g723_3Codec::getByFormat(const char *format)
{
    return (AudioCodec *)new g723_3Codec();
}



g723_2Codec::g723_2Codec() : AudioCodec()
{
    unsigned pos;

    info.framesize = 1;
    info.framecount = 4;
    info.bitrate = 16000;
    info.encoding = g723_3bit;
    info.annotation = (char *)"g.723/2";
    info.rate = 8000;
    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    encode_state.yl = decode_state.yl = 34816;
    encode_state.yu = decode_state.yu = 544;
    encode_state.sr[0] = encode_state.sr[1] = decode_state.sr[0] = decode_state.sr[1] = 32;

    for(pos = 0; pos < 6; ++pos)
        encode_state.dq[pos] = decode_state.dq[pos] = 32;
}

g723_2Codec::g723_2Codec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    info.framesize = 1;
    info.framecount = 4;
    info.bitrate = 16000;
    info.rate = 8000;
    info.annotation = (char *)"g.723/2";
}

g723_2Codec::~g723_2Codec()
{}

unsigned char g723_2Codec::encoder(short sl, state_t *state_ptr)
{
    short sezi, se, sez, sei;
    short d, sr, y, dqsez, dq, i;

    sl >>= 2;

    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    d = sl - se;

    /* quantize prediction difference d */
    y = step_size(state_ptr);       /* quantizer step size */
    i = quantize(d, y, qtab_723_16, 1);  /* i = ADPCM code */

      /* Since quantize() only produces a three level output
       * (1, 2, or 3), we must create the fourth one on our own
       */
    if (i == 3)                          /* i code for the zero region */
      if ((d & 0x8000) == 0)             /* If d > 0, i=3 isn't right... */
    i = 0;

    dq = reconstruct(i & 2, _dqlntab[i], y); /* quantized diff. */

    sr = (dq < 0) ? se - (dq & 0x3FFF) : se + dq; /* reconstructed signal */
    dqsez = sr + sez - se;          /* pole prediction diff. */

    update(2, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);


    return (unsigned char)(i);
}

short g723_2Codec::coder(state_t *state_ptr, int i)
{
    short sezi, sei, sez, se;
    short y, sr, dq, dqsez;

    i &= 0x03;                      /* mask to get proper bits */

    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    y = step_size(state_ptr);       /* adaptive quantizer step size */
    dq = reconstruct(i & 0x02, _dqlntab[i], y); /* unquantize pred diff */

    sr = (dq < 0) ? (se - (dq & 0x3FFF)) : (se + dq); /* reconst. signal */

    dqsez = sr - se + sez;                  /* pole prediction diff. */

    update(2, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);


    return sr << 2;
}

unsigned g723_2Codec::encode(linear_t buffer, void *coded, unsigned lsamples)
{
    unsigned count = (lsamples / 4);
    encoded_t dest = (encoded_t)coded;
    unsigned i, data, byte, bits;

    while(count--) {
        bits = 0;
        data = 0;
        for(i = 0; i < 4; ++i)
        {
            byte = encoder(*(buffer++), &encode_state);
            data |= (byte << bits);
            bits += 2;
            if(bits >= 8) {
                *(dest++) = (data & 0xff);
                bits -= 8;
                data >>= 8;
            }
        }
    }
    return (lsamples / 4) * 4;
}

unsigned g723_2Codec::decode(linear_t buffer, void *from, unsigned lsamples)
{
    encoded_t src = (encoded_t)from;
    unsigned count = (lsamples / 4) * 4;
    unsigned char byte, nib;
    unsigned bits = 0, data = 0;

    while(count--) {
        if(bits < 2) {
            byte = *(src++);
            data |= (byte << bits);
            bits += 8;
        }
        nib = data & 0x03;
        data >>= 2;
        bits -= 2;
        *(buffer++) = coder(&decode_state, nib);
    }
    return (lsamples / 4) * 4;
}

AudioCodec *g723_2Codec::getByInfo(info_t& info)
{
    return (AudioCodec *)new g723_2Codec();
}

AudioCodec *g723_2Codec::getByFormat(const char *format)
{
    return (AudioCodec *)new g723_2Codec();
}

g723_5Codec::g723_5Codec() : AudioCodec()
{
    unsigned pos;

    info.framesize = 5;
    info.framecount = 8;
    info.bitrate = 40000;
    info.encoding = g723_5bit;
    info.annotation = (char *)"g.723/5";
    info.rate = 8000;
    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    encode_state.yl = decode_state.yl = 34816;
    encode_state.yu = decode_state.yu = 544;
    encode_state.sr[0] = encode_state.sr[1] = decode_state.sr[0] = decode_state.sr[1] = 32;

    for(pos = 0; pos < 6; ++pos)
        encode_state.dq[pos] = decode_state.dq[pos] = 32;
}

g723_5Codec::g723_5Codec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    info.framesize = 5;
    info.framecount = 8;
    info.bitrate = 40000;
    info.rate = 8000;
    info.annotation = (char *)"g.723/5";
}

g723_5Codec::~g723_5Codec()
{}

unsigned char g723_5Codec::encoder(short sl, state_t *state_ptr)
{
    short           sei, sezi, se, sez;     /* ACCUM */
    short           d;                      /* SUBTA */
    short           y;                      /* MIX */
    short           sr;                     /* ADDB */
    short           dqsez;                  /* ADDC */
    short           dq, i;

    sl >>= 2;

    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    d = sl - se;                    /* d = estimation difference */

    /* quantize prediction difference */
    y = step_size(state_ptr);       /* adaptive quantizer step size */
    i = quantize(d, y, qtab_723_40, 15);    /* i = ADPCM code */

    dq = reconstruct(i & 0x10, _dqlntab[i], y);     /* quantized diff */

    sr = (dq < 0) ? se - (dq & 0x7FFF) : se + dq; /* reconstructed signal */
    dqsez = sr + sez - se;          /* dqsez = pole prediction diff. */

    update(5, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);

    return (unsigned char)(i);
}

short g723_5Codec::coder(state_t *state_ptr, int i)
{
    short           sezi, sei, sez, se;     /* ACCUM */
    short           y;                 /* MIX */
    short           sr;                     /* ADDB */
    short           dq;
    short           dqsez;

    i &= 0x1f;                      /* mask to get proper bits */
    sezi = predictor_zero(state_ptr);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state_ptr);
    se = sei >> 1;                  /* se = estimated signal */

    y = step_size(state_ptr);       /* adaptive quantizer step size */
    dq = reconstruct(i & 0x10, _dqlntab[i], y);     /* estimation diff. */

    sr = (dq < 0) ? (se - (dq & 0x7FFF)) : (se + dq); /* reconst. signal */

    dqsez = sr - se + sez;          /* pole prediction diff. */

    update(5, y, _witab[i], _fitab[i], dq, sr, dqsez, state_ptr);
    return sr << 2;
}

unsigned g723_5Codec::encode(linear_t buffer, void *coded, unsigned lsamples)
{
    unsigned count = (lsamples / 8);
    encoded_t dest = (encoded_t)coded;
    unsigned i, data, byte, bits;

    while(count--) {
        bits = 0;
        data = 0;
        for(i = 0; i < 8; ++i)
        {
            byte = encoder(*(buffer++), &encode_state);
            data |= (byte << bits);
            bits += 5;
            if(bits >= 8) {
                *(dest++) = (data & 0xff);
                bits -= 8;
                data >>= 8;
            }
        }
    }
    return (lsamples / 8) * 8;
}

unsigned g723_5Codec::decode(linear_t buffer, void *from, unsigned lsamples)
{
    encoded_t src = (encoded_t)from;
    unsigned count = (lsamples / 8) * 8;
    unsigned char byte, nib;
    unsigned bits = 0, data = 0;

    while(count--) {
        if(bits < 5) {
            byte = *(src++);
            data |= (byte << bits);
            bits += 8;
        }
        nib = data & 0x1f;
        data >>= 5;
        bits -= 5;
        *(buffer++) = coder(&decode_state, nib);
    }
    return (lsamples / 8) * 8;
}

AudioCodec *g723_5Codec::getByInfo(info_t& info)
{
    return (AudioCodec *)new g723_5Codec();
}

AudioCodec *g723_5Codec::getByFormat(const char *format)
{
    return (AudioCodec *)new g723_5Codec();
}

g721Codec::g721Codec() : AudioCodec()
{
    unsigned pos;

    info.framesize = 1;
    info.framecount = 2;
    info.rate = 8000;
    info.bitrate = 32000;
    info.annotation = (char *)"g.721";
    info.encoding = g721ADPCM;

    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    encode_state.yl = decode_state.yl = 34816;
    encode_state.yu = decode_state.yu = 544;
    encode_state.sr[0] = encode_state.sr[1] = decode_state.sr[0] = decode_state.sr[1] = 32;

    for(pos = 0; pos < 6; ++pos)
        encode_state.dq[pos] = decode_state.dq[pos] = 32;
}

g721Codec::g721Codec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    info.framesize = 1;
    info.framecount = 2;
    info.rate = 8000;
    info.bitrate = 32000;
    info.annotation = (char *)"g.721";
}

g721Codec::~g721Codec()
{}

unsigned char g721Codec::encoder(short sl, state_t *state)
{
    short sezi, se, sez;
    short d, sr, y, dqsez, dq, i;

    sl >>= 2;

    sezi = predictor_zero(state);
    sez = sezi >> 1;
    se = (sezi + predictor_pole(state)) >> 1;

    d = sl - se;

    y = step_size(state);
    i = quantize(d, y, qtab_721, 7);
    dq = reconstruct(i & 8, _dqlntab[i], y);
    sr = (dq < 0) ? se - (dq & 0x3FFF) : se + dq;

    dqsez = sr + sez - se;

    update(4, y, _witab[i] << 5, _fitab[i], dq, sr, dqsez, state);

    return (unsigned char)(i);
}

short g721Codec::coder(state_t *state, int i)
{
    short sezi, sei, sez, se;
    short y, sr, dq, dqsez;

    sezi = predictor_zero(state);
    sez = sezi >> 1;
    sei = sezi + predictor_pole(state);
    se = sei >> 1;
    y = step_size(state);
    dq = reconstruct(i & 0x08, _dqlntab[i], y);
    sr = (dq < 0) ? (se - (dq & 0x3fff)) : se + dq;
    dqsez = sr - se + sez;
    update(4, y, _witab[i] << 5, _fitab[i], dq, sr, dqsez, state);
    return sr << 2;
}

unsigned g721Codec::encode(linear_t buffer, void *coded, unsigned lsamples)
{
    unsigned count = (lsamples / 2);
    unsigned char byte = 0;
    encoded_t dest = (encoded_t)coded;
    unsigned data, bits, i;

    while(count--) {
        bits = 0;
        data = 0;
        for(i = 0; i < 2; ++i)
        {
            byte = encoder(*(buffer++), &encode_state);
            data |= (byte << bits);
            bits += 4;
            if(bits >= 8)
                *(dest++) = (data & 0xff);
        }
    }
    return (lsamples / 2) * 2;
}

unsigned g721Codec::decode(linear_t buffer, void *from, unsigned lsamples)
{
    encoded_t src = (encoded_t)from;
    unsigned count = lsamples / 2;
    unsigned data;

    while(count--) {
        data = *(src++);
        *(buffer++) = coder(&decode_state, (data & 0x0f));
        data >>= 4;
        *(buffer++) = coder(&decode_state, (data & 0x0f));
    }
    return (lsamples / 2) * 2;
}

AudioCodec *g721Codec::getByInfo(info_t& info)
{
    return (AudioCodec *)new g721Codec();
}

AudioCodec *g721Codec::getByFormat(const char *format)
{
    return (AudioCodec *)new g721Codec();
}

static int oki_index[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

static int oki_steps[49] = {
  16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73,
  80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279,
  307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
  1060, 1166, 1282, 1411, 1552
};

static class __LOCAL okiCodec : private AudioCodec
{
private:
    typedef struct state {
        short last;
        short ssindex;
    }   state_t;

    state_t encode_state, decode_state;

    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned decode(linear_t buffer, void *from, unsigned lsamples);
    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    unsigned char encode_sample(state_t *state, short sample);
    short decode_sample(state_t *state, unsigned char code);

public:
    okiCodec(const char *id, encoding_t e);
    okiCodec(encoding_t e);
    ~okiCodec();
} voxcodec("vox", Audio::voxADPCM), okicodec("oki", Audio::okiADPCM);

okiCodec::okiCodec(encoding_t e) : AudioCodec()
{
    info.framesize = 1;
    info.framecount = 2;
    info.encoding = e;

    if(encoding == voxADPCM) {
        info.rate = 6000;
        info.bitrate = 24000;
        info.annotation = (char *)"vox";
    }
    else {
        info.rate = 8000;
        info.bitrate = 24000;
        info.annotation = (char *)"oki";
    }

    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    info.set();
}

okiCodec::okiCodec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    info.framesize = 1;
    info.framecount = 2;

    if(encoding == voxADPCM) {
        info.rate = 6000;
        info.bitrate = 24000;
        info.annotation = (char *)"vox";
    }
    else {
        info.rate = 8000;
        info.bitrate = 24000;
        info.annotation = (char *)"oki";
    }
    memset(&encode_state, 0, sizeof(encode_state));
    memset(&decode_state, 0, sizeof(decode_state));
    info.set();
}

okiCodec::~okiCodec()
{}

unsigned char okiCodec::encode_sample(state_t *state, short sample)
{
    unsigned char code = 0;
    short diff, step;

    step = oki_steps[state->ssindex];
    diff = sample - state->last;
    if(diff < 0) {
        diff = -diff;
        code = 0x08;
    }

    if(diff >= step) {
        code |= 0x04;
        diff -= step;
    }
    if(diff >= step/2) {
        code |= 0x02;
        diff -= step/2;
    }
    if(diff >= step/4)
        code |= 0x01;

    decode_sample(state, code);
    return code;
}

short okiCodec::decode_sample(state_t *state, unsigned char code)
{
    short diff, step, sample;

    step = oki_steps[state->ssindex];
    diff = step / 8;
    if(code & 0x01)
        diff += step / 4;
    if(code & 0x02)
        diff += step / 2;
    if(code & 0x04)
        diff += step;
    if(code & 0x08)
        diff = -diff;
    sample = state->last + diff;
    if(sample > 2047)
        sample = 2047;
    else if(sample < -2047)
        sample = -2047;
    state->last = sample;
    state->ssindex += oki_index[code & 0x07];
    if(state->ssindex < 0)
        state->ssindex = 0;
    if(state->ssindex > 48)
        state->ssindex = 48;
    return sample;
}

unsigned okiCodec::encode(linear_t buffer, void *coded, unsigned lsamples)
{
    unsigned count = (lsamples / 2) * 2;
    bool hi = false;
    unsigned char byte = 0;
    encoded_t dest = (encoded_t)coded;

    while(count--) {
        if(hi) {
            byte |= encode_sample(&encode_state, *(buffer++) / 16 );
            *(dest++) = byte;
        }
        else
            byte = encode_sample(&encode_state, *(buffer++) / 16 ) << 4 ;
    }
    return (lsamples / 2) * 2;
}

unsigned okiCodec::decode(linear_t buffer, void *from, unsigned lsamples)
{
    encoded_t src = (encoded_t)from;
    unsigned count = lsamples / 2;
    unsigned char byte;

    while(count--) {
        byte = ((*src >> 4) & 0x0f);
        *(buffer++) = (decode_sample(&decode_state, byte) * 16);
        byte = (*src & 0x0f);
        *(buffer++) = (decode_sample(&decode_state, byte) * 16);
        ++src;
    }
    return (lsamples / 2) * 2;
}

AudioCodec *okiCodec::getByInfo(info_t& info)
{
    return (AudioCodec *)new okiCodec(info.encoding);
}

AudioCodec *okiCodec::getByFormat(const char *format)
{
    return (AudioCodec *)new okiCodec(info.encoding);
}

#if defined(HAVE_GSM_H) || defined(HAVE_GSM_GSM_H)

static class __LOCAL GSMCodec : private AudioCodec
{
private:
    gsm encoder, decoder;
    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);

    unsigned encode(linear_t data, void *dest, unsigned samples);
    unsigned decode(linear_t data, void *source, unsigned samples);

public:
    GSMCodec(const char *id, encoding_t e);
    GSMCodec();
    ~GSMCodec();
} gsm_codec("gsm", Audio::gsmVoice);

GSMCodec::GSMCodec()
{
    encoder = gsm_create();
    decoder = gsm_create();
    info.framesize = 33;
    info.framecount = 160;
    info.rate = 8000;
    info.bitrate = 13200;
    info.annotation = (char *)"gsm";
    info.encoding = gsmVoice;
}

GSMCodec::GSMCodec(const char *id, encoding_t e) : AudioCodec(id, e)
{
    encoder = gsm_create();
    decoder = gsm_create();
    info.framesize = 33;
    info.framecount = 160;
    info.rate = 8000;
    info.bitrate = 13200;
    info.annotation = (char *)"gsm";
}

GSMCodec::~GSMCodec()
{
    gsm_destroy(encoder);
    gsm_destroy(decoder);
}

AudioCodec *GSMCodec::getByInfo(info_t& info)
{
    return (AudioCodec *)new GSMCodec();
}

AudioCodec *GSMCodec::getByFormat(const char *format)
{
    return (AudioCodec *)new GSMCodec();
}

unsigned GSMCodec::encode(linear_t from, void *dest, unsigned samples)
{
    unsigned count = samples / 160;
    unsigned result = count * 33;
    gsm_byte *encoded = (gsm_byte *)dest;

    if(!count)
        return 0;

    while(count--) {
        gsm_encode(encoder, from, encoded);
        from += 160;
        encoded += 33;
    }
    return result;
}

unsigned GSMCodec::decode(linear_t dest, void *from, unsigned samples)
{
    unsigned count = samples / 160;
    unsigned result = count * 33;
    gsm_byte *encoded = (gsm_byte *)from;
    if(!count)
        return 0;

    while(count--) {
        gsm_decode(decoder, encoded, dest);
        encoded += 160;
        dest += 160;
    }
    return result;
}

#endif

#ifdef  HAVE_SPEEX_SPEEX_H

static class __LOCAL SpeexCommon: public AudioCodec
{
protected:
    const SpeexMode *spx_mode;
    SpeexBits enc_bits, dec_bits;
    unsigned int spx_clock, spx_channel;
    void *encoder, *decoder;
    int spx_frame;

public:
    SpeexCommon(encoding_t enc, const char *name);
    SpeexCommon();
    ~SpeexCommon();

    unsigned encode(linear_t buffer, void *dest, unsigned lsamples);
    unsigned decode(linear_t buffer, void *source, unsigned lsamples);

    AudioCodec *getByInfo(info_t& info);
    AudioCodec *getByFormat(const char *format);
} speex_codec(Audio::speexVoice, "speex");

class __LOCAL SpeexAudio: public SpeexCommon
{
public:
    SpeexAudio();
};

class __LOCAL SpeexVoice: public SpeexCommon
{
public:
    SpeexVoice();
};

SpeexCommon::SpeexCommon(encoding_t enc, const char *id) :
AudioCodec("speex", enc)
{
    info.framesize = 20;
    info.framecount = 160;
    info.rate = 8000;
    info.bitrate = 24000;
    info.annotation = (char *)"speex/8000";

    spx_channel = 1;

    switch(enc) {
    case speexVoice:
        spx_clock = 8000;
        spx_mode = &speex_nb_mode;
        break;
    case speexAudio:
        info.annotation = (char *)"speex/16000";
        info.framesize = 40;
        info.rate = 16000;
        spx_clock = 16000;
        spx_mode = &speex_wb_mode;
    default:
        break;
    }

    encoder = decoder = NULL;
}

SpeexCommon::SpeexCommon() :
AudioCodec()
{
}

SpeexCommon::~SpeexCommon()
{
    if(decoder) {
        speex_bits_destroy(&dec_bits);
        speex_decoder_destroy(decoder);
    }
    if(encoder) {
        speex_bits_destroy(&enc_bits);
        speex_encoder_destroy(encoder);
    }
    decoder = encoder = NULL;
}

AudioCodec *SpeexCommon::getByFormat(const char *format)
{
    if(!strnicmp(format, "speex/16", 8))
        return (AudioCodec *)new SpeexAudio();
    return (AudioCodec *)new SpeexVoice();
}

AudioCodec *SpeexCommon::getByInfo(info_t& info)
{
    switch(info.encoding) {
    case speexAudio:
            return (AudioCodec *)new SpeexAudio();
    default:
        return (AudioCodec *)new SpeexVoice();
    }
}

unsigned SpeexCommon::decode(linear_t buffer, void *src, unsigned lsamples)
{
    unsigned count = lsamples / info.framecount;
    unsigned result = 0;
    char *encoded = (char *)src;

    if(!count)
        return 0;

    while(count--) {
        speex_bits_read_from(&dec_bits, encoded, info.framesize);
        if(speex_decode_int(decoder, &dec_bits, buffer))
            break;
        result += info.framesize;
    }
    return result;
}

unsigned SpeexCommon::encode(linear_t buffer, void *dest, unsigned lsamples)
{
    unsigned count = lsamples / info.framecount;
    unsigned result = 0;
    char *encoded = (char *)dest;

    if(!count)
        return 0;

    while(count--) {
        speex_bits_reset(&enc_bits);
        speex_encoder_ctl(encoder, SPEEX_SET_SAMPLING_RATE, &spx_clock);
        speex_encode_int(encoder, buffer, &enc_bits);
        int nb = speex_bits_write(&enc_bits, encoded, info.framesize);
        buffer += 160;
        encoded += nb;
        result += nb;
    }
    return result;
}

SpeexAudio::SpeexAudio() :
SpeexCommon()
{
    info.encoding = speexVoice;
    info.framesize = 40;
    info.framecount = 160;
    info.rate = 16000;
    info.bitrate = 48000;
    info.annotation = (char *)"SPEEX/16000";
    spx_clock = 16000;
    spx_channel = 1;
    spx_mode = &speex_wb_mode;
    speex_bits_init(&dec_bits);
    decoder = speex_decoder_init(spx_mode);
    speex_bits_init(&enc_bits);
    encoder = speex_encoder_init(spx_mode);
    speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &spx_frame);
    info.framecount = spx_frame;
    info.set();
}

SpeexVoice::SpeexVoice() :
SpeexCommon()
{
    info.encoding = speexVoice;
    info.framesize = 20;
    info.framecount = 160;
    info.rate = 8000;
    info.bitrate = 24000;
    info.annotation = (char *)"SPEEX/8000";
    spx_clock = 8000;
    spx_channel = 1;
    spx_mode = &speex_nb_mode;
    speex_bits_init(&dec_bits);
    decoder = speex_decoder_init(spx_mode);
    speex_bits_init(&enc_bits);
    encoder = speex_encoder_init(spx_mode);
    speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &spx_frame);
    info.framecount = spx_frame;
    info.set();
}

#endif

