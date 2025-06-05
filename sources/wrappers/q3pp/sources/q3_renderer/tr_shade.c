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
// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;
static bool      setArraysOnce;

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage(textureBundle_t* bundle)
{
    int32 index;

    if (bundle->numImageAnimations <= 1) {
        GL_Bind(bundle->image[0]);
        return;
    }

    // it is necessary to do this messy calc to make sure animations line up
    // exactly with waveforms of the same frequency
    index = myftol(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
    index >>= FUNCTABLE_SIZE2;

    if (index < 0) {
        index = 0; // may happen with shader time offsets
    }
    index %= bundle->numImageAnimations;

    GL_Bind(bundle->image[index]);
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface(shader_t* shader, int32 fogNum)
{
    shader_t* state = (shader->remappedShader) ? shader->remappedShader : shader;

    tess.numIndexes = 0;
    tess.numVertexes = 0;
    tess.shader = state;
    tess.fogNum = fogNum;
    tess.dlightBits = 0; // will be OR'd in by surface functions
    tess.xstages = state->stages;
    tess.numPasses = state->numUnfoggedPasses;
    tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

    tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
    if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
        tess.shaderTime = tess.shader->clampTime;
    }
}

/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture()
{
    int32  i, l;
    vec3_t origin;
    float* texCoords;
    uint8* colors;
    uint8  clipBits[SHADER_MAX_VERTEXES];
    float  texCoordsArray[SHADER_MAX_VERTEXES][2];
    uint8  colorArray[SHADER_MAX_VERTEXES][4];
    uint32 hitIndexes[SHADER_MAX_INDEXES];
    int32  numIndexes;
    float  scale;
    float  radius;
    vec3_t floatColor;
    float  modulate;

    if (!backEnd.refdef.num_dlights) {
        return;
    }


    for (l = 0; l < backEnd.refdef.num_dlights; l++) {
        dlight_t* dl;

        if (!(tess.dlightBits & (1 << l))) {
            continue; // this surface definately doesn't have any of this light
        }
        texCoords = texCoordsArray[0];
        colors = colorArray[0];

        dl = &backEnd.refdef.dlights[l];
        VectorCopy(dl->transformed, origin);

        radius = dl->radius;
        scale = 1.0f / radius;

        floatColor[0] = dl->color[0] * 255.0f;
        floatColor[1] = dl->color[1] * 255.0f;
        floatColor[2] = dl->color[2] * 255.0f;

        for (i = 0; i < tess.numVertexes; i++, texCoords += 2, colors += 4) {
            vec3_t dist;
            int32  clip;

            VectorSubtract(origin, tess.xyz[i].data(), dist);
            texCoords[0] = 0.5f + dist[0] * scale;
            texCoords[1] = 0.5f + dist[1] * scale;

            clip = 0;
            if (texCoords[0] < 0.0f) {
                clip |= 1;
            } else if (texCoords[0] > 1.0f) {
                clip |= 2;
            }
            if (texCoords[1] < 0.0f) {
                clip |= 4;
            } else if (texCoords[1] > 1.0f) {
                clip |= 8;
            }
            // modulate the strength based on the height and color
            if (dist[2] > radius) {
                clip |= 16;
                modulate = 0.0f;
            } else if (dist[2] < -radius) {
                clip |= 32;
                modulate = 0.0f;
            } else {
                dist[2] = Q_fabs(dist[2]);
                if (dist[2] < radius * 0.5f) {
                    modulate = 1.0f;
                } else {
                    modulate = 2.0f * (radius - dist[2]) * scale;
                }
            }
            clipBits[i] = clip;

            colors[0] = myftol(floatColor[0] * modulate);
            colors[1] = myftol(floatColor[1] * modulate);
            colors[2] = myftol(floatColor[2] * modulate);
            colors[3] = 255;
        }

        // build a list of triangles that need light
        numIndexes = 0;
        for (i = 0; i < tess.numIndexes; i += 3) {
            int32 a, b, c;

            a = tess.indexes[i];
            b = tess.indexes[i + 1];
            c = tess.indexes[i + 2];
            if (clipBits[a] & clipBits[b] & clipBits[c]) {
                continue; // not lighted
            }
            hitIndexes[numIndexes] = a;
            hitIndexes[numIndexes + 1] = b;
            hitIndexes[numIndexes + 2] = c;
            numIndexes += 3;
        }

        if (!numIndexes) {
            continue;
        }

        qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
        qglTexCoordPointer(2, GL_FLOAT, 0, texCoordsArray[0]);

        qglEnableClientState(GL_COLOR_ARRAY);
        qglColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

        GL_Bind(tr.dlightImage);
        // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
        // where they aren't rendered
        if (dl->additive) {
            GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
        } else {
            GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
        }
        qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, hitIndexes);
    }
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass()
{
    fog_t* fog;
    int32  i;

    qglEnableClientState(GL_COLOR_ARRAY);
    qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

    qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars._texCoords->data());

    fog = tr.world->fogs + tess.fogNum;

    for (i = 0; i < tess.numVertexes; i++) {
        *(int32*) &tess.svars.colors[i] = fog->colorInt;
    }

    RB_CalcFogTexCoords((float*) tess.svars._texCoords->data());

    GL_Bind(tr.fogImage);

    if (tess.shader->fogPass == FP_EQUAL) {
        GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
    } else {
        GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
    }

    qglDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, tess.indexes);
}

/*
===============
ComputeColors
===============
*/
static void ComputeColors(shaderStage_t* pStage)
{
    int32 i;

    //
    // rgbGen
    //
    switch (pStage->rgbGen) {
    case CGEN_IDENTITY:
        Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
        break;
    default:
    case CGEN_IDENTITY_LIGHTING:
        Com_Memset(tess.svars.colors, 255, tess.numVertexes * 4);
        break;
    case CGEN_LIGHTING_DIFFUSE:
        RB_CalcDiffuseColor((uint8*) tess.svars.colors);
        break;
    case CGEN_EXACT_VERTEX:
        Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
        break;
    case CGEN_CONST:
        for (i = 0; i < tess.numVertexes; i++) {
            *(int32*) tess.svars.colors[i] = *(int32*) pStage->constantColor;
        }
        break;
    case CGEN_VERTEX:
        Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
        break;
    case CGEN_ONE_MINUS_VERTEX:
        for (i = 0; i < tess.numVertexes; i++) {
            tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
            tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
            tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
        }
        break;
    case CGEN_FOG: {
        fog_t* fog;

        fog = tr.world->fogs + tess.fogNum;

        for (i = 0; i < tess.numVertexes; i++) {
            *(int32*) &tess.svars.colors[i] = fog->colorInt;
        }
    } break;
    case CGEN_WAVEFORM:
        RB_CalcWaveColor(&pStage->rgbWave, (uint8*) tess.svars.colors);
        break;
    case CGEN_ENTITY:
        RB_CalcColorFromEntity((uint8*) tess.svars.colors);
        break;
    case CGEN_ONE_MINUS_ENTITY:
        RB_CalcColorFromOneMinusEntity((uint8*) tess.svars.colors);
        break;
    }

    //
    // alphaGen
    //
    switch (pStage->alphaGen) {
    case AGEN_SKIP:
        break;
    case AGEN_IDENTITY:
        if ((pStage->rgbGen != CGEN_IDENTITY) && (pStage->rgbGen != CGEN_VERTEX)) {
            for (i = 0; i < tess.numVertexes; i++) {
                tess.svars.colors[i][3] = 0xff;
            }
        }
        break;
    case AGEN_CONST:
        if (pStage->rgbGen != CGEN_CONST) {
            for (i = 0; i < tess.numVertexes; i++) {
                tess.svars.colors[i][3] = pStage->constantColor[3];
            }
        }
        break;
    case AGEN_WAVEFORM:
        RB_CalcWaveAlpha(&pStage->alphaWave, (uint8*) tess.svars.colors);
        break;
    case AGEN_LIGHTING_SPECULAR:
        RB_CalcSpecularAlpha((uint8*) tess.svars.colors);
        break;
    case AGEN_ENTITY:
        RB_CalcAlphaFromEntity((uint8*) tess.svars.colors);
        break;
    case AGEN_ONE_MINUS_ENTITY:
        RB_CalcAlphaFromOneMinusEntity((uint8*) tess.svars.colors);
        break;
    case AGEN_VERTEX:
        if (pStage->rgbGen != CGEN_VERTEX) {
            for (i = 0; i < tess.numVertexes; i++) {
                tess.svars.colors[i][3] = tess.vertexColors[i][3];
            }
        }
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for (i = 0; i < tess.numVertexes; i++) {
            tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
    case AGEN_PORTAL: {
        uint8 alpha;

        for (i = 0; i < tess.numVertexes; i++) {
            float  len;
            vec3_t v;

            VectorSubtract(tess.xyz[i].data(), backEnd.viewParms.orient.origin, v);
            len = VectorLength(v);

            len /= tess.shader->portalRange;

            if (len < 0) {
                alpha = 0;
            } else if (len > 1) {
                alpha = 0xff;
            } else {
                alpha = len * 0xff;
            }

            tess.svars.colors[i][3] = alpha;
        }
    } break;
    }

    //
    // fog adjustment for colors to fade out as fog increases
    //
    if (tess.fogNum) {
        switch (pStage->adjustColorsForFog) {
        case ACFF_MODULATE_RGB:
            RB_CalcModulateColorsByFog((uint8*) tess.svars.colors);
            break;
        case ACFF_MODULATE_ALPHA:
            RB_CalcModulateAlphasByFog((uint8*) tess.svars.colors);
            break;
        case ACFF_MODULATE_RGBA:
            RB_CalcModulateRGBAsByFog((uint8*) tess.svars.colors);
            break;
        case ACFF_NONE:
            break;
        }
    }
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords(shaderStage_t* pStage)
{
    int32 i;
    int32 b;

    int32 tm;

    //
    // generate the texture coordinates
    //
    switch (pStage->bundle.tcGen) {
    case TCGEN_IDENTITY:
        Com_Memset(tess.svars._texCoords->data(), 0, sizeof(float) * 2 * tess.numVertexes);
        break;
    case TCGEN_TEXTURE:
        for (i = 0; i < tess.numVertexes; i++) {
            tess.svars._texCoords[i] = tess._texCoords[i][0];
        }
        break;
    case TCGEN_LIGHTMAP:
        for (i = 0; i < tess.numVertexes; i++) {
            tess.svars._texCoords[i] = tess._texCoords[i][1];
        }
        break;
    case TCGEN_VECTOR:
        for (i = 0; i < tess.numVertexes; i++) {
            tess.svars._texCoords[i].x = DotProduct(tess.xyz[i].data(), pStage->bundle.tcGenVectors[0]);
            tess.svars._texCoords[i].y = DotProduct(tess.xyz[i].data(), pStage->bundle.tcGenVectors[1]);
        }
        break;
    case TCGEN_FOG:
        RB_CalcFogTexCoords(tess.svars._texCoords->data());
        break;
    case TCGEN_ENVIRONMENT_MAPPED:
        RB_CalcEnvironmentTexCoords(tess.svars._texCoords->data());
        break;
    case TCGEN_BAD:
        return;
    }

    //
    // alter texture coordinates
    //
    for (tm = 0; tm < pStage->bundle.numTexMods; tm++) {
        switch (pStage->bundle.texMods[tm].type) {
        case TMOD_NONE:
            tm = TR_MAX_TEXMODS; // break out of for loop
            break;

        case TMOD_TURBULENT:
            RB_CalcTurbulentTexCoords(&pStage->bundle.texMods[tm].wave, tess.svars._texCoords->data());
            break;

        case TMOD_ENTITY_TRANSLATE:
            RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord, tess.svars._texCoords->data());
            break;

        case TMOD_SCROLL:
            RB_CalcScrollTexCoords(pStage->bundle.texMods[tm].scroll, tess.svars._texCoords->data());
            break;

        case TMOD_SCALE:
            RB_CalcScaleTexCoords(pStage->bundle.texMods[tm].scale, tess.svars._texCoords->data());
            break;

        case TMOD_STRETCH:
            RB_CalcStretchTexCoords(&pStage->bundle.texMods[tm].wave, tess.svars._texCoords->data());
            break;

        case TMOD_TRANSFORM:
            RB_CalcTransformTexCoords(&pStage->bundle.texMods[tm], tess.svars._texCoords->data());
            break;

        case TMOD_ROTATE:
            RB_CalcRotateTexCoords(pStage->bundle.texMods[tm].rotateSpeed, tess.svars._texCoords->data());
            break;

        default:
            Com_Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle.texMods[tm].type, tess.shader->name);
            break;
        }
    }
}

/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric(shaderCommands_t* input)
{
    int32 stage;

    for (stage = 0; stage < MAX_SHADER_STAGES; stage++) {
        shaderStage_t* pStage = tess.xstages[stage];

        if (!pStage) {
            break;
        }

        ComputeColors(pStage);
        ComputeTexCoords(pStage);

        if (!setArraysOnce) {
            qglEnableClientState(GL_COLOR_ARRAY);
            qglColorPointer(4, GL_UNSIGNED_BYTE, 0, input->svars.colors);
        }


        if (!setArraysOnce) {
            qglTexCoordPointer(2, GL_FLOAT, 0, input->svars._texCoords->data());
        }

        //
        // set state
        //
        // for dynamic lighting
        R_BindAnimatedImage(&pStage->bundle);

        GL_State(pStage->stateBits);

        //
        // draw
        //
        qglDrawElements(GL_TRIANGLES, input->numIndexes, GL_INDEX_TYPE, input->indexes);

        // allow skipping out to show just lightmaps during development
        if (r_lightmap->integer && (pStage->bundle.isLightmap || pStage->bundle.vertexLightmap)) {
            break;
        }
    }
}

/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric()
{
    shaderCommands_t* input;

    input = &tess;

    RB_DeformTessGeometry();

    //
    // set face culling appropriately
    //
    GL_Cull(input->shader->cullType);

    // set polygon offset if necessary
    if (input->shader->polygonOffset) {
        qglEnable(GL_POLYGON_OFFSET_FILL);
        qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
    }

    //
    // if there is only a single pass then we can enable color
    // and texture arrays before we compile, otherwise we need
    // to avoid compiling those arrays since they will change
    // during multipass rendering
    //
    if (tess.numPasses > 1) {
        setArraysOnce = false;
        qglDisableClientState(GL_COLOR_ARRAY);
        qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
    } else {
        setArraysOnce = true;

        qglEnableClientState(GL_COLOR_ARRAY);
        qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);

        qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
        qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars._texCoords->data());
    }

    //
    // lock XYZ
    //
    qglVertexPointer(3, GL_FLOAT, 16, input->xyz); // padded for SIMD

    //
    // enable color and texcoord arrays after the lock if necessary
    //
    if (!setArraysOnce) {
        qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
        qglEnableClientState(GL_COLOR_ARRAY);
    }

    //
    // call shader function
    //
    RB_IterateStagesGeneric(input);

    //
    // now do any dynamic lighting needed
    //
    if (tess.dlightBits && tess.shader->sort <= SS_OPAQUE
        && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY))) {
        ProjectDlightTexture();
    }

    //
    // now do fog
    //
    if (tess.fogNum && tess.shader->fogPass) {
        RB_FogPass();
    }

    //
    // reset polygon offset
    //
    if (input->shader->polygonOffset) {
        qglDisable(GL_POLYGON_OFFSET_FILL);
    }
}

/*
** RB_EndSurface
*/
void RB_EndSurface()
{
    shaderCommands_t* input;

    input = &tess;

    if (input->numIndexes == 0) {
        return;
    }

    if (input->indexes[SHADER_MAX_INDEXES - 1] != 0) {
        Com_Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
    }
    if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0) {
        Com_Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
    }

    //
    // call off to shader specific tess end function
    //
    tess.currentStageIteratorFunc();

    // clear shader so we can tell we don't have any unclosed surfaces
    tess.numIndexes = 0;
}

