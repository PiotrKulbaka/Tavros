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

/* R_IssueRenderCommands */
static void R_IssueRenderCommands()
{
    renderCommandList_t* cmdList;

    cmdList = &(backEndData->commands);
    assert(cmdList); // bk001205
    // add an end-of-list command
    *(int32*) (cmdList->cmds + cmdList->used) = RC_END_OF_LIST;

    // clear it out, in case this is a sync and not a buffer flip
    cmdList->used = 0;

    // actually start the commands going
    // let it start on the new batch
    RB_ExecuteRenderCommands(cmdList->cmds);
}

/*
============
R_GetCommandBuffer

make sure there is enough command space
============
*/
void* R_GetCommandBuffer(int32 bytes)
{
    renderCommandList_t* cmdList;

    cmdList = &(backEndData->commands);

    // always leave room for the end of list command
    if (cmdList->used + bytes + 4 > MAX_RENDER_COMMANDS) {
        if (bytes > MAX_RENDER_COMMANDS - 4) {
            Com_Error(ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes);
        }
        // if we run out of room, just start dropping commands
        return NULL;
    }

    cmdList->used += bytes;

    return cmdList->cmds + cmdList->used - bytes;
}


/*
=============
R_AddDrawSurfCmd
=============
*/
void R_AddDrawSurfCmd(drawSurf_t* drawSurfs, int32 numDrawSurfs)
{
    drawSurfsCommand_t* cmd;

    cmd = (drawSurfsCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_DRAW_SURFS;

    cmd->drawSurfs = drawSurfs;
    cmd->numDrawSurfs = numDrawSurfs;

    cmd->refdef = tr.refdef;
    cmd->viewParms = tr.viewParms;
}


/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void RE_SetColor(const float* rgba)
{
    setColorCommand_t* cmd;

    if (!tr.registered) {
        return;
    }
    cmd = (setColorCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_SET_COLOR;
    if (!rgba) {
        static float colorWhite[4] = {1, 1, 1, 1};

        rgba = colorWhite;
    }

    cmd->color[0] = rgba[0];
    cmd->color[1] = rgba[1];
    cmd->color[2] = rgba[2];
    cmd->color[3] = rgba[3];
}


/*
=============
RE_StretchPic
=============
*/
void RE_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader)
{
    stretchPicCommand_t* cmd;

    if (!tr.registered) {
        return;
    }
    cmd = (stretchPicCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_STRETCH_PIC;
    cmd->shader = R_GetShaderByHandle(hShader);
    cmd->point = tavros::math::vec2(x, y);
    cmd->size = tavros::math::vec2(w, h);
    cmd->uv1 = tavros::math::vec2(s1, t1);
    cmd->uv2 = tavros::math::vec2(s2, t2);
}


/*
====================
RE_BeginFrame

for each RE_EndFrame
====================
*/
void RE_BeginFrame()
{
    drawBufferCommand_t* cmd;

    if (!tr.registered) {
        return;
    }

    tr.frameCount++;
    tr.frameSceneNum = 0;

    //
    // draw buffer stuff
    //
    cmd = (drawBufferCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_DRAW_BUFFER;
}


/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame(int32* frontEndMsec, int32* backEndMsec)
{
    swapBuffersCommand_t* cmd;

    if (!tr.registered) {
        return;
    }
    cmd = (swapBuffersCommand_t*) R_GetCommandBuffer(sizeof(*cmd));
    if (!cmd) {
        return;
    }
    cmd->commandId = RC_SWAP_BUFFERS;

    R_IssueRenderCommands();

    // use the other buffers next frame, because another CPU
    // may still be rendering into the current ones
    R_ToggleSmpFrame();

    qglClearColor(1, 1, 1, 1);
    qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (frontEndMsec) {
        *frontEndMsec = tr.frontEndMsec;
    }
    tr.frontEndMsec = 0;
    if (backEndMsec) {
        *backEndMsec = backEnd.backEndMsec;
    }
    backEnd.backEndMsec = 0;
}

