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

#include "driver.h"

using namespace UCOMMON_NAMESPACE;
using namespace BAYONNE_NAMESPACE;

class driver : public Driver
{
public:
    driver();
};

driver::driver() : Driver("h323", "registry")
{
}

static SERVICE_MAIN(main, argc, argv)
{
    signals::service("bayonne");
    server::start(argc, argv);
}

PROGRAM_MAIN(argc, argv)
{
    new driver;
    server::start(argc, argv, &service_main);
    PROGRAM_EXIT(0);
}


