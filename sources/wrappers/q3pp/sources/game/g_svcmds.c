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

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

static tavros::core::logger logger("g_svcmds");

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and you can use '*' to match any value
so you can specify an entire class C network with "addip 192.246.40.*"

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
still, you should rely on PB for banning instead

==============================================================================
*/

typedef struct ipFilter_s
{
    uint32 mask;
    uint32 compare;
} ipFilter_t;

#define MAX_IPFILTERS 1024

static ipFilter_t ipFilters[MAX_IPFILTERS];
static int32      numIPFilters;

/*
=================
StringToFilter
=================
*/
static bool StringToFilter(char* s, ipFilter_t* f)
{
    char  num[128];
    int32 i, j;
    uint8 b[4];
    uint8 m[4];

    for (i = 0; i < 4; i++) {
        b[i] = 0;
        m[i] = 0;
    }

    for (i = 0; i < 4; i++) {
        if (*s < '0' || *s > '9') {
            if (*s == '*') // 'match any'
            {
                // b[i] and m[i] to 0
                s++;
                if (!*s) {
                    break;
                }
                s++;
                continue;
            }
            logger.info("Bad filter address: %s", s);
            return false;
        }

        j = 0;
        while (*s >= '0' && *s <= '9') {
            num[j++] = *s++;
        }
        num[j] = 0;
        b[i] = atoi(num);
        m[i] = 255;

        if (!*s) {
            break;
        }
        s++;
    }

    f->mask = *(uint32*) m;
    f->compare = *(uint32*) b;

    return true;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans()
{
    uint8 b[4];
    uint8 m[4];
    int32 i, j;
    char  iplist_final[256];
    char  ip[64];

    *iplist_final = 0;
    for (i = 0; i < numIPFilters; i++) {
        if (ipFilters[i].compare == 0xffffffff) {
            continue;
        }

        *(uint32*) b = ipFilters[i].compare;
        *(uint32*) m = ipFilters[i].mask;
        *ip = 0;
        for (j = 0; j < 4; j++) {
            if (m[j] != 255) {
                Q_strcat(ip, sizeof(ip), "*");
            } else {
                Q_strcat(ip, sizeof(ip), va("%i", b[j]));
            }
            Q_strcat(ip, sizeof(ip), (j < 3) ? "." : " ");
        }
        if (strlen(iplist_final) + strlen(ip) < sizeof(iplist_final)) {
            Q_strcat(iplist_final, sizeof(iplist_final), ip);
        } else {
            logger.info("g_banIPs overflowed at MAX_CVAR_VALUE_STRING");
            break;
        }
    }

    Cvar_Set("g_banIPs", iplist_final);
}

/*
=================
G_FilterPacket
=================
*/
bool G_FilterPacket(char* from)
{
    int32  i;
    uint32 in;
    uint8  m[4] = {0, 0, 0, 0};
    char*  p;

    i = 0;
    p = from;
    while (*p && i < 4) {
        m[i] = 0;
        while (*p >= '0' && *p <= '9') {
            m[i] = m[i] * 10 + (*p - '0');
            p++;
        }
        if (!*p || *p == ':') {
            break;
        }
        i++, p++;
    }

    in = *(uint32*) m;

    for (i = 0; i < numIPFilters; i++) {
        if ((in & ipFilters[i].mask) == ipFilters[i].compare) {
            return g_filterBan->integer != 0;
        }
    }

    return g_filterBan->integer == 0;
}

/*
=================
AddIP
=================
*/
static void AddIP(char* str)
{
    int32 i;

    for (i = 0; i < numIPFilters; i++) {
        if (ipFilters[i].compare == 0xffffffff) {
            break; // free spot
        }
    }
    if (i == numIPFilters) {
        if (numIPFilters == MAX_IPFILTERS) {
            logger.info("IP filter list is full");
            return;
        }
        numIPFilters++;
    }

    if (!StringToFilter(str, &ipFilters[i])) {
        ipFilters[i].compare = 0xffffffffu;
    }

    UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans()
{
    char *s, *t;
    char  str[256];

    Q_strncpyz(str, g_banIPs->string, sizeof(str));

    for (t = s = g_banIPs->string; *t; /* */) {
        s = strchr(s, ' ');
        if (!s) {
            break;
        }
        while (*s == ' ') {
            *s++ = 0;
        }
        if (*t) {
            AddIP(t);
        }
        t = s;
    }
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f()
{
    char str[MAX_TOKEN_CHARS];

    if (Cmd_Argc() < 2) {
        logger.info("Usage:  addip <ip-mask>");
        return;
    }

    Cmd_ArgvBuffer(1, str, sizeof(str));

    AddIP(str);
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f()
{
    ipFilter_t f;
    int32      i;
    char       str[MAX_TOKEN_CHARS];

    if (Cmd_Argc() < 2) {
        logger.info("Usage:  sv removeip <ip-mask>");
        return;
    }

    Cmd_ArgvBuffer(1, str, sizeof(str));

    if (!StringToFilter(str, &f)) {
        return;
    }

    for (i = 0; i < numIPFilters; i++) {
        if (ipFilters[i].mask == f.mask && ipFilters[i].compare == f.compare) {
            ipFilters[i].compare = 0xffffffffu;
            logger.info("Removed.");

            UpdateIPBans();
            return;
        }
    }

    logger.info("Didn't find %s.", str);
}

/*
===================
Svcmd_EntityList_f
===================
*/
void Svcmd_EntityList_f()
{
    int32       e;
    gentity_t*  check;
    const char* info = "";

    check = g_entities + 1;
    for (e = 1; e < level.num_entities; e++, check++) {
        if (!check->inuse) {
            continue;
        }
        switch (check->s.eType) {
        case ET_GENERAL:
            info = "ET_GENERAL";
            break;
        case ET_PLAYER:
            info = "ET_PLAYER";
            break;
        case ET_ITEM:
            info = "ET_ITEM";
            break;
        case ET_MISSILE:
            info = "ET_MISSILE";
            break;
        case ET_MOVER:
            info = "ET_MOVER";
            break;
        case ET_BEAM:
            info = "ET_BEAM";
            break;
        case ET_PORTAL:
            info = "ET_PORTAL";
            break;
        case ET_SPEAKER:
            info = "ET_SPEAKER";
            break;
        case ET_PUSH_TRIGGER:
            info = "ET_PUSH_TRIGGER";
            break;
        case ET_TELEPORT_TRIGGER:
            info = "ET_TELEPORT_TRIGGER";
            break;
        case ET_INVISIBLE:
            info = "ET_INVISIBLE";
            break;
        case ET_GRAPPLE:
            info = "ET_GRAPPLE";
            break;
        default:
            info = "<unknown>";
            break;
        }

        logger.info("%3i: [%3i]%s %s", e, check->s.eType, info, (check->classname ? check->classname : ""));
    }
}

gclient_t* ClientForString(const char* s)
{
    gclient_t* cl;
    int32      i;
    int32      idnum;

    // numeric values are just slot numbers
    if (s[0] >= '0' && s[0] <= '9') {
        idnum = atoi(s);
        if (idnum < 0 || idnum >= level.maxclients) {
            logger.info("Bad client slot: %i", idnum);
            return NULL;
        }

        cl = &level.clients[idnum];
        if (cl->pers.connected == CON_DISCONNECTED) {
            logger.info("Client %i is not connected", idnum);
            return NULL;
        }
        return cl;
    }

    // check for a name match
    for (i = 0; i < level.maxclients; i++) {
        cl = &level.clients[i];
        if (cl->pers.connected == CON_DISCONNECTED) {
            continue;
        }
        if (!Q_stricmp(cl->pers.netname, s)) {
            return cl;
        }
    }

    logger.info("User %s is not on the server", s);

    return NULL;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void Svcmd_ForceTeam_f()
{
    gclient_t* cl;
    char       str[MAX_TOKEN_CHARS];

    // find the player
    Cmd_ArgvBuffer(1, str, sizeof(str));
    cl = ClientForString(str);
    if (!cl) {
        return;
    }

    // set the team
    Cmd_ArgvBuffer(2, str, sizeof(str));
    SetTeam(&g_entities[cl - level.clients], str);
}

char* ConcatArgs(int32 start);

/*
=================
ConsoleCommand

=================
*/
bool ConsoleCommand()
{
    char cmd[MAX_TOKEN_CHARS];

    Cmd_ArgvBuffer(0, cmd, sizeof(cmd));

    logger.debug("Execute command: '%s'", cmd);

    if (Q_stricmp(cmd, "entitylist") == 0) {
        Svcmd_EntityList_f();
        return true;
    }

    if (Q_stricmp(cmd, "forceteam") == 0) {
        Svcmd_ForceTeam_f();
        return true;
    }

    if (Q_stricmp(cmd, "game_memory") == 0) {
        Svcmd_GameMem_f();
        return true;
    }

    if (Q_stricmp(cmd, "addbot") == 0) {
        Svcmd_AddBot_f();
        return true;
    }

    if (Q_stricmp(cmd, "botlist") == 0) {
        Svcmd_BotList_f();
        return true;
    }

    if (Q_stricmp(cmd, "abort_podium") == 0) {
        Svcmd_AbortPodium_f();
        return true;
    }

    if (Q_stricmp(cmd, "addip") == 0) {
        Svcmd_AddIP_f();
        return true;
    }

    if (Q_stricmp(cmd, "removeip") == 0) {
        Svcmd_RemoveIP_f();
        return true;
    }

    if (Q_stricmp(cmd, "listip") == 0) {
        Cbuf_ExecuteText(EXEC_NOW, "g_banIPs\n");
        return true;
    }

    if (g_dedicated->integer) {
        if (Q_stricmp(cmd, "say") == 0) {
            trap_SendServerCommand(-1, va("print \"server: %s\"", ConcatArgs(1)));
            return true;
        }
        // everything else will also be printed as a say command
        trap_SendServerCommand(-1, va("print \"server: %s\"", ConcatArgs(0)));
        return true;
    }

    return false;
}

