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
//
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640(float* x, float* y, float* w, float* h)
{
    // scale for screen sizes
    *x *= cgs.screenXScale;
    *y *= cgs.screenYScale;
    *w *= cgs.screenXScale;
    *h *= cgs.screenYScale;
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect(float x, float y, float width, float height, const float* color)
{
    RE_SetColor(color);

    CG_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader);

    RE_SetColor(NULL);
}

/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader)
{
    CG_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}


/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar(int32 x, int32 y, int32 width, int32 height, int32 ch)
{
    int32 row, col;
    float frow, fcol;
    float size;
    float ax, ay, aw, ah;

    ch &= 255;

    if (ch == ' ') {
        return;
    }

    ax = x;
    ay = y;
    aw = width;
    ah = height;
    CG_AdjustFrom640(&ax, &ay, &aw, &ah);

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, cgs.media.charsetShader);
}


/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt(int32 x, int32 y, const char* string, tavros::math::vec4 setColor, bool forceColor, bool shadow, int32 charWidth, int32 charHeight, int32 maxChars)
{
    tavros::math::vec4 color;
    const char*        s;
    int32              xx;
    int32              cnt;

    if (maxChars <= 0) {
        maxChars = 32767; // do them all!
    }

    // draw the drop shadow
    if (shadow) {
        color = tavros::math::vec4(0.0f, 0.0f, 0.0f, setColor.a);
        RE_SetColor(color.data());
        s = string;
        xx = x;
        cnt = 0;
        while (*s && cnt < maxChars) {
            if (Q_IsColorString(s)) {
                s += 2;
                continue;
            }
            CG_DrawChar(xx + 2, y + 2, charWidth, charHeight, *s);
            cnt++;
            xx += charWidth;
            s++;
        }
    }

    // draw the colored text
    s = string;
    xx = x;
    cnt = 0;
    RE_SetColor(setColor.data());
    while (*s && cnt < maxChars) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                color = g_color_table[ColorIndex(*(s + 1))];
                color[3] = setColor[3];
                RE_SetColor(color.data());
            }
            s += 2;
            continue;
        }
        CG_DrawChar(xx, y, charWidth, charHeight, *s);
        xx += charWidth;
        cnt++;
        s++;
    }
    RE_SetColor(NULL);
}

void CG_DrawBigString(int32 x, int32 y, const char* s, float alpha)
{
    tavros::math::vec4 color(1.0f, 1.0f, 1.0f, alpha);
    CG_DrawStringExt(x, y, s, color, false, true, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);
}

void CG_DrawBigStringColor(int32 x, int32 y, const char* s, tavros::math::vec4 color)
{
    CG_DrawStringExt(x, y, s, color, true, true, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);
}

void CG_DrawSmallString(int32 x, int32 y, const char* s, float alpha)
{
    tavros::math::vec4 color(1.0f, 1.0f, 1.0f, alpha);
    CG_DrawStringExt(x, y, s, color, false, false, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
}

void CG_DrawSmallStringColor(int32 x, int32 y, const char* s, tavros::math::vec4 color)
{
    CG_DrawStringExt(x, y, s, color, true, false, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int32 CG_DrawStrlen(const char* str)
{
    const char* s = str;
    int32       count = 0;

    while (*s) {
        if (Q_IsColorString(s)) {
            s += 2;
        } else {
            count++;
            s++;
        }
    }

    return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox(int32 x, int32 y, int32 w, int32 h, qhandle_t hShader)
{
    float s1, t1, s2, t2;

    s1 = x / 64.0;
    t1 = y / 64.0;
    s2 = (x + w) / 64.0;
    t2 = (y + h) / 64.0;
    RE_DrawStretchPic(x, y, w, h, s1, t1, s2, t2, hShader);
}


/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear()
{
    int32 top, bottom, left, right;
    int32 w, h;

    w = cgs.glconfig.vidWidth;
    h = cgs.glconfig.vidHeight;

    if (cg.refdef.x == 0 && cg.refdef.y == 0 && cg.refdef.width == w && cg.refdef.height == h) {
        return; // full screen rendering
    }

    top = cg.refdef.y;
    bottom = top + cg.refdef.height - 1;
    left = cg.refdef.x;
    right = left + cg.refdef.width - 1;

    // clear above view screen
    CG_TileClearBox(0, 0, w, top, cgs.media.backTileShader);

    // clear below view screen
    CG_TileClearBox(0, bottom, w, h - bottom, cgs.media.backTileShader);

    // clear left of view screen
    CG_TileClearBox(0, top, left, bottom - top + 1, cgs.media.backTileShader);

    // clear right of view screen
    CG_TileClearBox(right, top, w - right, bottom - top + 1, cgs.media.backTileShader);
}


/*
================
CG_FadeColor
================
*/
tavros::math::vec4 CG_FadeColor(int32 startMsec, int32 totalMsec)
{
    if (startMsec == 0) {
        return tavros::math::vec4(0.0f);
    }

    int32 t = cg.time - startMsec;

    if (t >= totalMsec) {
        return tavros::math::vec4(0.0f);
    }

    // fade out
    if (totalMsec - t < FADE_TIME) {
        float fade = (totalMsec - t) * 1.0 / FADE_TIME;
        return tavros::math::vec4(1.0f, 1.0f, 1.0f, fade);
    }
    return tavros::math::vec4(1.0f);
}

/*
=================
CG_GetColorForHealth
=================
*/
tavros::math::vec4 CG_GetColorForHealth(int32 health, int32 armor)
{
    int32 count;
    int32 max;

    // calculate the total points of damage that can
    // be sustained at the current health / armor level
    if (health <= 0) {
        return tavros::math::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    count = armor;
    max = health * ARMOR_PROTECTION / (1.0 - ARMOR_PROTECTION);
    if (max < count) {
        count = max;
    }
    health += count;

    // set the color based on health
    float g = 0.0f, b = 0.0f;
    if (health >= 100) {
        b = 1.0;
    } else if (health < 66) {
        b = 0;
    } else {
        b = (health - 66) / 33.0;
    }

    if (health > 60) {
        g = 1.0;
    } else if (health < 30) {
        g = 0;
    } else {
        g = (health - 30) / 30.0;
    }

    return tavros::math::vec4(1.0f, g, b, 1.0f);
}

/*
=================
CG_ColorForHealth
=================
*/
tavros::math::vec4 CG_ColorForHealth()
{
    return CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH], cg.snap->ps.stats[STAT_ARMOR]);
}

/*
=================
UI_DrawProportionalString2
=================
*/
static int32 propMap[128][3] = {
    {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, -1},

    {0, 0, PROP_SPACE_WIDTH}, // SPACE
    {11, 122, 7},             // !
    {154, 181, 14},           // "
    {55, 122, 17},            // #
    {79, 122, 18},            // $
    {101, 122, 23},           // %
    {153, 122, 18},           // &
    {9, 93, 7},               // '
    {207, 122, 8},            // (
    {230, 122, 9},            // )
    {177, 122, 18},           // *
    {30, 152, 18},            // +
    {85, 181, 7},             // ,
    {34, 93, 11},             // -
    {110, 181, 6},            // .
    {130, 152, 14},           // /

    {22, 64, 17},             // 0
    {41, 64, 12},             // 1
    {58, 64, 17},             // 2
    {78, 64, 18},             // 3
    {98, 64, 19},             // 4
    {120, 64, 18},            // 5
    {141, 64, 18},            // 6
    {204, 64, 16},            // 7
    {162, 64, 17},            // 8
    {182, 64, 18},            // 9
    {59, 181, 7},             // :
    {35, 181, 7},             // ;
    {203, 152, 14},           // <
    {56, 93, 14},             // =
    {228, 152, 14},           // >
    {177, 181, 18},           // ?

    {28, 122, 22},            // @
    {5, 4, 18},               // A
    {27, 4, 18},              // B
    {48, 4, 18},              // C
    {69, 4, 17},              // D
    {90, 4, 13},              // E
    {106, 4, 13},             // F
    {121, 4, 18},             // G
    {143, 4, 17},             // H
    {164, 4, 8},              // I
    {175, 4, 16},             // J
    {195, 4, 18},             // K
    {216, 4, 12},             // L
    {230, 4, 23},             // M
    {6, 34, 18},              // N
    {27, 34, 18},             // O

    {48, 34, 18},             // P
    {68, 34, 18},             // Q
    {90, 34, 17},             // R
    {110, 34, 18},            // S
    {130, 34, 14},            // T
    {146, 34, 18},            // U
    {166, 34, 19},            // V
    {185, 34, 29},            // W
    {215, 34, 18},            // X
    {234, 34, 18},            // Y
    {5, 64, 14},              // Z
    {60, 152, 7},             // [
    {106, 151, 13},           // '\'
    {83, 152, 7},             // ]
    {128, 122, 17},           // ^
    {4, 152, 21},             // _

    {134, 181, 5},            // '
    {5, 4, 18},               // A
    {27, 4, 18},              // B
    {48, 4, 18},              // C
    {69, 4, 17},              // D
    {90, 4, 13},              // E
    {106, 4, 13},             // F
    {121, 4, 18},             // G
    {143, 4, 17},             // H
    {164, 4, 8},              // I
    {175, 4, 16},             // J
    {195, 4, 18},             // K
    {216, 4, 12},             // L
    {230, 4, 23},             // M
    {6, 34, 18},              // N
    {27, 34, 18},             // O

    {48, 34, 18},             // P
    {68, 34, 18},             // Q
    {90, 34, 17},             // R
    {110, 34, 18},            // S
    {130, 34, 14},            // T
    {146, 34, 18},            // U
    {166, 34, 19},            // V
    {185, 34, 29},            // W
    {215, 34, 18},            // X
    {234, 34, 18},            // Y
    {5, 64, 14},              // Z
    {153, 152, 13},           // {
    {11, 181, 5},             // |
    {180, 152, 13},           // }
    {79, 93, 17},             // ~
    {0, 0, -1}                // DEL
};

static int32 propMapB[26][3] = {
    {11, 12, 33},
    {49, 12, 31},
    {85, 12, 31},
    {120, 12, 30},
    {156, 12, 21},
    {183, 12, 21},
    {207, 12, 32},

    {13, 55, 30},
    {49, 55, 13},
    {66, 55, 29},
    {101, 55, 31},
    {135, 55, 21},
    {158, 55, 40},
    {204, 55, 32},

    {12, 97, 31},
    {48, 97, 31},
    {82, 97, 30},
    {118, 97, 30},
    {153, 97, 30},
    {185, 97, 25},
    {213, 97, 30},

    {11, 139, 32},
    {42, 139, 51},
    {93, 139, 32},
    {126, 139, 31},
    {158, 139, 25},
};

#define PROPB_GAP_WIDTH   4
#define PROPB_SPACE_WIDTH 12
#define PROPB_HEIGHT      36

void        UI_DrawBannerString(int32 x, int32 y, const char* str, int32 style, tavros::math::vec4 color);                              // implemented in ui_atoms.c
int32       UI_ProportionalStringWidth(const char* str);                                                                                // implemented in ui_atoms.c
float       UI_ProportionalSizeScale(int32 style);                                                                                      // implemented in ui_atoms.c
static void UI_DrawProportionalString2(int32 x, int32 y, const char* str, tavros::math::vec4 color, float sizeScale, qhandle_t charset) // duplicate symbol
{
    const char* s;
    uint8       ch;
    float       ax;
    float       ay;
    float       aw;
    float       ah;
    float       frow;
    float       fcol;
    float       fwidth;
    float       fheight;

    // draw the colored text
    RE_SetColor(color.data());

    ax = x * cgs.screenXScale + cgs.screenXBias;
    ay = y * cgs.screenXScale;

    s = str;
    while (*s) {
        ch = *s & 127;
        if (ch == ' ') {
            aw = (float) PROP_SPACE_WIDTH * cgs.screenXScale * sizeScale;
        } else if (propMap[ch][2] != -1) {
            fcol = (float) propMap[ch][0] / 256.0f;
            frow = (float) propMap[ch][1] / 256.0f;
            fwidth = (float) propMap[ch][2] / 256.0f;
            fheight = (float) PROP_HEIGHT / 256.0f;
            aw = (float) propMap[ch][2] * cgs.screenXScale * sizeScale;
            ah = (float) PROP_HEIGHT * cgs.screenXScale * sizeScale;
            RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
        } else {
            aw = 0;
        }

        ax += (aw + (float) PROP_GAP_WIDTH * cgs.screenXScale * sizeScale);
        s++;
    }

    RE_SetColor(NULL);
}
