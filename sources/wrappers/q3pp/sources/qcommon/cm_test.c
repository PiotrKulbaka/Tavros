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


/*
==================
CM_PointLeafnum_r

==================
*/
int32 CM_PointLeafnum_r(const vec3_t p, int32 num)
{
    float     d;
    cNode_t*  node;
    cplane_t* plane;

    while (num >= 0) {
        node = cm.nodes + num;
        plane = node->plane;

        if (plane->side_type < 3) {
            d = p[plane->side_type] - plane->dist;
        } else {
            d = DotProduct(plane->normal, p) - plane->dist;
        }
        if (d < 0) {
            num = node->children[1];
        } else {
            num = node->children[0];
        }
    }

    c_pointcontents++; // optimize counter

    return -1 - num;
}

int32 CM_PointLeafnum(const vec3_t p)
{
    if (!cm.numNodes) { // map not loaded
        return 0;
    }
    return CM_PointLeafnum_r(p, 0);
}


/*
======================================================================

LEAF LISTING

======================================================================
*/


void CM_StoreLeafs(leafList_t* ll, int32 nodenum)
{
    int32 leafNum;

    leafNum = -1 - nodenum;

    // store the lastLeaf even if the list is overflowed
    if (cm.leafs[leafNum].cluster != -1) {
        ll->lastLeaf = leafNum;
    }

    if (ll->count >= ll->maxcount) {
        ll->overflowed = true;
        return;
    }
    ll->list[ll->count++] = leafNum;
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r(leafList_t* ll, int32 nodenum)
{
    cplane_t* plane;
    cNode_t*  node;
    int32     s;

    while (1) {
        if (nodenum < 0) {
            ll->storeLeafs(ll, nodenum);
            return;
        }

        node = &cm.nodes[nodenum];
        plane = node->plane;
        s = BoxOnPlaneSide(ll->bounds[0], ll->bounds[1], plane);
        if (s == 1) {
            nodenum = node->children[0];
        } else if (s == 2) {
            nodenum = node->children[1];
        } else {
            // go down both
            CM_BoxLeafnums_r(ll, node->children[0]);
            nodenum = node->children[1];
        }
    }
}

/*
==================
CM_BoxLeafnums
==================
*/
int32 CM_BoxLeafnums(const vec3_t mins, const vec3_t maxs, int32* list, int32 listsize, int32* lastLeaf)
{
    leafList_t ll;

    cm.checkcount++;

    VectorCopy(mins, ll.bounds[0]);
    VectorCopy(maxs, ll.bounds[1]);
    ll.count = 0;
    ll.maxcount = listsize;
    ll.list = list;
    ll.storeLeafs = CM_StoreLeafs;
    ll.lastLeaf = 0;
    ll.overflowed = false;

    CM_BoxLeafnums_r(&ll, 0);

    *lastLeaf = ll.lastLeaf;
    return ll.count;
}

//====================================================================

/*
==================
CM_PointContents
==================
*/
int32 CM_PointContents(const vec3_t p, clipHandle_t model)
{
    int32     leafnum;
    int32     i, k;
    int32     brushnum;
    cLeaf_t*  leaf;
    cbrush_t* b;
    int32     contents;
    float     d;
    cmodel_t* clipm;

    if (!cm.numNodes) { // map not loaded
        return 0;
    }

    if (model) {
        clipm = CM_ClipHandleToModel(model);
        leaf = &clipm->leaf;
    } else {
        leafnum = CM_PointLeafnum_r(p, 0);
        leaf = &cm.leafs[leafnum];
    }

    contents = 0;
    for (k = 0; k < leaf->numLeafBrushes; k++) {
        brushnum = cm.leafbrushes[leaf->firstLeafBrush + k];
        b = &cm.brushes[brushnum];

        // see if the point is in the brush
        for (i = 0; i < b->numsides; i++) {
            d = DotProduct(p, b->sides[i].plane->normal);
            // FIXME test for Cash
            //            if ( d >= b->sides[i].plane->dist ) {
            if (d > b->sides[i].plane->dist) {
                break;
            }
        }

        if (i == b->numsides) {
            contents |= b->contents;
        }
    }

    return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int32 CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles)
{
    vec3_t p_l;
    vec3_t temp;
    vec3_t forward, right, up;

    // subtract origin offset
    VectorSubtract(p, origin, p_l);

    // rotate start and end into the models frame of reference
    if (model != BOX_MODEL_HANDLE && (angles[0] || angles[1] || angles[2])) {
        AngleVectors(angles, forward, right, up);

        VectorCopy(p_l, temp);
        p_l[0] = DotProduct(temp, forward);
        p_l[1] = -DotProduct(temp, right);
        p_l[2] = DotProduct(temp, up);
    }

    return CM_PointContents(p_l, model);
}


/*
===============================================================================

PVS

===============================================================================
*/

uint8* CM_ClusterPVS(int32 cluster)
{
    if (cluster < 0 || cluster >= cm.numClusters || !cm.vised) {
        return cm.visibility;
    }

    return cm.visibility + cluster * cm.clusterBytes;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r(int32 areaNum, int32 floodnum)
{
    int32    i;
    cArea_t* area;
    int32*   con;

    area = &cm.areas[areaNum];

    if (area->floodvalid == cm.floodvalid) {
        if (area->floodnum == floodnum) {
            return;
        }
        Com_Error(ERR_DROP, "FloodArea_r: reflooded");
    }

    area->floodnum = floodnum;
    area->floodvalid = cm.floodvalid;
    con = cm.areaPortals + areaNum * cm.numAreas;
    for (i = 0; i < cm.numAreas; i++) {
        if (con[i] > 0) {
            CM_FloodArea_r(i, floodnum);
        }
    }
}

/*
====================
CM_FloodAreaConnections

====================
*/
void CM_FloodAreaConnections()
{
    int32    i;
    cArea_t* area;
    int32    floodnum;

    // all current floods are now invalid
    cm.floodvalid++;
    floodnum = 0;

    for (i = 0; i < cm.numAreas; i++) {
        area = &cm.areas[i];
        if (area->floodvalid == cm.floodvalid) {
            continue; // already flooded into
        }
        floodnum++;
        CM_FloodArea_r(i, floodnum);
    }
}

/*
====================
CM_AdjustAreaPortalState

====================
*/
void CM_AdjustAreaPortalState(int32 area1, int32 area2, bool open)
{
    if (area1 < 0 || area2 < 0) {
        return;
    }

    if (area1 >= cm.numAreas || area2 >= cm.numAreas) {
        Com_Error(ERR_DROP, "CM_ChangeAreaPortalState: bad area number");
    }

    if (open) {
        cm.areaPortals[area1 * cm.numAreas + area2]++;
        cm.areaPortals[area2 * cm.numAreas + area1]++;
    } else {
        cm.areaPortals[area1 * cm.numAreas + area2]--;
        cm.areaPortals[area2 * cm.numAreas + area1]--;
        if (cm.areaPortals[area2 * cm.numAreas + area1] < 0) {
            Com_Error(ERR_DROP, "CM_AdjustAreaPortalState: negative reference count");
        }
    }

    CM_FloodAreaConnections();
}

/*
====================
CM_AreasConnected

====================
*/
bool CM_AreasConnected(int32 area1, int32 area2)
{
    if (cm_noAreas->integer) {
        return true;
    }

    if (area1 < 0 || area2 < 0) {
        return false;
    }

    if (area1 >= cm.numAreas || area2 >= cm.numAreas) {
        Com_Error(ERR_DROP, "area >= cm.numAreas");
    }

    if (cm.areas[area1].floodnum == cm.areas[area2].floodnum) {
        return true;
    }
    return false;
}


/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int32 CM_WriteAreaBits(uint8* buffer, int32 area)
{
    int32 i;
    int32 floodnum;
    int32 bytes;

    bytes = (cm.numAreas + 7) >> 3;

    if (cm_noAreas->integer || area == -1) { // for debugging, send everything
        Com_Memset(buffer, 255, bytes);
    } else {
        floodnum = cm.areas[area].floodnum;
        for (i = 0; i < cm.numAreas; i++) {
            if (cm.areas[i].floodnum == floodnum || area == -1) {
                buffer[i >> 3] |= 1 << (i & 7);
            }
        }
    }

    return bytes;
}

