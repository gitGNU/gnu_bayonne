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

AudioStream::AudioStream() : AudioFile()
{
    codec = NULL;
    framebuf = NULL;
    bufferFrame = NULL;
    encBuffer = NULL;
    decBuffer = NULL;
    encSize = decSize = 0;
    bufferPosition = 0;
}

AudioStream::AudioStream(const char *fname, mode_t m, timeout_t framing) :
AudioFile()
{
    codec = NULL;
    framebuf = NULL;
    bufferFrame = NULL;
    bufferPosition = 0;

    open(fname, m, framing);
}

AudioStream::AudioStream(const char *fname, info_t& info, timeout_t framing) :
AudioFile()
{
    codec = NULL;
    framebuf = NULL;
    bufferFrame = NULL;
    bufferPosition = 0;

    create(fname, info, framing);
}

AudioStream::~AudioStream()
{
    AudioStream::close();
    AudioFile::clear();
}

ssize_t AudioStream::getBuffer(encoded_t data, size_t request)
{
    if(!request)
        return getPacket(data);

    return AudioFile::getBuffer(data, request);
}

ssize_t AudioStream::getPacket(encoded_t data)
{
    size_t count;
    unsigned status = 0;

    if(!is_streamable())
        return AudioFile::getBuffer(data, 0);

    for(;;)
    {
        count = codec->getEstimated();
        if(count)
            status = AudioFile::getBuffer(framebuf, count);
        if(count && (size_t)status != count)
            return 0;

        status = codec->getPacket(data, framebuf, status);
        if(status == Audio::ndata)
            break;

        if(status)
            return status;
    }

    return 0;
}

bool AudioStream::is_streamable(void)
{
    if(!is_open())
        return false;

    if(!streamable)
        return false;

    return true;
}

void AudioStream::flush(void)
{
    unsigned fpos;

    if(!bufferFrame)
        return;

    if(bufferPosition) {
        for(fpos = bufferPosition; fpos < getCount() * bufferChannels; ++fpos)
            bufferFrame[fpos] = 0;
        if(bufferChannels == 1)
            putMono(bufferFrame, 1);
        else
            putStereo(bufferFrame, 1);
    }

    delete[] bufferFrame;
    bufferFrame = NULL;
    bufferPosition = 0;
}

void AudioStream::close(void)
{
    flush();

    if(codec)
        AudioCodec::release(codec);

    if(framebuf)
        delete[] framebuf;

    if(encBuffer)
        delete[] encBuffer;

    if(decBuffer)
        delete[] decBuffer;

    encSize = decSize = 0;
    encBuffer = decBuffer = NULL;
    framebuf = NULL;
    codec = NULL;
    AudioFile::close();
}

void AudioStream::create(const char *fname, info_t& info, timeout_t framing)
{
    if(!framing)
        framing = 20;

    close();
    AudioFile::create(fname, info, framing);
    if(!is_open())
        return;

    streamable = true;

    if(is_linear(AudioFile::info.encoding))
        return;

    codec = AudioCodec::get(AudioFile::info);
    if(!codec) {
        streamable = false;
        return;
    }
    framebuf = new unsigned char[maxFramesize(AudioFile::info)];
}

void AudioStream::open(const char *fname, mode_t m, timeout_t framing)
{
    if(!framing)
        framing = 20;

    close();
    AudioFile::open(fname, m, framing);
    if(!is_open())
        return;

    streamable = true;

    if(is_linear(info.encoding))
        return;

    codec = AudioCodec::get(info);
    if(!codec)
        streamable = false;
    else
        framebuf = new unsigned char[maxFramesize(info)];
}

unsigned AudioStream::getCount(void)
{
    if(!is_streamable())
        return 0;

    return info.framecount;
}

unsigned AudioStream::getMono(linear_t buffer, unsigned frames)
{
    unsigned char *iobuf = (unsigned char *)buffer;
    unsigned count, offset, copied = 0;
    ssize_t len;
    linear_t dbuf = NULL;

    if(!is_streamable())
        return 0;

    if(!frames)
        ++frames;

    count = frames * getCount();

    if(is_stereo(info.encoding))
        dbuf = new sample_t[count * 2];
    if(codec)
        iobuf = framebuf;
    else if(dbuf)
        iobuf = (unsigned char *)dbuf;

    while(frames--) {
        len = AudioFile::getBuffer(iobuf);  // packet read
        if(len < (ssize_t)info.framesize)
            break;
        ++copied;
        if(codec) {
            codec->decode(buffer, iobuf, info.framecount);
            goto stereo;
        }

        if(dbuf)
            swapEndian(info, dbuf, info.framecount);
        else
            swapEndian(info, buffer, info.framecount);

stereo:
        if(!dbuf) {
            buffer += info.framecount;
            continue;
        }

        for(offset = 0; offset < info.framecount; ++offset)
            buffer[offset] =
                dbuf[offset * 2] / 2 + dbuf[offset * 2 + 1] / 2;

        buffer += info.framecount;
    }

    if(dbuf)
        delete[] dbuf;

    return copied;
}

unsigned AudioStream::getStereo(linear_t buffer, unsigned frames)
{
    unsigned char *iobuf = (unsigned char *)buffer;
    unsigned offset, copied = 0;
    ssize_t len;

    if(!is_streamable())
        return 0;

    if(!frames)
        ++frames;

    if(codec)
        iobuf = framebuf;

    while(frames--) {
        len = AudioFile::getBuffer(iobuf);  // packet read
        if(len < (ssize_t)info.framesize)
            break;
        ++copied;

        if(codec) {
            codec->decode(buffer, iobuf, info.framecount);
            goto stereo;
        }
        swapEndian(info, buffer, info.framecount);

stereo:
        if(is_stereo(info.encoding)) {
            buffer += (info.framecount * 2);
            continue;
        }
        offset = info.framecount;
        while(offset--) {
            buffer[offset * 2] = buffer[offset];
            buffer[offset * 2 + 1] = buffer[offset];
        }
        buffer += (info.framecount * 2);
    }
    return copied;
}

unsigned AudioStream::putMono(linear_t buffer, unsigned frames)
{
    linear_t iobuf = buffer, dbuf = NULL;
    unsigned offset, copied = 0;
    ssize_t len;

    if(!is_streamable())
        return 0;

    if(!frames)
        ++frames;

    if(is_stereo(info.encoding)) {
        dbuf = new sample_t[info.framecount * 2];
        iobuf = dbuf;
    }

    while(frames--) {
        if(dbuf) {
            for(offset = 0; offset < info.framecount; ++offset)
                dbuf[offset * 2] = dbuf[offset * 2 + 1] = buffer[offset];
        }

        if(codec) {
            codec->encode(iobuf, framebuf, info.framecount);
            len = putBuffer(framebuf);
            if(len < (ssize_t)info.framesize)
                break;
            ++copied;
            buffer += info.framecount;
            continue;
        }
        swapEndian(info, iobuf, info.framecount);
        len = putBuffer((encoded_t)iobuf);
        if(len < (ssize_t)info.framesize)
            break;
        ++copied;
        buffer += info.framecount;
    }
    if(dbuf)
        delete[] dbuf;

    return copied;
}

unsigned AudioStream::putStereo(linear_t buffer, unsigned frames)
{
    linear_t iobuf = buffer, mbuf = NULL;
    unsigned offset, copied = 0;
    ssize_t len;

    if(!is_streamable())
        return 0;

    if(!frames)
        ++frames;

    if(!is_stereo(info.encoding)) {
        mbuf = new sample_t[info.framecount];
        iobuf = mbuf;
    }

    while(frames--) {
        if(mbuf) {
            for(offset = 0; offset < info.framecount; ++offset)
                mbuf[offset] = buffer[offset * 2] / 2 + buffer[offset * 2 + 1] / 2;
        }

        if(codec) {
            codec->encode(iobuf, framebuf, info.framecount);
            len = putBuffer(framebuf);
            if(len < (ssize_t)info.framesize)
                break;
            ++copied;
            buffer += info.framecount;
            continue;
        }
        swapEndian(info, iobuf, info.framecount);
        len = putBuffer((encoded_t)iobuf);
        if(len < (ssize_t)info.framesize)
            break;
        ++copied;
    }
    if(mbuf)
        delete[] mbuf;

    return copied;
}

unsigned AudioStream::bufMono(linear_t samples, unsigned count)
{
    unsigned size = getCount();

    if(bufferChannels != 1)
        flush();

    if(!bufferFrame) {
        bufferFrame = new sample_t[size];
        bufferChannels = 1;
        bufferPosition = 0;
    }
    return bufAudio(samples, count, size);
}

unsigned AudioStream::bufStereo(linear_t samples, unsigned count)
{
    unsigned size = getCount() * 2;

    if(bufferChannels != 2)
        flush();

    if(!bufferFrame) {
        bufferFrame = new sample_t[size];
        bufferChannels = 2;
        bufferPosition = 0;
    }
    return bufAudio(samples, count * 2, size);
}

unsigned AudioStream::bufAudio(linear_t samples, unsigned count, unsigned size)
{
    unsigned fill = 0;
    unsigned frames = 0, copy, result;

    if(bufferPosition)
        fill = size - bufferPosition;
    else if(count < size)
        fill = count;

    if(fill > count)
        fill = count;

    if(fill) {
        memcpy(&bufferFrame[bufferPosition], samples, fill * 2);
        bufferPosition += fill;
        samples += fill;
        count -= fill;
    }

    if(bufferPosition == size) {
        if(bufferChannels == 1)
            frames = putMono(bufferFrame, 1);
        else
            frames = putStereo(bufferFrame, 1);
        bufferPosition = 0;
        if(!frames)
            return 0;
    }

    if(!count)
        return frames;

    if(count >= size) {
        copy = (count / size);
        if(bufferChannels == 1)
            result = putMono(samples, copy);
        else
            result = putStereo(samples, copy);

        if(result < copy)
            return frames + result;

        samples += copy * size;
        count -= copy * size;
        frames += result;
    }
    if(count) {
        memcpy(bufferFrame, samples, count * 2);
        bufferPosition = count;
    }
    return frames;
}

unsigned AudioStream::getEncoded(encoded_t addr, unsigned frames)
{
    unsigned count = 0, len;

    if(is_linear(info.encoding))
        return getMono((linear_t)addr, frames);

    while(frames--) {
        len = AudioFile::getBuffer(addr);   // packet read
        if(len < info.framesize)
            break;
        addr += info.framesize;
        ++count;
    }
    return count;
}

unsigned AudioStream::putEncoded(encoded_t addr, unsigned frames)
{
    unsigned count = 0;
    ssize_t len;

    if(is_linear(info.encoding))
        return putMono((linear_t)addr, frames);

    while(frames--) {
        len = putBuffer(addr);
        if(len < (ssize_t)info.framesize)
            break;
        addr += info.framesize;
        ++count;
    }
    return count;
}

unsigned AudioStream::getEncoded(AudioCodec *codec, encoded_t addr, unsigned frames)
{
    Info ci;
    unsigned count = 0;
    unsigned bufsize = 0;
    unsigned used = 0;
    bool eof = false;

    if(!codec)
        return getEncoded(addr, frames);

    ci = codec->getInfo();

    if(ci.encoding == info.encoding && ci.framecount == info.framecount)
        return getEncoded(addr, frames);

    if(!is_streamable())
        return 0;

    while(bufsize < ci.framesize)
        bufsize += info.framesize;

    if(encSize != bufsize) {
        if(encBuffer)
            delete[] encBuffer;

        encBuffer = new sample_t[bufsize];
        encSize = bufsize;
    }

    while(count < frames && !eof) {
        while(used < ci.framesize) {
            if(getMono(encBuffer + used, 1) < 1) {
                eof = true;
                break;
            }
            used += info.framesize;
        }
        codec->encode(encBuffer, addr, ci.framesize);
        if(ci.framesize < used)
            memcpy(encBuffer, encBuffer + ci.framesize, used - ci.framesize);
        used -= ci.framesize;
    }
    return count;
}

unsigned AudioStream::putEncoded(AudioCodec *codec, encoded_t addr, unsigned frames)
{
    Info ci;
    unsigned count = 0;

    if(!codec)
        return putEncoded(addr, frames);

    ci = codec->getInfo();
    if(ci.encoding == info.encoding && ci.framecount == info.framecount)
        return putEncoded(addr, frames);

    if(!is_streamable())
        return 0;

    if(ci.framecount != decSize) {
        if(decBuffer)
            delete[] decBuffer;
        decBuffer = new sample_t[ci.framecount];
        decSize = ci.framecount;
    }

    while(count < frames) {
        codec->decode(decBuffer, addr, ci.framecount);
        if(bufMono(decBuffer, ci.framecount) < ci.framecount)
            break;
        ++count;
        addr += ci.framesize;
    }

    return count;
}
