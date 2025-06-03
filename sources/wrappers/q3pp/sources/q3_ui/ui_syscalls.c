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

#include "ui_local.h"
#include "../client/client.h"

/*
====================
Ket_SetCatcher
====================
*/
void Key_SetCatcher(int32 catcher)
{
    cls.keyCatchers = catcher;
}

/*
====================
Key_GetCatcher
====================
*/
int32 Key_GetCatcher()
{
    return cls.keyCatchers;
}

void __UI_GetClientState(uiClientState_t* state)
{
    state->connectPacketCount = clc.connectPacketCount;
    state->connState = cls.state;
    Q_strncpyz(state->servername, cls.servername, sizeof(state->servername));
    Q_strncpyz(state->updateInfoString, cls.updateInfoString, sizeof(state->updateInfoString));
    Q_strncpyz(state->messageString, clc.serverMessage, sizeof(state->messageString));
    state->clientNum = cl.snap.ps.clientNum;
}

int32 __UI_GetConfigString(int32 index, char* buff, int32 buffsize)
{
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        return false;
    }

    int32 offset = cl.gameState.stringOffsets[index];
    if (!offset) {
        if (buffsize) {
            buff[0] = 0;
        }
        return false;
    }
    Q_strncpyz(buff, cl.gameState.stringData + offset, buffsize);

    return true;
}
