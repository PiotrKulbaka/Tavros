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

#include <botlib/be_aas_def.h>

#ifdef AASINTERN

extern aas_t aasworld;

// AAS error message
void QDECL AAS_Error(const char* fmt, ...);
// set AAS initialized
void AAS_SetInitialized();
// setup AAS with the given number of entities and clients
int32 AAS_Setup();
// shutdown AAS
void AAS_Shutdown();
// start a new map
int32 AAS_LoadMap(const char* mapname);
// start a new time frame
int32 AAS_StartFrame(float time);
#endif // AASINTERN

// returns true if AAS is initialized
int32 AAS_Initialized();
// returns true if the AAS file is loaded
int32 AAS_Loaded();
// returns the current time
float AAS_Time();
//
void AAS_ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
