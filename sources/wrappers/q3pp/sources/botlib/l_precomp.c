/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// #define QUAKEC

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_log.h"

// #define DEBUG_EVAL

#define MAX_DEFINEPARMS 128

#define DEFINEHASHING   1

// directive name with parse function
typedef struct directive_s
{
    const char* name;
    int32       (*func)(source_t* source);
} directive_t;

#define DEFINEHASHSIZE  1024

#define TOKEN_HEAP_SIZE 4096

int32 numtokens;

// list with global defines added to every source loaded
define_t* globaldefines;

void QDECL SourceError(source_t* source, const char* str, ...)
{
    char    text[1024];
    va_list ap;

    va_start(ap, str);
    vsprintf(text, str, ap);
    va_end(ap);
    botimport.Print(PRT_ERROR, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

void QDECL SourceWarning(source_t* source, const char* str, ...)
{
    char    text[1024];
    va_list ap;

    va_start(ap, str);
    vsprintf(text, str, ap);
    va_end(ap);
    botimport.Print(PRT_WARNING, "file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

void PC_PushIndent(source_t* source, int32 type, int32 skip)
{
    indent_t* indent;

    indent = (indent_t*) GetMemory(sizeof(indent_t));
    indent->type = type;
    indent->script = source->scriptstack;
    indent->skip = (skip != 0);
    source->skip += indent->skip;
    indent->next = source->indentstack;
    source->indentstack = indent;
}

void PC_PopIndent(source_t* source, int32* type, int32* skip)
{
    indent_t* indent;

    *type = 0;
    *skip = 0;

    indent = source->indentstack;
    if (!indent) {
        return;
    }

    // must be an indent from the current script
    if (source->indentstack->script != source->scriptstack) {
        return;
    }

    *type = indent->type;
    *skip = indent->skip;
    source->indentstack = source->indentstack->next;
    source->skip -= indent->skip;
    FreeMemory(indent);
}

void PC_PushScript(source_t* source, script_t* script)
{
    script_t* s;

    for (s = source->scriptstack; s; s = s->next) {
        if (!Q_stricmp(s->filename, script->filename)) {
            SourceError(source, "%s recursively included", script->filename);
            return;
        }
    }
    // push the script on the script stack
    script->next = source->scriptstack;
    source->scriptstack = script;
}

void PC_InitTokenHeap()
{
}

token_t* PC_CopyToken(token_t* token)
{
    token_t* t;

    //    t = (token_t *) malloc(sizeof(token_t));
    t = (token_t*) GetMemory(sizeof(token_t));
    //    t = freetokens;
    if (!t) {
        Com_Error(ERR_FATAL, "out of token space\n");
        return NULL;
    }
    //    freetokens = freetokens->next;
    Com_Memcpy(t, token, sizeof(token_t));
    t->next = NULL;
    numtokens++;
    return t;
}

void PC_FreeToken(token_t* token)
{
    FreeMemory(token);
    numtokens--;
}

int32 PC_ReadSourceToken(source_t* source, token_t* token)
{
    token_t*  t;
    script_t* script;
    int32     type, skip;

    // if there's no token already available
    while (!source->tokens) {
        // if there's a token to read from the script
        if (PS_ReadToken(source->scriptstack, token)) {
            return true;
        }
        // if at the end of the script
        if (EndOfScript(source->scriptstack)) {
            // remove all indents of the script
            while (source->indentstack && source->indentstack->script == source->scriptstack) {
                SourceWarning(source, "missing #endif");
                PC_PopIndent(source, &type, &skip);
            }
        }
        // if this was the initial script
        if (!source->scriptstack->next) {
            return false;
        }
        // remove the script and return to the last one
        script = source->scriptstack;
        source->scriptstack = source->scriptstack->next;
        FreeScript(script);
    }
    // copy the already available token
    Com_Memcpy(token, source->tokens, sizeof(token_t));
    // free the read token
    t = source->tokens;
    source->tokens = source->tokens->next;
    PC_FreeToken(t);
    return true;
}

int32 PC_UnreadSourceToken(source_t* source, token_t* token)
{
    token_t* t;

    t = PC_CopyToken(token);
    t->next = source->tokens;
    source->tokens = t;
    return true;
}

int32 PC_ReadDefineParms(source_t* source, define_t* define, token_t** parms, int32 maxparms)
{
    token_t token, *t, *last;
    int32   i, done, lastcomma, numparms, indent;

    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "define %s missing parms", define->name);
        return false;
    }
    //
    if (define->numparms > maxparms) {
        SourceError(source, "define with more than %d parameters", maxparms);
        return false;
    }
    //
    for (i = 0; i < define->numparms; i++) {
        parms[i] = NULL;
    }
    // if no leading "("
    if (strcmp(token.string, "(")) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "define %s missing parms", define->name);
        return false;
    }
    // read the define parameters
    for (done = 0, numparms = 0, indent = 0; !done;) {
        if (numparms >= maxparms) {
            SourceError(source, "define %s with too many parms", define->name);
            return false;
        }
        if (numparms >= define->numparms) {
            SourceWarning(source, "define %s has too many parms", define->name);
            return false;
        }
        parms[numparms] = NULL;
        lastcomma = 1;
        last = NULL;
        while (!done) {
            //
            if (!PC_ReadSourceToken(source, &token)) {
                SourceError(source, "define %s incomplete", define->name);
                return false;
            }
            //
            if (!strcmp(token.string, ",")) {
                if (indent <= 0) {
                    if (lastcomma) {
                        SourceWarning(source, "too many comma's");
                    }
                    lastcomma = 1;
                    break;
                }
            }
            lastcomma = 0;
            //
            if (!strcmp(token.string, "(")) {
                indent++;
                continue;
            } else if (!strcmp(token.string, ")")) {
                if (--indent <= 0) {
                    if (!parms[define->numparms - 1]) {
                        SourceWarning(source, "too few define parms");
                    }
                    done = 1;
                    break;
                }
            }
            //
            if (numparms < define->numparms) {
                //
                t = PC_CopyToken(&token);
                t->next = NULL;
                if (last) {
                    last->next = t;
                } else {
                    parms[numparms] = t;
                }
                last = t;
            }
        }
        numparms++;
    }
    return true;
}

int32 PC_StringizeTokens(token_t* tokens, token_t* token)
{
    token_t* t;

    token->type = TT_STRING;
    token->whitespace_p = NULL;
    token->endwhitespace_p = NULL;
    token->string[0] = '\0';
    strcat(token->string, "\"");
    for (t = tokens; t; t = t->next) {
        strncat(token->string, t->string, MAX_TOKEN - strlen(token->string));
    }
    strncat(token->string, "\"", MAX_TOKEN - strlen(token->string));
    return true;
}

int32 PC_MergeTokens(token_t* t1, token_t* t2)
{
    // merging of a name with a name or number
    if (t1->type == TT_NAME && (t2->type == TT_NAME || t2->type == TT_NUMBER)) {
        strcat(t1->string, t2->string);
        return true;
    }
    // merging of two strings
    if (t1->type == TT_STRING && t2->type == TT_STRING) {
        // remove trailing double quote
        t1->string[strlen(t1->string) - 1] = '\0';
        // concat without leading double quote
        strcat(t1->string, &t2->string[1]);
        return true;
    }
    // FIXME: merging of two number of the same sub type
    return false;
}

#if DEFINEHASHING

int32 PC_NameHash(char* name)
{
    int32 register hash, i;

    hash = 0;
    for (i = 0; name[i] != '\0'; i++) {
        hash += name[i] * (119 + i);
    }
    hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (DEFINEHASHSIZE - 1);
    return hash;
}

void PC_AddDefineToHash(define_t* define, define_t** definehash)
{
    int32 hash;

    hash = PC_NameHash(define->name);
    define->hashnext = definehash[hash];
    definehash[hash] = define;
}

define_t* PC_FindHashedDefine(define_t** definehash, char* name)
{
    define_t* d;
    int32     hash;

    hash = PC_NameHash(name);
    for (d = definehash[hash]; d; d = d->hashnext) {
        if (!strcmp(d->name, name)) {
            return d;
        }
    }
    return NULL;
}
#endif // DEFINEHASHING

define_t* PC_FindDefine(define_t* defines, char* name)
{
    define_t* d;

    for (d = defines; d; d = d->next) {
        if (!strcmp(d->name, name)) {
            return d;
        }
    }
    return NULL;
}

int32 PC_FindDefineParm(define_t* define, char* name)
{
    token_t* p;
    int32    i;

    i = 0;
    for (p = define->parms; p; p = p->next) {
        if (!strcmp(p->string, name)) {
            return i;
        }
        i++;
    }
    return -1;
}

void PC_FreeDefine(define_t* define)
{
    token_t *t, *next;

    // free the define parameters
    for (t = define->parms; t; t = next) {
        next = t->next;
        PC_FreeToken(t);
    }
    // free the define tokens
    for (t = define->tokens; t; t = next) {
        next = t->next;
        PC_FreeToken(t);
    }
    // free the define
    FreeMemory(define);
}

int32 PC_ExpandBuiltinDefine(source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken)
{
    token_t* token;
    time_t   t;
    char*    curtime;

    token = PC_CopyToken(deftoken);
    switch (define->builtin) {
    case BUILTIN_LINE: {
        sprintf(token->string, "%d", deftoken->line);
#ifdef NUMBERVALUE
        token->intvalue = deftoken->line;
        token->floatvalue = deftoken->line;
#endif // NUMBERVALUE
        token->type = TT_NUMBER;
        token->subtype = TT_DECIMAL | TT_INTEGER;
        *firsttoken = token;
        *lasttoken = token;
        break;
    }
    case BUILTIN_FILE: {
        strcpy(token->string, source->scriptstack->filename);
        token->type = TT_NAME;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;
    }
    case BUILTIN_DATE: {
        t = time(NULL);
        curtime = ctime(&t);
        strcpy(token->string, "\"");
        strncat(token->string, curtime + 4, 7);
        strncat(token->string + 7, curtime + 20, 4);
        strcat(token->string, "\"");
        free(curtime);
        token->type = TT_NAME;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;
    }
    case BUILTIN_TIME: {
        t = time(NULL);
        curtime = ctime(&t);
        strcpy(token->string, "\"");
        strncat(token->string, curtime + 11, 8);
        strcat(token->string, "\"");
        free(curtime);
        token->type = TT_NAME;
        token->subtype = strlen(token->string);
        *firsttoken = token;
        *lasttoken = token;
        break;
    }
    case BUILTIN_STDC:
    default: {
        *firsttoken = NULL;
        *lasttoken = NULL;
        break;
    }
    }
    return true;
}

int32 PC_ExpandDefine(source_t* source, token_t* deftoken, define_t* define, token_t** firsttoken, token_t** lasttoken)
{
    token_t *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
    token_t *t1, *t2, *first, *last, *nextpt, token;
    int32    parmnum, i;

    // if it is a builtin define
    if (define->builtin) {
        return PC_ExpandBuiltinDefine(source, deftoken, define, firsttoken, lasttoken);
    }
    // if the define has parameters
    if (define->numparms) {
        if (!PC_ReadDefineParms(source, define, parms, MAX_DEFINEPARMS)) {
            return false;
        }
#ifdef DEBUG_EVAL
        for (i = 0; i < define->numparms; i++) {
            Log_Write("define parms %d:", i);
            for (pt = parms[i]; pt; pt = pt->next) {
                Log_Write("%s", pt->string);
            }
        }
#endif // DEBUG_EVAL
    }
    // empty list at first
    first = NULL;
    last = NULL;
    // create a list with tokens of the expanded define
    for (dt = define->tokens; dt; dt = dt->next) {
        parmnum = -1;
        // if the token is a name, it could be a define parameter
        if (dt->type == TT_NAME) {
            parmnum = PC_FindDefineParm(define, dt->string);
        }
        // if it is a define parameter
        if (parmnum >= 0) {
            for (pt = parms[parmnum]; pt; pt = pt->next) {
                t = PC_CopyToken(pt);
                // add the token to the list
                t->next = NULL;
                if (last) {
                    last->next = t;
                } else {
                    first = t;
                }
                last = t;
            }
        } else {
            // if stringizing operator
            if (dt->string[0] == '#' && dt->string[1] == '\0') {
                // the stringizing operator must be followed by a define parameter
                if (dt->next) {
                    parmnum = PC_FindDefineParm(define, dt->next->string);
                } else {
                    parmnum = -1;
                }
                //
                if (parmnum >= 0) {
                    // step over the stringizing operator
                    dt = dt->next;
                    // stringize the define parameter tokens
                    if (!PC_StringizeTokens(parms[parmnum], &token)) {
                        SourceError(source, "can't stringize tokens");
                        return false;
                    }
                    t = PC_CopyToken(&token);
                } else {
                    SourceWarning(source, "stringizing operator without define parameter");
                    continue;
                }
            } else {
                t = PC_CopyToken(dt);
            }
            // add the token to the list
            t->next = NULL;
            if (last) {
                last->next = t;
            } else {
                first = t;
            }
            last = t;
        }
    }
    // check for the merging operator
    for (t = first; t;) {
        if (t->next) {
            // if the merging operator
            if (t->next->string[0] == '#' && t->next->string[1] == '#') {
                t1 = t;
                t2 = t->next->next;
                if (t2) {
                    if (!PC_MergeTokens(t1, t2)) {
                        SourceError(source, "can't merge %s with %s", t1->string, t2->string);
                        return false;
                    }
                    PC_FreeToken(t1->next);
                    t1->next = t2->next;
                    if (t2 == last) {
                        last = t1;
                    }
                    PC_FreeToken(t2);
                    continue;
                }
            }
        }
        t = t->next;
    }
    // store the first and last token of the list
    *firsttoken = first;
    *lasttoken = last;
    // free all the parameter tokens
    for (i = 0; i < define->numparms; i++) {
        for (pt = parms[i]; pt; pt = nextpt) {
            nextpt = pt->next;
            PC_FreeToken(pt);
        }
    }
    //
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ExpandDefineIntoSource(source_t* source, token_t* deftoken, define_t* define)
{
    token_t *firsttoken, *lasttoken;

    if (!PC_ExpandDefine(source, deftoken, define, &firsttoken, &lasttoken)) {
        return false;
    }

    if (firsttoken && lasttoken) {
        lasttoken->next = source->tokens;
        source->tokens = firsttoken;
        return true;
    }
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_ConvertPath(char* path)
{
    char* ptr;

    // remove double path seperators
    for (ptr = path; *ptr;) {
        if ((*ptr == '\\' || *ptr == '/') && (*(ptr + 1) == '\\' || *(ptr + 1) == '/')) {
            strcpy(ptr, ptr + 1);
        } else {
            ptr++;
        }
    }
    // set OS dependent path seperators
    for (ptr = path; *ptr;) {
        if (*ptr == '/' || *ptr == '\\') {
            *ptr = PATHSEPERATOR_CHAR;
        }
        ptr++;
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_include(source_t* source)
{
    script_t* script;
    token_t   token;
    char      path[MAX_PATH];

    if (source->skip > 0) {
        return true;
    }
    //
    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "#include without file name");
        return false;
    }
    if (token.linescrossed > 0) {
        SourceError(source, "#include without file name");
        return false;
    }
    if (token.type == TT_STRING) {
        StripDoubleQuotes(token.string);
        PC_ConvertPath(token.string);
        script = LoadScriptFile(token.string);
        if (!script) {
            strcpy(path, source->includepath);
            strcat(path, token.string);
            script = LoadScriptFile(path);
        }
    } else if (token.type == TT_PUNCTUATION && *token.string == '<') {
        strcpy(path, source->includepath);
        while (PC_ReadSourceToken(source, &token)) {
            if (token.linescrossed > 0) {
                PC_UnreadSourceToken(source, &token);
                break;
            }
            if (token.type == TT_PUNCTUATION && *token.string == '>') {
                break;
            }
            strncat(path, token.string, MAX_PATH);
        }
        if (*token.string != '>') {
            SourceWarning(source, "#include missing trailing >");
        }
        if (!strlen(path)) {
            SourceError(source, "#include without file name between < >");
            return false;
        }
        PC_ConvertPath(path);
        script = LoadScriptFile(path);
    } else {
        SourceError(source, "#include without file name");
        return false;
    }
    if (!script) {
        SourceError(source, "file %s not found", path);
        return false;
    }
    PC_PushScript(source, script);
    return true;
}
//============================================================================
// reads a token from the current line, continues reading on the next
// line only if a backslash '\' is encountered.
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ReadLine(source_t* source, token_t* token)
{
    int32 crossline;

    crossline = 0;
    do {
        if (!PC_ReadSourceToken(source, token)) {
            return false;
        }

        if (token->linescrossed > crossline) {
            PC_UnreadSourceToken(source, token);
            return false;
        }
        crossline = 1;
    } while (!strcmp(token->string, "\\"));
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_WhiteSpaceBeforeToken(token_t* token)
{
    return token->endwhitespace_p - token->whitespace_p > 0;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_ClearTokenWhiteSpace(token_t* token)
{
    token->whitespace_p = NULL;
    token->endwhitespace_p = NULL;
    token->linescrossed = 0;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_undef(source_t* source)
{
    token_t   token;
    define_t *define, *lastdefine;
    int32     hash;

    if (source->skip > 0) {
        return true;
    }
    //
    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "undef without name");
        return false;
    }
    if (token.type != TT_NAME) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name, found %s", token.string);
        return false;
    }
#if DEFINEHASHING

    hash = PC_NameHash(token.string);
    for (lastdefine = NULL, define = source->definehash[hash]; define; define = define->hashnext) {
        if (!strcmp(define->name, token.string)) {
            if (define->flags & DEFINE_FIXED) {
                SourceWarning(source, "can't undef %s", token.string);
            } else {
                if (lastdefine) {
                    lastdefine->hashnext = define->hashnext;
                } else {
                    source->definehash[hash] = define->hashnext;
                }
                PC_FreeDefine(define);
            }
            break;
        }
        lastdefine = define;
    }
#else  // DEFINEHASHING
    for (lastdefine = NULL, define = source->defines; define; define = define->next) {
        if (!strcmp(define->name, token.string)) {
            if (define->flags & DEFINE_FIXED) {
                SourceWarning(source, "can't undef %s", token.string);
            } else {
                if (lastdefine) {
                    lastdefine->next = define->next;
                } else {
                    source->defines = define->next;
                }
                PC_FreeDefine(define);
            }
            break;
        }
        lastdefine = define;
    }
#endif // DEFINEHASHING
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_define(source_t* source)
{
    token_t   token, *t, *last;
    define_t* define;

    if (source->skip > 0) {
        return true;
    }
    //
    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "#define without name");
        return false;
    }
    if (token.type != TT_NAME) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name after #define, found %s", token.string);
        return false;
    }
    // check if the define already exists
#if DEFINEHASHING
    define = PC_FindHashedDefine(source->definehash, token.string);
#else
    define = PC_FindDefine(source->defines, token.string);
#endif // DEFINEHASHING
    if (define) {
        if (define->flags & DEFINE_FIXED) {
            SourceError(source, "can't redefine %s", token.string);
            return false;
        }
        SourceWarning(source, "redefinition of %s", token.string);
        // unread the define name before executing the #undef directive
        PC_UnreadSourceToken(source, &token);
        if (!PC_Directive_undef(source)) {
            return false;
        }
        // if the define was not removed (define->flags & DEFINE_FIXED)
#if DEFINEHASHING
        define = PC_FindHashedDefine(source->definehash, token.string);
#else
        define = PC_FindDefine(source->defines, token.string);
#endif // DEFINEHASHING
    }
    // allocate define
    define = (define_t*) GetMemory(sizeof(define_t) + strlen(token.string) + 1);
    Com_Memset(define, 0, sizeof(define_t));
    define->name = (char*) define + sizeof(define_t);
    strcpy(define->name, token.string);
    // add the define to the source
#if DEFINEHASHING
    PC_AddDefineToHash(define, source->definehash);
#else  // DEFINEHASHING
    define->next = source->defines;
    source->defines = define;
#endif // DEFINEHASHING
    // if nothing is defined, just return
    if (!PC_ReadLine(source, &token)) {
        return true;
    }
    // if it is a define with parameters
    if (!PC_WhiteSpaceBeforeToken(&token) && !strcmp(token.string, "(")) {
        // read the define parameters
        last = NULL;
        if (!PC_CheckTokenString(source, ")")) {
            while (1) {
                if (!PC_ReadLine(source, &token)) {
                    SourceError(source, "expected define parameter");
                    return false;
                }
                // if it isn't a name
                if (token.type != TT_NAME) {
                    SourceError(source, "invalid define parameter");
                    return false;
                }
                //
                if (PC_FindDefineParm(define, token.string) >= 0) {
                    SourceError(source, "two the same define parameters");
                    return false;
                }
                // add the define parm
                t = PC_CopyToken(&token);
                PC_ClearTokenWhiteSpace(t);
                t->next = NULL;
                if (last) {
                    last->next = t;
                } else {
                    define->parms = t;
                }
                last = t;
                define->numparms++;
                // read next token
                if (!PC_ReadLine(source, &token)) {
                    SourceError(source, "define parameters not terminated");
                    return false;
                }
                //
                if (!strcmp(token.string, ")")) {
                    break;
                }
                // then it must be a comma
                if (strcmp(token.string, ",")) {
                    SourceError(source, "define not terminated");
                    return false;
                }
            }
        }
        if (!PC_ReadLine(source, &token)) {
            return true;
        }
    }
    // read the defined stuff
    last = NULL;
    do {
        t = PC_CopyToken(&token);
        if (t->type == TT_NAME && !strcmp(t->string, define->name)) {
            SourceError(source, "recursive define (removed recursion)");
            continue;
        }
        PC_ClearTokenWhiteSpace(t);
        t->next = NULL;
        if (last) {
            last->next = t;
        } else {
            define->tokens = t;
        }
        last = t;
    } while (PC_ReadLine(source, &token));
    //
    if (last) {
        // check for merge operators at the beginning or end
        if (!strcmp(define->tokens->string, "##") || !strcmp(last->string, "##")) {
            SourceError(source, "define with misplaced ##");
            return false;
        }
    }
    return true;
}
//============================================================================
// remove all globals defines
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_RemoveAllGlobalDefines()
{
    define_t* define;

    for (define = globaldefines; define; define = globaldefines) {
        globaldefines = globaldefines->next;
        PC_FreeDefine(define);
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
define_t* PC_CopyDefine(source_t* source, define_t* define)
{
    define_t* newdefine;
    token_t * token, *newtoken, *lasttoken;

    newdefine = (define_t*) GetMemory(sizeof(define_t) + strlen(define->name) + 1);
    // copy the define name
    newdefine->name = (char*) newdefine + sizeof(define_t);
    strcpy(newdefine->name, define->name);
    newdefine->flags = define->flags;
    newdefine->builtin = define->builtin;
    newdefine->numparms = define->numparms;
    // the define is not linked
    newdefine->next = NULL;
    newdefine->hashnext = NULL;
    // copy the define tokens
    newdefine->tokens = NULL;
    for (lasttoken = NULL, token = define->tokens; token; token = token->next) {
        newtoken = PC_CopyToken(token);
        newtoken->next = NULL;
        if (lasttoken) {
            lasttoken->next = newtoken;
        } else {
            newdefine->tokens = newtoken;
        }
        lasttoken = newtoken;
    }
    // copy the define parameters
    newdefine->parms = NULL;
    for (lasttoken = NULL, token = define->parms; token; token = token->next) {
        newtoken = PC_CopyToken(token);
        newtoken->next = NULL;
        if (lasttoken) {
            lasttoken->next = newtoken;
        } else {
            newdefine->parms = newtoken;
        }
        lasttoken = newtoken;
    }
    return newdefine;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_AddGlobalDefinesToSource(source_t* source)
{
    define_t *define, *newdefine;

    for (define = globaldefines; define; define = define->next) {
        newdefine = PC_CopyDefine(source, define);
#if DEFINEHASHING
        PC_AddDefineToHash(newdefine, source->definehash);
#else  // DEFINEHASHING
        newdefine->next = source->defines;
        source->defines = newdefine;
#endif // DEFINEHASHING
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_if_def(source_t* source, int32 type)
{
    token_t   token;
    define_t* d;
    int32     skip;

    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "#ifdef without name");
        return false;
    }
    if (token.type != TT_NAME) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "expected name after #ifdef, found %s", token.string);
        return false;
    }
#if DEFINEHASHING
    d = PC_FindHashedDefine(source->definehash, token.string);
#else
    d = PC_FindDefine(source->defines, token.string);
#endif // DEFINEHASHING
    skip = (type == INDENT_IFDEF) == (d == NULL);
    PC_PushIndent(source, type, skip);
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_ifdef(source_t* source)
{
    return PC_Directive_if_def(source, INDENT_IFDEF);
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_ifndef(source_t* source)
{
    return PC_Directive_if_def(source, INDENT_IFNDEF);
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_else(source_t* source)
{
    int32 type, skip;

    PC_PopIndent(source, &type, &skip);
    if (!type) {
        SourceError(source, "misplaced #else");
        return false;
    }
    if (type == INDENT_ELSE) {
        SourceError(source, "#else after #else");
        return false;
    }
    PC_PushIndent(source, INDENT_ELSE, !skip);
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_endif(source_t* source)
{
    int32 type, skip;

    PC_PopIndent(source, &type, &skip);
    if (!type) {
        SourceError(source, "misplaced #endif");
        return false;
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
typedef struct operator_s
{
    int32              operator_type;
    int32              priority;
    int32              parentheses;
    struct operator_s *prev, *next;
} operator_t;

typedef struct value_s
{
    int32           intvalue;
    double          floatvalue;
    int32           parentheses;
    struct value_s *prev, *next;
} value_t;

int32 PC_OperatorPriority(int32 op)
{
    switch (op) {
    case P_MUL:
        return 15;
    case P_DIV:
        return 15;
    case P_MOD:
        return 15;
    case P_ADD:
        return 14;
    case P_SUB:
        return 14;

    case P_LOGIC_AND:
        return 7;
    case P_LOGIC_OR:
        return 6;
    case P_LOGIC_GEQ:
        return 12;
    case P_LOGIC_LEQ:
        return 12;
    case P_LOGIC_EQ:
        return 11;
    case P_LOGIC_UNEQ:
        return 11;

    case P_LOGIC_NOT:
        return 16;
    case P_LOGIC_GREATER:
        return 12;
    case P_LOGIC_LESS:
        return 12;

    case P_RSHIFT:
        return 13;
    case P_LSHIFT:
        return 13;

    case P_BIN_AND:
        return 10;
    case P_BIN_OR:
        return 8;
    case P_BIN_XOR:
        return 9;
    case P_BIN_NOT:
        return 16;

    case P_COLON:
        return 5;
    case P_QUESTIONMARK:
        return 5;
    }
    return false;
}

// #define AllocValue()            GetClearedMemory(sizeof(value_t));
// #define FreeValue(val)        FreeMemory(val)
// #define AllocOperator(op)        op = (operator_t *) GetClearedMemory(sizeof(operator_t));
// #define FreeOperator(op)        FreeMemory(op);

#define MAX_VALUES    64
#define MAX_OPERATORS 64
#define AllocValue(val)                              \
    if (numvalues >= MAX_VALUES) {                   \
        SourceError(source, "out of value space\n"); \
        error = 1;                                   \
        break;                                       \
    } else                                           \
        val = &value_heap[numvalues++];
#define FreeValue(val)
//
#define AllocOperator(op)                               \
    if (numoperators >= MAX_OPERATORS) {                \
        SourceError(source, "out of operator space\n"); \
        error = 1;                                      \
        break;                                          \
    } else                                              \
        op = &operator_heap[numoperators++];
#define FreeOperator(op)

int32 PC_EvaluateTokens(source_t* source, token_t* tokens, int32* intvalue, double* floatvalue, int32 integer)
{
    operator_t *o, *firstoperator, *lastoperator;
    value_t *   v, *firstvalue, *lastvalue, *v1, *v2;
    token_t*    t;
    int32       brace = 0;
    int32       parentheses = 0;
    int32       error = 0;
    int32       lastwasvalue = 0;
    int32       negativevalue = 0;
    int32       questmarkintvalue = 0;
    double      questmarkfloatvalue = 0;
    int32       gotquestmarkvalue = false;
    int32       lastoperatortype = 0;
    //
    operator_t operator_heap[MAX_OPERATORS];
    int32      numoperators = 0;
    value_t    value_heap[MAX_VALUES];
    int32      numvalues = 0;

    firstoperator = lastoperator = NULL;
    firstvalue = lastvalue = NULL;
    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0;
    }
    for (t = tokens; t; t = t->next) {
        switch (t->type) {
        case TT_NAME: {
            if (lastwasvalue || negativevalue) {
                SourceError(source, "syntax error in #if/#elif");
                error = 1;
                break;
            }
            if (strcmp(t->string, "defined")) {
                SourceError(source, "undefined name %s in #if/#elif", t->string);
                error = 1;
                break;
            }
            t = t->next;
            if (!strcmp(t->string, "(")) {
                brace = true;
                t = t->next;
            }
            if (!t || t->type != TT_NAME) {
                SourceError(source, "defined without name in #if/#elif");
                error = 1;
                break;
            }
            // v = (value_t *) GetClearedMemory(sizeof(value_t));
            AllocValue(v);
#if DEFINEHASHING
            if (PC_FindHashedDefine(source->definehash, t->string))
#else
            if (PC_FindDefine(source->defines, t->string))
#endif // DEFINEHASHING
            {
                v->intvalue = 1;
                v->floatvalue = 1;
            } else {
                v->intvalue = 0;
                v->floatvalue = 0;
            }
            v->parentheses = parentheses;
            v->next = NULL;
            v->prev = lastvalue;
            if (lastvalue) {
                lastvalue->next = v;
            } else {
                firstvalue = v;
            }
            lastvalue = v;
            if (brace) {
                t = t->next;
                if (!t || strcmp(t->string, ")")) {
                    SourceError(source, "defined without ) in #if/#elif");
                    error = 1;
                    break;
                }
            }
            brace = false;
            // defined() creates a value
            lastwasvalue = 1;
            break;
        }
        case TT_NUMBER: {
            if (lastwasvalue) {
                SourceError(source, "syntax error in #if/#elif");
                error = 1;
                break;
            }
            // v = (value_t *) GetClearedMemory(sizeof(value_t));
            AllocValue(v);
            if (negativevalue) {
                v->intvalue = -(int32) t->intvalue;
                v->floatvalue = -t->floatvalue;
            } else {
                v->intvalue = t->intvalue;
                v->floatvalue = t->floatvalue;
            }
            v->parentheses = parentheses;
            v->next = NULL;
            v->prev = lastvalue;
            if (lastvalue) {
                lastvalue->next = v;
            } else {
                firstvalue = v;
            }
            lastvalue = v;
            // last token was a value
            lastwasvalue = 1;
            //
            negativevalue = 0;
            break;
        }
        case TT_PUNCTUATION: {
            if (negativevalue) {
                SourceError(source, "misplaced minus sign in #if/#elif");
                error = 1;
                break;
            }
            if (t->subtype == P_PARENTHESESOPEN) {
                parentheses++;
                break;
            } else if (t->subtype == P_PARENTHESESCLOSE) {
                parentheses--;
                if (parentheses < 0) {
                    SourceError(source, "too many ) in #if/#elsif");
                    error = 1;
                }
                break;
            }
            // check for invalid operators on floating point values
            if (!integer) {
                if (t->subtype == P_BIN_NOT || t->subtype == P_MOD || t->subtype == P_RSHIFT || t->subtype == P_LSHIFT || t->subtype == P_BIN_AND || t->subtype == P_BIN_OR || t->subtype == P_BIN_XOR) {
                    SourceError(source, "illigal operator %s on floating point operands\n", t->string);
                    error = 1;
                    break;
                }
            }
            switch (t->subtype) {
            case P_LOGIC_NOT:
            case P_BIN_NOT: {
                if (lastwasvalue) {
                    SourceError(source, "! or ~ after value in #if/#elif");
                    error = 1;
                    break;
                }
                break;
            }
            case P_INC:
            case P_DEC: {
                SourceError(source, "++ or -- used in #if/#elif");
                break;
            }
            case P_SUB: {
                if (!lastwasvalue) {
                    negativevalue = 1;
                    break;
                }
            }

            case P_MUL:
            case P_DIV:
            case P_MOD:
            case P_ADD:

            case P_LOGIC_AND:
            case P_LOGIC_OR:
            case P_LOGIC_GEQ:
            case P_LOGIC_LEQ:
            case P_LOGIC_EQ:
            case P_LOGIC_UNEQ:

            case P_LOGIC_GREATER:
            case P_LOGIC_LESS:

            case P_RSHIFT:
            case P_LSHIFT:

            case P_BIN_AND:
            case P_BIN_OR:
            case P_BIN_XOR:

            case P_COLON:
            case P_QUESTIONMARK: {
                if (!lastwasvalue) {
                    SourceError(source, "operator %s after operator in #if/#elif", t->string);
                    error = 1;
                    break;
                }
                break;
            }
            default: {
                SourceError(source, "invalid operator %s in #if/#elif", t->string);
                error = 1;
                break;
            }
            }
            if (!error && !negativevalue) {
                // o = (operator_t *) GetClearedMemory(sizeof(operator_t));
                AllocOperator(o);
                o->operator_type = t->subtype;
                o->priority = PC_OperatorPriority(t->subtype);
                o->parentheses = parentheses;
                o->next = NULL;
                o->prev = lastoperator;
                if (lastoperator) {
                    lastoperator->next = o;
                } else {
                    firstoperator = o;
                }
                lastoperator = o;
                lastwasvalue = 0;
            }
            break;
        }
        default: {
            SourceError(source, "unknown %s in #if/#elif", t->string);
            error = 1;
            break;
        }
        }
        if (error) {
            break;
        }
    }
    if (!error) {
        if (!lastwasvalue) {
            SourceError(source, "trailing operator in #if/#elif");
            error = 1;
        } else if (parentheses) {
            SourceError(source, "too many ( in #if/#elif");
            error = 1;
        }
    }
    //
    gotquestmarkvalue = false;
    questmarkintvalue = 0;
    questmarkfloatvalue = 0;
    // while there are operators
    while (!error && firstoperator) {
        v = firstvalue;
        for (o = firstoperator; o->next; o = o->next) {
            // if the current operator is nested deeper in parentheses
            // than the next operator
            if (o->parentheses > o->next->parentheses) {
                break;
            }
            // if the current and next operator are nested equally deep in parentheses
            if (o->parentheses == o->next->parentheses) {
                // if the priority of the current operator is equal or higher
                // than the priority of the next operator
                if (o->priority >= o->next->priority) {
                    break;
                }
            }
            // if the arity of the operator isn't equal to 1
            if (o->operator_type != P_LOGIC_NOT && o->operator_type != P_BIN_NOT) {
                v = v->next;
            }
            // if there's no value or no next value
            if (!v) {
                SourceError(source, "mising values in #if/#elif");
                error = 1;
                break;
            }
        }
        if (error) {
            break;
        }
        v1 = v;
        v2 = v->next;
#ifdef DEBUG_EVAL
        if (integer) {
            Log_Write("operator %s, value1 = %d", PunctuationFromNum(source->scriptstack, o->operator_type), v1->intvalue);
            if (v2) {
                Log_Write("value2 = %d", v2->intvalue);
            }
        } else {
            Log_Write("operator %s, value1 = %f", PunctuationFromNum(source->scriptstack, o->operator_type), v1->floatvalue);
            if (v2) {
                Log_Write("value2 = %f", v2->floatvalue);
            }
        }
#endif // DEBUG_EVAL
        switch (o->operator_type) {
        case P_LOGIC_NOT:
            v1->intvalue = !v1->intvalue;
            v1->floatvalue = !v1->floatvalue;
            break;
        case P_BIN_NOT:
            v1->intvalue = ~v1->intvalue;
            break;
        case P_MUL:
            v1->intvalue *= v2->intvalue;
            v1->floatvalue *= v2->floatvalue;
            break;
        case P_DIV:
            if (!v2->intvalue || !v2->floatvalue) {
                SourceError(source, "divide by zero in #if/#elif\n");
                error = 1;
                break;
            }
            v1->intvalue /= v2->intvalue;
            v1->floatvalue /= v2->floatvalue;
            break;
        case P_MOD:
            if (!v2->intvalue) {
                SourceError(source, "divide by zero in #if/#elif\n");
                error = 1;
                break;
            }
            v1->intvalue %= v2->intvalue;
            break;
        case P_ADD:
            v1->intvalue += v2->intvalue;
            v1->floatvalue += v2->floatvalue;
            break;
        case P_SUB:
            v1->intvalue -= v2->intvalue;
            v1->floatvalue -= v2->floatvalue;
            break;
        case P_LOGIC_AND:
            v1->intvalue = v1->intvalue && v2->intvalue;
            v1->floatvalue = v1->floatvalue && v2->floatvalue;
            break;
        case P_LOGIC_OR:
            v1->intvalue = v1->intvalue || v2->intvalue;
            v1->floatvalue = v1->floatvalue || v2->floatvalue;
            break;
        case P_LOGIC_GEQ:
            v1->intvalue = v1->intvalue >= v2->intvalue;
            v1->floatvalue = v1->floatvalue >= v2->floatvalue;
            break;
        case P_LOGIC_LEQ:
            v1->intvalue = v1->intvalue <= v2->intvalue;
            v1->floatvalue = v1->floatvalue <= v2->floatvalue;
            break;
        case P_LOGIC_EQ:
            v1->intvalue = v1->intvalue == v2->intvalue;
            v1->floatvalue = v1->floatvalue == v2->floatvalue;
            break;
        case P_LOGIC_UNEQ:
            v1->intvalue = v1->intvalue != v2->intvalue;
            v1->floatvalue = v1->floatvalue != v2->floatvalue;
            break;
        case P_LOGIC_GREATER:
            v1->intvalue = v1->intvalue > v2->intvalue;
            v1->floatvalue = v1->floatvalue > v2->floatvalue;
            break;
        case P_LOGIC_LESS:
            v1->intvalue = v1->intvalue < v2->intvalue;
            v1->floatvalue = v1->floatvalue < v2->floatvalue;
            break;
        case P_RSHIFT:
            v1->intvalue >>= v2->intvalue;
            break;
        case P_LSHIFT:
            v1->intvalue <<= v2->intvalue;
            break;
        case P_BIN_AND:
            v1->intvalue &= v2->intvalue;
            break;
        case P_BIN_OR:
            v1->intvalue |= v2->intvalue;
            break;
        case P_BIN_XOR:
            v1->intvalue ^= v2->intvalue;
            break;
        case P_COLON: {
            if (!gotquestmarkvalue) {
                SourceError(source, ": without ? in #if/#elif");
                error = 1;
                break;
            }
            if (integer) {
                if (!questmarkintvalue) {
                    v1->intvalue = v2->intvalue;
                }
            } else {
                if (!questmarkfloatvalue) {
                    v1->floatvalue = v2->floatvalue;
                }
            }
            gotquestmarkvalue = false;
            break;
        }
        case P_QUESTIONMARK: {
            if (gotquestmarkvalue) {
                SourceError(source, "? after ? in #if/#elif");
                error = 1;
                break;
            }
            questmarkintvalue = v1->intvalue;
            questmarkfloatvalue = v1->floatvalue;
            gotquestmarkvalue = true;
            break;
        }
        }
#ifdef DEBUG_EVAL
        if (integer) {
            Log_Write("result value = %d", v1->intvalue);
        } else {
            Log_Write("result value = %f", v1->floatvalue);
        }
#endif // DEBUG_EVAL
        if (error) {
            break;
        }
        lastoperatortype = o->operator_type;
        // if not an operator with arity 1
        if (o->operator_type != P_LOGIC_NOT && o->operator_type != P_BIN_NOT) {
            // remove the second value if not question mark operator
            if (o->operator_type != P_QUESTIONMARK) {
                v = v->next;
            }
            //
            if (v->prev) {
                v->prev->next = v->next;
            } else {
                firstvalue = v->next;
            }
            if (v->next) {
                v->next->prev = v->prev;
            } else {
                lastvalue = v->prev;
            }
            // FreeMemory(v);
            FreeValue(v);
        }
        // remove the operator
        if (o->prev) {
            o->prev->next = o->next;
        } else {
            firstoperator = o->next;
        }
        if (o->next) {
            o->next->prev = o->prev;
        } else {
            lastoperator = o->prev;
        }
        // FreeMemory(o);
        FreeOperator(o);
    }
    if (firstvalue) {
        if (intvalue) {
            *intvalue = firstvalue->intvalue;
        }
        if (floatvalue) {
            *floatvalue = firstvalue->floatvalue;
        }
    }
    for (o = firstoperator; o; o = lastoperator) {
        lastoperator = o->next;
        // FreeMemory(o);
        FreeOperator(o);
    }
    for (v = firstvalue; v; v = lastvalue) {
        lastvalue = v->next;
        // FreeMemory(v);
        FreeValue(v);
    }
    if (!error) {
        return true;
    }
    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0;
    }
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Evaluate(source_t* source, int32* intvalue, double* floatvalue, int32 integer)
{
    token_t   token, *firsttoken, *lasttoken;
    token_t * t, *nexttoken;
    define_t* define;
    int32     defined = false;

    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0;
    }
    //
    if (!PC_ReadLine(source, &token)) {
        SourceError(source, "no value after #if/#elif");
        return false;
    }
    firsttoken = NULL;
    lasttoken = NULL;
    do {
        // if the token is a name
        if (token.type == TT_NAME) {
            if (defined) {
                defined = false;
                t = PC_CopyToken(&token);
                t->next = NULL;
                if (lasttoken) {
                    lasttoken->next = t;
                } else {
                    firsttoken = t;
                }
                lasttoken = t;
            } else if (!strcmp(token.string, "defined")) {
                defined = true;
                t = PC_CopyToken(&token);
                t->next = NULL;
                if (lasttoken) {
                    lasttoken->next = t;
                } else {
                    firsttoken = t;
                }
                lasttoken = t;
            } else {
                // then it must be a define
#if DEFINEHASHING
                define = PC_FindHashedDefine(source->definehash, token.string);
#else
                define = PC_FindDefine(source->defines, token.string);
#endif // DEFINEHASHING
                if (!define) {
                    SourceError(source, "can't evaluate %s, not defined", token.string);
                    return false;
                }
                if (!PC_ExpandDefineIntoSource(source, &token, define)) {
                    return false;
                }
            }
        }
        // if the token is a number or a punctuation
        else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
            t = PC_CopyToken(&token);
            t->next = NULL;
            if (lasttoken) {
                lasttoken->next = t;
            } else {
                firsttoken = t;
            }
            lasttoken = t;
        } else // can't evaluate the token
        {
            SourceError(source, "can't evaluate %s", token.string);
            return false;
        }
    } while (PC_ReadLine(source, &token));
    //
    if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer)) {
        return false;
    }
    //
#ifdef DEBUG_EVAL
    Log_Write("eval:");
#endif // DEBUG_EVAL
    for (t = firsttoken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
        Log_Write(" %s", t->string);
#endif // DEBUG_EVAL
        nexttoken = t->next;
        PC_FreeToken(t);
    }
#ifdef DEBUG_EVAL
    if (integer) {
        Log_Write("eval result: %d", *intvalue);
    } else {
        Log_Write("eval result: %f", *floatvalue);
    }
#endif // DEBUG_EVAL
    //
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_DollarEvaluate(source_t* source, int32* intvalue, double* floatvalue, int32 integer)
{
    int32     indent, defined = false;
    token_t   token, *firsttoken, *lasttoken;
    token_t * t, *nexttoken;
    define_t* define;

    if (intvalue) {
        *intvalue = 0;
    }
    if (floatvalue) {
        *floatvalue = 0;
    }
    //
    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "no leading ( after $evalint/$evalfloat");
        return false;
    }
    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "nothing to evaluate");
        return false;
    }
    indent = 1;
    firsttoken = NULL;
    lasttoken = NULL;
    do {
        // if the token is a name
        if (token.type == TT_NAME) {
            if (defined) {
                defined = false;
                t = PC_CopyToken(&token);
                t->next = NULL;
                if (lasttoken) {
                    lasttoken->next = t;
                } else {
                    firsttoken = t;
                }
                lasttoken = t;
            } else if (!strcmp(token.string, "defined")) {
                defined = true;
                t = PC_CopyToken(&token);
                t->next = NULL;
                if (lasttoken) {
                    lasttoken->next = t;
                } else {
                    firsttoken = t;
                }
                lasttoken = t;
            } else {
                // then it must be a define
#if DEFINEHASHING
                define = PC_FindHashedDefine(source->definehash, token.string);
#else
                define = PC_FindDefine(source->defines, token.string);
#endif // DEFINEHASHING
                if (!define) {
                    SourceError(source, "can't evaluate %s, not defined", token.string);
                    return false;
                }
                if (!PC_ExpandDefineIntoSource(source, &token, define)) {
                    return false;
                }
            }
        }
        // if the token is a number or a punctuation
        else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
            if (*token.string == '(') {
                indent++;
            } else if (*token.string == ')') {
                indent--;
            }
            if (indent <= 0) {
                break;
            }
            t = PC_CopyToken(&token);
            t->next = NULL;
            if (lasttoken) {
                lasttoken->next = t;
            } else {
                firsttoken = t;
            }
            lasttoken = t;
        } else // can't evaluate the token
        {
            SourceError(source, "can't evaluate %s", token.string);
            return false;
        }
    } while (PC_ReadSourceToken(source, &token));
    //
    if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer)) {
        return false;
    }
    //
#ifdef DEBUG_EVAL
    Log_Write("$eval:");
#endif // DEBUG_EVAL
    for (t = firsttoken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
        Log_Write(" %s", t->string);
#endif // DEBUG_EVAL
        nexttoken = t->next;
        PC_FreeToken(t);
    }
#ifdef DEBUG_EVAL
    if (integer) {
        Log_Write("$eval result: %d", *intvalue);
    } else {
        Log_Write("$eval result: %f", *floatvalue);
    }
#endif // DEBUG_EVAL
    //
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_elif(source_t* source)
{
    int32 value;
    int32 type, skip;

    PC_PopIndent(source, &type, &skip);
    if (!type || type == INDENT_ELSE) {
        SourceError(source, "misplaced #elif");
        return false;
    }
    if (!PC_Evaluate(source, &value, NULL, true)) {
        return false;
    }
    skip = (value == 0);
    PC_PushIndent(source, INDENT_ELIF, skip);
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_if(source_t* source)
{
    int32 value;
    int32 skip;

    if (!PC_Evaluate(source, &value, NULL, true)) {
        return false;
    }
    skip = (value == 0);
    PC_PushIndent(source, INDENT_IF, skip);
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_line(source_t* source)
{
    SourceError(source, "#line directive not supported");
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_error(source_t* source)
{
    token_t token;

    strcpy(token.string, "");
    PC_ReadSourceToken(source, &token);
    SourceError(source, "#error directive: %s", token.string);
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_pragma(source_t* source)
{
    token_t token;

    SourceWarning(source, "#pragma directive not supported");
    while (PC_ReadLine(source, &token))
        ;
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void UnreadSignToken(source_t* source)
{
    token_t token;

    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    strcpy(token.string, "-");
    token.type = TT_PUNCTUATION;
    token.subtype = P_SUB;
    PC_UnreadSourceToken(source, &token);
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_eval(source_t* source)
{
    int32   value;
    token_t token;

    if (!PC_Evaluate(source, &value, NULL, true)) {
        return false;
    }
    //
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    sprintf(token.string, "%d", abs(value));
    token.type = TT_NUMBER;
    token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
    PC_UnreadSourceToken(source, &token);
    if (value < 0) {
        UnreadSignToken(source);
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_Directive_evalfloat(source_t* source)
{
    double  value;
    token_t token;

    if (!PC_Evaluate(source, NULL, &value, false)) {
        return false;
    }
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    sprintf(token.string, "%1.2f", fabs(value));
    token.type = TT_NUMBER;
    token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
    PC_UnreadSourceToken(source, &token);
    if (value < 0) {
        UnreadSignToken(source);
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
directive_t directives[20] =
    {
        {"if", PC_Directive_if},
        {"ifdef", PC_Directive_ifdef},
        {"ifndef", PC_Directive_ifndef},
        {"elif", PC_Directive_elif},
        {"else", PC_Directive_else},
        {"endif", PC_Directive_endif},
        {"include", PC_Directive_include},
        {"define", PC_Directive_define},
        {"undef", PC_Directive_undef},
        {"line", PC_Directive_line},
        {"error", PC_Directive_error},
        {"pragma", PC_Directive_pragma},
        {"eval", PC_Directive_eval},
        {"evalfloat", PC_Directive_evalfloat},
        {NULL, NULL}
};

int32 PC_ReadDirective(source_t* source)
{
    token_t token;
    int32   i;

    // read the directive name
    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "found # without name");
        return false;
    }
    // directive name must be on the same line
    if (token.linescrossed > 0) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "found # at end of line");
        return false;
    }
    // if if is a name
    if (token.type == TT_NAME) {
        // find the precompiler directive
        for (i = 0; directives[i].name; i++) {
            if (!strcmp(directives[i].name, token.string)) {
                return directives[i].func(source);
            }
        }
    }
    SourceError(source, "unknown precompiler directive %s", token.string);
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_DollarDirective_evalint(source_t* source)
{
    int32   value;
    token_t token;

    if (!PC_DollarEvaluate(source, &value, NULL, true)) {
        return false;
    }
    //
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    sprintf(token.string, "%d", abs(value));
    token.type = TT_NUMBER;
    token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
    token.intvalue = value;
    token.floatvalue = value;
#endif // NUMBERVALUE
    PC_UnreadSourceToken(source, &token);
    if (value < 0) {
        UnreadSignToken(source);
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_DollarDirective_evalfloat(source_t* source)
{
    double  value;
    token_t token;

    if (!PC_DollarEvaluate(source, NULL, &value, false)) {
        return false;
    }
    token.line = source->scriptstack->line;
    token.whitespace_p = source->scriptstack->script_p;
    token.endwhitespace_p = source->scriptstack->script_p;
    token.linescrossed = 0;
    sprintf(token.string, "%1.2f", fabs(value));
    token.type = TT_NUMBER;
    token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
#ifdef NUMBERVALUE
    token.intvalue = (uint32) value;
    token.floatvalue = value;
#endif // NUMBERVALUE
    PC_UnreadSourceToken(source, &token);
    if (value < 0) {
        UnreadSignToken(source);
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
directive_t dollardirectives[20] =
    {
        {"evalint", PC_DollarDirective_evalint},
        {"evalfloat", PC_DollarDirective_evalfloat},
        {NULL, NULL}
};

int32 PC_ReadDollarDirective(source_t* source)
{
    token_t token;
    int32   i;

    // read the directive name
    if (!PC_ReadSourceToken(source, &token)) {
        SourceError(source, "found $ without name");
        return false;
    }
    // directive name must be on the same line
    if (token.linescrossed > 0) {
        PC_UnreadSourceToken(source, &token);
        SourceError(source, "found $ at end of line");
        return false;
    }
    // if if is a name
    if (token.type == TT_NAME) {
        // find the precompiler directive
        for (i = 0; dollardirectives[i].name; i++) {
            if (!strcmp(dollardirectives[i].name, token.string)) {
                return dollardirectives[i].func(source);
            }
        }
    }
    PC_UnreadSourceToken(source, &token);
    SourceError(source, "unknown precompiler directive %s", token.string);
    return false;
}

#ifdef QUAKEC
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 BuiltinFunction(source_t* source)
{
    token_t token;

    if (!PC_ReadSourceToken(source, &token)) {
        return false;
    }
    if (token.type == TT_NUMBER) {
        PC_UnreadSourceToken(source, &token);
        return true;
    } else {
        PC_UnreadSourceToken(source, &token);
        return false;
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 QuakeCMacro(source_t* source)
{
    int32   i;
    token_t token;

    if (!PC_ReadSourceToken(source, &token)) {
        return true;
    }
    if (token.type != TT_NAME) {
        PC_UnreadSourceToken(source, &token);
        return true;
    }
    // find the precompiler directive
    for (i = 0; dollardirectives[i].name; i++) {
        if (!strcmp(dollardirectives[i].name, token.string)) {
            PC_UnreadSourceToken(source, &token);
            return false;
        }
    }
    PC_UnreadSourceToken(source, &token);
    return true;
}
#endif // QUAKEC
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ReadToken(source_t* source, token_t* token)
{
    define_t* define;

    while (1) {
        if (!PC_ReadSourceToken(source, token)) {
            return false;
        }
        // check for precompiler directives
        if (token->type == TT_PUNCTUATION && *token->string == '#') {
#ifdef QUAKEC
            if (!BuiltinFunction(source))
#endif // QUAKC
            {
                // read the precompiler directive
                if (!PC_ReadDirective(source)) {
                    return false;
                }
                continue;
            }
        }
        if (token->type == TT_PUNCTUATION && *token->string == '$') {
#ifdef QUAKEC
            if (!QuakeCMacro(source))
#endif // QUAKEC
            {
                // read the precompiler directive
                if (!PC_ReadDollarDirective(source)) {
                    return false;
                }
                continue;
            }
        }
        // recursively concatenate strings that are behind each other still resolving defines
        if (token->type == TT_STRING) {
            token_t newtoken;
            if (PC_ReadToken(source, &newtoken)) {
                if (newtoken.type == TT_STRING) {
                    token->string[strlen(token->string) - 1] = '\0';
                    if (strlen(token->string) + strlen(newtoken.string + 1) + 1 >= MAX_TOKEN) {
                        SourceError(source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN);
                        return false;
                    }
                    strcat(token->string, newtoken.string + 1);
                } else {
                    PC_UnreadToken(source, &newtoken);
                }
            }
        }
        // if skipping source because of conditional compilation
        if (source->skip) {
            continue;
        }
        // if the token is a name
        if (token->type == TT_NAME) {
            // check if the name is a define macro
#if DEFINEHASHING
            define = PC_FindHashedDefine(source->definehash, token->string);
#else
            define = PC_FindDefine(source->defines, token->string);
#endif // DEFINEHASHING
       // if it is a define macro
            if (define) {
                // expand the defined macro
                if (!PC_ExpandDefineIntoSource(source, token, define)) {
                    return false;
                }
                continue;
            }
        }
        // copy token for unreading
        Com_Memcpy(&source->token, token, sizeof(token_t));
        // found a token
        return true;
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ExpectTokenString(source_t* source, const char* string)
{
    token_t token;

    if (!PC_ReadToken(source, &token)) {
        SourceError(source, "couldn't find expected %s", string);
        return false;
    }

    if (strcmp(token.string, string)) {
        SourceError(source, "expected %s, found %s", string, token.string);
        return false;
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ExpectTokenType(source_t* source, int32 type, int32 subtype, token_t* token)
{
    char str[MAX_TOKEN];

    if (!PC_ReadToken(source, token)) {
        SourceError(source, "couldn't read expected token");
        return false;
    }

    if (token->type != type) {
        strcpy(str, "");
        if (type == TT_STRING) {
            strcpy(str, "string");
        }
        if (type == TT_LITERAL) {
            strcpy(str, "literal");
        }
        if (type == TT_NUMBER) {
            strcpy(str, "number");
        }
        if (type == TT_NAME) {
            strcpy(str, "name");
        }
        if (type == TT_PUNCTUATION) {
            strcpy(str, "punctuation");
        }
        SourceError(source, "expected a %s, found %s", str, token->string);
        return false;
    }
    if (token->type == TT_NUMBER) {
        if ((token->subtype & subtype) != subtype) {
            if (subtype & TT_DECIMAL) {
                strcpy(str, "decimal");
            }
            if (subtype & TT_HEX) {
                strcpy(str, "hex");
            }
            if (subtype & TT_OCTAL) {
                strcpy(str, "octal");
            }
            if (subtype & TT_BINARY) {
                strcpy(str, "binary");
            }
            if (subtype & TT_LONG) {
                strcat(str, " long");
            }
            if (subtype & TT_UNSIGNED) {
                strcat(str, " unsigned");
            }
            if (subtype & TT_FLOAT) {
                strcat(str, " float");
            }
            if (subtype & TT_INTEGER) {
                strcat(str, " integer");
            }
            SourceError(source, "expected %s, found %s", str, token->string);
            return false;
        }
    } else if (token->type == TT_PUNCTUATION) {
        if (token->subtype != subtype) {
            SourceError(source, "found %s", token->string);
            return false;
        }
    }
    return true;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
int32 PC_ExpectAnyToken(source_t* source, token_t* token)
{
    if (!PC_ReadToken(source, token)) {
        SourceError(source, "couldn't read expected token");
        return false;
    } else {
        return true;
    }
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
bool PC_CheckTokenString(source_t* source, const char* string)
{
    token_t tok;

    if (!PC_ReadToken(source, &tok)) {
        return false;
    }
    // if the token is available
    if (!strcmp(tok.string, string)) {
        return true;
    }
    //
    PC_UnreadSourceToken(source, &tok);
    return false;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_UnreadLastToken(source_t* source)
{
    PC_UnreadSourceToken(source, &source->token);
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void PC_UnreadToken(source_t* source, token_t* token)
{
    PC_UnreadSourceToken(source, token);
}
//============================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//============================================================================
source_t* LoadSourceFile(const char* filename)
{
    source_t* source;
    script_t* script;

    PC_InitTokenHeap();

    script = LoadScriptFile(filename);
    if (!script) {
        return NULL;
    }

    script->next = NULL;

    source = (source_t*) GetMemory(sizeof(source_t));
    Com_Memset(source, 0, sizeof(source_t));

    strncpy(source->filename, filename, MAX_PATH);
    source->scriptstack = script;
    source->tokens = NULL;
    source->defines = NULL;
    source->indentstack = NULL;
    source->skip = 0;

#if DEFINEHASHING
    source->definehash = (define_t**) GetClearedMemory(DEFINEHASHSIZE * sizeof(define_t*));
#endif // DEFINEHASHING
    PC_AddGlobalDefinesToSource(source);
    return source;
}
//============================================================================
//
// Parameter:                -
// Returns:                    -
// Changes Globals:        -
//============================================================================
void FreeSource(source_t* source)
{
    script_t* script;
    token_t*  token;
    define_t* define;
    indent_t* indent;
    int32     i;

    // free all the scripts
    while (source->scriptstack) {
        script = source->scriptstack;
        source->scriptstack = source->scriptstack->next;
        FreeScript(script);
    }
    // free all the tokens
    while (source->tokens) {
        token = source->tokens;
        source->tokens = source->tokens->next;
        PC_FreeToken(token);
    }
#if DEFINEHASHING
    for (i = 0; i < DEFINEHASHSIZE; i++) {
        while (source->definehash[i]) {
            define = source->definehash[i];
            source->definehash[i] = source->definehash[i]->hashnext;
            PC_FreeDefine(define);
        }
    }
#else  // DEFINEHASHING
    // free all defines
    while (source->defines) {
        define = source->defines;
        source->defines = source->defines->next;
        PC_FreeDefine(define);
    }
#endif // DEFINEHASHING
    // free all indents
    while (source->indentstack) {
        indent = source->indentstack;
        source->indentstack = source->indentstack->next;
        FreeMemory(indent);
    }
#if DEFINEHASHING
    //
    if (source->definehash) {
        FreeMemory(source->definehash);
    }
#endif // DEFINEHASHING
    // free the source itself
    FreeMemory(source);
}
//============================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//============================================================================

#define MAX_SOURCEFILES 64

source_t* sourceFiles[MAX_SOURCEFILES];

//============================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//============================================================================
void PC_SetBaseFolder(const char* path)
{
    PS_SetBaseFolder(path);
}
//============================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//============================================================================
void PC_CheckOpenSourceHandles()
{
    int32 i;

    for (i = 1; i < MAX_SOURCEFILES; i++) {
        if (sourceFiles[i]) {
            botimport.Print(PRT_ERROR, "file %s still open in precompiler\n", sourceFiles[i]->scriptstack->filename);
        }
    }
}

