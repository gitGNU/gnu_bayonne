// Copyright (C) 2008-2011 David Sugar, Tycho Softworks.
//
// This file is part of GNU Bayonne.
//
// GNU Bayonne is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// GNU Bayonne is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Bayonne.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <bayonne.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

bool Script::methods::scrDo(void)
{
    push();
    skip();
    return true;
}

bool Script::methods::scrPrevious(void)
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

bool Script::methods::scrExpand(void)
{
    unsigned index = 1;
    Script::line_t *line = stack[frame].line;
    const char *tuples = getContent(line->argv[0]);
    unsigned tcount = Script::count(tuples);
    Script::symbol *sym;
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
            cp = Script::get(tuples, index - 1);
            Script::copy(cp, sym->data, sym->size + 1);
        }
        ++index;
    }
    skip();
    return true;
}

bool Script::methods::scrForeach(void)
{
    Script::line_t *line = stack[frame].line;
    Script::symbol *sym = find(line->argv[0]);
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
            Script::symbol *skip = find(cp);
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

    cp = Script::get(getContent(line->argv[1]), index);
    if(cp == NULL) {
        stack[frame].index = 0;
        skip();
        return scrBreak();
    }

    Script::copy(cp, sym->data, sym->size + 1);
    stack[frame].index = ++index;
    push();
    skip();
    return true;
}

bool Script::methods::scrWhile(void)
{
    if(isConditional(0)) {
        push();
        skip();
        return true;
    }
    skip();
    return scrBreak();
}

bool Script::methods::scrEndcase(void)
{
    pullLoop();
    return true;
}

bool Script::methods::scrCase(void)
{
    Script::method_t method = getLooping();
    Script::line_t *line = stack[frame].line;

    if(method == (method_t)&Script::methods::scrCase) {
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

    if(stack[frame].line->method == (method_t)&Script::methods::scrOtherwise) {
        push();
        skip();
        return false;
    }
    return false;
}

bool Script::methods::scrOtherwise(void)
{
    Script::line_t *line = stack[frame].line;

    skip();
    while(stack[frame].line && stack[frame].line->loop > line->loop)
        skip();

    return true;
}

bool Script::methods::scrUntil(void)
{
    if(isConditional(0))
        pullLoop();
    else
        --frame;
    return true;
}

bool Script::methods::scrLoop(void)
{
    --frame;
    return true;
}

bool Script::methods::scrElif(void)
{
    return scrElse();
}

bool Script::methods::scrEndif(void)
{
    return scrNop();
}

bool Script::methods::scrDefine(void)
{
    return scrNop();
}

bool Script::methods::scrInvoke(void)
{
    Script::line_t *line = stack[frame].line;
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

bool Script::methods::scrElse(void)
{
    Script::line_t *line = stack[frame].line;
    unsigned loop = 0;

    skip();
    while(NULL != (line = stack[frame].line)) {
        if(line->method == (method_t)&Script::methods::scrIf)
            ++loop;
        else if(line->method == (method_t)&Script::methods::scrEndif) {
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

bool Script::methods::scrIf(void)
{
    Script::line_t *line = stack[frame].line;
    unsigned loop = 0;

    if(isConditional(0)) {
        skip();
        return true;
    }

    skip();
    while(NULL != (line = stack[frame].line)) {
        if(line->method == (method_t)&Script::methods::scrIf)
            ++loop;
        else if(line->method == (method_t)&Script::methods::scrEndif) {
            if(!loop) {
                skip();
                return true;
            }
            --loop;
        }
        else if(!loop && line->method == (method_t)&Script::methods::scrElse) {
            skip();
            return true;
        }
        else if(!loop && line->method == (method_t)&Script::methods::scrElif) {
            if(isConditional(0)) {
                skip();
                return true;
            }
        }
        skip();
    }
    return false;
}

bool Script::methods::scrPause(void)
{
    return false;
}

bool Script::methods::scrBreak(void)
{
    Script::line_t *line = stack[frame].line;
    Script::method_t method = getLooping();

    if(method == (method_t)&Script::methods::scrCase || method == (method_t)&Script::methods::scrOtherwise) {
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

bool Script::methods::scrRepeat(void)
{
    --frame;
    --stack[frame].index;
    return true;
}

bool Script::methods::scrContinue(void)
{
    --frame;
    return true;
}

bool Script::methods::scrNop(void)
{
    skip();
    return true;
}

bool Script::methods::scrWhen(void)
{
    if(!isConditional(0))
        skip();

    skip();
    return true;
}

bool Script::methods::scrExit(void)
{
    frame = 0;
    stack[frame].line = NULL;
    return false;
}

bool Script::methods::scrRestart(void)
{
    unsigned mask = stack[frame].resmask;
    pullBase();
    setStack(stack[frame].scr);
    if(mask == stack[frame].resmask)
        return true;
    return false;
}

bool Script::methods::scrGoto(void)
{
    Script::line_t *line = stack[frame].line;
    const char *cp;
    Script::header *scr = NULL;
    unsigned index = 0;
    unsigned mask = stack[frame].resmask;

    while(!scr && index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '^') {
            if(scriptEvent(++cp))
                return true;
        }
        else
            scr = Script::find(*image, line->argv[0]);
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

bool Script::methods::scrGosub(void)
{
    unsigned index = stack[frame].index;
    Script::line_t *line = stack[frame].line;
    Script::header *scr = NULL;
    const char *cp;
    unsigned mask = stack[frame].resmask;
    Script::event *ev = NULL;

    while(ev == NULL && scr == NULL && index < line->argc) {
        cp = line->argv[index++];
        if(*cp == '@') {
            scr = Script::find(*image, cp);
            if(!scr && image->shared.get())
                scr = Script::find(image->shared.get(), cp);
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

bool Script::methods::scrReturn(void)
{
    unsigned mask = stack[frame].resmask;
    if(!pop())
        return scrExit();

    if(mask == stack[frame].resmask)
        return true;
    return false;
}

bool Script::methods::scrIndex(void)
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

bool Script::methods::scrRef(void)
{
    const char *cp = stack[frame].line->argv[0];
    cp = getContent(cp);
    skip();
    return true;
}

bool Script::methods::scrExpr(void)
{
    unsigned exdecimals = Script::decimals;
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

bool Script::methods::scrVar(void)
{
    unsigned index = 0;
    Script::line_t *line = stack[frame].line;
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

bool Script::methods::scrConst(void)
{
    unsigned index = 0;
    Script::line_t *line = stack[frame].line;
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

bool Script::methods::scrError(void)
{
    char msg[65];
    unsigned index = 0;
    Script::line_t *line = stack[frame].line;
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

bool Script::methods::scrClear(void)
{
    unsigned index = 0;
    Script::line_t *line = stack[frame].line;
    const char *id;
    Script::symbol *sym;

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

bool Script::methods::scrAdd(void)
{
    unsigned index = 1;
    Script::line_t *line = stack[frame].line;
    Script::symbol *sym = find(line->argv[0]);
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

bool Script::methods::scrPack(void)
{
    unsigned index = 1;
    Script::line_t *line = stack[frame].line;
    Script::symbol *sym = find(line->argv[0]);
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

bool Script::methods::scrPush(void)
{
    Script::line_t *line = stack[frame].line;
    Script::symbol *sym = createSymbol(line->argv[0]);
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

bool Script::methods::scrSet(void)
{
    unsigned index = 1;
    Script::line_t *line = stack[frame].line;
    Script::symbol *sym = createSymbol(line->argv[0]);
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

