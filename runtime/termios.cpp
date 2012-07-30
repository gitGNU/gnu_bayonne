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

#if !defined(_MSWINDOWS_) && defined(HAVE_TERMIOS_H)
#include <termios.h>
#include <sys/select.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#ifdef  HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#else
#include <ioctl.h>
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

class __LOCAL serial : public Serial
{
private:
    fd_t fd;
    struct termios original, current;

    void restore(void);
    bool set(const char *format);
    void dtr(timeout_t timeout);
    size_t get(void *data, size_t len);
    size_t put(void *data, size_t len);
    void bin(size_t size, timeout_t timeout);
    void text(char nl1, char nl2);
    void clear(void);
    bool flush(timeout_t timeout);
    bool wait(timeout_t timeout);
    void sync(void);

    operator bool();
    bool operator!();

public:
    serial(const char *name);
    ~serial();
};

serial::serial(const char *name) : Serial()
{
    fd = ::open(name, O_RDWR | O_NDELAY);
    if(fd < 0) {
        error = errno;
        return;
    }

    long ioflags = fcntl(fd, F_GETFL);
    tcgetattr(fd, &current);
    tcgetattr(fd, &original);

    current.c_oflag = current.c_lflag = 0;
    current.c_cflag = CLOCAL | CREAD | HUPCL;
    current.c_iflag = IGNBRK;

    memset(&current.c_cc, 0, sizeof(current.c_cc));
    current.c_cc[VMIN] = 1;

    cfsetispeed(&current, cfgetispeed(&original));
    cfsetospeed(&current, cfgetospeed(&original));

    current.c_cflag |= original.c_cflag & (CRTSCTS | CSIZE | PARENB | PARODD | CSTOPB);
    current.c_iflag |= original.c_iflag & (IXON | IXANY | IXOFF);

    tcsetattr(fd, TCSANOW, &current);
    fcntl(fd, F_SETFL, ioflags & ~O_NDELAY);

#if defined(TIOCM_RTS) && defined(TIOCMODG)
    int mcs = 0;
    ioctl(fd, TIOCMODG, &mcs);
    mcs |= TIOCM_RTS;
    ioctl(fd, TIOCMODS, &mcs);
#endif
}

serial::~serial()
{
    if(fd < 0)
        return;

    restore();
    ::close(fd);
    fd = -1;
}

serial::operator bool()
{
    if(fd > -1)
        return true;

    return false;
}

bool serial::operator!()
{
    if(fd < 0)
        return true;

    return false;
}

void serial::restore(void)
{
    if(fd < 0)
        return;

    memcpy(&current, &original, sizeof(current));
    tcsetattr(fd, TCSANOW, &current);
}

bool serial::set(const char *format)
{
    assert(format != NULL);

    if(fd < 0)
        return false;

    unsigned long opt;
    char buf[256];
    String::set(buf, sizeof(buf), format);

    char *cp = strtok(buf, ",");
    while(cp) {
        switch(*cp) {
        case 'n':
        case 'N':
            current.c_cflag &= ~(PARENB | PARODD);
            break;
        case 'e':
        case 'E':
            current.c_cflag = (current.c_cflag & ~PARODD) | PARENB;
            break;
        case 'o':
        case 'O':
            current.c_cflag |= (PARODD | PARENB);
            break;
        case 's':
        case 'S':
            current.c_cflag &= ~CRTSCTS;
            current.c_cflag |= (IXON | IXANY | IXOFF);
            break;
        case 'h':
        case 'H':
            current.c_cflag |= CRTSCTS;
            current.c_cflag &= ~(IXON | IXANY | IXOFF);
            break;
        case 'b':
        case 'B':
            current.c_cflag |= CRTSCTS;
            current.c_cflag |= (IXON | IXANY | IXOFF);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            opt = atol(cp);
            switch(opt) {
            case 1:
                current.c_cflag &= ~CSTOPB;
                break;
            case 2:
                current.c_cflag |= CSTOPB;
                break;
            case 5:
                current.c_cflag = (current.c_cflag & ~CSIZE) | CS5;
                break;
            case 6:
                current.c_cflag = (current.c_cflag & ~CSIZE) | CS6;
                break;
            case 7:
                current.c_cflag = (current.c_cflag & ~CSIZE) | CS7;
                break;
            case 8:
                current.c_cflag = (current.c_cflag & ~CSIZE) | CS8;
                break;
#ifdef  B921600
            case 921600l:
                cfsetispeed(&current, B921600);
                cfsetospeed(&current, B921600);
                break;
#endif
#ifdef  B576000
            case 576000l:
                cfsetispeed(&current, B576000);
                cfsetospeed(&current, B576000);
                break;
#endif
#ifdef  B500000
            case 500000l:
                cfsetispeed(&current, B500000);
                cfsetospeed(&current, B500000);
                break;
#endif
#ifdef  B460800
            case 460800l:
                cfsetispeed(&current, B460800);
                cfsetospeed(&current, B460800);
                break;
#endif
#ifdef  B230400
            case 230400l:
                cfsetispeed(&current, B230400);
                cfsetospeed(&current, B230400);
                break;
#endif
#ifdef  B115200
            case 115200l:
                cfsetispeed(&current, B115200);
                cfsetospeed(&current, B115200);
                break;
#endif
#ifdef  B57600
            case 57600l:
                cfsetispeed(&current, B57600);
                cfsetospeed(&current, B57600);
                break;
#endif
#ifdef  B38400
            case 38400l:
                cfsetispeed(&current, B38400);
                cfsetospeed(&current, B38400);
                break;
#endif
            case 19200l:
                cfsetispeed(&current, B19200);
                cfsetospeed(&current, B19200);
                break;
            case 9600l:
                cfsetispeed(&current, B9600);
                cfsetospeed(&current, B9600);
                break;
            case 4800l:
                cfsetispeed(&current, B4800);
                cfsetospeed(&current, B4800);
                break;
            case 2400l:
                cfsetispeed(&current, B2400);
                cfsetospeed(&current, B2400);
                break;
            case 1200l:
                cfsetispeed(&current, B1200);
                cfsetospeed(&current, B1200);
                break;
            case 600l:
                cfsetispeed(&current, B600);
                cfsetospeed(&current, B600);
                break;
            case 300l:
                cfsetispeed(&current, B300);
                cfsetospeed(&current, B300);
                break;
            case 110l:
                cfsetispeed(&current, B110);
                cfsetospeed(&current, B110);
                break;
#ifdef  B200
            case 200l:
                cfsetispeed(&current, B200);
                cfsetospeed(&current, B200);
                break;
#endif
#ifdef  B150
            case 150l:
                cfsetispeed(&current, B150);
                cfsetospeed(&current, B150);
                break;
#endif
#ifdef  B134
            case 134l:
                cfsetispeed(&current, B134);
                cfsetospeed(&current, B134);
                break;
#endif
#ifdef  B0
            case 0:
                cfsetispeed(&current, B0);
                cfsetospeed(&current, B0);
                break;
#endif
            default:
                error = EINVAL;
                return false;
            }
            break;
        default:
            error = EINVAL;
            return false;
        }
        cp = strtok(NULL, ",");
    }
    tcsetattr(fd, TCSANOW, &current);
    return true;
}

void serial::dtr(timeout_t timeout)
{
    if(fd < 0)
        return;

    struct termios tty, old;
    tcgetattr(fd, &tty);
    tcgetattr(fd, &old);
    cfsetospeed(&tty, B0);
    cfsetispeed(&tty, B0);
    tcsetattr(fd, TCSANOW, &tty);

    if(timeout) {
        Thread::sleep(timeout);
        tcsetattr(fd, TCSANOW, &old);
    }
}

size_t serial::get(void *data, size_t len)
{
    if(fd < 0)
        return 0;

    ssize_t rts = ::read(fd, data, len);
    if(rts < 0) {
        error = errno;
        return 0;
    }
    return rts;
}

size_t serial::put(void *data, size_t len)
{
    if(fd < 0)
        return 0;

    ssize_t rts = ::write(fd, data, len);
    if(rts < 0) {
        error = errno;
        return 0;
    }
    return rts;
}

void serial::bin(size_t size, timeout_t timeout)
{
    if(fd < 0)
        return;

#ifdef  _PC_MAX_INPUT
    int max = fpathconf(fd, _PC_MAX_INPUT);
#else
    int max = MAX_INPUT;
#endif
    if(size > (size_t)max)
        size = max;

    current.c_cc[VEOL] = current.c_cc[VEOL2] = 0;
    current.c_cc[VMIN] = (unsigned char)size;
    current.c_cc[VTIME] = (unsigned char)( (timeout + 99l) / 100l);
    current.c_lflag &= ~ICANON;
    tcsetattr(fd, TCSANOW, &current);
}

void serial::text(char nl1, char nl2)
{
    current.c_cc[VMIN] = current.c_cc[VTIME] = 0;
    current.c_cc[VEOL] = nl1;
    current.c_cc[VEOL2] = nl2;
    current.c_lflag |= ICANON;
    tcsetattr(fd, TCSANOW, &current);
}

void serial::clear(void)
{
    tcflush(fd, TCIFLUSH);
}

void serial::sync(void)
{
    tcdrain(fd);
}

bool serial::wait(timeout_t timeout)
{
    struct timeval tv;
    fd_set grp;
    struct timeval *tvp = &tv;
    int status;

    if(fd < 0)
        return false;

    if(timeout == Timer::inf)
        tvp = NULL;
    else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
    }

    FD_ZERO(&grp);
    FD_SET(fd, &grp);
    status = select(fd + 1, &grp, NULL, NULL, tvp);
    if(status < 0)
        error = errno;

    if(status < 1)
        return false;

    if(FD_ISSET(fd, &grp))
        return true;

    return false;
}

bool serial::flush(timeout_t timeout)
{
    struct timeval tv;
    fd_set grp;
    struct timeval *tvp = &tv;
    int status;
    bool rtn = false;

    if(fd < 0)
        return false;

    if(timeout == Timer::inf)
        tvp = NULL;
    else {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
    }

    FD_ZERO(&grp);
    FD_SET(fd, &grp);
    status = select(fd + 1, NULL, &grp, NULL, tvp);
    if(status < 0) {
        error = errno;
        return false;
    }

    if(status > 0 && FD_ISSET(fd, &grp))
        rtn = true;

    if(rtn)
        tcflush(fd, TCOFLUSH);

    return rtn;
}

Serial *Serial::create(const char *name)
{
    assert(name != NULL);

    char buf[64];
    fsys::fileinfo_t ino;

    if(*name == '/') {
        String::set(buf, sizeof(buf), name);
        goto check;
    }

    if(eq(name, "tty", 3))
        snprintf(buf, sizeof(buf), "/dev/%s", name);
    else
        snprintf(buf, sizeof(buf), "/dev/tty%s", name);

check:
    char *cp = strchr(buf, ':');
    if(cp)
        *cp = 0;
    fsys::fileinfo(buf, &ino);
    if(!fsys::ischar(&ino))
        return NULL;
    serial_t dev = new serial(buf);
    name = strchr(name, ':');
    if(dev && name)
        dev->set(++name);
    return dev;
}

stringlist_t *Serial::list(void)
{
    stringlist_t *list = new stringlist_t;
    char filename[64];
    fsys_t dir;

    fsys::open(dir, "/dev", fsys::ACCESS_DIRECTORY);
    while(is(dir) && fsys::read(dir, filename, sizeof(filename)) > 0) {
        if(eq(filename, "tty", 3))
            list->add(filename + 3);
    }
    fsys::close(dir);
    return list;
}

#endif


