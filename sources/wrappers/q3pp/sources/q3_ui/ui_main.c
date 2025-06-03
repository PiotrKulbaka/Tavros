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

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars()
{
    Cvar_Get("ui_ffa_fraglimit", "20", CVAR_ARCHIVE);
    Cvar_Get("ui_ffa_timelimit", "0", CVAR_ARCHIVE);

    Cvar_Get("ui_tourney_fraglimit", "0", CVAR_ARCHIVE);
    Cvar_Get("ui_tourney_timelimit", "15", CVAR_ARCHIVE);

    Cvar_Get("ui_team_fraglimit", "0", CVAR_ARCHIVE);
    Cvar_Get("ui_team_timelimit", "20", CVAR_ARCHIVE);
    Cvar_Get("ui_team_friendly", "1", CVAR_ARCHIVE);

    Cvar_Get("ui_ctf_capturelimit", "8", CVAR_ARCHIVE);
    Cvar_Get("ui_ctf_timelimit", "30", CVAR_ARCHIVE);
    Cvar_Get("ui_ctf_friendly", "0", CVAR_ARCHIVE);

    Cvar_Get("ui_spSelection", "", CVAR_ROM);

    Cvar_Get("ui_browserMaster", "0", CVAR_ARCHIVE);
    Cvar_Get("ui_browserGameType", "0", CVAR_ARCHIVE);
    Cvar_Get("ui_browserSortKey", "4", CVAR_ARCHIVE);
    Cvar_Get("ui_browserShowFull", "1", CVAR_ARCHIVE);
    Cvar_Get("ui_browserShowEmpty", "1", CVAR_ARCHIVE);
}
