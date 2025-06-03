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

typedef struct
{
    connstate_t connState;
    int32       connectPacketCount;
    int32       clientNum;
    char        servername[MAX_STRING_CHARS];
    char        updateInfoString[MAX_STRING_CHARS];
    char        messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum
{
    UIMENU_NONE,
    UIMENU_MAIN,
    UIMENU_INGAME,
    UIMENU_TEAM,
    UIMENU_POSTGAME
} uiMenuCommand_t;

#define SORT_HOST    0
#define SORT_MAP     1
#define SORT_CLIENTS 2
#define SORT_GAME    3
#define SORT_PING    4

void UI_Init();
void UI_Shutdown();
void UI_KeyEvent(int32 key, int32 down);
void UI_MouseEvent(int32 dx, int32 dy);
void UI_Refresh(int32 realtime);
bool UI_IsFullscreen();
void UI_SetActiveMenu(uiMenuCommand_t menu);
bool UI_ConsoleCommand(int32 realTime);
void UI_DrawConnectScreen(bool overlay);
