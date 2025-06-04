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

static tavros::core::logger logger("tr_shader");

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char* s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t      shader;
static texModInfo_t  texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static bool          deferLoad;

#define FILE_HASH_SIZE 1024
static shader_t* hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH 2048
static char** shaderTextHashTable[MAX_SHADERTEXT_HASH];

/*
================
return a hash value for the filename
================
*/
static int32 generateHashValue(const char* fname, const int32 size)
{
    int32 i;
    int32 hash;
    char  letter;

    hash = 0;
    i = 0;
    while (fname[i] != '\0') {
        letter = tolower(fname[i]);
        if (letter == '.') {
            break; // don't include extension
        }
        if (letter == '\\') {
            letter = '/'; // damn path names
        }
        if (letter == PATH_SEP) {
            letter = '/'; // damn path names
        }
        hash += (int32) (letter) * (i + 119);
        i++;
    }
    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    hash &= (size - 1);
    return hash;
}

void R_RemapShader(const char* shaderName, const char* newShaderName, const char* timeOffset)
{
    char      strippedName[MAX_QPATH];
    int32     hash;
    shader_t *sh, *sh2;
    qhandle_t h;

    sh = R_FindShaderByName(shaderName);
    if (sh == NULL || sh == tr.defaultShader) {
        h = RE_RegisterShaderLightMap(shaderName, 0);
        sh = R_GetShaderByHandle(h);
    }
    if (sh == NULL || sh == tr.defaultShader) {
        logger.warning("R_RemapShader: shader %s not found", shaderName);
        return;
    }

    sh2 = R_FindShaderByName(newShaderName);
    if (sh2 == NULL || sh2 == tr.defaultShader) {
        h = RE_RegisterShaderLightMap(newShaderName, 0);
        sh2 = R_GetShaderByHandle(h);
    }

    if (sh2 == NULL || sh2 == tr.defaultShader) {
        logger.warning("R_RemapShader: new shader %s not found", newShaderName);
        return;
    }

    // remap all the shaders with the given name
    // even tho they might have different lightmaps
    COM_StripExtension(shaderName, strippedName);
    hash = generateHashValue(strippedName, FILE_HASH_SIZE);
    for (sh = hashTable[hash]; sh; sh = sh->next) {
        if (Q_stricmp(sh->name, strippedName) == 0) {
            if (sh != sh2) {
                sh->remappedShader = sh2;
            } else {
                sh->remappedShader = NULL;
            }
        }
    }
    if (timeOffset) {
        sh2->timeOffset = atof(timeOffset);
    }
}

/*
===============
ParseVector
===============
*/
static bool ParseVector(char** text, int32 count, float* v)
{
    char* token;
    int32 i;

    // FIXME: spaces are currently required after parens, should change parseext...
    token = COM_ParseExt(text, false);
    if (strcmp(token, "(")) {
        logger.warning("missing parenthesis in shader '%s'", shader.name);
        return false;
    }

    for (i = 0; i < count; i++) {
        token = COM_ParseExt(text, false);
        if (!token[0]) {
            logger.warning("missing vector element in shader '%s'", shader.name);
            return false;
        }
        v[i] = atof(token);
    }

    token = COM_ParseExt(text, false);
    if (strcmp(token, ")")) {
        logger.warning("missing parenthesis in shader '%s'", shader.name);
        return false;
    }

    return true;
}


/*
===============
NameToAFunc
===============
*/
static uint32 NameToAFunc(const char* funcname)
{
    if (!Q_stricmp(funcname, "GT0")) {
        return GLS_ATEST_GT_0;
    } else if (!Q_stricmp(funcname, "LT128")) {
        return GLS_ATEST_LT_80;
    } else if (!Q_stricmp(funcname, "GE128")) {
        return GLS_ATEST_GE_80;
    }

    logger.warning("invalid alphaFunc name '%s' in shader '%s'", funcname, shader.name);
    return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int32 NameToSrcBlendMode(const char* name)
{
    if (!Q_stricmp(name, "GL_ONE")) {
        return GLS_SRCBLEND_ONE;
    } else if (!Q_stricmp(name, "GL_ZERO")) {
        return GLS_SRCBLEND_ZERO;
    } else if (!Q_stricmp(name, "GL_DST_COLOR")) {
        return GLS_SRCBLEND_DST_COLOR;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR")) {
        return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
    } else if (!Q_stricmp(name, "GL_SRC_ALPHA")) {
        return GLS_SRCBLEND_SRC_ALPHA;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA")) {
        return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
    } else if (!Q_stricmp(name, "GL_DST_ALPHA")) {
        return GLS_SRCBLEND_DST_ALPHA;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA")) {
        return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
    } else if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE")) {
        return GLS_SRCBLEND_ALPHA_SATURATE;
    }

    logger.warning("unknown blend mode '%s' in shader '%s', substituting GL_ONE", name, shader.name);
    return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int32 NameToDstBlendMode(const char* name)
{
    if (!Q_stricmp(name, "GL_ONE")) {
        return GLS_DSTBLEND_ONE;
    } else if (!Q_stricmp(name, "GL_ZERO")) {
        return GLS_DSTBLEND_ZERO;
    } else if (!Q_stricmp(name, "GL_SRC_ALPHA")) {
        return GLS_DSTBLEND_SRC_ALPHA;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA")) {
        return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    } else if (!Q_stricmp(name, "GL_DST_ALPHA")) {
        return GLS_DSTBLEND_DST_ALPHA;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA")) {
        return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
    } else if (!Q_stricmp(name, "GL_SRC_COLOR")) {
        return GLS_DSTBLEND_SRC_COLOR;
    } else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR")) {
        return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
    }

    logger.warning("unknown blend mode '%s' in shader '%s', substituting GL_ONE", name, shader.name);
    return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc(const char* funcname)
{
    if (!Q_stricmp(funcname, "sin")) {
        return GF_SIN;
    } else if (!Q_stricmp(funcname, "square")) {
        return GF_SQUARE;
    } else if (!Q_stricmp(funcname, "triangle")) {
        return GF_TRIANGLE;
    } else if (!Q_stricmp(funcname, "sawtooth")) {
        return GF_SAWTOOTH;
    } else if (!Q_stricmp(funcname, "inversesawtooth")) {
        return GF_INVERSE_SAWTOOTH;
    } else if (!Q_stricmp(funcname, "noise")) {
        return GF_NOISE;
    }

    logger.warning("invalid genfunc name '%s' in shader '%s'", funcname, shader.name);
    return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm(char** text, waveForm_t* wave)
{
    char* token;

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing waveform parm in shader '%s'", shader.name);
        return;
    }
    wave->func = NameToGenFunc(token);

    // BASE, AMP, PHASE, FREQ
    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing waveform parm in shader '%s'", shader.name);
        return;
    }
    wave->base = atof(token);

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing waveform parm in shader '%s'", shader.name);
        return;
    }
    wave->amplitude = atof(token);

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing waveform parm in shader '%s'", shader.name);
        return;
    }
    wave->phase = atof(token);

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing waveform parm in shader '%s'", shader.name);
        return;
    }
    wave->frequency = atof(token);
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod(char* _text, shaderStage_t* stage)
{
    const char*   token;
    char**        text = &_text;
    texModInfo_t* tmi;

    if (stage->bundle.numTexMods == TR_MAX_TEXMODS) {
        Com_Error(ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name);
        return;
    }

    tmi = &stage->bundle.texMods[stage->bundle.numTexMods];
    stage->bundle.numTexMods++;

    token = COM_ParseExt(text, false);

    //
    // turb
    //
    if (!Q_stricmp(token, "turb")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing tcMod turb parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.base = atof(token);
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing tcMod turb in shader '%s'", shader.name);
            return;
        }
        tmi->wave.amplitude = atof(token);
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing tcMod turb in shader '%s'", shader.name);
            return;
        }
        tmi->wave.phase = atof(token);
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing tcMod turb in shader '%s'", shader.name);
            return;
        }
        tmi->wave.frequency = atof(token);

        tmi->type = TMOD_TURBULENT;
    }
    //
    // scale
    //
    else if (!Q_stricmp(token, "scale")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing scale parms in shader '%s'", shader.name);
            return;
        }
        tmi->scale[0] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing scale parms in shader '%s'", shader.name);
            return;
        }
        tmi->scale[1] = atof(token);
        tmi->type = TMOD_SCALE;
    }
    //
    // scroll
    //
    else if (!Q_stricmp(token, "scroll")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing scale scroll parms in shader '%s'", shader.name);
            return;
        }
        tmi->scroll[0] = atof(token);
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing scale scroll parms in shader '%s'", shader.name);
            return;
        }
        tmi->scroll[1] = atof(token);
        tmi->type = TMOD_SCROLL;
    }
    //
    // stretch
    //
    else if (!Q_stricmp(token, "stretch")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing stretch parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.func = NameToGenFunc(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing stretch parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.base = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing stretch parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.amplitude = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing stretch parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.phase = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing stretch parms in shader '%s'", shader.name);
            return;
        }
        tmi->wave.frequency = atof(token);

        tmi->type = TMOD_STRETCH;
    }
    //
    // transform
    //
    else if (!Q_stricmp(token, "transform")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->matrix[0][0] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->matrix[0][1] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->matrix[1][0] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->matrix[1][1] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->translate[0] = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing transform parms in shader '%s'", shader.name);
            return;
        }
        tmi->translate[1] = atof(token);

        tmi->type = TMOD_TRANSFORM;
    }
    //
    // rotate
    //
    else if (!Q_stricmp(token, "rotate")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing tcMod rotate parms in shader '%s'", shader.name);
            return;
        }
        tmi->rotateSpeed = atof(token);
        tmi->type = TMOD_ROTATE;
    }
    //
    // entityTranslate
    //
    else if (!Q_stricmp(token, "entityTranslate")) {
        tmi->type = TMOD_ENTITY_TRANSLATE;
    } else {
        logger.warning("unknown tcMod '%s' in shader '%s'", token, shader.name);
    }
}


/*
===================
ParseStage
===================
*/
static bool ParseStage(shaderStage_t* stage, char** text)
{
    char* token;
    int32 depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
    bool  depthMaskExplicit = false;

    stage->active = true;

    while (1) {
        token = COM_ParseExt(text, true);
        if (!token[0]) {
            logger.warning("no matching '}' found");
            return false;
        }

        if (token[0] == '}') {
            break;
        }
        //
        // map <name>
        //
        else if (!Q_stricmp(token, "map")) {
            token = COM_ParseExt(text, false);
            if (!token[0]) {
                logger.warning("missing parameter for 'map' keyword in shader '%s'", shader.name);
                return false;
            }

            if (!Q_stricmp(token, "$whiteimage")) {
                stage->bundle.image[0] = tr.whiteImage;
                continue;
            } else if (!Q_stricmp(token, "$lightmap")) {
                stage->bundle.isLightmap = true;
                if (shader.lightmapIndex < 0) {
                    stage->bundle.image[0] = tr.whiteImage;
                } else {
                    stage->bundle.image[0] = tr.lightmaps[shader.lightmapIndex];
                }
                continue;
            } else {
                stage->bundle.image[0] = R_FindImageFile(token, !shader.noMipMaps, GL_REPEAT);
                if (!stage->bundle.image[0]) {
                    logger.warning("R_FindImageFile could not find '%s' in shader '%s'", token, shader.name);
                    return false;
                }
            }
        }
        //
        // clampmap <name>
        //
        else if (!Q_stricmp(token, "clampmap")) {
            token = COM_ParseExt(text, false);
            if (!token[0]) {
                logger.warning("missing parameter for 'clampmap' keyword in shader '%s'", shader.name);
                return false;
            }

            stage->bundle.image[0] = R_FindImageFile(token, !shader.noMipMaps, GL_CLAMP);
            if (!stage->bundle.image[0]) {
                logger.warning("R_FindImageFile could not find '%s' in shader '%s'", token, shader.name);
                return false;
            }
        }
        //
        // animMap <frequency> <image1> .... <imageN>
        //
        else if (!Q_stricmp(token, "animMap")) {
            token = COM_ParseExt(text, false);
            if (!token[0]) {
                logger.warning("missing parameter for 'animMmap' keyword in shader '%s'", shader.name);
                return false;
            }
            stage->bundle.imageAnimationSpeed = atof(token);

            // parse up to MAX_IMAGE_ANIMATIONS animations
            while (1) {
                int32 num;

                token = COM_ParseExt(text, false);
                if (!token[0]) {
                    break;
                }
                num = stage->bundle.numImageAnimations;
                if (num < MAX_IMAGE_ANIMATIONS) {
                    stage->bundle.image[num] = R_FindImageFile(token, !shader.noMipMaps, GL_REPEAT);
                    if (!stage->bundle.image[num]) {
                        logger.warning("R_FindImageFile could not find '%s' in shader '%s'", token, shader.name);
                        return false;
                    }
                    stage->bundle.numImageAnimations++;
                }
            }
        }
        //
        // alphafunc <func>
        //
        else if (!Q_stricmp(token, "alphaFunc")) {
            token = COM_ParseExt(text, false);
            if (!token[0]) {
                logger.warning("missing parameter for 'alphaFunc' keyword in shader '%s'", shader.name);
                return false;
            }

            atestBits = NameToAFunc(token);
        }
        //
        // depthFunc <func>
        //
        else if (!Q_stricmp(token, "depthfunc")) {
            token = COM_ParseExt(text, false);

            if (!token[0]) {
                logger.warning("missing parameter for 'depthfunc' keyword in shader '%s'", shader.name);
                return false;
            }

            if (!Q_stricmp(token, "lequal")) {
                depthFuncBits = 0;
            } else if (!Q_stricmp(token, "equal")) {
                depthFuncBits = GLS_DEPTHFUNC_EQUAL;
            } else {
                logger.warning("unknown depthfunc '%s' in shader '%s'", token, shader.name);
                continue;
            }
        }
        //
        // detail
        //
        else if (!Q_stricmp(token, "detail")) {
            stage->isDetail = true;
        }
        //
        // blendfunc <srcFactor> <dstFactor>
        // or blendfunc <add|filter|blend>
        //
        else if (!Q_stricmp(token, "blendfunc")) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing parm for blendFunc in shader '%s'", shader.name);
                continue;
            }
            // check for "simple" blends first
            if (!Q_stricmp(token, "add")) {
                blendSrcBits = GLS_SRCBLEND_ONE;
                blendDstBits = GLS_DSTBLEND_ONE;
            } else if (!Q_stricmp(token, "filter")) {
                blendSrcBits = GLS_SRCBLEND_DST_COLOR;
                blendDstBits = GLS_DSTBLEND_ZERO;
            } else if (!Q_stricmp(token, "blend")) {
                blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
                blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
            } else {
                // complex double blends
                blendSrcBits = NameToSrcBlendMode(token);

                token = COM_ParseExt(text, false);
                if (token[0] == 0) {
                    logger.warning("missing parm for blendFunc in shader '%s'", shader.name);
                    continue;
                }
                blendDstBits = NameToDstBlendMode(token);
            }

            // clear depth mask for blended surfaces
            if (!depthMaskExplicit) {
                depthMaskBits = 0;
            }
        }
        //
        // rgbGen
        //
        else if (!Q_stricmp(token, "rgbGen")) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing parameters for rgbGen in shader '%s'", shader.name);
                continue;
            }

            if (!Q_stricmp(token, "wave")) {
                ParseWaveForm(text, &stage->rgbWave);
                stage->rgbGen = CGEN_WAVEFORM;
            } else if (!Q_stricmp(token, "const")) {
                vec3_t color;

                ParseVector(text, 3, color);
                stage->constantColor[0] = 255 * color[0];
                stage->constantColor[1] = 255 * color[1];
                stage->constantColor[2] = 255 * color[2];

                stage->rgbGen = CGEN_CONST;
            } else if (!Q_stricmp(token, "identity")) {
                stage->rgbGen = CGEN_IDENTITY;
            } else if (!Q_stricmp(token, "identityLighting")) {
                stage->rgbGen = CGEN_IDENTITY_LIGHTING;
            } else if (!Q_stricmp(token, "entity")) {
                stage->rgbGen = CGEN_ENTITY;
            } else if (!Q_stricmp(token, "oneMinusEntity")) {
                stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
            } else if (!Q_stricmp(token, "vertex")) {
                stage->rgbGen = CGEN_VERTEX;
                if (stage->alphaGen == 0) {
                    stage->alphaGen = AGEN_VERTEX;
                }
            } else if (!Q_stricmp(token, "exactVertex")) {
                stage->rgbGen = CGEN_EXACT_VERTEX;
            } else if (!Q_stricmp(token, "lightingDiffuse")) {
                stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
            } else if (!Q_stricmp(token, "oneMinusVertex")) {
                stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
            } else {
                logger.warning("unknown rgbGen parameter '%s' in shader '%s'", token, shader.name);
                continue;
            }
        }
        //
        // alphaGen
        //
        else if (!Q_stricmp(token, "alphaGen")) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing parameters for alphaGen in shader '%s'", shader.name);
                continue;
            }

            if (!Q_stricmp(token, "wave")) {
                ParseWaveForm(text, &stage->alphaWave);
                stage->alphaGen = AGEN_WAVEFORM;
            } else if (!Q_stricmp(token, "const")) {
                token = COM_ParseExt(text, false);
                stage->constantColor[3] = 255 * atof(token);
                stage->alphaGen = AGEN_CONST;
            } else if (!Q_stricmp(token, "identity")) {
                stage->alphaGen = AGEN_IDENTITY;
            } else if (!Q_stricmp(token, "entity")) {
                stage->alphaGen = AGEN_ENTITY;
            } else if (!Q_stricmp(token, "oneMinusEntity")) {
                stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
            } else if (!Q_stricmp(token, "vertex")) {
                stage->alphaGen = AGEN_VERTEX;
            } else if (!Q_stricmp(token, "lightingSpecular")) {
                stage->alphaGen = AGEN_LIGHTING_SPECULAR;
            } else if (!Q_stricmp(token, "oneMinusVertex")) {
                stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
            } else if (!Q_stricmp(token, "portal")) {
                stage->alphaGen = AGEN_PORTAL;
                token = COM_ParseExt(text, false);
                if (token[0] == 0) {
                    shader.portalRange = 256;
                    logger.warning("missing range parameter for alphaGen portal in shader '%s', defaulting to 256", shader.name);
                } else {
                    shader.portalRange = atof(token);
                }
            } else {
                logger.warning("unknown alphaGen parameter '%s' in shader '%s'", token, shader.name);
                continue;
            }
        }
        //
        // tcGen <function>
        //
        else if (!Q_stricmp(token, "texgen") || !Q_stricmp(token, "tcGen")) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing texgen parm in shader '%s'", shader.name);
                continue;
            }

            if (!Q_stricmp(token, "environment")) {
                stage->bundle.tcGen = TCGEN_ENVIRONMENT_MAPPED;
            } else if (!Q_stricmp(token, "lightmap")) {
                stage->bundle.tcGen = TCGEN_LIGHTMAP;
            } else if (!Q_stricmp(token, "texture") || !Q_stricmp(token, "base")) {
                stage->bundle.tcGen = TCGEN_TEXTURE;
            } else if (!Q_stricmp(token, "vector")) {
                ParseVector(text, 3, stage->bundle.tcGenVectors[0]);
                ParseVector(text, 3, stage->bundle.tcGenVectors[1]);

                stage->bundle.tcGen = TCGEN_VECTOR;
            } else {
                logger.warning("unknown texgen parm in shader '%s'", shader.name);
            }
        }
        //
        // tcMod <type> <...>
        //
        else if (!Q_stricmp(token, "tcMod")) {
            char buffer[1024] = "";

            while (1) {
                token = COM_ParseExt(text, false);
                if (token[0] == 0) {
                    break;
                }
                strcat(buffer, token);
                strcat(buffer, " ");
            }

            ParseTexMod(buffer, stage);

            continue;
        }
        //
        // depthmask
        //
        else if (!Q_stricmp(token, "depthwrite")) {
            depthMaskBits = GLS_DEPTHMASK_TRUE;
            depthMaskExplicit = true;

            continue;
        } else {
            logger.warning("unknown parameter '%s' in shader '%s'", token, shader.name);
            return false;
        }
    }

    //
    // if cgen isn't explicitly specified, use either identity or identitylighting
    //
    if (stage->rgbGen == CGEN_BAD) {
        if (blendSrcBits == 0 || blendSrcBits == GLS_SRCBLEND_ONE || blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) {
            stage->rgbGen = CGEN_IDENTITY_LIGHTING;
        } else {
            stage->rgbGen = CGEN_IDENTITY;
        }
    }


    //
    // implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
    //
    if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ZERO)) {
        blendDstBits = blendSrcBits = 0;
        depthMaskBits = GLS_DEPTHMASK_TRUE;
    }

    // decide which agens we can skip
    if (stage->alphaGen == CGEN_IDENTITY) {
        if (stage->rgbGen == CGEN_IDENTITY
            || stage->rgbGen == CGEN_LIGHTING_DIFFUSE) {
            stage->alphaGen = AGEN_SKIP;
        }
    }

    //
    // compute state bits
    //
    stage->stateBits = depthMaskBits | blendSrcBits | blendDstBits | atestBits | depthFuncBits;

    return true;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform(char** text)
{
    char*          token;
    deformStage_t* ds;

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing deform parm in shader '%s'", shader.name);
        return;
    }

    if (shader.numDeforms == MAX_SHADER_DEFORMS) {
        logger.warning("MAX_SHADER_DEFORMS in '%s'", shader.name);
        return;
    }

    ds = &shader.deforms[shader.numDeforms];
    shader.numDeforms++;

    if (!Q_stricmp(token, "projectionShadow")) {
        ds->deformation = DEFORM_NONE;
        return;
    }

    if (!Q_stricmp(token, "autosprite")) {
        ds->deformation = DEFORM_AUTOSPRITE;
        return;
    }

    if (!Q_stricmp(token, "autosprite2")) {
        ds->deformation = DEFORM_AUTOSPRITE2;
        return;
    }

    if (!Q_stricmpn(token, "text", 4)) {
        int32 n;

        n = token[4] - '0';
        if (n < 0 || n > 7) {
            n = 0;
        }
        ds->deformation = (deform_t) (DEFORM_TEXT0 + n);
        return;
    }

    if (!Q_stricmp(token, "bulge")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes bulge parm in shader '%s'", shader.name);
            return;
        }
        ds->bulgeWidth = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes bulge parm in shader '%s'", shader.name);
            return;
        }
        ds->bulgeHeight = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes bulge parm in shader '%s'", shader.name);
            return;
        }
        ds->bulgeSpeed = atof(token);

        ds->deformation = DEFORM_BULGE;
        return;
    }

    if (!Q_stricmp(token, "wave")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes parm in shader '%s'", shader.name);
            return;
        }

        if (atof(token) != 0) {
            ds->deformationSpread = 1.0f / atof(token);
        } else {
            ds->deformationSpread = 100.0f;
            logger.warning("illegal div value of 0 in deformVertexes command for shader '%s'", shader.name);
        }

        ParseWaveForm(text, &ds->deformationWave);
        ds->deformation = DEFORM_WAVE;
        return;
    }

    if (!Q_stricmp(token, "normal")) {
        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes parm in shader '%s'", shader.name);
            return;
        }
        ds->deformationWave.amplitude = atof(token);

        token = COM_ParseExt(text, false);
        if (token[0] == 0) {
            logger.warning("missing deformVertexes parm in shader '%s'", shader.name);
            return;
        }
        ds->deformationWave.frequency = atof(token);

        ds->deformation = DEFORM_NORMALS;
        return;
    }

    if (!Q_stricmp(token, "move")) {
        int32 i;

        for (i = 0; i < 3; i++) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing deformVertexes parm in shader '%s'", shader.name);
                return;
            }
            ds->moveVector[i] = atof(token);
        }

        ParseWaveForm(text, &ds->deformationWave);
        ds->deformation = DEFORM_MOVE;
        return;
    }

    logger.warning("unknown deformVertexes subtype '%s' found in shader '%s'", token, shader.name);
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms(char** text)
{
    char*              token;
    static const char* suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
    char               pathname[MAX_QPATH];
    int32              i;

    // outerbox
    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("'skyParms' missing parameter in shader '%s'", shader.name);
        return;
    }
    if (strcmp(token, "-")) {
        for (i = 0; i < 6; i++) {
            Com_sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);
            shader.sky.outerbox[i] = R_FindImageFile((char*) pathname, true, GL_CLAMP);
            if (!shader.sky.outerbox[i]) {
                shader.sky.outerbox[i] = tr.defaultImage;
            }
        }
    }

    // cloudheight
    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("'skyParms' missing parameter in shader '%s'", shader.name);
        return;
    }
    shader.sky.cloudHeight = atof(token);
    if (!shader.sky.cloudHeight) {
        shader.sky.cloudHeight = 512;
    }
    R_InitSkyTexCoords(shader.sky.cloudHeight);


    // innerbox
    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("'skyParms' missing parameter in shader '%s'", shader.name);
        return;
    }
    if (strcmp(token, "-")) {
        for (i = 0; i < 6; i++) {
            Com_sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);
            shader.sky.innerbox[i] = R_FindImageFile((char*) pathname, true, GL_REPEAT);
            if (!shader.sky.innerbox[i]) {
                shader.sky.innerbox[i] = tr.defaultImage;
            }
        }
    }

    shader.isSky = true;
}


/*
=================
ParseSort
=================
*/
void ParseSort(char** text)
{
    char* token;

    token = COM_ParseExt(text, false);
    if (token[0] == 0) {
        logger.warning("missing sort parameter in shader '%s'", shader.name);
        return;
    }

    if (!Q_stricmp(token, "portal")) {
        shader.sort = SS_PORTAL;
    } else if (!Q_stricmp(token, "sky")) {
        shader.sort = SS_ENVIRONMENT;
    } else if (!Q_stricmp(token, "opaque")) {
        shader.sort = SS_OPAQUE;
    } else if (!Q_stricmp(token, "decal")) {
        shader.sort = SS_DECAL;
    } else if (!Q_stricmp(token, "seeThrough")) {
        shader.sort = SS_SEE_THROUGH;
    } else if (!Q_stricmp(token, "banner")) {
        shader.sort = SS_BANNER;
    } else if (!Q_stricmp(token, "additive")) {
        shader.sort = SS_BLEND1;
    } else if (!Q_stricmp(token, "nearest")) {
        shader.sort = SS_NEAREST;
    } else if (!Q_stricmp(token, "underwater")) {
        shader.sort = SS_UNDERWATER;
    } else {
        shader.sort = atof(token);
    }
}


// this table is also present in q3map

typedef struct
{
    const char* name;
    int32       clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t infoParms[] = {
    // server relevant contents
    {"water", 1, 0, CONTENTS_WATER},
    {"slime", 1, 0, CONTENTS_SLIME}, // mildly damaging
    {"lava", 1, 0, CONTENTS_LAVA},   // very damaging
    {"playerclip", 1, 0, CONTENTS_PLAYERCLIP},
    {"monsterclip", 1, 0, CONTENTS_MONSTERCLIP},
    {"nodrop", 1, 0, CONTENTS_NODROP}, // don't drop items or leave bodies (death fog, lava, etc)
    {"nonsolid", 1, SURF_NONSOLID, 0}, // clears the solid flag

    // utility relevant attributes
    {"origin", 1, 0, CONTENTS_ORIGIN},               // center of rotating brushes
    {"trans", 0, 0, CONTENTS_TRANSLUCENT},           // don't eat contained surfaces
    {"detail", 0, 0, CONTENTS_DETAIL},               // don't include in structural bsp
    {"structural", 0, 0, CONTENTS_STRUCTURAL},       // force into structural bsp even if trnas
    {"areaportal", 1, 0, CONTENTS_AREAPORTAL},       // divides areas
    {"clusterportal", 1, 0, CONTENTS_CLUSTERPORTAL}, // for bots
    {"donotenter", 1, 0, CONTENTS_DONOTENTER},       // for bots

    {"fog", 1, 0, CONTENTS_FOG},                     // carves surfaces entering
    {"sky", 0, SURF_SKY, 0},                         // emit light from an environment map
    {"lightfilter", 0, SURF_LIGHTFILTER, 0},         // filter light going through it
    {"alphashadow", 0, SURF_ALPHASHADOW, 0},         // test light on a per-pixel basis
    {"hint", 0, SURF_HINT, 0},                       // use as a primary splitter

    // server attributes
    {"slick", 0, SURF_SLICK, 0},
    {"noimpact", 0, SURF_NOIMPACT, 0}, // don't make impact explosions or marks
    {"nomarks", 0, SURF_NOMARKS, 0},   // don't make impact marks, but still explode
    {"ladder", 0, SURF_LADDER, 0},
    {"nodamage", 0, SURF_NODAMAGE, 0},
    {"metalsteps", 0, SURF_METALSTEPS, 0},
    {"flesh", 0, SURF_FLESH, 0},
    {"nosteps", 0, SURF_NOSTEPS, 0},

    // drawsurf attributes
    {"nodraw", 0, SURF_NODRAW, 0},         // don't generate a drawsurface (or a lightmap)
    {"pointlight", 0, SURF_POINTLIGHT, 0}, // sample lighting at vertexes
    {"nolightmap", 0, SURF_NOLIGHTMAP, 0}, // don't generate a lightmap
    {"nodlight", 0, SURF_NODLIGHT, 0},     // don't ever add dynamic lights
    {"dust", 0, SURF_DUST, 0}              // leave a dust trail when walking on this surface
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm(char** text)
{
    char* token;
    int32 numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);
    int32 i;

    token = COM_ParseExt(text, false);
    for (i = 0; i < numInfoParms; i++) {
        if (!Q_stricmp(token, infoParms[i].name)) {
            shader.surfaceFlags |= infoParms[i].surfaceFlags;
            shader.contentFlags |= infoParms[i].contents;
            break;
        }
    }
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static bool ParseShader(char** text)
{
    char* token;
    int32 s;

    s = 0;

    token = COM_ParseExt(text, true);
    if (token[0] != '{') {
        logger.warning("expecting '{', found '%s' instead in shader '%s'", token, shader.name);
        return false;
    }

    while (1) {
        token = COM_ParseExt(text, true);
        if (!token[0]) {
            logger.warning("no concluding '}' in shader %s", shader.name);
            return false;
        }

        // end of shader definition
        if (token[0] == '}') {
            break;
        }
        // stage definition
        else if (token[0] == '{') {
            if (!ParseStage(&stages[s], text)) {
                return false;
            }
            stages[s].active = true;
            s++;
            continue;
        }
        // skip stuff that only the QuakeEdRadient needs
        else if (!Q_stricmpn(token, "qer", 3)) {
            SkipRestOfLine(text);
            continue;
        }
        // sun parms
        else if (!Q_stricmp(token, "q3map_sun")) {
            float a, b;

            token = COM_ParseExt(text, false);
            tr.sunLight[0] = atof(token);
            token = COM_ParseExt(text, false);
            tr.sunLight[1] = atof(token);
            token = COM_ParseExt(text, false);
            tr.sunLight[2] = atof(token);

            VectorNormalize(tr.sunLight);

            token = COM_ParseExt(text, false);
            a = atof(token);
            VectorScale(tr.sunLight, a, tr.sunLight);

            token = COM_ParseExt(text, false);
            a = atof(token);
            a = a / 180 * M_PI;

            token = COM_ParseExt(text, false);
            b = atof(token);
            b = b / 180 * M_PI;

            tr.sunDirection[0] = cos(a) * cos(b);
            tr.sunDirection[1] = sin(a) * cos(b);
            tr.sunDirection[2] = sin(b);
        } else if (!Q_stricmp(token, "deformVertexes")) {
            ParseDeform(text);
            continue;
        } else if (!Q_stricmp(token, "tesssize")) {
            SkipRestOfLine(text);
            continue;
        } else if (!Q_stricmp(token, "clampTime")) {
            token = COM_ParseExt(text, false);
            if (token[0]) {
                shader.clampTime = atof(token);
            }
        }
        // skip stuff that only the q3map needs
        else if (!Q_stricmpn(token, "q3map", 5)) {
            SkipRestOfLine(text);
            continue;
        }
        // skip stuff that only q3map or the server needs
        else if (!Q_stricmp(token, "surfaceParm")) {
            ParseSurfaceParm(text);
            continue;
        }
        // no mip maps
        else if (!Q_stricmp(token, "nomipmaps")) {
            shader.noMipMaps = true;
            continue;
        }
        // no picmip adjustment
        else if (!Q_stricmp(token, "nopicmip")) {
            continue;
        }
        // polygonOffset
        else if (!Q_stricmp(token, "polygonOffset")) {
            shader.polygonOffset = true;
            continue;
        }
        // entityMergable, allowing sprite surfaces from multiple entities
        // to be merged into one batch.  This is a savings for smoke
        // puffs and blood, but can't be used for anything where the
        // shader calcs (not the surface function) reference the entity color or scroll
        else if (!Q_stricmp(token, "entityMergable")) {
            shader.entityMergable = true;
            continue;
        }
        // fogParms
        else if (!Q_stricmp(token, "fogParms")) {
            if (!ParseVector(text, 3, shader.fogParms.color)) {
                return false;
            }

            token = COM_ParseExt(text, false);
            if (!token[0]) {
                logger.warning("missing parm for 'fogParms' keyword in shader '%s'", shader.name);
                continue;
            }
            shader.fogParms.depthForOpaque = atof(token);

            // skip any old gradient directions
            SkipRestOfLine(text);
            continue;
        }
        // portal
        else if (!Q_stricmp(token, "portal")) {
            shader.sort = SS_PORTAL;
            continue;
        }
        // skyparms <cloudheight> <outerbox> <innerbox>
        else if (!Q_stricmp(token, "skyparms")) {
            ParseSkyParms(text);
            continue;
        }
        // light <value> determines flaring in q3map, not needed here
        else if (!Q_stricmp(token, "light")) {
            token = COM_ParseExt(text, false);
            continue;
        }
        // cull <face>
        else if (!Q_stricmp(token, "cull")) {
            token = COM_ParseExt(text, false);
            if (token[0] == 0) {
                logger.warning("missing cull parms in shader '%s'", shader.name);
                continue;
            }

            if (!Q_stricmp(token, "none") || !Q_stricmp(token, "twosided") || !Q_stricmp(token, "disable")) {
                shader.cullType = CT_TWO_SIDED;
            } else if (!Q_stricmp(token, "back") || !Q_stricmp(token, "backside") || !Q_stricmp(token, "backsided")) {
                shader.cullType = CT_BACK_SIDED;
            } else {
                logger.warning("invalid cull parm '%s' in shader '%s'", token, shader.name);
            }
            continue;
        }
        // sort
        else if (!Q_stricmp(token, "sort")) {
            ParseSort(text);
            continue;
        } else {
            logger.warning("unknown general shader parameter '%s' in '%s'", token, shader.name);
            return false;
        }
    }

    //
    // ignore shaders that don't have any stages, unless it is a sky or fog
    //
    if (s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG)) {
        return false;
    }

    shader.explicitlyDefined = true;

    return true;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc()
{
    shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

    //
    // see if this should go into the sky path
    //
    if (shader.isSky) {
        shader.optimalStageIteratorFunc = RB_StageIteratorSky;
    }
}

/*
=============
FixRenderCommandList
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList(int32 newShader)
{
    renderCommandList_t* cmdList = &backEndData->commands;

    if (cmdList) {
        const void* curCmd = cmdList->cmds;

        while (1) {
            switch (*(const int32*) curCmd) {
            case RC_SET_COLOR: {
                const setColorCommand_t* sc_cmd = (const setColorCommand_t*) curCmd;
                curCmd = (const void*) (sc_cmd + 1);
                break;
            }
            case RC_STRETCH_PIC: {
                const stretchPicCommand_t* sp_cmd = (const stretchPicCommand_t*) curCmd;
                curCmd = (const void*) (sp_cmd + 1);
                break;
            }
            case RC_DRAW_SURFS: {
                int32                     i;
                drawSurf_t*               drawSurf;
                shader_t*                 shader;
                int32                     fogNum;
                int32                     entityNum;
                int32                     dlightMap;
                int32                     sortedIndex;
                const drawSurfsCommand_t* ds_cmd = (const drawSurfsCommand_t*) curCmd;

                for (i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++) {
                    R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap);
                    sortedIndex = ((drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1));
                    if (sortedIndex >= newShader) {
                        sortedIndex++;
                        drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | (fogNum << QSORT_FOGNUM_SHIFT) | (int32) dlightMap;
                    }
                }
                curCmd = (const void*) (ds_cmd + 1);
                break;
            }
            case RC_DRAW_BUFFER: {
                const drawBufferCommand_t* db_cmd = (const drawBufferCommand_t*) curCmd;
                curCmd = (const void*) (db_cmd + 1);
                break;
            }
            case RC_SWAP_BUFFERS: {
                const swapBuffersCommand_t* sb_cmd = (const swapBuffersCommand_t*) curCmd;
                curCmd = (const void*) (sb_cmd + 1);
                break;
            }
            case RC_END_OF_LIST:
            default:
                return;
            }
        }
    }
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader()
{
    int32     i;
    float     sort;
    shader_t* newShader;

    newShader = tr.shaders[tr.numShaders - 1];
    sort = newShader->sort;

    for (i = tr.numShaders - 2; i >= 0; i--) {
        if (tr.sortedShaders[i]->sort <= sort) {
            break;
        }
        tr.sortedShaders[i + 1] = tr.sortedShaders[i];
        tr.sortedShaders[i + 1]->sortedIndex++;
    }

    FixRenderCommandList(i + 1);

    newShader->sortedIndex = i + 1;
    tr.sortedShaders[i + 1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t* GeneratePermanentShader()
{
    shader_t* newShader;
    int32     i;
    int32     size, hash;

    if (tr.numShaders == MAX_SHADERS) {
        logger.warning("GeneratePermanentShader - MAX_SHADERS hit");
        return tr.defaultShader;
    }

    newShader = (shader_t*) Hunk_Alloc(sizeof(shader_t), h_low);

    *newShader = shader;

    if (shader.sort <= SS_OPAQUE) {
        newShader->fogPass = FP_EQUAL;
    } else if (shader.contentFlags & CONTENTS_FOG) {
        newShader->fogPass = FP_LE;
    }

    tr.shaders[tr.numShaders] = newShader;
    newShader->index = tr.numShaders;

    tr.sortedShaders[tr.numShaders] = newShader;
    newShader->sortedIndex = tr.numShaders;

    tr.numShaders++;

    for (i = 0; i < newShader->numUnfoggedPasses; i++) {
        if (!stages[i].active) {
            break;
        }
        newShader->stages[i] = (shaderStage_t*) Hunk_Alloc(sizeof(stages[i]), h_low);
        *newShader->stages[i] = stages[i];

        size = newShader->stages[i]->bundle.numTexMods * sizeof(texModInfo_t);
        newShader->stages[i]->bundle.texMods = (texModInfo_t*) Hunk_Alloc(size, h_low);
        Com_Memcpy(newShader->stages[i]->bundle.texMods, stages[i].bundle.texMods, size);
    }

    SortNewShader();

    hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
    newShader->next = hashTable[hash];
    hashTable[hash] = newShader;

    return newShader;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t* FinishShader()
{
    int32 stage;
    bool  hasLightmapStage;
    bool  vertexLightmap;

    hasLightmapStage = false;
    vertexLightmap = false;

    //
    // set sky stuff appropriate
    //
    if (shader.isSky) {
        shader.sort = SS_ENVIRONMENT;
    }

    //
    // set polygon offset
    //
    if (shader.polygonOffset && !shader.sort) {
        shader.sort = SS_DECAL;
    }

    //
    // set appropriate stage information
    //
    for (stage = 0; stage < MAX_SHADER_STAGES; stage++) {
        shaderStage_t* pStage = &stages[stage];

        if (!pStage->active) {
            break;
        }

        // check for a missing texture
        if (!pStage->bundle.image[0]) {
            logger.warning("Shader %s has a stage with no image", shader.name);
            pStage->active = false;
            continue;
        }

        //
        // default texture coordinate generation
        //
        if (pStage->bundle.isLightmap) {
            if (pStage->bundle.tcGen == TCGEN_BAD) {
                pStage->bundle.tcGen = TCGEN_LIGHTMAP;
            }
            hasLightmapStage = true;
        } else {
            if (pStage->bundle.tcGen == TCGEN_BAD) {
                pStage->bundle.tcGen = TCGEN_TEXTURE;
            }
        }


        // not a true lightmap but we want to leave existing
        // behaviour in place and not print out a warning
        // if (pStage->rgbGen == CGEN_VERTEX) {
        //  vertexLightmap = true;
        //}


        //
        // determine sort order and fog color adjustment
        //
        if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) && (stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))) {
            int32 blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
            int32 blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

            // fog color adjustment only works for blend modes that have a contribution
            // that aproaches 0 as the modulate values aproach 0 --
            // GL_ONE, GL_ONE
            // GL_ZERO, GL_ONE_MINUS_SRC_COLOR
            // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

            // modulate, additive
            if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) || ((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR))) {
                pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
            }
            // strict blend
            else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)) {
                pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
            }
            // premultiplied alpha
            else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)) {
                pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
            } else {
                // we can't adjust this one correctly, so it won't be exactly correct in fog
            }

            // don't screw with sort order if this is a portal or environment
            if (!shader.sort) {
                // see through item, like a grill or grate
                if (pStage->stateBits & GLS_DEPTHMASK_TRUE) {
                    shader.sort = SS_SEE_THROUGH;
                } else {
                    shader.sort = SS_BLEND0;
                }
            }
        }
    }

    // there are times when you will need to manually apply a sort to
    // opaque alpha tested shaders that have later blend passes
    if (!shader.sort) {
        shader.sort = SS_OPAQUE;
    }

    if (shader.lightmapIndex >= 0 && !hasLightmapStage) {
        if (vertexLightmap) {
            logger.debug("WARNING: shader '%s' has VERTEX forced lightmap!", shader.name);
        } else {
            logger.debug("WARNING: shader '%s' has lightmap but no lightmap stage!", shader.name);
            shader.lightmapIndex = LIGHTMAP_NONE;
        }
    }


    //
    // compute number of passes
    //
    shader.numUnfoggedPasses = stage;

    // fogonly shaders don't have any normal passes
    if (stage == 0) {
        shader.sort = SS_FOG;
    }

    // determine which stage iterator function is appropriate
    ComputeStageIteratorFunc();

    return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char* FindShaderInShaderText(const char* shadername)
{
    char *token, *p;

    int32 i, hash;

    hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

    for (i = 0; shaderTextHashTable[hash][i]; i++) {
        p = shaderTextHashTable[hash][i];
        token = COM_ParseExt(&p, true);
        if (!Q_stricmp(token, shadername)) {
            return p;
        }
    }

    p = s_shaderText;

    if (!p) {
        return NULL;
    }

    // look for label
    while (1) {
        token = COM_ParseExt(&p, true);
        if (token[0] == 0) {
            break;
        }

        if (!Q_stricmp(token, shadername)) {
            return p;
        } else {
            // skip the definition
            SkipBracedSection(&p);
        }
    }

    return NULL;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t* R_FindShaderByName(const char* name)
{
    char      strippedName[MAX_QPATH];
    int32     hash;
    shader_t* sh;

    if ((name == NULL) || (name[0] == 0)) { // bk001205
        return tr.defaultShader;
    }

    COM_StripExtension(name, strippedName);

    hash = generateHashValue(strippedName, FILE_HASH_SIZE);

    //
    // see if the shader is already loaded
    //
    for (sh = hashTable[hash]; sh; sh = sh->next) {
        // NOTE: if there was no shader or image available with the name strippedName
        // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
        // have to check all default shaders otherwise for every call to R_FindShader
        // with that same strippedName a new default shader is created.
        if (Q_stricmp(sh->name, strippedName) == 0) {
            // match found
            return sh;
        }
    }

    return tr.defaultShader;
}


/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
shader_t* R_FindShader(const char* name, int32 lightmapIndex, bool mipRawImage)
{
    char         strippedName[MAX_QPATH];
    char         fileName[MAX_QPATH];
    int32        i, hash;
    char*        shaderText;
    texture_type image;
    shader_t*    sh;

    if (name[0] == 0) {
        return tr.defaultShader;
    }

    // use (fullbright) vertex lighting if the bsp file doesn't have
    // lightmaps
    if (lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps) {
        lightmapIndex = LIGHTMAP_BY_VERTEX;
    }

    COM_StripExtension(name, strippedName);

    hash = generateHashValue(strippedName, FILE_HASH_SIZE);

    //
    // see if the shader is already loaded
    //
    for (sh = hashTable[hash]; sh; sh = sh->next) {
        // NOTE: if there was no shader or image available with the name strippedName
        // then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
        // have to check all default shaders otherwise for every call to R_FindShader
        // with that same strippedName a new default shader is created.
        if ((sh->lightmapIndex == lightmapIndex || sh->defaultShader) && !Q_stricmp(sh->name, strippedName)) {
            // match found
            return sh;
        }
    }

    // clear the global shader
    Com_Memset(&shader, 0, sizeof(shader));
    Com_Memset(&stages, 0, sizeof(stages));
    Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
    shader.lightmapIndex = lightmapIndex;
    for (i = 0; i < MAX_SHADER_STAGES; i++) {
        stages[i].bundle.texMods = texMods[i];
    }

    // FIXME: set these "need" values apropriately
    shader.needsNormal = true;
    shader.needsST1 = true;
    shader.needsST2 = true;
    shader.needsColor = true;

    //
    // attempt to define shader from an explicit parameter file
    //
    shaderText = FindShaderInShaderText(strippedName);
    if (shaderText) {
        // enable this when building a pak file to get a global list
        // of all explicit shaders
        logger.info("*SHADER* %s", name);

        if (!ParseShader(&shaderText)) {
            // had errors, so use default shader
            shader.defaultShader = true;
        }
        sh = FinishShader();
        return sh;
    }


    //
    // if not defined in the in-memory shader descriptions,
    // look for a single TGA, BMP, or PCX
    //
    Q_strncpyz(fileName, name, sizeof(fileName));
    COM_DefaultExtension(fileName, sizeof(fileName), ".tga");
    image = R_FindImageFile(fileName, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP);
    if (!image) {
        logger.debug("Couldn't find image for shader %s", name);
        shader.defaultShader = true;
        return FinishShader();
    }

    //
    // create the default shading commands
    //
    if (shader.lightmapIndex == LIGHTMAP_NONE) {
        // dynamic colors at vertexes
        stages[0].bundle.image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
        stages[0].stateBits = GLS_DEFAULT;
    } else if (shader.lightmapIndex == LIGHTMAP_BY_VERTEX) {
        // explicit colors at vertexes
        stages[0].bundle.image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_EXACT_VERTEX;
        stages[0].alphaGen = AGEN_SKIP;
        stages[0].stateBits = GLS_DEFAULT;
    } else if (shader.lightmapIndex == LIGHTMAP_2D) {
        // GUI elements
        stages[0].bundle.image[0] = image;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_VERTEX;
        stages[0].alphaGen = AGEN_VERTEX;
        stages[0].stateBits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
    } else if (shader.lightmapIndex == LIGHTMAP_WHITEIMAGE) {
        // fullbright level
        stages[0].bundle.image[0] = tr.whiteImage;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
        stages[0].stateBits = GLS_DEFAULT;

        stages[1].bundle.image[0] = image;
        stages[1].active = true;
        stages[1].rgbGen = CGEN_IDENTITY;
        stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
    } else {
        // two pass lightmap
        stages[0].bundle.image[0] = tr.lightmaps[shader.lightmapIndex];
        stages[0].bundle.isLightmap = true;
        stages[0].active = true;
        stages[0].rgbGen = CGEN_IDENTITY; // lightmaps are scaled on creation
                                          // for identitylight
        stages[0].stateBits = GLS_DEFAULT;

        stages[1].bundle.image[0] = image;
        stages[1].active = true;
        stages[1].rgbGen = CGEN_IDENTITY;
        stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
    }

    return FinishShader();
}

/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap(const char* name, int32 lightmapIndex)
{
    shader_t* sh;

    if (strlen(name) >= MAX_QPATH) {
        logger.info("Shader name exceeds MAX_QPATH");
        return 0;
    }

    sh = R_FindShader(name, lightmapIndex, true);

    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if (sh->defaultShader) {
        return 0;
    }

    return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader(const char* name)
{
    shader_t* sh;

    if (strlen(name) >= MAX_QPATH) {
        logger.info("Shader name exceeds MAX_QPATH");
        return 0;
    }

    sh = R_FindShader(name, LIGHTMAP_2D, true);

    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if (sh->defaultShader) {
        return 0;
    }

    return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip(const char* name)
{
    shader_t* sh;

    if (strlen(name) >= MAX_QPATH) {
        logger.info("Shader name exceeds MAX_QPATH");
        return 0;
    }

    sh = R_FindShader(name, LIGHTMAP_2D, false);

    // we want to return 0 if the shader failed to
    // load for some reason, but R_FindShader should
    // still keep a name allocated for it, so if
    // something calls RE_RegisterShader again with
    // the same name, we don't try looking for it again
    if (sh->defaultShader) {
        return 0;
    }

    return sh->index;
}


/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t* R_GetShaderByHandle(qhandle_t hShader)
{
    if (hShader < 0) {
        logger.warning("R_GetShaderByHandle: out of range hShader '%d'", hShader); // bk: FIXME name
        return tr.defaultShader;
    }
    if (hShader >= tr.numShaders) {
        logger.warning("R_GetShaderByHandle: out of range hShader '%d'", hShader);
        return tr.defaultShader;
    }
    return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void R_ShaderList_f()
{
    int32     i;
    int32     count;
    shader_t* shader;

    logger.info("-----------------------");

    count = 0;
    for (i = 0; i < tr.numShaders; i++) {
        if (Cmd_Argc() > 1) {
            shader = tr.sortedShaders[i];
        } else {
            shader = tr.shaders[i];
        }

        logger.info("%i ", shader->numUnfoggedPasses);

        if (shader->lightmapIndex >= 0) {
            logger.info("L ");
        } else {
            logger.info("  ");
        }

        if (shader->explicitlyDefined) {
            logger.info("E ");
        } else {
            logger.info("  ");
        }

        if (shader->optimalStageIteratorFunc == RB_StageIteratorGeneric) {
            logger.info("gen ");
        } else if (shader->optimalStageIteratorFunc == RB_StageIteratorSky) {
            logger.info("sky ");
        } else {
            logger.info("    ");
        }

        if (shader->defaultShader) {
            logger.info(": %s (DEFAULTED)", shader->name);
        } else {
            logger.info(": %s", shader->name);
        }
        count++;
    }
    logger.info("%i total shaders", count);
    logger.info("------------------");
}


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define MAX_SHADER_FILES 4096
static void ScanAndLoadShaderFiles()
{
    char** shaderFiles;
    char*  buffers[MAX_SHADER_FILES];
    char*  p;
    int32  numShaders;
    int32  i;
    char * oldp, *token, *hashMem;
    int32  shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;

    int32 sum = 0;
    // scan for shader files
    shaderFiles = FS_ListFiles("scripts", ".shader", &numShaders);

    if (!shaderFiles || !numShaders) {
        logger.warning("no shader files found");
        return;
    }

    if (numShaders > MAX_SHADER_FILES) {
        numShaders = MAX_SHADER_FILES;
    }

    // load and parse shader files
    for (i = 0; i < numShaders; i++) {
        char filename[MAX_QPATH];

        Com_sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
        logger.info("...loading '%s'", filename);
        sum += FS_ReadFile(filename, (void**) &buffers[i]);
        if (!buffers[i]) {
            Com_Error(ERR_DROP, "Couldn't load %s", filename);
        }
    }

    // build single large buffer
    s_shaderText = (char*) Hunk_Alloc(sum + numShaders * 2, h_low);

    // free in reverse order, so the temp files are all dumped
    for (i = numShaders - 1; i >= 0; i--) {
        strcat(s_shaderText, "\n");
        p = &s_shaderText[strlen(s_shaderText)];
        strcat(s_shaderText, buffers[i]);
        FS_FreeFile(buffers[i]);
        buffers[i] = p;
        COM_Compress(p);
    }

    // free up memory
    FS_FreeFileList(shaderFiles);

    Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
    size = 0;
    //
    for (i = 0; i < numShaders; i++) {
        // pointer to the first shader file
        p = buffers[i];
        // look for label
        while (1) {
            token = COM_ParseExt(&p, true);
            if (token[0] == 0) {
                break;
            }

            hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
            shaderTextHashTableSizes[hash]++;
            size++;
            SkipBracedSection(&p);
            // if we passed the pointer to the next shader file
            if (i < numShaders - 1) {
                if (p > buffers[i + 1]) {
                    break;
                }
            }
        }
    }

    size += MAX_SHADERTEXT_HASH;

    hashMem = (char*) Hunk_Alloc(size * sizeof(char*), h_low);

    for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
        shaderTextHashTable[i] = (char**) hashMem;
        hashMem = ((char*) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char*));
    }

    Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
    //
    for (i = 0; i < numShaders; i++) {
        // pointer to the first shader file
        p = buffers[i];
        // look for label
        while (1) {
            oldp = p;
            token = COM_ParseExt(&p, true);
            if (token[0] == 0) {
                break;
            }

            hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
            shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

            SkipBracedSection(&p);
            // if we passed the pointer to the next shader file
            if (i < numShaders - 1) {
                if (p > buffers[i + 1]) {
                    break;
                }
            }
        }
    }

    return;
}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders()
{
    tr.numShaders = 0;

    // init the default shader
    Com_Memset(&shader, 0, sizeof(shader));
    Com_Memset(&stages, 0, sizeof(stages));

    Q_strncpyz(shader.name, "<default>", sizeof(shader.name));

    shader.lightmapIndex = LIGHTMAP_NONE;
    stages[0].bundle.image[0] = tr.defaultImage;
    stages[0].active = true;
    stages[0].stateBits = GLS_DEFAULT;
    tr.defaultShader = FinishShader();

    // shadow shader is just a marker
    Q_strncpyz(shader.name, "<stencil shadow>", sizeof(shader.name));
    shader.sort = SS_BLEND0;
}

static void CreateExternalShaders()
{
    tr.sunShader = R_FindShader("sun", LIGHTMAP_NONE, true);
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders()
{
    logger.info("Initializing Shaders");
    Com_Memset(hashTable, 0, sizeof(hashTable));
    deferLoad = false;
    CreateInternalShaders();
    ScanAndLoadShaderFiles();
    CreateExternalShaders();
}
