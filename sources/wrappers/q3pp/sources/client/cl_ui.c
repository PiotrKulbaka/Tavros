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

#include "client.h"

static tavros::core::logger logger("cl_ui");

#include "../game/botlib.h"

extern botlib_export_t* botlib_export;

static bool s_ui_is_init = false;

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI()
{
    s_ui_is_init = false;
    cls.keyCatchers &= ~KEYCATCH_UI;
    cls.uiStarted = false;
    UI_Shutdown();
}

/*
====================
CL_InitUI
====================
*/
void CL_InitUI()
{
    UI_Init();
    s_ui_is_init = true;
}

bool CL_UIIsInit()
{
    return s_ui_is_init;
}

/*
====================
UI_GameCommand
See if the current console command is claimed by the ui
====================
*/
bool UI_GameCommand()
{
    return UI_ConsoleCommand(cls.realtime);
}
