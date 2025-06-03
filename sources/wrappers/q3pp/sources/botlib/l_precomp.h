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
#pragma once

#include <game/q_shared.h>
#include "l_script.h"

#ifndef MAX_PATH
    #define MAX_PATH MAX_QPATH
#endif

#ifndef PATH_SEPERATORSTR
    #define PATHSEPERATOR_STR "\\"
#endif
#ifndef PATH_SEPERATORCHAR
    #define PATHSEPERATOR_CHAR '\\'
#endif

#if !defined(QDECL)
    #define QDECL
#endif


#define DEFINE_FIXED  0x0001

#define BUILTIN_LINE  1
#define BUILTIN_FILE  2
#define BUILTIN_DATE  3
#define BUILTIN_TIME  4
#define BUILTIN_STDC  5

#define INDENT_IF     0x0001
#define INDENT_ELSE   0x0002
#define INDENT_ELIF   0x0004
#define INDENT_IFDEF  0x0008
#define INDENT_IFNDEF 0x0010

// macro definitions
typedef struct define_s
{
    char*            name;     // define name
    int32            flags;    // define flags
    int32            builtin;  // > 0 if builtin define
    int32            numparms; // number of define parameters
    token_t*         parms;    // define parameters
    token_t*         tokens;   // macro tokens (possibly containing parm tokens)
    struct define_s* next;     // next defined macro in a list
    struct define_s* hashnext; // next define in the hash chain
} define_t;

// indents
// used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s
{
    int32            type;   // indent type
    int32            skip;   // true if skipping current indent
    script_t*        script; // script the indent was in
    struct indent_s* next;   // next indent on the indent stack
} indent_t;

// source file
typedef struct source_s
{
    char           filename[1024];    // file name of the script
    char           includepath[1024]; // path to include files
    punctuation_t* punctuations;      // punctuations to use
    script_t*      scriptstack;       // stack with scripts of the source
    token_t*       tokens;            // tokens to read first
    define_t*      defines;           // list with macro definitions
    define_t**     definehash;        // hash chain with defines
    indent_t*      indentstack;       // stack with indents
    int32          skip;              // > 0 if skipping conditional code
    token_t        token;             // last read token
} source_t;


// read a token from the source
int32 PC_ReadToken(source_t* source, token_t* token);
// expect a certain token
int32 PC_ExpectTokenString(source_t* source, const char* string);
// expect a certain token type
int32 PC_ExpectTokenType(source_t* source, int32 type, int32 subtype, token_t* token);
// expect a token
int32 PC_ExpectAnyToken(source_t* source, token_t* token);
// returns true when the token is available
bool PC_CheckTokenString(source_t* source, const char* string);
// unread the last token read from the script
void PC_UnreadLastToken(source_t* source);
// unread the given token
void PC_UnreadToken(source_t* source, token_t* token);
// read a token only if on the same line, lines are concatenated with a slash
int32 PC_ReadLine(source_t* source, token_t* token);
// returns true if there was a white space in front of the token
int32 PC_WhiteSpaceBeforeToken(token_t* token);
// remove all globals defines
void PC_RemoveAllGlobalDefines();
// set the base folder to load files from
void PC_SetBaseFolder(const char* path);
// load a source file
source_t* LoadSourceFile(const char* filename);
// free the given source
void FreeSource(source_t* source);
// print a source error
void QDECL SourceError(source_t* source, const char* str, ...);
// print a source warning
void QDECL SourceWarning(source_t* source, const char* str, ...);

//
void PC_CheckOpenSourceHandles();
