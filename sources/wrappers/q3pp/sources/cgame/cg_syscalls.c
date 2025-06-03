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

#include "cg_local.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

static tavros::core::logger logger("cg_sysvall");

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState(gameState_t* gs)
{
    *gs = cl.gameState;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void CL_GetCurrentSnapshotNumber(int32* snapshotNumber, int32* serverTime)
{
    *snapshotNumber = cl.snap.messageNum;
    *serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
bool CL_GetSnapshot(int32 snapshotNumber, snapshot_t* snapshot)
{
    clSnapshot_t* clSnap;
    int32         i, count;

    if (snapshotNumber > cl.snap.messageNum) {
        Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
    }

    // if the frame has fallen out of the circular buffer, we can't return it
    if (cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP) {
        return false;
    }

    // if the frame is not valid, we can't return it
    clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
    if (!clSnap->valid) {
        return false;
    }

    // if the entities in the frame have fallen out of their
    // circular buffer, we can't return it
    if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES) {
        return false;
    }

    // write the snapshot
    snapshot->snapFlags = clSnap->snapFlags;
    snapshot->serverCommandSequence = clSnap->serverCommandNum;
    snapshot->ping = clSnap->ping;
    snapshot->serverTime = clSnap->serverTime;
    Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
    snapshot->ps = clSnap->ps;
    count = clSnap->numEntities;
    if (count > MAX_ENTITIES_IN_SNAPSHOT) {
        logger.debug("CL_GetSnapshot: truncated %i entities to %i", count, MAX_ENTITIES_IN_SNAPSHOT);
        count = MAX_ENTITIES_IN_SNAPSHOT;
    }
    snapshot->numEntities = count;
    for (i = 0; i < count; i++) {
        snapshot->entities[i] =
            cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
    }

    // FIXME: configstring changes and server commands!!!

    return true;
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified()
{
    char *      old, *s;
    int32       i, index;
    char*       dup;
    gameState_t oldGs;
    int32       len;

    index = atoi(Cmd_Argv(1));
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
    }
    // get everything after "cs <num>"
    s = Cmd_ArgsFrom(2);

    old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
    if (!strcmp(old, s)) {
        return; // unchanged
    }

    // build the new gameState_t
    oldGs = cl.gameState;

    Com_Memset(&cl.gameState, 0, sizeof(cl.gameState));

    // leave the first 0 for uninitialized strings
    cl.gameState.dataCount = 1;

    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if (i == index) {
            dup = s;
        } else {
            dup = oldGs.stringData + oldGs.stringOffsets[i];
        }
        if (!dup[0]) {
            continue; // leave with the default empty string
        }

        len = strlen(dup);

        if (len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS) {
            Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
        }

        // append it to the gameState string buffer
        cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
        Com_Memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
        cl.gameState.dataCount += len + 1;
    }

    if (index == CS_SYSTEMINFO) {
        // parse serverId and other cvars
        CL_SystemInfoChanged();
    }
}

/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
bool CL_GetServerCommand(int32 serverCommandNumber)
{
    const char* s;
    const char* cmd;
    static char bigConfigString[BIG_INFO_STRING];
    int32       argc;

    // if we have irretrievably lost a reliable command, drop the connection
    if (serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS) {
        // when a demo record was started after the client got a whole bunch of
        // reliable commands then the client never got those first reliable commands
        if (clc.demoplaying) {
            return false;
        }
        Com_Error(ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out");
        return false;
    }

    if (serverCommandNumber > clc.serverCommandSequence) {
        Com_Error(ERR_DROP, "CL_GetServerCommand: requested a command not received");
        return false;
    }

    s = clc.serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS - 1)];
    clc.lastExecutedServerCommand = serverCommandNumber;

    logger.debug("serverCommand: %i : %s", serverCommandNumber, s);

rescan:
    Cmd_TokenizeString(s);
    cmd = Cmd_Argv(0);
    argc = Cmd_Argc();

    if (!strcmp(cmd, "disconnect")) {
        // allow server to indicate why they were disconnected
        if (argc >= 2) {
            const char* s = Cmd_Argv(1);
            Com_Error(ERR_SERVERDISCONNECT, va("Server Disconnected - %s", s));
        } else {
            Com_Error(ERR_SERVERDISCONNECT, "Server disconnected\n");
        }
    }

    if (!strcmp(cmd, "bcs0")) {
        Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
        return false;
    }

    if (!strcmp(cmd, "bcs1")) {
        s = Cmd_Argv(2);
        if (strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING) {
            Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
        }
        strcat(bigConfigString, s);
        return false;
    }

    if (!strcmp(cmd, "bcs2")) {
        s = Cmd_Argv(2);
        if (strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING) {
            Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
        }
        strcat(bigConfigString, s);
        strcat(bigConfigString, "\"");
        s = bigConfigString;
        goto rescan;
    }

    if (!strcmp(cmd, "cs")) {
        CL_ConfigstringModified();
        // reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
        Cmd_TokenizeString(s);
        return true;
    }

    if (!strcmp(cmd, "map_restart")) {
        // clear notify lines and outgoing commands before passing
        // the restart to the cgame
        Com_Memset(cl.cmds, 0, sizeof(cl.cmds));
        return true;
    }

    // cgame can now act on the command
    return true;
}

/*
===================
CL_GetCurrentCmdNumber
===================
*/
int32 CL_GetCurrentCmdNumber()
{
    return cl.cmdNumber;
}

/*
====================
CL_GetUserCmd
====================
*/
bool CL_GetUserCmd(int32 cmdNumber, usercmd_t* ucmd)
{
    // cmds[cmdNumber] is the last properly generated command

    // can't return anything that we haven't created yet
    if (cmdNumber > cl.cmdNumber) {
        Com_Error(ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber);
    }

    // the usercmd has been overwritten in the wrapping
    // buffer because it is too far out of date
    if (cmdNumber <= cl.cmdNumber - CMD_BACKUP) {
        return false;
    }

    *ucmd = cl.cmds[cmdNumber & CMD_MASK];

    return true;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
void CL_SetUserCmdValue(int32 userCmdValue, float sensitivityScale)
{
    cl.cgameUserCmdValue = userCmdValue;
    cl.cgameSensitivity = sensitivityScale;
}
