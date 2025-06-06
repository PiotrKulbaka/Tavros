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
#include <qcommon/qcommon.h>
#include <game/q_shared.h>

// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories

// surface geometry should not exceed these limits
#define SHADER_MAX_VERTEXES 1000
#define SHADER_MAX_INDEXES  (6 * SHADER_MAX_VERTEXES)


// the maximum size of game relative pathnames
#define MAX_QPATH           64

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT           (('3' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD3_VERSION         15

// limits
#define MD3_MAX_LODS        3
#define MD3_MAX_TRIANGLES   8192 // per surface
#define MD3_MAX_VERTS       4096 // per surface
#define MD3_MAX_SHADERS     256  // per surface
#define MD3_MAX_FRAMES      1024 // per model
#define MD3_MAX_SURFACES    32   // per model
#define MD3_MAX_TAGS        16   // per frame

// vertex scales
#define MD3_XYZ_SCALE       (1.0 / 64)

typedef struct md3Frame_s
{
    vec3_t bounds[2];
    vec3_t localOrigin;
    float  radius;
    char   name[16];
} md3Frame_t;

typedef struct md3Tag_s
{
    char   name[MAX_QPATH]; // tag name
    vec3_t origin;
    vec3_t axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK            SIZE
** header            sizeof( md3Surface_t )
** shaders            sizeof( md3Shader_t ) * numShaders
** triangles[0]        sizeof( md3Triangle_t ) * numTriangles
** st                sizeof( md3St_t ) * numVerts
** XyzNormals        sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct
{
    int32 ident;          //

    char name[MAX_QPATH]; // polyset name

    int32 flags;
    int32 numFrames;  // all surfaces in a model should have the same

    int32 numShaders; // all surfaces in a model should have the same
    int32 numVerts;

    int32 numTriangles;
    int32 ofsTriangles;

    int32 ofsShaders;    // offset from start of md3Surface_t
    int32 ofsSt;         // texture coords are common for all frames
    int32 ofsXyzNormals; // numVerts * numFrames

    int32 ofsEnd;        // next surface follows
} md3Surface_t;

typedef struct
{
    char  name[MAX_QPATH];
    int32 shaderIndex; // for in-game use
} md3Shader_t;

typedef struct
{
    int32 indexes[3];
} md3Triangle_t;

typedef struct
{
    int16 xyz[3];
    int16 normal;
} md3XyzNormal_t;

typedef struct
{
    int32 ident;
    int32 version;

    char name[MAX_QPATH]; // model name

    int32 flags;

    int32 numFrames;
    int32 numTags;
    int32 numSurfaces;

    int32 numSkins;

    int32 ofsFrames;   // offset for first frame
    int32 ofsTags;     // numFrames * numTags
    int32 ofsSurfaces; // first surface, others follow

    int32 ofsEnd;      // end of file
} md3Header_t;

/*
==============================================================================

MD4 file format

==============================================================================
*/

#define MD4_IDENT     (('4' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD4_VERSION   1
#define MD4_MAX_BONES 128

typedef struct
{
    int32  boneIndex;  // these are indexes into the boneReferences,
    float  boneWeight; // not the global per-frame bone list
    vec3_t offset;
} md4Weight_t;

typedef struct
{
    vec3_t             normal;
    tavros::math::vec2 _texCoords;
    int32              numWeights;
    md4Weight_t        weights[1]; // variable sized
} md4Vertex_t;

typedef struct
{
    int32 indexes[3];
} md4Triangle_t;

typedef struct
{
    int32 ident;

    char  name[MAX_QPATH]; // polyset name
    char  shader[MAX_QPATH];
    int32 shaderIndex;     // for in-game use

    int32 ofsHeader;       // this will be a negative number

    int32 numVerts;
    int32 ofsVerts;

    int32 numTriangles;
    int32 ofsTriangles;

    // Bone references are a set of ints representing all the bones
    // present in any vertex weights for this surface.  This is
    // needed because a model may have surfaces that need to be
    // drawn at different sort times, and we don't want to have
    // to re-interpolate all the bones for each surface.
    int32 numBoneReferences;
    int32 ofsBoneReferences;

    int32 ofsEnd; // next surface follows
} md4Surface_t;

typedef struct
{
    float matrix[3][4];
} md4Bone_t;

typedef struct
{
    vec3_t    bounds[2];   // bounds of all surfaces of all LOD's for this frame
    vec3_t    localOrigin; // midpoint of bounds, used for sphere cull
    float     radius;      // dist from localOrigin to corner
    md4Bone_t bones[1];    // [numBones]
} md4Frame_t;

typedef struct
{
    int32 numSurfaces;
    int32 ofsSurfaces; // first surface, others follow
    int32 ofsEnd;      // next lod follows
} md4LOD_t;

typedef struct
{
    int32 ident;
    int32 version;

    char name[MAX_QPATH]; // model name

    // frames and bones are shared by all levels of detail
    int32 numFrames;
    int32 numBones;
    int32 ofsBoneNames; // char    name[ MAX_QPATH ]
    int32 ofsFrames;    // md4Frame_t[numFrames]

    // each level of detail has completely separate sets of surfaces
    int32 numLODs;
    int32 ofsLODs;

    int32 ofsEnd; // end of file
} md4Header_t;


/*
==============================================================================

  .BSP file format

==============================================================================
*/


#define BSP_IDENT            (('P' << 24) + ('S' << 16) + ('B' << 8) + 'I')
// little-endian "IBSP"

#define BSP_VERSION          46


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define MAX_MAP_MODELS       0x400
#define MAX_MAP_BRUSHES      0x8000
#define MAX_MAP_ENTITIES     0x800
#define MAX_MAP_ENTSTRING    0x40000
#define MAX_MAP_SHADERS      0x400

#define MAX_MAP_AREAS        0x100 // MAX_MAP_AREA_BYTES in q_shared must match!
#define MAX_MAP_FOGS         0x100
#define MAX_MAP_PLANES       0x20000
#define MAX_MAP_NODES        0x20000
#define MAX_MAP_BRUSHSIDES   0x20000
#define MAX_MAP_LEAFS        0x20000
#define MAX_MAP_LEAFFACES    0x20000
#define MAX_MAP_LEAFBRUSHES  0x40000
#define MAX_MAP_PORTALS      0x20000
#define MAX_MAP_LIGHTING     0x800000
#define MAX_MAP_LIGHTGRID    0x800000
#define MAX_MAP_VISIBILITY   0x200000

#define MAX_MAP_DRAW_SURFS   0x20000
#define MAX_MAP_DRAW_VERTS   0x80000
#define MAX_MAP_DRAW_INDEXES 0x80000


// key / value pair sizes in the entities lump
#define MAX_KEY              32
#define MAX_VALUE            1024

// the editor uses these predefined yaw angles to orient entities up or down
#define ANGLE_UP             -1
#define ANGLE_DOWN           -2

#define LIGHTMAP_WIDTH       128
#define LIGHTMAP_HEIGHT      128

#define MAX_WORLD_COORD      (128 * 1024)
#define MIN_WORLD_COORD      (-128 * 1024)
#define WORLD_SIZE           (MAX_WORLD_COORD - MIN_WORLD_COORD)

//=============================================================================


typedef struct
{
    int32 fileofs, filelen;
} lump_t;

#define LUMP_ENTITIES     0
#define LUMP_SHADERS      1
#define LUMP_PLANES       2
#define LUMP_NODES        3
#define LUMP_LEAFS        4
#define LUMP_LEAFSURFACES 5
#define LUMP_LEAFBRUSHES  6
#define LUMP_MODELS       7
#define LUMP_BRUSHES      8
#define LUMP_BRUSHSIDES   9
#define LUMP_DRAWVERTS    10
#define LUMP_DRAWINDEXES  11
#define LUMP_FOGS         12
#define LUMP_SURFACES     13
#define LUMP_LIGHTMAPS    14
#define LUMP_LIGHTGRID    15
#define LUMP_VISIBILITY   16
#define HEADER_LUMPS      17

typedef struct
{
    int32 ident;
    int32 version;

    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
    float mins[3], maxs[3];
    int32 firstSurface, numSurfaces;
    int32 firstBrush, numBrushes;
} dmodel_t;

typedef struct
{
    char  shader[MAX_QPATH];
    int32 surfaceFlags;
    int32 contentFlags;
} dshader_t;

// planes x^1 is allways the opposite of plane x

typedef struct
{
    float normal[3];
    float dist;
} dplane_t;

typedef struct
{
    int32 planeNum;
    int32 children[2]; // negative numbers are -(leafs+1), not nodes
    int32 mins[3];     // for frustom culling
    int32 maxs[3];
} dnode_t;

typedef struct
{
    int32 cluster; // -1 = opaque cluster (do I still store these?)
    int32 area;

    int32 mins[3]; // for frustum culling
    int32 maxs[3];

    int32 firstLeafSurface;
    int32 numLeafSurfaces;

    int32 firstLeafBrush;
    int32 numLeafBrushes;
} dleaf_t;

typedef struct
{
    int32 planeNum; // positive plane side faces out of the leaf
    int32 shaderNum;
} dbrushside_t;

typedef struct
{
    int32 firstSide;
    int32 numSides;
    int32 shaderNum; // the shader that determines the contents flags
} dbrush_t;

typedef struct
{
    char  shader[MAX_QPATH];
    int32 brushNum;
    int32 visibleSide; // the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

typedef struct
{
    vec3_t xyz;
    float  st[2];
    float  lightmap[2];
    vec3_t normal;
    uint8  color[4];
} drawVert_t;

typedef enum
{
    MST_BAD,
    MST_PLANAR,
    MST_PATCH,
    MST_TRIANGLE_SOUP,
    MST_FLARE
} mapSurfaceType_t;

typedef struct
{
    int32 shaderNum;
    int32 fogNum;
    int32 surfaceType;

    int32 firstVert;
    int32 numVerts;

    int32 firstIndex;
    int32 numIndexes;

    int32 lightmapNum;
    int32 lightmapX, lightmapY;
    int32 lightmapWidth, lightmapHeight;

    vec3_t lightmapOrigin;
    vec3_t lightmapVecs[3]; // for patches, [0] and [1] are lodbounds

    int32 patchWidth;
    int32 patchHeight;
} dsurface_t;
