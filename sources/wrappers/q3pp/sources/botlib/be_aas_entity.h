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
#include <game/be_aas.h>
#include <game/botlib.h>

#include <botlib/be_aas_def.h>

#ifdef AASINTERN
// invalidates all entity infos
void AAS_InvalidateEntities();
// unlink not updated entities
void AAS_UnlinkInvalidEntities();
// resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks();
// updates an entity
int32 AAS_UpdateEntity(int32 ent, bot_entitystate_t* state);
// gives the entity data used for collision detection
void AAS_EntityBSPData(int32 entnum, bsp_entdata_t* entdata);
#endif // AASINTERN

// returns the BSP model number of the entity
int32 AAS_EntityModelNum(int32 entnum);
// returns the origin of an entity with the given model number
int32 AAS_OriginOfMoverWithModelNum(int32 modelnum, vec3_t origin);
// returns the info of the given entity
void AAS_EntityInfo(int32 entnum, aas_entityinfo_t* info);
// returns the next entity
int32 AAS_NextEntity(int32 entnum);
// returns the entity type
int32 AAS_EntityType(int32 entnum);
// returns the model index of the entity
int32 AAS_EntityModelindex(int32 entnum);

