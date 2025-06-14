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

#include "cm_local.h"
#include "cm_patch.h"

static tavros::core::logger logger("cm_patch");

int32 c_totalPatchBlocks;

/*
=================
CM_ClearLevelPatches
=================
*/
void CM_ClearLevelPatches()
{
}

/*
=================
CM_SignbitsForNormal
=================
*/
static int32 CM_SignbitsForNormal(vec3_t normal)
{
    int32 bits, j;

    bits = 0;
    for (j = 0; j < 3; j++) {
        if (normal[j] < 0) {
            bits |= 1 << j;
        }
    }
    return bits;
}

/*
=====================
CM_PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
static bool CM_PlaneFromPoints(float* plane /* as vec4 */, vec3_t a, vec3_t b, vec3_t c)
{
    vec3_t d1, d2;

    VectorSubtract(b, a, d1);
    VectorSubtract(c, a, d2);
    CrossProduct(d2, d1, plane);
    if (VectorNormalize(plane) == 0) {
        return false;
    }

    plane[3] = DotProduct(a, plane);
    return true;
}


/*
================================================================================

GRID SUBDIVISION

================================================================================
*/

/*
=================
CM_NeedsSubdivision

Returns true if the given quadratic curve is not flat enough for our
collision detection purposes
=================
*/
static bool CM_NeedsSubdivision(vec3_t a, vec3_t b, vec3_t c)
{
    vec3_t cmid;
    vec3_t lmid;
    vec3_t delta;
    float  dist;
    int32  i;

    // calculate the linear midpoint
    for (i = 0; i < 3; i++) {
        lmid[i] = 0.5 * (a[i] + c[i]);
    }

    // calculate the exact curve midpoint
    for (i = 0; i < 3; i++) {
        cmid[i] = 0.5 * (0.5 * (a[i] + b[i]) + 0.5 * (b[i] + c[i]));
    }

    // see if the curve is far enough away from the linear mid
    VectorSubtract(cmid, lmid, delta);
    dist = VectorLength(delta);

    return dist >= SUBDIVIDE_DISTANCE;
}

/*
===============
CM_Subdivide

a, b, and c are control points.
the subdivided sequence will be: a, out1, out2, out3, c
===============
*/
static void CM_Subdivide(vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3)
{
    int32 i;

    for (i = 0; i < 3; i++) {
        out1[i] = 0.5 * (a[i] + b[i]);
        out3[i] = 0.5 * (b[i] + c[i]);
        out2[i] = 0.5 * (out1[i] + out3[i]);
    }
}

/*
=================
CM_TransposeGrid

Swaps the rows and columns in place
=================
*/
static void CM_TransposeGrid(cGrid_t* grid)
{
    int32  i, j, l;
    vec3_t temp;
    bool   tempWrap;

    if (grid->width > grid->height) {
        for (i = 0; i < grid->height; i++) {
            for (j = i + 1; j < grid->width; j++) {
                if (j < grid->height) {
                    // swap the value
                    VectorCopy(grid->points[i][j], temp);
                    VectorCopy(grid->points[j][i], grid->points[i][j]);
                    VectorCopy(temp, grid->points[j][i]);
                } else {
                    // just copy
                    VectorCopy(grid->points[j][i], grid->points[i][j]);
                }
            }
        }
    } else {
        for (i = 0; i < grid->width; i++) {
            for (j = i + 1; j < grid->height; j++) {
                if (j < grid->width) {
                    // swap the value
                    VectorCopy(grid->points[j][i], temp);
                    VectorCopy(grid->points[i][j], grid->points[j][i]);
                    VectorCopy(temp, grid->points[i][j]);
                } else {
                    // just copy
                    VectorCopy(grid->points[i][j], grid->points[j][i]);
                }
            }
        }
    }

    l = grid->width;
    grid->width = grid->height;
    grid->height = l;

    tempWrap = grid->wrapWidth;
    grid->wrapWidth = grid->wrapHeight;
    grid->wrapHeight = tempWrap;
}

/*
===================
CM_SetGridWrapWidth

If the left and right columns are exactly equal, set grid->wrapWidth true
===================
*/
static void CM_SetGridWrapWidth(cGrid_t* grid)
{
    int32 i, j;
    float d;

    for (i = 0; i < grid->height; i++) {
        for (j = 0; j < 3; j++) {
            d = grid->points[0][i][j] - grid->points[grid->width - 1][i][j];
            if (d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON) {
                break;
            }
        }
        if (j != 3) {
            break;
        }
    }
    if (i == grid->height) {
        grid->wrapWidth = true;
    } else {
        grid->wrapWidth = false;
    }
}

/*
=================
CM_SubdivideGridColumns

Adds columns as necessary to the grid until
all the aproximating points are within SUBDIVIDE_DISTANCE
from the true curve
=================
*/
static void CM_SubdivideGridColumns(cGrid_t* grid)
{
    int32 i, j, k;

    for (i = 0; i < grid->width - 2;) {
        // grid->points[i][x] is an interpolating control point
        // grid->points[i+1][x] is an aproximating control point
        // grid->points[i+2][x] is an interpolating control point

        //
        // first see if we can collapse the aproximating collumn away
        //
        for (j = 0; j < grid->height; j++) {
            if (CM_NeedsSubdivision(grid->points[i][j], grid->points[i + 1][j], grid->points[i + 2][j])) {
                break;
            }
        }
        if (j == grid->height) {
            // all of the points were close enough to the linear midpoints
            // that we can collapse the entire column away
            for (j = 0; j < grid->height; j++) {
                // remove the column
                for (k = i + 2; k < grid->width; k++) {
                    VectorCopy(grid->points[k][j], grid->points[k - 1][j]);
                }
            }

            grid->width--;

            // go to the next curve segment
            i++;
            continue;
        }

        //
        // we need to subdivide the curve
        //
        for (j = 0; j < grid->height; j++) {
            vec3_t prev, mid, next;

            // save the control points now
            VectorCopy(grid->points[i][j], prev);
            VectorCopy(grid->points[i + 1][j], mid);
            VectorCopy(grid->points[i + 2][j], next);

            // make room for two additional columns in the grid
            // columns i+1 will be replaced, column i+2 will become i+4
            // i+1, i+2, and i+3 will be generated
            for (k = grid->width - 1; k > i + 1; k--) {
                VectorCopy(grid->points[k][j], grid->points[k + 2][j]);
            }

            // generate the subdivided points
            CM_Subdivide(prev, mid, next, grid->points[i + 1][j], grid->points[i + 2][j], grid->points[i + 3][j]);
        }

        grid->width += 2;

        // the new aproximating point at i+1 may need to be removed
        // or subdivided farther, so don't advance i
    }
}

/*
======================
CM_ComparePoints
======================
*/
#define POINT_EPSILON 0.1
static bool CM_ComparePoints(float* a, float* b)
{
    float d;

    d = a[0] - b[0];
    if (d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }
    d = a[1] - b[1];
    if (d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }
    d = a[2] - b[2];
    if (d < -POINT_EPSILON || d > POINT_EPSILON) {
        return false;
    }
    return true;
}

/*
=================
CM_RemoveDegenerateColumns

If there are any identical columns, remove them
=================
*/
static void CM_RemoveDegenerateColumns(cGrid_t* grid)
{
    int32 i, j, k;

    for (i = 0; i < grid->width - 1; i++) {
        for (j = 0; j < grid->height; j++) {
            if (!CM_ComparePoints(grid->points[i][j], grid->points[i + 1][j])) {
                break;
            }
        }

        if (j != grid->height) {
            continue; // not degenerate
        }

        for (j = 0; j < grid->height; j++) {
            // remove the column
            for (k = i + 2; k < grid->width; k++) {
                VectorCopy(grid->points[k][j], grid->points[k - 1][j]);
            }
        }
        grid->width--;

        // check against the next column
        i--;
    }
}

/*
================================================================================

PATCH COLLIDE GENERATION

================================================================================
*/

static int32        numPlanes;
static patchPlane_t planes[MAX_PATCH_PLANES];

static int32   numFacets;
static facet_t facets[MAX_PATCH_PLANES]; // maybe MAX_FACETS ??

#define NORMAL_EPSILON 0.0001
#define DIST_EPSILON   0.02

/*
==================
CM_PlaneEqual
==================
*/
int32 CM_PlaneEqual(patchPlane_t* p, float plane[4], int32* flipped)
{
    float invplane[4];

    if (
        fabs(p->plane[0] - plane[0]) < NORMAL_EPSILON
        && fabs(p->plane[1] - plane[1]) < NORMAL_EPSILON
        && fabs(p->plane[2] - plane[2]) < NORMAL_EPSILON
        && fabs(p->plane[3] - plane[3]) < DIST_EPSILON
    ) {
        *flipped = false;
        return true;
    }

    VectorNegate(plane, invplane);
    invplane[3] = -plane[3];

    if (
        fabs(p->plane[0] - invplane[0]) < NORMAL_EPSILON
        && fabs(p->plane[1] - invplane[1]) < NORMAL_EPSILON
        && fabs(p->plane[2] - invplane[2]) < NORMAL_EPSILON
        && fabs(p->plane[3] - invplane[3]) < DIST_EPSILON
    ) {
        *flipped = true;
        return true;
    }

    return false;
}

/*
==================
CM_SnapVector
==================
*/
void CM_SnapVector(vec3_t normal)
{
    int32 i;

    for (i = 0; i < 3; i++) {
        if (fabs(normal[i] - 1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = 1;
            break;
        }
        if (fabs(normal[i] - -1) < NORMAL_EPSILON) {
            VectorClear(normal);
            normal[i] = -1;
            break;
        }
    }
}

/*
==================
CM_FindPlane2
==================
*/
int32 CM_FindPlane2(float plane[4], int32* flipped)
{
    int32 i;

    // see if the points are close enough to an existing plane
    for (i = 0; i < numPlanes; i++) {
        if (CM_PlaneEqual(&planes[i], plane, flipped)) {
            return i;
        }
    }

    // add a new plane
    if (numPlanes == MAX_PATCH_PLANES) {
        Com_Error(ERR_DROP, "MAX_PATCH_PLANES");
    }

    Vector4Copy(plane, planes[numPlanes].plane);
    planes[numPlanes].signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    *flipped = false;

    return numPlanes - 1;
}

/*
==================
CM_FindPlane
==================
*/
static int32 CM_FindPlane(float* p1, float* p2, float* p3)
{
    float plane[4];
    int32 i;
    float d;

    if (!CM_PlaneFromPoints(plane, p1, p2, p3)) {
        return -1;
    }

    // see if the points are close enough to an existing plane
    for (i = 0; i < numPlanes; i++) {
        if (DotProduct(plane, planes[i].plane) < 0) {
            continue; // allow backwards planes?
        }

        d = DotProduct(p1, planes[i].plane) - planes[i].plane[3];
        if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        d = DotProduct(p2, planes[i].plane) - planes[i].plane[3];
        if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        d = DotProduct(p3, planes[i].plane) - planes[i].plane[3];
        if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON) {
            continue;
        }

        // found it
        return i;
    }

    // add a new plane
    if (numPlanes == MAX_PATCH_PLANES) {
        Com_Error(ERR_DROP, "MAX_PATCH_PLANES");
    }

    Vector4Copy(plane, planes[numPlanes].plane);
    planes[numPlanes].signbits = CM_SignbitsForNormal(plane);

    numPlanes++;

    return numPlanes - 1;
}

/*
==================
CM_PointOnPlaneSide
==================
*/
static int32 CM_PointOnPlaneSide(float* p, int32 planeNum)
{
    float* plane;
    float  d;

    if (planeNum == -1) {
        return SIDE_ON;
    }
    plane = planes[planeNum].plane;

    d = DotProduct(p, plane) - plane[3];

    if (d > PLANE_TRI_EPSILON) {
        return SIDE_FRONT;
    }

    if (d < -PLANE_TRI_EPSILON) {
        return SIDE_BACK;
    }

    return SIDE_ON;
}

/*
==================
CM_GridPlane
==================
*/
static int32 CM_GridPlane(int32 gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int32 i, int32 j, int32 tri)
{
    int32 p;

    p = gridPlanes[i][j][tri];
    if (p != -1) {
        return p;
    }
    p = gridPlanes[i][j][!tri];
    if (p != -1) {
        return p;
    }

    // should never happen
    logger.warning("CM_GridPlane unresolvable");
    return -1;
}

/*
==================
CM_EdgePlaneNum
==================
*/
static int32 CM_EdgePlaneNum(cGrid_t* grid, int32 gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int32 i, int32 j, int32 k)
{
    float *p1, *p2;
    vec3_t up;
    int32  p;

    switch (k) {
    case 0: // top border
        p1 = grid->points[i][j];
        p2 = grid->points[i + 1][j];
        p = CM_GridPlane(gridPlanes, i, j, 0);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p1, p2, up);

    case 2: // bottom border
        p1 = grid->points[i][j + 1];
        p2 = grid->points[i + 1][j + 1];
        p = CM_GridPlane(gridPlanes, i, j, 1);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p2, p1, up);

    case 3: // left border
        p1 = grid->points[i][j];
        p2 = grid->points[i][j + 1];
        p = CM_GridPlane(gridPlanes, i, j, 1);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p2, p1, up);

    case 1: // right border
        p1 = grid->points[i + 1][j];
        p2 = grid->points[i + 1][j + 1];
        p = CM_GridPlane(gridPlanes, i, j, 0);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p1, p2, up);

    case 4: // diagonal out of triangle 0
        p1 = grid->points[i + 1][j + 1];
        p2 = grid->points[i][j];
        p = CM_GridPlane(gridPlanes, i, j, 0);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p1, p2, up);

    case 5: // diagonal out of triangle 1
        p1 = grid->points[i][j];
        p2 = grid->points[i + 1][j + 1];
        p = CM_GridPlane(gridPlanes, i, j, 1);
        VectorMA(p1, 4, planes[p].plane, up);
        return CM_FindPlane(p1, p2, up);
    }

    Com_Error(ERR_DROP, "CM_EdgePlaneNum: bad k");
    return -1;
}

/*
===================
CM_SetBorderInward
===================
*/
static void CM_SetBorderInward(facet_t* facet, cGrid_t* grid, int32 gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int32 i, int32 j, int32 which)
{
    int32  k, l;
    float* points[4];
    int32  numPoints;

    switch (which) {
    case -1:
        points[0] = grid->points[i][j];
        points[1] = grid->points[i + 1][j];
        points[2] = grid->points[i + 1][j + 1];
        points[3] = grid->points[i][j + 1];
        numPoints = 4;
        break;
    case 0:
        points[0] = grid->points[i][j];
        points[1] = grid->points[i + 1][j];
        points[2] = grid->points[i + 1][j + 1];
        numPoints = 3;
        break;
    case 1:
        points[0] = grid->points[i + 1][j + 1];
        points[1] = grid->points[i][j + 1];
        points[2] = grid->points[i][j];
        numPoints = 3;
        break;
    default:
        Com_Error(ERR_FATAL, "CM_SetBorderInward: bad parameter");
        numPoints = 0;
        break;
    }

    for (k = 0; k < facet->numBorders; k++) {
        int32 front, back;

        front = 0;
        back = 0;

        for (l = 0; l < numPoints; l++) {
            int32 side;

            side = CM_PointOnPlaneSide(points[l], facet->borderPlanes[k]);
            if (side == SIDE_FRONT) {
                front++;
            }
            if (side == SIDE_BACK) {
                back++;
            }
        }

        if (front && !back) {
            facet->borderInward[k] = true;
        } else if (back && !front) {
            facet->borderInward[k] = false;
        } else if (!front && !back) {
            // flat side border
            facet->borderPlanes[k] = -1;
        } else {
            // bisecting side border
            logger.debug("WARNING: CM_SetBorderInward: mixed plane sides");
            facet->borderInward[k] = false;
        }
    }
}

/*
==================
CM_ValidateFacet

If the facet isn't bounded by its borders, we screwed up.
==================
*/
static bool CM_ValidateFacet(facet_t* facet)
{
    float      plane[4];
    int32      j;
    winding_t* w;
    vec3_t     bounds[2];

    if (facet->surfacePlane == -1) {
        return false;
    }

    Vector4Copy(planes[facet->surfacePlane].plane, plane);
    w = BaseWindingForPlane(plane, plane[3]);
    for (j = 0; j < facet->numBorders && w; j++) {
        if (facet->borderPlanes[j] == -1) {
            return false;
        }
        Vector4Copy(planes[facet->borderPlanes[j]].plane, plane);
        if (!facet->borderInward[j]) {
            VectorSubtract(vec3_origin, plane, plane);
            plane[3] = -plane[3];
        }
        ChopWindingInPlace(&w, plane, plane[3], 0.1f);
    }

    if (!w) {
        return false; // winding was completely chopped away
    }

    // see if the facet is unreasonably large
    WindingBounds(w, bounds[0], bounds[1]);
    FreeWinding(w);

    for (j = 0; j < 3; j++) {
        if (bounds[1][j] - bounds[0][j] > MAX_MAP_BOUNDS) {
            return false; // we must be missing a plane
        }
        if (bounds[0][j] >= MAX_MAP_BOUNDS) {
            return false;
        }
        if (bounds[1][j] <= -MAX_MAP_BOUNDS) {
            return false;
        }
    }
    return true; // winding is fine
}

/*
==================
CM_AddFacetBevels
==================
*/
void CM_AddFacetBevels(facet_t* facet)
{
    int32      i, j, k, l;
    int32      axis, dir, order, flipped;
    float      plane[4], d, newplane[4];
    winding_t *w, *w2;
    vec3_t     mins, maxs, vec, vec2;

    Vector4Copy(planes[facet->surfacePlane].plane, plane);

    w = BaseWindingForPlane(plane, plane[3]);
    for (j = 0; j < facet->numBorders && w; j++) {
        if (facet->borderPlanes[j] == facet->surfacePlane) {
            continue;
        }
        Vector4Copy(planes[facet->borderPlanes[j]].plane, plane);

        if (!facet->borderInward[j]) {
            VectorSubtract(vec3_origin, plane, plane);
            plane[3] = -plane[3];
        }

        ChopWindingInPlace(&w, plane, plane[3], 0.1f);
    }
    if (!w) {
        return;
    }

    WindingBounds(w, mins, maxs);

    // add the axial planes
    order = 0;
    for (axis = 0; axis < 3; axis++) {
        for (dir = -1; dir <= 1; dir += 2, order++) {
            VectorClear(plane);
            plane[axis] = dir;
            if (dir == 1) {
                plane[3] = maxs[axis];
            } else {
                plane[3] = -mins[axis];
            }
            // if it's the surface plane
            if (CM_PlaneEqual(&planes[facet->surfacePlane], plane, &flipped)) {
                continue;
            }
            // see if the plane is allready present
            for (i = 0; i < facet->numBorders; i++) {
                if (CM_PlaneEqual(&planes[facet->borderPlanes[i]], plane, &flipped)) {
                    break;
                }
            }

            if (i == facet->numBorders) {
                if (facet->numBorders > 4 + 6 + 16) {
                    logger.error("Too many bevels");
                }
                facet->borderPlanes[facet->numBorders] = CM_FindPlane2(plane, &flipped);
                facet->borderNoAdjust[facet->numBorders] = 0;
                facet->borderInward[facet->numBorders] = flipped;
                facet->numBorders++;
            }
        }
    }
    //
    // add the edge bevels
    //
    // test the non-axial plane edges
    for (j = 0; j < w->numpoints; j++) {
        k = (j + 1) % w->numpoints;
        VectorSubtract(w->p[j], w->p[k], vec);
        // if it's a degenerate edge
        if (VectorNormalize(vec) < 0.5) {
            continue;
        }
        CM_SnapVector(vec);
        for (k = 0; k < 3; k++) {
            if (vec[k] == -1 || vec[k] == 1) {
                break; // axial
            }
        }
        if (k < 3) {
            continue; // only test non-axial edges
        }

        // try the six possible slanted axials from this edge
        for (axis = 0; axis < 3; axis++) {
            for (dir = -1; dir <= 1; dir += 2) {
                // construct a plane
                VectorClear(vec2);
                vec2[axis] = dir;
                CrossProduct(vec, vec2, plane);
                if (VectorNormalize(plane) < 0.5) {
                    continue;
                }
                plane[3] = DotProduct(w->p[j], plane);

                // if all the points of the facet winding are
                // behind this plane, it is a proper edge bevel
                for (l = 0; l < w->numpoints; l++) {
                    d = DotProduct(w->p[l], plane) - plane[3];
                    if (d > 0.1) {
                        break; // point in front
                    }
                }
                if (l < w->numpoints) {
                    continue;
                }

                // if it's the surface plane
                if (CM_PlaneEqual(&planes[facet->surfacePlane], plane, &flipped)) {
                    continue;
                }
                // see if the plane is allready present
                for (i = 0; i < facet->numBorders; i++) {
                    if (CM_PlaneEqual(&planes[facet->borderPlanes[i]], plane, &flipped)) {
                        break;
                    }
                }

                if (i == facet->numBorders) {
                    if (facet->numBorders > 4 + 6 + 16) {
                        logger.error("Too many bevels");
                    }
                    facet->borderPlanes[facet->numBorders] = CM_FindPlane2(plane, &flipped);

                    for (k = 0; k < facet->numBorders; k++) {
                        if (facet->borderPlanes[facet->numBorders] == facet->borderPlanes[k]) {
                            logger.warning("Bevel plane already used");
                        }
                    }

                    facet->borderNoAdjust[facet->numBorders] = 0;
                    facet->borderInward[facet->numBorders] = flipped;
                    //
                    w2 = CopyWinding(w);
                    Vector4Copy(planes[facet->borderPlanes[facet->numBorders]].plane, newplane);
                    if (!facet->borderInward[facet->numBorders]) {
                        VectorNegate(newplane, newplane);
                        newplane[3] = -newplane[3];
                    }
                    ChopWindingInPlace(&w2, newplane, newplane[3], 0.1f);
                    if (!w2) {
                        logger.debug("WARNING: CM_AddFacetBevels... invalid bevel");
                        continue;
                    } else {
                        FreeWinding(w2);
                    }
                    //
                    facet->numBorders++;
                    // already got a bevel
                    //                    break;
                }
            }
        }
    }
    FreeWinding(w);

    // add opposite plane
    facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
    facet->borderNoAdjust[facet->numBorders] = 0;
    facet->borderInward[facet->numBorders] = true;
    facet->numBorders++;
}

typedef enum
{
    EN_TOP,
    EN_RIGHT,
    EN_BOTTOM,
    EN_LEFT
} edgeName_t;

/*
==================
CM_PatchCollideFromGrid
==================
*/
static void CM_PatchCollideFromGrid(cGrid_t* grid, patchCollide_t* pf)
{
    int32    i, j;
    float *  p1, *p2, *p3;
    int32    gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2];
    facet_t* facet;
    int32    borders[4];
    int32    noAdjust[4];

    numPlanes = 0;
    numFacets = 0;

    // find the planes for each triangle of the grid
    for (i = 0; i < grid->width - 1; i++) {
        for (j = 0; j < grid->height - 1; j++) {
            p1 = grid->points[i][j];
            p2 = grid->points[i + 1][j];
            p3 = grid->points[i + 1][j + 1];
            gridPlanes[i][j][0] = CM_FindPlane(p1, p2, p3);

            p1 = grid->points[i + 1][j + 1];
            p2 = grid->points[i][j + 1];
            p3 = grid->points[i][j];
            gridPlanes[i][j][1] = CM_FindPlane(p1, p2, p3);
        }
    }

    // create the borders for each facet
    for (i = 0; i < grid->width - 1; i++) {
        for (j = 0; j < grid->height - 1; j++) {
            borders[EN_TOP] = -1;
            if (j > 0) {
                borders[EN_TOP] = gridPlanes[i][j - 1][1];
            } else if (grid->wrapHeight) {
                borders[EN_TOP] = gridPlanes[i][grid->height - 2][1];
            }
            noAdjust[EN_TOP] = (borders[EN_TOP] == gridPlanes[i][j][0]);
            if (borders[EN_TOP] == -1 || noAdjust[EN_TOP]) {
                borders[EN_TOP] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 0);
            }

            borders[EN_BOTTOM] = -1;
            if (j < grid->height - 2) {
                borders[EN_BOTTOM] = gridPlanes[i][j + 1][0];
            } else if (grid->wrapHeight) {
                borders[EN_BOTTOM] = gridPlanes[i][0][0];
            }
            noAdjust[EN_BOTTOM] = (borders[EN_BOTTOM] == gridPlanes[i][j][1]);
            if (borders[EN_BOTTOM] == -1 || noAdjust[EN_BOTTOM]) {
                borders[EN_BOTTOM] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 2);
            }

            borders[EN_LEFT] = -1;
            if (i > 0) {
                borders[EN_LEFT] = gridPlanes[i - 1][j][0];
            } else if (grid->wrapWidth) {
                borders[EN_LEFT] = gridPlanes[grid->width - 2][j][0];
            }
            noAdjust[EN_LEFT] = (borders[EN_LEFT] == gridPlanes[i][j][1]);
            if (borders[EN_LEFT] == -1 || noAdjust[EN_LEFT]) {
                borders[EN_LEFT] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 3);
            }

            borders[EN_RIGHT] = -1;
            if (i < grid->width - 2) {
                borders[EN_RIGHT] = gridPlanes[i + 1][j][1];
            } else if (grid->wrapWidth) {
                borders[EN_RIGHT] = gridPlanes[0][j][1];
            }
            noAdjust[EN_RIGHT] = (borders[EN_RIGHT] == gridPlanes[i][j][0]);
            if (borders[EN_RIGHT] == -1 || noAdjust[EN_RIGHT]) {
                borders[EN_RIGHT] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 1);
            }

            if (numFacets == MAX_FACETS) {
                Com_Error(ERR_DROP, "MAX_FACETS");
            }
            facet = &facets[numFacets];
            Com_Memset(facet, 0, sizeof(*facet));

            if (gridPlanes[i][j][0] == gridPlanes[i][j][1]) {
                if (gridPlanes[i][j][0] == -1) {
                    continue; // degenrate
                }
                facet->surfacePlane = gridPlanes[i][j][0];
                facet->numBorders = 4;
                facet->borderPlanes[0] = borders[EN_TOP];
                facet->borderNoAdjust[0] = noAdjust[EN_TOP];
                facet->borderPlanes[1] = borders[EN_RIGHT];
                facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
                facet->borderPlanes[2] = borders[EN_BOTTOM];
                facet->borderNoAdjust[2] = noAdjust[EN_BOTTOM];
                facet->borderPlanes[3] = borders[EN_LEFT];
                facet->borderNoAdjust[3] = noAdjust[EN_LEFT];
                CM_SetBorderInward(facet, grid, gridPlanes, i, j, -1);
                if (CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }
            } else {
                // two seperate triangles
                facet->surfacePlane = gridPlanes[i][j][0];
                facet->numBorders = 3;
                facet->borderPlanes[0] = borders[EN_TOP];
                facet->borderNoAdjust[0] = noAdjust[EN_TOP];
                facet->borderPlanes[1] = borders[EN_RIGHT];
                facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
                facet->borderPlanes[2] = gridPlanes[i][j][1];
                if (facet->borderPlanes[2] == -1) {
                    facet->borderPlanes[2] = borders[EN_BOTTOM];
                    if (facet->borderPlanes[2] == -1) {
                        facet->borderPlanes[2] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 4);
                    }
                }
                CM_SetBorderInward(facet, grid, gridPlanes, i, j, 0);
                if (CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }

                if (numFacets == MAX_FACETS) {
                    Com_Error(ERR_DROP, "MAX_FACETS");
                }
                facet = &facets[numFacets];
                Com_Memset(facet, 0, sizeof(*facet));

                facet->surfacePlane = gridPlanes[i][j][1];
                facet->numBorders = 3;
                facet->borderPlanes[0] = borders[EN_BOTTOM];
                facet->borderNoAdjust[0] = noAdjust[EN_BOTTOM];
                facet->borderPlanes[1] = borders[EN_LEFT];
                facet->borderNoAdjust[1] = noAdjust[EN_LEFT];
                facet->borderPlanes[2] = gridPlanes[i][j][0];
                if (facet->borderPlanes[2] == -1) {
                    facet->borderPlanes[2] = borders[EN_TOP];
                    if (facet->borderPlanes[2] == -1) {
                        facet->borderPlanes[2] = CM_EdgePlaneNum(grid, gridPlanes, i, j, 5);
                    }
                }
                CM_SetBorderInward(facet, grid, gridPlanes, i, j, 1);
                if (CM_ValidateFacet(facet)) {
                    CM_AddFacetBevels(facet);
                    numFacets++;
                }
            }
        }
    }

    // copy the results out
    pf->numPlanes = numPlanes;
    pf->numFacets = numFacets;
    pf->facets = (facet_t*) Hunk_Alloc(numFacets * sizeof(*pf->facets), h_high);
    Com_Memcpy(pf->facets, facets, numFacets * sizeof(*pf->facets));
    pf->planes = (patchPlane_t*) Hunk_Alloc(numPlanes * sizeof(*pf->planes), h_high);
    Com_Memcpy(pf->planes, planes, numPlanes * sizeof(*pf->planes));
}


/*
===================
CM_GeneratePatchCollide

Creates an internal structure that will be used to perform
collision detection with a patch mesh.

Points is packed as concatenated rows.
===================
*/
struct patchCollide_s* CM_GeneratePatchCollide(int32 width, int32 height, vec3_t* points)
{
    patchCollide_t* pf;
    cGrid_t         grid;
    int32           i, j;

    if (width <= 2 || height <= 2 || !points) {
        Com_Error(ERR_DROP, "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)", width, height, points);
    }

    if (!(width & 1) || !(height & 1)) {
        Com_Error(ERR_DROP, "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes");
    }

    if (width > MAX_GRID_SIZE || height > MAX_GRID_SIZE) {
        Com_Error(ERR_DROP, "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE");
    }

    // build a grid
    grid.width = width;
    grid.height = height;
    grid.wrapWidth = false;
    grid.wrapHeight = false;
    for (i = 0; i < width; i++) {
        for (j = 0; j < height; j++) {
            VectorCopy(points[j * width + i], grid.points[i][j]);
        }
    }

    // subdivide the grid
    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);

    CM_TransposeGrid(&grid);

    CM_SetGridWrapWidth(&grid);
    CM_SubdivideGridColumns(&grid);
    CM_RemoveDegenerateColumns(&grid);

    // we now have a grid of points exactly on the curve
    // the aproximate surface defined by these points will be
    // collided against
    pf = (patchCollide_t*) Hunk_Alloc(sizeof(*pf), h_high);
    ClearBounds(pf->bounds[0], pf->bounds[1]);
    for (i = 0; i < grid.width; i++) {
        for (j = 0; j < grid.height; j++) {
            AddPointToBounds(grid.points[i][j], pf->bounds[0], pf->bounds[1]);
        }
    }

    c_totalPatchBlocks += (grid.width - 1) * (grid.height - 1);

    // generate a bsp tree for the surface
    CM_PatchCollideFromGrid(&grid, pf);

    // expand by one unit for epsilon purposes
    pf->bounds[0][0] -= 1;
    pf->bounds[0][1] -= 1;
    pf->bounds[0][2] -= 1;

    pf->bounds[1][0] += 1;
    pf->bounds[1][1] += 1;
    pf->bounds[1][2] += 1;

    return pf;
}

/*
================================================================================

TRACE TESTING

================================================================================
*/

/*
====================
CM_TracePointThroughPatchCollide

  special case for point traces because the patch collide "brushes" have no volume
====================
*/
void CM_TracePointThroughPatchCollide(traceWork_t* tw, const struct patchCollide_s* pc)
{
    bool                frontFacing[MAX_PATCH_PLANES];
    float               intersection[MAX_PATCH_PLANES];
    float               intersect;
    const patchPlane_t* planes;
    const facet_t*      facet;
    int32               i, j, k;
    float               offset;
    float               d1, d2;

    if (!cm_playerCurveClip->integer || !tw->isPoint) {
        return;
    }

    // determine the trace's relationship to all planes
    planes = pc->planes;
    for (i = 0; i < pc->numPlanes; i++, planes++) {
        offset = DotProduct(tw->offsets[planes->signbits], planes->plane);
        d1 = DotProduct(tw->start, planes->plane) - planes->plane[3] + offset;
        d2 = DotProduct(tw->end, planes->plane) - planes->plane[3] + offset;
        if (d1 <= 0) {
            frontFacing[i] = false;
        } else {
            frontFacing[i] = true;
        }
        if (d1 == d2) {
            intersection[i] = 99999;
        } else {
            intersection[i] = d1 / (d1 - d2);
            if (intersection[i] <= 0) {
                intersection[i] = 99999;
            }
        }
    }


    // see if any of the surface planes are intersected
    facet = pc->facets;
    for (i = 0; i < pc->numFacets; i++, facet++) {
        if (!frontFacing[facet->surfacePlane]) {
            continue;
        }
        intersect = intersection[facet->surfacePlane];
        if (intersect < 0) {
            continue; // surface is behind the starting point
        }
        if (intersect > tw->trace.fraction) {
            continue; // already hit something closer
        }
        for (j = 0; j < facet->numBorders; j++) {
            k = facet->borderPlanes[j];
            if (frontFacing[k] ^ facet->borderInward[j]) {
                if (intersection[k] > intersect) {
                    break;
                }
            } else {
                if (intersection[k] < intersect) {
                    break;
                }
            }
        }
        if (j == facet->numBorders) {
            planes = &pc->planes[facet->surfacePlane];

            // calculate intersection with a slight pushoff
            offset = DotProduct(tw->offsets[planes->signbits], planes->plane);
            d1 = DotProduct(tw->start, planes->plane) - planes->plane[3] + offset;
            d2 = DotProduct(tw->end, planes->plane) - planes->plane[3] + offset;
            tw->trace.fraction = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);

            if (tw->trace.fraction < 0) {
                tw->trace.fraction = 0;
            }

            VectorCopy(planes->plane, tw->trace.plane.normal);
            tw->trace.plane.dist = planes->plane[3];
        }
    }
}

/*
====================
CM_CheckFacetPlane
====================
*/
int32 CM_CheckFacetPlane(float* plane, vec3_t start, vec3_t end, float* enterFrac, float* leaveFrac, int32* hit)
{
    float d1, d2, f;

    *hit = false;

    d1 = DotProduct(start, plane) - plane[3];
    d2 = DotProduct(end, plane) - plane[3];

    // if completely in front of face, no intersection with the entire facet
    if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1)) {
        return false;
    }

    // if it doesn't cross the plane, the plane isn't relevent
    if (d1 <= 0 && d2 <= 0) {
        return true;
    }

    // crosses face
    if (d1 > d2) { // enter
        f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
        if (f < 0) {
            f = 0;
        }
        // always favor previous plane hits and thus also the surface plane hit
        if (f > *enterFrac) {
            *enterFrac = f;
            *hit = true;
        }
    } else { // leave
        f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);
        if (f > 1) {
            f = 1;
        }
        if (f < *leaveFrac) {
            *leaveFrac = f;
        }
    }
    return true;
}

/*
====================
CM_TraceThroughPatchCollide
====================
*/
void CM_TraceThroughPatchCollide(traceWork_t* tw, const struct patchCollide_s* pc)
{
    int32         i, j, hit, hitnum;
    float         offset, enterFrac, leaveFrac, t;
    patchPlane_t* planes;
    facet_t*      facet;
    float         plane[4], bestplane[4];
    vec3_t        startp, endp;

    if (tw->isPoint) {
        CM_TracePointThroughPatchCollide(tw, pc);
        return;
    }

    facet = pc->facets;
    for (i = 0; i < pc->numFacets; i++, facet++) {
        enterFrac = -1.0;
        leaveFrac = 1.0;
        hitnum = -1;
        //
        planes = &pc->planes[facet->surfacePlane];
        VectorCopy(planes->plane, plane);
        plane[3] = planes->plane[3];
        if (tw->sphere.use) {
            // adjust the plane distance apropriately for radius
            plane[3] += tw->sphere.radius;

            // find the closest point on the capsule to the plane
            t = DotProduct(plane, tw->sphere.offset);
            if (t > 0.0f) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
                VectorSubtract(tw->end, tw->sphere.offset, endp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
                VectorAdd(tw->end, tw->sphere.offset, endp);
            }
        } else {
            offset = DotProduct(tw->offsets[planes->signbits], plane);
            plane[3] -= offset;
            VectorCopy(tw->start, startp);
            VectorCopy(tw->end, endp);
        }

        if (!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac, &hit)) {
            continue;
        }
        if (hit) {
            Vector4Copy(plane, bestplane);
        }

        for (j = 0; j < facet->numBorders; j++) {
            planes = &pc->planes[facet->borderPlanes[j]];
            if (facet->borderInward[j]) {
                VectorNegate(planes->plane, plane);
                plane[3] = -planes->plane[3];
            } else {
                VectorCopy(planes->plane, plane);
                plane[3] = planes->plane[3];
            }
            if (tw->sphere.use) {
                // adjust the plane distance apropriately for radius
                plane[3] += tw->sphere.radius;

                // find the closest point on the capsule to the plane
                t = DotProduct(plane, tw->sphere.offset);
                if (t > 0.0f) {
                    VectorSubtract(tw->start, tw->sphere.offset, startp);
                    VectorSubtract(tw->end, tw->sphere.offset, endp);
                } else {
                    VectorAdd(tw->start, tw->sphere.offset, startp);
                    VectorAdd(tw->end, tw->sphere.offset, endp);
                }
            } else {
                // NOTE: this works even though the plane might be flipped because the bbox is centered
                offset = DotProduct(tw->offsets[planes->signbits], plane);
                plane[3] += fabs(offset);
                VectorCopy(tw->start, startp);
                VectorCopy(tw->end, endp);
            }

            if (!CM_CheckFacetPlane(plane, startp, endp, &enterFrac, &leaveFrac, &hit)) {
                break;
            }
            if (hit) {
                hitnum = j;
                Vector4Copy(plane, bestplane);
            }
        }
        if (j < facet->numBorders) {
            continue;
        }
        // never clip against the back side
        if (hitnum == facet->numBorders - 1) {
            continue;
        }

        if (enterFrac < leaveFrac && enterFrac >= 0) {
            if (enterFrac < tw->trace.fraction) {
                if (enterFrac < 0) {
                    enterFrac = 0;
                }

                tw->trace.fraction = enterFrac;
                VectorCopy(bestplane, tw->trace.plane.normal);
                tw->trace.plane.dist = bestplane[3];
            }
        }
    }
}


/*
=======================================================================

POSITION TEST

=======================================================================
*/

/*
====================
CM_PositionTestInPatchCollide
====================
*/
bool CM_PositionTestInPatchCollide(traceWork_t* tw, const struct patchCollide_s* pc)
{
    int32         i, j;
    float         offset, t;
    patchPlane_t* planes;
    facet_t*      facet;
    float         plane[4];
    vec3_t        startp;

    if (tw->isPoint) {
        return false;
    }
    //
    facet = pc->facets;
    for (i = 0; i < pc->numFacets; i++, facet++) {
        planes = &pc->planes[facet->surfacePlane];
        VectorCopy(planes->plane, plane);
        plane[3] = planes->plane[3];
        if (tw->sphere.use) {
            // adjust the plane distance apropriately for radius
            plane[3] += tw->sphere.radius;

            // find the closest point on the capsule to the plane
            t = DotProduct(plane, tw->sphere.offset);
            if (t > 0) {
                VectorSubtract(tw->start, tw->sphere.offset, startp);
            } else {
                VectorAdd(tw->start, tw->sphere.offset, startp);
            }
        } else {
            offset = DotProduct(tw->offsets[planes->signbits], plane);
            plane[3] -= offset;
            VectorCopy(tw->start, startp);
        }

        if (DotProduct(plane, startp) - plane[3] > 0.0f) {
            continue;
        }

        for (j = 0; j < facet->numBorders; j++) {
            planes = &pc->planes[facet->borderPlanes[j]];
            if (facet->borderInward[j]) {
                VectorNegate(planes->plane, plane);
                plane[3] = -planes->plane[3];
            } else {
                VectorCopy(planes->plane, plane);
                plane[3] = planes->plane[3];
            }
            if (tw->sphere.use) {
                // adjust the plane distance apropriately for radius
                plane[3] += tw->sphere.radius;

                // find the closest point on the capsule to the plane
                t = DotProduct(plane, tw->sphere.offset);
                if (t > 0.0f) {
                    VectorSubtract(tw->start, tw->sphere.offset, startp);
                } else {
                    VectorAdd(tw->start, tw->sphere.offset, startp);
                }
            } else {
                // NOTE: this works even though the plane might be flipped because the bbox is centered
                offset = DotProduct(tw->offsets[planes->signbits], plane);
                plane[3] += fabs(offset);
                VectorCopy(tw->start, startp);
            }

            if (DotProduct(plane, startp) - plane[3] > 0.0f) {
                break;
            }
        }
        if (j < facet->numBorders) {
            continue;
        }
        // inside this patch facet
        return true;
    }
    return false;
}

/*
=======================================================================

DEBUGGING

=======================================================================
*/
