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
// #define    CULL_BBOX

/*

This file does not reference any globals, and has these entry points:

void CM_ClearLevelPatches( void );
struct patchCollide_s    *CM_GeneratePatchCollide( int32 width, int32 height, const vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
bool CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_DrawDebugSurface( void (*drawPoly)(int32 color, int32 numPoints, flaot *points) );


Issues for collision against curved surfaces:

Surface edges need to be handled differently than surface planes

Plane expansion causes raw surfaces to expand past expanded bounding box

Position test of a volume against a surface is tricky.

Position test of a point against a surface is not well defined, because the surface has no volume.


Tracing leading edge points instead of volumes?
Position test by tracing corner to corner? (8*7 traces -- ouch)

coplanar edges
triangulated patches
degenerate patches

  endcaps
  degenerate

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/

#include <tavros/core/types.hpp>
#include <game/q_shared.h>

#define MAX_FACETS       1024
#define MAX_PATCH_PLANES 2048

typedef struct
{
    float plane[4];
    int32 signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
} patchPlane_t;

typedef struct
{
    int32 surfacePlane;
    int32 numBorders; // 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
    int32 borderPlanes[4 + 6 + 16];
    int32 borderInward[4 + 6 + 16];
    bool  borderNoAdjust[4 + 6 + 16];
} facet_t;

typedef struct patchCollide_s
{
    vec3_t        bounds[2];
    int32         numPlanes; // surface planes plus edge planes
    patchPlane_t* planes;
    int32         numFacets;
    facet_t*      facets;
} patchCollide_t;


#define MAX_GRID_SIZE 129

typedef struct
{
    int32  width;
    int32  height;
    bool   wrapWidth;
    bool   wrapHeight;
    vec3_t points[MAX_GRID_SIZE][MAX_GRID_SIZE]; // [width][height]
} cGrid_t;

#define SUBDIVIDE_DISTANCE 16 // 4    // never more than this units away from curve
#define PLANE_TRI_EPSILON  0.1
#define WRAP_POINT_EPSILON 0.1


struct patchCollide_s* CM_GeneratePatchCollide(int32 width, int32 height, vec3_t* points);
