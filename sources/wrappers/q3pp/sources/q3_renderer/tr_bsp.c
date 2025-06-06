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
// tr_map.c

#include "tr_local.h"

static tavros::core::logger logger("tr_bsp");

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

static world_t s_worldData;
static uint8*  fileBase;

int32 c_gridVerts;

//===============================================================================

static void HSVtoRGB(float h, float s, float v, float rgb[3])
{
    int32 i;
    float f;
    float p, q, t;

    h *= 5;

    i = floor(h);
    f = h - i;

    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i) {
    case 0:
        rgb[0] = v;
        rgb[1] = t;
        rgb[2] = p;
        break;
    case 1:
        rgb[0] = q;
        rgb[1] = v;
        rgb[2] = p;
        break;
    case 2:
        rgb[0] = p;
        rgb[1] = v;
        rgb[2] = t;
        break;
    case 3:
        rgb[0] = p;
        rgb[1] = q;
        rgb[2] = v;
        break;
    case 4:
        rgb[0] = t;
        rgb[1] = p;
        rgb[2] = v;
        break;
    case 5:
        rgb[0] = v;
        rgb[1] = p;
        rgb[2] = q;
        break;
    }
}

/*
===============
R_ColorShiftLightingBytes

===============
*/
static void R_ColorShiftLightingBytes(uint8 in[4], uint8 out[4])
{
    int32 shift, r, g, b;

    // shift the color data based on overbright range
    shift = 2;

    // shift the data based on overbright range
    r = in[0] << shift;
    g = in[1] << shift;
    b = in[2] << shift;

    // normalize by color instead of saturating to white
    if ((r | g | b) > 255) {
        int32 max;

        max = r > g ? r : g;
        max = max > b ? max : b;
        r = r * 255 / max;
        g = g * 255 / max;
        b = b * 255 / max;
    }

    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = in[3];
}

/*
===============
R_LoadLightmaps

===============
*/
#define LIGHTMAP_SIZE 128
static void R_LoadLightmaps(lump_t* l)
{
    uint8 *buf, *buf_p;
    int32  len;
    uint8  image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
    int32  i, j;
    float  maxIntensity = 0;
    double sumIntensity = 0;

    len = l->filelen;
    if (!len) {
        return;
    }
    buf = fileBase + l->fileofs;

    // create all the lightmaps
    tr.numLightmaps = len / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);
    if (tr.numLightmaps == 1) {
        // FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
        // this avoids this, but isn't the correct solution.
        tr.numLightmaps++;
    }

    for (i = 0; i < tr.numLightmaps; i++) {
        // expand the 24 bit on-disk to 32 bit
        buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

        if (r_lightmap->integer == 2) { // color code by intensity as development tool    (FIXME: check range)
            for (j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++) {
                float r = buf_p[j * 3 + 0];
                float g = buf_p[j * 3 + 1];
                float b = buf_p[j * 3 + 2];
                float intensity;
                float out[3];

                intensity = 0.33f * r + 0.685f * g + 0.063f * b;

                if (intensity > 255) {
                    intensity = 1.0f;
                } else {
                    intensity /= 255.0f;
                }

                if (intensity > maxIntensity) {
                    maxIntensity = intensity;
                }

                HSVtoRGB(intensity, 1.00, 0.50, out);

                image[j * 4 + 0] = out[0] * 255;
                image[j * 4 + 1] = out[1] * 255;
                image[j * 4 + 2] = out[2] * 255;
                image[j * 4 + 3] = 255;

                sumIntensity += intensity;
            }
        } else {
            for (j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++) {
                R_ColorShiftLightingBytes(&buf_p[j * 3], &image[j * 4]);
                image[j * 4 + 3] = 255;
            }
        }

        const char* name = va("*lightmap%d", i);
        tr.lightmaps[i] = create_texture(name, LIGHTMAP_SIZE, LIGHTMAP_SIZE, image);
        set_texture_sampler(tr.lightmaps[i], GL_LINEAR, GL_LINEAR, GL_CLAMP);
    }

    if (r_lightmap->integer == 2) {
        logger.info("Brightest lightmap value: %d", (int32) (maxIntensity * 255));
    }
}

/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility(lump_t* l)
{
    int32  len;
    uint8* buf;

    len = (s_worldData.numClusters + 63) & ~63;
    s_worldData.novis = (uint8*) Hunk_Alloc(len, h_low);
    Com_Memset(s_worldData.novis, 0xff, len);

    len = l->filelen;
    if (!len) {
        return;
    }
    buf = fileBase + l->fileofs;

    s_worldData.numClusters = (((int32*) buf)[0]);
    s_worldData.clusterBytes = (((int32*) buf)[1]);

    // CM_Load should have given us the vis data to share, so
    // we don't need to allocate another copy
    if (tr.externalVisData) {
        s_worldData.vis = tr.externalVisData;
    } else {
        uint8* dest;

        dest = (uint8*) Hunk_Alloc(len - 8, h_low);
        Com_Memcpy(dest, buf + 8, len - 8);
        s_worldData.vis = dest;
    }
}

//===============================================================================


/*
===============
ShaderForShaderNum
===============
*/
static shader_t* ShaderForShaderNum(int32 shaderNum, int32 lightmapNum)
{
    shader_t*  shader;
    dshader_t* dsh;

    shaderNum = (shaderNum);
    if (shaderNum < 0 || shaderNum >= s_worldData.numShaders) {
        Com_Error(ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum);
    }
    dsh = &s_worldData.shaders[shaderNum];

    shader = R_FindShader(dsh->shader, lightmapNum, true);

    // if the shader had errors, just use default shader
    if (shader->defaultShader) {
        return tr.defaultShader;
    }

    return shader;
}

/*
===============
ParseFace
===============
*/
static void ParseFace(dsurface_t* ds, drawVert_t* verts, msurface_t* surf, int32* indexes)
{
    int32             i, j;
    srfSurfaceFace_t* cv;
    int32             numPoints, numIndexes;
    int32             lightmapNum;
    int32             sfaceSize, ofsIndexes;

    lightmapNum = (ds->lightmapNum);

    // get fog volume
    surf->fogIndex = (ds->fogNum) + 1;

    // get shader value
    surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);

    numPoints = (ds->numVerts);
    if (numPoints > MAX_FACE_POINTS) {
        logger.warning("MAX_FACE_POINTS exceeded: %i", numPoints);
        numPoints = MAX_FACE_POINTS;
        surf->shader = tr.defaultShader;
    }

    numIndexes = (ds->numIndexes);

    // create the srfSurfaceFace_t
    sfaceSize = (int32) & ((srfSurfaceFace_t*) 0)->points[numPoints];
    ofsIndexes = sfaceSize;
    sfaceSize += sizeof(int32) * numIndexes;

    cv = (srfSurfaceFace_t*) Hunk_Alloc(sfaceSize, h_low);
    cv->surfaceType = SF_FACE;
    cv->numPoints = numPoints;
    cv->numIndices = numIndexes;
    cv->ofsIndices = ofsIndexes;

    verts += (ds->firstVert);
    for (i = 0; i < numPoints; i++) {
        for (j = 0; j < 3; j++) {
            cv->points[i][j] = (verts[i].xyz[j]);
        }
        for (j = 0; j < 2; j++) {
            cv->points[i][3 + j] = (verts[i].st[j]);
            cv->points[i][5 + j] = (verts[i].lightmap[j]);
        }
        R_ColorShiftLightingBytes(verts[i].color, (uint8*) &cv->points[i][7]);
    }

    indexes += (ds->firstIndex);
    for (i = 0; i < numIndexes; i++) {
        ((int32*) ((uint8*) cv + cv->ofsIndices))[i] = (indexes[i]);
    }

    // take the plane information from the lightmap vector
    for (i = 0; i < 3; i++) {
        cv->plane.normal[i] = (ds->lightmapVecs[2][i]);
    }
    cv->plane.dist = DotProduct(cv->points[0], cv->plane.normal);
    SetPlaneSignbits(&cv->plane);
    cv->plane.side_type = PlaneTypeForNormal(cv->plane.normal);

    surf->data = (surfaceType_t*) cv;
}


/*
===============
ParseMesh
===============
*/
static void ParseMesh(dsurface_t* ds, drawVert_t* verts, msurface_t* surf)
{
    srfGridMesh_t*       grid;
    int32                i, j;
    int32                width, height, numPoints;
    drawVert_t           points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
    int32                lightmapNum;
    vec3_t               bounds[2];
    vec3_t               tmpVec;
    static surfaceType_t skipData = SF_SKIP;

    lightmapNum = (ds->lightmapNum);

    // get fog volume
    surf->fogIndex = (ds->fogNum) + 1;

    // get shader value
    surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);

    // we may have a nodraw surface, because they might still need to
    // be around for movement clipping
    if (s_worldData.shaders[(ds->shaderNum)].surfaceFlags & SURF_NODRAW) {
        surf->data = &skipData;
        return;
    }

    width = (ds->patchWidth);
    height = (ds->patchHeight);

    verts += (ds->firstVert);
    numPoints = width * height;
    for (i = 0; i < numPoints; i++) {
        for (j = 0; j < 3; j++) {
            points[i].xyz[j] = (verts[i].xyz[j]);
            points[i].normal[j] = (verts[i].normal[j]);
        }
        for (j = 0; j < 2; j++) {
            points[i].st[j] = (verts[i].st[j]);
            points[i].lightmap[j] = (verts[i].lightmap[j]);
        }
        R_ColorShiftLightingBytes(verts[i].color, points[i].color);
    }

    // pre-tesseleate
    grid = R_SubdividePatchToGrid(width, height, points);
    surf->data = (surfaceType_t*) grid;

    // copy the level of detail origin, which is the center
    // of the group of all curves that must subdivide the same
    // to avoid cracking
    for (i = 0; i < 3; i++) {
        bounds[0][i] = (ds->lightmapVecs[0][i]);
        bounds[1][i] = (ds->lightmapVecs[1][i]);
    }
    VectorAdd(bounds[0], bounds[1], bounds[1]);
    VectorScale(bounds[1], 0.5f, grid->lodOrigin);
    VectorSubtract(bounds[0], grid->lodOrigin, tmpVec);
    grid->lodRadius = VectorLength(tmpVec);
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf(dsurface_t* ds, drawVert_t* verts, msurface_t* surf, int32* indexes)
{
    srfTriangles_t* tri;
    int32           i, j;
    int32           numVerts, numIndexes;

    // get fog volume
    surf->fogIndex = (ds->fogNum) + 1;

    // get shader
    surf->shader = ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);

    numVerts = (ds->numVerts);
    numIndexes = (ds->numIndexes);

    tri = (srfTriangles_t*) Hunk_Alloc(sizeof(*tri) + numVerts * sizeof(tri->verts[0]) + numIndexes * sizeof(tri->indexes[0]), h_low);
    tri->surfaceType = SF_TRIANGLES;
    tri->numVerts = numVerts;
    tri->numIndexes = numIndexes;
    tri->verts = (drawVert_t*) (tri + 1);
    tri->indexes = (int32*) (tri->verts + tri->numVerts);

    surf->data = (surfaceType_t*) tri;

    // copy vertexes
    ClearBounds(tri->bounds[0], tri->bounds[1]);
    verts += (ds->firstVert);
    for (i = 0; i < numVerts; i++) {
        for (j = 0; j < 3; j++) {
            tri->verts[i].xyz[j] = (verts[i].xyz[j]);
            tri->verts[i].normal[j] = (verts[i].normal[j]);
        }
        AddPointToBounds(tri->verts[i].xyz, tri->bounds[0], tri->bounds[1]);
        for (j = 0; j < 2; j++) {
            tri->verts[i].st[j] = (verts[i].st[j]);
            tri->verts[i].lightmap[j] = (verts[i].lightmap[j]);
        }

        R_ColorShiftLightingBytes(verts[i].color, tri->verts[i].color);
    }

    // copy indexes
    indexes += (ds->firstIndex);
    for (i = 0; i < numIndexes; i++) {
        tri->indexes[i] = (indexes[i]);
        if (tri->indexes[i] < 0 || tri->indexes[i] >= numVerts) {
            Com_Error(ERR_DROP, "Bad index in triangle surface");
        }
    }
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare(dsurface_t* ds, drawVert_t* verts, msurface_t* surf, int32* indexes)
{
    srfFlare_t* flare;
    int32       i;

    // get fog volume
    surf->fogIndex = (ds->fogNum) + 1;

    // get shader
    surf->shader = ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);

    flare = (srfFlare_t*) Hunk_Alloc(sizeof(*flare), h_low);
    flare->surfaceType = SF_FLARE;

    surf->data = (surfaceType_t*) flare;

    for (i = 0; i < 3; i++) {
        flare->origin[i] = (ds->lightmapOrigin[i]);
        flare->color[i] = (ds->lightmapVecs[0][i]);
        flare->normal[i] = (ds->lightmapVecs[2][i]);
    }
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int32 R_MergedWidthPoints(srfGridMesh_t* grid, int32 offset)
{
    int32 i, j;

    for (i = 1; i < grid->width - 1; i++) {
        for (j = i + 1; j < grid->width - 1; j++) {
            if (fabs(grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0]) > .1) {
                continue;
            }
            if (fabs(grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1]) > .1) {
                continue;
            }
            if (fabs(grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2]) > .1) {
                continue;
            }
            return true;
        }
    }
    return false;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int32 R_MergedHeightPoints(srfGridMesh_t* grid, int32 offset)
{
    int32 i, j;

    for (i = 1; i < grid->height - 1; i++) {
        for (j = i + 1; j < grid->height - 1; j++) {
            if (fabs(grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0]) > .1) {
                continue;
            }
            if (fabs(grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1]) > .1) {
                continue;
            }
            if (fabs(grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2]) > .1) {
                continue;
            }
            return true;
        }
    }
    return false;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r(int32 start, srfGridMesh_t* grid1)
{
    int32          j, k, l, m, n, offset1, offset2, touch;
    srfGridMesh_t* grid2;

    for (j = start; j < s_worldData.numsurfaces; j++) {
        //
        grid2 = (srfGridMesh_t*) s_worldData.surfaces[j].data;
        // if this surface is not a grid
        if (grid2->surfaceType != SF_GRID) {
            continue;
        }
        // if the LOD errors are already fixed for this patch
        if (grid2->lodFixed == 2) {
            continue;
        }
        // grids in the same LOD group should have the exact same lod radius
        if (grid1->lodRadius != grid2->lodRadius) {
            continue;
        }
        // grids in the same LOD group should have the exact same lod origin
        if (grid1->lodOrigin[0] != grid2->lodOrigin[0]) {
            continue;
        }
        if (grid1->lodOrigin[1] != grid2->lodOrigin[1]) {
            continue;
        }
        if (grid1->lodOrigin[2] != grid2->lodOrigin[2]) {
            continue;
        }
        //
        touch = false;
        for (n = 0; n < 2; n++) {
            //
            if (n) {
                offset1 = (grid1->height - 1) * grid1->width;
            } else {
                offset1 = 0;
            }
            if (R_MergedWidthPoints(grid1, offset1)) {
                continue;
            }
            for (k = 1; k < grid1->width - 1; k++) {
                for (m = 0; m < 2; m++) {
                    if (m) {
                        offset2 = (grid2->height - 1) * grid2->width;
                    } else {
                        offset2 = 0;
                    }
                    if (R_MergedWidthPoints(grid2, offset2)) {
                        continue;
                    }
                    for (l = 1; l < grid2->width - 1; l++) {
                        //
                        if (fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) {
                            continue;
                        }
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
                for (m = 0; m < 2; m++) {
                    if (m) {
                        offset2 = grid2->width - 1;
                    } else {
                        offset2 = 0;
                    }
                    if (R_MergedHeightPoints(grid2, offset2)) {
                        continue;
                    }
                    for (l = 1; l < grid2->height - 1; l++) {
                        //
                        if (fabs(grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) {
                            continue;
                        }
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->widthLodError[k];
                        touch = true;
                    }
                }
            }
        }
        for (n = 0; n < 2; n++) {
            //
            if (n) {
                offset1 = grid1->width - 1;
            } else {
                offset1 = 0;
            }
            if (R_MergedHeightPoints(grid1, offset1)) {
                continue;
            }
            for (k = 1; k < grid1->height - 1; k++) {
                for (m = 0; m < 2; m++) {
                    if (m) {
                        offset2 = (grid2->height - 1) * grid2->width;
                    } else {
                        offset2 = 0;
                    }
                    if (R_MergedWidthPoints(grid2, offset2)) {
                        continue;
                    }
                    for (l = 1; l < grid2->width - 1; l++) {
                        //
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2]) > .1) {
                            continue;
                        }
                        // ok the points are equal and should have the same lod error
                        grid2->widthLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
                for (m = 0; m < 2; m++) {
                    if (m) {
                        offset2 = grid2->width - 1;
                    } else {
                        offset2 = 0;
                    }
                    if (R_MergedHeightPoints(grid2, offset2)) {
                        continue;
                    }
                    for (l = 1; l < grid2->height - 1; l++) {
                        //
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1]) > .1) {
                            continue;
                        }
                        if (fabs(grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2]) > .1) {
                            continue;
                        }
                        // ok the points are equal and should have the same lod error
                        grid2->heightLodError[l] = grid1->heightLodError[k];
                        touch = true;
                    }
                }
            }
        }
        if (touch) {
            grid2->lodFixed = 2;
            R_FixSharedVertexLodError_r(start, grid2);
            // NOTE: this would be correct but makes things really slow
            // grid2->lodFixed = 1;
        }
    }
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError()
{
    int32          i;
    srfGridMesh_t* grid1;

    for (i = 0; i < s_worldData.numsurfaces; i++) {
        //
        grid1 = (srfGridMesh_t*) s_worldData.surfaces[i].data;
        // if this surface is not a grid
        if (grid1->surfaceType != SF_GRID) {
            continue;
        }
        //
        if (grid1->lodFixed) {
            continue;
        }
        //
        grid1->lodFixed = 2;
        // recursively fix other patches in the same LOD group
        R_FixSharedVertexLodError_r(i + 1, grid1);
    }
}


/*
===============
R_StitchPatches
===============
*/
int32 R_StitchPatches(int32 grid1num, int32 grid2num)
{
    float *        v1, *v2;
    srfGridMesh_t *grid1, *grid2;
    int32          k, l, m, n, offset1, offset2, row, column;

    grid1 = (srfGridMesh_t*) s_worldData.surfaces[grid1num].data;
    grid2 = (srfGridMesh_t*) s_worldData.surfaces[grid2num].data;
    for (n = 0; n < 2; n++) {
        //
        if (n) {
            offset1 = (grid1->height - 1) * grid1->width;
        } else {
            offset1 = 0;
        }
        if (R_MergedWidthPoints(grid1, offset1)) {
            continue;
        }
        for (k = 0; k < grid1->width - 2; k += 2) {
            for (m = 0; m < 2; m++) {
                if (grid2->width >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = (grid2->height - 1) * grid2->width;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->width - 1; l++) {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert column into grid2 right after after column l
                    if (m) {
                        row = grid2->height - 1;
                    } else {
                        row = 0;
                    }
                    grid2 = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
            for (m = 0; m < 2; m++) {
                if (grid2->height >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = grid2->width - 1;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->height - 1; l++) {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[k + 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert row into grid2 right after after row l
                    if (m) {
                        column = grid2->width - 1;
                    } else {
                        column = 0;
                    }
                    grid2 = R_GridInsertRow(grid2, l + 1, column, grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
        }
    }
    for (n = 0; n < 2; n++) {
        //
        if (n) {
            offset1 = grid1->width - 1;
        } else {
            offset1 = 0;
        }
        if (R_MergedHeightPoints(grid1, offset1)) {
            continue;
        }
        for (k = 0; k < grid1->height - 2; k += 2) {
            for (m = 0; m < 2; m++) {
                if (grid2->width >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = (grid2->height - 1) * grid2->width;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->width - 1; l++) {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[(l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert column into grid2 right after after column l
                    if (m) {
                        row = grid2->height - 1;
                    } else {
                        row = 0;
                    }
                    grid2 = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
            for (m = 0; m < 2; m++) {
                if (grid2->height >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = grid2->width - 1;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->height - 1; l++) {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[grid1->width * (k + 2) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert row into grid2 right after after row l
                    if (m) {
                        column = grid2->width - 1;
                    } else {
                        column = 0;
                    }
                    grid2 = R_GridInsertRow(grid2, l + 1, column, grid1->verts[grid1->width * (k + 1) + offset1].xyz, grid1->heightLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
        }
    }
    for (n = 0; n < 2; n++) {
        //
        if (n) {
            offset1 = (grid1->height - 1) * grid1->width;
        } else {
            offset1 = 0;
        }
        if (R_MergedWidthPoints(grid1, offset1)) {
            continue;
        }
        for (k = grid1->width - 1; k > 1; k -= 2) {
            for (m = 0; m < 2; m++) {
                if (grid2->width >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = (grid2->height - 1) * grid2->width;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->width - 1; l++) {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[(l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert column into grid2 right after after column l
                    if (m) {
                        row = grid2->height - 1;
                    } else {
                        row = 0;
                    }
                    grid2 = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
            for (m = 0; m < 2; m++) {
                if (grid2->height >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = grid2->width - 1;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->height - 1; l++) {
                    //
                    v1 = grid1->verts[k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[k - 2 + offset1].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert row into grid2 right after after row l
                    if (m) {
                        column = grid2->width - 1;
                    } else {
                        column = 0;
                    }
                    grid2 = R_GridInsertRow(grid2, l + 1, column, grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1]);
                    if (!grid2) {
                        break;
                    }
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
        }
    }
    for (n = 0; n < 2; n++) {
        //
        if (n) {
            offset1 = grid1->width - 1;
        } else {
            offset1 = 0;
        }
        if (R_MergedHeightPoints(grid1, offset1)) {
            continue;
        }
        for (k = grid1->height - 1; k > 1; k -= 2) {
            for (m = 0; m < 2; m++) {
                if (grid2->width >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = (grid2->height - 1) * grid2->width;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->width - 1; l++) {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
                    v2 = grid2->verts[l + 1 + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[l + offset2].xyz;
                    v2 = grid2->verts[(l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert column into grid2 right after after column l
                    if (m) {
                        row = grid2->height - 1;
                    } else {
                        row = 0;
                    }
                    grid2 = R_GridInsertColumn(grid2, l + 1, row, grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
            for (m = 0; m < 2; m++) {
                if (grid2->height >= MAX_GRID_SIZE) {
                    break;
                }
                if (m) {
                    offset2 = grid2->width - 1;
                } else {
                    offset2 = 0;
                }
                for (l = 0; l < grid2->height - 1; l++) {
                    //
                    v1 = grid1->verts[grid1->width * k + offset1].xyz;
                    v2 = grid2->verts[grid2->width * l + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }

                    v1 = grid1->verts[grid1->width * (k - 2) + offset1].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) > .1) {
                        continue;
                    }
                    if (fabs(v1[1] - v2[1]) > .1) {
                        continue;
                    }
                    if (fabs(v1[2] - v2[2]) > .1) {
                        continue;
                    }
                    //
                    v1 = grid2->verts[grid2->width * l + offset2].xyz;
                    v2 = grid2->verts[grid2->width * (l + 1) + offset2].xyz;
                    if (fabs(v1[0] - v2[0]) < .01 && fabs(v1[1] - v2[1]) < .01 && fabs(v1[2] - v2[2]) < .01) {
                        continue;
                    }
                    //
                    // found highest LoD crack between two patches
                    // insert row into grid2 right after after row l
                    if (m) {
                        column = grid2->width - 1;
                    } else {
                        column = 0;
                    }
                    grid2 = R_GridInsertRow(grid2, l + 1, column, grid1->verts[grid1->width * (k - 1) + offset1].xyz, grid1->heightLodError[k + 1]);
                    grid2->lodStitched = false;
                    s_worldData.surfaces[grid2num].data = (surfaceType_t*) grid2;
                    return true;
                }
            }
        }
    }
    return false;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int32 R_TryStitchingPatch(int32 grid1num)
{
    int32          j, numstitches;
    srfGridMesh_t *grid1, *grid2;

    numstitches = 0;
    grid1 = (srfGridMesh_t*) s_worldData.surfaces[grid1num].data;
    for (j = 0; j < s_worldData.numsurfaces; j++) {
        //
        grid2 = (srfGridMesh_t*) s_worldData.surfaces[j].data;
        // if this surface is not a grid
        if (grid2->surfaceType != SF_GRID) {
            continue;
        }
        // grids in the same LOD group should have the exact same lod radius
        if (grid1->lodRadius != grid2->lodRadius) {
            continue;
        }
        // grids in the same LOD group should have the exact same lod origin
        if (grid1->lodOrigin[0] != grid2->lodOrigin[0]) {
            continue;
        }
        if (grid1->lodOrigin[1] != grid2->lodOrigin[1]) {
            continue;
        }
        if (grid1->lodOrigin[2] != grid2->lodOrigin[2]) {
            continue;
        }
        //
        while (R_StitchPatches(grid1num, j)) {
            numstitches++;
        }
    }
    return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches()
{
    int32          i, stitched, numstitches;
    srfGridMesh_t* grid1;

    numstitches = 0;
    do {
        stitched = false;
        for (i = 0; i < s_worldData.numsurfaces; i++) {
            //
            grid1 = (srfGridMesh_t*) s_worldData.surfaces[i].data;
            // if this surface is not a grid
            if (grid1->surfaceType != SF_GRID) {
                continue;
            }
            //
            if (grid1->lodStitched) {
                continue;
            }
            //
            grid1->lodStitched = true;
            stitched = true;
            //
            numstitches += R_TryStitchingPatch(i);
        }
    } while (stitched);
    logger.info("Stitched %d LoD cracks", numstitches);
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk()
{
    int32          i, size;
    srfGridMesh_t *grid, *hunkgrid;

    for (i = 0; i < s_worldData.numsurfaces; i++) {
        //
        grid = (srfGridMesh_t*) s_worldData.surfaces[i].data;
        // if this surface is not a grid
        if (grid->surfaceType != SF_GRID) {
            continue;
        }
        //
        size = (grid->width * grid->height - 1) * sizeof(drawVert_t) + sizeof(*grid);
        hunkgrid = (srfGridMesh_t*) Hunk_Alloc(size, h_low);
        Com_Memcpy(hunkgrid, grid, size);

        hunkgrid->widthLodError = (float*) Hunk_Alloc(grid->width * 4, h_low);
        Com_Memcpy(hunkgrid->widthLodError, grid->widthLodError, grid->width * 4);

        hunkgrid->heightLodError = (float*) Hunk_Alloc(grid->height * 4, h_low);
        Com_Memcpy(grid->heightLodError, grid->heightLodError, grid->height * 4);

        R_FreeSurfaceGridMesh(grid);

        s_worldData.surfaces[i].data = (surfaceType_t*) hunkgrid;
    }
}

/*
===============
R_LoadSurfaces
===============
*/
static void R_LoadSurfaces(lump_t* surfs, lump_t* verts, lump_t* indexLump)
{
    dsurface_t* in;
    msurface_t* out;
    drawVert_t* dv;
    int32*      indexes;
    int32       count;
    int32       numFaces, numMeshes, numTriSurfs, numFlares;
    int32       i;

    numFaces = 0;
    numMeshes = 0;
    numTriSurfs = 0;
    numFlares = 0;

    in = (dsurface_t*) (fileBase + surfs->fileofs);
    if (surfs->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = surfs->filelen / sizeof(*in);

    dv = (drawVert_t*) (fileBase + verts->fileofs);
    if (verts->filelen % sizeof(*dv)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }

    indexes = (int32*) (fileBase + indexLump->fileofs);
    if (indexLump->filelen % sizeof(*indexes)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }

    out = (msurface_t*) Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.surfaces = out;
    s_worldData.numsurfaces = count;

    for (i = 0; i < count; i++, in++, out++) {
        switch ((in->surfaceType)) {
        case MST_PATCH:
            ParseMesh(in, dv, out);
            numMeshes++;
            break;
        case MST_TRIANGLE_SOUP:
            ParseTriSurf(in, dv, out, indexes);
            numTriSurfs++;
            break;
        case MST_PLANAR:
            ParseFace(in, dv, out, indexes);
            numFaces++;
            break;
        case MST_FLARE:
            ParseFlare(in, dv, out, indexes);
            numFlares++;
            break;
        default:
            Com_Error(ERR_DROP, "Bad surfaceType");
        }
    }

    R_StitchAllPatches();
    R_FixSharedVertexLodError();
    R_MovePatchSurfacesToHunk();

    logger.info("...loaded %d faces, %i meshes, %i trisurfs, %i flares", numFaces, numMeshes, numTriSurfs, numFlares);
}


/*
=================
R_LoadSubmodels
=================
*/
static void R_LoadSubmodels(lump_t* l)
{
    dmodel_t* in;
    bmodel_t* out;
    int32     i, j, count;

    in = (dmodel_t*) (fileBase + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = l->filelen / sizeof(*in);

    s_worldData.bmodels = out = (bmodel_t*) Hunk_Alloc(count * sizeof(*out), h_low);

    for (i = 0; i < count; i++, in++, out++) {
        model_t* model;

        model = R_AllocModel();

        assert(model != NULL); // this should never happen

        model->type = MOD_BRUSH;
        model->bmodel = out;
        Com_sprintf(model->name, sizeof(model->name), "*%d", i);

        for (j = 0; j < 3; j++) {
            out->bounds[0][j] = (in->mins[j]);
            out->bounds[1][j] = (in->maxs[j]);
        }

        out->firstSurface = s_worldData.surfaces + (in->firstSurface);
        out->numSurfaces = (in->numSurfaces);
    }
}


//==================================================================

/*
=================
R_SetParent
=================
*/
static void R_SetParent(mnode_t* node, mnode_t* parent)
{
    node->parent = parent;
    if (node->contents != -1) {
        return;
    }
    R_SetParent(node->children[0], node);
    R_SetParent(node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs(lump_t* nodeLump, lump_t* leafLump)
{
    int32    i, j, p;
    dnode_t* in;
    dleaf_t* inLeaf;
    mnode_t* out;
    int32    numNodes, numLeafs;

    in = (dnode_t*) (fileBase + nodeLump->fileofs);
    if (nodeLump->filelen % sizeof(dnode_t) || leafLump->filelen % sizeof(dleaf_t)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    numNodes = nodeLump->filelen / sizeof(dnode_t);
    numLeafs = leafLump->filelen / sizeof(dleaf_t);

    out = (mnode_t*) Hunk_Alloc((numNodes + numLeafs) * sizeof(*out), h_low);

    s_worldData.nodes = out;
    s_worldData.numnodes = numNodes + numLeafs;
    s_worldData.numDecisionNodes = numNodes;

    // load nodes
    for (i = 0; i < numNodes; i++, in++, out++) {
        for (j = 0; j < 3; j++) {
            out->mins[j] = (in->mins[j]);
            out->maxs[j] = (in->maxs[j]);
        }

        p = (in->planeNum);
        out->plane = s_worldData.planes + p;

        out->contents = CONTENTS_NODE; // differentiate from leafs

        for (j = 0; j < 2; j++) {
            p = (in->children[j]);
            if (p >= 0) {
                out->children[j] = s_worldData.nodes + p;
            } else {
                out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
            }
        }
    }

    // load leafs
    inLeaf = (dleaf_t*) (fileBase + leafLump->fileofs);
    for (i = 0; i < numLeafs; i++, inLeaf++, out++) {
        for (j = 0; j < 3; j++) {
            out->mins[j] = (inLeaf->mins[j]);
            out->maxs[j] = (inLeaf->maxs[j]);
        }

        out->cluster = (inLeaf->cluster);
        out->area = (inLeaf->area);

        if (out->cluster >= s_worldData.numClusters) {
            s_worldData.numClusters = out->cluster + 1;
        }

        out->firstmarksurface = s_worldData.marksurfaces + (inLeaf->firstLeafSurface);
        out->nummarksurfaces = (inLeaf->numLeafSurfaces);
    }

    // chain decendants
    R_SetParent(s_worldData.nodes, NULL);
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders(lump_t* l)
{
    int32      i, count;
    dshader_t *in, *out;

    in = (dshader_t*) (fileBase + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = l->filelen / sizeof(*in);
    out = (dshader_t*) Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.shaders = out;
    s_worldData.numShaders = count;

    Com_Memcpy(out, in, count * sizeof(*out));

    for (i = 0; i < count; i++) {
        out[i].surfaceFlags = (out[i].surfaceFlags);
        out[i].contentFlags = (out[i].contentFlags);
    }
}


/*
=================
R_LoadMarksurfaces
=================
*/
static void R_LoadMarksurfaces(lump_t* l)
{
    int32        i, j, count;
    int32*       in;
    msurface_t** out;

    in = (int32*) (fileBase + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = l->filelen / sizeof(*in);
    out = (msurface_t**) Hunk_Alloc(count * sizeof(*out), h_low);

    s_worldData.marksurfaces = out;
    s_worldData.nummarksurfaces = count;

    for (i = 0; i < count; i++) {
        j = (in[i]);
        out[i] = s_worldData.surfaces + j;
    }
}


/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes(lump_t* l)
{
    int32     i, j;
    cplane_t* out;
    dplane_t* in;
    int32     count;
    int32     bits;

    in = (dplane_t*) (fileBase + l->fileofs);
    if (l->filelen % sizeof(*in)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = l->filelen / sizeof(*in);
    out = (cplane_t*) Hunk_Alloc(count * 2 * sizeof(*out), h_low);

    s_worldData.planes = out;
    s_worldData.numplanes = count;

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
R_LoadFogs

=================
*/
static void R_LoadFogs(lump_t* l, lump_t* brushesLump, lump_t* sidesLump)
{
    int32         i;
    fog_t*        out;
    dfog_t*       fogs;
    dbrush_t *    brushes, *brush;
    dbrushside_t* sides;
    int32         count, brushesCount, sidesCount;
    int32         sideNum;
    int32         planeNum;
    shader_t*     shader;
    float         d;
    int32         firstSide;

    fogs = (dfog_t*) (fileBase + l->fileofs);
    if (l->filelen % sizeof(*fogs)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    count = l->filelen / sizeof(*fogs);

    // create fog strucutres for them
    s_worldData.numfogs = count + 1;
    s_worldData.fogs = (fog_t*) Hunk_Alloc(s_worldData.numfogs * sizeof(*out), h_low);
    out = s_worldData.fogs + 1;

    if (!count) {
        return;
    }

    brushes = (dbrush_t*) (fileBase + brushesLump->fileofs);
    if (brushesLump->filelen % sizeof(*brushes)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    brushesCount = brushesLump->filelen / sizeof(*brushes);

    sides = (dbrushside_t*) (fileBase + sidesLump->fileofs);
    if (sidesLump->filelen % sizeof(*sides)) {
        Com_Error(ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name);
    }
    sidesCount = sidesLump->filelen / sizeof(*sides);

    for (i = 0; i < count; i++, fogs++) {
        out->originalBrushNumber = (fogs->brushNum);

        if ((uint32) out->originalBrushNumber >= brushesCount) {
            Com_Error(ERR_DROP, "fog brushNumber out of range");
        }
        brush = brushes + out->originalBrushNumber;

        firstSide = (brush->firstSide);

        if ((uint32) firstSide > sidesCount - 6) {
            Com_Error(ERR_DROP, "fog brush sideNumber out of range");
        }

        // brushes are always sorted with the axial sides first
        sideNum = firstSide + 0;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[0][0] = -s_worldData.planes[planeNum].dist;

        sideNum = firstSide + 1;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[1][0] = s_worldData.planes[planeNum].dist;

        sideNum = firstSide + 2;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[0][1] = -s_worldData.planes[planeNum].dist;

        sideNum = firstSide + 3;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[1][1] = s_worldData.planes[planeNum].dist;

        sideNum = firstSide + 4;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[0][2] = -s_worldData.planes[planeNum].dist;

        sideNum = firstSide + 5;
        planeNum = (sides[sideNum].planeNum);
        out->bounds[1][2] = s_worldData.planes[planeNum].dist;

        // get information from the shader for fog parameters
        shader = R_FindShader(fogs->shader, LIGHTMAP_NONE, true);

        out->parms = shader->fogParms;

        out->colorInt = ColorBytes4(shader->fogParms.color[0], shader->fogParms.color[1], shader->fogParms.color[2], 1.0);

        d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
        out->tcScale = 1.0f / (d * 8);

        // set the gradient vector
        sideNum = (fogs->visibleSide);

        if (sideNum == -1) {
            out->hasSurface = false;
        } else {
            out->hasSurface = true;
            planeNum = (sides[firstSide + sideNum].planeNum);
            VectorSubtract(vec3_origin, s_worldData.planes[planeNum].normal, out->surface);
            out->surface[3] = -s_worldData.planes[planeNum].dist;
        }

        out++;
    }
}


/*
================
R_LoadLightGrid

================
*/
void R_LoadLightGrid(lump_t* l)
{
    int32    i;
    vec3_t   maxs;
    int32    numGridPoints;
    world_t* w;
    float *  wMins, *wMaxs;

    w = &s_worldData;

    w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
    w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
    w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

    wMins = w->bmodels[0].bounds[0];
    wMaxs = w->bmodels[0].bounds[1];

    for (i = 0; i < 3; i++) {
        w->lightGridOrigin[i] = w->lightGridSize[i] * ceil(wMins[i] / w->lightGridSize[i]);
        maxs[i] = w->lightGridSize[i] * floor(wMaxs[i] / w->lightGridSize[i]);
        w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i]) / w->lightGridSize[i] + 1;
    }

    numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

    if (l->filelen != numGridPoints * 8) {
        logger.warning("light grid mismatch");
        w->lightGridData = NULL;
        return;
    }

    w->lightGridData = (uint8*) Hunk_Alloc(l->filelen, h_low);
    Com_Memcpy(w->lightGridData, (void*) (fileBase + l->fileofs), l->filelen);

    // deal with overbright bits
    for (i = 0; i < numGridPoints; i++) {
        R_ColorShiftLightingBytes(&w->lightGridData[i * 8], &w->lightGridData[i * 8]);
        R_ColorShiftLightingBytes(&w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3]);
    }
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities(lump_t* l)
{
    char *   p, *token, *s;
    char     keyname[MAX_TOKEN_CHARS];
    char     value[MAX_TOKEN_CHARS];
    world_t* w;

    w = &s_worldData;
    w->lightGridSize[0] = 64;
    w->lightGridSize[1] = 64;
    w->lightGridSize[2] = 128;

    p = (char*) (fileBase + l->fileofs);

    // store for reference by the cgame
    w->entityString = (char*) Hunk_Alloc(l->filelen + 1, h_low);
    strcpy(w->entityString, p);
    w->entityParsePoint = w->entityString;

    token = COM_ParseExt(&p, true);
    if (!*token || *token != '{') {
        return;
    }

    // only parse the world spawn
    while (1) {
        // parse key
        token = COM_ParseExt(&p, true);

        if (!*token || *token == '}') {
            break;
        }
        Q_strncpyz(keyname, token, sizeof(keyname));

        // parse value
        token = COM_ParseExt(&p, true);

        if (!*token || *token == '}') {
            break;
        }
        Q_strncpyz(value, token, sizeof(value));

        // check for remapping of shaders for vertex lighting
        s = (char*) "vertexremapshader";
        if (!Q_strncmp(keyname, s, strlen(s))) {
            s = strchr(value, ';');
            if (!s) {
                logger.warning("No semi colon in vertexshaderremap '%s'", value);
                break;
            }
            *s++ = 0;
            continue;
        }
        // check for remapping of shaders
        s = (char*) "remapshader";
        if (!Q_strncmp(keyname, s, strlen(s))) {
            s = strchr(value, ';');
            if (!s) {
                logger.warning("No semi colon in shaderremap '%s'", value);
                break;
            }
            *s++ = 0;
            R_RemapShader(value, s, "0");
            continue;
        }
        // check for a different grid size
        if (!Q_stricmp(keyname, "gridsize")) {
            sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2]);
            continue;
        }
    }
}

/*
=================
R_GetEntityToken
=================
*/
bool R_GetEntityToken(char* buffer, int32 size)
{
    const char* s;

    s = COM_Parse(&s_worldData.entityParsePoint);
    Q_strncpyz(buffer, s, size);
    if (!s_worldData.entityParsePoint || !s[0]) {
        s_worldData.entityParsePoint = s_worldData.entityString;
        return false;
    } else {
        return true;
    }
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap(const char* name)
{
    int32      i;
    dheader_t* header;
    uint8*     buffer;
    uint8*     startMarker;

    if (tr.worldMapLoaded) {
        Com_Error(ERR_DROP, "ERROR: attempted to redundantly load world map\n");
    }

    // set default sun direction to be used if it isn't
    // overridden by a shader
    tr.sunDirection[0] = 0.45f;
    tr.sunDirection[1] = 0.3f;
    tr.sunDirection[2] = 0.9f;

    VectorNormalize(tr.sunDirection);

    tr.worldMapLoaded = true;

    // load it
    FS_ReadFile(name, (void**) &buffer);
    if (!buffer) {
        Com_Error(ERR_DROP, "RE_LoadWorldMap: %s not found", name);
    }

    // clear tr.world so if the level fails to load, the next
    // try will not look at the partially loaded version
    tr.world = NULL;

    Com_Memset(&s_worldData, 0, sizeof(s_worldData));
    Q_strncpyz(s_worldData.name, name, sizeof(s_worldData.name));

    Q_strncpyz(s_worldData.baseName, COM_SkipPath(s_worldData.name), sizeof(s_worldData.name));
    COM_StripExtension(s_worldData.baseName, s_worldData.baseName);

    startMarker = (uint8*) Hunk_Alloc(0, h_low);
    c_gridVerts = 0;

    header = (dheader_t*) buffer;
    fileBase = (uint8*) header;

    i = (header->version);
    if (i != BSP_VERSION) {
        Com_Error(ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", name, i, BSP_VERSION);
    }

    // load into heap
    R_LoadShaders(&header->lumps[LUMP_SHADERS]);
    R_LoadLightmaps(&header->lumps[LUMP_LIGHTMAPS]);
    R_LoadPlanes(&header->lumps[LUMP_PLANES]);
    R_LoadFogs(&header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES]);
    R_LoadSurfaces(&header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES]);
    R_LoadMarksurfaces(&header->lumps[LUMP_LEAFSURFACES]);
    R_LoadNodesAndLeafs(&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
    R_LoadSubmodels(&header->lumps[LUMP_MODELS]);
    R_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
    R_LoadEntities(&header->lumps[LUMP_ENTITIES]);
    R_LoadLightGrid(&header->lumps[LUMP_LIGHTGRID]);

    s_worldData.dataSize = (uint8*) Hunk_Alloc(0, h_low) - startMarker;

    // only set tr.world now that we know the entire level has loaded properly
    tr.world = &s_worldData;

    FS_FreeFile(buffer);
}

