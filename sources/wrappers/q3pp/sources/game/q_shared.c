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
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

tavros::core::logger logger("q_shared");

float Com_Clamp(float min, float max, float value)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}


/*
============
COM_SkipPath
============
*/
char* COM_SkipPath(char* pathname)
{
    char* last;

    last = pathname;
    while (*pathname) {
        if (*pathname == '/') {
            last = pathname + 1;
        }
        pathname++;
    }
    return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension(const char* in, char* out)
{
    while (*in && *in != '.') {
        *out++ = *in++;
    }
    *out = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension(char* path, int32 maxSize, const char* extension)
{
    char  oldPath[MAX_QPATH];
    char* src;

    //
    // if path doesn't have a .EXT, append extension
    // (extension should include the .)
    //
    src = path + strlen(path) - 1;

    while (*src != '/' && src != path) {
        if (*src == '.') {
            return; // it has an extension
        }
        src--;
    }

    Q_strncpyz(oldPath, path, sizeof(oldPath));
    Com_sprintf(path, maxSize, "%s%s", oldPath, extension);
}

/*
============================================================================

PARSING

============================================================================
*/

static char  com_token[MAX_TOKEN_CHARS];
static int32 com_lines;

char* COM_Parse(char** data_p)
{
    return COM_ParseExt(data_p, true);
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is true then an empty
string will be returned if the next token is
a newline.
==============
*/
static char* SkipWhitespace(char* data, bool* hasNewLines)
{
    int32 c;

    while ((c = *data) <= ' ') {
        if (!c) {
            return NULL;
        }
        if (c == '\n') {
            com_lines++;
            *hasNewLines = true;
        }
        data++;
    }

    return data;
}

int32 COM_Compress(char* data_p)
{
    char *in, *out;
    int32 c;
    bool  newline = false, whitespace = false;

    in = out = data_p;
    if (in) {
        while ((c = *in) != 0) {
            // skip double slash comments
            if (c == '/' && in[1] == '/') {
                while (*in && *in != '\n') {
                    in++;
                }
                // skip /* */ comments
            } else if (c == '/' && in[1] == '*') {
                while (*in && (*in != '*' || in[1] != '/')) {
                    in++;
                }
                if (*in) {
                    in += 2;
                }
                // record when we hit a newline
            } else if (c == '\n' || c == '\r') {
                newline = true;
                in++;
                // record when we hit whitespace
            } else if (c == ' ' || c == '\t') {
                whitespace = true;
                in++;
                // an actual token
            } else {
                // if we have a pending newline, emit it (and it counts as whitespace)
                if (newline) {
                    *out++ = '\n';
                    newline = false;
                    whitespace = false;
                }
                if (whitespace) {
                    *out++ = ' ';
                    whitespace = false;
                }

                // copy quoted strings unmolested
                if (c == '"') {
                    *out++ = c;
                    in++;
                    while (1) {
                        c = *in;
                        if (c && c != '"') {
                            *out++ = c;
                            in++;
                        } else {
                            break;
                        }
                    }
                    if (c == '"') {
                        *out++ = c;
                        in++;
                    }
                } else {
                    *out = c;
                    out++;
                    in++;
                }
            }
        }
    }
    *out = 0;
    return out - data_p;
}

char* COM_ParseExt(char** data_p, bool allowLineBreaks)
{
    int32 c = 0, len;
    bool  hasNewLines = false;
    char* data;

    data = *data_p;
    len = 0;
    com_token[0] = 0;

    // make sure incoming data is valid
    if (!data) {
        *data_p = NULL;
        return com_token;
    }

    while (1) {
        // skip whitespace
        data = SkipWhitespace(data, &hasNewLines);
        if (!data) {
            *data_p = NULL;
            return com_token;
        }
        if (hasNewLines && !allowLineBreaks) {
            *data_p = data;
            return com_token;
        }

        c = *data;

        // skip double slash comments
        if (c == '/' && data[1] == '/') {
            data += 2;
            while (*data && *data != '\n') {
                data++;
            }
        }
        // skip /* */ comments
        else if (c == '/' && data[1] == '*') {
            data += 2;
            while (*data && (*data != '*' || data[1] != '/')) {
                data++;
            }
            if (*data) {
                data += 2;
            }
        } else {
            break;
        }
    }

    // handle quoted strings
    if (c == '\"') {
        data++;
        while (1) {
            c = *data++;
            if (c == '\"' || !c) {
                com_token[len] = 0;
                *data_p = (char*) data;
                return com_token;
            }
            if (len < MAX_TOKEN_CHARS) {
                com_token[len] = c;
                len++;
            }
        }
    }

    // parse a regular word
    do {
        if (len < MAX_TOKEN_CHARS) {
            com_token[len] = c;
            len++;
        }
        data++;
        c = *data;
        if (c == '\n') {
            com_lines++;
        }
    } while (c > 32);

    if (len == MAX_TOKEN_CHARS) {
        len = 0;
    }
    com_token[len] = 0;

    *data_p = (char*) data;
    return com_token;
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection(char** program)
{
    char* token;
    int32 depth;

    depth = 0;
    do {
        token = COM_ParseExt(program, true);
        if (token[1] == 0) {
            if (token[0] == '{') {
                depth++;
            } else if (token[0] == '}') {
                depth--;
            }
        }
    } while (depth && *program);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine(char** data)
{
    char* p;
    int32 c;

    p = *data;
    while ((c = *p++) != 0) {
        if (c == '\n') {
            com_lines++;
            break;
        }
    }

    *data = p;
}

/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int32 Q_isprint(int32 c)
{
    if (c >= 0x20 && c <= 0x7E) {
        return (1);
    }
    return (0);
}

int32 Q_islower(int32 c)
{
    if (c >= 'a' && c <= 'z') {
        return (1);
    }
    return (0);
}

int32 Q_isupper(int32 c)
{
    if (c >= 'A' && c <= 'Z') {
        return (1);
    }
    return (0);
}

int32 Q_isalpha(int32 c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return (1);
    }
    return (0);
}

char* Q_strrchr(const char* string, int32 c)
{
    char  cc = c;
    char* s;
    char* sp = (char*) 0;

    s = (char*) string;

    while (*s) {
        if (*s == cc) {
            sp = s;
        }
        s++;
    }
    if (cc == 0) {
        sp = s;
    }

    return sp;
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz(char* dest, const char* src, int32 destsize)
{
    // bk001129 - also NULL dest
    if (!dest) {
        Com_Error(ERR_FATAL, "Q_strncpyz: NULL dest");
    }
    if (!src) {
        Com_Error(ERR_FATAL, "Q_strncpyz: NULL src");
    }
    if (destsize < 1) {
        Com_Error(ERR_FATAL, "Q_strncpyz: destsize < 1");
    }

    strncpy(dest, src, destsize - 1);
    dest[destsize - 1] = 0;
}

int32 Q_stricmpn(const char* s1, const char* s2, int32 n)
{
    int32 c1, c2;

    // bk001129 - moved in 1.17 fix not in id codebase
    if (s1 == NULL) {
        if (s2 == NULL) {
            return 0;
        } else {
            return -1;
        }
    } else if (s2 == NULL) {
        return 1;
    }


    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--) {
            return 0; // strings are equal until end point
        }

        if (c1 != c2) {
            if (c1 >= 'a' && c1 <= 'z') {
                c1 -= ('a' - 'A');
            }
            if (c2 >= 'a' && c2 <= 'z') {
                c2 -= ('a' - 'A');
            }
            if (c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
        }
    } while (c1);

    return 0; // strings are equal
}

int32 Q_strncmp(const char* s1, const char* s2, int32 n)
{
    int32 c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--) {
            return 0; // strings are equal until end point
        }

        if (c1 != c2) {
            return c1 < c2 ? -1 : 1;
        }
    } while (c1);

    return 0; // strings are equal
}

int32 Q_stricmp(const char* s1, const char* s2)
{
    return (s1 && s2) ? Q_stricmpn(s1, s2, 99999) : -1;
}


char* Q_strlwr(char* s1)
{
    char* s;

    s = s1;
    while (*s) {
        *s = tolower(*s);
        s++;
    }
    return s1;
}

char* Q_strupr(char* s1)
{
    char* s;

    s = s1;
    while (*s) {
        *s = toupper(*s);
        s++;
    }
    return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat(char* dest, int32 size, const char* src)
{
    int32 l1;

    l1 = strlen(dest);
    if (l1 >= size) {
        Com_Error(ERR_FATAL, "Q_strcat: already overflowed");
    }
    Q_strncpyz(dest + l1, src, size - l1);
}


int32 Q_PrintStrlen(const char* string)
{
    int32       len;
    const char* p;

    if (!string) {
        return 0;
    }

    len = 0;
    p = string;
    while (*p) {
        if (Q_IsColorString(p)) {
            p += 2;
            continue;
        }
        p++;
        len++;
    }

    return len;
}


char* Q_CleanStr(char* string)
{
    char* d;
    char* s;
    int32 c;

    s = string;
    d = string;
    while ((c = *s) != 0) {
        if (Q_IsColorString(s)) {
            s++;
        } else if (c >= 0x20 && c <= 0x7E) {
            *d++ = c;
        }
        s++;
    }
    *d = '\0';

    return string;
}


void QDECL Com_sprintf(char* dest, int32 size, const char* fmt, ...)
{
    int32   len;
    va_list argptr;
    char    bigbuffer[32000];

    va_start(argptr, fmt);
    len = vsprintf(bigbuffer, fmt, argptr);
    va_end(argptr);
    if (len >= sizeof(bigbuffer)) {
        Com_Error(ERR_FATAL, "Com_sprintf: overflowed bigbuffer");
    }
    if (len >= size) {
        logger.error("Com_sprintf: overflow of %i in %i", len, size);
    }
    Q_strncpyz(dest, bigbuffer, size);
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char* QDECL va(const char* format, ...)
{
    va_list      argptr;
    static char  string[2][32000]; // in case va is called by nested functions
    static int32 index = 0;
    char*        buf;

    buf = string[index & 1];
    index++;

    va_start(argptr, format);
    vsprintf(buf, format, argptr);
    va_end(argptr);

    return buf;
}


/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char* Info_ValueForKey(const char* s, const char* key)
{
    char        pkey[BIG_INFO_KEY];
    static char value[2][BIG_INFO_VALUE]; // use two buffers so compares
                                          // work without stomping on each other
    static int32 valueindex = 0;
    char*        o;

    if (!s || !key) {
        return (char*) "";
    }

    if (strlen(s) >= BIG_INFO_STRING) {
        Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring");
    }

    valueindex ^= 1;
    if (*s == '\\') {
        s++;
    }
    while (1) {
        o = pkey;
        while (*s != '\\') {
            if (!*s) {
                return (char*) "";
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;

        o = value[valueindex];

        while (*s != '\\' && *s) {
            *o++ = *s++;
        }
        *o = 0;

        if (!Q_stricmp(key, pkey)) {
            return value[valueindex];
        }

        if (!*s) {
            break;
        }
        s++;
    }

    return (char*) "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair(const char** head, char* key, char* value)
{
    char*       o;
    const char* s;

    s = *head;

    if (*s == '\\') {
        s++;
    }
    key[0] = 0;
    value[0] = 0;

    o = key;
    while (*s != '\\') {
        if (!*s) {
            *o = 0;
            *head = s;
            return;
        }
        *o++ = *s++;
    }
    *o = 0;
    s++;

    o = value;
    while (*s != '\\' && *s) {
        *o++ = *s++;
    }
    *o = 0;

    *head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey(char* s, const char* key)
{
    char* start;
    char  pkey[MAX_INFO_KEY];
    char  value[MAX_INFO_VALUE];
    char* o;

    if (strlen(s) >= MAX_INFO_STRING) {
        Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring");
    }

    if (strchr(key, '\\')) {
        return;
    }

    while (1) {
        start = s;
        if (*s == '\\') {
            s++;
        }
        o = pkey;
        while (*s != '\\') {
            if (!*s) {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;

        o = value;
        while (*s != '\\' && *s) {
            if (!*s) {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;

        if (!strcmp(key, pkey)) {
            strcpy(start, s); // remove this part
            return;
        }

        if (!*s) {
            return;
        }
    }
}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big(char* s, const char* key)
{
    char* start;
    char  pkey[BIG_INFO_KEY];
    char  value[BIG_INFO_VALUE];
    char* o;

    if (strlen(s) >= BIG_INFO_STRING) {
        Com_Error(ERR_DROP, "Info_RemoveKey_Big: oversize infostring");
    }

    if (strchr(key, '\\')) {
        return;
    }

    while (1) {
        start = s;
        if (*s == '\\') {
            s++;
        }
        o = pkey;
        while (*s != '\\') {
            if (!*s) {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;
        s++;

        o = value;
        while (*s != '\\' && *s) {
            if (!*s) {
                return;
            }
            *o++ = *s++;
        }
        *o = 0;

        if (!strcmp(key, pkey)) {
            strcpy(start, s); // remove this part
            return;
        }

        if (!*s) {
            return;
        }
    }
}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
bool Info_Validate(const char* s)
{
    if (strchr(s, '\"')) {
        return false;
    }
    if (strchr(s, ';')) {
        return false;
    }
    return true;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey(char* s, const char* key, const char* value)
{
    char newi[MAX_INFO_STRING];

    if (strlen(s) >= MAX_INFO_STRING) {
        Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring");
    }

    if (strchr(key, '\\') || strchr(value, '\\')) {
        logger.warning("Can't use keys or values with a \\");
        return;
    }

    if (strchr(key, ';') || strchr(value, ';')) {
        logger.warning("Can't use keys or values with a semicolon");
        return;
    }

    if (strchr(key, '\"') || strchr(value, '\"')) {
        logger.warning("Can't use keys or values with a \"");
        return;
    }

    Info_RemoveKey(s, key);
    if (!value || !strlen(value)) {
        return;
    }

    Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

    if (strlen(newi) + strlen(s) > MAX_INFO_STRING) {
        logger.warning("Info string length exceeded");
        return;
    }

    strcat(newi, s);
    strcpy(s, newi);
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big(char* s, const char* key, const char* value)
{
    char newi[BIG_INFO_STRING];

    if (strlen(s) >= BIG_INFO_STRING) {
        Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring");
    }

    if (strchr(key, '\\') || strchr(value, '\\')) {
        logger.warning("Can't use keys or values with a \\");
        return;
    }

    if (strchr(key, ';') || strchr(value, ';')) {
        logger.warning("Can't use keys or values with a semicolon");
        return;
    }

    if (strchr(key, '\"') || strchr(value, '\"')) {
        logger.warning("Can't use keys or values with a \"");
        return;
    }

    Info_RemoveKey_Big(s, key);
    if (!value || !strlen(value)) {
        return;
    }

    Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

    if (strlen(newi) + strlen(s) > BIG_INFO_STRING) {
        logger.warning("BIG Info string length exceeded");
        return;
    }

    strcat(s, newi);
}


//====================================================================


