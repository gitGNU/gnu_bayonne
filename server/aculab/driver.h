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
#include <vpbapi.h>

#define TiNG_USE_RESOURCE_MANAGER

extern "C" {

#include <acu_type.h>
#include <res_lib.h>
#include <sw_lib.h>
#include <cl_lib.h>
#include <iptel_lib.h>

#include <smdrvr.h>
#include <smbesp.h>
#include <smhlib.h>
#include <smclib.h>
#include <smwavlib.h>
#include <smbfhlib.h>
#include <smbfwavlib.h>
#include <smdc.h>

#ifdef HAVE_ACULAB_ASR
#include "smcwr_api.h"
#endif

};

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

END_NAMESPACE

