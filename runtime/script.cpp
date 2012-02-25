// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2011 David Sugar, Tycho Softworks.
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
#include <ctype.h>

using namespace BAYONNE_NAMESPACE;
using namespace UCOMMON_NAMESPACE;

static Script::keyword_t *keywords = NULL;
static unsigned long icounter = 0;

size_t Script::paging = 0;  // default page size used...
unsigned Script::sizing = 64;   // default symbol size is 64...
unsigned Script::indexing = 77; // hash size of symbol index
unsigned Script::stacking = 20; // max stack depth 20...
unsigned Script::stepping = 7;  // automatic stepping...
unsigned Script::decimals = 2;  // two decimal places...us default

static bool ideq(const char *id1, const char *id2)
{
    unsigned count = 0;

    if(!id1)
        id1 = "";

    if(!id2)
        id2 = "";

    while(id1[count] && id1[count] != ':') {
        if(id2[count] != id1[count])
            return false;
        ++count;
    }
    if(id2[count] && id2[count] != ':')
        return false;

    return true;
}
static bool iskeyword(const char *str)
{
    if(!str || !*str)
        return false;

    if(*str == '_')
        ++str;

    while(*str) {
        if(!isalpha(*str) && *str != '.')
            return false;
        ++str;
    }
    return true;
}

static bool isend(const char *str)
{
    if(!str || !*str)
        return true;

    if(!strncmp("%%", str, 2))
        return true;

    if(!strncmp("//", str, 2))
        return true;

    return false;
}

static bool preparse(char **tokens)
{
    if(!tokens || !*tokens)
        return true;

    while(*tokens && isspace(**tokens))
        ++*tokens;

    char *str = *tokens;
    if(isend(str)) {
        *str = 0;
        return true;
    }

    switch(*str) {
    case '=':
        if(isspace(str[1]) || (str[1] == '=' && isspace(str[2]))) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
#ifdef  HAVE_REGEX_H
    case '~':
        if(isspace(str[1])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
#endif
    case '>':
        if(eq(str, ">>%", 3)) {
            --*tokens;
            --str;
            str[0] = '$';
            str[3] = ':';
            return true;
        }
        if(isspace(str[1]) || (str[1] == '=' && isspace(str[2]))) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
    case '$':
        if(isspace(str[1])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        break;
    case '?':
        if(isspace(str[1])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
    case '<':
        if(eq(str, "<<%", 3)) {
            --*tokens;
            --str;
            str[0] = '$';
            str[3] = ':';
            return true;
        }
        if(isspace(str[1]) || (str[1] == '=' && isspace(str[2])) || (str[1] == '>' && isspace(str[2]))) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
    case '!':
        if(str[1] == '=' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        else if(str[1] == '?' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        else if(str[1] == '$' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        else if(str[1] == '~' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        if(isalnum(str[1]))
            break;
        return false;
    case '&':
        if(str[1] == '&' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        else if(isalnum(str[1])) {
            *str = '%';
            break;
        }
        return false;
    case '|':
        if(str[1] == '|' && isspace(str[2])) {
            --*tokens;
            **tokens = '?';
            return true;
        }
        return false;
    case '/':
    case '*':
        if(isspace(str[1]))
            break;
        if(str[1] == '=' && isspace(str[2]))
            break;
        return false;
    case ':':
        if(str[1] == '=' && isspace(str[2]))
            return true;
    case ',':
    case '`':
    case '(':
    case '[':
    case ')':
    case ']':
    case ';':
    case '_':
        return false;
    case '+':
        if(eq(str, "++%", 3)) {
            --*tokens;
            --str;
            str[0] = '$';
            str[3] = ':';
            return true;
        }
        if(isdigit(str[1]) || str[1] == '.') {
            ++*tokens;
            return true;
        }
        if(isspace(str[1]))
            break;
        if(str[1] == '=' && isspace(str[2]))
            break;
        return false;
    case '.':
        if(isdigit(str[1]))
            break;
        return false;
    case '-':
        if(eq(str, "--%", 3)) {
            --*tokens;
            --str;
            str[0] = '$';
            str[3] = ':';
            return true;
        }
        if(isalnum(str[1]) || str[1] == '.')
            return true;
        if(isspace(str[1]))
            break;
        if(str[1] == '=' && isspace(str[2]))
            break;
        return false;
    case '\'':
    case '\"':
    case '{':
        --str;
        --*tokens;
        str[0] = str[1];
        str[1] = '&';
        return true;
    }

    while(*str && (isalnum(*str) || *str == ':' || *str == '.'))
        ++str;

    // we flip xxx=... into =xxx objects...

    if(*str == '=') {
        --*tokens;
        **tokens = '=';
        *str = ' ';
    }

    return true;
}

Script::error::error(Script *img, unsigned line, const char *msg) :
OrderedObject(&img->errlist)
{
    errmsg = img->dup(msg);
    errline = line;
    filename = img->filename;
}

Script::Script() :
CountedObject(), memalloc()
{
    instance = icounter++;
    errors = 0;
    loop = 0;
    lines = 0;
    first = NULL;
    global = NULL;
    headers = NULL;
    scheduler = NULL;
    stack = (line_t **)alloc(sizeof(line_t *) * Script::stacking);
    scripts = (LinkedObject **)alloc(sizeof(LinkedObject *) * Script::indexing);
    memset(scripts, 0, sizeof(LinkedObject **) * Script::indexing);

    if(instance)
        shell::log(shell::INFO, "creating image instance %lu\n", instance);
}

Script::~Script()
{
    purge();
    shared = NULL;
    if(instance)
        shell::log(shell::INFO, "releasing image instance %lu\n", instance);
}

void Script::errlog(unsigned line, const char *fmt, ...)
{
    char text[65];
    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);

    ++errors;
    caddr_t mp = (caddr_t)alloc(sizeof(Script::error));
    new(mp) Script::error(this, line, text);
    va_end(args);
}

void Script::assign(Script::keyword_t *keyword)
{
    while(keyword && keyword->name) {
        keyword->next = keywords;
        keywords = keyword;
        ++keyword;
    }
}

Script::keyword_t *Script::find(const char *cmd)
{
    keyword_t *keyword = keywords;

    while(keyword != NULL) {
        if(eq(cmd, keyword->name))
            break;
        keyword = keyword->next;
    }
    return keyword;
}

void Script::init(void)
{
    static keyword_t keywords[] = {
        {"pause", (method_t)&methods::scrPause, (check_t)&checks::chkNop},
        {"nop", (method_t)&methods::scrNop, (check_t)&checks::chkNop},
        {"exit", (method_t)&methods::scrExit, (check_t)&checks::chkExit},
        {"return", (method_t)&methods::scrReturn, (check_t)&checks::chkExit},
        {"restart", (method_t)&methods::scrRestart, (check_t)&checks::chkExit},
        {"goto", (method_t)&methods::scrGoto, (check_t)&checks::chkGoto},
        {"gosub", (method_t)&methods::scrGosub, (check_t)&checks::chkGosub},
        {"var", (method_t)&methods::scrVar, (check_t)&checks::chkVar},
        {"const", (method_t)&methods::scrConst, (check_t)&checks::chkConst},
        {"error", (method_t)&methods::scrError, (check_t)&checks::chkError},
        {"clear", (method_t)&methods::scrClear, (check_t)&checks::chkClear},
        {"push", (method_t)&methods::scrPush, (check_t)&checks::chkPush},
        {"set", (method_t)&methods::scrSet, (check_t)&checks::chkSet},
        {"add", (method_t)&methods::scrAdd, (check_t)&checks::chkSet},
        {"pack", (method_t)&methods::scrPack, (check_t)&checks::chkPack},
        {"expand", (method_t)&methods::scrExpand, (check_t)&checks::chkExpand},
        {"do", (method_t)&methods::scrDo, (check_t)&checks::chkDo},
        {"until", (method_t)&methods::scrUntil, (check_t)&checks::chkUntil},
        {"break", (method_t)&methods::scrBreak, (check_t)&checks::chkBreak},
        {"continue", (method_t)&methods::scrContinue, (check_t)&checks::chkContinue},
        {"previous", (method_t)&methods::scrPrevious, (check_t)&checks::chkPrevious},
        {"repeat", (method_t)&methods::scrRepeat, (check_t)&checks::chkPrevious},
        {"index", (method_t)&methods::scrIndex, (check_t)&checks::chkIndex},
        {"loop", (method_t)&methods::scrLoop, (check_t)&checks::chkLoop},
        {"while", (method_t)&methods::scrWhile, (check_t)&checks::chkWhile},
        {"for", (method_t)&methods::scrForeach, (check_t)&checks::chkForeach},
        {"foreach", (method_t)&methods::scrForeach, (check_t)&checks::chkForeach},
        {"case", (method_t)&methods::scrCase, (check_t)&checks::chkCase},
        {"otherwise", (method_t)&methods::scrOtherwise, (check_t)&checks::chkOtherwise},
        {"endcase", (method_t)&methods::scrEndcase, (check_t)&checks::chkEndcase},
        {"if", (method_t)&methods::scrIf, (check_t)&checks::chkIf},
        {"elif", (method_t)&methods::scrElif, (check_t)&checks::chkElif},
        {"else", (method_t)&methods::scrElse, (check_t)&checks::chkElse},
        {"endif", (method_t)&methods::scrEndif, (check_t)&checks::chkEndif},
        {"expr", (method_t)&methods::scrExpr, (check_t)&checks::chkExpr},
        {"strict", (method_t)NULL, (check_t)&checks::chkStrict},
        {"apply", (method_t)NULL, (check_t)&checks::chkApply},
        {"ignore", (method_t)NULL, (check_t)&checks::chkIgnmask},
        {"_ifthen", (method_t)&methods::scrWhen, (check_t)&checks::chkWhen},
        {"_define", (method_t)&methods::scrDefine, (check_t)&checks::chkDefine},
        {"_invoke", (method_t)&methods::scrInvoke, (check_t)&checks::chkInvoke},
        {"_ref", (method_t)&methods::scrRef, (check_t)&checks::chkRef},
        {NULL}
    };

    static bool initial = false;

    if(!initial) {
        initial = true;
        assign(keywords);
    }
}

Script::header *Script::find(Script *img, const char *id)
{
    unsigned path = NamedObject::keyindex(id, Script::indexing);
    linked_pointer<header> hp = img->scripts[path];

    while(is(hp)) {
        if(eq(hp->name, id))
            break;

        hp.next();
    }
    return *hp;
}

Script *Script::compile(Script *merge, const char *fn, Script *cfg)
{
//  linked_pointer<script::strict> sp;

    static unsigned serial;

    char **argv = new char *[256];
    stringbuf<512> buffer;
    char localname[256];
    Script *img;
    charfile cf(fn, "r");
    header *scr = NULL;
    const char *name = "_init_";
    Script::event *current;
    Script::event *prior;
    char *tokens;
    const char *token = NULL;
    char *arg;
    unsigned lnum = 0;
    line_t *line, *last;
    bool section = false;
    unsigned argc;
    const char *err, *op;
    char *assigned;
    unsigned path;
    bool indented, formed;
    Script::header *invoke;
    bool define = false;
    bool when = false;
    bool label = false;
    unsigned pos;
    const char *cp;
    bool requires = false;
    char *ep;
    unsigned len;

    if(!is(cf)) {
        if(merge)
            return merge;
        return NULL;
    }

    if(merge)
        img = merge;
    else
        img = new Script();

    img->shared = cfg;
    img->filename = strrchr(fn, '/');
    if(!img->filename)
        img->filename = strrchr(fn, '\\');
    if(!img->filename)
        img->filename = strrchr(fn, ':');
    if(img->filename)
        ++img->filename;
    else
        img->filename = fn;

    img->filename = img->dup(img->filename);
    img->serial = ++serial;

initial:
    current = NULL;
    prior = NULL;
    last = NULL;

    if(img->first && eq(name, "_init_")) {
        scr = img->first;
        last = scr->first;
        while(last && last->next)
            last = last->next;
    }
    else {
        scr = (header *)img->alloc(sizeof(header));
        memset(scr, 0, sizeof(header));
        scr->name = img->dup(name);
        scr->file = img->dup(img->filename);
        scr->events = NULL;
        scr->first = NULL;
        scr->resmask = 0;
        scr->scoped = NULL;
        ep = (char *)strrchr(scr->file, '.');
        if(ep)
            *ep = 0;
    }

    keyword_t *keyword;
    img->loop = 0;
    char *temp;

    // we drop additional initializers during append
    if(!eq(name, "_init_") || !merge) {
        path = NamedObject::keyindex(name, Script::indexing);
        scr->enlist(&img->scripts[path]);
    }

    if(!img->first)
        img->first = scr;

    while(when || label || define || cf.readline(buffer)) {
        if(define) {
            indented = true;
            goto parse;
        }

        if(when) {
            indented = true;
            when = false;
            goto parse;
        }

        if(label) {
            label = false;
            indented = true;
            token = String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
            if(!token)
                continue;
            goto parse;
        }

        img->lines = ++lnum;
        img->thencheck = false;

        indented = false;
        tokens = buffer.c_mem();
        if(isspace(*tokens)) {
            while(isspace(*tokens))
                ++tokens;
            indented = true;
            if(!preparse(&tokens)) {
                img->errlog(lnum, "malformed line");
                continue;
            }
        }

        String::trim(buffer, " \t\r\n");

        // if empty line or comment, continue...
        if(isend(*buffer))
            continue;

        tokens = NULL;
        token = String::token(buffer.c_mem(), &tokens, " \t", "{}\'\'\"\"");

        if(eq(token, "endreq") || eq(token, "endrequires")) {
            if(indented) {
                img->errlog(lnum, "endreq cannot be indented");
                continue;
            }
            requires = false;
            continue;
        }
        else if(eq(token, "requires")) {
            if(indented) {
                img->errlog(lnum, "requires cannot be indented");
                continue;
            }
            requires = true;
            while(NULL != (token = String::token(NULL, &tokens, " \t", "{}\'\'\"\""))) {
                bool rev = false;
                if(*token == '!') {
                    rev = true;
                    ++token;
                }
                keyword = find(token);
                invoke = find(img, token);
                if(!invoke && is(img->shared))
                    invoke = find(*(img->shared), token);
                if(!rev && (keyword || invoke)) {
                    requires = false;
                    break;
                }
                if(rev && !keyword && !invoke) {
                    requires = false;
                    break;
                }
            }
            continue;
        }

        if(requires)
            continue;

        if(*token == '^' || *token == '-') {
            if(scr == img->first) {
                img->errlog(lnum, "events cannot be in init segment");
                continue;
            }
            current = (event *)img->alloc(sizeof(Script::event));
            if(!prior)
                prior = current;

            last = NULL;
            memset(current, 0, sizeof(event));
            if(*token == '^')
                current->enlist(&scr->events);
            else
                current->enlist(&scr->methods);
            current->name = (char *)img->dup(++token);
            current->first = NULL;
            continue;
        }

        if(*token == '@') {
            section = true;
            name = token;
            label = true;
            goto closure;
        }

        if(*token == ':') {
            section = true;
            snprintf(localname, sizeof(localname), "@%04x%s", serial, token);
            name = localname;
            label = true;
            goto closure;
        }

        if(eq(token, "template")) {
            if(section) {
                img->errlog(lnum, "templates must be before named sections");
                continue;
            }
            if(indented) {
                img->errlog(lnum, "templates cannot be indented");
                continue;
            }
            token = String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
            if(!token)
                img->errlog(lnum, "template must be named");
            name = token;
            goto closure;
        }

        if(eq(token, "define")) {
            if(section) {
                img->errlog(lnum, "defines must be before named sections");
                continue;
            }
            if(indented) {
                img->errlog(lnum, "define cannot be indented");
                continue;
            }
            token = String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
            keyword = find(token);
            if(keyword) {
                img->errlog(lnum, "cannot redefine existing command");
                continue;
            }
            define = true;
            name = token;
            goto closure;
        }
        if(!indented) {
            img->errlog(lnum, "unindented statement");
            continue;
        }

parse:
        assigned = NULL;
        op = NULL;
        if(*token == '%') {
            assigned = img->dup(token);
            op = String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
            if(op && (eq(op, ":=") || eq(op, "+=") || eq(op, "-=") || eq(op, "*=") || eq(op, "/=") || eq(op, "%=") || eq(op, "#=")))
                token = "expr";
            else if(op && (eq(op, "=") || eq(op, "==") || eq(op, "$="))) {
                op = NULL;
                token = "set";
            }
            else if(op && (eq(op, ".") || eq(op, ".="))) {
                op = NULL;
                token = "add";
            }
            else if(op && (eq(op, ",") || eq(op, ",="))) {
                op = NULL;
                token = "pack";
            }
            else {
                img->errlog(lnum, "invalid assignment");
                continue;
            }
        }
        else if(*token == '$') {
            assigned = img->dup(token);
            token = "_ref";
        }
        invoke = NULL;
        if(!iskeyword(token)) {
            img->errlog(lnum, "invalid keyword");
            continue;
        }

        if(define) {
            define = false;
            keyword = find("_define");
            if(!keyword) {
                img->errlog(lnum, "define unsupported");
                continue;
            }
        }
        else {
            keyword = find(token);
            if(!keyword && NULL != (invoke = find(img, token)))
                keyword = find("_invoke");
            if(!keyword && is(img->shared) && NULL != (invoke = find(*(img->shared), token)))
                keyword = find("_invoke");

            if(!keyword) {
                img->errlog(lnum, "unknown keyword \"%s\"", token);
                continue;
            }
        }

        // protects in backparse
        token = img->dup(token);

        line = (line_t *)img->alloc(sizeof(line_t));
        memset(line, 0, sizeof(line_t));
        line->next = NULL;
        line->lnum = lnum;
        line->mask = 0;
        if(invoke)
            line->sub = invoke;
        else
            line->cmd = token;
        line->loop = img->loop;
        line->method = keyword->method;

        formed = true;
        argc = 0;
        arg = NULL;

        if(assigned)
            argv[argc++] = assigned;

        if(op)
            argv[argc++] = img->dup(op);

        while(argc < 249 && formed) {
            if(!arg) {
                formed = preparse(&tokens);
                if(!formed)
                    break;
                arg = (char *)String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
            }
            if(!arg)
                break;

            if(eq(token, "if")) {
                if(eq(arg, "if")) {
                    formed = false;
                    break;
                }
                if(eq(arg, "then")) {
                    keyword = find("_ifthen");
                    line->method = keyword->method;
                    token = String::token(NULL, &tokens, " \t", "{}\'\'\"\"");
                    if(!token || !keyword || img->thencheck)
                        formed = false;
                    else
                        img->thencheck = when = true;
                    break;
                }
            }

            ep = strchr(arg, '<');
            if(*arg == '%' && ep) {
                len = strlen(arg) + 10;
                *(ep++) = 0;
                temp = (char *)img->alloc(len);
                String::set(temp, len, "$map/");
                String::add(temp, len, ep);
                ep = strchr(temp, '>');
                if(ep)
                    *ep = ':';
                String::add(temp, len, ++arg);
                argv[argc++] = temp;
                arg = NULL;
                continue;
            }

            ep = strchr(arg, '[');
            if(*arg == '%' && ep) {
                len = strlen(arg) + 10;
                *(ep++) = 0;
                temp = (char *)img->alloc(len);
                if(atoi(ep))
                    String::set(temp, len, "$offset/");
                else
                    String::set(temp, len, "$find/");
                String::add(temp, len, ep);
                ep = strchr(temp, ']');
                if(ep)
                    *ep = ':';
                String::add(temp, len, ++arg);
                argv[argc++] = temp;
                arg = NULL;
                continue;
            }

            ep = strchr(arg, '(');
            if(*arg == '%' && ep) {
                len = strlen(arg) + 10;
                *(ep++) = 0;
                temp = (char *)img->alloc(len);
                String::set(temp, len, "$index/");
                String::add(temp, len, ep);
                ep = strchr(temp, ')');
                if(ep)
                    *ep = ':';
                String::add(temp, len, ++arg);
                argv[argc++] = temp;
                arg = NULL;
                continue;
            }

            argv[argc++] = img->dup(arg);
            arg = NULL;
        }

        if(!formed) {
            img->errlog(lnum, "malformed statement or argument");
            continue;
        }

        line->argc = argc;
        argv[argc++] = NULL;
        line->argv = (char **)img->alloc(sizeof(char *) * argc);
        memcpy(line->argv, argv, sizeof(char *) * argc);

        err = (*(keyword->check))(img, scr, line);
        if(err) {
            img->errlog(lnum, "%s", err);
            continue;
        }

        pos = 0;
        while(img->isStrict() && pos < line->argc) {
            cp = line->argv[pos++];
            if(!Script::strict::find(img, scr, cp))
                img->errlog(lnum, "undefined symbol reference %s\n", cp);
        }

        // if compile-time only line, then no runtime method, so we skip...

        if(line->method == (method_t)NULL)
            continue;

        // after error checking...we may add starting line...

        if(last)
            last->next = line;
        else {
            if(current) {
                linked_pointer<event> ep = prior;
                while(is(ep) && *ep != current) {
                    ep->first = line;
                    ep.next();
                }
                current->first = line;
            }
            else
                scr->first = line;
        }
        last = line;
    }

closure:
    while(img->loop) {
        line = img->stack[--img->loop];
        img->errlog(line->lnum, "%s never completed loop", line->cmd);
    }

    keyword = find("_close");
    if(keyword) {
        err = (*(keyword->check))(img, scr, scr->first);
        if(err)
            img->errlog(lnum, "%s", err);
    }

//  sp = scr->scoped;
//  while(is(sp)) {
//      sp->put(stdout, scr->name);
//      sp.next();
//  }

    if(!cf.eof())
        goto initial;

    delete[] argv;
//  sp = img->global;
//  while(is(sp)) {
//      sp->put(stdout, "*");
//      sp.next();
//  }

    return img;
}

bool Script::isEvent(header *scr, const char *id)
{
    linked_pointer<event> ep = scr->events;

    while(is(ep)) {
        if(eq(ep->name, id))
            return true;
        ep.next();
    }
    return false;
}

bool Script::push(line_t *line)
{
    if(loop < stacking) {
        stack[loop++] = line;
        return true;
    }
    return false;
}

Script::method_t Script::pull(void)
{
    if(!loop)
        return NULL;

    return stack[--loop]->method;
}

Script::method_t Script::looping(void)
{
    if(!loop)
        return NULL;

    return stack[loop - 1]->method;
}

void Script::strict::createVar(Script* image, Script::header *scr, const char *id)
{
    assert(id && *id);
    assert(scr != NULL);
    assert(image != NULL);

    linked_pointer<Script::strict> sp;
    Script::strict *sym;

    if(*id == '%' || *id == '=' || *id == '$')
        ++id;

    if(!image->global)
        return;

    if(*(scr->name) != '@') {
        sp = scr->scoped;
        while(is(sp)) {
            if(ideq(sp->id, id))
                return;
            sp.next();
        }
        sym = (Script::strict *)image->zalloc(sizeof(Script::strict));
        sym->enlist(&scr->scoped);
        sym->id = id;
        return;
    }
    sp = image->global;
    while(is(sp)) {
        if(ideq(sp->id, id))
            return;
        sp.next();
    }
    sym = (Script::strict *)image->zalloc(sizeof(Script::strict));
    sym->enlist(&image->global);
    sym->id = id;
}

void Script::strict::createSym(Script* image, Script::header *scr, const char *id)
{
    assert(id && *id);
    assert(scr != NULL);
    assert(image != NULL);

    linked_pointer<Script::strict> sp;
    Script::strict *sym;

    if(*id == '%' || *id == '=' || *id == '$')
        ++id;

    if(scr && !image->global)
        return;

    if(*(scr->name) != '@') {
        sp = scr->scoped;
        while(is(sp)) {
            if(ideq(sp->id, id))
                return;
            sp.next();
        }
    }
    sp = image->global;
    while(is(sp)) {
        if(ideq(sp->id, id))
            return;
        sp.next();
    }
    sym = (Script::strict *)image->zalloc(sizeof(Script::strict));
    sym->enlist(&image->global);
    sym->id = id;
}

void Script::strict::createAny(Script* image, Script::header *scr, const char *id)
{
    assert(id && *id);
    assert(scr != NULL);
    assert(image != NULL);

    linked_pointer<Script::strict> sp;
    Script::strict *sym;

    if(*id == '%' || *id == '=' || *id == '$')
        ++id;

    if(scr && !image->global)
        return;

    if(*(scr->name) != '@') {
        sp = scr->scoped;
        while(is(sp)) {
            if(ideq(sp->id, id))
                return;
            sp.next();
        }
    }
    sp = image->global;
    while(is(sp)) {
        if(ideq(sp->id, id))
            return;
        sp.next();
    }
    sym = (Script::strict *)image->zalloc(sizeof(Script::strict));
    if(*(scr->name) != '@')
        sym->enlist(&scr->scoped);
    else
        sym->enlist(&image->global);
    sym->id = id;
}

void Script::strict::createGlobal(Script *image, const char *id)
{
    assert(id && *id);
    assert(image != NULL);

    linked_pointer<Script::strict> sp;
    Script::strict *sym;

    if(*id == '%' || *id == '=' || *id == '$')
        ++id;

    sp = image->global;
    while(is(sp)) {
        if(ideq(sp->id, id))
            return;
        sp.next();
    }
    sym = (Script::strict *)image->zalloc(sizeof(Script::strict));
    sym->enlist(&image->global);
    sym->id = id;
}

bool Script::strict::find(Script* image, Script::header *scr, const char *id)
{
    assert(id && *id);
    assert(scr != NULL);
    assert(image != NULL);

    char buf[64];
    const char *cp;
    char *ep;
    linked_pointer<Script::strict> sp;

    if(!image->global)
        return true;

    if(*id != '%' && *id != '$')
        return true;

    if(eq(id, "$map/", 5)) {
        buf[0] = '%';
        String::set(buf + 1, sizeof(buf) - 1, id + 5);
        ep = strchr(buf, ':');
        if(ep)
            *ep = 0;

        if(!find(image, scr, buf))
            return false;
    }

    if(*id == '$') {
        cp = strchr(id, ':');
        if(cp)
            id = ++cp;
        else
            ++id;
    }
    else
        ++id;

    if(*(scr->name) != '@') {
        sp = scr->scoped;
        while(is(sp)) {
            if(ideq(sp->id, id))
                return true;
            sp.next();
        }
    }

    sp = image->global;
    while(is(sp)) {
        if(ideq(sp->id, id))
            return true;
        sp.next();
    }
    return false;
}

void Script::strict::put(FILE *fp, const char *header)
{
    assert(fp != NULL);
    assert(id != NULL);

    const char *pid = id;
    if(header) {
        fputs(header, fp);
        fputc(':', fp);
    }
    while(*pid && *pid != ':')
        fputc(*(pid++), fp);
    fputc('\n', fp);
}

unsigned Script::count(const char *data)
{
    unsigned count = 0;
    char quote = 0;
    unsigned paren = 0;

    if(!data || !*data)
        return 0;

    ++count;
    while(*data) {
        if(*data == ',' && !quote && !paren)
            ++count;
        else if(*data == quote && !paren)
            quote = 0;
        else if(*data == '\"' && !paren)
            quote = *data;
        else if(*data == '(' && !quote)
            ++paren;
        else if(*data == ')' && !quote)
            --paren;
        ++data;
    }
    return count;
}

void Script::copy(const char *list, char *item, unsigned size)
{
    assert(size > 0);
    assert(item != NULL);

    bool lead = false;
    char quote = 0;
    char prev = 0;
    unsigned paren = 0;
    bool leadparen = false;

    if(!list || !*list || *list == ',') {
        *item = 0;
        return;
    }

    while(isspace(*list))
        ++list;

    if(*list == '(') {
        leadparen = true;
        ++paren;
        ++list;
    }
    else {
        if(*list == '\"' || *list == '\'') {
            quote = *(list++);
            lead = true;
        }
    }

    while(--size && *list) {
        if(paren == 1 && leadparen && *list == ')' && !quote)
            break;
        if(*list == '(')
            ++paren;
        else if(*list == ')')
            --paren;
        if(*list == ',' && !quote && !paren)
            break;
        if(*list == quote && lead)
            break;
        if(*list == quote) {
            *(item++) = quote;
            break;
        }
        if(!quote && *list == '\"' && prev == '=' && !paren)
            quote = *list;
        prev = *item;
        *(item++) = *(list++);
    }
    *item = 0;
}

unsigned Script::offset(const char *list, unsigned index)
{
    const char *cp = get(list, index);
    return cp - list;
}

const char *Script::get(const char *list, unsigned index)
{
    char quote = 0;
    unsigned paren = 0;

    if(!list || !*list)
        return NULL;

    if(!index)
        return list;

    while(*list && index) {
        if(*list == ',' && !quote && !paren) {
            --index;
            if(!index)
                return ++list;
        } else if(*list == quote)
            quote = 0;
        else if(*list == '(' && !quote)
            ++paren;
        else if(*list == ')' && !quote)
            --paren;
        else if(*list == '\"' && !paren)
            quote = *list;
        ++list;
    }
    return NULL;
}

char *Script::get(char *list, unsigned index)
{
    char quote = 0;
    unsigned paren = 0;

    if(!list || !*list)
        return NULL;

    if(!index)
        return list;

    while(*list && index) {
        if(*list == ',' && !quote && !paren) {
            --index;
            if(!index)
                return ++list;
        } else if(*list == quote)
            quote = 0;
        else if(*list == '(' && !quote)
            ++paren;
        else if(*list == ')' && !quote)
            --paren;
        else if(*list == '\"' && !paren)
            quote = *list;
        ++list;
    }
    return NULL;
}

