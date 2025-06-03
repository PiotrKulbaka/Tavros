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
// initialize the AAS routing
void AAS_InitRouting();
// free the AAS routing caches
void AAS_FreeRoutingCaches();
// returns the travel time from start to end in the given area
uint16 AAS_AreaTravelTime(int32 areanum, vec3_t start, vec3_t end);
//
void AAS_CreateAllRoutingCache();
void AAS_WriteRouteCache();
//
void AAS_RoutingInfo();
#endif // AASINTERN

// returns the travel flag for the given travel type
int32 AAS_TravelFlagForType(int32 traveltype);
// return the travel flag(s) for traveling through this area
int32 AAS_AreaContentsTravelFlags(int32 areanum);
// returns the index of the next reachability for the given area
int32 AAS_NextAreaReachability(int32 areanum, int32 reachnum);
// returns the reachability with the given index
void AAS_ReachabilityFromNum(int32 num, struct aas_reachability_s* reach);
// enable or disable an area for routing
int32 AAS_EnableRoutingArea(int32 areanum, int32 enable);
// returns the travel time within the given area from start to end
uint16 AAS_AreaTravelTime(int32 areanum, vec3_t start, vec3_t end);
// returns the travel time from the area to the goal area using the given travel flags
int32 AAS_AreaTravelTimeToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags);
// predict a route up to a stop event
int32 AAS_PredictRoute(struct aas_predictroute_s* route, int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32 maxareas, int32 maxtime, int32 stopevent, int32 stopcontents, int32 stoptfl, int32 stopareanum);


