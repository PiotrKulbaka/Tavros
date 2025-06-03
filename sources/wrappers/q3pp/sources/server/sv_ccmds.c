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

#include "server.h"

static tavros::core::logger logger("sv_ccmds");

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/


/*
==================
SV_GetPlayerByName

Returns the player with name from Cmd_Argv(1)
==================
*/
static client_t* SV_GetPlayerByName()
{
    client_t*   cl;
    int32       i;
    const char* s;
    char        cleanName[64];

    // make sure server is running
    if (!com_sv_running->integer) {
        return NULL;
    }

    if (Cmd_Argc() < 2) {
        logger.info("No player specified.");
        return NULL;
    }

    s = Cmd_Argv(1);

    // check for a name match
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (!cl->state) {
            continue;
        }
        if (!Q_stricmp(cl->name, s)) {
            return cl;
        }

        Q_strncpyz(cleanName, cl->name, sizeof(cleanName));
        Q_CleanStr(cleanName);
        if (!Q_stricmp(cleanName, s)) {
            return cl;
        }
    }

    logger.info("Player %s is not on the server", s);

    return NULL;
}

/*
==================
SV_GetPlayerByNum

Returns the player with idnum from Cmd_Argv(1)
==================
*/
static client_t* SV_GetPlayerByNum()
{
    client_t*   cl;
    int32       i;
    int32       idnum;
    const char* s;

    // make sure server is running
    if (!com_sv_running->integer) {
        return NULL;
    }

    if (Cmd_Argc() < 2) {
        logger.info("No player specified.");
        return NULL;
    }

    s = Cmd_Argv(1);

    for (i = 0; s[i]; i++) {
        if (s[i] < '0' || s[i] > '9') {
            logger.info("Bad slot number: %s", s);
            return NULL;
        }
    }
    idnum = atoi(s);
    if (idnum < 0 || idnum >= sv_maxclients->integer) {
        logger.info("Bad client slot: %i", idnum);
        return NULL;
    }

    cl = &svs.clients[idnum];
    if (!cl->state) {
        logger.info("Client %i is not active", idnum);
        return NULL;
    }
    return cl;

    return NULL;
}

//=========================================================


/*
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f()
{
    const char* cmd;
    const char* map;
    bool        killBots, cheat;
    char        expanded[MAX_QPATH];
    char        mapname[MAX_QPATH];

    map = Cmd_Argv(1);
    if (!map) {
        return;
    }

    // make sure the level exists before trying to change, so that
    // a typo at the server console won't end the game
    Com_sprintf(expanded, sizeof(expanded), "maps/%s.bsp", map);
    if (FS_ReadFile(expanded, NULL) == -1) {
        logger.info("Can't find map %s", expanded);
        return;
    }

    // force latched values to get set
    Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH);

    cmd = Cmd_Argv(0);
    if (Q_stricmpn(cmd, "sp", 2) == 0) {
        Cvar_SetValue("g_gametype", GT_SINGLE_PLAYER);
        Cvar_SetValue("g_doWarmup", 0);
        // may not set sv_maxclients directly, always set latched
        Cvar_SetLatched("sv_maxclients", "8");
        cmd += 2;
        cheat = false;
        killBots = true;
    } else {
        if (!Q_stricmp(cmd, "devmap") || !Q_stricmp(cmd, "spdevmap")) {
            cheat = true;
            killBots = true;
        } else {
            cheat = false;
            killBots = false;
        }
        if (sv_gametype->integer == GT_SINGLE_PLAYER) {
            Cvar_SetValue("g_gametype", GT_FFA);
        }
    }

    // save the map name here cause on a map restart we reload the q3config.cfg
    // and thus nuke the arguments of the map command
    Q_strncpyz(mapname, map, sizeof(mapname));

    // start up the map
    SV_SpawnServer(mapname, killBots);

    // set the cheat value
    // if the level was started with "map <levelname>", then
    // cheats will not be allowed.  If started with "devmap <levelname>"
    // then cheats will be allowed
    cheat = true; // TODO: force true, remove this line

    if (cheat) {
        Cvar_Set("sv_cheats", "1");
    } else {
        Cvar_Set("sv_cheats", "0");
    }
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f()
{
    int32       i;
    client_t*   client;
    const char* denied;
    bool        isBot;
    int32       delay;

    // make sure we aren't restarting twice in the same frame
    if (com_frameTime == sv.serverId) {
        return;
    }

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (sv.restartTime) {
        return;
    }

    if (Cmd_Argc() > 1) {
        delay = atoi(Cmd_Argv(1));
    } else {
        delay = 5;
    }
    if (delay && !Cvar_VariableValue("g_doWarmup")) {
        sv.restartTime = svs.time + delay * 1000;
        SV_SetConfigstring(CS_WARMUP, va("%i", sv.restartTime));
        return;
    }

    // check for changes in variables that can't just be restarted
    // check for maxclients change
    if (sv_maxclients->modified || sv_gametype->modified) {
        char mapname[MAX_QPATH];

        logger.info("variable change -- restarting.");
        // restart the map the slow way
        Q_strncpyz(mapname, Cvar_VariableString("mapname"), sizeof(mapname));

        SV_SpawnServer(mapname, false);
        return;
    }

    // toggle the server bit so clients can detect that a
    // map_restart has happened
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

    // generate a new serverid
    // TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
    sv.serverId = com_frameTime;
    Cvar_Set("sv_serverid", va("%i", sv.serverId));

    // reset all the vm data in place without changing memory allocation
    // note that we do NOT set sv.state = SS_LOADING, so configstrings that
    // had been changed from their default values will generate broadcast updates
    sv.state = SS_LOADING;
    sv.restarting = true;

    SV_RestartGameProgs();

    // run a few frames to allow everything to settle
    for (i = 0; i < 3; i++) {
        G_RunFrame(svs.time);
        svs.time += 100;
    }

    sv.state = SS_GAME;
    sv.restarting = false;

    // connect and begin all the clients
    for (i = 0; i < sv_maxclients->integer; i++) {
        client = &svs.clients[i];

        // send the new gamestate to all connected clients
        if (client->state < CS_CONNECTED) {
            continue;
        }

        if (client->netchan.remoteAddress.type == NA_BOT) {
            isBot = true;
        } else {
            isBot = false;
        }

        // add the map_restart command
        SV_AddServerCommand(client, "map_restart\n");

        // connect the client again, without the firstTime flag
        denied = (const char*) ClientConnect(i, false, isBot);
        if (denied) {
            // this generally shouldn't happen, because the client
            // was connected before the level change
            SV_DropClient(client, denied);
            logger.info("SV_MapRestart_f(%d): dropped client %i - denied!", delay, i); // bk010125
            continue;
        }

        client->state = CS_ACTIVE;

        SV_ClientEnterWorld(client, &client->lastUsercmd);
    }

    // run another frame to allow things to look at all the players
    G_RunFrame(svs.time);
    svs.time += 100;
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server  FIXME: move to game
==================
*/
static void SV_Kick_f()
{
    client_t* cl;
    int32     i;

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() != 2) {
        logger.info("Usage: kick <player name>\nkick all = kick everyone\nkick allbots = kick all bots");
        return;
    }

    cl = SV_GetPlayerByName();
    if (!cl) {
        if (!Q_stricmp(Cmd_Argv(1), "all")) {
            for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
                if (!cl->state) {
                    continue;
                }
                if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
                    continue;
                }
                SV_DropClient(cl, "was kicked");
                cl->lastPacketTime = svs.time; // in case there is a funny zombie
            }
        } else if (!Q_stricmp(Cmd_Argv(1), "allbots")) {
            for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
                if (!cl->state) {
                    continue;
                }
                if (cl->netchan.remoteAddress.type != NA_BOT) {
                    continue;
                }
                SV_DropClient(cl, "was kicked");
                cl->lastPacketTime = svs.time; // in case there is a funny zombie
            }
        }
        return;
    }
    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
        return;
    }

    SV_DropClient(cl, "was kicked");
    cl->lastPacketTime = svs.time; // in case there is a funny zombie
}

/*
==================
SV_Ban_f

Ban a user from being able to play on this server through the auth
server
==================
*/
static void SV_Ban_f()
{
    client_t* cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() != 2) {
        logger.info("Usage: banUser <player name>");
        return;
    }

    cl = SV_GetPlayerByName();

    if (!cl) {
        return;
    }

    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
        return;
    }

    // look up the authorize server's IP
    if (!svs.authorizeAddress.ip[0] && svs.authorizeAddress.type != NA_BAD) {
        logger.info("Resolving %s", AUTHORIZE_SERVER_NAME);
        if (!NET_StringToAdr(AUTHORIZE_SERVER_NAME, &svs.authorizeAddress)) {
            logger.info("Couldn't resolve address");
            return;
        }
        svs.authorizeAddress.port = BigShort(PORT_AUTHORIZE);
        logger.info("%s resolved to %i.%i.%i.%i:%i", AUTHORIZE_SERVER_NAME, svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1], svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3], BigShort(svs.authorizeAddress.port));
    }

    // otherwise send their ip to the authorize server
    if (svs.authorizeAddress.type != NA_BAD) {
        NET_OutOfBandPrint(NS_SERVER, svs.authorizeAddress, "banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1], cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);
        logger.info("%s was banned from coming back", cl->name);
    }
}

/*
==================
SV_BanNum_f

Ban a user from being able to play on this server through the auth
server
==================
*/
static void SV_BanNum_f()
{
    client_t* cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() != 2) {
        logger.info("Usage: banClient <client number>");
        return;
    }

    cl = SV_GetPlayerByNum();
    if (!cl) {
        return;
    }
    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
        return;
    }

    // look up the authorize server's IP
    if (!svs.authorizeAddress.ip[0] && svs.authorizeAddress.type != NA_BAD) {
        logger.info("Resolving %s", AUTHORIZE_SERVER_NAME);
        if (!NET_StringToAdr(AUTHORIZE_SERVER_NAME, &svs.authorizeAddress)) {
            logger.info("Couldn't resolve address");
            return;
        }
        svs.authorizeAddress.port = BigShort(PORT_AUTHORIZE);
        logger.info("%s resolved to %i.%i.%i.%i:%i", AUTHORIZE_SERVER_NAME, svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1], svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3], BigShort(svs.authorizeAddress.port));
    }

    // otherwise send their ip to the authorize server
    if (svs.authorizeAddress.type != NA_BAD) {
        NET_OutOfBandPrint(NS_SERVER, svs.authorizeAddress, "banUser %i.%i.%i.%i", cl->netchan.remoteAddress.ip[0], cl->netchan.remoteAddress.ip[1], cl->netchan.remoteAddress.ip[2], cl->netchan.remoteAddress.ip[3]);
        logger.info("%s was banned from coming back", cl->name);
    }
}

/*
==================
SV_KickNum_f

Kick a user off of the server  FIXME: move to game
==================
*/
static void SV_KickNum_f()
{
    client_t* cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() != 2) {
        logger.info("Usage: kicknum <client number>");
        return;
    }

    cl = SV_GetPlayerByNum();
    if (!cl) {
        return;
    }
    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_SendServerCommand(NULL, "print \"%s\"", "Cannot kick host player\n");
        return;
    }

    SV_DropClient(cl, "was kicked");
    cl->lastPacketTime = svs.time; // in case there is a funny zombie
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f()
{
    int32          i, j, l;
    client_t*      cl;
    playerState_t* ps;
    const char*    s;
    int32          ping;
    char           buf[256];

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    logger.info("map: %s", sv_mapname->string);

    logger.info("num score ping name            lastmsg address               qport rate");
    logger.info("--- ----- ---- --------------- ------- --------------------- ----- -----");
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (!cl->state) {
            continue;
        }
        ps = SV_GameClientNum(i);
        int len = sprintf(buf, "%3i %5i ", i, ps->persistant[PERS_SCORE]);

        if (cl->state == CS_CONNECTED) {
            len += sprintf(buf + len, "CNCT ");
        } else if (cl->state == CS_ZOMBIE) {
            len += sprintf(buf + len, "ZMBI ");
        } else {
            ping = cl->ping < 9999 ? cl->ping : 9999;
            len += sprintf(buf + len, "%4i ", ping);
        }

        len += sprintf(buf + len, "%s", cl->name);
        l = 16 - strlen(cl->name);
        for (j = 0; j < l; j++) {
            len += sprintf(buf + len, " ");
        }

        len += sprintf(buf + len, "%7i ", svs.time - cl->lastPacketTime);

        s = NET_AdrToString(cl->netchan.remoteAddress);
        len += sprintf(buf + len, "%s", s);
        l = 22 - strlen(s);
        for (j = 0; j < l; j++) {
            len += sprintf(buf + len, " ");
        }

        len += sprintf(buf + len, "%5i %5i", cl->netchan.qport, cl->rate);
        logger.info("%s", buf);
    }
}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f()
{
    char* p;
    char  text[1024];

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() < 2) {
        return;
    }

    strcpy(text, "console: ");
    p = Cmd_Args();

    if (*p == '"') {
        p++;
        p[strlen(p) - 1] = 0;
    }

    strcat(text, p);

    SV_SendServerCommand(NULL, "chat \"%s\n\"", text);
}


/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f()
{
    svs.nextHeartbeatTime = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f()
{
    logger.info("Server info settings: %s", Cvar_InfoString(CVAR_SERVERINFO));
}


/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f()
{
    logger.info("System info settings: %s", Cvar_InfoString(CVAR_SYSTEMINFO));
}


/*
===========
SV_DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
static void SV_DumpUser_f()
{
    client_t* cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        logger.info("Server is not running.");
        return;
    }

    if (Cmd_Argc() != 2) {
        logger.info("Usage: info <userid>");
        return;
    }

    cl = SV_GetPlayerByName();
    if (!cl) {
        return;
    }

    logger.info("Userinfo: %s", cl->userinfo);
}


/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f()
{
    SV_Shutdown("killserver");
}

//===========================================================

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands()
{
    static bool initialized;

    if (initialized) {
        return;
    }
    initialized = true;

    Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
    Cmd_AddCommand("kick", SV_Kick_f);
    Cmd_AddCommand("banUser", SV_Ban_f);
    Cmd_AddCommand("banClient", SV_BanNum_f);
    Cmd_AddCommand("clientkick", SV_KickNum_f);
    Cmd_AddCommand("status", SV_Status_f);
    Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
    Cmd_AddCommand("systeminfo", SV_Systeminfo_f);
    Cmd_AddCommand("dumpuser", SV_DumpUser_f);
    Cmd_AddCommand("map_restart", SV_MapRestart_f);
    Cmd_AddCommand("sectorlist", SV_SectorList_f);
    Cmd_AddCommand("map", SV_Map_f);
    Cmd_AddCommand("devmap", SV_Map_f);
    Cmd_AddCommand("spmap", SV_Map_f);
    Cmd_AddCommand("spdevmap", SV_Map_f);
    Cmd_AddCommand("killserver", SV_KillServer_f);
    if (com_dedicated->integer) {
        Cmd_AddCommand("say", SV_ConSay_f);
    }
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands()
{
}

