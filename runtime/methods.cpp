// Copyright (C) 2008-2010 David Sugar, Tycho Softworks.
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

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

bool script::methods::scrDo(void)
{
    push();
    skip();
    return true;
}

bool script::methods::scrPrevious(void)
{
    --frame;
    if(stack[frame].index < 2) {
        stack[frame].index = 0;
        skip();
        return scrBreak();
    }
    stack[frame].index -= 2;
    return true;
}

bool script::methods::scrExpand(void)
{
    unsigned index = 1;
    script::line_t *line = stack[frame].line;
    const char *tuples = getContent(line->argv[0]);
    unsigned tcount = script::count(tuples);
    script::symbol *sym;
    const char *cp;

    while(line->argv[index]) {
        sym = find(line->argv[index]);

        if(!sym)
            sym = getVar(line->argv[index]);

        if(!sym)
            return error("symbol not found");

        if(!sym->size)
            return error("symbol not writable");

        if(index > tcount)
            String::set(sym->data, sym->size + 1, "");
        else {
            cp = script::get(tuples, index - 1);
            script::copy(cp, sym->data, sym->size + 1);
        }
        ++index;
    }
    skip();
    return true;
}

bool script::methods::scrForeach(void)
{
    script::line_t *line = stack[frame].line;
    script::symbol *sym = find(line->argv[0]);
    const char *cp;
    unsigned index = stack[frame].index;

    if(!sym)
        sym = getVar(line->argv[0]);

    if(!sym)
        return error("symbol not found");

    if(!sym->size)
        return error("symbol not writable");

    if(!index) {
        cp = line->argv[2];
        if(cp && (*cp == '%' || *cp == '&')) {
            script::symbol *skip = find(cp);
            if(!skip)
                skip = getVar(cp);
            if(skip) {
                index = atoi(skip->data);
                if(skip->size)
                    String::set(skip->data, 2, "0");
            }
        }
        else if(NULL != (cp = getContent(cp))) {
            cp = getContent(cp);
            if(cp)
                index = atoi(cp);
        }
    }

    cp = script::get(getContent(line->argv[1]), index);
    if(cp == NULL) {
        stack[frame].index = 0;
        skip();
        return scrBreak();
    }

    script::copy(cp, sym->data, sym->size + 1);
    stack[frame].index = ++index;
    push();
    skip();
    return true;
}

bool script::methods::scrWhile(void)
{
    if(isConditional(0)) {
        push();
        skip();
        return true;
    }
    skip();
    return scrBreak();
}

bool script::methods::scrEndcase(void)
{
    pullLoop();
    return true;
}

bool script::methods::scrCase(void)
{
    script::method_t method = getLooping();
    script::line_t *line = stack[frame].line;

    if(method == (method_t)&script::methods::scrCase) {
        skip();
        while(stack[frame].line && stack[frame].line->loop > line->loop)
            skip();
        return true;
    }

    if(isConditional(0)) {
        push();
        skip();
        return true;
    }

    return scrOtherwise();

    if(stack[frame].line->method == (method_t)&script::methods::scrOtherwise) {
        push();
        skip();
        return false;
    }
    return false;
}

bool script::methods::scrOtherwise(void)
{
    script::line_t *line = stack[frame].line;

    skip();
    while(stack[frame].line && stack[frame].line->loop > line->loop)
        skip();

    return true;
}

bool script::methods::scrUntil(void)
{
    if(isConditional(0))
        pullLoop();
    else
        --frame;
    return true;
}

bool script::methods::scrLoop(void)
{
    --frame;
    return true;
}

bool script::methods::scrElif(void)
{
    return scrElse();
}

bool script::methods::scrEndif(void)
{
    return scrNop();
}

bool script::methods::scrDefine(void)
{
    return scrNop();
}

bool script::methods::scrInvoke(void)
{
    script::line_t *line = stack[frame].line;
    unsigned mask;

    getParams(line->sub, line->sub->first);
    getParams(line->sub, line);

    skip(); // return to next line
    push(); // push where we are
    stack[frame].scope = line->sub;
    stack[frame].line = line->sub->first->next;
    startScript(line->sub);

    mask = stack[frame].resmask |= line->sub->resmask;
    if(mask != stack[frame].resmask) {
        stack[frame].resmask = mask;
        return false;
    }
    return true;
}

bool script::methods::scrElse(void)
{
    script::line_t *line = stack[frame].line;
    unsigned loop = 0;

    skip();
    while(NULL != (line = stack[frame].line)) {
        if(line->method == (method_t)&script::methods::scrIf)
            ++loop;
        else if(line->method == (method_t)&script::methods::scrEndif) {
            if(!loop) {
                skip();
                return true;
            }
            --loop;
        }
        skip();
    }
    return true;
}

bool script::methods::scrIf(void)
{
    script::line_t *line = stack[frame].line;
    unsigned loop = 0;

    if(isConditional(0)) {
        skip();
        return true;
    }

    skip();
    while(NULL != (line = stack[frame].line)) {
        if(line->method == (method_t)&script::methods::scrIf)
            ++loop;
        else if(line->method == (method_t)&script::methods::scrEndif) {
            if(!loop) {
                skip();
                return true;
            }
            --loop;
        }
        else if(!loop && line->method == (method_t)&script::methods::scrElse) {
            skip();
            return true;
        }
        else if(!loop && line->method == (method_t)&script::methods::scrElif) {
            if(isConditional(0)) {
                skip();
                return true;
            }
        }
        skip();
    }
    return false;
}

bool script::methods::scrPause(void)
{
    return false;
}

bool script::methods::scrBreak(void)
{
    script::line_t *line = stack[frame].line;
    script::method_t method = getLooping();

    if(method == (method_t)&script::methods::scrCase || method == (method_t)&script::methods::scrOtherwise) {
        skip();
        while(stack[frame].line && stack[frame].line->loop >= line->loop)
            skip();
        return true;
    }

    pullLoop();
    while(stack[frame].line && stack[frame].line->loop >= line->loop)
        skip();
    return true;
}

bool script::methods::scrRepeat(void)
{
    --frame;
    --stack[frame].index;
    return true;
}

bool script::methods::scrContinue(void)
{
    --frame;
    return true;
}

bool script::methods::scrNop(void)
{
    skip();
    return true;
}

bool script::methods::scrWhen(void)
{
    if(!isConditional(0))
        skip();

    skip();
    return true;
}

bool script::methods::scrExit(void)
{
    frame = 0;
    stack[frame].line = NULL;
    return false;
}

bool script::methods::scrRestart(void)
{
    unsigned mask = stack[frame].resmask;
    pullBase();
    setStack(stack[frame].scr);
    if(mask == stack[frame].resmask)
        return true;
    return false;
}

bool script::methods::scrGoto(void)
{
    script::line_t *line = stack[frame].line;
    const char *cp;
    script::header *scr = NULL;
    unsigned index = 0;
    unsigned mask = stack[frame].resmask;

    while(!scr && index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '^') {
            if(scriptEvent(++cp))
                return true;
        }
        else
            scr = script::find(*image, line->argv[0]);
    }
    if(!scr)
        return error("label not found");

    pullBase();
    setStack(scr);
    startScript(scr);
    if(mask == stack[frame].resmask)
        return true;
    return false;
}

bool script::methods::scrGosub(void)
{
    unsigned index = stack[frame].index;
    script::line_t *line = stack[frame].line;
    script::header *scr = NULL;
    const char *cp;
    unsigned mask = stack[frame].resmask;
    script::event *ev = NULL;

    while(ev == NULL && scr == NULL && index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '@') {
            scr = script::find(*image, cp);
            if(!scr && image->shared.get())
                scr = script::find(image->shared.get(), cp);
        }
        else
            ev = scriptMethod(cp);
    }

    if(!scr && !ev) {
        if(!stack[frame].index)
            return error("label not found");

        stack[frame].index = 0;
        skip();
        return true;
    }

    stack[frame].index = index; // save index for re-call on return...
    push();
    if(scr) {
        setStack(scr);
        startScript(scr);
        stack[frame].base = frame;  // set base level to new script...
        if(mask == stack[frame].resmask)
            return true;
    }
    else {
        stack[frame].line = ev->first;
        stack[frame].index = 0;
        return true;
    }
    return false;
}

bool script::methods::scrReturn(void)
{
    unsigned mask = stack[frame].resmask;
    if(!pop())
        return scrExit();

    if(mask == stack[frame].resmask)
        return true;
    return false;
}

bool script::methods::scrIndex(void)
{
    const char *op;
    int result = 0, value;

    stack[frame].index = 0;
    result = atoi(getValue());

    while(NULL != (op = getValue())) {
        switch(*op) {
        case '+':
            result += atoi(getValue());
            break;
        case '-':
            result -= atoi(getValue());
            break;
        case '*':
            result *= atoi(getValue());
            break;
        case '/':
            value = atoi(getValue());
            if(value == 0)
                return error("div by zero");
            result /= value;
            break;
        case '#':
            value = atoi(getValue());
            if(value == 0)
                return error("div by zero");
            result %= value;
            break;
        }
    }

    --frame;
    if(!result) {
        skip();
        return scrBreak();
    }
    else
        stack[frame].index = --result;

    return true;
}

bool script::methods::scrRef(void)
{
    const char *cp = stack[frame].line->argv[0];
    cp = getContent(cp);
    skip();
    return true;
}

bool script::methods::scrExpr(void)
{
    unsigned exdecimals = script::decimals;
    const char *id, *cp, *op, *aop;
    double result, value;
    long lvalue;
    char cvt[8];
    char buf[32];

    stack[frame].index = 0;

    cp = getKeyword("decimals");
    if(cp)
        exdecimals = atoi(cp);

    id = getContent();
    aop = getValue();
    result = atof(getValue());

    while(NULL != (op = getValue())) {
        switch(*op) {
        case '+':
            result += atof(getValue());
            break;
        case '-':
            result -= atof(getValue());
            break;
        case '*':
            result *= atof(getValue());
            break;
        case '/':
            value = atof(getValue());
            if(value == 0.) {
                return error("div by zero");
            }
            result /= value;
            break;
        case '#':
            lvalue = atol(getValue());
            if(lvalue == 0)
                return error("div by zero");
            result = (long)result % lvalue;
        }
    }
    cp = getContent(id);
    if(cp)
        value = atof(cp);
    else
        value = 0;
    if(eq(aop, "+="))
        value += result;
    else if(eq(aop, "#="))
        value = (long)value % (long)result;
    else if(eq(aop, "-="))
        value -= result;
    else if(eq(aop, "*="))
        value *= result;
    else if(eq(aop, "/=") && result)
        value /= result;
    else if(eq(aop, "?=") || eq(aop, ":="))
        value = result;

    stack[frame].index = 0;
    cvt[0] = '%';
    cvt[1] = '.';
    cvt[2] = '0' + exdecimals;
    cvt[3] = 'f';
    cvt[4] = 0;
    snprintf(buf, sizeof(buf), cvt, value);

    if(!getVar(id, buf))
        return error("invalid symbol");

    skip();
    return true;
}

bool script::methods::scrVar(void)
{
    unsigned index = 0;
    script::line_t *line = stack[frame].line;
    const char *id;

    while(index < line->argc) {
        id = line->argv[index++];
        if(*id == '=') {
            if(!getVar(++id, getContent(line->argv[index++])))
                return error("invalid symbol");
        }
        else {
            if(!getVar(id))
                return error("invalid symbol");
        }
    }
    skip();
    return true;
}

bool script::methods::scrConst(void)
{
    unsigned index = 0;
    script::line_t *line = stack[frame].line;
    const char *id;

    while(index < line->argc) {
        id = line->argv[index++];
        if(*id == '=') {
            if(!setConst(++id, getContent(line->argv[index++])))
                return error("invalid constant");
        }
    }
    skip();
    return true;
}

bool script::methods::scrError(void)
{
    char msg[65];
    unsigned index = 0;
    script::line_t *line = stack[frame].line;
    bool space = false;
    const char *cp;

    msg[0] = 0;
    while(index < line->argc) {
        if(space) {
            String::add(msg, 65, " ");
            space = false;
        }
        cp = line->argv[index++];
        if(*cp != '&')
            space = true;
        String::add(msg, 65, getContent(cp));
    }
    return error(msg);
}

bool script::methods::scrClear(void)
{
    unsigned index = 0;
    script::line_t *line = stack[frame].line;
    const char *id;
    script::symbol *sym;

    while(index < line->argc) {
        id = line->argv[index++];
        sym = find(id);

        if(!sym || !sym->size)
            return error("symbol not found");

        sym->data[0] = 0;
    }
    skip();
    return true;
}

bool script::methods::scrAdd(void)
{
    unsigned index = 1;
    script::line_t *line = stack[frame].line;
    script::symbol *sym = find(line->argv[0]);
    const char *cp;

    if(!sym)
        sym = getVar(line->argv[0]);

    if(!sym)
        return error("symbol not found");

    if(!sym->size)
        return error("symbol not writable");

    while(index < line->argc) {
        cp = getContent(line->argv[index++]);
        String::add(sym->data, sym->size + 1, cp);
    }
    skip();
    return true;
}

bool script::methods::scrPack(void)
{
    unsigned index = 1;
    script::line_t *line = stack[frame].line;
    script::symbol *sym = find(line->argv[0]);
    const char *cp;
    char quote[2] = {0,0};

    if(!sym)
        sym = getVar(line->argv[0]);

    if(!sym)
        return error("symbol not found");

    if(!sym->size)
        return error("symbol not writable");

    while(index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '=') {
            if(sym->data[0])
                String::add(sym->data, sym->size + 1, ",");

            String::add(sym->data, sym->size + 1, ++cp);
            String::add(sym->data, sym->size + 1, "=");
            cp = getContent(line->argv[index++]);
            quote[0] = '\'';
            goto add;
        }
        else {
            cp = getContent(cp);
            quote[0] = 0;
add:
            if(sym->data[0])
                String::add(sym->data, sym->size + 1, ",");

            if(!quote[0] && strchr(cp, ',')) {
                if(*cp != '\'' && *cp != '\"')
                    quote[0] = '\'';
            }
            // tuple nesting...
            if(!quote[0] && *cp == '\'')
                quote[0] = '(';
            if(quote[0])
                String::add(sym->data, sym->size + 1, quote);
            String::add(sym->data, sym->size + 1, cp);
            if(quote[0] == '(')
                quote[0] = ')';
            if(quote[0])
                String::add(sym->data, sym->size + 1, quote);
        }
    }
    skip();
    return true;
}

bool script::methods::scrPush(void)
{
    script::line_t *line = stack[frame].line;
    script::symbol *sym = createSymbol(line->argv[0]);
    unsigned size = 0;
    bool qflag = false;
    const char *key = NULL, *value;

    if(!sym)
        return error("invalid symbol id");

    if(!sym->size)
        return error("symbol not writable");

    if(line->argv[2]) {
        key = line->argv[1];
        value = line->argv[2];
    }
    else
        value = line->argv[1];

    if(key)
        size = strlen(key) + 1;

    if(*value != '\"' && *value != '\'') {
        qflag = true;
        size += 2;
    }
    size += strlen(value);

    size = strlen(line->argv[1]) + 3;
    if(line->argv[2])
        size += strlen(line->argv[2]);

    if(strlen(sym->data) + size > sym->size)
        return error("symbol too small");

    if(sym->data[0])
        String::add(sym->data, sym->size + 1, ",");

    size = strlen(sym->data);
    if(key && qflag)
        snprintf(sym->data + size, sym->size + 1 - size, "%s='%s'", key, value);
    else if(key)
        snprintf(sym->data + size, sym->size + 1 - size, "%s=%s", key, value);
    else if(qflag)
        snprintf(sym->data + size, sym->size + 1 - size, "'%s'", value);
    else
        String::set(sym->data + size, sym->size + 1, value);
    skip();
    return true;
}

bool script::methods::scrSet(void)
{
    unsigned index = 1;
    script::line_t *line = stack[frame].line;
    script::symbol *sym = createSymbol(line->argv[0]);
    const char *cp;

    if(!sym)
        return error("invalid symbol id");

    if(!sym->size)
        return error("symbol not writable");

    sym->data[0] = 0;
    while(index < line->argc) {
        cp = getContent(line->argv[index++]);
        String::add(sym->data, sym->size + 1, cp);
    }
    skip();
    return true;
}

