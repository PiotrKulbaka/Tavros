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

#include <botlib/be_aas_def.h>


#include <tavros/core/types.hpp>
#include <game/q_shared.h>

#ifdef AASINTERN
// initialize calculating the reachabilities
void AAS_InitReachability();
// continue calculating the reachabilities
int32 AAS_ContinueInitReachability(float time);
//
int32 AAS_BestReachableLinkArea(aas_link_t* areas);
#endif // AASINTERN

// returns true if the are has reachabilities to other areas
int32 AAS_AreaReachability(int32 areanum);
// returns the best reachable area and goal origin for a bounding box at the given origin
int32 AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin);
// returns the best jumppad area from which the bbox at origin is reachable
int32 AAS_BestReachableFromJumpPadArea(vec3_t origin, vec3_t mins, vec3_t maxs);
// returns the next reachability using the given model
int32 AAS_NextModelReachability(int32 num, int32 modelnum);
// returns true if the area is crouch only
int32 AAS_AreaCrouch(int32 areanum);
// returns true if a player can swim in this area
int32 AAS_AreaSwim(int32 areanum);
// returns true if the area has one or more ground faces
int32 AAS_AreaGrounded(int32 areanum);
// returns true if the area has one or more ladder faces
int32 AAS_AreaLadder(int32 areanum);
// returns true if the area is a jump pad
int32 AAS_AreaJumpPad(int32 areanum);
// returns true if the area is donotenter
int32 AAS_AreaDoNotEnter(int32 areanum);
