// Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <common/server.h>

#define P_HAS_SEMAPHORES
#define P_USE_PRAGMA
#define P_SSL 1
#define P_PTHREADS 1
#define PHAS_TEMPLATES
#define PTRACING 1
#define HAS_OSS

#if __BYTE_ORDER == __LITTLE_ENDIAN
  #define PBYTE_ORDER PLITTLE_ENDIAN
#else
  #define PBYTE_ORDER PBIG_ENDIAN
#endif

//typedef unsigned int PUInt32b;
//typedef unsigned short PUInt16b;

#define P_USE_PRAGRMA
#define PHAS_TEMPLATES
#define P_PLATFORM_HAS_THREADS
#define P_PTHREADS 1
#define P_HAS_SEMAPHORES
#define NDEBUG

#include <ptlib.h>
#include <openh323/h323.h>
#include <openh323/h323pdu.h>

#include <ptclib/pwavfile.h>
#include <ptclib/delaychan.h>

#define NDEBUG

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

END_NAMESPACE

