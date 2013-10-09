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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

driver::driver() :
Driver("vpbapi")
{
}

Driver *driver::create(void)
{
    return new driver();
}

void driver::update(void)
{
    keydata *keys = keyfile::get("vpbapi");

    linked_pointer<keydata::keyvalue> kv;
    if(keys)
        kv = keys->begin();

    while(is(kv)) {
        kv.next();
    }
    Driver::update();
}

void driver::start(void)
{
    int major, minor, patch;
    Driver *drv = Driver::get();
    keydata *keys = drv->keyfile::get("vpbapi");
    linked_pointer<keydata::keyvalue> kv;

    board_count = vpb_get_num_cards();
    board_alloc = sizeof(board);

    vpb_get_driver_version(&major, &minor, &patch);
    shell::log(shell::NOTIFY, "vpbapi: detected %d card(s), driver=%d.%d.%d",
        board_count, major, minor, patch);

    if(!board_count)
        return;

    boards = (caddr_t)new board[board_count];
    // a stat node for each board and each span, allocates stats to board...
    stats = statmap::create(board_count + board::allspans());

    // ts_count = board::timeslots();


    // span_count = Board::allspans();
    // span_alloc = sizeof(span);
    // if(span_count)
    //  spans = (caddr_t)new spans[span_count];

    // if failed to find cards, we return...
    if(!ts_count)
        return;

    if(keys)
        kv = keys->begin();

    while(is(kv)) {
        kv.next();
    }

    Driver::start();
    Driver::release(drv);
}

void driver::stop(void)
{
    Driver::stop();
}

const char *driver::dispatch(char **argv, int pid)
{
    return Driver::dispatch(argv, pid);
}

extern "C" int main(int argc, char **argv)
{
    static Script::keyword_t keywords[] = {
        {NULL}};

    server::parse(argc, argv, "voicetronix");
    server::startup();

    Script::assign(keywords);   // bind local driver keywords if any...

    Driver::commit(new driver());

    driver::start();
    server::dispatch();
    driver::stop();

    server::release();
}
