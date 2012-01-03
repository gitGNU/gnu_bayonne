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

#ifdef  HAVE_LINUX_TELEPHONY_H
#include <linux/telephony.h>
#ifdef  HAVE_LINUX_IXJUSER_H
#include <linux/ixjuser.h>
#endif
#endif

#ifdef HAVE_SYS_TELEPHONY_H
#include <sys/telephony.h>
#ifdef HAVE_SYS_IXJUSER_H
#include <sys/ixjuser.h>
#endif
#endif

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

END_NAMESPACE

