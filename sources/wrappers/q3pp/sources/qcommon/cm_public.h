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

#include "qfiles.h"

#include <tavros/core/types.hpp>
#include <qcommon/qcommon.h>
#include <game/q_shared.h>


void         CM_LoadMap(const char* name, bool clientload, int32* checksum);
void         CM_ClearMap();
clipHandle_t CM_InlineModel(int32 index); // 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel(const vec3_t mins, const vec3_t maxs, int32 capsule);

void CM_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);

int32 CM_NumInlineModels();
char* CM_EntityString();

// returns an ORed contents mask
int32 CM_PointContents(const vec3_t p, clipHandle_t model);
int32 CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);

void CM_BoxTrace(trace_t* results, const vec3_t start, const vec3_t end, vec3_t mins, vec3_t maxs, clipHandle_t model, int32 brushmask, int32 capsule);
void CM_TransformedBoxTrace(trace_t* results, const vec3_t start, const vec3_t end, vec3_t mins, vec3_t maxs, clipHandle_t model, int32 brushmask, const vec3_t origin, const vec3_t angles, int32 capsule);

uint8* CM_ClusterPVS(int32 cluster);

int32 CM_PointLeafnum(const vec3_t p);

// only returns non-solid leafs
// overflow if return listsize and if *lastLeaf != list[listsize-1]
int32 CM_BoxLeafnums(const vec3_t mins, const vec3_t maxs, int32* list, int32 listsize, int32* lastLeaf);

int32 CM_LeafCluster(int32 leafnum);
int32 CM_LeafArea(int32 leafnum);

void CM_AdjustAreaPortalState(int32 area1, int32 area2, bool open);
bool CM_AreasConnected(int32 area1, int32 area2);

int32 CM_WriteAreaBits(uint8* buffer, int32 area);
