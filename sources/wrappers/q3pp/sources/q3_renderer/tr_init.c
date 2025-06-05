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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

static tavros::core::logger logger("tr_init");

#include <stb/stb_image_write.h>

#include <glad/glad.h>

glconfig_t glConfig;
glstate_t  glState;

static void GfxInfo_f();
static void GL_SetDefaultState();

cvar_t* r_novis;
cvar_t* r_nocurves;

cvar_t* r_lightmap;
cvar_t* r_offsetFactor;
cvar_t* r_offsetUnits;
cvar_t* r_lockpvs;

cvar_t* r_subdivisions;
cvar_t* r_lodCurveError;

cvar_t* r_ambientScale;
cvar_t* r_directedScale;

static bool glIsInitialized = false;
/*
** InitOpenGL
*/
static void InitOpenGL()
{
    if (!glIsInitialized) {
        GLimp_Init();

        int32 tex_size = qglGetInteger(GL_MAX_TEXTURE_SIZE);
        if (tex_size < 2048) {
            logger.error("Small texture size: %d", tex_size);
        }
        glIsInitialized = true;
    }

    // print info
    GfxInfo_f();

    // set default state
    GL_SetDefaultState();
}

/*
==============================================================================

                        SCREEN SHOTS

some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir

command: "screenshot"
we use statics to store a count and start writing the first screenshot/screenshot????.jpg available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_TakeScreenshotJPEG
==================
*/
void RB_TakeScreenshotJPEG(int32 x, int32 y, int32 width, int32 height, char* fileName)
{
    uint8* buffer;

    buffer = (uint8*) Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight * 4);

    qglReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    FS_WriteFile(fileName, buffer, 1); // create path

    stbi_write_jpg(fileName, glConfig.vidWidth, glConfig.vidHeight, 3, buffer, glConfig.vidWidth * 3);

    Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void* RB_TakeScreenshotCmd(const void* data)
{
    const screenshotCommand_t* cmd;

    cmd = (const screenshotCommand_t*) data;

    RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

    return (const void*) (cmd + 1);
}

/*
==================
R_TakeScreenshotJPEG
==================
*/
void R_TakeScreenshotJPEG(int32 x, int32 y, int32 width, int32 height, char* name)
{
    static char          fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
    screenshotCommand_t* cmd;

    cmd = (screenshotCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_SCREENSHOT;

    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    Q_strncpyz(fileName, name, sizeof(fileName));
    cmd->fileName = fileName;
}

void R_ScreenShotJPEG_f()
{
    char name[MAX_OSPATH];
    bool silent;

    if (!strcmp(Cmd_Argv(1), "silent")) {
        silent = true;
    } else {
        silent = false;
    }

    if (Cmd_Argc() == 2 && !silent) {
        // explicit filename
        Com_sprintf(name, MAX_OSPATH, "screenshots/%s.jpg", Cmd_Argv(1));
    } else {
        time_t    timestamp = time(NULL);
        struct tm datetime = *localtime(&timestamp);
        strftime(name, sizeof(name), "screenshots/%Y%m%d_%H%M%S", &datetime);
    }

    R_TakeScreenshotJPEG(0, 0, glConfig.vidWidth, glConfig.vidHeight, name);

    if (!silent) {
        logger.info("Wrote %s", name);
    }
}

//============================================================================

/* GL_SetDefaultState */
static void GL_SetDefaultState()
{
    qglClearDepth(1.0f);

    qglCullFace(GL_FRONT);

    qglColor4f(1, 1, 1, 1);

    qglEnable(GL_TEXTURE_2D);
    GL_TexEnv(GL_MODULATE);

    qglShadeModel(GL_SMOOTH);
    qglDepthFunc(GL_LEQUAL);

    // the vertex array is always enabled, but the color and texture
    // arrays are enabled and disabled around the compiled vertex array call
    qglEnableClientState(GL_VERTEX_ARRAY);

    //
    // make sure our GL state vector is set correctly
    //
    glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

    qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    qglDepthMask(GL_TRUE);
    qglDisable(GL_DEPTH_TEST);
    qglEnable(GL_SCISSOR_TEST);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_BLEND);
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f()
{
    logger.info("GL_VENDOR: %s", qglGetString(GL_VENDOR));
    logger.info("GL_RENDERER: %s", qglGetString(GL_RENDERER));
    logger.info("GL_VERSION: %s", qglGetString(GL_VERSION));
    logger.info("GL_MAX_TEXTURE_SIZE: %d", qglGetInteger(GL_MAX_TEXTURE_SIZE));
}

/*
===============
R_Register
===============
*/
static void R_Register()
{
    r_subdivisions = Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

    //
    // archived variables that can change at any time
    //
    r_lodCurveError = Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT);

    r_ambientScale = Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
    r_directedScale = Cvar_Get("r_directedScale", "1", CVAR_CHEAT);

    r_nocurves = Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
    r_lightmap = Cvar_Get("r_lightmap", "0", CVAR_CHEAT);

    r_novis = Cvar_Get("r_novis", "0", CVAR_CHEAT);
    r_offsetFactor = Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
    r_offsetUnits = Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
    r_lockpvs = Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);

    // make sure all the commands added here are also
    // removed in R_Shutdown
    Cmd_AddCommand("shaderlist", R_ShaderList_f);
    Cmd_AddCommand("modellist", R_Modellist_f);
    Cmd_AddCommand("screenshot", R_ScreenShotJPEG_f);
    Cmd_AddCommand("gfxinfo", GfxInfo_f);
}

/*
===============
R_Init
===============
*/
void R_Init()
{
    int32  err;
    int32  i;
    uint8* ptr;

    logger.info("----- R_Init -----");

    // clear all our internal state
    Com_Memset(&tr, 0, sizeof(tr));
    Com_Memset(&backEnd, 0, sizeof(backEnd));
    Com_Memset(&tess, 0, sizeof(tess));

    //    Swap_Init();

    if ((int32) tess.xyz & 15) {
        logger.warning("tess.xyz not 16 uint8 aligned");
    }
    Com_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

    //
    // init function tables
    //
    for (i = 0; i < FUNCTABLE_SIZE; i++) {
        tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float) (FUNCTABLE_SIZE - 1))));
        tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
        tr.sawToothTable[i] = (float) i / FUNCTABLE_SIZE;
        tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

        if (i < FUNCTABLE_SIZE / 2) {
            if (i < FUNCTABLE_SIZE / 4) {
                tr.triangleTable[i] = (float) i / (FUNCTABLE_SIZE / 4);
            } else {
                tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
            }
        } else {
            tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
        }
    }

    R_InitFogTable();

    R_NoiseInit();

    R_Register();

    ptr = (uint8*) Hunk_Alloc(sizeof(backEndData_t) + sizeof(srfPoly_t) * MAX_POLYS + sizeof(polyVert_t) * MAX_POLYVERTS, h_low);
    backEndData = (backEndData_t*) ptr;
    backEndData->polys = (srfPoly_t*) ((char*) ptr + sizeof(backEndData_t));
    backEndData->polyVerts = (polyVert_t*) ((char*) ptr + sizeof(backEndData_t) + sizeof(srfPoly_t) * MAX_POLYS);

    InitOpenGL();

    gladLoadGL();

    R_InitImages();

    R_InitShaders();

    R_InitSkins();

    R_ModelInit();

    err = qglGetError();
    if (err != GL_NO_ERROR) {
        logger.info("glGetError() = 0x%x", err);
    }

    logger.info("----- finished R_Init -----");
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown(bool destroyWindow)
{
    logger.info("RE_Shutdown( %i )", destroyWindow);

    Cmd_RemoveCommand("modellist");
    Cmd_RemoveCommand("screenshot");
    Cmd_RemoveCommand("shaderlist");
    Cmd_RemoveCommand("gfxinfo");

    if (tr.registered) {
        R_DeleteTextures();
    }

    // shut down platform specific OpenGL stuff
    if (destroyWindow) {
        GLimp_Shutdown();
        glIsInitialized = false;
    }

    tr.registered = false;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration()
{
}
