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
// cmodel.c -- model loading

#include "cm_local.h"

static tavros::core::logger logger("cm_load");

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define BOX_BRUSHES 1
#define BOX_SIDES   6
#define BOX_LEAFS   2
#define BOX_PLANES  12

#define LL(x)       x = (x)


clipMap_t cm;
int32     c_pointcontents;
int32     c_traces, c_brush_traces, c_patch_traces;


uint8* cmod_base;

cvar_t* cm_noAreas;
cvar_t* cm_noCurves;
cvar_t* cm_playerCurveClip;

cmodel_t  box_model;
cplane_t* box_planes;
cbrush_t* box_brush;


void CM_InitBoxHull();
void CM_FloodAreaConnections();


/*
===============================================================================

                    MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders(lump_t* l)
{
    dshader_t *in, *out;
    int32      i, count;

    in = (dshader_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "CMod_LoadShaders: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    if (count < 1) {
        Com_Error(ERR_DROP, "Map with no shaders");
    }
    cm.shaders = (dshader_t*) Hunk_Alloc(count * sizeof(*cm.shaders), h_high);
    cm.numShaders = count;

    Com_Memcpy(cm.shaders, in, count * sizeof(*cm.shaders));

    out = cm.shaders;
    for (i = 0; i < count; i++, in++, out++) {
        out->contentFlags = (out->contentFlags);
        out->surfaceFlags = (out->surfaceFlags);
    }
}


/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels(lump_t* l)
{
    dmodel_t* in;
    cmodel_t* out;
    int32     i, j, count;
    int32*    indexes;

    in = (dmodel_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "CMod_LoadSubmodels: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    if (count < 1) {
        Com_Error(ERR_DROP, "Map with no models");
    }
    cm.cmodels = (cmodel_t*) Hunk_Alloc(count * sizeof(*cm.cmodels), h_high);
    cm.numSubModels = count;

    if (count > MAX_SUBMODELS) {
        Com_Error(ERR_DROP, "MAX_SUBMODELS exceeded");
    }

    for (i = 0; i < count; i++, in++, out++) {
        out = &cm.cmodels[i];

        for (j = 0; j < 3; j++) { // spread the mins / maxs by a pixel
            out->mins[j] = (in->mins[j]) - 1;
            out->maxs[j] = (in->maxs[j]) + 1;
        }

        if (i == 0) {
            continue; // world model doesn't need other info
        }

        // make a "leaf" just to hold the model's brushes and surfaces
        out->leaf.numLeafBrushes = (in->numBrushes);
        indexes = (int32*) Hunk_Alloc(out->leaf.numLeafBrushes * 4, h_high);
        out->leaf.firstLeafBrush = indexes - cm.leafbrushes;
        for (j = 0; j < out->leaf.numLeafBrushes; j++) {
            indexes[j] = (in->firstBrush) + j;
        }

        out->leaf.numLeafSurfaces = (in->numSurfaces);
        indexes = (int32*) Hunk_Alloc(out->leaf.numLeafSurfaces * 4, h_high);
        out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;
        for (j = 0; j < out->leaf.numLeafSurfaces; j++) {
            indexes[j] = (in->firstSurface) + j;
        }
    }
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes(lump_t* l)
{
    dnode_t* in;
    int32    child;
    cNode_t* out;
    int32    i, j, count;

    in = (dnode_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    if (count < 1) {
        Com_Error(ERR_DROP, "Map has no nodes");
    }
    cm.nodes = (cNode_t*) Hunk_Alloc(count * sizeof(*cm.nodes), h_high);
    cm.numNodes = count;

    out = cm.nodes;

    for (i = 0; i < count; i++, out++, in++) {
        out->plane = cm.planes + (in->planeNum);
        for (j = 0; j < 2; j++) {
            child = (in->children[j]);
            out->children[j] = child;
        }
    }
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush(cbrush_t* b)
{
    b->bounds[0][0] = -b->sides[0].plane->dist;
    b->bounds[1][0] = b->sides[1].plane->dist;

    b->bounds[0][1] = -b->sides[2].plane->dist;
    b->bounds[1][1] = b->sides[3].plane->dist;

    b->bounds[0][2] = -b->sides[4].plane->dist;
    b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes(lump_t* l)
{
    dbrush_t* in;
    cbrush_t* out;
    int32     i, count;

    in = (dbrush_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    cm.brushes = (cbrush_t*) Hunk_Alloc((BOX_BRUSHES + count) * sizeof(*cm.brushes), h_high);
    cm.numBrushes = count;

    out = cm.brushes;

    for (i = 0; i < count; i++, out++, in++) {
        out->sides = cm.brushsides + (in->firstSide);
        out->numsides = (in->numSides);

        out->shaderNum = (in->shaderNum);
        if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders) {
            Com_Error(ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum);
        }
        out->contents = cm.shaders[out->shaderNum].contentFlags;

        CM_BoundBrush(out);
    }
}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs(lump_t* l)
{
    int32    i;
    cLeaf_t* out;
    dleaf_t* in;
    int32    count;

    in = (dleaf_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    if (count < 1) {
        Com_Error(ERR_DROP, "Map with no leafs");
    }

    cm.leafs = (cLeaf_t*) Hunk_Alloc((BOX_LEAFS + count) * sizeof(*cm.leafs), h_high);
    cm.numLeafs = count;

    out = cm.leafs;
    for (i = 0; i < count; i++, in++, out++) {
        out->cluster = (in->cluster);
        out->area = (in->area);
        out->firstLeafBrush = (in->firstLeafBrush);
        out->numLeafBrushes = (in->numLeafBrushes);
        out->firstLeafSurface = (in->firstLeafSurface);
        out->numLeafSurfaces = (in->numLeafSurfaces);

        if (out->cluster >= cm.numClusters) {
            cm.numClusters = out->cluster + 1;
        }
        if (out->area >= cm.numAreas) {
            cm.numAreas = out->area + 1;
        }
    }

    cm.areas = (cArea_t*) Hunk_Alloc(cm.numAreas * sizeof(*cm.areas), h_high);
    cm.areaPortals = (int32*) Hunk_Alloc(cm.numAreas * cm.numAreas * sizeof(*cm.areaPortals), h_high);
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes(lump_t* l)
{
    int32     i, j;
    cplane_t* out;
    dplane_t* in;
    int32     count;
    int32     bits;

    in = (dplane_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    if (count < 1) {
        Com_Error(ERR_DROP, "Map with no planes");
    }
    cm.planes = (cplane_t*) Hunk_Alloc((BOX_PLANES + count) * sizeof(*cm.planes), h_high);
    cm.numPlanes = count;

    out = cm.planes;

    for (i = 0; i < count; i++, in++, out++) {
        bits = 0;
        for (j = 0; j < 3; j++) {
            out->normal[j] = (in->normal[j]);
            if (out->normal[j] < 0) {
                bits |= 1 << j;
            }
        }

        out->dist = (in->dist);
        out->side_type = PlaneTypeForNormal(out->normal);
        out->signbits = bits;
    }
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes(lump_t* l)
{
    int32  i;
    int32* out;
    int32* in;
    int32  count;

    in = (int32*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    cm.leafbrushes = (int32*) Hunk_Alloc((count + BOX_BRUSHES) * sizeof(*cm.leafbrushes), h_high);
    cm.numLeafBrushes = count;

    out = cm.leafbrushes;

    for (i = 0; i < count; i++, in++, out++) {
        *out = (*in);
    }
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces(lump_t* l)
{
    int32  i;
    int32* out;
    int32* in;
    int32  count;

    in = (int32*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    cm.leafsurfaces = (int32*) Hunk_Alloc(count * sizeof(*cm.leafsurfaces), h_high);
    cm.numLeafSurfaces = count;

    out = cm.leafsurfaces;

    for (i = 0; i < count; i++, in++, out++) {
        *out = (*in);
    }
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides(lump_t* l)
{
    int32         i;
    cbrushside_t* out;
    dbrushside_t* in;
    int32         count;
    int32         num;

    in = (dbrushside_t*) (cmod_base + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    count = l->filelen / sizeof(*in);

    cm.brushsides = (cbrushside_t*) Hunk_Alloc((BOX_SIDES + count) * sizeof(*cm.brushsides), h_high);
    cm.numBrushSides = count;

    out = cm.brushsides;

    for (i = 0; i < count; i++, in++, out++) {
        num = (in->planeNum);
        out->plane = &cm.planes[num];
        out->shaderNum = (in->shaderNum);
        if (out->shaderNum < 0 || out->shaderNum >= cm.numShaders) {
            Com_Error(ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum);
        }
        out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
    }
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString(lump_t* l)
{
    cm.entityString = (char*) Hunk_Alloc(l->filelen, h_high);
    cm.numEntityChars = l->filelen;
    Com_Memcpy(cm.entityString, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadVisibility
=================
*/
#define VIS_HEADER 8
void CMod_LoadVisibility(lump_t* l)
{
    int32  len;
    uint8* buf;

    len = l->filelen;
    if (!len) {
        cm.clusterBytes = (cm.numClusters + 31) & ~31;
        cm.visibility = (uint8*) Hunk_Alloc(cm.clusterBytes, h_high);
        Com_Memset(cm.visibility, 255, cm.clusterBytes);
        return;
    }
    buf = cmod_base + l->fileofs;

    cm.vised = true;
    cm.visibility = (uint8*) Hunk_Alloc(len, h_high);
    cm.numClusters = (((int32*) buf)[0]);
    cm.clusterBytes = (((int32*) buf)[1]);
    Com_Memcpy(cm.visibility, buf + VIS_HEADER, len - VIS_HEADER);
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
#define MAX_PATCH_VERTS 1024
void CMod_LoadPatches(lump_t* surfs, lump_t* verts)
{
    drawVert_t *dv, *dv_p;
    dsurface_t* in;
    int32       count;
    int32       i, j;
    int32       c;
    cPatch_t*   patch;
    vec3_t      points[MAX_PATCH_VERTS];
    int32       width, height;
    int32       shaderNum;

    in = (dsurface_t*) (cmod_base + surfs->fileofs);
    if (surfs->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }
    cm.numSurfaces = count = surfs->filelen / sizeof(*in);
    cm.surfaces = (cPatch_t**) Hunk_Alloc(cm.numSurfaces * sizeof(cm.surfaces[0]), h_high);

    dv = (drawVert_t*) (cmod_base + verts->fileofs);
    if (verts->filelen % sizeof(*dv)) {
        Com_Error(ERR_DROP, "MOD_LoadBmodel: funny lump size");
    }

    // scan through all the surfaces, but only load patches,
    // not planar faces
    for (i = 0; i < count; i++, in++) {
        if ((in->surfaceType) != MST_PATCH) {
            continue; // ignore other surfaces
        }
        // FIXME: check for non-colliding patches

        cm.surfaces[i] = patch = (cPatch_t*) Hunk_Alloc(sizeof(*patch), h_high);

        // load the full drawverts onto the stack
        width = (in->patchWidth);
        height = (in->patchHeight);
        c = width * height;
        if (c > MAX_PATCH_VERTS) {
            Com_Error(ERR_DROP, "ParseMesh: MAX_PATCH_VERTS");
        }

        dv_p = dv + (in->firstVert);
        for (j = 0; j < c; j++, dv_p++) {
            points[j][0] = (dv_p->xyz[0]);
            points[j][1] = (dv_p->xyz[1]);
            points[j][2] = (dv_p->xyz[2]);
        }

        shaderNum = (in->shaderNum);
        patch->contents = cm.shaders[shaderNum].contentFlags;
        patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

        // create the internal facet structure
        patch->pc = CM_GeneratePatchCollide(width, height, points);
    }
}

//==================================================================

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap(const char* name, bool clientload, int32* checksum)
{
    int32*        buf;
    int32         i;
    dheader_t     header;
    int32         length;
    static uint32 last_checksum;

    if (!name || !name[0]) {
        Com_Error(ERR_DROP, "CM_LoadMap: NULL name");
    }

    cm_noAreas = Cvar_Get("cm_noAreas", "0", CVAR_CHEAT);
    cm_noCurves = Cvar_Get("cm_noCurves", "0", CVAR_CHEAT);
    cm_playerCurveClip = Cvar_Get("cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT);

    logger.debug("CM_LoadMap( %s, %i )", name, clientload);

    if (!strcmp(cm.name, name) && clientload) {
        *checksum = last_checksum;
        return;
    }

    // free old stuff
    Com_Memset(&cm, 0, sizeof(cm));
    CM_ClearLevelPatches();

    if (!name[0]) {
        cm.numLeafs = 1;
        cm.numClusters = 1;
        cm.numAreas = 1;
        cm.cmodels = (cmodel_t*) Hunk_Alloc(sizeof(*cm.cmodels), h_high);
        *checksum = 0;
        return;
    }

    //
    // load the file
    //
    length = FS_ReadFile(name, (void**) &buf);

    if (!buf) {
        Com_Error(ERR_DROP, "Couldn't load %s", name);
    }

    last_checksum = (Com_BlockChecksum(buf, length));
    *checksum = last_checksum;

    header = *(dheader_t*) buf;
    for (i = 0; i < sizeof(dheader_t) / 4; i++) {
        ((int32*) &header)[i] = (((int32*) &header)[i]);
    }

    if (header.version != BSP_VERSION) {
        Com_Error(ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)", name, header.version, BSP_VERSION);
    }

    cmod_base = (uint8*) buf;

    // load into heap
    CMod_LoadShaders(&header.lumps[LUMP_SHADERS]);
    CMod_LoadLeafs(&header.lumps[LUMP_LEAFS]);
    CMod_LoadLeafBrushes(&header.lumps[LUMP_LEAFBRUSHES]);
    CMod_LoadLeafSurfaces(&header.lumps[LUMP_LEAFSURFACES]);
    CMod_LoadPlanes(&header.lumps[LUMP_PLANES]);
    CMod_LoadBrushSides(&header.lumps[LUMP_BRUSHSIDES]);
    CMod_LoadBrushes(&header.lumps[LUMP_BRUSHES]);
    CMod_LoadSubmodels(&header.lumps[LUMP_MODELS]);
    CMod_LoadNodes(&header.lumps[LUMP_NODES]);
    CMod_LoadEntityString(&header.lumps[LUMP_ENTITIES]);
    CMod_LoadVisibility(&header.lumps[LUMP_VISIBILITY]);
    CMod_LoadPatches(&header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS]);

    // we are NOT freeing the file, because it is cached for the ref
    FS_FreeFile(buf);

    CM_InitBoxHull();

    CM_FloodAreaConnections();

    // allow this to be cached if it is loaded by the server
    if (!clientload) {
        Q_strncpyz(cm.name, name, sizeof(cm.name));
    }
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap()
{
    Com_Memset(&cm, 0, sizeof(cm));
    CM_ClearLevelPatches();
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t* CM_ClipHandleToModel(clipHandle_t handle)
{
    if (handle < 0) {
        Com_Error(ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle);
    }
    if (handle < cm.numSubModels) {
        return &cm.cmodels[handle];
    }
    if (handle == BOX_MODEL_HANDLE) {
        return &box_model;
    }
    if (handle < MAX_SUBMODELS) {
        Com_Error(ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i", cm.numSubModels, handle, MAX_SUBMODELS);
    }
    Com_Error(ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS);

    return NULL;
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t CM_InlineModel(int32 index)
{
    if (index < 0 || index >= cm.numSubModels) {
        Com_Error(ERR_DROP, "CM_InlineModel: bad number");
    }
    return index;
}

int32 CM_NumInlineModels()
{
    return cm.numSubModels;
}

char* CM_EntityString()
{
    return cm.entityString;
}

int32 CM_LeafCluster(int32 leafnum)
{
    if (leafnum < 0 || leafnum >= cm.numLeafs) {
        Com_Error(ERR_DROP, "CM_LeafCluster: bad number");
    }
    return cm.leafs[leafnum].cluster;
}

int32 CM_LeafArea(int32 leafnum)
{
    if (leafnum < 0 || leafnum >= cm.numLeafs) {
        Com_Error(ERR_DROP, "CM_LeafArea: bad number");
    }
    return cm.leafs[leafnum].area;
}

//=======================================================================


/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull()
{
    int32         i;
    int32         side;
    cplane_t*     p;
    cbrushside_t* s;

    box_planes = &cm.planes[cm.numPlanes];

    box_brush = &cm.brushes[cm.numBrushes];
    box_brush->numsides = 6;
    box_brush->sides = cm.brushsides + cm.numBrushSides;
    box_brush->contents = CONTENTS_BODY;

    box_model.leaf.numLeafBrushes = 1;
    //    box_model.leaf.firstLeafBrush = cm.numBrushes;
    box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
    cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;

    for (i = 0; i < 6; i++) {
        side = i & 1;

        // brush sides
        s = &cm.brushsides[cm.numBrushSides + i];
        s->plane = cm.planes + (cm.numPlanes + i * 2 + side);
        s->surfaceFlags = 0;

        // planes
        p = &box_planes[i * 2];
        p->side_type = i >> 1;
        p->signbits = 0;
        VectorClear(p->normal);
        p->normal[i >> 1] = 1;

        p = &box_planes[i * 2 + 1];
        p->side_type = 3 + (i >> 1);
        p->signbits = 0;
        VectorClear(p->normal);
        p->normal[i >> 1] = -1;

        SetPlaneSignbits(p);
    }
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel(const vec3_t mins, const vec3_t maxs, int32 capsule)
{
    VectorCopy(mins, box_model.mins);
    VectorCopy(maxs, box_model.maxs);

    if (capsule) {
        return CAPSULE_MODEL_HANDLE;
    }

    box_planes[0].dist = maxs[0];
    box_planes[1].dist = -maxs[0];
    box_planes[2].dist = mins[0];
    box_planes[3].dist = -mins[0];
    box_planes[4].dist = maxs[1];
    box_planes[5].dist = -maxs[1];
    box_planes[6].dist = mins[1];
    box_planes[7].dist = -mins[1];
    box_planes[8].dist = maxs[2];
    box_planes[9].dist = -maxs[2];
    box_planes[10].dist = mins[2];
    box_planes[11].dist = -mins[2];

    VectorCopy(mins, box_brush->bounds[0]);
    VectorCopy(maxs, box_brush->bounds[1]);

    return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs)
{
    cmodel_t* cmod;

    cmod = CM_ClipHandleToModel(model);
    VectorCopy(cmod->mins, mins);
    VectorCopy(cmod->maxs, maxs);
}


