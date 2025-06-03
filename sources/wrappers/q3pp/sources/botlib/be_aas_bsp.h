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
#include <game/botlib.h>

#include <botlib/be_aas_def.h>

#ifdef AASINTERN
// loads the given BSP file
int32 AAS_LoadBSPFile();
// dump the loaded BSP data
void AAS_DumpBSPData();
// unlink the given entity from the bsp tree leaves
void AAS_UnlinkFromBSPLeaves(bsp_link_t* leaves);
// link the given entity to the bsp tree leaves of the given model
bsp_link_t* AAS_BSPLinkEntity(vec3_t absmins, vec3_t absmaxs, int32 entnum, int32 modelnum);

// calculates collision with given entity
bool AAS_EntityCollision(int32 entnum, vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end, int32 contentmask, bsp_trace_t* trace);
#endif // AASINTERN

#define MAX_EPAIRKEY 128

// trace through the world
bsp_trace_t AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int32 passent, int32 contentmask);
// returns the contents at the given point
int32 AAS_PointContents(vec3_t point);
// gets the mins, maxs and origin of a BSP model
void AAS_BSPModelMinsMaxsOrigin(int32 modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
// handle to the next bsp entity
int32 AAS_NextBSPEntity(int32 ent);
// return the value of the BSP epair key
int32 AAS_ValueForBSPEpairKey(int32 ent, const char* key, char* value, int32 size);
// get a vector for the BSP epair key
int32 AAS_VectorForBSPEpairKey(int32 ent, const char* key, vec3_t v);
// get a float for the BSP epair key
int32 AAS_FloatForBSPEpairKey(int32 ent, const char* key, float* value);
// get an integer for the BSP epair key
int32 AAS_IntForBSPEpairKey(int32 ent, const char* key, int32* value);

