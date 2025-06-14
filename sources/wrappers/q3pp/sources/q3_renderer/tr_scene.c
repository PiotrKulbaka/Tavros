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

#include "tr_local.h"

static tavros::core::logger logger("tr_scene");

static int32 r_firstSceneDrawSurf = 0;
static int32 r_numdlights = 0;
static int32 r_firstSceneDlight = 0;
static int32 r_numentities = 0;
static int32 r_firstSceneEntity = 0;
static int32 r_numpolys = 0;
static int32 r_firstScenePoly = 0;
static int32 r_numpolyverts = 0;


/*
====================
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame()
{
    backEndData->commands.used = 0;

    r_firstSceneDrawSurf = 0;
    r_numdlights = 0;
    r_firstSceneDlight = 0;
    r_numentities = 0;
    r_firstSceneEntity = 0;
    r_numpolys = 0;
    r_firstScenePoly = 0;
    r_numpolyverts = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene()
{
    r_firstSceneDlight = r_numdlights;
    r_firstSceneEntity = r_numentities;
    r_firstScenePoly = r_numpolys;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces()
{
    int32      i;
    shader_t*  sh;
    srfPoly_t* poly;

    tr.currentEntityNum = ENTITYNUM_WORLD;
    tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

    for (i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++) {
        sh = R_GetShaderByHandle(poly->hShader);
        R_AddDrawSurf((surfaceType_t*) poly, sh, poly->fogIndex, false);
    }
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene(qhandle_t hShader, int32 numVerts, const polyVert_t* verts, int32 numPolys)
{
    srfPoly_t* poly;
    int32      i, j;
    int32      fogIndex;
    fog_t*     fog;
    vec3_t     bounds[2];

    if (!tr.registered) {
        return;
    }

    if (!hShader) {
        logger.debug("WARNING: RE_AddPolyToScene: NULL poly shader");
        return;
    }

    for (j = 0; j < numPolys; j++) {
        if (r_numpolyverts + numVerts > MAX_POLYVERTS || r_numpolys >= MAX_POLYS) {
            /*
            NOTE TTimo this was initially a PRINT_WARNING
            but it happens a lot with high fighting scenes and particles
            since we don't plan on changing the const and making for room for those effects
            simply cut this message to developer only
            */
            logger.debug("WARNING: RE_AddPolyToScene: MAX_POLYVERTS or MAX_POLYS reached");
            return;
        }

        poly = &backEndData->polys[r_numpolys];
        poly->surfaceType = SF_POLY;
        poly->hShader = hShader;
        poly->numVerts = numVerts;
        poly->verts = &backEndData->polyVerts[r_numpolyverts];

        Com_Memcpy(poly->verts, &verts[numVerts * j], numVerts * sizeof(*verts));

        // done.
        r_numpolys++;
        r_numpolyverts += numVerts;

        // if no world is loaded
        if (tr.world == NULL) {
            fogIndex = 0;
        }
        // see if it is in a fog volume
        else if (tr.world->numfogs == 1) {
            fogIndex = 0;
        } else {
            // find which fog volume the poly is in
            VectorCopy(poly->verts[0].xyz, bounds[0]);
            VectorCopy(poly->verts[0].xyz, bounds[1]);
            for (i = 1; i < poly->numVerts; i++) {
                AddPointToBounds(poly->verts[i].xyz, bounds[0], bounds[1]);
            }
            for (fogIndex = 1; fogIndex < tr.world->numfogs; fogIndex++) {
                fog = &tr.world->fogs[fogIndex];
                if (bounds[1][0] >= fog->bounds[0][0]
                    && bounds[1][1] >= fog->bounds[0][1]
                    && bounds[1][2] >= fog->bounds[0][2]
                    && bounds[0][0] <= fog->bounds[1][0]
                    && bounds[0][1] <= fog->bounds[1][1]
                    && bounds[0][2] <= fog->bounds[1][2]) {
                    break;
                }
            }
            if (fogIndex == tr.world->numfogs) {
                fogIndex = 0;
            }
        }
        poly->fogIndex = fogIndex;
    }
}


//=================================================================================


/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene(const refEntity_t* ent)
{
    if (!tr.registered) {
        return;
    }
    if (r_numentities >= ENTITYNUM_WORLD) {
        return;
    }
    if (ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE) {
        Com_Error(ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType);
    }

    backEndData->entities[r_numentities].e = *ent;
    backEndData->entities[r_numentities].lightingCalculated = false;

    r_numentities++;
}


/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene(const vec3_t org, float intensity, float r, float g, float b, int32 additive)
{
    dlight_t* dl;

    if (!tr.registered) {
        return;
    }
    if (r_numdlights >= MAX_DLIGHTS) {
        return;
    }
    if (intensity <= 0) {
        return;
    }

    dl = &backEndData->dlights[r_numdlights++];
    VectorCopy(org, dl->origin);
    dl->radius = intensity;
    dl->color[0] = r;
    dl->color[1] = g;
    dl->color[2] = b;
    dl->additive = additive;
}

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b)
{
    RE_AddDynamicLightToScene(org, intensity, r, g, b, false);
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
void RE_RenderScene(const refdef_t* fd)
{
    viewParms_t parms;
    int32       startTime;

    if (!tr.registered) {
        return;
    }

    startTime = Sys_Milliseconds();

    if (!tr.world && !(fd->rdflags & RDF_NOWORLDMODEL)) {
        Com_Error(ERR_DROP, "R_RenderScene: NULL worldmodel");
    }

    Com_Memcpy(tr.refdef.text, fd->text, sizeof(tr.refdef.text));

    tr.refdef.x = fd->x;
    tr.refdef.y = fd->y;
    tr.refdef.width = fd->width;
    tr.refdef.height = fd->height;
    tr.refdef.fov_x = fd->fov_x;
    tr.refdef.fov_y = fd->fov_y;

    VectorCopy(fd->vieworg, tr.refdef.vieworg);
    VectorCopy(fd->viewaxis[0], tr.refdef.viewaxis[0]);
    VectorCopy(fd->viewaxis[1], tr.refdef.viewaxis[1]);
    VectorCopy(fd->viewaxis[2], tr.refdef.viewaxis[2]);

    tr.refdef.time = fd->time;
    tr.refdef.rdflags = fd->rdflags;

    // copy the areamask data over and note if it has changed, which
    // will force a reset of the visible leafs even if the view hasn't moved
    tr.refdef.areamaskModified = false;
    if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) {
        int32 areaDiff;
        int32 i;

        // compare the area bits
        areaDiff = 0;
        for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
            areaDiff |= ((int32*) tr.refdef.areamask)[i] ^ ((int32*) fd->areamask)[i];
            ((int32*) tr.refdef.areamask)[i] = ((int32*) fd->areamask)[i];
        }

        if (areaDiff) {
            // a door just opened or something
            tr.refdef.areamaskModified = true;
        }
    }


    // derived info

    tr.refdef.floatTime = tr.refdef.time * 0.001f;

    tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
    tr.refdef.drawSurfs = backEndData->drawSurfs;

    tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
    tr.refdef.entities = &backEndData->entities[r_firstSceneEntity];

    tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
    tr.refdef.dlights = &backEndData->dlights[r_firstSceneDlight];

    tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
    tr.refdef.polys = &backEndData->polys[r_firstScenePoly];

    // a single frame may have multiple scenes draw inside it --
    // a 3D game view, 3D status bar renderings, 3D menus, etc.
    // They need to be distinguished by the light flare code, because
    // the visibility state for a given surface may be different in
    // each scene / view.
    tr.frameSceneNum++;
    tr.sceneCount++;

    // setup view parms for the initial view
    //
    // set up viewport
    // The refdef takes 0-at-the-top y coordinates, so
    // convert to GL's 0-at-the-bottom space
    //
    Com_Memset(&parms, 0, sizeof(parms));
    parms.viewportX = tr.refdef.x;
    parms.viewportY = glConfig.vidHeight - (tr.refdef.y + tr.refdef.height);
    parms.viewportWidth = tr.refdef.width;
    parms.viewportHeight = tr.refdef.height;
    parms.isPortal = false;

    parms.fovX = tr.refdef.fov_x;
    parms.fovY = tr.refdef.fov_y;

    VectorCopy(fd->vieworg, parms.orient.origin);
    VectorCopy(fd->viewaxis[0], parms.orient.axis[0]);
    VectorCopy(fd->viewaxis[1], parms.orient.axis[1]);
    VectorCopy(fd->viewaxis[2], parms.orient.axis[2]);

    VectorCopy(fd->vieworg, parms.pvsOrigin);

    R_RenderView(&parms);

    // the next scene rendered in this frame will tack on after this one
    r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
    r_firstSceneEntity = r_numentities;
    r_firstSceneDlight = r_numdlights;
    r_firstScenePoly = r_numpolys;

    tr.frontEndMsec += Sys_Milliseconds() - startTime;
}
