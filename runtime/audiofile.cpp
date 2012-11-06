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

#if defined(__GNUC__)
#define _PACKED
#elif !defined(__hpux) && !defined(_AIX)
#define _PACKED
#endif

#if !defined(__BIG_ENDIAN)
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412
#define __BYTE_ORDER    __LITTLE_ENDIAN
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

AudioFile::AudioFile(const char *name, unsigned long sample)
{
    pathname = NULL;
    initialize();
    AudioFile::open(name);
    if(!is_open())
        return;
    setPosition(sample);
}

AudioFile::AudioFile(const char *name, info_t& inf, unsigned long samples)
{
    info = inf;
    pathname = NULL;
    initialize();
    AudioFile::create(name, inf);
    if(!is_open())
        return;
    setMinimum(samples);
}

AudioFile::AudioFile()
{
    pathname = NULL;
    initialize();
}

AudioFile::~AudioFile()
{
    AudioFile::close();
    AudioFile::clear();
}

ssize_t AudioFile::read(void *buffer, size_t request)
{
    ssize_t result = fs.read(buffer, request);
    if(result > 0)
        pos += result;
    if(pos > eof)
        eof = pos;
    return result;
}

ssize_t AudioFile::write(void *buffer, size_t request)
{
    ssize_t result = fs.write(buffer, request);
    if(result > 0)
        pos += result;
    if(pos > eof)
        eof = pos;
    return result;
}

ssize_t AudioFile::peek(void *buffer, size_t request)
{
    ssize_t result = fs.read(buffer, request);
    if(result > 0)
        fs.seek(pos);
    return result;
}

bool AudioFile::seek(size_t offset)
{
    if(!fs.seek(offset)) {
        pos = offset;
        return true;
    }
    return false;
}

AudioCodec *AudioFile::getCodec(void)
{
    encoding_t e = getEncoding();
    switch(e) {
    case alawAudio:
        return AudioCodec::get(e, "g.711");
    case mulawAudio:
        return AudioCodec::get(e, "g.711");
    case g721ADPCM:
    case okiADPCM:
    case voxADPCM:
        return AudioCodec::get(e, "g.721");
    case g722_7bit:
    case g722_6bit:
        return AudioCodec::get(e, "g.722");
    case g723_3bit:
    case g723_5bit:
        return AudioCodec::get(e, "g.723");
    default:
        return NULL;
    }
}

void AudioFile::setShort(unsigned char *data, unsigned short val)
{
    if(info.order == __BIG_ENDIAN) {
        data[0] = val / 256;
        data[1] = val % 256;
    }
    else {
        data[1] = val / 256;
        data[0] = val % 256;
    }
}

unsigned short AudioFile::getShort(unsigned char *data) const
{
    if(info.order == __BIG_ENDIAN)
        return data[0] * 256 + data[1];
    else
        return data[1] * 256 + data[0];
}

void AudioFile::setLong(unsigned char *data, unsigned long val)
{
    int i = 4;

    while(i-- > 0) {
        if(info.order == __BIG_ENDIAN)
            data[i] = (unsigned char)(val & 0xff);
        else
            data[3 - i] = (unsigned char)(val & 0xff);
        val /= 256;
    }
}

unsigned long AudioFile::getLong(unsigned char * data) const
{
    int i = 4;
    unsigned long val =0;

    while(i-- > 0) {
        if(info.order == __BIG_ENDIAN)
            val = (val << 8) | data[3 - i];
        else
            val = (val << 8) | data[i];
    }
    return val;
}

ssize_t AudioFile::putNative(encoded_t data, size_t bytes)
{
    if(!is(fs))
        return 0;

    swapEncoded(info, data, bytes);
    return putBuffer(data, bytes);
}

ssize_t AudioFile::getNative(encoded_t data, size_t bytes)
{
    ssize_t result = 0;

    if(is(fs))
        result = getBuffer(data, bytes);

    if(result < 1)
        return result;

    swapEncoded(info, data, result);
    return result;
}

void AudioFile::create(const char *name, info_t& myinfo, timeout_t framing)
{
    int pcm = 0;
    unsigned char aufile[24];
    unsigned char riffhdr[40];
    const char *ext = strrchr(name, '/');
    pos = 0l;

    if(!ext)
        ext = strrchr(name, '\\');

    if(!ext)
        ext = strrchr(name, ':');

    if(!ext)
        ext = strrchr(name, '.');
    else
        ext = strrchr(ext, '.');

    if(!ext)
        ext = ".none";

    mode = modeWrite;
    fs.create(name, fsys::ACCESS_STREAM, 0660);
    if(!is(fs))
        return;

    memset(riffhdr, 0, sizeof(riffhdr));
    info = myinfo;
    info.annotation = NULL;
    pathname = new char[strlen(name) + 1];
    strcpy(pathname, name);
    if(myinfo.annotation) {
        info.annotation = new char[strlen(myinfo.annotation) + 1];
        strcpy(info.annotation, myinfo.annotation);
    }

    if(!stricmp(ext, ".raw") || !stricmp(ext, ".bin")) {
        info.format = raw;
        if(info.encoding == unknownEncoding)
            info.encoding = pcm16Mono;
    }
    else if(!stricmp(ext, ".au") || !stricmp(ext, ".snd"))
    {
        info.order = 0;
        info.format = snd;
    }
    else if(!stricmp(ext, ".wav") || !stricmp(ext, ".wave"))
    {
        info.order = 0;
        info.format = wave;
    }
    else if(!stricmp(ext, ".ul") || !stricmp(ext, ".ulaw") || !stricmp(ext, ".mulaw"))
    {
        info.encoding = mulawAudio;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".al") || !stricmp(ext, ".alaw"))
    {
        info.encoding = alawAudio;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".sw") || !stricmp(ext, ".pcm"))
    {
        info.encoding = pcm16Mono;
        info.format = raw;
        info.order = 0;
    }
    else if(!stricmp(ext, ".vox"))
    {
        info.encoding = voxADPCM;
        info.format = raw;
        info.order = 0;
        info.rate = 6000;
    }
    else if(!stricmp(ext, ".gsm"))
    {
        info.encoding = gsmVoice;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
        info.framecount = 160;
        info.framesize = 33;
        info.bitrate = 13200;
    }
    else if(!stricmp(ext, ".adpcm") || !stricmp(ext, ".a32"))
    {
        info.encoding = g721ADPCM;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".a24"))
    {
        info.encoding = g723_3bit;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".a16"))
    {
        info.encoding = g723_2bit;
        info.format = raw;
        info.order = 0;
        info.order = 8000;
    }
    else if(!stricmp(ext, ".sx"))
    {
        info.encoding = sx96Voice;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".a40"))
    {
        info.encoding = g723_5bit;
        info.format = raw;
        info.order = 0;
        info.rate = 8000;
    }
    else if(!stricmp(ext, ".cda"))
    {
        info.encoding = cdaStereo;
        info.format = raw;
        info.order = __LITTLE_ENDIAN;
        info.rate = 44100;
    }

    switch(info.format) {
    case wave:
    case riff:
        /*
         * RIFF header
         *
         * 12 bytes
         *
         * 0-3: RIFF magic "RIFF" (0x52 49 46 46)
         *
         * 4-7: RIFF header chunk size
         *      (4 + (8 + subchunk 1 size) + (8 + subchunk 2 size))
         *
         * 8-11: WAVE RIFF type magic "WAVE" (0x57 41 56 45)
         */
        if(!info.order)
            info.order = __LITTLE_ENDIAN;
        if(info.order == __LITTLE_ENDIAN)
            strncpy((char *)riffhdr, "RIFF", 4);
        else
            strncpy((char *)riffhdr, "RIFX", 4);
        if(!info.rate)
            info.rate = Audio::getRate(info.encoding);
        if(!info.rate)
            info.rate = rate8khz;

        header = 0;

        memset(riffhdr + 4, 0xff, 4);
        strncpy((char *)riffhdr + 8, "WAVE", 4);
        if(write(riffhdr, 12) != 12) {
            AudioFile::close();
            return;
        }

        /*
         * Subchunk 1: WAVE metadata
         *
         * Length: 24+
         *
         * (offset from start of subchunk 1) startoffset-endoffset
         *
         * (0) 12-15: WAVE metadata magic "fmt " (0x 66 6d 74 20)
         *
         * (4) 16-19: subchunk 1 size minus 8.  0x10 for PCM.
         *
         * (8) 20-21: Audio format code.  0x01 for linear PCM.
         * More codes here:
         * http://www.goice.co.jp/member/mo/formats/wav.html
         *
         * (10) 22-23: Number of channels.  Mono = 0x01,
         * Stereo = 0x02.
         *
         * (12) 24-27: Sample rate in samples per second. (8000,
         * 44100, etc)
         *
         * (16) 28-31: Bytes per second = SampleRate *
         * NumChannels * (BitsPerSample / 8)
         *
         * (20) 32-33: Block align (the number of bytes for
         * one multi-channel sample) = Numchannels *
         * (BitsPerSample / 8)
         *
         * (22) 34-35: Bits per single-channel sample.  8 bits
         * per channel sample = 0x8, 16 bits per channel
         * sample = 0x10
         *
         * (24) 36-37: Size of extra PCM parameters for
         * non-PCM formats.  If a PCM format code is
         * specified, this doesn't exist.
         *
         * Subchunk 3: Optional 'fact' subchunk for non-PCM formats
         * (26) 38-41: WAVE metadata magic 'fact' (0x 66 61 63 74)
         *
         * (30) 42-45: Length of 'fact' subchunk, not
         * including this field and the fact
         * identification field (usually 4)
         *
         * (34) 46-49: ??? sndrec32.exe outputs 0x 00 00 00 00
         * here.  See
         * http://www.health.uottawa.ca/biomech/csb/ARCHIVES/riff-for.txt
         *
         */

        memset(riffhdr, 0, sizeof(riffhdr));
        strncpy((char *)riffhdr, "fmt ", 4);
        // FIXME A bunch of the following only works for PCM
        // and mulaw/alaw, so you'll probably have to fix it
        // if you want to use one of the other formats.
        if(info.encoding < cdaStereo)
            setLong(riffhdr + 4, 18);
        else
            setLong(riffhdr + 4, 16);

        setShort(riffhdr + 8, 0x01); // default in case invalid encoding specified
        if(is_mono(info.encoding))
            setShort(riffhdr + 10, 1);
        else
            setShort(riffhdr + 10, 2);
        setLong(riffhdr + 12, info.rate);
        setLong(riffhdr + 16, (unsigned long)toBytes(info, info.rate));
        setShort(riffhdr + 20, (snd16_t)toBytes(info, 1));
        setShort(riffhdr + 22, 0);

        switch(info.encoding) {
        case pcm8Mono:
        case pcm8Stereo:
            setShort(riffhdr + 22, 8);
            pcm = 1;
            break;
        case pcm16Mono:
        case pcm16Stereo:
        case cdaMono:
        case cdaStereo:
            setShort(riffhdr + 22, 16);
            pcm = 1;
            break;
        case pcm32Mono:
        case pcm32Stereo:
            setShort(riffhdr + 22, 32);
            pcm = 1;
            break;
        case alawAudio:
            setShort(riffhdr + 8, 6);
            setShort(riffhdr + 22, 8);
            break;
        case mulawAudio:
            setShort(riffhdr + 8, 7);
            setShort(riffhdr + 22, 8);
            break;

            // FIXME I'm pretty sure these are supposed to
            // be writing to offset 22 instead of 24...
        case okiADPCM:
            setShort(riffhdr + 8, 0x10);
            setShort(riffhdr + 24, 4);
            break;
        case voxADPCM:
            setShort(riffhdr + 8, 0x17);
            setShort(riffhdr + 24, 4);
            break;
        case g721ADPCM:
            setShort(riffhdr + 8, 0x40);
            setShort(riffhdr + 24, 4);
            break;
        case g722Audio:
            setShort(riffhdr + 8, 0x64);
            setShort(riffhdr + 24, 8);
            break;
        case gsmVoice:
        case msgsmVoice:
            setShort(riffhdr + 8, 0x31);
            setShort(riffhdr + 24, 260);
            break;
        case g723_3bit:
            setShort(riffhdr + 8, 0x14);
            setShort(riffhdr + 24, 3);
            break;
        case g723_5bit:
            setShort(riffhdr + 8, 0x14);
            setShort(riffhdr + 24, 5);
        case unknownEncoding:
        default:
            break;
        }

        if(pcm == 0) {
            setShort(riffhdr + 24, 0);
            strncpy((char *)(riffhdr + 26), "fact", 4);
            setLong(riffhdr + 30, 4);
            setLong(riffhdr + 34, 0);
            if(write(riffhdr, 38) != 38) {
                AudioFile::close();
                return;
            }
        }
        else {
            if(write(riffhdr, 24) != 24) {
                AudioFile::close();
                return;
            }
        }

        /*
         * Subchunk 2: data subchunk
         *
         * Length: 8+
         *
         * (0) 36-39: data subchunk magic "data" (0x 64 61 74 61)
         *
         * (4) 40-43: subchunk 2 size =
         * NumSamples * NumChannels * (BitsPerSample / 8)
         * Note that this does not include the size of this
         * field and the previous one.
         *
         * (8) 44+: Samples
         */

        memset(riffhdr, 0, sizeof(riffhdr));
        strncpy((char *)riffhdr, "data", 4);
        memset(riffhdr + 4, 0xff, 4);
        if(write(riffhdr, 8) != 8) {
            AudioFile::close();
            return;
        }

        header = getAbsolutePosition();
        break;

    case snd:
//      if(!info.order)
            info.order = __BIG_ENDIAN;

        if(!info.rate)
            info.rate = Audio::getRate(info.encoding);
        if(!info.rate)
            info.rate = rate8khz;

        strncpy((char *)aufile, ".snd", 4);
        if(info.annotation)
            setLong(aufile + 4, 24 + (unsigned long)strlen(info.annotation) + 1);
        else
            setLong(aufile + 4, 24);
        header = getLong(aufile + 4);
        setLong(aufile + 8, ~0l);
        switch(info.encoding) {
        case pcm8Stereo:
        case pcm8Mono:
            setLong(aufile + 12, 2);
            break;
        case pcm16Stereo:
        case pcm16Mono:
        case cdaStereo:
        case cdaMono:
            setLong(aufile + 12, 3);
            break;
        case pcm32Stereo:
        case pcm32Mono:
            setLong(aufile + 12, 5);
            break;
        case g721ADPCM:
            setLong(aufile + 12, 23);
            break;
        case g722Audio:
        case g722_7bit:
        case g722_6bit:
            setLong(aufile + 12, 24);
            break;
        case g723_3bit:
            setLong(aufile + 12, 25);
            break;
        case g723_5bit:
            setLong(aufile + 12, 26);
            break;
        case gsmVoice:
            setLong(aufile + 12, 28);
            break;
        case alawAudio:
            setLong(aufile + 12, 27);
            break;
        default:
            setLong(aufile + 12, 1);
        }
        setLong(aufile + 16, info.rate);
        if(is_mono(info.encoding))
            setLong(aufile + 20, 1);
        else
            setLong(aufile + 20, 2);
        if(write(aufile, 24) != 24) {
            AudioFile::close();
            return;
        }
        if(info.annotation)
            write((unsigned char *)info.annotation, (unsigned long)strlen(info.annotation) + 1);
        header = getAbsolutePosition();
        break;
    case raw:
        break;
    }
    if(framing)
        info.setFraming(framing);
    else
        info.set();
}

void AudioFile::getWaveFormat(int request)
{
    unsigned char filehdr[24];
    int bitsize;
    int channels;

    if(request > 24)
        request = 24;

    if(peek(filehdr, request) < 1) {
        AudioFile::close();
        return;
    }
    channels = getShort(filehdr + 2);
    info.rate = getLong(filehdr + 4);

    switch(getShort(filehdr)) {
    case 1:
        bitsize = getShort(filehdr + 14);
        switch(bitsize) {
        case 8:
            if(channels > 1)
                info.encoding = pcm8Stereo;
            else
                info.encoding = pcm8Mono;
            break;
        case 16:
            if(info.rate == 44100) {
                if(channels > 1)
                    info.encoding = cdaStereo;
                else
                    info.encoding = cdaMono;
                break;
            }
            if(channels > 1)
                    info.encoding = pcm16Stereo;
            else
                info.encoding = pcm16Mono;
            break;
        case 32:
            if(channels > 1)
                info.encoding = pcm32Stereo;
            else
                info.encoding = pcm32Mono;
            break;
        default:
            info.encoding = unknownEncoding;
        }
        break;
    case 6:
        info.encoding = alawAudio;
        break;
    case 7:
        info.encoding = mulawAudio;
        break;
    case 0x10:
        info.encoding = okiADPCM;
        break;
    case 0x17:
        info.encoding = voxADPCM;
        break;
    case 0x40:
        info.encoding = g721ADPCM;
        break;
    case 0x65:
        info.encoding = g722Audio;
        break;
    case 0x31:
        info.encoding = msgsmVoice;
        break;
    case 0x14:
        bitsize = getLong(filehdr + 8) * 8 / info.rate;
        if(bitsize == 3)
            info.encoding = g723_3bit;
        else
            info.encoding = g723_5bit;
        break;
    default:
        info.encoding = unknownEncoding;
    }
}

void AudioFile::open(const char *name, mode_t m, timeout_t framing)
{
    unsigned char filehdr[24];
    unsigned int count;
    char *ext;
    unsigned channels;
    mode = m;
    pos = 0;
    fsys::access_t fm;
    fsys::fileinfo_t ino;

    switch(mode) {
    case modeWrite:
        fm = fsys::ACCESS_REWRITE;
        break;
    case modeAppend:
        fm = fsys::ACCESS_APPEND;
        break;
    default:
        fm = fsys::ACCESS_STREAM;
    }

    for(;;) {
        fs.open(name, fm);
        if(is(fs))
            break;

        fsys::info(name, &ino);
        eof = ino.st_size;

        if(mode == modeReadAny || mode == modeReadOne)
            name = getContinuation();
        else
            name = NULL;
    }

    if(!name)
        return;

    pathname = new char[strlen(name) + 1];
    strcpy(pathname, name);
    header = 0l;

    info.framesize = 0;
    info.framecount = 0;
    info.encoding = mulawAudio;
    info.format = raw;
    info.order = 0;
    ext = strrchr(pathname, '.');
    if(!ext)
        goto done;

    info.encoding = Audio::getEncoding(ext);
    switch(info.encoding) {
    case cdaStereo:
        info.order = __LITTLE_ENDIAN;
        break;
    case unknownEncoding:
        info.encoding = mulawAudio;
    default:
        break;
    }

    strcpy((char *)filehdr, ".xxx");

    if(peek(filehdr, 24) < 1) {
        AudioFile::close();
        return;
    }

    if(!strncmp((char *)filehdr, "RIFF", 4)) {
        info.format = riff;
        info.order = __LITTLE_ENDIAN;
    }

    if(!strncmp((char *)filehdr, "RIFX", 4)) {
        info.order = __BIG_ENDIAN;
        info.format = riff;
    }

    if(!strncmp((char *)filehdr + 8, "WAVE", 4) && info.format == riff) {
        header = 12;
        for(;;)
        {
            if(!seek(header)) {
                AudioFile::close();
                return;
            }
            if(peek(filehdr, 8) < 1) {
                AudioFile::close();
                return;
            }
            header += 8;
            if(!strncmp((char *)filehdr, "data", 4))
                break;

            count = getLong(filehdr + 4);
            header += count;
            if(!strncmp((char *)filehdr, "fmt ", 4))
                getWaveFormat(count);

        }
        seek(header);
        goto done;
    }

    if(!strncmp((char *)filehdr, ".snd", 4)) {
        info.format = snd;
        info.order = __BIG_ENDIAN;
        header = getLong(filehdr + 4);
        info.rate = getLong(filehdr + 16);
        channels = getLong(filehdr + 20);

        switch(getLong(filehdr + 12)) {
        case 2:
            if(channels > 1)
                info.encoding = pcm8Stereo;
            else
                info.encoding = pcm8Mono;
            break;
        case 3:
            if(info.rate == 44100) {
                if(channels > 1)
                    info.encoding = cdaStereo;
                else
                    info.encoding = cdaMono;
                break;
            }
            if(channels > 1)
                info.encoding = pcm16Stereo;
            else
                info.encoding = pcm16Mono;
            break;
        case 5:
            if(channels > 1)
                info.encoding = pcm32Stereo;
            else
                info.encoding = pcm32Mono;
            break;
        case 23:
            info.encoding = g721ADPCM;
            break;
        case 24:
            info.encoding = g722Audio;
            break;
        case 25:
            info.encoding = g723_3bit;
            break;
        case 26:
            info.encoding = g723_5bit;
            break;
        case 27:
            info.encoding = alawAudio;
            break;
        case 28:
            info.encoding = gsmVoice;
            break;
        case 1:
            info.encoding = mulawAudio;
            break;
        default:
            info.encoding = unknownEncoding;
        }
        if(header > 24) {
            info.annotation = new char[header - 24];
            seek(24);
            read((unsigned char *)info.annotation, header - 24);
        }
        goto done;
    }

    seek(0);

done:
    info.headersize = 0;
    if(framing)
        info.setFraming(framing);
    else
        info.set();

    if(mode == modeFeed) {
        fsys::fileinfo_t ino;
        fsys::info(name, &ino);
        iolimit = ino.st_size;
    }
}

void AudioFile::close(void)
{
    unsigned char buf[58];

    if(!is(fs))
        return;

    if(mode != modeWrite) {
        fs.close();
        clear();
        return;
    }

    if(!seek(0)) {
        fs.close();
        clear();
        return;
    }

    if(-1 == peek(buf, 58)) {
        fs.close();
        clear();
        return;
    }

    switch(info.format) {
    case riff:
    case wave:
        // RIFF header
        setLong(buf + 4, eof - 8);

        // If it's a non-PCM datatype, the offsets are a bit
        // different for subchunk 2.
        switch(info.encoding) {
        case cdaStereo:
        case cdaMono:
        case pcm8Stereo:
        case pcm8Mono:
        case pcm16Stereo:
        case pcm16Mono:
        case pcm32Stereo:
        case pcm32Mono:
            setLong(buf + 40, eof - header);
            break;
        default:
            setLong(buf + 54, eof - header);
        }

        write(buf, 58);
        break;
    case snd:
        setLong(buf + 8, eof - header);
        write(buf, 12);
        break;
    default:
        break;
    }
    fs.close();
    clear();
}

void AudioFile::clear(void)
{
    if(pathname) {
        if(pathname)
            delete[] pathname;
        pathname = NULL;
    }
    if(info.annotation) {
        if(info.annotation)
            delete[] info.annotation;
        info.annotation = NULL;
    }
    minimum = 0l;
    iolimit = 0l;
    pos = eof = 0l;
}

void AudioFile::initialize(void)
{
    pos = eof = 0l;
    minimum = 0l;
    pathname = NULL;
    info.annotation = NULL;
    header = 0l;
    iolimit = 0l;
    mode = modeInfo;
    fs.close();
}

int AudioFile::getSamples(void *addr, unsigned request)
{
    const char *fname;
    unsigned char *caddr = (unsigned char *)addr;
    ssize_t count, bytes;

    if(!request)
        request = info.framecount;

    for(;;)
    {
        bytes = (int)toBytes(info, request);
        if(bytes < 1)
            return EINVAL;
        count = read(caddr, bytes);
        if(count == bytes)
            return 0;

        if(count < 0)
            return fs.err();

        if(count > 0) {
            caddr += count;
            count = request - toSamples(info.encoding, count);
        }
        else
            count = request;

        if(mode == modeFeed) {
            setPosition(0);
            request = count;
            continue;
        }

retry:
        if(mode == modeReadOne)
            fname = NULL;
        else
            fname = getContinuation();

        if(!fname)
            break;

        AudioFile::close();
        AudioFile::open(fname, modeRead);
        if(!is(fs)) {
            if(mode == modeReadAny)
                goto retry;
            break;
        }

        request = count;
    }
    if(count)
        Audio::fill(caddr, count, info.encoding);
    return EPIPE;
}

int AudioFile::putSamples(void *addr, unsigned samples)
{
    int count;
    int bytes;

    if(!samples)
        samples = info.framecount;

    bytes = (int)toBytes(info, samples);
    if(bytes < 1)
        return EINVAL;

    count = write((unsigned char *)addr, bytes);
    if(count == bytes)
        return 0;
    else if(count < 1)
        return fs.err();
    else
        return EPIPE;
}

ssize_t AudioFile::putBuffer(encoded_t addr, size_t len)
{
    ssize_t count;
    unsigned long curpos;

    if(!len)
        len = info.framesize;

    curpos = (unsigned long)toBytes(info, getPosition());
    if(curpos >= iolimit && mode == modeFeed) {
        curpos = 0;
        setPosition(0);
    }

    if (iolimit && ((curpos + len) > iolimit))
        len = iolimit - curpos;

    if (len <= 0)
        return 0;

    count = write((unsigned char *)addr, (unsigned)len);
    if(count == (ssize_t)len)
        return count;
    if(count < 1)
        return count;
    else
        return count;
}

ssize_t AudioFile::getBuffer(encoded_t addr, size_t bytes)
{
    info_t prior;
    const char *fname;
    ssize_t count;
    unsigned long curpos, xfer = 0;

    if(!bytes)
        bytes = info.framesize;

    curpos = (unsigned long)toBytes(info, getPosition());
    if(curpos >= iolimit && mode == modeFeed) {
        curpos = 0;
        setPosition(0);
    }
    if (iolimit && ((curpos + bytes) > iolimit))
        bytes = iolimit - curpos;
    if (bytes < 0)
        bytes = 0;

    getInfo(prior);

    for(;;)
    {
        count = read(addr, (unsigned)bytes);
        if(count < 0) {
            if(!xfer)
                return count;
            return xfer;
        }
        xfer += count;
        if(count == (ssize_t)bytes)
            return xfer;
        if(mode == modeFeed) {
            setPosition(0l);
            goto cont;
        }

retry:
        if(mode == modeReadOne)
            fname = NULL;
        else
            fname = getContinuation();

        if(!fname)
            return xfer;
        AudioFile::close();
        AudioFile::open(fname, modeRead, info.framing);
        if(!is(fs)) {
            if(mode == modeReadAny)
                goto retry;
            return xfer;
        }

        if(prior.encoding != info.encoding) {
            AudioFile::close();
            return xfer;
        }
cont:
        bytes -= count;
        addr += count;
    }
}


int AudioFile::setPosition(unsigned long samples)
{
    size_t offset;
    fsys::fileinfo_t ino;

    if(!is(fs))
        return EBADF;

    fs.info(&ino);

    if(samples == (unsigned long)~0l)
        return seek(ino.st_size);

    offset = header + toBytes(info, samples);
    if(offset > (size_t)ino.st_size)
        offset = ino.st_size;

    return seek(offset);
}

unsigned long AudioFile::getPosition(void)
{
    if(!is(fs) || !pos)
        return 0;

    return toSamples(info, (pos - header));
}

int AudioFile::setMinimum(unsigned long samples)
{
    if(!is(fs))
        return EBADF;
    minimum = samples;
    return 0;
}



const char *AudioFile::getContinuation(void)
{
    return NULL;
}
