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
// sv_game.c -- interface to the game dll

#include "server.h"

#include "../game/botlib.h"

static tavros::core::logger logger("sv_game");


botlib_export_t* botlib_export;

sharedEntity_t* SV_GentityNum(int32 num)
{
    sharedEntity_t* ent;

    ent = (sharedEntity_t*) ((uint8*) sv.gentities + sv.gentitySize * (num));

    return ent;
}

playerState_t* SV_GameClientNum(int32 num)
{
    playerState_t* ps;

    ps = (playerState_t*) ((uint8*) sv.gameClients + sv.gameClientSize * (num));

    return ps;
}

svEntity_t* SV_SvEntityForGentity(sharedEntity_t* gEnt)
{
    if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES) {
        Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
    }
    return &sv.svEntities[gEnt->s.number];
}

sharedEntity_t* SV_GEntityForSvEntity(svEntity_t* svEnt)
{
    int32 num;

    num = svEnt - sv.svEntities;
    return SV_GentityNum(num);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int32 clientNum, const char* text)
{
    if (clientNum == -1) {
        SV_SendServerCommand(NULL, "%s", text);
    } else {
        if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
            return;
        }
        SV_SendServerCommand(svs.clients + clientNum, "%s", text);
    }
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int32 clientNum, const char* reason)
{
    if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
        return;
    }
    SV_DropClient(svs.clients + clientNum, reason);
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(sharedEntity_t* ent, const char* name)
{
    clipHandle_t h;
    vec3_t       mins, maxs;

    if (!name) {
        Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
    }

    if (name[0] != '*') {
        Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
    }


    ent->s.modelindex = atoi(name + 1);

    h = CM_InlineModel(ent->s.modelindex);
    CM_ModelBounds(h, mins, maxs);
    VectorCopy(mins, ent->r.mins);
    VectorCopy(maxs, ent->r.maxs);
    ent->r.bmodel = true;

    ent->r.contents = -1; // we don't know exactly what is in the brushes

    SV_LinkEntity(ent);   // FIXME: remove
}


/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
bool SV_inPVS(const vec3_t p1, const vec3_t p2)
{
    int32  leafnum;
    int32  cluster;
    int32  area1, area2;
    uint8* mask;

    leafnum = CM_PointLeafnum(p1);
    cluster = CM_LeafCluster(leafnum);
    area1 = CM_LeafArea(leafnum);
    mask = CM_ClusterPVS(cluster);

    leafnum = CM_PointLeafnum(p2);
    cluster = CM_LeafCluster(leafnum);
    area2 = CM_LeafArea(leafnum);
    if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7))))) {
        return false;
    }
    if (!CM_AreasConnected(area1, area2)) {
        return false; // a door blocks sight
    }
    return true;
}

/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(sharedEntity_t* ent, bool open)
{
    svEntity_t* svEnt;

    svEnt = SV_SvEntityForGentity(ent);
    if (svEnt->areanum2 == -1) {
        return;
    }
    CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
bool SV_EntityContact(vec3_t mins, vec3_t maxs, const sharedEntity_t* gEnt, int32 capsule)
{
    const float *origin, *angles;
    clipHandle_t ch;
    trace_t      trace;

    // check for exact collision
    origin = gEnt->r.currentOrigin;
    angles = gEnt->r.currentAngles;

    ch = SV_ClipHandleForEntity(gEnt);
    CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, capsule);

    return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char* buffer, int32 bufferSize)
{
    if (bufferSize < 1) {
        Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
    }
    Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(sharedEntity_t* gEnts, int32 numGEntities, int32 sizeofGEntity_t, playerState_t* clients, int32 sizeofGameClient)
{
    sv.gentities = gEnts;
    sv.gentitySize = sizeofGEntity_t;
    sv.num_entities = numGEntities;

    sv.gameClients = clients;
    sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int32 clientNum, usercmd_t* cmd)
{
    if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
        Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
    }
    *cmd = svs.clients[clientNum].lastUsercmd;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs()
{
    G_ShutdownGame(false);
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(bool restart)
{
    int32 i;

    // start the entity parsing at the beginning
    sv.entityParsePoint = CM_EntityString();

    // clear all gentity pointers that might still be set from
    // a previous level
    //   now done before GAME_INIT call
    for (i = 0; i < sv_maxclients->integer; i++) {
        svs.clients[i].gentity = NULL;
    }

    // use the current msec count for a random seed
    // init for this gamestate
    G_InitGame(svs.time, Com_Milliseconds(), restart);
}


/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs()
{
    G_ShutdownGame(true);
    SV_InitGameVM(true);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs()
{
    cvar_t* var;
    // FIXME these are temp while I make bots run in vm
    extern int32 bot_enable;

    var = Cvar_Get("bot_enable", "1", CVAR_LATCH);
    if (var) {
        bot_enable = var->integer;
    } else {
        bot_enable = 0;
    }

    SV_InitGameVM(false);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
bool SV_GameCommand()
{
    if (sv.state != SS_GAME) {
        return false;
    }

    ConsoleCommand();
}

