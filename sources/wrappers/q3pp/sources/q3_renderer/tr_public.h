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
#pragma once

#include "../cgame/tr_types.h"

// called before the library is unloaded
// if the system is just reconfiguring, pass destroyWindow = false,
// which will keep the screen from flashing to the desktop.
void RE_Shutdown(bool destroyWindow);

// All data that will be used in a level should be
// registered before rendering any frames to prevent disk hits,
// but they can still be registered at a later time
// if necessary.
//
// BeginRegistration makes any existing media pointers invalid
// and returns the current gl configuration, including screen width
// and height, which can be used by the client to intelligently
// size display elements
void      RE_BeginRegistration(glconfig_t* glconfig);
qhandle_t RE_RegisterModel(const char* name);
qhandle_t RE_RegisterSkin(const char* name);
qhandle_t RE_RegisterShader(const char* name);
qhandle_t RE_RegisterShaderNoMip(const char* name);
void      RE_LoadWorldMap(const char* mapname);

// EndRegistration will draw a tiny polygon with each texture, forcing
// them to be loaded into card memory
void RE_EndRegistration();

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void RE_ClearScene();
void RE_AddRefEntityToScene(const refEntity_t* ent);
void RE_AddPolyToScene(qhandle_t hShader, int32 numVerts, const polyVert_t* verts, int32 num);
void RE_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void RE_RenderScene(const refdef_t* fd);
void RE_SetColor(const float* rgba);
void RE_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader);
void RE_BeginFrame();

// if the pointers are not NULL, timing info will be returned
void  RE_EndFrame(int32* frontEndMsec, int32* backEndMsec);
int32 R_MarkFragments(int32 numPoints, const vec3_t* points, const vec3_t projection, int32 maxPoints, vec3_t pointBuffer, int32 maxFragments, markFragment_t* fragmentBuffer);
int32 R_LerpTag(orientation_t* tag, qhandle_t handle, int32 startFrame, int32 endFrame, float frac, const char* tagName);
void  R_ModelBounds(qhandle_t handle, vec3_t mins, vec3_t maxs);
void  R_RemapShader(const char* oldShader, const char* newShader, const char* timeOffset);
