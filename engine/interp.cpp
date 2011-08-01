// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>
#include <ctype.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#ifdef __MINGW32__
#define NUM_FORMAT "%lld"
#else
#define NUM_FORMAT "%ld"
#endif

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

script::interp::interp() :
memalloc(script::paging)
{
}

script::interp::~interp()
{
    detach();
}

void script::interp::initialize(void)
{
    unsigned tempcount = sizeof(temps) / sizeof(char *);
    unsigned pos = 0;;

    frame = 0;
    tempindex = 0;

    purge();

    image = NULL;
    stack = (script::stack_t *)alloc(sizeof(script::stack_t) * script::stacking);
    syms = (LinkedObject **)alloc(sizeof(LinkedObject *) * script::indexing);
    memset(syms, 0, sizeof(script::symbol *) * script::indexing);
    while(pos < tempcount)
        temps[pos++] = (char *)alloc(script::sizing + 1);

    errmsg = NULL;
    script::symbol *err = createSymbol("error:64");
    if(err)
        errmsg = err->data;
}

bool script::interp::error(const char *msg)
{
#ifndef _MSWINDOWS_
    if(getppid() > 1)
        fprintf(stderr, "*** %s(%d): %s\n", image->filename, stack[frame].line->lnum, msg);
#endif

    if(errmsg)
        String::set(errmsg, 65, msg);

    if(!eq(stack[frame].scr->name, "_init_")) {
        if(scriptEvent("error"))
            return false;
    }

    skip();
    return true;
}

char *script::interp::getTemp(void)
{
    unsigned tempcount = sizeof(temps) / sizeof(char *);

    char *ptr = temps[tempindex++];
    if(tempindex >= tempcount)
        tempindex = 0;
    return ptr;
}

const char *script::interp::getFormat(script::symbol *sym, const char *id, char *tp)
{
#ifdef  __MINGW32__
    long long val;
#else
    long val;
#endif

    static char null[1] = {0};

    unsigned dcount = 0, pos;
    char *cp;
    char *ep;
    char idbuf[32];
    unsigned len;
    char quote = 0;
    script::symbol *index = NULL;
    unsigned paren = 0;

    cp = sym->data;
    while(isspace(*cp))
        ++cp;

    if(eq(id, "len:", 4))
        snprintf(tp, script::sizing + 1, "%u", (unsigned)strlen(sym->data));
    else if(eq(id, "key:", 4)) {
        String::set(tp, script::sizing + 1, cp);
        ep = strchr(tp, '=');
        if(ep)
            *ep = 0;
        else
            String::set(tp, 2, "-");
    }
    else if(eq(id, "tail:", 5)) {
        dcount = script::count(sym->data);
        if(dcount)
            String::set(tp, script::sizing + 1, script::get(cp, --dcount));
        else
            tp[0] = 0;
    }
    else if(eq(id, "head:", 5)) {
        String::set(tp, script::sizing + 1, sym->data);
        if(script::count(tp) > 1) {
            dcount = script::offset(tp, 1);
            if(dcount)
                tp[--dcount] = 0;
        }
    }
    else if(eq(id, "pull:", 5) || eq(id, "<<:", 3)) {
        String::set(tp, script::sizing + 1, sym->data);
        if(script::count(tp) > 1) {
            dcount = script::offset(tp, 1);
            if(dcount) {
                memcpy(sym->data, sym->data + dcount, strlen(sym->data + dcount) + 1);
                tp[--dcount] = 0;
            }
        }
    }
    else if(eq(id, "pop:", 4) || eq(id, ">>:", 3)) {
        dcount = script::count(sym->data);
        if(dcount) {
            String::set(tp, script::sizing + 1, script::get(sym->data, --dcount));
            if(dcount) {
                dcount = script::offset(sym->data, dcount);
                sym->data[--dcount] = 0;
            }
            else
                sym->data[0] = 0;
        }
        else
            tp[0] = 0;
    }
    else if(eq(id, "val:", 4) && (*cp == '\'' || *cp == '\"' || *cp == '(')) {
        String::set(tp, script::sizing + 1, cp + 1);
        if(*cp == '(')
            ep = strrchr(tp, ')');
        else
            ep = strchr(tp, *cp);
        if(ep)
            *ep = 0;
    }
    else if(eq(id, "val:", 4) && strchr(sym->data, '=')) {
        cp = strchr(sym->data, '=') + 1;
        if(*cp == '\"' || *cp == '\'') {
            String::set(tp, script::sizing + 1, cp + 1);
            ep = strchr(tp, *cp);
            if(ep)
                *ep = 0;
        }
        else {
            String::set(tp, script::sizing + 1, cp);
            ep = strchr(tp, ',');
            if(ep)
                *ep = 0;
        }
    }
    else if(eq(id, "unquote:", 8) && sym->size) {
        cp = sym->data;
        while(isspace(*cp))
            ++cp;
        if(*cp == '(') {
            ++cp;
            ep = strrchr(cp, ')');
            if(ep)
                *ep = 0;
        }
        else if(*cp == '\"' || *cp == '\'') {
            ep = strchr(cp + 1, *cp);
            if(ep)
                *ep = 0;
            ++cp;
        }
        String::set(sym->data, sym->size + 1, cp);
        return sym->data;
    }
    else if(eq(id, "upper:", 6) && sym->size) {
        String::upper(sym->data);
        return sym->data;
    }
    else if(eq(id, "lower:", 6) && sym->size) {
        String::lower(sym->data);
        return sym->data;
    }
    else if((eq(id, "inc:", 4) || eq(id, "++:", 3)) && sym->size) {
        val = atol(cp);
        snprintf(sym->data, sym->size + 1, NUM_FORMAT, ++val);
        return sym->data;
    }
    else if(eq(id, "count:", 6))
        snprintf(tp, script::sizing + 1, "%u", script::count(sym->data));
    else if(eq(id, "index/", 6)) {
        id += 6;
        pos = atoi(id);
        if(pos)
            --pos;
        else
            return "";
        if(pos < strlen(sym->data))
            cp = sym->data + pos;
        else
            cp = null;
        script::copy(cp, tp, script::sizing);
    }
    else if(eq(id, "offset/", 7)) {
        id += 7;
        pos = atoi(id);
        if(pos)
            --pos;
        else
            return "";
offset:
        cp = script::get(sym->data, pos);
        script::copy(cp, tp, script::sizing);
    }
    else if((eq(id, "dec:", 4) || eq(id, "--:", 3)) && sym->size) {
        val = atoi(cp);
        snprintf(sym->data, sym->size + 1, NUM_FORMAT, --val);
        return sym->data;
    }
    else if(eq(id, "size:", 5))
        snprintf(tp, script::sizing + 1, "%d", sym->size);
    else if(eq(id, "val:", 4) || eq(id, "int:", 4)) {
        val = atol(cp);
        snprintf(tp, script::sizing + 1, NUM_FORMAT, val);
    }
    else if(eq(id, "num:", 4)) {
        val = atol(cp);
        snprintf(tp, script::sizing + 1, NUM_FORMAT, val);
        if(script::decimals)
            String::add(tp, script::sizing + 1, ".");
        pos = strlen(tp);
        cp = strchr(sym->data, '.');
        if(cp)
            ++cp;
        else
            cp = null;
        while(dcount++ < script::decimals && pos < script::sizing) {
            if(*cp && isdigit(*cp))
                tp[pos++] = *(cp++);
            else
                tp[pos++] = '0';
        }
        tp[pos] = 0;
    }
    else if(eq(id, "map/", 4)) {
        idbuf[0] = '%';
        String::set(idbuf, sizeof(idbuf), id + 4);
        ep = strchr(idbuf, ':');
        if(ep)
            *ep = 0;
        index = find(idbuf);
        if(!index || !index->data[0])
            return "";
        pos = atoi(sym->data);
        if(pos)
            goto offset;
        snprintf(idbuf, sizeof(idbuf), ",%s", sym->data);
        paren = 0;
        goto search;
    }
    else if(eq(id, "find/", 5)) {
        id += 5;
        paren = 0;
        dcount = 1;
        idbuf[0] = ',';
        while(dcount < 30 && *id && *id != ':')
            idbuf[dcount++] = *(id++);
        idbuf[dcount++] = '=';
        idbuf[dcount] = 0;
search:
        cp = sym->data;
        len = strlen(idbuf);
        if(!strncmp(cp, idbuf + 1, len - 1))
            cp = sym->data + len - 1;
        else if(NULL != (cp = strstr(sym->data, idbuf)))
            cp = cp + len;
        else
            cp = null;
        dcount = 0;
        if(*cp == '\"') {
            quote = *cp;
            ++cp;
        }
        else if(*cp == '(') {
            paren = 1;
            ++cp;
        }
        else
            quote=',';
        while(dcount < script::sizing && *cp && *cp != quote) {
            if(*cp == '(' && paren)
                ++paren;
            if(*cp == ')' && paren) {
                if(paren-- == 1)
                    break;
            }
            if(*cp == '=' && quote == ',' && cp[1] == '\"') {
                tp[dcount++] = *(cp++);
                quote = *(cp++);
            }
            else
                tp[dcount++] = *(cp++);
        }
        tp[dcount] = 0;
    }
    else if(eq(id, "bool:", 5)) {
        if(atoi(sym->data) > 0 || tolower(sym->data[0]) == 't' || tolower(sym->data[0] == 'y'))
            snprintf(tp, script::sizing + 1, "true");
        else
            snprintf(tp, script::sizing + 1, "false");
    }
    return tp;
}

const char *script::interp::getContent(const char *cp)
{
    script::symbol *sym;
    const char *id;
    char *tp;

    if(!cp)
        return NULL;

    switch(*cp) {
    case '$':
        tp = getTemp();
        id = strchr(cp, ':');
        if(id) {
            sym = find(++id);
            if(!sym)
                return "";
            *tp = 0;
            return getFormat(sym, ++cp, tp);
        }
    case '%':
        if(eq(cp, "%index"))
            return getIndex();
        sym = find(++cp);
        if(sym)
            return sym->data;
        else
            return "";
    case '&':
    case '=':
    case '+':
        if(cp[1] && cp[1] != '=')
            return ++cp;
    }
    return cp;
}

const char *script::interp::getKeyword(const char *id)
{
    line_t *line = stack[frame].line;
    unsigned index = 0;
    const char *cp;

    while(index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '=') {
            if(eq(id, ++cp))
                return getContent(line->argv[index]);
            ++index;
        }
    }
    return NULL;
}

const char *script::interp::getIndex(void)
{
    method_t method;

    unsigned pos = frame;
    while(pos) {
        method = stack[--pos].line->method;
        if(method == (method_t)&script::methods::scrForeach) {
            char *temp = getTemp();
            snprintf(temp, script::sizing, "%d", stack[pos].index);
            return temp;
        }
    }
    return "0";
}

script::method_t script::interp::getLooping(void)
{
    if(!frame)
        return (method_t)NULL;

    return stack[frame - 1].line->method;
}

const char *script::interp::getContent(void)
{
    const char *cp;
    line_t *line = stack[frame].line;
    while(stack[frame].index < line->argc) {
        cp = line->argv[stack[frame].index++];
        switch(*cp) {
        case '=':
            ++stack[frame].index;
            break;
        default:
            return cp;
        }
    }
    return NULL;
}

const char *script::interp::getValue(void)
{
    const char *cp;
    line_t *line = stack[frame].line;
    while(stack[frame].index < line->argc) {
        cp = line->argv[stack[frame].index++];
        switch(*cp) {
        case '=':
            ++stack[frame].index;
            break;
        default:
            return getContent(cp);
        }
    }
    return NULL;
}

bool script::interp::setConst(const char *id, const char *value)
{
    unsigned path;
    linked_pointer<script::symbol> sp;
    script::symbol *var = NULL;

    if(*id == '=' || *id == '%')
        ++id;

    if(!isalnum(*id))
        return false;

    path = NamedObject::keyindex(id, script::indexing);
    sp = syms[path];

    while(is(sp)) {
        if(eq(sp->name, id) && sp->scope == stack[frame].scope)
            return false;
        sp.next();
    }

    var = (script::symbol *)alloc(sizeof(script::symbol));

    var->name = dup(id);
    var->scope = stack[frame].scope;
    var->size = 0;
    var->data = dup(value);
    var->enlist(&syms[path]);
    return true;
}

script::symbol *script::interp::createSymbol(const char *id)
{
    unsigned path;
    linked_pointer<script::symbol> sp;
    script::symbol *var = NULL, *local = NULL;
    unsigned size = script::sizing;
    char *cp;

    if(*id == '=' || *id == '%')
        ++id;

    if(!isalnum(*id))
        return NULL;

    if(strchr(id, ':')) {
        char *temp = getTemp();

        String::set(temp, script::sizing + 1, id);
        cp = strchr(temp, ':');
        if(cp) {
            *(cp++) = 0;
            id = temp;
            size = atoi(cp);
        }
    }

    path = NamedObject::keyindex(id, script::indexing);
    sp = syms[path];

    if(!size)
        size = script::sizing;

    while(is(sp)) {
        if(eq(sp->name, id) && sp->scope == NULL)
            var = *sp;
        if(eq(sp->name, id) && sp->scope == stack[frame].scope)
            local = *sp;
        sp.next();
    }

    if(local)
        var = local;

    if(!var) {
        var = (script::symbol *)alloc(sizeof(script::symbol));

        var->name = dup(id);

        var->scope = NULL;
        var->size = size;
        var->data = (char *)alloc(size + 1);
        var->enlist(&syms[path]);
        var->data[0] = 0;
    }
    return var;
}

unsigned script::interp::getTypesize(const char *type_id)
{
    if(!type_id)
        return 0;

    if(eq(type_id, "int"))
        return 10;

    if(eq(type_id, "num"))
        return 20;

    if(eq(type_id, "bool"))
        return 1;

    return 0;
}

const char *script::interp::getTypeinit(const char *type_id)
{
    if(!type_id)
        return NULL;

    if(eq(type_id, "int") || eq(type_id, "num"))
        return "0";

    if(eq(type_id, "bool"))
        return "f";

    return NULL;
}

void script::interp::getParams(script::header *scope, script::line_t *line)
{
    unsigned index = 0;
    const char *cp;
    const char *id;
    unsigned param = 0;
    char pbuf[8];
    script::symbol *sym;

    while(index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '=') {
            id = ++cp;
            cp = line->argv[index++];
        }
        else {
            snprintf(pbuf, sizeof(pbuf), "%d", ++param);
            id = pbuf;
        }
        if(eq(cp, "required") || eq(cp, "optional"))
            setRef(scope, id, const_cast<char *>(""), 0);
        else if(*cp == '%') {
            sym = find(++cp);
            if(sym) {
                setRef(scope, id, sym->data, sym->size);
                continue;
            }
            else
                setRef(scope, id, const_cast<char *>(""), 0);

        }
        else {
            cp = getContent(cp);
            setRef(scope, id, const_cast<char *>(cp), 0);
        }
    }
}

void script::interp::setRef(script::header *scope, const char *id, char *value, unsigned size)
{
    unsigned path;
    linked_pointer<script::symbol> sp;
    script::symbol *var = NULL;

    if(*id == '=' || *id == '%')
        ++id;

    if(!isalnum(*id))
        return;

    path =  NamedObject::keyindex(id, script::indexing);
    sp = syms[path];

    while(is(sp)) {
        if(eq(sp->name, id) && sp->scope == scope) {
            var = *sp;
            break;
        }
        sp.next();
    }

    if(!var) {
        var = (script::symbol *)alloc(sizeof(script::symbol));

        var->name = dup(id);

        var->scope = scope;
        var->enlist(&syms[path]);
    }

    var->size = size;
    var->data = value;
}

script::symbol *script::interp::getVar(const char *id, const char *value)
{
    unsigned path;
    linked_pointer<script::symbol> sp;
    script::symbol *var = NULL;
    unsigned size = script::sizing;
    char *cp;

    if(*id == '=' || *id == '%')
        ++id;

    if(!isalnum(*id))
        return NULL;

    if(strchr(id, ':')) {
        char *temp = getTemp();

        String::set(temp, script::sizing + 1, id);
        cp = strchr(temp, ':');
        if(cp) {
            *(cp++) = 0;
            id = temp;
            size = getTypesize(cp);
            if(!value)
                value = getTypeinit(cp);
            if(!size)
                size = atoi(cp);
        }
    }

    path = NamedObject::keyindex(id, script::indexing);
    sp = syms[path];

    if(!size)
        size = script::sizing;

    while(is(sp)) {
        if(eq(sp->name, id) && sp->scope == stack[frame].scope) {
            var = *sp;
            break;
        }
        sp.next();
    }

    if(!var) {
        var = (script::symbol *)alloc(sizeof(script::symbol));

        var->name = dup(id);

        var->scope = stack[frame].scope;
        var->size = size;
        var->data = (char *)alloc(size + 1);
        var->enlist(&syms[path]);
        var->data[0] = 0;
    }

    // if const, we do not re-write...
    if(!var->size)
        return var;

    // assign value, whether new or exists to reset...
    if(value && *value)
        String::set(var->data, var->size + 1, value);
    else if(value)
        var->data[0] = 0;
    return var;
}

script::symbol *script::interp::find(const char *id)
{
    unsigned path = NamedObject::keyindex(id, script::indexing);
    linked_pointer<script::symbol> sp = syms[path];
    script::symbol *global = NULL, *local = NULL;

    while(is(sp)) {
        if(eq(sp->name, id)) {
            if(sp->scope == NULL)
                global = *sp;
            else if(sp->scope == stack[frame].scope)
                local = *sp;
        }
        sp.next();
    }

    if(local)
        return local;

    return global;
}

void script::interp::detach(void)
{
    image = NULL;
}

void script::interp::startScript(script::header *scr)
{
    linked_pointer<script::event> ep = scr->events;

    while(is(ep)) {
        if(eq(ep->name, "init")) {
            push();
            stack[frame].event = *ep;
            stack[frame].line = ep->first;
            return;
        }
        ep.next();
    }
}

bool script::interp::attach(script *img, const char *entry)
{
    script::header *main = NULL;
    linked_pointer<script::header> hp;
    unsigned path = 0;
    const char *cp;

    image = img;

    if(!img)
        return false;

    if(entry && *entry == '@')
        main = script::find(img, entry);
    else while(entry && main == NULL && path < script::indexing) {
        hp = img->scripts[path++];
        while(is(hp)) {
            cp = hp->name;
            if(*cp == '@' && match(cp, entry)) {
                main = *hp;
                break;
            }
            hp.next();
        }
    }

    if(!main)
        main = script::find(img, "@main");

    if(main) {
        setStack(main);
        push();
        setStack(img->first);

        while(frame && stack[frame].line)
            (this->*(stack[frame].line->method))();

        if(is(img->shared)) {
            frame = 1;
            setStack(img->shared->first);
            while(frame && stack[frame].line)
                (this->*(stack[frame].line->method))();
        }

        frame = 0;
        startScript(main);
        return true;
    }
    image = NULL;
    return false;
}

void script::interp::skip(void)
{
    stack[frame].line = stack[frame].line->next;
}

bool script::interp::match(const char *found, const char *name)
{
    assert(found != NULL);
    assert(name != NULL);

    if(*found == '@')
        ++found;

    return !stricmp(found, name);
}

bool script::interp::isInherited(const char *name)
{
    return true;
}

script::event *script::interp::scriptMethod(const char *name)
{
    linked_pointer<script::event> mp = stack[frame].scr->methods;

    while(is(mp)) {
        if(match(mp->name, name))
            return *mp;
        mp.next();
    }
    return NULL;
}

bool script::interp::scriptEvent(const char *name)
{
    linked_pointer<script::event> ep;
    bool inherit = isInherited(name);
    unsigned stackp = frame;

    for(;;) {
        ep = stack[stackp].scr->events;

        while(is(ep)) {
            if(match(ep->name, name))
                break;

            ep.next();
        }

        if(!is(ep) && !inherit)
            return false;

        if(is(ep)) {

            if(stack[stackp].event == *ep)
                return false;

            frame = stackp;

            pullScope();
            setStack(stack[frame].scr, *ep);
            return true;
        }

        while(stackp > stack[stackp].base && stack[stackp].line->loop)
            --stackp;

        if(stackp && stackp >= stack[stackp].base)
            --stackp;
        else
            return false;
    }
}

void script::interp::setStack(script::header *scr, script::event *ev)
{
    stack[frame].scr = scr;
    stack[frame].event = ev;
    stack[frame].index = 0;
    stack[frame].resmask = scr->resmask;

    if(ev)
        stack[frame].line = ev->first;
    else
        stack[frame].line = scr->first;
}

void script::interp::pullBase(void)
{
    while(frame && stack[frame - 1].base == stack[frame].base)
        --frame;
}

void script::interp::pullScope(void)
{
    while(frame && stack[frame - 1].scr == stack[frame].scr)
        --frame;
}

void script::interp::pullLoop(void)
{
    skip();
    if(frame) {
        stack[frame - 1].line = stack[frame].line;
        stack[frame - 1].index = 0;
        --frame;
    }
}

unsigned script::interp::getResource(void)
{
    if(!stack || !stack[frame].scr || (frame == 0 && !stack[frame].line))
        return 0;

    if(stack[frame].line)
        return stack[frame].line->mask | stack[frame].resmask;

    return stack[frame].resmask;
}

bool script::interp::pop(void)
{
    pullScope();
    if(frame)
        --frame;
    else
        return false;

    return true;
}

void script::interp::push(void)
{
    if(frame >= script::stacking) {
        if(!scriptEvent("stack")) {
            frame = 0;
            stack[frame].line = NULL;
            return;
        }
    }

    stack[frame + 1] = stack[frame];
    ++frame;
}

bool script::interp::trylabel(const char *label)
{
    if(*label != '@')
        return false;

    script::header *scr = script::find(*image, label);
    if(!scr || !scr->first || stack[stack[frame].base].scr == scr)
        return false;

    frame = stack[frame].base;
    setStack(scr, NULL);
    return true;
}

bool script::interp::tryexit(void)
{
    if(stack[frame].event && eq("exit", stack[frame].event->name))
        return false;

    if(scriptEvent("exit"))
        return true;

    script::header *ex = script::find(image.get(), "@exit");
    frame = 0;

    if(!ex || stack[frame].scr == ex)
        return false;

    setStack(ex, NULL);
    return true;
}

bool script::interp::step(void)
{
    unsigned scount = script::stepping;
    line_t *line = stack[frame].line;
    bool rtn = true;

    while(line && rtn && scount--) {
        rtn = (this->*(line->method))();
        line = stack[frame].line;
    }

    if(line)
        return true;

    while(stack[frame].line == NULL && frame)
        --frame;

    if(!stack[frame].line && !tryexit())
        return false;

    return true;
}

bool script::interp::getCondition(const char *test, const char *v)
{
    unsigned points = 0;
    script::symbol *sym;

    if(eq(test, "defined")) {
        if(!v)
            return false;
        if(*v == '%') {
            sym = find(++v);
            if(!sym)
                return false;
            return true;
        }
        v = getContent(v);
        if(!*v)
            return false;
        return true;
    }

    if(eq(test, "const")) {
        if(!v)
            return false;

        if(*v == '%') {
            sym = find(++v);
            if(!sym)
                return false;
            if(sym->size)
                return false;
            return true;
        }
        else
            return false;
    }

    if(eq(test, "modify")) {
        if(!v)
            return false;

        if(*v == '%') {
            sym = find(++v);
            if(!sym)
                return false;
            if(sym->size)
                return true;
            return false;
        }
        else
            return false;
    }

    v = getContent(v);
    if(eq(test, "empty")) {
        if(v && *v)
            return false;
        return true;
    }

    if(eq(test, "integer")) {
        if(!v || !*v)
            return false;

        if(*v == '-')
            ++v;

        while(*v) {
            if(!isdigit(*v))
                return false;
            ++v;
        }
        return true;
    }

    if(eq(test, "digits")) {
        if(!v || !*v)
            return false;

        while(*v) {
            if(!isdigit(*v))
                return false;
            ++v;
        }
        return true;
    }

    if(eq(test, "number")) {
        if(!v || !*v)
            return false;

        if(*v == '-')
            ++v;

        while(*v) {
            if(*v == '.') {
                if(points)
                    return false;
                ++points;
                ++v;
                continue;
            }
            if(!isdigit(*v))
                return false;
            ++v;
        }

        return true;
    }

    return false;
}

bool script::interp::isConditional(unsigned index)
{
    script::line_t *line = stack[frame].line;
    const char *cp;
    bool rtn = false;

    while(index < line->argc) {
        rtn = getExpression(index);
        cp = line->argv[index];
        if((*cp == '-' || *cp == '!') && isalpha(cp[1]))
            index += 2;
        else
            index += 3;

        if(index >= line->argc)
            return rtn;

        cp = line->argv[index++];

        if(eq(cp, "?&&") || eq(cp, "and")) {
            if(!rtn)
                return false;
            rtn = false;
            continue;
        }
        if(eq(cp, "?||") || eq(cp, "or")) {
            if(rtn)
                return true;
            rtn = false;
            continue;
        }
        break;
    }
    return rtn;
}

bool script::interp::getExpression(unsigned index)
{
    script::line_t *line = stack[frame].line;
    const char *v1 = "", *v2 = "", *op;
    const char *d1, *d2;
    unsigned len;
    char dec1[9], dec2[9];
    long dv1, dv2;
    unsigned pos = 0, pcount;

#ifdef  __MINGW32__
    long long dv;
#else
    long dv;
#endif

    if(index < line->argc) {
        v1 = line->argv[index++];
        if((*v1 == '-' || *v1 == '!') && isalpha(v1[1])) {
            if(index < line->argc)
                v2 = line->argv[index++];
            else
                v2 = NULL;
            if(*v1 == '-')
                return getCondition(++v1, v2);
            else
                return !getCondition(++v1, v2);
        }
        v1 = getContent(v1);
    }
    else
        return false;

    if(index <= line->argc) {
        op = line->argv[index++];
        if(*op == '?')
            ++op;
    }
    else if(v1 && *v1)
        return true;
    else
        return false;

    if(index <= line->argc)
        v2 = getContent(line->argv[index++]);

    d1 = strchr(v1, '.');
    d2 = strchr(v2, '.');
    dec1[0] = 0;
    dec2[0] = 0;
    if(d1) {
        dv = atol(++d1);
        snprintf(dec1, sizeof(dec1), NUM_FORMAT, dv);
    }
    if(d2) {
        dv = atol(++d2);
        snprintf(dec2, sizeof(dec2), NUM_FORMAT, dv);
    }

    unsigned c1 = strlen(dec1);
    unsigned c2 = strlen(dec2);
    while(c1 < 8)
        dec1[c1++] = '0';

    while(c2 < 8)
        dec2[c2++] = '0';

    dec1[8] = dec2[8] = 0;
    if(eq(op, "="))
        return ((atol(v1) == atol(v2)) && !strcmp(dec1, dec2));

    if(eq(op, "<>"))
        return ((atol(v1) != atol(v2)) || strcmp(dec1, dec2));

    if(eq(op, "&&"))
        return (*v1 && *v2);

    if(eq(op, "||"))
        return (*v1 || *v2);

    if(eq(op, "gt")) {
        if(strcmp(v1, v2) > 0)
            return true;
        return false;
    }

    if(eq(op, "lt")) {
        if(strcmp(v1, v2) < 0)
            return true;
        return false;
    }

    if(eq(op, "ge")) {
        if(strcmp(v1, v2) >= 0)
            return true;
        return false;
    }

    if(eq(op, "le")) {
        if(strcmp(v1, v2) <= 0)
            return true;
        return false;
    }

    if(eq(op, "==") || eq(op, "eq"))
        return eq(v1, v2);

    if(eq(op, "!=") || eq(op, "ne"))
        return !eq(v1, v2);

    if(eq(op, "<")) {
        dv1 = atol(dec1);
        dv2 = atol(dec2);
        if(*v1 == '-') {
            dv1 = -dv1;
            dv2 = -dv2;
        }
        return (atol(v1) < atol(v2) || (atol(v1) == atol(v2) && dv1 < dv2));
    }

    if(eq(op, "<=")) {
        dv1 = atol(dec1);
        dv2 = atol(dec2);
        if(*v1 == '-') {
            dv1 = -dv1;
            dv2 = -dv2;
        }
        return (atoi(v1) < atoi(v2) || (atol(v1) == atol(v2) && dv1 <= dv2));
    }

    if(eq(op, ">")) {
        dv1 = atol(dec1);
        dv2 = atol(dec2);
        if(*v1 == '-') {
            dv1 = -dv1;
            dv2 = -dv2;
        }
        return (atol(v1) > atol(v2) || (atol(v1) == atol(v2) && dv1 > dv2));
    }

    if(eq(op, ">=")) {
        dv1 = atol(dec1);
        dv2 = atol(dec2);
        if(*v1 == '-') {
            dv1 = -dv1;
            dv2 = -dv2;
        }
        return (atol(v1) > atol(v2) || (atol(v1) == atol(v2) && dv1 >= dv2));
    }

    if(eq(op, "?") || eq(op, "in")) {
        pcount = script::count(v2);
        if(!pcount)
            return false;

        len = strlen(v1);
        while(pos < pcount) {
            op = script::get(v2, pos++);
            if(op && eq(v1, op, len) && (op[len] == ',' || op[len] == 0 || op[len] == '='))
                return true;
        }
        return false;
    }

    if(eq(op, "!?") || eq(op, "notin")) {
        pcount = script::count(v2);
        if(!pcount)
            return true;

        len = strlen(v1);
        while(pos < pcount) {
            op = script::get(v2, pos++);
            if(op && eq(v1, op, len) && (op[len] == ',' || op[len] == 0 || op[len] == '='))
                return false;
        }
        return true;
    }

    if(eq(op, "!$") || eq(op, "isnot"))
        return !match(v2, v1);

    if(eq(op, "$") || eq(op, "is"))
        return match(v2, v1);

#ifdef  HAVE_REGEX_H
    if(eq(op, "~") || eq(op, "!~")) {
        bool rtn = false;
        regex_t *regex = new regex_t;
        memset(regex, 0, sizeof(regex_t));

        if(regcomp(regex, v2, REG_ICASE|REG_NOSUB|REG_NEWLINE)) {
            regfree(regex);
            delete regex;
            return false;
        }

        if(regexec(regex, v1, 0, NULL, 0))
            rtn = false;
        else
            rtn = true;

        regfree(regex);
        delete regex;
        if(*op == '!')
            return !rtn;
        return rtn;
    }
#endif
    return false;
}
