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
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "tr_public.h"
#include "qgl.h"

#include <tavros/core/math.hpp>

#define GL_INDEX_TYPE         GL_UNSIGNED_INT

#define myftol(x)             ((int32) (x))


// 12 bits
// see QSORT_SHADERNUM_SHIFT
#define MAX_SHADERS           16384

// #define MAX_SHADER_STATES 2048
#define MAX_STATES_PER_SHADER 32
#define MAX_STATE_NAME        32

// can't be increased without changing bit packing for drawsurfs

using texture_type = uint64;

typedef struct dlight_s
{
    vec3_t origin;
    vec3_t color; // range from 0.0 to 1.0, should be color normalized
    float  radius;

    vec3_t transformed; // origin in local coordinate system
    int32  additive;    // texture detail is lost tho when the lightmap is dark
} dlight_t;


// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct
{
    refEntity_t e;

    float axisLength;       // compensate for non-normalized axis

    bool   needDlights;     // true for bmodels that touch a dlight
    bool   lightingCalculated;
    vec3_t lightDir;        // normalized direction towards light
    vec3_t ambientLight;    // color normalized to 0-255
    int32  ambientLightInt; // 32 bit rgba packed
    vec3_t directedLight;
} trRefEntity_t;


typedef struct
{
    vec3_t origin;     // in world coordinates
    vec3_t axis[3];    // orientation in world
    vec3_t viewOrigin; // viewParms->or.origin in local coordinates
    float  modelMatrix[16];
} orientationr_t;

//===============================================================================

typedef enum
{
    SS_BAD,
    SS_PORTAL,      // mirrors, portals, viewscreens
    SS_ENVIRONMENT, // sky box
    SS_OPAQUE,      // opaque

    SS_DECAL,       // scorch marks, etc.
    SS_SEE_THROUGH, // ladders, grates, grills that may have small blended edges
                    // in addition to alpha test
    SS_BANNER,

    SS_FOG,

    SS_UNDERWATER, // for items that should be drawn in front of the water plane

    SS_BLEND0,     // regular transparency and filters
    SS_BLEND1,     // generally only used for additive type effects
    SS_BLEND2,
    SS_BLEND3,

    SS_BLEND6,
    SS_ALMOST_NEAREST, // gun smoke puffs

    SS_NEAREST         // blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum
{
    GF_NONE,

    GF_SIN,
    GF_SQUARE,
    GF_TRIANGLE,
    GF_SAWTOOTH,
    GF_INVERSE_SAWTOOTH,

    GF_NOISE

} genFunc_t;


typedef enum
{
    DEFORM_NONE,
    DEFORM_WAVE,
    DEFORM_NORMALS,
    DEFORM_BULGE,
    DEFORM_MOVE,
    DEFORM_AUTOSPRITE,
    DEFORM_AUTOSPRITE2,
    DEFORM_TEXT0,
    DEFORM_TEXT1,
    DEFORM_TEXT2,
    DEFORM_TEXT3,
    DEFORM_TEXT4,
    DEFORM_TEXT5,
    DEFORM_TEXT6,
    DEFORM_TEXT7
} deform_t;

typedef enum
{
    AGEN_IDENTITY,
    AGEN_SKIP,
    AGEN_ENTITY,
    AGEN_ONE_MINUS_ENTITY,
    AGEN_VERTEX,
    AGEN_ONE_MINUS_VERTEX,
    AGEN_LIGHTING_SPECULAR,
    AGEN_WAVEFORM,
    AGEN_PORTAL,
    AGEN_CONST
} alphaGen_t;

typedef enum
{
    CGEN_BAD,
    CGEN_IDENTITY_LIGHTING, // tr.identityLight
    CGEN_IDENTITY,          // always (1,1,1,1)
    CGEN_ENTITY,            // grabbed from entity's modulate field
    CGEN_ONE_MINUS_ENTITY,  // grabbed from 1 - entity.modulate
    CGEN_EXACT_VERTEX,      // tess.vertexColors
    CGEN_VERTEX,            // tess.vertexColors * tr.identityLight
    CGEN_ONE_MINUS_VERTEX,
    CGEN_WAVEFORM,          // programmatically generated
    CGEN_LIGHTING_DIFFUSE,
    CGEN_FOG,               // standard fog
    CGEN_CONST              // fixed color
} colorGen_t;

typedef enum
{
    TCGEN_BAD,
    TCGEN_IDENTITY, // clear to 0,0
    TCGEN_LIGHTMAP,
    TCGEN_TEXTURE,
    TCGEN_ENVIRONMENT_MAPPED,
    TCGEN_FOG,
    TCGEN_VECTOR // S and T from world coordinates
} texCoordGen_t;

typedef enum
{
    ACFF_NONE,
    ACFF_MODULATE_RGB,
    ACFF_MODULATE_RGBA,
    ACFF_MODULATE_ALPHA
} acff_t;

typedef struct
{
    genFunc_t func;

    float base;
    float amplitude;
    float phase;
    float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum
{
    TMOD_NONE,
    TMOD_TRANSFORM,
    TMOD_TURBULENT,
    TMOD_SCROLL,
    TMOD_SCALE,
    TMOD_STRETCH,
    TMOD_ROTATE,
    TMOD_ENTITY_TRANSLATE
} texMod_t;

#define MAX_SHADER_DEFORMS 3
typedef struct
{
    deform_t deformation; // vertex coordinate modification type

    vec3_t     moveVector;
    waveForm_t deformationWave;
    float      deformationSpread;

    float bulgeWidth;
    float bulgeHeight;
    float bulgeSpeed;
} deformStage_t;


typedef struct
{
    texMod_t type;

    // used for TMOD_TURBULENT and TMOD_STRETCH
    waveForm_t wave;

    // used for TMOD_TRANSFORM
    float matrix[2][2]; // s' = s * m[0][0] + t * m[1][0] + trans[0]
    float translate[2]; // t' = s * m[0][1] + t * m[0][1] + trans[1]

    // used for TMOD_SCALE
    float scale[2]; // s *= scale[0]
                    // t *= scale[1]

    // used for TMOD_SCROLL
    float scroll[2]; // s' = s + scroll[0] * time
                     // t' = t + scroll[1] * time

    // + = clockwise
    // - = counterclockwise
    float rotateSpeed;

} texModInfo_t;


#define MAX_IMAGE_ANIMATIONS 8

typedef struct
{
    texture_type image[MAX_IMAGE_ANIMATIONS];
    int32        numImageAnimations;
    float        imageAnimationSpeed;

    texCoordGen_t tcGen;
    vec3_t        tcGenVectors[2];

    int32         numTexMods;
    texModInfo_t* texMods;

    bool isLightmap;
    bool vertexLightmap;
} textureBundle_t;

typedef struct
{
    bool active;

    textureBundle_t bundle;

    waveForm_t rgbWave;
    colorGen_t rgbGen;

    waveForm_t alphaWave;
    alphaGen_t alphaGen;

    uint8 constantColor[4]; // for CGEN_CONST and AGEN_CONST

    uint32 stateBits;       // GLS_xxxx mask

    acff_t adjustColorsForFog;

    bool isDetail;
} shaderStage_t;

struct shaderCommands_s;

#define LIGHTMAP_2D         -4 // shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3 // pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

typedef enum
{
    CT_FRONT_SIDED,
    CT_BACK_SIDED,
    CT_TWO_SIDED
} cullType_t;

typedef enum
{
    FP_NONE,  // surface is translucent and will just be adjusted properly
    FP_EQUAL, // surface is opaque but possibly alpha tested
    FP_LE     // surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct
{
    float        cloudHeight;
    texture_type outerbox[6], innerbox[6];
} skyParms_t;

typedef struct
{
    vec3_t color;
    float  depthForOpaque;
} fogParms_t;


typedef struct shader_s
{
    char  name[MAX_QPATH];  // game path, including extension
    int32 lightmapIndex;    // for a shader to match, both name and lightmapIndex must match

    int32 index;            // this shader == tr.shaders[index]
    int32 sortedIndex;      // this shader == tr.sortedShaders[sortedIndex]

    float sort;             // lower numbered shaders draw before higher numbered

    bool defaultShader;     // we want to return index 0 if the shader failed to
                            // load for some reason, but R_FindShader should
                            // still keep a name allocated for it, so if
                            // something calls RE_RegisterShader again with
                            // the same name, we don't try looking for it again

    bool explicitlyDefined; // found in a .shader file

    int32 surfaceFlags;     // if explicitlyDefined, this will have SURF_* flags
    int32 contentFlags;

    bool entityMergable; // merge across entites optimizable (smoke, blood)

    bool       isSky;
    skyParms_t sky;
    fogParms_t fogParms;

    float portalRange;        // distance to fog out at

    cullType_t cullType;      // CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
    bool       polygonOffset; // set for decals and other items that must be offset
    bool       noMipMaps;     // for console fonts, 2D elements, etc.

    fogPass_t fogPass;        // draw a blended pass, possibly with depth test equals

    bool needsNormal;         // not all shaders will need all data to be gathered
    bool needsST1;
    bool needsST2;
    bool needsColor;

    int32         numDeforms;
    deformStage_t deforms[MAX_SHADER_DEFORMS];

    int32          numUnfoggedPasses;
    shaderStage_t* stages[MAX_SHADER_STAGES];

    void (*optimalStageIteratorFunc)();

    float clampTime;                           // time this shader is clamped to
    float timeOffset;                          // current time offset for this shader

    int32            numStates;                // if non-zero this is a state shader
    struct shader_s* currentShader;            // current state if this is a state shader
    struct shader_s* parentShader;             // current state if this is a state shader
    int32            currentState;             // current state index for cycle purposes
    int32            expireTime;               // time in milliseconds this expires

    struct shader_s* remappedShader;           // current shader this one is remapped too

    int32 shaderStates[MAX_STATES_PER_SHADER]; // index to valid shader states

    struct shader_s* next;
} shader_t;

typedef struct shaderState_s
{
    char      shaderName[MAX_QPATH];  // name of shader this state belongs to
    char      name[MAX_STATE_NAME];   // name of this state
    char      stateShader[MAX_QPATH]; // shader this name invokes
    int32     cycleTime;              // time this cycle lasts, <= 0 is forever
    shader_t* shader;
} shaderState_t;


// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct
{
    int32  x, y, width, height;
    float  fov_x, fov_y;
    vec3_t vieworg;
    vec3_t viewaxis[3]; // transformation matrix

    int32 time;         // time in milliseconds for shader effects and other time dependent rendering issues
    int32 rdflags;      // RDF_NOWORLDMODEL, etc

    // 1 bits will prevent the associated area from rendering at all
    uint8 areamask[MAX_MAP_AREA_BYTES];
    bool  areamaskModified; // true if areamask changed since last scene

    float floatTime;        // tr.refdef.time / 1000.0

    // text messages for deform text shaders
    char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

    int32          num_entities;
    trRefEntity_t* entities;

    int32            num_dlights;
    struct dlight_s* dlights;

    int32             numPolys;
    struct srfPoly_s* polys;

    int32              numDrawSurfs;
    struct drawSurf_s* drawSurfs;


} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct
{
    char      name[MAX_QPATH];
    shader_t* shader;
} skinSurface_t;

typedef struct skin_s
{
    char           name[MAX_QPATH]; // game path, including extension
    int32          numSurfaces;
    skinSurface_t* surfaces[MD3_MAX_SURFACES];
} skin_t;


typedef struct
{
    int32  originalBrushNumber;
    vec3_t bounds[2];

    uint32     colorInt; // in packed uint8 format
    float      tcScale;  // texture coordinate vector scales
    fogParms_t parms;

    // for clipping distance in fog when outside
    bool  hasSurface;
    float surface[4];
} fog_t;

typedef struct
{
    orientationr_t orient;
    orientationr_t world;
    vec3_t         pvsOrigin;     // may be different than or.origin for portals
    bool           isPortal;      // true if this view is through a portal
    bool           isMirror;      // the portal is a mirror, invert the face culling
    int32          frameSceneNum; // copied from tr.frameSceneNum
    int32          frameCount;    // copied from tr.frameCount
    cplane_t       portalPlane;   // clip anything behind this if mirroring
    int32          viewportX, viewportY, viewportWidth, viewportHeight;
    float          fovX, fovY;
    float          projectionMatrix[16];
    cplane_t       frustum[4];
    vec3_t         visBounds[2];
    float          zFar;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum
{
    SF_BAD,
    SF_SKIP, // ignore
    SF_FACE,
    SF_GRID,
    SF_TRIANGLES,
    SF_POLY,
    SF_MD3,
    SF_FLARE,
    SF_ENTITY, // beams, rails, lightning, etc that can be determined by entity
    SF_DISPLAY_LIST,

    SF_NUM_SURFACE_TYPES,
    SF_MAX = 0x7fffffff // ensures that sizeof( surfaceType_t ) == sizeof( int32 )
} surfaceType_t;

typedef struct drawSurf_s
{
    uint32         sort;    // bit combination for fast compares
    surfaceType_t* surface; // any of surface*_t
} drawSurf_t;

#define MAX_FACE_POINTS 64

#define MAX_PATCH_SIZE  32 // max dimensions of a patch mesh in map file
#define MAX_GRID_SIZE   65 // max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s
{
    surfaceType_t surfaceType;
    qhandle_t     hShader;
    int32         fogIndex;
    int32         numVerts;
    polyVert_t*   verts;
} srfPoly_t;

typedef struct srfDisplayList_s
{
    surfaceType_t surfaceType;
    int32         listNum;
} srfDisplayList_t;


typedef struct srfFlare_s
{
    surfaceType_t surfaceType;
    vec3_t        origin;
    vec3_t        normal;
    vec3_t        color;
} srfFlare_t;

typedef struct srfGridMesh_s
{
    surfaceType_t surfaceType;

    // dynamic lighting information
    int32 dlightBits;

    // culling information
    vec3_t meshBounds[2];
    vec3_t localOrigin;
    float  meshRadius;

    // lod information, which may be different
    // than the culling information to allow for
    // groups of curves that LOD as a unit
    vec3_t lodOrigin;
    float  lodRadius;
    int32  lodFixed;
    int32  lodStitched;

    // vertexes
    int32      width, height;
    float*     widthLodError;
    float*     heightLodError;
    drawVert_t verts[1]; // variable sized
} srfGridMesh_t;


#define VERTEXSIZE 8
typedef struct
{
    surfaceType_t surfaceType;
    cplane_t      plane;

    // dynamic lighting information
    int32 dlightBits;

    // triangle definitions (no normals at points)
    int32 numPoints;
    int32 numIndices;
    int32 ofsIndices;
    float points[1][VERTEXSIZE]; // variable sized
                                 // there is a variable length list of indices here also
} srfSurfaceFace_t;


// misc_models in maps are turned into direct geometry by q3map
typedef struct
{
    surfaceType_t surfaceType;

    // dynamic lighting information
    int32 dlightBits;

    // culling information (FIXME: use this!)
    vec3_t bounds[2];
    vec3_t localOrigin;
    float  radius;

    // triangle definitions
    int32  numIndexes;
    int32* indexes;

    int32       numVerts;
    drawVert_t* verts;
} srfTriangles_t;


extern void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void*);

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define SIDE_FRONT 0
#define SIDE_BACK  1
#define SIDE_ON    2

typedef struct msurface_s
{
    int32            viewCount; // if == tr.viewCount, already added
    struct shader_s* shader;
    int32            fogIndex;

    surfaceType_t* data; // any of srf*_t
} msurface_t;


#define CONTENTS_NODE -1
typedef struct mnode_s
{
    // common with leaf and node
    int32           contents;   // -1 for nodes, to differentiate from leafs
    int32           visframe;   // node needs to be traversed if current
    vec3_t          mins, maxs; // for bounding box culling
    struct mnode_s* parent;

    // node specific
    cplane_t*       plane;
    struct mnode_s* children[2];

    // leaf specific
    int32 cluster;
    int32 area;

    msurface_t** firstmarksurface;
    int32        nummarksurfaces;
} mnode_t;

typedef struct
{
    vec3_t      bounds[2]; // for culling
    msurface_t* firstSurface;
    int32       numSurfaces;
} bmodel_t;

typedef struct
{
    char name[MAX_QPATH];     // ie: maps/tim_dm2.bsp
    char baseName[MAX_QPATH]; // ie: tim_dm2

    int32 dataSize;

    int32      numShaders;
    dshader_t* shaders;

    bmodel_t* bmodels;

    int32     numplanes;
    cplane_t* planes;

    int32    numnodes; // includes leafs
    int32    numDecisionNodes;
    mnode_t* nodes;

    int32       numsurfaces;
    msurface_t* surfaces;

    int32        nummarksurfaces;
    msurface_t** marksurfaces;

    int32  numfogs;
    fog_t* fogs;

    vec3_t lightGridOrigin;
    vec3_t lightGridSize;
    vec3_t lightGridInverseSize;
    int32  lightGridBounds[3];
    uint8* lightGridData;


    int32        numClusters;
    int32        clusterBytes;
    const uint8* vis; // may be passed in by CM_LoadMap to save space

    uint8* novis;     // clusterBytes of 0xff

    char* entityString;
    char* entityParsePoint;
} world_t;

//======================================================================

typedef enum
{
    MOD_BAD,
    MOD_BRUSH,
    MOD_MESH,
} modtype_t;

struct model_t
{
    char      name[MAX_QPATH];
    modtype_t type;
    int32     index;                // model = tr.models[model->index]

    int32        dataSize;          // just for listing purposes
    bmodel_t*    bmodel;            // only if type == MOD_BRUSH
    md3Header_t* md3[MD3_MAX_LODS]; // only if type == MOD_MESH

    int32 numLods;
};

#define MAX_MOD_KNOWN 1024

void     R_ModelInit();
model_t* R_GetModelByHandle(qhandle_t hModel);

void R_Modellist_f();

//====================================================
#define MAX_DRAWIMAGES        2048
#define MAX_LIGHTMAPS         256
#define MAX_SKINS             1024


#define MAX_DRAWSURFS         0x10000
#define DRAWSURF_MASK         (MAX_DRAWSURFS - 1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

21 - 31    : sorted shader index
11 - 20    : entity index
2 - 6    : fog index
//2        : used to be clipped flag REMOVED - 03.21.00 rad
0 - 1    : dlightmap index

    TTimo - 1.32
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
0-1   : dlightmap index
*/
#define QSORT_SHADERNUM_SHIFT 17
#define QSORT_ENTITYNUM_SHIFT 7
#define QSORT_FOGNUM_SHIFT    2

#define FOG_TABLE_SIZE        256
#define FUNCTABLE_SIZE        1024
#define FUNCTABLE_SIZE2       10
#define FUNCTABLE_MASK        (FUNCTABLE_SIZE - 1)

#define FUNCTABLE_TO_2PI(x)   ((float) x * 2.0 * M_PI / (float) FUNCTABLE_SIZE)
#define UINT8_TO_RAD(x)       (((float) ((x) & 0xff)) * 2.0 * M_PI / 256.0)
#define DEG_TO_RAD(x)         ((float) (x) * 2.0 * M_PI / 360.0f)


// the renderer front end should never modify glstate_t
typedef struct
{
    int32  currenttexture;
    int32  texEnv;
    int32  faceCulling;
    uint32 glStateBits;
} glstate_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct
{
    trRefdef_t     refdef;
    viewParms_t    viewParms;
    orientationr_t orient;
    trRefEntity_t* currentEntity;
    bool           skyRenderedThisView; // flag for drawing sun

    bool          projection2D;         // if true, drawstretchpic doesn't need to change modes
    uint8         color2D[4];
    bool          vertexes2D;           // shader needs to be finished
    trRefEntity_t entity2D;             // currentEntity will point at this when doing 2D rendering
    int32         backEndMsec;
} backEndState_t;

/*
** trGlobals_t
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct
{
    bool registered;     // cleared at shutdown, set at beginRegistration

    int32 visCount;      // incremented every time a new vis cluster is entered
    int32 frameCount;    // incremented every frame
    int32 sceneCount;    // incremented every scene
    int32 viewCount;     // incremented every view (twice a scene if portaled)
                         // and every R_MarkFragments call

    int32 frameSceneNum; // zeroed at RE_BeginFrame

    bool     worldMapLoaded;
    world_t* world;

    const uint8* externalVisData;

    texture_type defaultImage;
    texture_type fogImage;
    texture_type dlightImage; // inverse-quare highlight for projective adding
    texture_type whiteImage;  // full of 0xff

    shader_t* defaultShader;

    shader_t* sunShader;

    int32        numLightmaps;
    texture_type lightmaps[MAX_LIGHTMAPS];

    trRefEntity_t* currentEntity;
    trRefEntity_t  worldEntity;      // point currentEntity at this when rendering world
    int32          currentEntityNum;
    int32          shiftedEntityNum; // currentEntityNum << QSORT_ENTITYNUM_SHIFT
    model_t*       currentModel;

    viewParms_t viewParms;

    orientationr_t orient; // for current entity

    trRefdef_t refdef;

    int32 viewCluster;

    vec3_t sunLight; // from the sky shader for this level
    vec3_t sunDirection;

    int32 frontEndMsec; // not in pc due to clearing issue

    //
    // put large tables at the end, so most elements will be
    // within the +/32K indexed range on risc processors
    //
    model_t* models[MAX_MOD_KNOWN];
    int32    numModels;

    int32        numImages;
    texture_type images[MAX_DRAWIMAGES];

    // shader indexes from other modules will be looked up in tr.shaders[]
    // shader indexes from drawsurfs will be looked up in sortedShaders[]
    // lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
    int32     numShaders;
    shader_t* shaders[MAX_SHADERS];
    shader_t* sortedShaders[MAX_SHADERS];

    int32   numSkins;
    skin_t* skins[MAX_SKINS];

    float sinTable[FUNCTABLE_SIZE];
    float squareTable[FUNCTABLE_SIZE];
    float triangleTable[FUNCTABLE_SIZE];
    float sawToothTable[FUNCTABLE_SIZE];
    float inverseSawToothTable[FUNCTABLE_SIZE];
    float fogTable[FOG_TABLE_SIZE];
} trGlobals_t;

extern backEndState_t backEnd;
extern trGlobals_t    tr;
extern glconfig_t     glConfig; // outside of TR since it shouldn't be cleared during ref re-init
extern glstate_t      glState;  // outside of TR since it shouldn't be cleared during ref re-init


//
// cvars
//
extern cvar_t* r_novis; // disable/enable usage of PVS
extern cvar_t* r_nocurves;

extern cvar_t* r_offsetFactor;
extern cvar_t* r_offsetUnits;

extern cvar_t* r_lightmap; // render lightmaps only

extern cvar_t* r_lockpvs;

extern cvar_t* r_subdivisions;
extern cvar_t* r_lodCurveError;

//====================================================================

float R_NoiseGet4f(float x, float y, float z, float t);
void  R_NoiseInit();

void R_RenderView(viewParms_t* parms);

void R_AddMD3Surfaces(trRefEntity_t* e);

void R_AddPolygonSurfaces();

void R_DecomposeSort(uint32 sort, int32* entityNum, shader_t** shader, int32* fogNum, int32* dlightMap);

void R_AddDrawSurf(surfaceType_t* surface, shader_t* shader, int32 fogIndex, int32 dlightMap);


#define CULL_IN   0 // completely unclipped
#define CULL_CLIP 1 // clipped by one or more planes
#define CULL_OUT  2 // completely outside the clipping planes
void  R_LocalNormalToWorld(vec3_t local, vec3_t world);
void  R_LocalPointToWorld(vec3_t local, vec3_t world);
int32 R_CullLocalBox(vec3_t bounds[2]);
int32 R_CullPointAndRadius(vec3_t origin, float radius);
int32 R_CullLocalPointAndRadius(vec3_t origin, float radius);

void R_RotateForEntity(const trRefEntity_t* ent, const viewParms_t* viewParms, orientationr_t* orient);

/*
** GL wrapper/helper functions
*/
void GL_Bind(texture_type image);
void GL_State(uint32 stateVector);
void GL_TexEnv(int32 env);
void GL_Cull(int32 cullType);

#define GLS_SRCBLEND_ZERO                0x00000001
#define GLS_SRCBLEND_ONE                 0x00000002
#define GLS_SRCBLEND_DST_COLOR           0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR 0x00000004
#define GLS_SRCBLEND_SRC_ALPHA           0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA 0x00000006
#define GLS_SRCBLEND_DST_ALPHA           0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA 0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE      0x00000009
#define GLS_SRCBLEND_BITS                0x0000000f

#define GLS_DSTBLEND_ZERO                0x00000010
#define GLS_DSTBLEND_ONE                 0x00000020
#define GLS_DSTBLEND_SRC_COLOR           0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR 0x00000040
#define GLS_DSTBLEND_SRC_ALPHA           0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA 0x00000060
#define GLS_DSTBLEND_DST_ALPHA           0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA 0x00000080
#define GLS_DSTBLEND_BITS                0x000000f0

#define GLS_DEPTHMASK_TRUE               0x00000100

#define GLS_POLYMODE_LINE                0x00001000

#define GLS_DEPTHTEST_DISABLE            0x00010000
#define GLS_DEPTHFUNC_EQUAL              0x00020000

#define GLS_ATEST_GT_0                   0x10000000
#define GLS_ATEST_LT_80                  0x20000000
#define GLS_ATEST_GE_80                  0x40000000
#define GLS_ATEST_BITS                   0x70000000

#define GLS_DEFAULT                      GLS_DEPTHMASK_TRUE

bool R_GetEntityToken(char* buffer, int32 size);

model_t* R_AllocModel();


void         set_texture_sampler(texture_type tex, int32_t min_filter, int32_t mag_filter, int32_t wrap);
texture_type create_texture(tavros::core::string_view name, int32_t width, int32_t height, const uint8_t* pixels);
void         R_Init();
texture_type R_FindImageFile(const char* name, bool mipmap, int32 glWrapClampMode);

const void* RB_TakeScreenshotCmd(const void* data);
;

void    R_InitFogTable();
float   R_FogFactor(float s, float t);
void    R_InitImages();
void    R_DeleteTextures();
void    R_InitSkins();
skin_t* R_GetSkinByHandle(qhandle_t hSkin);


//
// tr_shader.c
//
qhandle_t RE_RegisterShaderLightMap(const char* name, int32 lightmapIndex);

shader_t* R_FindShader(const char* name, int32 lightmapIndex, bool mipRawImage);
shader_t* R_GetShaderByHandle(qhandle_t hShader);
shader_t* R_FindShaderByName(const char* name);
void      R_InitShaders();
void      R_ShaderList_f();

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void GLimp_Init();
void GLimp_Shutdown();
void GLimp_EndFrame();

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/
typedef uint8 color4ub_t[4];

typedef struct stageVars
{
    color4ub_t         colors[SHADER_MAX_VERTEXES];
    tavros::math::vec2 _texCoords[SHADER_MAX_VERTEXES];
} stageVars_t;

typedef struct shaderCommands_s
{
    uint32             indexes[SHADER_MAX_INDEXES]; // OpenGL indices
    tavros::math::vec4 xyz[SHADER_MAX_VERTEXES];
    tavros::math::vec4 normal[SHADER_MAX_VERTEXES];
    tavros::math::vec2 _texCoords[SHADER_MAX_VERTEXES][2];
    color4ub_t         vertexColors[SHADER_MAX_VERTEXES];
    int32              vertexDlightBits[SHADER_MAX_VERTEXES];

    stageVars_t svars;

    color4ub_t constantColor255[SHADER_MAX_VERTEXES];

    shader_t* shader;
    float     shaderTime;
    int32     fogNum;

    int32 dlightBits; // or together of all vertexDlightBits

    int32 numIndexes;
    int32 numVertexes;

    // info extracted from current shader
    int32           numPasses;
    void            (*currentStageIteratorFunc)();
    shaderStage_t** xstages;
} shaderCommands_t;

extern shaderCommands_t tess;

void RB_BeginSurface(shader_t* shader, int32 fogNum);
void RB_EndSurface();
void RB_CheckOverflow(int32 verts, int32 indexes);
#define RB_CHECKOVERFLOW(v, i)                                                                          \
    if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES) { \
        RB_CheckOverflow(v, i);                                                                         \
    }

void RB_StageIteratorGeneric();
void RB_StageIteratorSky();

void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, uint8* color);
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, uint8* color, float s1, float t1, float s2, float t2);


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces(trRefEntity_t* e);
void R_AddWorldSurfaces();

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel(bmodel_t* bmodel);
void R_SetupEntityLighting(const trRefdef_t* refdef, trRefEntity_t* ent);
void R_TransformDlights(int32 count, dlight_t* dl, orientationr_t* orient);

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData(shaderCommands_t* shader);
void R_InitSkyTexCoords(float cloudLayerHeight);
void RB_ClipSkyPolygons(shaderCommands_t* shader);

/*
============================================================

CURVE TESSELATION

============================================================
*/

srfGridMesh_t* R_SubdividePatchToGrid(int32 width, int32 height, drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE]);
srfGridMesh_t* R_GridInsertColumn(srfGridMesh_t* grid, int32 column, int32 row, vec3_t point, float loderror);
srfGridMesh_t* R_GridInsertRow(srfGridMesh_t* grid, int32 row, int32 column, vec3_t point, float loderror);
void           R_FreeSurfaceGridMesh(srfGridMesh_t* grid);

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_ToggleSmpFrame();

/*
=============================================================
=============================================================
*/
void R_TransformModelToClip(const vec3_t src, const float* modelMatrix, const float* projectionMatrix, tavros::math::vec4& eye, tavros::math::vec4& dst);

void RB_DeformTessGeometry();

void RB_CalcEnvironmentTexCoords(float* dstTexCoords);
void RB_CalcFogTexCoords(float* dstTexCoords);
void RB_CalcScrollTexCoords(const float scroll[2], float* dstTexCoords);
void RB_CalcRotateTexCoords(float rotSpeed, float* dstTexCoords);
void RB_CalcScaleTexCoords(const float scale[2], float* dstTexCoords);
void RB_CalcTurbulentTexCoords(const waveForm_t* wf, float* dstTexCoords);
void RB_CalcTransformTexCoords(const texModInfo_t* tmi, float* dstTexCoords);
void RB_CalcModulateColorsByFog(uint8* dstColors);
void RB_CalcModulateAlphasByFog(uint8* dstColors);
void RB_CalcModulateRGBAsByFog(uint8* dstColors);
void RB_CalcWaveAlpha(const waveForm_t* wf, uint8* dstColors);
void RB_CalcWaveColor(const waveForm_t* wf, uint8* dstColors);
void RB_CalcAlphaFromEntity(uint8* dstColors);
void RB_CalcAlphaFromOneMinusEntity(uint8* dstColors);
void RB_CalcStretchTexCoords(const waveForm_t* wf, float* texCoords);
void RB_CalcColorFromEntity(uint8* dstColors);
void RB_CalcColorFromOneMinusEntity(uint8* dstColors);
void RB_CalcSpecularAlpha(uint8* alphas);
void RB_CalcDiffuseColor(uint8* colors);

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define MAX_RENDER_COMMANDS 0x40000

typedef struct
{
    uint8 cmds[MAX_RENDER_COMMANDS];
    int32 used;
} renderCommandList_t;

typedef struct
{
    int32 commandId;
    float color[4];
} setColorCommand_t;

typedef struct
{
    int32 commandId;
} drawBufferCommand_t;

typedef struct
{
    int32        commandId;
    texture_type image;
    int32        width;
    int32        height;
    void*        data;
} subImageCommand_t;

typedef struct
{
    int32 commandId;
} swapBuffersCommand_t;

typedef struct
{
    int32 commandId;
    int32 buffer;
} endFrameCommand_t;

typedef struct
{
    int32              commandId;
    shader_t*          shader;
    tavros::math::vec2 point;
    tavros::math::vec2 size;
    tavros::math::vec2 uv1;
    tavros::math::vec2 uv2;
} stretchPicCommand_t;

typedef struct
{
    int32       commandId;
    trRefdef_t  refdef;
    viewParms_t viewParms;
    drawSurf_t* drawSurfs;
    int32       numDrawSurfs;
} drawSurfsCommand_t;

typedef struct
{
    int32 commandId;
    int32 x;
    int32 y;
    int32 width;
    int32 height;
    char* fileName;
} screenshotCommand_t;

typedef enum
{
    RC_END_OF_LIST,
    RC_SET_COLOR,
    RC_STRETCH_PIC,
    RC_DRAW_SURFS,
    RC_DRAW_BUFFER,
    RC_SWAP_BUFFERS,
    RC_SCREENSHOT
} renderCommand_t;


// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define MAX_POLYS     6000
#define MAX_POLYVERTS 30000

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end
typedef struct
{
    drawSurf_t          drawSurfs[MAX_DRAWSURFS];
    dlight_t            dlights[MAX_DLIGHTS];
    trRefEntity_t       entities[MAX_ENTITIES];
    srfPoly_t*          polys;     //[MAX_POLYS];
    polyVert_t*         polyVerts; //[MAX_POLYVERTS];
    renderCommandList_t commands;
} backEndData_t;

extern backEndData_t* backEndData; // the second one may not be allocated

void* R_GetCommandBuffer(int32 bytes);
void  RB_ExecuteRenderCommands(const void* data);

void R_AddDrawSurfCmd(drawSurf_t* drawSurfs, int32 numDrawSurfs);
