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
#include "../q3_ui/keycodes.h"

#include <tavros/core/types.hpp>
#include <qcommon/qcommon.h>
#include <game/q_shared.h>

#define MAX_KEYS 256

typedef struct
{
    bool  down;
    int32 repeats; // if > 1, it is autorepeating
    char* binding;
} qkey_t;

extern bool   key_overstrikeMode;
extern qkey_t keys[MAX_KEYS];

// NOTE TTimo the declaration of field_t and Field_Clear is now in qcommon/qcommon.h
void Field_KeyDownEvent(field_t* edit, int32 key);
void Field_CharEvent(field_t* edit, int32 ch);
void Field_Draw(field_t* edit, int32 x, int32 y, int32 width, bool showCursor);
void Field_BigDraw(field_t* edit, int32 x, int32 y, int32 width, bool showCursor);

#define COMMAND_HISTORY 32
extern field_t historyEditLines[COMMAND_HISTORY];

extern field_t g_consoleField;
extern field_t chatField;
extern bool    anykeydown;
extern bool    chat_team;
extern int32   chat_playerNum;

void        Key_WriteBindings(fileHandle_t f);
void        Key_SetBinding(int32 keynum, const char* binding);
const char* Key_GetBinding(int32 keynum);
bool        Key_IsDown(int32 keynum);
bool        Key_GetOverstrikeMode();
void        Key_SetOverstrikeMode(bool state);
void        Key_ClearStates();
