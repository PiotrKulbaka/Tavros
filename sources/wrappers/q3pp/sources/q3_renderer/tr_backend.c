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

static tavros::core::logger logger("tr_backend");

backEndData_t* backEndData;
backEndState_t backEnd;


static float s_flipMatrix[16] = {
    // convert from our coordinate system (looking down X)
    // to OpenGL's coordinate system (looking down -Z)
    0, 0, -1, 0,
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 1
};


/* GL_Bind */
void GL_Bind(texture_type image)
{
    int32 texnum;

    if (0 == image) {
        logger.warning("GL_Bind: NULL image");
        texnum = tr.defaultImage;
    } else {
        texnum = image;
    }

    if (glState.currenttexture != texnum) {
        glState.currenttexture = texnum;
        qglBindTexture(GL_TEXTURE_2D, texnum);
    }
}

/* GL_Cull */
void GL_Cull(int32 cullType)
{
    if (glState.faceCulling == cullType) {
        return;
    }

    glState.faceCulling = cullType;

    if (cullType == CT_TWO_SIDED) {
        qglDisable(GL_CULL_FACE);
    } else {
        qglEnable(GL_CULL_FACE);

        if (cullType == CT_BACK_SIDED) {
            if (backEnd.viewParms.isMirror) {
                qglCullFace(GL_FRONT);
            } else {
                qglCullFace(GL_BACK);
            }
        } else {
            if (backEnd.viewParms.isMirror) {
                qglCullFace(GL_BACK);
            } else {
                qglCullFace(GL_FRONT);
            }
        }
    }
}

/* GL_TexEnv */
void GL_TexEnv(int32 env)
{
    if (env == glState.texEnv) {
        return;
    }

    glState.texEnv = env;


    switch (env) {
    case GL_MODULATE:
        qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;
    case GL_REPLACE:
        qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        break;
    case GL_DECAL:
        qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        break;
    case GL_ADD:
        qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        break;
    default:
        Com_Error(ERR_DROP, "GL_TexEnv: invalid env '%d' passed\n", env);
        break;
    }
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State(uint32 stateBits)
{
    uint32 diff = stateBits ^ glState.glStateBits;

    if (!diff) {
        return;
    }

    //
    // check depthFunc bits
    //
    if (diff & GLS_DEPTHFUNC_EQUAL) {
        if (stateBits & GLS_DEPTHFUNC_EQUAL) {
            qglDepthFunc(GL_EQUAL);
        } else {
            qglDepthFunc(GL_LEQUAL);
        }
    }

    //
    // check blend bits
    //
    if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
        GLenum srcFactor, dstFactor;

        if (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
            switch (stateBits & GLS_SRCBLEND_BITS) {
            case GLS_SRCBLEND_ZERO:
                srcFactor = GL_ZERO;
                break;
            case GLS_SRCBLEND_ONE:
                srcFactor = GL_ONE;
                break;
            case GLS_SRCBLEND_DST_COLOR:
                srcFactor = GL_DST_COLOR;
                break;
            case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
                srcFactor = GL_ONE_MINUS_DST_COLOR;
                break;
            case GLS_SRCBLEND_SRC_ALPHA:
                srcFactor = GL_SRC_ALPHA;
                break;
            case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
                srcFactor = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case GLS_SRCBLEND_DST_ALPHA:
                srcFactor = GL_DST_ALPHA;
                break;
            case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
                srcFactor = GL_ONE_MINUS_DST_ALPHA;
                break;
            case GLS_SRCBLEND_ALPHA_SATURATE:
                srcFactor = GL_SRC_ALPHA_SATURATE;
                break;
            default:
                srcFactor = GL_ONE; // to get warning to shut up
                Com_Error(ERR_DROP, "GL_State: invalid src blend state bits\n");
                break;
            }

            switch (stateBits & GLS_DSTBLEND_BITS) {
            case GLS_DSTBLEND_ZERO:
                dstFactor = GL_ZERO;
                break;
            case GLS_DSTBLEND_ONE:
                dstFactor = GL_ONE;
                break;
            case GLS_DSTBLEND_SRC_COLOR:
                dstFactor = GL_SRC_COLOR;
                break;
            case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
                dstFactor = GL_ONE_MINUS_SRC_COLOR;
                break;
            case GLS_DSTBLEND_SRC_ALPHA:
                dstFactor = GL_SRC_ALPHA;
                break;
            case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
                dstFactor = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case GLS_DSTBLEND_DST_ALPHA:
                dstFactor = GL_DST_ALPHA;
                break;
            case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
                dstFactor = GL_ONE_MINUS_DST_ALPHA;
                break;
            default:
                dstFactor = GL_ONE; // to get warning to shut up
                Com_Error(ERR_DROP, "GL_State: invalid dst blend state bits\n");
                break;
            }

            qglEnable(GL_BLEND);
            qglBlendFunc(srcFactor, dstFactor);
        } else {
            qglDisable(GL_BLEND);
        }
    }

    //
    // check depthmask
    //
    if (diff & GLS_DEPTHMASK_TRUE) {
        if (stateBits & GLS_DEPTHMASK_TRUE) {
            qglDepthMask(GL_TRUE);
        } else {
            qglDepthMask(GL_FALSE);
        }
    }

    //
    // fill/line mode
    //
    if (diff & GLS_POLYMODE_LINE) {
        if (stateBits & GLS_POLYMODE_LINE) {
            qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    //
    // depthtest
    //
    if (diff & GLS_DEPTHTEST_DISABLE) {
        if (stateBits & GLS_DEPTHTEST_DISABLE) {
            qglDisable(GL_DEPTH_TEST);
        } else {
            qglEnable(GL_DEPTH_TEST);
        }
    }

    //
    // alpha test
    //
    if (diff & GLS_ATEST_BITS) {
        switch (stateBits & GLS_ATEST_BITS) {
        case 0:
            qglDisable(GL_ALPHA_TEST);
            break;
        case GLS_ATEST_GT_0:
            qglEnable(GL_ALPHA_TEST);
            qglAlphaFunc(GL_GREATER, 0.0f);
            break;
        case GLS_ATEST_LT_80:
            qglEnable(GL_ALPHA_TEST);
            qglAlphaFunc(GL_LESS, 0.5f);
            break;
        case GLS_ATEST_GE_80:
            qglEnable(GL_ALPHA_TEST);
            qglAlphaFunc(GL_GEQUAL, 0.5f);
            break;
        default:
            assert(0);
            break;
        }
    }

    glState.glStateBits = stateBits;
}


/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace()
{
    float c;

    c = (backEnd.refdef.time & 255) / 255.0f;
    qglClearColor(c, c, c, 1);
    qglClear(GL_COLOR_BUFFER_BIT);
}


static void SetViewportAndScissor()
{
    qglMatrixMode(GL_PROJECTION);
    qglLoadMatrixf(backEnd.viewParms.projectionMatrix);
    qglMatrixMode(GL_MODELVIEW);

    // set the window clipping
    qglViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
    qglScissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
static void RB_BeginDrawingView()
{
    int32 clearBits = 0;

    // we will need to change the projection matrix before drawing
    // 2D images again
    backEnd.projection2D = false;

    //
    // set the modelview matrix for the viewer
    //
    SetViewportAndScissor();

    // ensures that depth writes are enabled for the depth clear
    GL_State(GLS_DEFAULT);
    // clear relevant buffers
    clearBits = GL_DEPTH_BUFFER_BIT;

    qglClear(clearBits);

    if ((backEnd.refdef.rdflags & RDF_HYPERSPACE)) {
        RB_Hyperspace();
        return;
    }

    glState.faceCulling = -1; // force face culling to set next time

    // we will only draw a sun if there was sky rendered in this view
    backEnd.skyRenderedThisView = false;

    // clip to the plane of the portal
    if (backEnd.viewParms.isPortal) {
        float  plane[4];
        double plane2[4];

        plane[0] = backEnd.viewParms.portalPlane.normal[0];
        plane[1] = backEnd.viewParms.portalPlane.normal[1];
        plane[2] = backEnd.viewParms.portalPlane.normal[2];
        plane[3] = backEnd.viewParms.portalPlane.dist;

        plane2[0] = DotProduct(backEnd.viewParms.orient.axis[0], plane);
        plane2[1] = DotProduct(backEnd.viewParms.orient.axis[1], plane);
        plane2[2] = DotProduct(backEnd.viewParms.orient.axis[2], plane);
        plane2[3] = DotProduct(plane, backEnd.viewParms.orient.origin) - plane[3];

        qglLoadMatrixf(s_flipMatrix);
        qglClipPlane(GL_CLIP_PLANE0, plane2);
        qglEnable(GL_CLIP_PLANE0);
    } else {
        qglDisable(GL_CLIP_PLANE0);
    }
}


#define MAC_EVENT_PUMP_MSEC 5

/*
==================
RB_RenderDrawSurfList
==================
*/
static void RB_RenderDrawSurfList(drawSurf_t* drawSurfs, int32 numDrawSurfs)
{
    shader_t *  shader, *oldShader;
    int32       fogNum, oldFogNum;
    int32       entityNum, oldEntityNum;
    int32       dlighted, oldDlighted;
    bool        depthRange, oldDepthRange;
    int32       i;
    drawSurf_t* drawSurf;
    int32       oldSort;
    float       originalTime;

    // save original time for entity shader offsets
    originalTime = backEnd.refdef.floatTime;

    // clear the z buffer, set the modelview, etc
    RB_BeginDrawingView();

    // draw everything
    oldEntityNum = -1;
    backEnd.currentEntity = &tr.worldEntity;
    oldShader = NULL;
    oldFogNum = -1;
    oldDepthRange = false;
    oldDlighted = false;
    oldSort = -1;
    depthRange = false;

    for (i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++) {
        if (drawSurf->sort == oldSort) {
            // fast path, same as previous sort
            rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
            continue;
        }
        oldSort = drawSurf->sort;
        R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted);

        //
        // change the tess parameters if needed
        // a "entityMergable" shader is a shader that can have surfaces from seperate
        // entities merged into a single batch, like smoke and blood puff sprites
        if (shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
            || (entityNum != oldEntityNum && !shader->entityMergable)) {
            if (oldShader != NULL) {
                RB_EndSurface();
            }
            RB_BeginSurface(shader, fogNum);
            oldShader = shader;
            oldFogNum = fogNum;
            oldDlighted = dlighted;
        }

        //
        // change the modelview matrix if needed
        //
        if (entityNum != oldEntityNum) {
            depthRange = false;

            if (entityNum != ENTITYNUM_WORLD) {
                backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
                backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
                // we have to reset the shaderTime as well otherwise image animations start
                // from the wrong frame
                tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

                // set up the transformation matrix
                R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.orient);

                // set up the dynamic lighting if needed
                if (backEnd.currentEntity->needDlights) {
                    R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient);
                }

                if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK) {
                    // hack the depth range to prevent view model from poking into walls
                    depthRange = true;
                }
            } else {
                backEnd.currentEntity = &tr.worldEntity;
                backEnd.refdef.floatTime = originalTime;
                backEnd.orient = backEnd.viewParms.world;
                // we have to reset the shaderTime as well otherwise image animations on
                // the world (like water) continue with the wrong frame
                tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
                R_TransformDlights(backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orient);
            }

            qglLoadMatrixf(backEnd.orient.modelMatrix);

            //
            // change depthrange if needed
            //
            if (oldDepthRange != depthRange) {
                if (depthRange) {
                    qglDepthRange(0, 0.3);
                } else {
                    qglDepthRange(0, 1);
                }
                oldDepthRange = depthRange;
            }

            oldEntityNum = entityNum;
        }

        // add the triangles for this surface
        rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
    }

    backEnd.refdef.floatTime = originalTime;

    // draw the contents of the last shader batch
    if (oldShader != NULL) {
        RB_EndSurface();
    }

    // go back to the world modelview matrix
    qglLoadMatrixf(backEnd.viewParms.world.modelMatrix);
    if (depthRange) {
        qglDepthRange(0, 1);
    }
}

/*
================
RB_SetGL2D
================
*/
void RB_SetGL2D()
{
    backEnd.projection2D = true;

    // set 2D virtual screen size
    qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
    qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
    qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
    qglOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
    qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();

    GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

    qglDisable(GL_CULL_FACE);
    qglDisable(GL_CLIP_PLANE0);

    // set time for 2D shaders
    backEnd.refdef.time = Sys_Milliseconds();
    backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}

static const void* RB_SetColor(const void* data)
{
    const setColorCommand_t* cmd = (const setColorCommand_t*) data;

    backEnd.color2D[0] = cmd->color[0] * 255;
    backEnd.color2D[1] = cmd->color[1] * 255;
    backEnd.color2D[2] = cmd->color[2] * 255;
    backEnd.color2D[3] = cmd->color[3] * 255;

    return (const void*) (cmd + 1);
}

static const void* RB_StretchPic(const void* data)
{
    const stretchPicCommand_t* cmd;
    shader_t*                  shader;
    int32                      numVerts, numIndexes;

    cmd = (const stretchPicCommand_t*) data;

    if (!backEnd.projection2D) {
        RB_SetGL2D();
    }

    shader = cmd->shader;
    if (shader != tess.shader) {
        if (tess.numIndexes) {
            RB_EndSurface();
        }
        backEnd.currentEntity = &backEnd.entity2D;
        RB_BeginSurface(shader, 0);
    }

    RB_CHECKOVERFLOW(4, 6);
    numVerts = tess.numVertexes;
    numIndexes = tess.numIndexes;

    tess.numVertexes += 4;
    tess.numIndexes += 6;

    tess.indexes[numIndexes] = numVerts + 3;
    tess.indexes[numIndexes + 1] = numVerts + 0;
    tess.indexes[numIndexes + 2] = numVerts + 2;
    tess.indexes[numIndexes + 3] = numVerts + 2;
    tess.indexes[numIndexes + 4] = numVerts + 0;
    tess.indexes[numIndexes + 5] = numVerts + 1;

    *(int32*) tess.vertexColors[numVerts] =
        *(int32*) tess.vertexColors[numVerts + 1] =
            *(int32*) tess.vertexColors[numVerts + 2] =
                *(int32*) tess.vertexColors[numVerts + 3] = *(int32*) backEnd.color2D;

    tess.xyz[numVerts][0] = cmd->point.x;
    tess.xyz[numVerts][1] = cmd->point.y;
    tess.xyz[numVerts][2] = 0;

    tess._texCoords[numVerts][0] = cmd->uv1;

    tess.xyz[numVerts + 1][0] = cmd->point.x + cmd->size.width;
    tess.xyz[numVerts + 1][1] = cmd->point.y;
    tess.xyz[numVerts + 1][2] = 0;

    tess._texCoords[numVerts + 1][0] = tavros::math::vec2(cmd->uv2.x, cmd->uv1.y);

    tess.xyz[numVerts + 2][0] = cmd->point.x + cmd->size.width;
    tess.xyz[numVerts + 2][1] = cmd->point.y + cmd->size.height;
    tess.xyz[numVerts + 2][2] = 0;

    tess._texCoords[numVerts + 2][0] = cmd->uv2;

    tess.xyz[numVerts + 3][0] = cmd->point.x;
    tess.xyz[numVerts + 3][1] = cmd->point.y + cmd->size.height;
    tess.xyz[numVerts + 3][2] = 0;

    tess._texCoords[numVerts + 3][0] = tavros::math::vec2(cmd->uv1.x, cmd->uv2.y);

    return (const void*) (cmd + 1);
}

static const void* RB_DrawSurfs(const void* data)
{
    const drawSurfsCommand_t* cmd;

    // finish any 2D drawing if needed
    if (tess.numIndexes) {
        RB_EndSurface();
    }

    cmd = (const drawSurfsCommand_t*) data;

    backEnd.refdef = cmd->refdef;
    backEnd.viewParms = cmd->viewParms;

    RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs);

    return (const void*) (cmd + 1);
}

static const void* RB_DrawBuffer(const void* data)
{
    const drawBufferCommand_t* cmd;

    cmd = (const drawBufferCommand_t*) data;

    qglDrawBuffer(GL_BACK);

    // clear screen for debugging

    return (const void*) (cmd + 1);
}

static const void* RB_SwapBuffers(const void* data)
{
    const swapBuffersCommand_t* cmd;

    // finish any 2D drawing if needed
    if (tess.numIndexes) {
        RB_EndSurface();
    }

    cmd = (const swapBuffersCommand_t*) data;

    GLimp_EndFrame();

    backEnd.projection2D = false;

    return (const void*) (cmd + 1);
}

/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands(const void* data)
{
    int32 t1, t2;

    t1 = Sys_Milliseconds();

    while (1) {
        switch (*(const int32*) data) {
        case RC_SET_COLOR:
            data = RB_SetColor(data);
            break;
        case RC_STRETCH_PIC:
            data = RB_StretchPic(data);
            break;
        case RC_DRAW_SURFS:
            data = RB_DrawSurfs(data);
            break;
        case RC_DRAW_BUFFER:
            data = RB_DrawBuffer(data);
            break;
        case RC_SWAP_BUFFERS:
            data = RB_SwapBuffers(data);
            break;
        case RC_SCREENSHOT:
            data = RB_TakeScreenshotCmd(data);
            break;

        case RC_END_OF_LIST:
        default:
            // stop rendering
            t2 = Sys_Milliseconds();
            backEnd.backEndMsec = t2 - t1;
            return;
        }
    }
}
