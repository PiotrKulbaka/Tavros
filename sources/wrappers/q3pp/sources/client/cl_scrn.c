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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

static tavros::core::logger logger("cl_scrn");

bool scr_initialized; // ready to draw

cvar_t* cl_timegraph;
cvar_t* cl_debuggraph;
cvar_t* cl_graphheight;
cvar_t* cl_graphscale;
cvar_t* cl_graphshift;

/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640(float* x, float* y, float* w, float* h)
{
    float xscale;
    float yscale;

    // scale for screen sizes
    xscale = cls.glconfig.vidWidth / 640.0;
    yscale = cls.glconfig.vidHeight / 480.0;
    if (x) {
        *x *= xscale;
    }
    if (y) {
        *y *= yscale;
    }
    if (w) {
        *w *= xscale;
    }
    if (h) {
        *h *= yscale;
    }
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect(float x, float y, float width, float height, const float* color)
{
    RE_SetColor(color);

    SCR_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);

    RE_SetColor(NULL);
}


/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
    SCR_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}


/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar(int32 x, int32 y, float size, int32 ch)
{
    int32 row, col;
    float frow, fcol;
    float ax, ay, aw, ah;

    ch &= 255;

    if (ch == ' ') {
        return;
    }

    if (y < -size) {
        return;
    }

    ax = x;
    ay = y;
    aw = size;
    ah = size;
    SCR_AdjustFrom640(&ax, &ay, &aw, &ah);

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, cls.charSetShader);
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar(int32 x, int32 y, int32 ch)
{
    int32 row, col;
    float frow, fcol;
    float size;

    ch &= 255;

    if (ch == ' ') {
        return;
    }

    if (y < -SMALLCHAR_HEIGHT) {
        return;
    }

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    RE_DrawStretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, cls.charSetShader);
}


/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt(int32 x, int32 y, float size, const char* string, float* setColor, bool forceColor)
{
    tavros::math::vec4 color;
    const char*        s;
    int32              xx;

    // draw the drop shadow
    color[0] = color[1] = color[2] = 0;
    color[3] = setColor[3];
    RE_SetColor(color.data());
    s = string;
    xx = x;
    while (*s) {
        if (Q_IsColorString(s)) {
            s += 2;
            continue;
        }
        SCR_DrawChar(xx + 2, y + 2, size, *s);
        xx += size;
        s++;
    }


    // draw the colored text
    s = string;
    xx = x;
    RE_SetColor(setColor);
    while (*s) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                color = g_color_table[ColorIndex(*(s + 1))];
                color[3] = setColor[3];
                RE_SetColor(color.data());
            }
            s += 2;
            continue;
        }
        SCR_DrawChar(xx, y, size, *s);
        xx += size;
        s++;
    }
    RE_SetColor(NULL);
}


void SCR_DrawBigString(int32 x, int32 y, const char* s, float alpha)
{
    float color[4];

    color[0] = color[1] = color[2] = 1.0;
    color[3] = alpha;
    SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, false);
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt(int32 x, int32 y, const char* string, float* setColor, bool forceColor)
{
    tavros::math::vec4 color;
    const char*        s;
    int32              xx;

    // draw the colored text
    s = string;
    xx = x;
    RE_SetColor(setColor);
    while (*s) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                color = g_color_table[ColorIndex(*(s + 1))];
                color[3] = setColor[3];
                RE_SetColor(color.data());
            }
            s += 2;
            continue;
        }
        SCR_DrawSmallChar(xx, y, *s);
        xx += SMALLCHAR_WIDTH;
        s++;
    }
    RE_SetColor(NULL);
}

//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording()
{
    char  string[1024];
    int32 pos;

    if (!clc.demorecording) {
        return;
    }
    if (clc.spDemoRecording) {
        return;
    }

    pos = FS_FTell(clc.demofile);
    sprintf(string, "RECORDING %s: %ik", clc.demoName, pos / 1024);

    SCR_DrawStringExt(320 - strlen(string) * 4, 20, 8, string, g_color_table[7].data(), true);
}


/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
    float value;
    int32 color;
} graphsamp_t;

static int32       current;
static graphsamp_t values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph(float value, int32 color)
{
    values[current & 1023].value = value;
    values[current & 1023].color = color;
    current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph()
{
    int32 a, x, y, w, i, h;
    float v;
    int32 color;

    //
    // draw the graph
    //
    w = cls.glconfig.vidWidth;
    x = 0;
    y = cls.glconfig.vidHeight;
    RE_SetColor(g_color_table[0].data());
    RE_DrawStretchPic(x, y - cl_graphheight->integer, w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
    RE_SetColor(NULL);

    for (a = 0; a < w; a++) {
        i = (current - 1 - a + 1024) & 1023;
        v = values[i].value;
        color = values[i].color;
        v = v * cl_graphscale->integer + cl_graphshift->integer;

        if (v < 0) {
            v += cl_graphheight->integer * (1 + (int32) (-v / cl_graphheight->integer));
        }
        h = (int32) v % cl_graphheight->integer;
        RE_DrawStretchPic(x + w - 1 - a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader);
    }
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init()
{
    cl_timegraph = Cvar_Get("timegraph", "0", CVAR_CHEAT);
    cl_debuggraph = Cvar_Get("debuggraph", "0", CVAR_CHEAT);
    cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
    cl_graphscale = Cvar_Get("graphscale", "1", CVAR_CHEAT);
    cl_graphshift = Cvar_Get("graphshift", "0", CVAR_CHEAT);

    scr_initialized = true;
}


//=======================================================

/*
==================
SCR_DrawScreenField
==================
*/
void SCR_DrawScreenField()
{
    RE_BeginFrame();

    // wide aspect ratio screens need to have the sides cleared
    // unless they are displaying game renderings
    if (cls.state != CA_ACTIVE) {
        if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640) {
            RE_SetColor(g_color_table[0].data());
            RE_DrawStretchPic(0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader);
            RE_SetColor(NULL);
        }
    }

    if (!CL_UIIsInit()) {
        ::logger.debug("Draw screen without UI loaded.");
        return;
    }

    // if the menu is going to cover the entire screen, we
    // don't need to render anything under it
    if (!UI_IsFullscreen()) {
        switch (cls.state) {
        default:
            Com_Error(ERR_FATAL, "SCR_DrawScreenField: bad cls.state");
            break;
        case CA_DISCONNECTED:
            // force menu up
            S_StopAllSounds();
            UI_SetActiveMenu(UIMENU_MAIN);
            break;
        case CA_CONNECTING:
        case CA_CHALLENGING:
        case CA_CONNECTED:
            // connecting clients will only show the connection dialog
            // refresh to update the time
            UI_Refresh(cls.realtime);
            UI_DrawConnectScreen(false);
            break;
        case CA_LOADING:
        case CA_PRIMED:
            // draw the game information screen and loading progress
            CL_CGameRendering();

            // also draw the connection information, so it doesn't
            // flash away too briefly on local or lan games
            // refresh to update the time
            UI_Refresh(cls.realtime);
            UI_DrawConnectScreen(true);
            break;
        case CA_ACTIVE:
            CL_CGameRendering();
            SCR_DrawDemoRecording();
            break;
        }
    }

    // the menu draws next
    if (cls.keyCatchers & KEYCATCH_UI) {
        UI_Refresh(cls.realtime);
    }

    // console draws next
    Con_DrawConsole();

    // debug graph can be drawn on top of anything
    if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer) {
        SCR_DrawDebugGraph();
    }
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen()
{
    static int32 recursive;

    if (!scr_initialized) {
        return; // not initialized yet
    }

    if (++recursive > 2) {
        Com_Error(ERR_FATAL, "SCR_UpdateScreen: recursively called");
    }
    recursive = 1;

    SCR_DrawScreenField();

    if (com_speeds->integer) {
        RE_EndFrame(&time_frontend, &time_backend);
    } else {
        RE_EndFrame(NULL, NULL);
    }

    recursive = 0;
}

