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
// tr_light.c

#include "tr_local.h"

#define DLIGHT_AT_RADIUS      16
// at the edge of a dlight's influence, this amount of light will be added

#define DLIGHT_MINIMUM_RADIUS 16
// never calculate a range less than this to prevent huge light numbers


/*
===============
R_TransformDlights

Transforms the origins of an array of dlights.
Used by both the front end (for DlightBmodel) and
the back end (before doing the lighting calculation)
===============
*/
void R_TransformDlights(int32 count, dlight_t* dl, orientationr_t* orient)
{
    int32  i;
    vec3_t temp;

    for (i = 0; i < count; i++, dl++) {
        VectorSubtract(dl->origin, orient->origin, temp);
        dl->transformed[0] = DotProduct(temp, orient->axis[0]);
        dl->transformed[1] = DotProduct(temp, orient->axis[1]);
        dl->transformed[2] = DotProduct(temp, orient->axis[2]);
    }
}

/*
=============
R_DlightBmodel

Determine which dynamic lights may effect this bmodel
=============
*/
void R_DlightBmodel(bmodel_t* bmodel)
{
    int32       i, j;
    dlight_t*   dl;
    int32       mask;
    msurface_t* surf;

    // transform all the lights
    R_TransformDlights(tr.refdef.num_dlights, tr.refdef.dlights, &tr.orient);

    mask = 0;
    for (i = 0; i < tr.refdef.num_dlights; i++) {
        dl = &tr.refdef.dlights[i];

        // see if the point is close enough to the bounds to matter
        for (j = 0; j < 3; j++) {
            if (dl->transformed[j] - bmodel->bounds[1][j] > dl->radius) {
                break;
            }
            if (bmodel->bounds[0][j] - dl->transformed[j] > dl->radius) {
                break;
            }
        }
        if (j < 3) {
            continue;
        }

        // we need to check this light
        mask |= 1 << i;
    }

    tr.currentEntity->needDlights = (mask != 0);

    // set the dlight bits in all the surfaces
    for (i = 0; i < bmodel->numSurfaces; i++) {
        surf = bmodel->firstSurface + i;

        if (*surf->data == SF_FACE) {
            ((srfSurfaceFace_t*) surf->data)->dlightBits = mask;
        } else if (*surf->data == SF_GRID) {
            ((srfGridMesh_t*) surf->data)->dlightBits = mask;
        } else if (*surf->data == SF_TRIANGLES) {
            ((srfTriangles_t*) surf->data)->dlightBits = mask;
        }
    }
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

extern cvar_t* r_ambientScale;
extern cvar_t* r_directedScale;

/*
=================
R_SetupEntityLightingGrid

=================
*/
static void R_SetupEntityLightingGrid(trRefEntity_t* ent)
{
    vec3_t lightOrigin;
    int32  pos[3];
    int32  i, j;
    uint8* gridData;
    float  frac[3];
    int32  gridStep[3];
    vec3_t direction;
    float  totalFactor;

    if (ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        // seperate lightOrigins are needed so an object that is
        // sinking into the ground can still be lit, and so
        // multi-part models can be lit identically
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    VectorSubtract(lightOrigin, tr.world->lightGridOrigin, lightOrigin);
    for (i = 0; i < 3; i++) {
        float v;

        v = lightOrigin[i] * tr.world->lightGridInverseSize[i];
        pos[i] = floor(v);
        frac[i] = v - pos[i];
        if (pos[i] < 0) {
            pos[i] = 0;
        } else if (pos[i] >= tr.world->lightGridBounds[i] - 1) {
            pos[i] = tr.world->lightGridBounds[i] - 1;
        }
    }

    VectorClear(ent->ambientLight);
    VectorClear(ent->directedLight);
    VectorClear(direction);

    assert(tr.world->lightGridData); // bk010103 - NULL with -nolight maps

    // trilerp the light value
    gridStep[0] = 8;
    gridStep[1] = 8 * tr.world->lightGridBounds[0];
    gridStep[2] = 8 * tr.world->lightGridBounds[0] * tr.world->lightGridBounds[1];
    gridData = tr.world->lightGridData + pos[0] * gridStep[0]
             + pos[1] * gridStep[1] + pos[2] * gridStep[2];

    totalFactor = 0;
    for (i = 0; i < 8; i++) {
        float  factor;
        uint8* data;
        vec3_t normal;

        factor = 1.0;
        data = gridData;
        for (j = 0; j < 3; j++) {
            if (i & (1 << j)) {
                factor *= frac[j];
                data += gridStep[j];
            } else {
                factor *= (1.0f - frac[j]);
            }
        }

        if (!(data[0] + data[1] + data[2])) {
            continue; // ignore samples in walls
        }
        totalFactor += factor;

        ent->ambientLight[0] += factor * data[0];
        ent->ambientLight[1] += factor * data[1];
        ent->ambientLight[2] += factor * data[2];

        ent->directedLight[0] += factor * data[3];
        ent->directedLight[1] += factor * data[4];
        ent->directedLight[2] += factor * data[5];

        // convert from uint8 to radians
        float lng = UINT8_TO_RAD(data[7]);
        float lat = UINT8_TO_RAD(data[6]);

        normal[0] = cosf(lat) * sinf(lng);
        normal[1] = sinf(lat) * sinf(lng);
        normal[2] = cosf(lng);

        VectorMA(direction, factor, normal, direction);
    }

    if (totalFactor > 0 && totalFactor < 0.99) {
        totalFactor = 1.0f / totalFactor;
        VectorScale(ent->ambientLight, totalFactor, ent->ambientLight);
        VectorScale(ent->directedLight, totalFactor, ent->directedLight);
    }

    VectorScale(ent->ambientLight, r_ambientScale->value, ent->ambientLight);
    VectorScale(ent->directedLight, r_directedScale->value, ent->directedLight);

    VectorNormalize2(direction, ent->lightDir);
}

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting(const trRefdef_t* refdef, trRefEntity_t* ent)
{
    int32     i;
    dlight_t* dl;
    float     power;
    vec3_t    dir;
    float     d;
    vec3_t    lightDir;
    vec3_t    lightOrigin;

    // lighting calculations
    if (ent->lightingCalculated) {
        return;
    }
    ent->lightingCalculated = true;

    //
    // trace a sample point down to find ambient light
    //
    if (ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        // seperate lightOrigins are needed so an object that is
        // sinking into the ground can still be lit, and so
        // multi-part models can be lit identically
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    // if NOWORLDMODEL, only use dynamic lights (menu system, etc)
    if (!(refdef->rdflags & RDF_NOWORLDMODEL)
        && tr.world->lightGridData) {
        R_SetupEntityLightingGrid(ent);
    } else {
        ent->ambientLight[0] = ent->ambientLight[1] = ent->ambientLight[2] = 150;
        ent->directedLight[0] = ent->directedLight[1] = ent->directedLight[2] = 150;
        VectorCopy(tr.sunDirection, ent->lightDir);
    }

    ent->ambientLight[0] += 32;
    ent->ambientLight[1] += 32;
    ent->ambientLight[2] += 23;

    //
    // modify the light by dynamic lights
    //
    d = VectorLength(ent->directedLight);
    VectorScale(ent->lightDir, d, lightDir);

    for (i = 0; i < refdef->num_dlights; i++) {
        dl = &refdef->dlights[i];
        VectorSubtract(dl->origin, lightOrigin, dir);
        d = VectorNormalize(dir);

        power = DLIGHT_AT_RADIUS * (dl->radius * dl->radius);
        if (d < DLIGHT_MINIMUM_RADIUS) {
            d = DLIGHT_MINIMUM_RADIUS;
        }
        d = power / (d * d);

        VectorMA(ent->directedLight, d, dl->color, ent->directedLight);
        VectorMA(lightDir, d, dir, lightDir);
    }

    // clamp ambient
    for (i = 0; i < 3; i++) {
        if (ent->ambientLight[i] > 255.0) {
            ent->ambientLight[i] = 255.0;
        }
    }

    // save out the uint8 packet version
    ((uint8*) &ent->ambientLightInt)[0] = myftol(ent->ambientLight[0]);
    ((uint8*) &ent->ambientLightInt)[1] = myftol(ent->ambientLight[1]);
    ((uint8*) &ent->ambientLightInt)[2] = myftol(ent->ambientLight[2]);
    ((uint8*) &ent->ambientLightInt)[3] = 0xff;

    // transform the direction to local space
    VectorNormalize(lightDir);
    ent->lightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
    ent->lightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
    ent->lightDir[2] = DotProduct(lightDir, ent->e.axis[2]);
}
