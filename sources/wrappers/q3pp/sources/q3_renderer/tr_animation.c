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

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

/*
==============
R_AddAnimSurfaces
==============
*/
void R_AddAnimSurfaces(trRefEntity_t* ent)
{
    md4Header_t*  header;
    md4Surface_t* surface;
    md4LOD_t*     lod;
    shader_t*     shader;
    int32         i;

    header = tr.currentModel->md4;
    lod = (md4LOD_t*) ((uint8*) header + header->ofsLODs);

    surface = (md4Surface_t*) ((uint8*) lod + lod->ofsSurfaces);
    for (i = 0; i < lod->numSurfaces; i++) {
        shader = R_GetShaderByHandle(surface->shaderIndex);
        R_AddDrawSurf((surfaceType_t*) surface, shader, 0 /*fogNum*/, false);
        surface = (md4Surface_t*) ((uint8*) surface + surface->ofsEnd);
    }
}


/*
==============
RB_SurfaceAnim
==============
*/
void RB_SurfaceAnim(md4Surface_t* surface)
{
    int32        i, j, k;
    float        frontlerp, backlerp;
    int32*       triangles;
    int32        indexes;
    int32        baseIndex, baseVertex;
    int32        numVerts;
    md4Vertex_t* v;
    md4Bone_t    bones[MD4_MAX_BONES];
    md4Bone_t *  bonePtr, *bone;
    md4Header_t* header;
    md4Frame_t*  frame;
    md4Frame_t*  oldFrame;
    int32        frameSize;


    if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) {
        backlerp = 0;
        frontlerp = 1;
    } else {
        backlerp = backEnd.currentEntity->e.backlerp;
        frontlerp = 1.0f - backlerp;
    }
    header = (md4Header_t*) ((uint8*) surface + surface->ofsHeader);

    frameSize = (int32) (&((md4Frame_t*) 0)->bones[header->numBones]);

    frame = (md4Frame_t*) ((uint8*) header + header->ofsFrames + backEnd.currentEntity->e.frame * frameSize);
    oldFrame = (md4Frame_t*) ((uint8*) header + header->ofsFrames + backEnd.currentEntity->e.oldframe * frameSize);

    RB_CheckOverflow(surface->numVerts, surface->numTriangles * 3);

    triangles = (int32*) ((uint8*) surface + surface->ofsTriangles);
    indexes = surface->numTriangles * 3;
    baseIndex = tess.numIndexes;
    baseVertex = tess.numVertexes;
    for (j = 0; j < indexes; j++) {
        tess.indexes[baseIndex + j] = baseIndex + triangles[j];
    }
    tess.numIndexes += indexes;

    //
    // lerp all the needed bones
    //
    if (!backlerp) {
        // no lerping needed
        bonePtr = frame->bones;
    } else {
        bonePtr = bones;
        for (i = 0; i < header->numBones * 12; i++) {
            ((float*) bonePtr)[i] = frontlerp * ((float*) frame->bones)[i]
                                  + backlerp * ((float*) oldFrame->bones)[i];
        }
    }

    //
    // deform the vertexes by the lerped bones
    //
    numVerts = surface->numVerts;
    // FIXME
    // This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
    // in for reference.
    // v = (md4Vertex_t *) ((uint8 *)surface + surface->ofsVerts + 12);
    v = (md4Vertex_t*) ((uint8*) surface + surface->ofsVerts);
    for (j = 0; j < numVerts; j++) {
        vec3_t       tempVert, tempNormal;
        md4Weight_t* w;

        VectorClear(tempVert);
        VectorClear(tempNormal);
        w = v->weights;
        for (k = 0; k < v->numWeights; k++, w++) {
            bone = bonePtr + w->boneIndex;

            tempVert[0] += w->boneWeight * (DotProduct(bone->matrix[0], w->offset) + bone->matrix[0][3]);
            tempVert[1] += w->boneWeight * (DotProduct(bone->matrix[1], w->offset) + bone->matrix[1][3]);
            tempVert[2] += w->boneWeight * (DotProduct(bone->matrix[2], w->offset) + bone->matrix[2][3]);

            tempNormal[0] += w->boneWeight * DotProduct(bone->matrix[0], v->normal);
            tempNormal[1] += w->boneWeight * DotProduct(bone->matrix[1], v->normal);
            tempNormal[2] += w->boneWeight * DotProduct(bone->matrix[2], v->normal);
        }

        tess.xyz[baseVertex + j][0] = tempVert[0];
        tess.xyz[baseVertex + j][1] = tempVert[1];
        tess.xyz[baseVertex + j][2] = tempVert[2];

        tess.normal[baseVertex + j][0] = tempNormal[0];
        tess.normal[baseVertex + j][1] = tempNormal[1];
        tess.normal[baseVertex + j][2] = tempNormal[2];

        tess._texCoords[baseVertex + j][0] = v->_texCoords;

        // FIXME
        // This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
        // in for reference.
        // v = (md4Vertex_t *)( ( uint8 * )&v->weights[v->numWeights] + 12 );
        v = (md4Vertex_t*) &v->weights[v->numWeights];
    }

    tess.numVertexes += surface->numVerts;
}


