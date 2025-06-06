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
// tr_surf.c
#include "tr_local.h"

static tavros::core::logger logger("tr_surface");

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/


//============================================================================


/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow(int32 verts, int32 indexes)
{
    if (tess.numVertexes + verts < SHADER_MAX_VERTEXES
        && tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
        return;
    }

    RB_EndSurface();

    if (verts >= SHADER_MAX_VERTEXES) {
        Com_Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES);
    }
    if (indexes >= SHADER_MAX_INDEXES) {
        Com_Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES);
    }

    RB_BeginSurface(tess.shader, tess.fogNum);
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt(vec3_t origin, vec3_t left, vec3_t up, uint8* color, float s1, float t1, float s2, float t2)
{
    vec3_t normal;
    int32  ndx;

    RB_CHECKOVERFLOW(4, 6);

    ndx = tess.numVertexes;

    // triangle indexes for a simple quad
    tess.indexes[tess.numIndexes] = ndx;
    tess.indexes[tess.numIndexes + 1] = ndx + 1;
    tess.indexes[tess.numIndexes + 2] = ndx + 3;

    tess.indexes[tess.numIndexes + 3] = ndx + 3;
    tess.indexes[tess.numIndexes + 4] = ndx + 1;
    tess.indexes[tess.numIndexes + 5] = ndx + 2;

    tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
    tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
    tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

    tess.xyz[ndx + 1][0] = origin[0] - left[0] + up[0];
    tess.xyz[ndx + 1][1] = origin[1] - left[1] + up[1];
    tess.xyz[ndx + 1][2] = origin[2] - left[2] + up[2];

    tess.xyz[ndx + 2][0] = origin[0] - left[0] - up[0];
    tess.xyz[ndx + 2][1] = origin[1] - left[1] - up[1];
    tess.xyz[ndx + 2][2] = origin[2] - left[2] - up[2];

    tess.xyz[ndx + 3][0] = origin[0] + left[0] - up[0];
    tess.xyz[ndx + 3][1] = origin[1] + left[1] - up[1];
    tess.xyz[ndx + 3][2] = origin[2] + left[2] - up[2];


    // constant normal all the way around
    VectorSubtract(vec3_origin, backEnd.viewParms.orient.axis[0], normal);

    tess.normal[ndx][0] = tess.normal[ndx + 1][0] = tess.normal[ndx + 2][0] = tess.normal[ndx + 3][0] = normal[0];
    tess.normal[ndx][1] = tess.normal[ndx + 1][1] = tess.normal[ndx + 2][1] = tess.normal[ndx + 3][1] = normal[1];
    tess.normal[ndx][2] = tess.normal[ndx + 1][2] = tess.normal[ndx + 2][2] = tess.normal[ndx + 3][2] = normal[2];

    // standard square texture coordinates
    tess._texCoords[ndx + 0][0] = tavros::math::vec2(s1, t1);
    tess._texCoords[ndx + 1][0] = tavros::math::vec2(s2, t1);
    tess._texCoords[ndx + 2][0] = tavros::math::vec2(s2, t2);
    tess._texCoords[ndx + 3][0] = tavros::math::vec2(s1, t2);

    // constant color all the way around
    // should this be identity and let the shader specify from entity?
    *(uint32*) &tess.vertexColors[ndx] =
        *(uint32*) &tess.vertexColors[ndx + 1] =
            *(uint32*) &tess.vertexColors[ndx + 2] =
                *(uint32*) &tess.vertexColors[ndx + 3] =
                    *(uint32*) color;


    tess.numVertexes += 4;
    tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp(vec3_t origin, vec3_t left, vec3_t up, uint8* color)
{
    RB_AddQuadStampExt(origin, left, up, color, 0, 0, 1, 1);
}

/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite()
{
    vec3_t left, up;
    float  radius;

    // calculate the xyz locations for the four corners
    radius = backEnd.currentEntity->e.radius;
    if (backEnd.currentEntity->e.rotation == 0) {
        VectorScale(backEnd.viewParms.orient.axis[1], radius, left);
        VectorScale(backEnd.viewParms.orient.axis[2], radius, up);
    } else {
        float s, c;
        float ang;

        ang = M_PI * backEnd.currentEntity->e.rotation / 180;
        s = sin(ang);
        c = cos(ang);

        VectorScale(backEnd.viewParms.orient.axis[1], c * radius, left);
        VectorMA(left, -s * radius, backEnd.viewParms.orient.axis[2], left);

        VectorScale(backEnd.viewParms.orient.axis[2], c * radius, up);
        VectorMA(up, s * radius, backEnd.viewParms.orient.axis[1], up);
    }
    if (backEnd.viewParms.isMirror) {
        VectorSubtract(vec3_origin, left, left);
    }

    RB_AddQuadStamp(backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA);
}


/*
=============
RB_SurfacePolychain
=============
*/
void RB_SurfacePolychain(srfPoly_t* p)
{
    int32 i;
    int32 numv;

    RB_CHECKOVERFLOW(p->numVerts, 3 * (p->numVerts - 2));

    // fan triangles into the tess array
    numv = tess.numVertexes;
    for (i = 0; i < p->numVerts; i++) {
        VectorCopy(p->verts[i].xyz, tess.xyz[numv].data());
        tess._texCoords[numv][0] = tavros::math::vec2(p->verts[i].st[0], p->verts[i].st[1]);
        *(int32*) &tess.vertexColors[numv] = *(int32*) p->verts[i].modulateU8;

        numv++;
    }

    // generate fan indexes into the tess array
    for (i = 0; i < p->numVerts - 2; i++) {
        tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
        tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
        tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
        tess.numIndexes += 3;
    }

    tess.numVertexes = numv;
}


/*
=============
RB_SurfaceTriangles
=============
*/
void RB_SurfaceTriangles(srfTriangles_t* srf)
{
    int32       i;
    drawVert_t* dv;
    float *     xyz, *normal, *texCoords;
    uint8*      color;
    int32       dlightBits;
    bool        needsNormal;

    dlightBits = srf->dlightBits;
    tess.dlightBits |= dlightBits;

    RB_CHECKOVERFLOW(srf->numVerts, srf->numIndexes);

    for (i = 0; i < srf->numIndexes; i += 3) {
        tess.indexes[tess.numIndexes + i + 0] = tess.numVertexes + srf->indexes[i + 0];
        tess.indexes[tess.numIndexes + i + 1] = tess.numVertexes + srf->indexes[i + 1];
        tess.indexes[tess.numIndexes + i + 2] = tess.numVertexes + srf->indexes[i + 2];
    }
    tess.numIndexes += srf->numIndexes;

    dv = srf->verts;
    xyz = tess.xyz[tess.numVertexes].data();
    normal = tess.normal[tess.numVertexes].data();
    texCoords = tess._texCoords[tess.numVertexes][0].data();
    color = tess.vertexColors[tess.numVertexes];
    needsNormal = tess.shader->needsNormal;

    for (i = 0; i < srf->numVerts; i++, dv++, xyz += 4, normal += 4, texCoords += 4, color += 4) {
        xyz[0] = dv->xyz[0];
        xyz[1] = dv->xyz[1];
        xyz[2] = dv->xyz[2];

        if (needsNormal) {
            normal[0] = dv->normal[0];
            normal[1] = dv->normal[1];
            normal[2] = dv->normal[2];
        }

        texCoords[0] = dv->st[0];
        texCoords[1] = dv->st[1];

        texCoords[2] = dv->lightmap[0];
        texCoords[3] = dv->lightmap[1];

        *(int32*) color = *(int32*) dv->color;
    }

    for (i = 0; i < srf->numVerts; i++) {
        tess.vertexDlightBits[tess.numVertexes + i] = dlightBits;
    }

    tess.numVertexes += srf->numVerts;
}


/*
==============
RB_SurfaceBeam
==============
*/
void RB_SurfaceBeam()
{
#define NUM_BEAM_SEGS 6
    refEntity_t* e;
    int32        i;
    vec3_t       perpvec;
    vec3_t       direction, normalized_direction;
    vec3_t       start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
    vec3_t       oldorigin, origin;

    e = &backEnd.currentEntity->e;

    oldorigin[0] = e->oldorigin[0];
    oldorigin[1] = e->oldorigin[1];
    oldorigin[2] = e->oldorigin[2];

    origin[0] = e->origin[0];
    origin[1] = e->origin[1];
    origin[2] = e->origin[2];

    normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
    normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
    normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

    if (VectorNormalize(normalized_direction) == 0) {
        return;
    }

    PerpendicularVector(perpvec, normalized_direction);

    VectorScale(perpvec, 4, perpvec);

    for (i = 0; i < NUM_BEAM_SEGS; i++) {
        RotatePointAroundVector(start_points[i], normalized_direction, perpvec, (360.0 / NUM_BEAM_SEGS) * i);
        //        VectorAdd( start_points[i], origin, start_points[i] );
        VectorAdd(start_points[i], direction, end_points[i]);
    }

    GL_Bind(tr.whiteImage);

    GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

    qglColor3f(1, 0, 0);

    qglBegin(GL_TRIANGLE_STRIP);
    for (i = 0; i <= NUM_BEAM_SEGS; i++) {
        qglVertex3fv(start_points[i % NUM_BEAM_SEGS]);
        qglVertex3fv(end_points[i % NUM_BEAM_SEGS]);
    }
    qglEnd();
}

//================================================================================

static void DoRailCore(const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth)
{
    float spanWidth2;
    int32 vbase;
    float t = len / 256.0f;

    vbase = tess.numVertexes;

    spanWidth2 = -spanWidth;

    // FIXME: use quad stamp?
    VectorMA(start, spanWidth, up, tess.xyz[tess.numVertexes].data());
    tess._texCoords[tess.numVertexes][0] = tavros::math::vec2(0.0, 0.0);
    tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25;
    tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25;
    tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25;
    tess.numVertexes++;

    VectorMA(start, spanWidth2, up, tess.xyz[tess.numVertexes].data());
    tess._texCoords[tess.numVertexes][0] = tavros::math::vec2(0.0, 1.0);
    tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
    tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
    tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
    tess.numVertexes++;

    VectorMA(end, spanWidth, up, tess.xyz[tess.numVertexes].data());

    tess._texCoords[tess.numVertexes][0] = tavros::math::vec2(t, 0.0);
    tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
    tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
    tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
    tess.numVertexes++;

    VectorMA(end, spanWidth2, up, tess.xyz[tess.numVertexes].data());
    tess._texCoords[tess.numVertexes][0] = tavros::math::vec2(t, 1.0);
    tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
    tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
    tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
    tess.numVertexes++;

    tess.indexes[tess.numIndexes++] = vbase;
    tess.indexes[tess.numIndexes++] = vbase + 1;
    tess.indexes[tess.numIndexes++] = vbase + 2;

    tess.indexes[tess.numIndexes++] = vbase + 2;
    tess.indexes[tess.numIndexes++] = vbase + 1;
    tess.indexes[tess.numIndexes++] = vbase + 3;
}

/*
** RB_SurfaceRailCore
*/
void RB_SurfaceRailCore()
{
    refEntity_t* e;
    int32        len;
    vec3_t       right;
    vec3_t       vec;
    vec3_t       start, end;
    vec3_t       v1, v2;

    e = &backEnd.currentEntity->e;

    VectorCopy(e->oldorigin, start);
    VectorCopy(e->origin, end);

    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);

    // compute side vector
    VectorSubtract(start, backEnd.viewParms.orient.origin, v1);
    VectorNormalize(v1);
    VectorSubtract(end, backEnd.viewParms.orient.origin, v2);
    VectorNormalize(v2);
    CrossProduct(v1, v2, right);
    VectorNormalize(right);

    DoRailCore(start, end, right, len, 2);
}

/*
** RB_SurfaceLightningBolt
*/
void RB_SurfaceLightningBolt()
{
    refEntity_t* e;
    int32        len;
    vec3_t       right;
    vec3_t       vec;
    vec3_t       start, end;
    vec3_t       v1, v2;
    int32        i;

    e = &backEnd.currentEntity->e;

    VectorCopy(e->oldorigin, end);
    VectorCopy(e->origin, start);

    // compute variables
    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);

    // compute side vector
    VectorSubtract(start, backEnd.viewParms.orient.origin, v1);
    VectorNormalize(v1);
    VectorSubtract(end, backEnd.viewParms.orient.origin, v2);
    VectorNormalize(v2);
    CrossProduct(v1, v2, right);
    VectorNormalize(right);

    for (i = 0; i < 4; i++) {
        vec3_t temp;

        DoRailCore(start, end, right, len, 8);
        RotatePointAroundVector(temp, vec, right, 45);
        VectorCopy(temp, right);
    }
}

/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(tavros::math::vec4* normals, uint32 count)
{
    //    assert(count);
    while (count--) {
        VectorNormalizeFast(normals->data());
        normals++;
    }
}


/*
** LerpMeshVertexes
*/
static void LerpMeshVertexes(md3Surface_t* surf, float backlerp)
{
    int16 *oldXyz, *newXyz, *oldNormals, *newNormals;
    float *outXyz, *outNormal;
    float  oldXyzScale, newXyzScale;
    float  oldNormalScale, newNormalScale;
    int32  vertNum;
    int32  numVerts;

    outXyz = tess.xyz[tess.numVertexes].data();
    outNormal = tess.normal[tess.numVertexes].data();

    newXyz = (int16*) ((uint8*) surf + surf->ofsXyzNormals)
           + (backEnd.currentEntity->e.frame * surf->numVerts * 4);
    newNormals = newXyz + 3;

    newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
    newNormalScale = 1.0 - backlerp;

    numVerts = surf->numVerts;

    if (backlerp == 0) {
        //
        // just copy the vertexes
        //
        for (vertNum = 0; vertNum < numVerts; vertNum++, newXyz += 4, newNormals += 4, outXyz += 4, outNormal += 4) {
            outXyz[0] = newXyz[0] * newXyzScale;
            outXyz[1] = newXyz[1] * newXyzScale;
            outXyz[2] = newXyz[2] * newXyzScale;

            // extract longitude and latitude to uint8
            uint8 lng_b = (newNormals[0] >> 8) & 0xff;
            uint8 lat_b = (newNormals[0] >> 0) & 0xff;

            // convert from uint8 to radians
            float lng = UINT8_TO_RAD(lng_b);
            float lat = UINT8_TO_RAD(lat_b);

            outNormal[0] = cosf(lat) * sinf(lng);
            outNormal[1] = sinf(lat) * sinf(lng);
            outNormal[2] = cosf(lng);
        }
    } else {
        //
        // interpolate and copy the vertex and normal
        //
        oldXyz = (int16*) ((uint8*) surf + surf->ofsXyzNormals)
               + (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
        oldNormals = oldXyz + 3;

        oldXyzScale = MD3_XYZ_SCALE * backlerp;
        oldNormalScale = backlerp;

        for (vertNum = 0; vertNum < numVerts; vertNum++,
            oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
            outXyz += 4, outNormal += 4) {
            vec3_t uncompressedOldNormal, uncompressedNewNormal;

            // interpolate the xyz
            outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
            outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
            outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

            // FIXME: interpolate lat/long instead?
            // extract longitude and latitude to uint8
            uint8 lng_b = (newNormals[0] >> 8) & 0xff;
            uint8 lat_b = (newNormals[0] >> 0) & 0xff;

            // convert from uint8 to radians
            float lng = UINT8_TO_RAD(lng_b);
            float lat = UINT8_TO_RAD(lat_b);

            uncompressedNewNormal[0] = cosf(lat) * sinf(lng);
            uncompressedNewNormal[1] = sinf(lat) * sinf(lng);
            uncompressedNewNormal[2] = cosf(lng);

            // extract longitude and latitude to uint8
            lat = (oldNormals[0] >> 8) & 0xff;
            lng = (oldNormals[0] >> 0) & 0xff;

            // convert from uint8 to radians
            lng = UINT8_TO_RAD(lng_b);
            lat = UINT8_TO_RAD(lat_b);

            uncompressedOldNormal[0] = cosf(lat) * sinf(lng);
            uncompressedOldNormal[1] = sinf(lat) * sinf(lng);
            uncompressedOldNormal[2] = cosf(lng);

            outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
            outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
            outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

            //  VectorNormalize (outNormal);
        }
        VectorArrayNormalize(&tess.normal[tess.numVertexes], numVerts);
    }
}

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh(md3Surface_t* surface)
{
    int32  j;
    float  backlerp;
    int32* triangles;
    float* texCoords;
    int32  indexes;
    int32  Bob, Doug;
    int32  numVerts;

    if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) {
        backlerp = 0;
    } else {
        backlerp = backEnd.currentEntity->e.backlerp;
    }

    RB_CHECKOVERFLOW(surface->numVerts, surface->numTriangles * 3);

    LerpMeshVertexes(surface, backlerp);

    triangles = (int32*) ((uint8*) surface + surface->ofsTriangles);
    indexes = surface->numTriangles * 3;
    Bob = tess.numIndexes;
    Doug = tess.numVertexes;
    for (j = 0; j < indexes; j++) {
        tess.indexes[Bob + j] = Doug + triangles[j];
    }
    tess.numIndexes += indexes;

    texCoords = (float*) ((uint8*) surface + surface->ofsSt);

    numVerts = surface->numVerts;
    for (j = 0; j < numVerts; j++) {
        tess._texCoords[Doug + j][0] = tavros::math::vec2(texCoords[j * 2 + 0], texCoords[j * 2 + 1]);
        // FIXME: fill in lightmapST for completeness?
    }

    tess.numVertexes += surface->numVerts;
}


/*
==============
RB_SurfaceFace
==============
*/
void RB_SurfaceFace(srfSurfaceFace_t* surf)
{
    int32   i;
    uint32 *indices, *tessIndexes;
    float*  v;
    float*  normal;
    int32   ndx;
    int32   Bob;
    int32   numPoints;
    int32   dlightBits;

    RB_CHECKOVERFLOW(surf->numPoints, surf->numIndices);

    dlightBits = surf->dlightBits;
    tess.dlightBits |= dlightBits;

    indices = (uint32*) (((char*) surf) + surf->ofsIndices);

    Bob = tess.numVertexes;
    tessIndexes = tess.indexes + tess.numIndexes;
    for (i = surf->numIndices - 1; i >= 0; i--) {
        tessIndexes[i] = indices[i] + Bob;
    }

    tess.numIndexes += surf->numIndices;

    v = surf->points[0];

    ndx = tess.numVertexes;

    numPoints = surf->numPoints;

    if (tess.shader->needsNormal) {
        normal = surf->plane.normal;
        for (i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++) {
            VectorCopy(normal, tess.normal[ndx].data());
        }
    }

    for (i = 0, v = surf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++) {
        VectorCopy(v, tess.xyz[ndx].data());
        tess._texCoords[ndx][0] = tavros::math::vec2(v[3], v[4]);
        tess._texCoords[ndx][1] = tavros::math::vec2(v[5], v[6]);
        *(uint32*) &tess.vertexColors[ndx] = *(uint32*) &v[7];
        tess.vertexDlightBits[ndx] = dlightBits;
    }


    tess.numVertexes += surf->numPoints;
}


static float LodErrorForVolume(vec3_t local, float radius)
{
    vec3_t world;
    float  d;

    // never let it go negative
    if (r_lodCurveError->value < 0) {
        return 0;
    }

    world[0] = local[0] * backEnd.orient.axis[0][0] + local[1] * backEnd.orient.axis[1][0] + local[2] * backEnd.orient.axis[2][0] + backEnd.orient.origin[0];
    world[1] = local[0] * backEnd.orient.axis[0][1] + local[1] * backEnd.orient.axis[1][1] + local[2] * backEnd.orient.axis[2][1] + backEnd.orient.origin[1];
    world[2] = local[0] * backEnd.orient.axis[0][2] + local[1] * backEnd.orient.axis[1][2] + local[2] * backEnd.orient.axis[2][2] + backEnd.orient.origin[2];

    VectorSubtract(world, backEnd.viewParms.orient.origin, world);
    d = DotProduct(world, backEnd.viewParms.orient.axis[0]);

    if (d < 0) {
        d = -d;
    }
    d -= radius;
    if (d < 1) {
        d = 1;
    }

    return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
void RB_SurfaceGrid(srfGridMesh_t* cv)
{
    int32       i, j;
    float*      xyz;
    float*      texCoords;
    float*      normal;
    uint8*      color;
    drawVert_t* dv;
    int32       rows, irows, vrows;
    int32       used;
    int32       widthTable[MAX_GRID_SIZE];
    int32       heightTable[MAX_GRID_SIZE];
    float       lodError;
    int32       lodWidth, lodHeight;
    int32       numVertexes;
    int32       dlightBits;
    int32*      vDlightBits;
    bool        needsNormal;

    dlightBits = cv->dlightBits;
    tess.dlightBits |= dlightBits;

    // determine the allowable discrepance
    lodError = LodErrorForVolume(cv->lodOrigin, cv->lodRadius);

    // determine which rows and columns of the subdivision
    // we are actually going to use
    widthTable[0] = 0;
    lodWidth = 1;
    for (i = 1; i < cv->width - 1; i++) {
        if (cv->widthLodError[i] <= lodError) {
            widthTable[lodWidth] = i;
            lodWidth++;
        }
    }
    widthTable[lodWidth] = cv->width - 1;
    lodWidth++;

    heightTable[0] = 0;
    lodHeight = 1;
    for (i = 1; i < cv->height - 1; i++) {
        if (cv->heightLodError[i] <= lodError) {
            heightTable[lodHeight] = i;
            lodHeight++;
        }
    }
    heightTable[lodHeight] = cv->height - 1;
    lodHeight++;


    // very large grids may have more points or indexes than can be fit
    // in the tess structure, so we may have to issue it in multiple passes

    used = 0;
    rows = 0;
    while (used < lodHeight - 1) {
        // see how many rows of both verts and indexes we can add without overflowing
        do {
            vrows = (SHADER_MAX_VERTEXES - tess.numVertexes) / lodWidth;
            irows = (SHADER_MAX_INDEXES - tess.numIndexes) / (lodWidth * 6);

            // if we don't have enough space for at least one strip, flush the buffer
            if (vrows < 2 || irows < 1) {
                RB_EndSurface();
                RB_BeginSurface(tess.shader, tess.fogNum);
            } else {
                break;
            }
        } while (1);

        rows = irows;
        if (vrows < irows + 1) {
            rows = vrows - 1;
        }
        if (used + rows > lodHeight) {
            rows = lodHeight - used;
        }

        numVertexes = tess.numVertexes;

        xyz = tess.xyz[numVertexes].data();
        normal = tess.normal[numVertexes].data();
        texCoords = tess._texCoords[numVertexes][0].data();
        color = (uint8*) &tess.vertexColors[numVertexes];
        vDlightBits = &tess.vertexDlightBits[numVertexes];
        needsNormal = tess.shader->needsNormal;

        for (i = 0; i < rows; i++) {
            for (j = 0; j < lodWidth; j++) {
                dv = cv->verts + heightTable[used + i] * cv->width
                   + widthTable[j];

                xyz[0] = dv->xyz[0];
                xyz[1] = dv->xyz[1];
                xyz[2] = dv->xyz[2];
                texCoords[0] = dv->st[0];
                texCoords[1] = dv->st[1];
                texCoords[2] = dv->lightmap[0];
                texCoords[3] = dv->lightmap[1];
                if (needsNormal) {
                    normal[0] = dv->normal[0];
                    normal[1] = dv->normal[1];
                    normal[2] = dv->normal[2];
                }
                *(uint32*) color = *(uint32*) dv->color;
                *vDlightBits++ = dlightBits;
                xyz += 4;
                normal += 4;
                texCoords += 4;
                color += 4;
            }
        }


        // add the indexes
        {
            int32 numIndexes;
            int32 w, h;

            h = rows - 1;
            w = lodWidth - 1;
            numIndexes = tess.numIndexes;
            for (i = 0; i < h; i++) {
                for (j = 0; j < w; j++) {
                    int32 v1, v2, v3, v4;

                    // vertex order to be reckognized as tristrips
                    v1 = numVertexes + i * lodWidth + j + 1;
                    v2 = v1 - 1;
                    v3 = v2 + lodWidth;
                    v4 = v3 + 1;

                    tess.indexes[numIndexes] = v2;
                    tess.indexes[numIndexes + 1] = v3;
                    tess.indexes[numIndexes + 2] = v1;

                    tess.indexes[numIndexes + 3] = v1;
                    tess.indexes[numIndexes + 4] = v3;
                    tess.indexes[numIndexes + 5] = v4;
                    numIndexes += 6;
                }
            }

            tess.numIndexes = numIndexes;
        }

        tess.numVertexes += rows * lodWidth;

        used += rows - 1;
    }
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
void RB_SurfaceAxis()
{
    GL_Bind(tr.whiteImage);
    qglLineWidth(3);
    qglBegin(GL_LINES);
    qglColor3f(1, 0, 0);
    qglVertex3f(0, 0, 0);
    qglVertex3f(16, 0, 0);
    qglColor3f(0, 1, 0);
    qglVertex3f(0, 0, 0);
    qglVertex3f(0, 16, 0);
    qglColor3f(0, 0, 1);
    qglVertex3f(0, 0, 0);
    qglVertex3f(0, 0, 16);
    qglEnd();
    qglLineWidth(1);
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
void RB_SurfaceEntity(surfaceType_t* surfType)
{
    switch (backEnd.currentEntity->e.reType) {
    case RT_SPRITE:
        RB_SurfaceSprite();
        break;
    case RT_BEAM:
        RB_SurfaceBeam();
        break;
    case RT_RAIL_CORE:
        RB_SurfaceRailCore();
        break;
    case RT_LIGHTNING:
        RB_SurfaceLightningBolt();
        break;
    default:
        RB_SurfaceAxis();
        break;
    }
    return;
}

void RB_SurfaceBad(surfaceType_t* surfType)
{
    logger.info("Bad surface tesselated.");
}


void RB_SurfaceFlare(srfFlare_t* surf)
{
}


void RB_SurfaceDisplayList(srfDisplayList_t* surf)
{
    // all apropriate state must be set in RB_BeginSurface
    // this isn't implemented yet...
    qglCallList(surf->listNum);
}

void RB_SurfaceSkip(void* surf)
{
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void*) = {
    (void (*)(void*)) RB_SurfaceBad,        // SF_BAD,
    (void (*)(void*)) RB_SurfaceSkip,       // SF_SKIP,
    (void (*)(void*)) RB_SurfaceFace,       // SF_FACE,
    (void (*)(void*)) RB_SurfaceGrid,       // SF_GRID,
    (void (*)(void*)) RB_SurfaceTriangles,  // SF_TRIANGLES,
    (void (*)(void*)) RB_SurfacePolychain,  // SF_POLY,
    (void (*)(void*)) RB_SurfaceMesh,       // SF_MD3,
    (void (*)(void*)) RB_SurfaceAnim,       // SF_MD4,
    (void (*)(void*)) RB_SurfaceFlare,      // SF_FLARE,
    (void (*)(void*)) RB_SurfaceEntity,     // SF_ENTITY
    (void (*)(void*)) RB_SurfaceDisplayList // SF_DISPLAY_LIST
};
