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

/**
 * Top level include directory for GNU Bayonne telephony server.
 * This is a master include file that will be used when producing
 * plugins for GNU Bayonne.  It includes all generic library headers
 * from both GNU Bayonne and uCommon.
 * @file bayonne/bayonne.h
 */

/**
 * @short A platform for telephony integration.
 * The Bayonne API library is meant to assist in the adapting of GNU Bayonne
 * architectures to various telephony API's by offering core services and
 * a class framework that all derived Bayonne adaptions will have in common.
 * This library is also meant to assist in development of generic plugins for
 * GNU Bayonne, which can be created separately from the core distribution.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @license GNU General Public License Version 3 or later.
 * @mainpage GNU Bayonne
 */

#ifndef _BAYONNE_BAYONNE_H_
#define _BAYONNE_BAYONNE_H_

#include <ucommon/ucommon.h>
#include <ccscript.h>

#include <bayonne/namespace.h>
#include <bayonne/server.h>
#include <bayonne/signals.h>
#include <bayonne/registry.h>
#include <bayonne/segment.h>
#include <bayonne/timeslot.h>
#include <bayonne/driver.h>
#include <bayonne/thread.h>
#include <bayonne/stats.h>
#include <bayonne/dbi.h>
#endif
