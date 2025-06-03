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

// g_public.h -- game module information visible to server

#define GAME_API_VERSION       8

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define SVF_NOCLIENT           0x00000001 // don't send entity to clients, even if it has effects

#define SVF_CLIENTMASK         0x00000002

#define SVF_BOT                0x00000008 // set if the entity is a bot
#define SVF_BROADCAST          0x00000020 // send to all connected clients
#define SVF_PORTAL             0x00000040 // merge a second pvs at origin2 into snapshots
#define SVF_USE_CURRENT_ORIGIN 0x00000080 // entity->r.currentOrigin instead of entity->s.origin
                                          // for link position (missiles and movers)
#define SVF_SINGLECLIENT       0x00000100 // only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO       0x00000200 // don't send CS_SERVERINFO updates to this client
                                          // so that it can be updated for ping tools without
                                          // lagging clients
#define SVF_CAPSULE            0x00000400 // use capsule for collision detection instead of bbox
#define SVF_NOTSINGLECLIENT    0x00000800 // send entity to everyone but one client
                                          // (entityShared_t->singleClient)


//===============================================================


typedef struct
{
    entityState_t s; // communicated by server to clients

    bool  linked;    // false if not in any good cluster
    int32 linkcount;

    int32 svFlags; // SVF_NOCLIENT, SVF_BROADCAST, etc

    // only send to this client when SVF_SINGLECLIENT is set
    // if SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
    int32 singleClient;

    bool bmodel;           // if false, assume an explicit mins / maxs bounding box
                           // only set by trap_SetBrushModel
    vec3_t mins, maxs;
    int32  contents;       // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
                           // a non-solid entity should set to 0

    vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation

    // currentOrigin will be used for all collision detection and world linking.
    // it will not necessarily be the same as the trajectory evaluation for the current
    // time, because each entity must be moved one at a time after time is advanced
    // to avoid simultanious collision issues
    vec3_t currentOrigin;
    vec3_t currentAngles;

    // when a trace call is made and passEntityNum != ENTITYNUM_NONE,
    // an ent will be excluded from testing if:
    // ent->s.number == passEntityNum    (don't interact with self)
    // ent->s.ownerNum = passEntityNum    (don't interact with your own missiles)
    // entity[ent->s.ownerNum].ownerNum = passEntityNum    (don't interact with other missiles from owner)
    int32 ownerNum;
} entityShared_t;


// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
typedef struct
{
    entityState_t  s; // communicated by server to clients
    entityShared_t r; // shared by both the server system and game
} sharedEntity_t;


//===============================================================

// init and shutdown will be called every single level
// The game should call G_GET_ENTITY_TOKEN to parse through all the
// entity configuration text and spawn gentities.
void G_InitGame(int32 levelTime, int32 randomSeed, int32 restart);
void G_ShutdownGame(int32 restart);

// return NULL if the client is allowed to connect, otherwise return
// a text string with the reason for denial
void        ClientBegin(int32 clientNum);
const char* ClientConnect(int32 clientNum, bool firstTime, bool isBot);
void        ClientUserinfoChanged(int32 clientNum);
void        ClientDisconnect(int32 clientNum);
void        ClientCommand(int32 clientNum);
void        ClientThink(int32 clientNum);
void        G_RunFrame(int32 levelTime);

// ConsoleCommand will be called when a command has been issued
// that is not recognized as a builtin function.
// The game can issue trap_argc() / trap_argv() commands to get the command
// and parameters.  Return false if the game doesn't recognize it as a command.
bool  ConsoleCommand();
int32 BotAIStartFrame(int32 time);
