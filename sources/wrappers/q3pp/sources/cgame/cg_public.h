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

#include <tavros/core/types.hpp>
#include <game/q_shared.h>

#define CMD_BACKUP               64
#define CMD_MASK                 (CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define MAX_ENTITIES_IN_SNAPSHOT 256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct
{
    int32 snapFlags; // SNAPFLAG_RATE_DELAYED, etc
    int32 ping;

    int32 serverTime;                                 // server time the message is valid for (in msec)

    uint8 areamask[MAX_MAP_AREA_BYTES];               // portalarea visibility bits

    playerState_t ps;                                 // complete information about the current player at this time

    int32         numEntities;                        // all of the entities that need to be presented
    entityState_t entities[MAX_ENTITIES_IN_SNAPSHOT]; // at the time of this snapshot

    int32 numServerCommands;                          // text based server commands to execute when this
    int32 serverCommandSequence;                      // snapshot becomes current
} snapshot_t;

enum
{
    CGAME_EVENT_NONE,
    CGAME_EVENT_TEAMMENU,
    CGAME_EVENT_SCOREBOARD,
    CGAME_EVENT_EDITHUD
};

/*
==================================================================
functions exported to the main executable
==================================================================
*/

// called when the level loads or when the renderer is restarted all media should be registered
// at this time cgame will display loading status by calling SCR_Update, which will call
// CG_DrawInformation during the loading process reliableCommandSequence will be 0 on fresh loads,
// but higher for demos, tourney restarts, or vid_restarts
void CG_Init(int32 serverMessageNum, int32 serverCommandSequence, int32 clientNum);

// oportunity to flush and close any open files
void CG_Shutdown();

// a console command has been issued locally that is not recognized by the main game system. Use
// Cmd_Argc()/Cmd_Argv() to read the command, return false if the command is not known to the game
bool CG_ConsoleCommand();

// Generates and draws a game scene and status information at the given time. If demoPlayback
// is set, local movement prediction will not be enabled
void CG_DrawActiveFrame(int32 serverTime, bool demoPlayback);

int32 CG_CrosshairPlayer();
int32 CG_LastAttacker();
void  CG_KeyEvent(int32 key, bool down);
void  CG_MouseEvent(int32 x, int32 y);
void  CG_EventHandling(int32 type);
