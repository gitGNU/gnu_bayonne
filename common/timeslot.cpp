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

#include <bayonne-config.h>
#include <ucommon/ucommon.h>
#include <ccscript.h>
#include <ucommon/export.h>
#include <bayonne/bayonne.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

#define TIMESLOT_INDEX_SIZE 177

static class __LOCAL map : public mapped_array<Timeslot::mapped_t>
{
public:
    map();
    ~map();

    void init(void);
} shm;

static unsigned counting = 0;
static LinkedObject *timeslots;
static LinkedObject *assigned[TIMESLOT_INDEX_SIZE];
static condlock_t private_locking;
static bool initial = false;

map::map() : mapped_array<Timeslot::mapped_t>()
{
    memset(assigned, 0, sizeof(assigned));
}

map::~map()
{
    if(initial) {
        release();
        remove(TIMESLOT_MAP);
        initial = false;
    }
}

void map::init(void)
{
    initial = true;
    remove(TIMESLOT_MAP);
    create(TIMESLOT_MAP, Driver::getCount());
    initialize();
}

Timeslot::Timeslot() : LinkedObject(&timeslots), Script::interp()
{
    registry = NULL;
    board = NULL;
    span = NULL;
    instance = counting++;
    sequence = rings = 0;
    expires = Timer::inf;
    handler = &Timeslot::idleHandler;
    tracing = traceflag = false;
    connected = answered = false;
    digits = voice = NULL;

    if(!instance)
        shm.init();

    mapped = shm(instance);
    if(!mapped)
        mapped = (mapped_t*)memget(sizeof(mapped_t));


    setMapped('-', "idle");
    mapped->started = 0;
    mapped->type = mapped_t::NONE;
    String::set(mapped->source, sizeof(mapped->source), "-");
    String::set(mapped->target, sizeof(mapped->target), "-");
    String::set(mapped->script, sizeof(mapped->script), "-");
    rings = 0;
}

void Timeslot::initialize(void)
{
    digits = voice = NULL;
    Script::symbol *sym;
    Driver *driver = Driver::get();
    keydata *keys = driver->keyfile::get("defaults");
    const char *default_voice = NULL;

    if(keys)
        default_voice = keys->get("voice");

    if(!default_voice)
        default_voice = "english/female";

    interp::initialize();
    sym = createSymbol("digits:64");
    if(sym)
        digits = sym->data;
    sym = createSymbol("voice:64");
    if(sym) {
        voice = sym->data;
        String::set(voice, 64, default_voice);
    }
    Driver::release(driver);
}

Timeslot *Timeslot::get(long cid)
{
    linked_pointer<Timeslot> tp;
    Timeslot *ts = NULL;
    unsigned path = (cid % TIMESLOT_INDEX_SIZE);

    private_locking.access();
    tp = assigned[path];

    while(is(tp)) {
        if(tp->cid == cid) {
            if(tp->mapped->started)
                ts = *tp;
            break;
        }
        tp.next();
    }

    private_locking.release();
    return ts;
}

void Timeslot::allocate(long new_cid, statmap::stat_t stat, Registration *reg)
{
    unsigned path = (new_cid % TIMESLOT_INDEX_SIZE);

    time(&mapped->started);
    if(timeslots == this)
        timeslots = Next;
    else
        delist(&timeslots);
    enlist(&assigned[path]);
    cid = new_cid;
    optional = NULL;
    reason = NULL;
    stats = stat;
    registry = reg;
    if(handler == &Timeslot::idleHandler)
        handler = &Timeslot::releaseHandler;
    ++sequence;
    Driver::assign(stats);
    if(span)
        span->assign(stats);
    if(board)
        board->assign(stats);

    dbi *call = dbi::get();
    call->type = dbi::START;
    call->timeslot = instance;
    call->sequence = sequence;
    call->starting = mapped->started;
    call->duration = 0;

    String::set(call->reason, sizeof(call->reason), "start");
    String::set(call->source, sizeof(call->source), mapped->source);
    String::set(call->target, sizeof(call->target), mapped->target);
    String::set(call->script, sizeof(call->script), mapped->script);
    dbi::post(call);
}

Timeslot *Timeslot::assign(Registration *reg, statmap::stat_t stat, long cid)
{
    Timeslot *ts = NULL;

    private_locking.modify();
    ts = (Timeslot *)timeslots;
    if(ts)
        ts->allocate(cid, stat, reg);

    private_locking.commit();
    return ts;
}

Timeslot *Timeslot::assign(statmap::stat_t stat, unsigned starting, unsigned ending)
{
    Timeslot *ts = NULL;
    unsigned path;

    private_locking.commit();
    for(path = starting; path <= ending; ++path) {
        ts = Driver::get(path);
        if(ts && ts->mapped->started == 0)
            break;
        ts = NULL;
    }
    if(ts)
        ts->allocate((long)path, stat);

    private_locking.commit();
    return ts;
}

Timeslot *Timeslot::assign(statmap::stat_t stat, unsigned starting, unsigned ending, long cid)
{
    Timeslot *ts = NULL;

    private_locking.commit();
    for(unsigned pos = starting; pos <= ending; ++pos) {
        ts = Driver::get(pos);
        if(ts && ts->mapped->started == 0)
            break;
        ts = NULL;
    }
    if(ts)
        ts->allocate(cid, stat);

    private_locking.commit();
    return ts;
}

void Timeslot::release(event_t *event)
{
    unsigned path = (cid % TIMESLOT_INDEX_SIZE);
    time_t ending;
    dbi *call = dbi::get();

    if(!reason)
        reason = "release";

    time(&ending);

    call->type = dbi::STOP;
    call->optional = optional;
    call->timeslot = instance;
    call->sequence = sequence;
    call->starting = mapped->started;
    call->duration = (long)(ending - mapped->started);

    String::set(call->reason, sizeof(call->reason), reason);
    String::set(call->source, sizeof(call->source), mapped->source);
    String::set(call->target, sizeof(call->target), mapped->target);
    String::set(call->script, sizeof(call->script), mapped->script);
    dbi::post(call);

    digits = voice = NULL;
    event->id = Timeslot::RELEASE;
    detach();
    setIdle();
    Driver::release(stats);
    if(registry)
        registry->release(stats);
    if(span)
        span->release(stats);
    if(board)
        board->release(stats);
    registry = NULL;
    mutex.release();
    private_locking.modify();
    delist(&assigned[path]);
    enlist(&timeslots);
    private_locking.commit();
}

timeout_t Timeslot::getExpires(time_t now)
{
    return Timer::inf;
}

void Timeslot::expire(void)
{
    event_t event = {Timeslot::TIMEOUT};

    if(expires == 0)
        (this->*handler)(&event);

    expires = Timer::inf;
    if(event.id != Timeslot::RELEASE)
        mutex.unlock();
}

void Timeslot::scriptHandler(event_t *event)
{
    switch(event->id) {
    case Timeslot::DISABLE:
        disable(event);
        break;
    case Timeslot::RELEASE:
        release(event);
        break;
    case Timeslot::DROP:
        disconnect(event);
        break;
    case Timeslot::HANGUP:
        hangup(event);
        break;
    case Timeslot::TIMEOUT:
        if(!interp::step())
            hangup(event);
        else if(handler == &Timeslot::scriptHandler)
            arm(Driver::getStepping());
        break;
    default:
        running(event);
        break;
    }
}

void Timeslot::offlineHandler(event_t *event)
{
    switch(event->id) {
    case Timeslot::SHUTDOWN:
        shutdown(event);
        return;
    case Timeslot::ENABLE:
        enable(event);
        return;
    default:
        event->id = Timeslot::REJECT;
        return;
    }
}

void Timeslot::idleHandler(event_t *event)
{
    switch(event->id) {
    case Timeslot::DISABLE:
        disable(event);
        return;
    case Timeslot::SHUTDOWN:
        shutdown(event);
        return;
    default:
        event->id = Timeslot::REJECT;
        return;
    }
}

void Timeslot::releaseHandler(event_t *event)
{
    switch(event->id) {
    case Timeslot::DISABLE:
        disable(event);
        return;
    case Timeslot::SHUTDOWN:
        shutdown(event);
        return;
    case Timeslot::TIMEOUT:
    case Timeslot::RELEASE:
        release(event);
        return;
    default:
        event->id = Timeslot::REJECT;
        return;
    }
}

void Timeslot::shutdown(event_t *event)
{
    setIdle();
}

void Timeslot::disconnect(event_t *event)
{
    connected = false;
    setMapped('-', "release");
    handler = &Timeslot::releaseHandler;
    return;
}

void Timeslot::hangup(event_t *event)
{
    Timeslot::disconnect(event);
}

void Timeslot::enable(event_t *event)
{
    time_t now;
    time(&now);

    server::printlog("timeslot %d online after %ld seconds",
        instance, (long)(now - mapped->started));

    setIdle();
    mutex.release();
    private_locking.modify();
    enlist(&timeslots);
    private_locking.commit();
    event->id = Timeslot::RELEASE;
}

void Timeslot::running(event_t *event)
{
    event->id = Timeslot::REJECT;
}

void Timeslot::disable(event_t *event)
{
    server::printlog("timeslot %u offline", instance);

    if(mapped->started) {
        hangup(event);
        release(event);
    }

    mapped->type = mapped_t::NONE;
    time(&mapped->started);
    disarm();
	interp::purge();
    setMapped('.', "offline");
    handler = &Timeslot::offlineHandler;
    connected = answered = false;
    event->id = Timeslot::RELEASE;

    mutex.unlock();
    private_locking.modify();
    delist(&timeslots);
    private_locking.commit();
}

void Timeslot::setScripting(void)
{
    setMapped('$', "script");
    handler = &Timeslot::scriptHandler;
    arm(Driver::getStepping());
}

void Timeslot::setIdle(void)
{
    setMapped('-', "idle");
    connected = answered = false;
    tracing = traceflag;
    handler = &Timeslot::idleHandler;
    mapped->started = 0;
    mapped->type = mapped_t::NONE;
    String::set(mapped->source, sizeof(mapped->source), "-");
    String::set(mapped->target, sizeof(mapped->target), "-");
    String::set(mapped->script, sizeof(mapped->script), "-");
    rings = 0;
    disarm();
    interp::purge();
}

void Timeslot::setMapped(const char id, const char *state)
{
    assert(!state || *state != 0);
    assert(id != 0);

    mapped->state[0] = id;
    if(state)
        String::set(mapped->state + 1, sizeof(mapped->state) - 1, state);
}

void Timeslot::post(event_t *event)
{
    assert(event != NULL);

    shell::debug(9, "timeslot %d event %d", instance, event->id);

    mutex.lock();
    (this->*handler)(event);
    if(event->id != Timeslot::RELEASE)
        mutex.unlock();
}
