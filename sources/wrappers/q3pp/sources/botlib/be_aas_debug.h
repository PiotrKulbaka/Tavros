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

// clear the shown debug lines
void AAS_ClearShownDebugLines();
//
void AAS_ClearShownPolygons();
// show a debug line
void AAS_DebugLine(vec3_t start, vec3_t end, int32 color);
// show a permenent line
void AAS_PermanentLine(vec3_t start, vec3_t end, int32 color);
//
void AAS_ShowAreaPolygons(int32 areanum, int32 color, int32 groundfacesonly);
// draw a cros
void AAS_DrawCross(vec3_t origin, float size, int32 color);
// print the travel type
void AAS_PrintTravelType(int32 traveltype);
// draw an arrow
void AAS_DrawArrow(vec3_t start, vec3_t end, int32 linecolor, int32 arrowcolor);
// visualize the given reachability
void AAS_ShowReachability(struct aas_reachability_s* reach);
// show the reachable areas from the given area
void AAS_ShowReachableAreas(int32 areanum);

