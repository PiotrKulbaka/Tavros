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

#define MAX_DLIGHTS        32   // can't be increased, because bit flags are used on surfaces
#define MAX_ENTITIES       1023 // can't be increased without changing drawsurf bit packing

// renderfx flags
#define RF_MINLIGHT        1   // allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON    2   // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON    4   // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK       8   // for view weapon Z crunching
#define RF_NOSHADOW        64  // don't add stencil shadows

#define RF_LIGHTING_ORIGIN 128 // use refEntity->lightingOrigin instead of refEntity->origin
                               // for lighting.  This allows entities to sink into the floor
                               // with their origin going solid, and allows all parts of a
                               // player to get the same lighting
#define RF_WRAP_FRAMES     512 // mod the model frames by the maxframes to allow continuous
                               // animation without needing to know the frame count

// refdef flags
#define RDF_NOWORLDMODEL   1 // used for player configuration screen
#define RDF_HYPERSPACE     4 // teleportation effect

typedef struct
{
    vec3_t xyz;
    float  st[2];
    uint8  modulateU8[4];
} polyVert_t;

typedef struct
{
    qhandle_t   hShader;
    int32       numVerts;
    polyVert_t* verts;
} poly_t;

typedef enum
{
    RT_MODEL,
    RT_SPRITE,
    RT_BEAM,
    RT_RAIL_CORE,
    RT_LIGHTNING,
    RT_PORTALSURFACE, // doesn't draw anything, just info for portals

    RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef struct
{
    refEntityType_t reType;
    int32           renderfx;

    qhandle_t hModel; // opaque type outside refresh

    // most recent data
    vec3_t lightingOrigin;    // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)

    vec3_t axis[3];           // rotation vectors
    bool   nonNormalizedAxes; // axis are not normalized, i.e. they have scale
    float  origin[3];         // also used as MODEL_BEAM's "from"
    int32  frame;             // also used as MODEL_BEAM's diameter

    // previous data for frame interpolation
    float oldorigin[3]; // also used as MODEL_BEAM's "to"
    int32 oldframe;
    float backlerp;     // 0.0 = current, 1.0 = old

    // texturing
    int32     skinNum;      // inline skin index
    qhandle_t customSkin;   // NULL for default skin
    qhandle_t customShader; // use one image for the entire thing

    // misc
    uint8 shaderRGBA[4];     // colors used by rgbgen entity shaders
    float shaderTexCoord[2]; // texture coordinates used by tcMod entity modifiers
    float shaderTime;        // subtracted from refdef time to control effect start times

    // extra sprite information
    float radius;
    float rotation;
} refEntity_t;


#define MAX_RENDER_STRINGS       8
#define MAX_RENDER_STRING_LENGTH 32

typedef struct
{
    int32  x, y, width, height;
    float  fov_x, fov_y;
    vec3_t vieworg;
    vec3_t viewaxis[3]; // transformation matrix

    // time in milliseconds for shader effects and other time dependent rendering issues
    int32 time;

    int32 rdflags; // RDF_NOWORLDMODEL, etc

    // 1 bits will prevent the associated area from rendering at all
    uint8 areamask[MAX_MAP_AREA_BYTES];

    // text messages for deform text shaders
    char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];
} refdef_t;

typedef struct
{
    int32 vidWidth, vidHeight;
} glconfig_t;
