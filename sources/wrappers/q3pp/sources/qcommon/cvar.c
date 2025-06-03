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
// cvar.c -- dynamic variable tracking

#include "../game/q_shared.h"
#include "qcommon.h"

static tavros::core::logger logger("cvar");

cvar_t* cvar_vars;
cvar_t* cvar_cheats;
int32   cvar_modifiedFlags;

#define MAX_CVARS 1024
cvar_t cvar_indexes[MAX_CVARS];
int32  cvar_numIndexes;

#define FILE_HASH_SIZE 256
static cvar_t* hashTable[FILE_HASH_SIZE];

cvar_t* Cvar_Set2(const char* var_name, const char* value, bool force);

/*
================
return a hash value for the filename
================
*/
static int32 generateHashValue(const char* fname)
{
    int32 i;
    int32 hash;
    char  letter;

    hash = 0;
    i = 0;
    while (fname[i] != '\0') {
        letter = tolower(fname[i]);
        hash += (int32) (letter) * (i + 119);
        i++;
    }
    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

/*
============
Cvar_ValidateString
============
*/
static bool Cvar_ValidateString(const char* s)
{
    if (!s) {
        return false;
    }
    if (strchr(s, '\\')) {
        return false;
    }
    if (strchr(s, '\"')) {
        return false;
    }
    if (strchr(s, ';')) {
        return false;
    }
    return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t* Cvar_FindVar(const char* var_name)
{
    cvar_t* var;
    int32   hash;

    hash = generateHashValue(var_name);

    for (var = hashTable[hash]; var; var = var->hashNext) {
        if (!Q_stricmp(var_name, var->name)) {
            return var;
        }
    }

    return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue(const char* var_name)
{
    cvar_t* var;

    var = Cvar_FindVar(var_name);
    if (!var) {
        return 0;
    }
    return var->value;
}


/*
============
Cvar_VariableIntegerValue
============
*/
int32 Cvar_VariableIntegerValue(const char* var_name)
{
    cvar_t* var = Cvar_FindVar(var_name);
    return var ? var->integer : 0;
}


/*
============
Cvar_VariableString
============
*/
const char* Cvar_VariableString(const char* var_name)
{
    cvar_t* var;

    var = Cvar_FindVar(var_name);
    if (!var) {
        return "";
    }
    return var->string;
}


/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer(const char* var_name, char* buffer, int32 bufsize)
{
    cvar_t* var;

    var = Cvar_FindVar(var_name);
    if (!var) {
        *buffer = 0;
    } else {
        Q_strncpyz(buffer, var->string, bufsize);
    }
}


/*
============
Cvar_CommandCompletion
============
*/
void Cvar_CommandCompletion(void (*callback)(const char* s))
{
    cvar_t* cvar;

    for (cvar = cvar_vars; cvar; cvar = cvar->next) {
        callback(cvar->name);
    }
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t* Cvar_Get(const char* var_name, const char* var_value, int32 flags)
{
    cvar_t* var;
    int32   hash;

    if (!var_name || !var_value) {
        Com_Error(ERR_FATAL, "Cvar_Get: NULL parameter");
    }

    if (!Cvar_ValidateString(var_name)) {
        logger.info("invalid cvar name string: %s", var_name);
        var_name = "BADNAME";
    }

    var = Cvar_FindVar(var_name);
    if (var) {
        // if the C code is now specifying a variable that the user already
        // set a value for, take the new value as the reset value
        if ((var->flags & CVAR_USER_CREATED) && !(flags & CVAR_USER_CREATED)
            && var_value[0]) {
            var->flags &= ~CVAR_USER_CREATED;
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);

            // ZOID--needs to be set so that cvars the game sets as
            // SERVERINFO get sent to clients
            cvar_modifiedFlags |= flags;
        }

        var->flags |= flags;
        // only allow one non-empty reset string without a warning
        if (!var->resetString[0]) {
            // we don't have a reset string yet
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);
        } else if (var_value[0] && strcmp(var->resetString, var_value)) {
            logger.debug("Warning: cvar '%s' given initial values: '%s' and '%s'", var_name, var->resetString, var_value);
        }
        // if we have a latched string, take that value now
        if (var->latchedString) {
            char* s;

            s = var->latchedString;
            var->latchedString = NULL; // otherwise cvar_set2 would free it
            Cvar_Set2(var_name, s, true);
            Z_Free(s);
        }

        // use a CVAR_SET for rom sets, get won't override
        return var;
    }

    //
    // allocate a new cvar
    //
    if (cvar_numIndexes >= MAX_CVARS) {
        Com_Error(ERR_FATAL, "MAX_CVARS");
    }
    var = &cvar_indexes[cvar_numIndexes];
    cvar_numIndexes++;
    var->name = CopyString(var_name);
    var->string = CopyString(var_value);
    var->modified = true;
    var->value = atof(var->string);
    var->integer = atoi(var->string);
    var->resetString = CopyString(var_value);

    // link the variable in
    var->next = cvar_vars;
    cvar_vars = var;

    var->flags = flags;

    hash = generateHashValue(var_name);
    var->hashNext = hashTable[hash];
    hashTable[hash] = var;

    return var;
}

/*
============
Cvar_Set2
============
*/
cvar_t* Cvar_Set2(const char* var_name, const char* value, bool force)
{
    cvar_t* var;

    logger.debug("Cvar_Set2: %s %s", var_name, value);

    if (!Cvar_ValidateString(var_name)) {
        logger.info("invalid cvar name string: %s", var_name);
        var_name = "BADNAME";
    }

    var = Cvar_FindVar(var_name);
    if (!var) {
        if (!value) {
            return NULL;
        }
        // create it
        if (!force) {
            return Cvar_Get(var_name, value, CVAR_USER_CREATED);
        } else {
            return Cvar_Get(var_name, value, 0);
        }
    }

    if (!value) {
        value = var->resetString;
    }

    if (!strcmp(value, var->string)) {
        return var;
    }
    // note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
    cvar_modifiedFlags |= var->flags;

    if (!force) {
        if (var->flags & CVAR_ROM) {
            logger.info("%s is read only.", var_name);
            return var;
        }

        if (var->flags & CVAR_INIT) {
            logger.info("%s is write protected.", var_name);
            return var;
        }

        if (var->flags & CVAR_LATCH) {
            if (var->latchedString) {
                if (strcmp(value, var->latchedString) == 0) {
                    return var;
                }
                Z_Free(var->latchedString);
            } else {
                if (strcmp(value, var->string) == 0) {
                    return var;
                }
            }

            logger.info("%s will be changed upon restarting.", var_name);
            var->latchedString = CopyString(value);
            var->modified = true;
            return var;
        }

        if ((var->flags & CVAR_CHEAT) && !cvar_cheats->integer) {
            logger.info("%s is cheat protected.", var_name);
            return var;
        }

    } else {
        if (var->latchedString) {
            Z_Free(var->latchedString);
            var->latchedString = NULL;
        }
    }

    if (!strcmp(value, var->string)) {
        return var; // not changed
    }

    var->modified = true;

    Z_Free(var->string); // free the old value string

    var->string = CopyString(value);
    var->value = atof(var->string);
    var->integer = atoi(var->string);

    return var;
}

/*
============
Cvar_Set
============
*/
void Cvar_Set(const char* var_name, const char* value)
{
    Cvar_Set2(var_name, value, true);
}

/*
============
Cvar_SetLatched
============
*/
void Cvar_SetLatched(const char* var_name, const char* value)
{
    Cvar_Set2(var_name, value, false);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue(const char* var_name, float value)
{
    char val[32];

    if (value == (int32) value) {
        Com_sprintf(val, sizeof(val), "%i", (int32) value);
    } else {
        Com_sprintf(val, sizeof(val), "%f", value);
    }
    Cvar_Set(var_name, val);
}


/*
============
Cvar_Reset
============
*/
void Cvar_Reset(const char* var_name)
{
    Cvar_Set2(var_name, NULL, false);
}


/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState()
{
    cvar_t* var;

    // set all default vars to the safe value
    for (var = cvar_vars; var; var = var->next) {
        if (var->flags & CVAR_CHEAT) {
            // the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here
            // because of a different var->latchedString
            if (var->latchedString) {
                Z_Free(var->latchedString);
                var->latchedString = NULL;
            }
            if (strcmp(var->resetString, var->string)) {
                Cvar_Set(var->name, var->resetString);
            }
        }
    }
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
bool Cvar_Command()
{
    cvar_t* v;

    // check variables
    v = Cvar_FindVar(Cmd_Argv(0));
    if (!v) {
        return false;
    }

    // perform a variable print or set
    if (Cmd_Argc() == 1) {
        logger.info("'%s' is:'%s' default:'%s'", v->name, v->string, v->resetString);
        if (v->latchedString) {
            logger.info("latched: '%s'", v->latchedString);
        }
        return true;
    }

    // set the value if forcing isn't required
    Cvar_Set2(v->name, Cmd_Argv(1), false);
    return true;
}


/*
============
Cvar_Toggle_f

Toggles a cvar for easy single key binding
============
*/
void Cvar_Toggle_f()
{
    int32 v;

    if (Cmd_Argc() != 2) {
        logger.info("usage: toggle <variable>");
        return;
    }

    v = Cvar_VariableValue(Cmd_Argv(1));
    v = !v;

    Cvar_Set2(Cmd_Argv(1), va("%i", v), false);
}

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void Cvar_Set_f()
{
    int32 i, c, l, len;
    char  combined[MAX_STRING_TOKENS];

    c = Cmd_Argc();
    if (c < 3) {
        logger.info("usage: set <variable> <value>");
        return;
    }

    combined[0] = 0;
    l = 0;
    for (i = 2; i < c; i++) {
        len = strlen(Cmd_Argv(i) + 1);
        if (l + len >= MAX_STRING_TOKENS - 2) {
            break;
        }
        strcat(combined, Cmd_Argv(i));
        if (i != c - 1) {
            strcat(combined, " ");
        }
        l += len;
    }
    Cvar_Set2(Cmd_Argv(1), combined, false);
}

/*
============
Cvar_SetU_f

As Cvar_Set, but also flags it as userinfo
============
*/
void Cvar_SetU_f()
{
    cvar_t* v;

    if (Cmd_Argc() != 3) {
        logger.info("usage: setu <variable> <value>");
        return;
    }
    Cvar_Set_f();
    v = Cvar_FindVar(Cmd_Argv(1));
    if (!v) {
        return;
    }
    v->flags |= CVAR_USERINFO;
}

/*
============
Cvar_SetS_f

As Cvar_Set, but also flags it as userinfo
============
*/
void Cvar_SetS_f()
{
    cvar_t* v;

    if (Cmd_Argc() != 3) {
        logger.info("usage: sets <variable> <value>");
        return;
    }
    Cvar_Set_f();
    v = Cvar_FindVar(Cmd_Argv(1));
    if (!v) {
        return;
    }
    v->flags |= CVAR_SERVERINFO;
}

/*
============
Cvar_SetA_f

As Cvar_Set, but also flags it as archived
============
*/
void Cvar_SetA_f()
{
    cvar_t* v;

    if (Cmd_Argc() != 3) {
        logger.info("usage: seta <variable> <value>");
        return;
    }
    Cvar_Set_f();
    v = Cvar_FindVar(Cmd_Argv(1));
    if (!v) {
        return;
    }
    v->flags |= CVAR_ARCHIVE;
}

/*
============
Cvar_Reset_f
============
*/
void Cvar_Reset_f()
{
    if (Cmd_Argc() != 2) {
        logger.info("usage: reset <variable>");
        return;
    }
    Cvar_Reset(Cmd_Argv(1));
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables(fileHandle_t f)
{
    cvar_t* var;
    char    buffer[1024];

    for (var = cvar_vars; var; var = var->next) {
        if (var->flags & CVAR_ARCHIVE) {
            // write the latched value, even if it hasn't taken effect yet
            if (var->latchedString) {
                Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
            } else {
                Com_sprintf(buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
            }
            FS_Printf(f, "%s", buffer);
        }
    }
}

/*
============
Cvar_List_f
============
*/
void Cvar_List_f()
{
    cvar_t*     var;
    int32       j;
    const char* match;

    if (Cmd_Argc() > 1) {
        match = Cmd_Argv(1);
    } else {
        match = NULL;
    }

    j = 0;
    for (var = cvar_vars; var; var = var->next, j++) {
        if (match && !Com_Filter(match, var->name, false)) {
            continue;
        }

        char s = var->flags & CVAR_SERVERINFO ? 'S' : '-';
        char u = var->flags & CVAR_USERINFO ? 'U' : '-';
        char r = var->flags & CVAR_ROM ? 'R' : '-';
        char i = var->flags & CVAR_INIT ? 'I' : '-';
        char a = var->flags & CVAR_ARCHIVE ? 'A' : '-';
        char l = var->flags & CVAR_LATCH ? 'L' : '-';
        char c = var->flags & CVAR_LATCH ? 'C' : '-';
        logger.info("%c%c%c%c%c%c%c %s \"%s\"\n", s, u, r, i, a, l, c, var->name, var->string);
    }

    logger.info("%i total cvars", j);
    logger.info("%i cvar indexes", cvar_numIndexes);
}

/*
============
Cvar_Restart_f

Resets all cvars to their hardcoded values
============
*/
void Cvar_Restart_f()
{
    cvar_t*  var;
    cvar_t** prev;

    prev = &cvar_vars;
    while (1) {
        var = *prev;
        if (!var) {
            break;
        }

        // don't mess with rom values, or some inter-module
        // communication will get broken (com_cl_running, etc)
        if (var->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)) {
            prev = &var->next;
            continue;
        }

        // throw out any variables the user created
        if (var->flags & CVAR_USER_CREATED) {
            *prev = var->next;
            if (var->name) {
                Z_Free(var->name);
            }
            if (var->string) {
                Z_Free(var->string);
            }
            if (var->latchedString) {
                Z_Free(var->latchedString);
            }
            if (var->resetString) {
                Z_Free(var->resetString);
            }
            // clear the var completely, since we
            // can't remove the index from the list
            Com_Memset(var, 0, sizeof(var));
            continue;
        }

        Cvar_Set(var->name, var->resetString);

        prev = &var->next;
    }
}


/*
=====================
Cvar_InfoString
=====================
*/
char* Cvar_InfoString(int32 bit)
{
    static char info[MAX_INFO_STRING];
    cvar_t*     var;

    info[0] = 0;

    for (var = cvar_vars; var; var = var->next) {
        if (var->flags & bit) {
            Info_SetValueForKey(info, var->name, var->string);
        }
    }
    return info;
}

/*
=====================
Cvar_InfoString_Big

  handles large info strings ( CS_SYSTEMINFO )
=====================
*/
char* Cvar_InfoString_Big(int32 bit)
{
    static char info[BIG_INFO_STRING];
    cvar_t*     var;

    info[0] = 0;

    for (var = cvar_vars; var; var = var->next) {
        if (var->flags & bit) {
            Info_SetValueForKey_Big(info, var->name, var->string);
        }
    }
    return info;
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init()
{
    cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO);

    Cmd_AddCommand("toggle", Cvar_Toggle_f);
    Cmd_AddCommand("set", Cvar_Set_f);
    Cmd_AddCommand("sets", Cvar_SetS_f);
    Cmd_AddCommand("setu", Cvar_SetU_f);
    Cmd_AddCommand("seta", Cvar_SetA_f);
    Cmd_AddCommand("reset", Cvar_Reset_f);
    Cmd_AddCommand("cvarlist", Cvar_List_f);
    Cmd_AddCommand("cvar_restart", Cvar_Restart_f);
}

class console_manager{
    public:
    private:
};
