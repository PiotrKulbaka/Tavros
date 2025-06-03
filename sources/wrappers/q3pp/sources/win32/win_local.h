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

#define DIRECTSOUND_VERSION 0x0300

#include <windows.h>
#include <dsound.h>
#include <winsock.h>
#include <wsipx.h>

#include <tavros/core/types.hpp>
#include <qcommon/qcommon.h>

void Sys_QueEvent(int32 time, sysEventType_t type, int32 value, int32 value2, int32 ptrLength, void* ptr);

bool Sys_GetPacket(netadr_t* net_from, msg_t* net_message);

void  SNDDMA_Activate();
int32 SNDDMA_InitDS();

typedef struct
{
    HWND hWnd;
    bool activeApp;
    bool isMinimized;

    // when we get a windows message, we store the time off so keyboard processing
    // can know the exact time of an event
    uint32 sysMsgTime;
} WinVars_t;

extern WinVars_t g_wv;
