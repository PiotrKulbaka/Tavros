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

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define MASK_SOLID CONTENTS_PLAYERCLIP

// FIXME: these might change
enum
{
    ET_GENERAL,
    ET_PLAYER,
    ET_ITEM,
    ET_MISSILE,
    ET_MOVER
};

int32 AAS_UpdateEntity(int32 entnum, bot_entitystate_t* state)
{
    int32         relink;
    aas_entity_t* ent;
    vec3_t        absmins, absmaxs;

    if (!aasworld.loaded) {
        botimport.Print(PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n");
        return BLERR_NOAASFILE;
    }

    ent = &aasworld.entities[entnum];

    if (!state) {
        // unlink the entity
        AAS_UnlinkFromAreas(ent->areas);
        // unlink the entity from the BSP leaves
        AAS_UnlinkFromBSPLeaves(ent->leaves);
        //
        ent->areas = NULL;
        //
        ent->leaves = NULL;
        return BLERR_NOERROR;
    }

    ent->i.update_time = AAS_Time() - ent->i.ltime;
    ent->i.type = state->type;
    ent->i.flags = state->flags;
    ent->i.ltime = AAS_Time();
    VectorCopy(ent->i.origin, ent->i.lastvisorigin);
    VectorCopy(state->old_origin, ent->i.old_origin);
    ent->i.solid = state->solid;
    ent->i.groundent = state->groundent;
    ent->i.modelindex = state->modelindex;
    ent->i.modelindex2 = state->modelindex2;
    ent->i.frame = state->frame;
    ent->i.event = state->event;
    ent->i.eventParm = state->eventParm;
    ent->i.powerups = state->powerups;
    ent->i.weapon = state->weapon;
    ent->i.legsAnim = state->legsAnim;
    ent->i.torsoAnim = state->torsoAnim;
    // number of the entity
    ent->i.number = entnum;
    // updated so set valid flag
    ent->i.valid = true;
    // link everything the first frame
    if (aasworld.numframes == 1) {
        relink = true;
    } else {
        relink = false;
    }
    //
    if (ent->i.solid == SOLID_BSP) {
        // if the angles of the model changed
        if (!VectorCompare(state->angles, ent->i.angles)) {
            VectorCopy(state->angles, ent->i.angles);
            relink = true;
        }
        // get the mins and maxs of the model
        // FIXME: rotate mins and maxs
        AAS_BSPModelMinsMaxsOrigin(ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs, NULL);
    } else if (ent->i.solid == SOLID_BBOX) {
        // if the bounding box size changed
        if (!VectorCompare(state->mins, ent->i.mins) || !VectorCompare(state->maxs, ent->i.maxs)) {
            VectorCopy(state->mins, ent->i.mins);
            VectorCopy(state->maxs, ent->i.maxs);
            relink = true;
        }
        VectorCopy(state->angles, ent->i.angles);
    }
    // if the origin changed
    if (!VectorCompare(state->origin, ent->i.origin)) {
        VectorCopy(state->origin, ent->i.origin);
        relink = true;
    }
    // if the entity should be relinked
    if (relink) {
        // don't link the world model
        if (entnum != ENTITYNUM_WORLD) {
            // absolute mins and maxs
            VectorAdd(ent->i.mins, ent->i.origin, absmins);
            VectorAdd(ent->i.maxs, ent->i.origin, absmaxs);
            // unlink the entity
            AAS_UnlinkFromAreas(ent->areas);
            // relink the entity to the AAS areas (use the larges bbox)
            ent->areas = AAS_LinkEntityClientBBox(absmins, absmaxs, entnum, PRESENCE_NORMAL);
            // unlink the entity from the BSP leaves
            AAS_UnlinkFromBSPLeaves(ent->leaves);
            // link the entity to the world BSP tree
            ent->leaves = AAS_BSPLinkEntity(absmins, absmaxs, entnum, 0);
        }
    }
    return BLERR_NOERROR;
}

void AAS_EntityInfo(int32 entnum, aas_entityinfo_t* info)
{
    if (!aasworld.initialized) {
        botimport.Print(PRT_FATAL, "AAS_EntityInfo: aasworld not initialized\n");
        Com_Memset(info, 0, sizeof(aas_entityinfo_t));
        return;
    }

    if (entnum < 0 || entnum >= aasworld.maxentities) {
        botimport.Print(PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum);
        Com_Memset(info, 0, sizeof(aas_entityinfo_t));
        return;
    }

    Com_Memcpy(info, &aasworld.entities[entnum].i, sizeof(aas_entityinfo_t));
}

int32 AAS_EntityModelindex(int32 entnum)
{
    if (entnum < 0 || entnum >= aasworld.maxentities) {
        botimport.Print(PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum);
        return 0;
    }
    return aasworld.entities[entnum].i.modelindex;
}

int32 AAS_EntityType(int32 entnum)
{
    if (!aasworld.initialized) {
        return 0;
    }

    if (entnum < 0 || entnum >= aasworld.maxentities) {
        botimport.Print(PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum);
        return 0;
    }
    return aasworld.entities[entnum].i.type;
}

int32 AAS_EntityModelNum(int32 entnum)
{
    if (!aasworld.initialized) {
        return 0;
    }

    if (entnum < 0 || entnum >= aasworld.maxentities) {
        botimport.Print(PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum);
        return 0;
    }
    return aasworld.entities[entnum].i.modelindex;
}

int32 AAS_OriginOfMoverWithModelNum(int32 modelnum, vec3_t origin)
{
    int32         i;
    aas_entity_t* ent;

    for (i = 0; i < aasworld.maxentities; i++) {
        ent = &aasworld.entities[i];
        if (ent->i.type == ET_MOVER) {
            if (ent->i.modelindex == modelnum) {
                VectorCopy(ent->i.origin, origin);
                return true;
            }
        }
    }
    return false;
}

void AAS_EntityBSPData(int32 entnum, bsp_entdata_t* entdata)
{
    aas_entity_t* ent;

    ent = &aasworld.entities[entnum];
    VectorCopy(ent->i.origin, entdata->origin);
    VectorCopy(ent->i.angles, entdata->angles);
    VectorAdd(ent->i.origin, ent->i.mins, entdata->absmins);
    VectorAdd(ent->i.origin, ent->i.maxs, entdata->absmaxs);
    entdata->solid = ent->i.solid;
    entdata->modelnum = ent->i.modelindex - 1;
}

void AAS_ResetEntityLinks()
{
    int32 i;
    for (i = 0; i < aasworld.maxentities; i++) {
        aasworld.entities[i].areas = NULL;
        aasworld.entities[i].leaves = NULL;
    }
}

void AAS_InvalidateEntities()
{
    int32 i;
    for (i = 0; i < aasworld.maxentities; i++) {
        aasworld.entities[i].i.valid = false;
        aasworld.entities[i].i.number = i;
    }
}

void AAS_UnlinkInvalidEntities()
{
    int32         i;
    aas_entity_t* ent;

    for (i = 0; i < aasworld.maxentities; i++) {
        ent = &aasworld.entities[i];
        if (!ent->i.valid) {
            AAS_UnlinkFromAreas(ent->areas);
            ent->areas = NULL;
            AAS_UnlinkFromBSPLeaves(ent->leaves);
            ent->leaves = NULL;
        }
    }
}

int32 AAS_NextEntity(int32 entnum)
{
    if (!aasworld.loaded) {
        return 0;
    }

    if (entnum < 0) {
        entnum = -1;
    }
    while (++entnum < aasworld.maxentities) {
        if (aasworld.entities[entnum].i.valid) {
            return entnum;
        }
    }
    return 0;
}
