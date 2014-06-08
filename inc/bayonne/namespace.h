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
 * GNU Bayonne library namespace.  This gives the server it's own
 * private namespace for drivers, plugins, etc.
 * @file bayonne/namespace.h
 */

#ifndef	_BAYONNE_NAMESPACE_H_
#define	_BAYONNE_NAMESPACE_H_

#define BAYONNE_NAMESPACE   bayonne
#define NAMESPACE_BAYONNE   namespace bayonne {

namespace bayonne {
using namespace ucommon;
}

/**
 * Common namespace for bayonne server.
 * We use a bayonne specific namespace to easily seperate bayonne
 * interfaces from other parts of GNU Telephony.  This namespace
 * is controlled by the namespace macros (BAYONNE_NAMESPACE and 
 * NAMESPACE_BAYONNE) and are used in place of direct namespace
 * declarations to make parsing of tab levels simpler and to allow easy
 * changes to the namespace name later if needed.
 * @namespace bayonne
 */

#endif

