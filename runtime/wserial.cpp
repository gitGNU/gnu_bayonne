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

#ifdef  _MSWINDOWS_
#include <io.h>

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

class __LOCAL serial : public Serial
{
private:
    HANDLE fd;
    DCB original, current;

    void restore(void);
    bool set(const char *format);
    size_t get(void *addr, size_t len);
    size_t put(void *addr, size_t len);
    void dtr(timeout_t toggle_time);
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
    fd = CreateFile(name,
        GENERIC_READ | GENERIC_WRITE,
        0,                    // exclusive access
        NULL,                 // no security attrs
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
        NULL);

    if(fd == INVALID_HANDLE_VALUE) {
        return;
    }

    current.DCBlength = sizeof(DCB);
    original.DCBlength = sizeof(DCB);

    GetCommState(fd, &original);
    GetCommState(fd, &current);

    current.DCBlength = sizeof(DCB);
    current.BaudRate = 1200;
    current.Parity = NOPARITY;
    current.ByteSize = 8;

    current.XonChar = ASCII_XON;
    current.XoffChar = ASCII_XOFF;
    current.XonLim = 100;
    current.XoffLim = 100;
    current.fOutxDsrFlow = 0;
    current.fDtrControl = DTR_CONTROL_ENABLE;
    current.fOutxCtsFlow = 1;
    current.fRtsControl = RTS_CONTROL_ENABLE;
    current.fInX = current.fOutX = 0;

    current.fBinary = true;
    current.fParity = true;

    SetCommState(fd, &current);
}

serial::~serial()
{
    if(fd == INVALID_HANDLE_VALUE)
        return;

    restore();
    CloseHandle(fd);
    fd = INVALID_HANDLE_VALUE;
}

serial::operator bool()
{
    if(fd == INVALID_HANDLE_VALUE)
        return false;

    return true;
}

bool serial::operator!()
{
    if(fd == INVALID_HANDLE_VALUE)
        return true;

    return false;
}

void serial::restore(void)
{
    if(fd == INVALID_HANDLE_VALUE)
        return;

    memcpy(&current, &original, sizeof(current));
    SetCommState(fd, &current);
}

bool serial::set(const char *format)
{
    assert(format != NULL);

    if(fd == INVALID_HANDLE_VALUE)
        return false;

    unsigned long opt;
    char buf[256];
    String::set(buf, sizeof(buf), format);

    char *cp = strtok(buf, ",");
    while(cp) {
        switch(*cp) {
        case 'n':
        case 'N':
            current.Parity = NOPARITY;
            break;
        case 'e':
        case 'E':
            current.Parity = EVENPARITY;
            break;
        case 'o':
        case 'O':
            current.Parity = ODDPARITY;
            break;
        case 's':
        case 'S':
            current.fInX = current.fOutX = TRUE;
            current.fOutxCtsFlow = FALSE;
            current.fRtsControl = FALSE;
            break;
        case 'H':
        case 'h':
            current.fInX = current.fOutX = FALSE;
            current.fOutxCtsFlow = TRUE;
            current.fRtsControl = RTS_CONTROL_HANDSHAKE;
            break;
        case 'b':
        case 'B':
            current.fInX = current.fOutX = TRUE;
            current.fOutxCtsFlow = TRUE;
            current.fRtsControl = RTS_CONTROL_HANDSHAKE;
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
                current.StopBits = ONESTOPBIT;
                break;
            case 2:
                current.StopBits = TWOSTOPBITS;
                break;
            case 5:
            case 6:
            case 7:
            case 8:
                current.ByteSize = (unsigned char)opt;
                break;
            case 110:
            case 300:
            case 600:
            case 1200:
            case 2400:
            case 4800:
            case 9600:
            case 14400:
            case 19200:
            case 38400:
            case 57600:
            case 115200:
            case 128000:
            case 256000:
                current.BaudRate = opt;
                break;
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
    current.DCBlength = sizeof(DCB);
    SetCommState(fd, &current);
    return true;
}

size_t serial::get(void *data, size_t len)
{
    DWORD   dwLength = 0, dwError, dwReadLength;
    COMSTAT cs;
    OVERLAPPED ol;

    // Return zero if handle is invalid
    if(fd == INVALID_HANDLE_VALUE)
        return 0;

    // Read max length or only what is available
    ClearCommError(fd, &dwError, &cs);

    // If not requiring an exact byte count, get whatever is available
    if(len > (size_t)cs.cbInQue)
        dwReadLength = cs.cbInQue;
    else
        dwReadLength = len;

    memset(&ol, 0, sizeof(OVERLAPPED));
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(dwReadLength > 0) {
        if(ReadFile(fd, data, dwReadLength, &dwLength, &ol) == FALSE) {
            if(GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(ol.hEvent, INFINITE);
                GetOverlappedResult(fd, &ol, &dwLength, TRUE);
            }
            else
                ClearCommError(fd, &dwError, &cs);
        }
    }

    if(ol.hEvent != INVALID_HANDLE_VALUE)
        CloseHandle(ol.hEvent);

    return dwLength;
}

size_t serial::put(void *data, size_t len)
{
    COMSTAT cs;
    unsigned long dwError = 0;
    OVERLAPPED ol;

    // Clear the com port of any error condition prior to read
    ClearCommError(fd, &dwError, &cs);

    unsigned long retSize = 0;

    memset(&ol, 0, sizeof(OVERLAPPED));
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(WriteFile(fd, data, len, &retSize, &ol) == FALSE) {
        if(GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(ol.hEvent, INFINITE);
            GetOverlappedResult(fd, &ol, &retSize, TRUE);
        }
        else
            ClearCommError(fd, &dwError, &cs);
    }

    if(ol.hEvent != INVALID_HANDLE_VALUE)
        CloseHandle(ol.hEvent);

    return retSize;
}

bool serial::wait(timeout_t timeout)
{
    unsigned long   dwError;
    COMSTAT cs;

    if(fd == INVALID_HANDLE_VALUE)
        return false;

    ClearCommError(fd, &dwError, &cs);

    if(timeout == 0)
        return (0 != cs.cbInQue);

    OVERLAPPED ol;
    DWORD dwMask;
    DWORD dwEvents = 0;
    BOOL suc;

    memset(&ol, 0, sizeof(OVERLAPPED));
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    dwMask = EV_RXCHAR;
    SetCommMask(fd, dwMask);
    // let's wait for event or timeout
    if((suc = WaitCommEvent(fd, &dwEvents, &ol)) == FALSE) {
        if(GetLastError() == ERROR_IO_PENDING) {
            DWORD transferred;

            dwError = WaitForSingleObject(ol.hEvent, timeout);
            if (dwError != WAIT_OBJECT_0)
                SetCommMask(fd, 0);

            suc = GetOverlappedResult(fd, &ol, &transferred, TRUE);
            if (suc)
                suc = (dwEvents & dwMask) ? TRUE : FALSE;
            }
        else
            ClearCommError(fd, &dwError, &cs);
    }

    if(ol.hEvent != INVALID_HANDLE_VALUE)
        CloseHandle(ol.hEvent);

    if(suc == FALSE)
        return false;
    return true;
}

bool serial::flush(timeout_t timeout)
{
    unsigned long   dwError;
    COMSTAT cs;

    if(fd == INVALID_HANDLE_VALUE)
        return false;

    ClearCommError(fd, &dwError, &cs);

    if(timeout == 0 && cs.cbOutQue > 0) {
        PurgeComm(fd, PURGE_TXABORT | PURGE_TXCLEAR);
        return true;
    }

    if(cs.cbOutQue == 0)
        return false;

    OVERLAPPED ol;
    DWORD dwMask;
    DWORD dwEvents = 0;
    BOOL suc;

    memset(&ol, 0, sizeof(OVERLAPPED));
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    dwMask = EV_TXEMPTY;
    SetCommMask(fd, dwMask);
    // let's wait for event or timeout
    if((suc = WaitCommEvent(fd, &dwEvents, &ol)) == FALSE) {
        if(GetLastError() == ERROR_IO_PENDING) {
            DWORD transferred;

            dwError = WaitForSingleObject(ol.hEvent, timeout);
            if (dwError != WAIT_OBJECT_0)
                SetCommMask(fd, 0);

            suc = GetOverlappedResult(fd, &ol, &transferred, TRUE);
            if (suc)
                suc = (dwEvents & dwMask) ? TRUE : FALSE;
            }
        else
            ClearCommError(fd, &dwError, &cs);
    }

    if(ol.hEvent != INVALID_HANDLE_VALUE)
        CloseHandle(ol.hEvent);

    if(suc == TRUE)
        return false;

    PurgeComm(fd, PURGE_TXABORT | PURGE_TXCLEAR);
    return true;
}

void serial::clear(void)
{
    PurgeComm(fd, PURGE_RXABORT | PURGE_RXCLEAR);
}

void serial::sync(void)
{
}

void serial::bin(size_t size, timeout_t timeout)
{
}

void serial::text(char nl1, char nl2)
{
}

void serial::dtr(timeout_t timeout)
{
    EscapeCommFunction(fd, CLRDTR);
    if(timeout) {
        Thread::sleep(timeout);
        EscapeCommFunction(fd, SETDTR);
    }
}

Serial *Serial::create(const char *name)
{
    char buf[65];

    String::set(buf, sizeof(buf), name);

    char *cp = strchr(buf, ':');
    if(cp)
        *cp = 0;

    serial_t dev = new serial(buf);
    name = strchr(name, ':');
    if(dev && name)
        dev->set(++name);

    return dev;
}

stringlist_t *Serial::list(void)
{
    DWORD index = 0;
    TCHAR keyname[4096];
    DWORD size = sizeof(keyname);
    stringlist_t *list = new stringlist_t;
    HKEY reg;
    FILETIME fTime;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &reg) == ERROR_SUCCESS) {
        while(RegEnumKeyEx(reg, index++, keyname, &size, NULL, NULL, NULL, &fTime) == ERROR_SUCCESS) {
            list->add(keyname);
        }
        RegCloseKey(reg);
    }
    return list;
}

#endif

