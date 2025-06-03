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
// tr_models.c -- model loading and caching

#include "tr_local.h"

static tavros::core::logger logger("tr_model");

static bool R_LoadMD3(model_t* mod, int32 lod, void* buffer, const char* name);
static bool R_LoadMD4(model_t* mod, void* buffer, const char* name);

model_t* loadmodel;

/*
** R_GetModelByHandle
*/
model_t* R_GetModelByHandle(qhandle_t index)
{
    model_t* mod;

    // out of range gets the defualt model
    if (index < 1 || index >= tr.numModels) {
        return tr.models[0];
    }

    mod = tr.models[index];

    return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t* R_AllocModel()
{
    model_t* mod;

    if (tr.numModels == MAX_MOD_KNOWN) {
        return NULL;
    }

    mod = (model_t*) Hunk_Alloc(sizeof(*tr.models[tr.numModels]), h_low);
    mod->index = tr.numModels;
    tr.models[tr.numModels] = mod;
    tr.numModels++;

    return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel(const char* name)
{
    model_t*  mod;
    uint32*   buf;
    int32     lod;
    int32     ident;
    bool      loaded;
    qhandle_t hModel;
    int32     numLoaded;

    if (!name || !name[0]) {
        logger.info("RE_RegisterModel: NULL name");
        return 0;
    }

    if (strlen(name) >= MAX_QPATH) {
        logger.warning("Model name exceeds MAX_QPATH");
        return 0;
    }

    //
    // search the currently loaded models
    //
    for (hModel = 1; hModel < tr.numModels; hModel++) {
        mod = tr.models[hModel];
        if (!strcmp(mod->name, name)) {
            if (mod->type == MOD_BAD) {
                return 0;
            }
            return hModel;
        }
    }

    // allocate a new model_t

    if ((mod = R_AllocModel()) == NULL) {
        logger.warning("RE_RegisterModel: R_AllocModel() failed for '%s'", name);
        return 0;
    }

    // only set the name after the model has been successfully loaded
    Q_strncpyz(mod->name, name, sizeof(mod->name));

    mod->numLods = 0;

    //
    // load the files
    //
    numLoaded = 0;

    for (lod = MD3_MAX_LODS - 1; lod >= 0; lod--) {
        char filename[1024];

        strcpy(filename, name);

        if (lod != 0) {
            char namebuf[80];

            if (strrchr(filename, '.')) {
                *strrchr(filename, '.') = 0;
            }
            sprintf(namebuf, "_%d.md3", lod);
            strcat(filename, namebuf);
        }

        FS_ReadFile(filename, (void**) &buf);
        if (!buf) {
            continue;
        }

        loadmodel = mod;

        ident = (*(uint32*) buf);
        if (ident == MD4_IDENT) {
            loaded = R_LoadMD4(mod, buf, name);
        } else {
            if (ident != MD3_IDENT) {
                logger.warning("RE_RegisterModel: unknown fileid for %s", name);
                goto fail;
            }

            loaded = R_LoadMD3(mod, lod, buf, name);
        }

        FS_FreeFile(buf);

        if (!loaded) {
            if (lod == 0) {
                goto fail;
            } else {
                break;
            }
        } else {
            mod->numLods++;
            numLoaded++;
        }
    }

    if (numLoaded) {
        // duplicate into higher lod spots that weren't  loaded
        for (lod--; lod >= 0; lod--) {
            mod->numLods++;
            mod->md3[lod] = mod->md3[lod + 1];
        }

        return mod->index;
    }
#ifdef DEBUG
    else {
        logger.warning("RE_RegisterModel: couldn't load %s", name);
    }
#endif

fail:
    // we still keep the model_t around, so if the model name is asked for
    // again, we won't bother scanning the filesystem
    mod->type = MOD_BAD;
    return 0;
}


/*
=================
R_LoadMD3
=================
*/
static bool R_LoadMD3(model_t* mod, int32 lod, void* buffer, const char* mod_name)
{
    int32           i, j;
    md3Header_t*    pinmodel;
    md3Frame_t*     frame;
    md3Surface_t*   surf;
    md3Shader_t*    shader;
    md3XyzNormal_t* xyz;
    md3Tag_t*       tag;
    int32           version;
    int32           size;

    pinmodel = (md3Header_t*) buffer;

    version = (pinmodel->version);
    if (version != MD3_VERSION) {
        logger.warning("R_LoadMD3: %s has wrong version (%i should be %i)", mod_name, version, MD3_VERSION);
        return false;
    }

    mod->type = MOD_MESH;
    size = (pinmodel->ofsEnd);
    mod->dataSize += size;
    mod->md3[lod] = (md3Header_t*) Hunk_Alloc(size, h_low);

    Com_Memcpy(mod->md3[lod], buffer, (pinmodel->ofsEnd));

    if (mod->md3[lod]->numFrames < 1) {
        logger.warning("R_LoadMD3: %s has no frames", mod_name);
        return false;
    }

    // swap all the frames
    frame = (md3Frame_t*) ((uint8*) mod->md3[lod] + mod->md3[lod]->ofsFrames);
    for (i = 0; i < mod->md3[lod]->numFrames; i++, frame++) {
        frame->radius = (frame->radius);
        for (j = 0; j < 3; j++) {
            frame->bounds[0][j] = (frame->bounds[0][j]);
            frame->bounds[1][j] = (frame->bounds[1][j]);
            frame->localOrigin[j] = (frame->localOrigin[j]);
        }
    }

    // swap all the tags
    tag = (md3Tag_t*) ((uint8*) mod->md3[lod] + mod->md3[lod]->ofsTags);
    for (i = 0; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames; i++, tag++) {
        for (j = 0; j < 3; j++) {
            tag->origin[j] = (tag->origin[j]);
            tag->axis[0][j] = (tag->axis[0][j]);
            tag->axis[1][j] = (tag->axis[1][j]);
            tag->axis[2][j] = (tag->axis[2][j]);
        }
    }

    // swap all the surfaces
    surf = (md3Surface_t*) ((uint8*) mod->md3[lod] + mod->md3[lod]->ofsSurfaces);
    for (i = 0; i < mod->md3[lod]->numSurfaces; i++) {
        if (surf->numVerts > SHADER_MAX_VERTEXES) {
            Com_Error(ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)", mod_name, SHADER_MAX_VERTEXES, surf->numVerts);
        }
        if (surf->numTriangles * 3 > SHADER_MAX_INDEXES) {
            Com_Error(ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)", mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles);
        }

        // change to surface identifier
        surf->ident = SF_MD3;

        // lowercase the surface name so skin compares are faster
        Q_strlwr(surf->name);

        // strip off a trailing _1 or _2
        // this is a crutch for q3data being a mess
        j = strlen(surf->name);
        if (j > 2 && surf->name[j - 2] == '_') {
            surf->name[j - 2] = 0;
        }

        // register the shaders
        shader = (md3Shader_t*) ((uint8*) surf + surf->ofsShaders);
        for (j = 0; j < surf->numShaders; j++, shader++) {
            shader_t* sh;

            sh = R_FindShader(shader->name, LIGHTMAP_NONE, true);
            if (sh->defaultShader) {
                shader->shaderIndex = 0;
            } else {
                shader->shaderIndex = sh->index;
            }
        }

        // find the next surface
        surf = (md3Surface_t*) ((uint8*) surf + surf->ofsEnd);
    }

    return true;
}


/*
=================
R_LoadMD4
=================
*/
static bool R_LoadMD4(model_t* mod, void* buffer, const char* mod_name)
{
    int32         i, j, k, lodindex;
    md4Header_t * pinmodel, *md4;
    md4Frame_t*   frame;
    md4LOD_t*     lod;
    md4Surface_t* surf;
    md4Vertex_t*  v;
    int32         version;
    int32         size;
    shader_t*     sh;
    int32         frameSize;

    pinmodel = (md4Header_t*) buffer;

    version = (pinmodel->version);
    if (version != MD4_VERSION) {
        logger.warning("R_LoadMD4: %s has wrong version (%i should be %i)", mod_name, version, MD4_VERSION);
        return false;
    }

    mod->type = MOD_MD4;
    size = (pinmodel->ofsEnd);
    mod->dataSize += size;
    md4 = mod->md4 = (md4Header_t*) Hunk_Alloc(size, h_low);

    Com_Memcpy(md4, buffer, (pinmodel->ofsEnd));

    if (md4->numFrames < 1) {
        logger.warning("R_LoadMD4: %s has no frames", mod_name);
        return false;
    }

    // we don't need to swap tags in the renderer, they aren't used

    // swap all the frames
    frameSize = (int32) (&((md4Frame_t*) 0)->bones[md4->numBones]);
    for (i = 0; i < md4->numFrames; i++, frame++) {
        frame = (md4Frame_t*) ((uint8*) md4 + md4->ofsFrames + i * frameSize);
        frame->radius = (frame->radius);
        for (j = 0; j < 3; j++) {
            frame->bounds[0][j] = (frame->bounds[0][j]);
            frame->bounds[1][j] = (frame->bounds[1][j]);
            frame->localOrigin[j] = (frame->localOrigin[j]);
        }
        for (j = 0; j < md4->numBones * sizeof(md4Bone_t) / 4; j++) {
            ((float*) frame->bones)[j] = (((float*) frame->bones)[j]);
        }
    }

    // swap all the LOD's
    lod = (md4LOD_t*) ((uint8*) md4 + md4->ofsLODs);
    for (lodindex = 0; lodindex < md4->numLODs; lodindex++) {
        // swap all the surfaces
        surf = (md4Surface_t*) ((uint8*) lod + lod->ofsSurfaces);
        for (i = 0; i < lod->numSurfaces; i++) {
            if (surf->numVerts > SHADER_MAX_VERTEXES) {
                Com_Error(ERR_DROP, "R_LoadMD3: %s has more than %i verts on a surface (%i)", mod_name, SHADER_MAX_VERTEXES, surf->numVerts);
            }
            if (surf->numTriangles * 3 > SHADER_MAX_INDEXES) {
                Com_Error(ERR_DROP, "R_LoadMD3: %s has more than %i triangles on a surface (%i)", mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles);
            }

            // change to surface identifier
            surf->ident = SF_MD4;

            // lowercase the surface name so skin compares are faster
            Q_strlwr(surf->name);

            // register the shaders
            sh = R_FindShader(surf->shader, LIGHTMAP_NONE, true);
            if (sh->defaultShader) {
                surf->shaderIndex = 0;
            } else {
                surf->shaderIndex = sh->index;
            }

            // swap all the vertexes
            // FIXME
            // This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
            // in for reference.
            // v = (md4Vertex_t *) ( (uint8 *)surf + surf->ofsVerts + 12);
            v = (md4Vertex_t*) ((uint8*) surf + surf->ofsVerts);
            for (j = 0; j < surf->numVerts; j++) {
                v->normal[0] = (v->normal[0]);
                v->normal[1] = (v->normal[1]);
                v->normal[2] = (v->normal[2]);

                v->numWeights = (v->numWeights);

                for (k = 0; k < v->numWeights; k++) {
                    v->weights[k].boneIndex = (v->weights[k].boneIndex);
                    v->weights[k].boneWeight = (v->weights[k].boneWeight);
                    v->weights[k].offset[0] = (v->weights[k].offset[0]);
                    v->weights[k].offset[1] = (v->weights[k].offset[1]);
                    v->weights[k].offset[2] = (v->weights[k].offset[2]);
                }
                // FIXME
                // This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
                // in for reference.
                // v = (md4Vertex_t *)( ( uint8 * )&v->weights[v->numWeights] + 12 );
                v = (md4Vertex_t*) ((uint8*) &v->weights[v->numWeights]);
            }

            // find the next surface
            surf = (md4Surface_t*) ((uint8*) surf + surf->ofsEnd);
        }

        // find the next LOD
        lod = (md4LOD_t*) ((uint8*) lod + lod->ofsEnd);
    }

    return true;
}


//=============================================================================

/*
** RE_BeginRegistration
*/
void RE_BeginRegistration(glconfig_t* glconfigOut)
{
    R_Init();

    *glconfigOut = glConfig;

    tr.viewCluster = -1; // force markleafs to regenerate
    RE_ClearScene();

    tr.registered = true;

    // NOTE: this sucks, for some reason the first stretch pic is never drawn
    // without this we'd see a white flash on a level load because the very
    // first time the level shot would not be drawn
    // RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
}

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit()
{
    model_t* mod;

    // leave a space for NULL model
    tr.numModels = 0;

    mod = R_AllocModel();
    mod->type = MOD_BAD;
}


/*
================
R_Modellist_f
================
*/
void R_Modellist_f()
{
    int32    i, j;
    model_t* mod;
    int32    total;
    int32    lods;

    total = 0;
    for (i = 1; i < tr.numModels; i++) {
        mod = tr.models[i];
        lods = 1;
        for (j = 1; j < MD3_MAX_LODS; j++) {
            if (mod->md3[j] && mod->md3[j] != mod->md3[j - 1]) {
                lods++;
            }
        }
        logger.info("%8i : (%i) %s", mod->dataSize, lods, mod->name);
        total += mod->dataSize;
    }
    logger.info("%8i : Total models", total);
}


//=============================================================================


/*
================
R_GetTag
================
*/
static md3Tag_t* R_GetTag(md3Header_t* mod, int32 frame, const char* tagName)
{
    md3Tag_t* tag;
    int32     i;

    if (frame >= mod->numFrames) {
        // it is possible to have a bad frame while changing models, so don't error
        frame = mod->numFrames - 1;
    }

    tag = (md3Tag_t*) ((uint8*) mod + mod->ofsTags) + frame * mod->numTags;
    for (i = 0; i < mod->numTags; i++, tag++) {
        if (!strcmp(tag->name, tagName)) {
            return tag; // found it
        }
    }

    return NULL;
}

/*
================
R_LerpTag
================
*/
int32 R_LerpTag(orientation_t* tag, qhandle_t handle, int32 startFrame, int32 endFrame, float frac, const char* tagName)
{
    md3Tag_t *start, *end;
    int32     i;
    float     frontLerp, backLerp;
    model_t*  model;

    model = R_GetModelByHandle(handle);
    if (!model->md3[0]) {
        AxisClear(tag->axis);
        VectorClear(tag->origin);
        return false;
    }

    start = R_GetTag(model->md3[0], startFrame, tagName);
    end = R_GetTag(model->md3[0], endFrame, tagName);
    if (!start || !end) {
        AxisClear(tag->axis);
        VectorClear(tag->origin);
        return false;
    }

    frontLerp = frac;
    backLerp = 1.0f - frac;

    for (i = 0; i < 3; i++) {
        tag->origin[i] = start->origin[i] * backLerp + end->origin[i] * frontLerp;
        tag->axis[0][i] = start->axis[0][i] * backLerp + end->axis[0][i] * frontLerp;
        tag->axis[1][i] = start->axis[1][i] * backLerp + end->axis[1][i] * frontLerp;
        tag->axis[2][i] = start->axis[2][i] * backLerp + end->axis[2][i] * frontLerp;
    }
    VectorNormalize(tag->axis[0]);
    VectorNormalize(tag->axis[1]);
    VectorNormalize(tag->axis[2]);
    return true;
}


/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs)
{
    model_t*     model;
    md3Header_t* header;
    md3Frame_t*  frame;

    model = R_GetModelByHandle(handle);

    if (model->bmodel) {
        VectorCopy(model->bmodel->bounds[0], mins);
        VectorCopy(model->bmodel->bounds[1], maxs);
        return;
    }

    if (!model->md3[0]) {
        VectorClear(mins);
        VectorClear(maxs);
        return;
    }

    header = model->md3[0];

    frame = (md3Frame_t*) ((uint8*) header + header->ofsFrames);

    VectorCopy(frame->bounds[0], mins);
    VectorCopy(frame->bounds[1], maxs);
}
