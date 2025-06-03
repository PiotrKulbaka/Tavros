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

#include "../game/q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"

#define MAX_SUBMODELS        256
#define BOX_MODEL_HANDLE     255
#define CAPSULE_MODEL_HANDLE 254


typedef struct
{
    cplane_t* plane;
    int32     children[2]; // negative numbers are leafs
} cNode_t;

typedef struct
{
    int32 cluster;
    int32 area;

    int32 firstLeafBrush;
    int32 numLeafBrushes;

    int32 firstLeafSurface;
    int32 numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s
{
    vec3_t  mins, maxs;
    cLeaf_t leaf; // submodels don't reference the main tree
} cmodel_t;

typedef struct
{
    cplane_t* plane;
    int32     surfaceFlags;
    int32     shaderNum;
} cbrushside_t;

typedef struct
{
    int32         shaderNum; // the shader that determined the contents
    int32         contents;
    vec3_t        bounds[2];
    int32         numsides;
    cbrushside_t* sides;
    int32         checkcount; // to avoid repeated testings
} cbrush_t;


typedef struct
{
    int32                  checkcount; // to avoid repeated testings
    int32                  surfaceFlags;
    int32                  contents;
    struct patchCollide_s* pc;
} cPatch_t;


typedef struct
{
    int32 floodnum;
    int32 floodvalid;
} cArea_t;

typedef struct
{
    char name[MAX_QPATH];

    int32      numShaders;
    dshader_t* shaders;

    int32         numBrushSides;
    cbrushside_t* brushsides;

    int32     numPlanes;
    cplane_t* planes;

    int32    numNodes;
    cNode_t* nodes;

    int32    numLeafs;
    cLeaf_t* leafs;

    int32  numLeafBrushes;
    int32* leafbrushes;

    int32  numLeafSurfaces;
    int32* leafsurfaces;

    int32     numSubModels;
    cmodel_t* cmodels;

    int32     numBrushes;
    cbrush_t* brushes;

    int32  numClusters;
    int32  clusterBytes;
    uint8* visibility;
    bool   vised; // if false, visibility is just a single cluster of ffs

    int32 numEntityChars;
    char* entityString;

    int32    numAreas;
    cArea_t* areas;
    int32*   areaPortals; // [ numAreas*numAreas ] reference counts

    int32      numSurfaces;
    cPatch_t** surfaces; // non-patches will be NULL

    int32 floodvalid;
    int32 checkcount; // incremented on each trace
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON (0.125)

extern clipMap_t cm;
extern int32     c_pointcontents;
extern int32     c_traces, c_brush_traces, c_patch_traces;
extern cvar_t*   cm_noAreas;
extern cvar_t*   cm_noCurves;
extern cvar_t*   cm_playerCurveClip;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct
{
    bool   use;
    float  radius;
    float  halfheight;
    vec3_t offset;
} sphere_t;

typedef struct
{
    vec3_t   start;
    vec3_t   end;
    vec3_t   size[2];     // size of the box being swept through the model
    vec3_t   offsets[8];  // [signbits][x] = either size[0][x] or size[1][x]
    float    maxOffset;   // longest corner length from origin
    vec3_t   extents;     // greatest of abs(size[0]) and abs(size[1])
    vec3_t   bounds[2];   // enclosing box of start and end surrounding by size
    vec3_t   modelOrigin; // origin of the model tracing through
    int32    contents;    // ored contents of the model tracing through
    bool     isPoint;     // optimized case
    trace_t  trace;       // returned from trace call
    sphere_t sphere;      // sphere for oriendted capsule collision
} traceWork_t;

typedef struct leafList_s
{
    int32  count;
    int32  maxcount;
    bool   overflowed;
    int32* list;
    vec3_t bounds[2];
    int32  lastLeaf; // for overflows where each leaf can't be stored individually
    void   (*storeLeafs)(struct leafList_s* ll, int32 nodenum);
} leafList_t;

void CM_StoreLeafs(leafList_t* ll, int32 nodenum);

void CM_BoxLeafnums_r(leafList_t* ll, int32 nodenum);

cmodel_t* CM_ClipHandleToModel(clipHandle_t handle);

// cm_patch.c

struct patchCollide_s* CM_GeneratePatchCollide(int32 width, int32 height, vec3_t* points);
void                   CM_TraceThroughPatchCollide(traceWork_t* tw, const struct patchCollide_s* pc);
bool                   CM_PositionTestInPatchCollide(traceWork_t* tw, const struct patchCollide_s* pc);
void                   CM_ClearLevelPatches();
