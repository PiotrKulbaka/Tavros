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

// this is only used for visualization tools in cm_ debug functions


#include "cm_local.h"


// counters are only bumped when running single threaded,
// because they are an awefull coherence problem
int32 c_active_windings;
int32 c_peak_windings;
int32 c_winding_allocs;
int32 c_winding_points;

void pw(winding_t* w)
{
    int32 i;
    for (i = 0; i < w->numpoints; i++) {
        printf("(%5.1f, %5.1f, %5.1f)\n", w->p[i][0], w->p[i][1], w->p[i][2]);
    }
}


/*
=============
AllocWinding
=============
*/
winding_t* AllocWinding(int32 points)
{
    winding_t* w;
    int32      s;

    c_winding_allocs++;
    c_winding_points += points;
    c_active_windings++;
    if (c_active_windings > c_peak_windings) {
        c_peak_windings = c_active_windings;
    }

    s = sizeof(vec_t) * 3 * points + sizeof(int32);
    w = (winding_t*) Z_Malloc(s);
    Com_Memset(w, 0, s);
    return w;
}

void FreeWinding(winding_t* w)
{
    if (*(uint32*) w == 0xdeaddead) {
        Com_Error(ERR_FATAL, "FreeWinding: freed a freed winding");
    }
    *(uint32*) w = 0xdeaddead;

    c_active_windings--;
    Z_Free(w);
}

/*
=============
WindingBounds
=============
*/
void WindingBounds(winding_t* w, vec3_t mins, vec3_t maxs)
{
    vec_t v;
    int32 i, j;

    mins[0] = mins[1] = mins[2] = MAX_MAP_BOUNDS;
    maxs[0] = maxs[1] = maxs[2] = -MAX_MAP_BOUNDS;

    for (i = 0; i < w->numpoints; i++) {
        for (j = 0; j < 3; j++) {
            v = w->p[i][j];
            if (v < mins[j]) {
                mins[j] = v;
            }
            if (v > maxs[j]) {
                maxs[j] = v;
            }
        }
    }
}

/*
=================
BaseWindingForPlane
=================
*/
winding_t* BaseWindingForPlane(vec3_t normal, vec_t dist)
{
    int32      i, x;
    vec_t      max, v;
    vec3_t     org, vright, vup;
    winding_t* w;

    // find the major axis

    max = -MAX_MAP_BOUNDS;
    x = -1;
    for (i = 0; i < 3; i++) {
        v = fabs(normal[i]);
        if (v > max) {
            x = i;
            max = v;
        }
    }
    if (x == -1) {
        Com_Error(ERR_DROP, "BaseWindingForPlane: no axis found");
    }

    VectorCopy(vec3_origin, vup);
    switch (x) {
    case 0:
    case 1:
        vup[2] = 1;
        break;
    case 2:
        vup[0] = 1;
        break;
    }

    v = DotProduct(vup, normal);
    VectorMA(vup, -v, normal, vup);
    VectorNormalize2(vup, vup);

    VectorScale(normal, dist, org);

    CrossProduct(vup, normal, vright);

    VectorScale(vup, MAX_MAP_BOUNDS, vup);
    VectorScale(vright, MAX_MAP_BOUNDS, vright);

    // project a really big    axis aligned box onto the plane
    w = AllocWinding(4);

    VectorSubtract(org, vright, w->p[0]);
    VectorAdd(w->p[0], vup, w->p[0]);

    VectorAdd(org, vright, w->p[1]);
    VectorAdd(w->p[1], vup, w->p[1]);

    VectorAdd(org, vright, w->p[2]);
    VectorSubtract(w->p[2], vup, w->p[2]);

    VectorSubtract(org, vright, w->p[3]);
    VectorSubtract(w->p[3], vup, w->p[3]);

    w->numpoints = 4;

    return w;
}

/*
==================
CopyWinding
==================
*/
winding_t* CopyWinding(winding_t* w)
{
    int32      size;
    winding_t* c;

    c = AllocWinding(w->numpoints);
    size = (int32) ((winding_t*) 0)->p[w->numpoints];
    Com_Memcpy(c, w, size);
    return c;
}

/*
=============
ChopWindingInPlace
=============
*/
void ChopWindingInPlace(winding_t** inout, vec3_t normal, vec_t dist, vec_t epsilon)
{
    winding_t*   in;
    vec_t        dists[MAX_POINTS_ON_WINDING + 4];
    int32        sides[MAX_POINTS_ON_WINDING + 4];
    int32        counts[3];
    static vec_t dot; // VC 4.2 optimizer bug if not static
    int32        i, j;
    vec_t *      p1, *p2;
    vec3_t       mid;
    winding_t*   f;
    int32        maxpts;

    in = *inout;
    counts[0] = counts[1] = counts[2] = 0;

    // determine sides for each point
    for (i = 0; i < in->numpoints; i++) {
        dot = DotProduct(in->p[i], normal);
        dot -= dist;
        dists[i] = dot;
        if (dot > epsilon) {
            sides[i] = SIDE_FRONT;
        } else if (dot < -epsilon) {
            sides[i] = SIDE_BACK;
        } else {
            sides[i] = SIDE_ON;
        }
        counts[sides[i]]++;
    }
    sides[i] = sides[0];
    dists[i] = dists[0];

    if (!counts[0]) {
        FreeWinding(in);
        *inout = NULL;
        return;
    }
    if (!counts[1]) {
        return; // inout stays the same
    }

    maxpts = in->numpoints + 4; // cant use counts[0]+2 because
                                // of fp grouping errors

    f = AllocWinding(maxpts);

    for (i = 0; i < in->numpoints; i++) {
        p1 = in->p[i];

        if (sides[i] == SIDE_ON) {
            VectorCopy(p1, f->p[f->numpoints]);
            f->numpoints++;
            continue;
        }

        if (sides[i] == SIDE_FRONT) {
            VectorCopy(p1, f->p[f->numpoints]);
            f->numpoints++;
        }

        if (sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i]) {
            continue;
        }

        // generate a split point
        p2 = in->p[(i + 1) % in->numpoints];

        dot = dists[i] / (dists[i] - dists[i + 1]);
        for (j = 0; j < 3; j++) { // avoid round off error when possible
            if (normal[j] == 1) {
                mid[j] = dist;
            } else if (normal[j] == -1) {
                mid[j] = -dist;
            } else {
                mid[j] = p1[j] + dot * (p2[j] - p1[j]);
            }
        }

        VectorCopy(mid, f->p[f->numpoints]);
        f->numpoints++;
    }

    if (f->numpoints > maxpts) {
        Com_Error(ERR_DROP, "ClipWinding: points exceeded estimate");
    }
    if (f->numpoints > MAX_POINTS_ON_WINDING) {
        Com_Error(ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING");
    }

    FreeWinding(in);
    *inout = f;
}
