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
/**********************************************************************
    UI_ATOMS.C

    User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

#include "../client/client.h"

static tavros::core::logger logger("ui_atoms");

uiStatic_t uis;
bool       m_entersound; // after a frame, so caching won't disrupt the sound

/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar(float min, float max, float value)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu(menuframework_s* menu)
{
    int32         i;
    menucommon_s* item;

    // avoid stacking menus invoked by hotkeys
    for (i = 0; i < uis.menusp; i++) {
        if (uis.stack[i] == menu) {
            uis.menusp = i;
            break;
        }
    }

    if (i == uis.menusp) {
        if (uis.menusp >= MAX_MENUDEPTH) {
            Com_Error(ERR_DROP, "UI_PushMenu: menu stack overflow");
        }

        uis.stack[uis.menusp++] = menu;
    }

    uis.activemenu = menu;

    // default cursor position
    menu->cursor = 0;
    menu->cursor_prev = 0;

    m_entersound = true;

    Key_SetCatcher(KEYCATCH_UI);

    // force first available item to have focus
    for (i = 0; i < menu->nitems; i++) {
        item = (menucommon_s*) menu->items[i];
        if (!(item->flags & (QMF_GRAYED | QMF_MOUSEONLY | QMF_INACTIVE))) {
            menu->cursor_prev = -1;
            Menu_SetCursor(menu, i);
            break;
        }
    }

    uis.firstdraw = true;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu()
{
    S_StartLocalSound(menu_out_sound, CHAN_LOCAL_SOUND);

    uis.menusp--;

    if (uis.menusp < 0) {
        Com_Error(ERR_DROP, "UI_PopMenu: menu stack underflow");
    }

    if (uis.menusp) {
        uis.activemenu = uis.stack[uis.menusp - 1];
        uis.firstdraw = true;
    } else {
        UI_ForceMenuOff();
    }
}

void UI_ForceMenuOff()
{
    uis.menusp = 0;
    uis.activemenu = NULL;

    Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_UI);
    Key_ClearStates();
    Cvar_Set("cl_paused", "0");
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

// bk001205 - code below duplicated in cgame/cg_drawtools.c
// bk001205 - FIXME: does this belong in ui_shared.c?
/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2(int32 x, int32 y, const char* str, tavros::math::vec4 color)
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

    ax = x * uis.scale + uis.bias;
    ay = y * uis.scale;

    s = str;
    while (*s) {
        ch = *s & 127;
        if (ch == ' ') {
            ax += ((float) PROPB_SPACE_WIDTH + (float) PROPB_GAP_WIDTH) * uis.scale;
        } else if (ch >= 'A' && ch <= 'Z') {
            ch -= 'A';
            fcol = (float) propMapB[ch][0] / 256.0f;
            frow = (float) propMapB[ch][1] / 256.0f;
            fwidth = (float) propMapB[ch][2] / 256.0f;
            fheight = (float) PROPB_HEIGHT / 256.0f;
            aw = (float) propMapB[ch][2] * uis.scale;
            ah = (float) PROPB_HEIGHT * uis.scale;
            RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, uis.charsetPropB);
            ax += (aw + (float) PROPB_GAP_WIDTH * uis.scale);
        }
        s++;
    }

    RE_SetColor(NULL);
}

void UI_DrawBannerString(int32 x, int32 y, const char* str, int32 style, tavros::math::vec4 color)
{
    const char* s;
    int32       ch;
    int32       width;

    // find the width of the drawn text
    s = str;
    width = 0;
    while (*s) {
        ch = *s;
        if (ch == ' ') {
            width += PROPB_SPACE_WIDTH;
        } else if (ch >= 'A' && ch <= 'Z') {
            width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
        }
        s++;
    }
    width -= PROPB_GAP_WIDTH;

    switch (style & UI_FORMATMASK) {
    case UI_CENTER:
        x -= width / 2;
        break;

    case UI_RIGHT:
        x -= width;
        break;

    case UI_LEFT:
    default:
        break;
    }

    if (style & UI_DROPSHADOW) {
        UI_DrawBannerString2(x + 2, y + 2, str, tavros::math::vec4(0.0f, 0.0f, 0.0f, color.w));
    }

    UI_DrawBannerString2(x, y, str, color);
}


int32 UI_ProportionalStringWidth(const char* str)
{
    const char* s;
    int32       ch;
    int32       charWidth;
    int32       width;

    s = str;
    width = 0;
    while (*s) {
        ch = *s & 127;
        charWidth = propMap[ch][2];
        if (charWidth != -1) {
            width += charWidth;
            width += PROP_GAP_WIDTH;
        }
        s++;
    }

    width -= PROP_GAP_WIDTH;
    return width;
}

static void UI_DrawProportionalString2(int32 x, int32 y, const char* str, tavros::math::vec4 color, float sizeScale, qhandle_t charset)
{
    const char* s;
    uint8       ch;
    float       ax;
    float       ay;
    float       aw = 0; // bk001204 - init
    float       ah;
    float       frow;
    float       fcol;
    float       fwidth;
    float       fheight;

    // draw the colored text
    RE_SetColor(color.data());

    ax = x * uis.scale + uis.bias;
    ay = y * uis.scale;

    s = str;
    while (*s) {
        ch = *s & 127;
        if (ch == ' ') {
            aw = (float) PROP_SPACE_WIDTH * uis.scale * sizeScale;
        } else if (propMap[ch][2] != -1) {
            fcol = (float) propMap[ch][0] / 256.0f;
            frow = (float) propMap[ch][1] / 256.0f;
            fwidth = (float) propMap[ch][2] / 256.0f;
            fheight = (float) PROP_HEIGHT / 256.0f;
            aw = (float) propMap[ch][2] * uis.scale * sizeScale;
            ah = (float) PROP_HEIGHT * uis.scale * sizeScale;
            RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
        }

        ax += (aw + (float) PROP_GAP_WIDTH * uis.scale * sizeScale);
        s++;
    }

    RE_SetColor(NULL);
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale(int32 style)
{
    if (style & UI_SMALLFONT) {
        return PROP_SMALL_SIZE_SCALE;
    }

    return 1.00;
}


/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString(int32 x, int32 y, const char* str, int32 style, tavros::math::vec4 color)
{
    int32 width;
    float sizeScale;

    sizeScale = UI_ProportionalSizeScale(style);

    switch (style & UI_FORMATMASK) {
    case UI_CENTER:
        width = UI_ProportionalStringWidth(str) * sizeScale;
        x -= width / 2;
        break;

    case UI_RIGHT:
        width = UI_ProportionalStringWidth(str) * sizeScale;
        x -= width;
        break;

    case UI_LEFT:
    default:
        break;
    }

    if (style & UI_DROPSHADOW) {
        auto draw_color = tavros::math::vec4(0.0f, 0.0f, 0.0f, color.w);
        UI_DrawProportionalString2(x + 2, y + 2, str, draw_color, sizeScale, uis.charsetProp);
    }

    if (style & UI_INVERSE) {
        auto draw_color = tavros::math::vec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, color.w);
        UI_DrawProportionalString2(x, y, str, draw_color, sizeScale, uis.charsetProp);
        return;
    }

    if (style & UI_PULSE) {
        auto draw_color = tavros::math::vec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, color.w);
        UI_DrawProportionalString2(x, y, str, color, sizeScale, uis.charsetProp);
        draw_color = tavros::math::vec4(color.x, color.y, color.z, 0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR));
        UI_DrawProportionalString2(x, y, str, draw_color, sizeScale, uis.charsetPropGlow);
        return;
    }

    UI_DrawProportionalString2(x, y, str, color, sizeScale, uis.charsetProp);
}

/*
=================
UI_DrawProportionalString_Wrapped
=================
*/
void UI_DrawProportionalString_AutoWrapped(int32 x, int32 y, int32 xmax, int32 ystep, const char* str, int32 style, tavros::math::vec4 color)
{
    int32 width;
    char *s1, *s2, *s3;
    char  c_bcp;
    char  buf[1024];
    float sizeScale;

    if (!str || str[0] == '\0') {
        return;
    }

    sizeScale = UI_ProportionalSizeScale(style);

    Q_strncpyz(buf, str, sizeof(buf));
    s1 = s2 = s3 = buf;

    while (1) {
        do {
            s3++;
        } while (*s3 != ' ' && *s3 != '\0');
        c_bcp = *s3;
        *s3 = '\0';
        width = UI_ProportionalStringWidth(s1) * sizeScale;
        *s3 = c_bcp;
        if (width > xmax) {
            if (s1 == s2) {
                // fuck, don't have a clean cut, we'll overflow
                s2 = s3;
            }
            *s2 = '\0';
            UI_DrawProportionalString(x, y, s1, style, color);
            y += ystep;
            if (c_bcp == '\0') {
                // that was the last word
                // we could start a new loop, but that wouldn't be much use
                // even if the word is too long, we would overflow it (see above)
                // so just print it now if needed
                s2++;
                if (*s2 != '\0') { // if we are printing an overflowing line we have s2 == s3
                    UI_DrawProportionalString(x, y, s2, style, color);
                }
                break;
            }
            s2++;
            s1 = s2;
            s3 = s2;
        } else {
            s2 = s3;
            if (c_bcp == '\0') // we reached the end
            {
                UI_DrawProportionalString(x, y, s1, style, color);
                break;
            }
        }
    }
}

/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2(int32 x, int32 y, const char* str, tavros::math::vec4 color, int32 charw, int32 charh)
{
    const char*        s;
    char               ch;
    int32              forceColor = false; // APSFIXME;
    tavros::math::vec4 tempcolor;
    float              ax;
    float              ay;
    float              aw;
    float              ah;
    float              frow;
    float              fcol;

    if (y < -charh) {
        // offscreen
        return;
    }

    // draw the colored text
    RE_SetColor(color.data());

    ax = x * uis.scale + uis.bias;
    ay = y * uis.scale;
    aw = charw * uis.scale;
    ah = charh * uis.scale;

    s = str;
    while (*s) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                tempcolor = g_color_table[ColorIndex(s[1])];
                tempcolor[3] = color[3];
                RE_SetColor(tempcolor.data());
            }
            s += 2;
            continue;
        }

        ch = *s & 255;
        if (ch != ' ') {
            frow = (ch >> 4) * 0.0625;
            fcol = (ch & 15) * 0.0625;
            RE_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset);
        }

        ax += aw;
        s++;
    }

    RE_SetColor(NULL);
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString(int32 x, int32 y, const char* str, int32 style, tavros::math::vec4 color)
{
    int32              len;
    int32              charw;
    int32              charh;
    tavros::math::vec4 newcolor;
    tavros::math::vec4 lowlight;
    tavros::math::vec4 dropcolor;
    tavros::math::vec4 drawcolor;

    if (!str) {
        return;
    }

    if ((style & UI_BLINK) && ((uis.realtime / BLINK_DIVISOR) & 1)) {
        return;
    }

    if (style & UI_SMALLFONT) {
        charw = SMALLCHAR_WIDTH;
        charh = SMALLCHAR_HEIGHT;
    } else if (style & UI_GIANTFONT) {
        charw = GIANTCHAR_WIDTH;
        charh = GIANTCHAR_HEIGHT;
    } else {
        charw = BIGCHAR_WIDTH;
        charh = BIGCHAR_HEIGHT;
    }

    if (style & UI_PULSE) {
        lowlight = color * 0.8f;
        newcolor = tavros::math::lerp(color, lowlight, 0.5 + 0.5 * sin(uis.realtime / PULSE_DIVISOR));
        drawcolor = newcolor;
    } else {
        drawcolor = color;
    }

    switch (style & UI_FORMATMASK) {
    case UI_CENTER:
        // center justify at x
        len = strlen(str);
        x = x - len * charw / 2;
        break;

    case UI_RIGHT:
        // right justify at x
        len = strlen(str);
        x = x - len * charw;
        break;

    default:
        // left justify at x
        break;
    }

    if (style & UI_DROPSHADOW) {
        dropcolor = tavros::math::vec4(0.0f, 0.0f, 0.0f, drawcolor.a);
        UI_DrawString2(x + 2, y + 2, str, dropcolor, charw, charh);
    }

    UI_DrawString2(x, y, str, drawcolor, charw, charh);
}

/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar(int32 x, int32 y, int32 ch, int32 style, tavros::math::vec4 color)
{
    char buff[2];

    buff[0] = ch;
    buff[1] = '\0';

    UI_DrawString(x, y, buff, style, color);
}

bool UI_IsFullscreen()
{
    if (uis.activemenu && (Key_GetCatcher() & KEYCATCH_UI)) {
        return uis.activemenu->fullscreen;
    }

    return false;
}

void UI_SetActiveMenu(uiMenuCommand_t menu)
{
    // this should be the ONLY way the menu system is brought up
    // enusure minumum menu data is cached
    Menu_Cache();

    switch (menu) {
    case UIMENU_NONE:
        UI_ForceMenuOff();
        return;
    case UIMENU_MAIN:
        UI_MainMenu();
        return;
    case UIMENU_INGAME:
        /*
        //GRank
        UI_RankingsMenu();
        return;
        */
        Cvar_Set("cl_paused", "1");
        UI_InGameMenu();
        return;

    // bk001204
    case UIMENU_TEAM:
    case UIMENU_POSTGAME:
    default:
        logger.debug("UI_SetActiveMenu: bad enum {}", (uint32) menu);
        break;
    }
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent(int32 key, int32 down)
{
    sfxHandle_t s;

    if (!uis.activemenu) {
        return;
    }

    if (!down) {
        return;
    }

    if (uis.activemenu->key) {
        s = uis.activemenu->key(key);
    } else {
        s = Menu_DefaultKey(uis.activemenu, key);
    }

    if ((s > 0) && (s != menu_null_sound)) {
        S_StartLocalSound(s, CHAN_LOCAL_SOUND);
    }
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent(int32 dx, int32 dy)
{
    int32         i;
    menucommon_s* m;

    if (!uis.activemenu) {
        return;
    }

    // update mouse screen position
    uis.cursorx += dx;
    if (uis.cursorx < 0) {
        uis.cursorx = 0;
    } else if (uis.cursorx > SCREEN_WIDTH) {
        uis.cursorx = SCREEN_WIDTH;
    }

    uis.cursory += dy;
    if (uis.cursory < 0) {
        uis.cursory = 0;
    } else if (uis.cursory > SCREEN_HEIGHT) {
        uis.cursory = SCREEN_HEIGHT;
    }

    // region test the active menu items
    for (i = 0; i < uis.activemenu->nitems; i++) {
        m = (menucommon_s*) uis.activemenu->items[i];

        if (m->flags & (QMF_GRAYED | QMF_INACTIVE)) {
            continue;
        }

        if ((uis.cursorx < m->left) || (uis.cursorx > m->right) || (uis.cursory < m->top) || (uis.cursory > m->bottom)) {
            // cursor out of item bounds
            continue;
        }

        // set focus to item at cursor
        if (uis.activemenu->cursor != i) {
            Menu_SetCursor(uis.activemenu, i);
            ((menucommon_s*) (uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

            if (!(((menucommon_s*) (uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT)) {
                S_StartLocalSound(menu_move_sound, CHAN_LOCAL_SOUND);
            }
        }

        ((menucommon_s*) (uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
        return;
    }

    if (uis.activemenu->nitems > 0) {
        // out of any region
        ((menucommon_s*) (uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
    }
}

char* UI_Argv(int32 arg)
{
    static char buffer[MAX_STRING_CHARS];

    Cmd_ArgvBuffer(arg, buffer, sizeof(buffer));

    return buffer;
}


char* UI_Cvar_VariableString(const char* var_name)
{
    static char buffer[MAX_STRING_CHARS];

    Cvar_VariableStringBuffer(var_name, buffer, sizeof(buffer));

    return buffer;
}


/*
=================
UI_Cache
=================
*/
void UI_Cache_f()
{
    MainMenu_Cache();
    InGame_Cache();
    ConfirmMenu_Cache();
    PlayerModel_Cache();
    PlayerSettings_Cache();
    Controls_Cache();
    Demos_Cache();
    Preferences_Cache();
    ServerInfo_Cache();
    SpecifyServer_Cache();
    ArenaServers_Cache();
    StartServer_Cache();
    ServerOptions_Cache();
    GraphicsOptions_Cache();
    UI_DisplayOptionsMenu_Cache();
    UI_SoundOptionsMenu_Cache();
    UI_NetworkOptionsMenu_Cache();
    UI_SPLevelMenu_Cache();
    UI_SPSkillMenu_Cache();
    UI_SPPostgameMenu_Cache();
    TeamMain_Cache();
    UI_AddBots_Cache();
    UI_RemoveBots_Cache();
    UI_SetupMenu_Cache();
    UI_BotSelectMenu_Cache();
}


/*
=================
UI_ConsoleCommand
=================
*/
bool UI_ConsoleCommand(int32 realTime)
{
    char* cmd;

    cmd = UI_Argv(0);

    logger.debug("Execute command: '{}'", cmd);

    // ensure minimum menu data is available
    Menu_Cache();

    if (Q_stricmp(cmd, "levelselect") == 0) {
        UI_SPLevelMenu_f();
        return true;
    }

    if (Q_stricmp(cmd, "postgame") == 0) {
        UI_SPPostgameMenu_f();
        return true;
    }

    if (Q_stricmp(cmd, "ui_cache") == 0) {
        UI_Cache_f();
        return true;
    }

    if (Q_stricmp(cmd, "ui_teamOrders") == 0) {
        UI_TeamOrdersMenu_f();
        return true;
    }

    if (Q_stricmp(cmd, "iamacheater") == 0) {
        UI_SPUnlock_f();
        return true;
    }

    if (Q_stricmp(cmd, "iamamonkey") == 0) {
        UI_SPUnlockMedals_f();
        return true;
    }

    return false;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown()
{
}

/*
=================
UI_Init
=================
*/
void UI_Init()
{
    UI_RegisterCvars();

    UI_InitGameinfo();

    // cache redundant calulations
    uis.glconfig = cls.glconfig;

    // for 640x480 virtualized screen
    uis.scale = uis.glconfig.vidHeight * (1.0 / 480.0);
    if (uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640) {
        // wide screen
        uis.bias = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0 / 480.0)));
    } else {
        // no wide screen
        uis.bias = 0;
    }

    // initialize the menu system
    Menu_Cache();

    uis.activemenu = NULL;
    uis.menusp = 0;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640(float* x, float* y, float* w, float* h)
{
    // expect valid pointers
    *x = *x * uis.scale + uis.bias;
    *y *= uis.scale;
    *w *= uis.scale;
    *h *= uis.scale;
}

void UI_DrawNamedPic(float x, float y, float width, float height, const char* picname)
{
    qhandle_t hShader;

    hShader = RE_RegisterShaderNoMip(picname);
    UI_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader)
{
    float s0;
    float s1;
    float t0;
    float t1;

    if (w < 0) { // flip about vertical
        w = -w;
        s0 = 1;
        s1 = 0;
    } else {
        s0 = 0;
        s1 = 1;
    }

    if (h < 0) { // flip about horizontal
        h = -h;
        t0 = 1;
        t1 = 0;
    } else {
        t0 = 0;
        t1 = 1;
    }

    UI_AdjustFrom640(&x, &y, &w, &h);
    RE_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect(float x, float y, float width, float height, tavros::math::vec4 color)
{
    RE_SetColor(color.data());

    UI_AdjustFrom640(&x, &y, &width, &height);
    RE_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, uis.whiteShader);

    RE_SetColor(NULL);
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect(float x, float y, float width, float height, tavros::math::vec4 color)
{
    RE_SetColor(color.data());

    UI_AdjustFrom640(&x, &y, &width, &height);

    RE_DrawStretchPic(x, y, width, 1, 0, 0, 0, 0, uis.whiteShader);
    RE_DrawStretchPic(x, y, 1, height, 0, 0, 0, 0, uis.whiteShader);
    RE_DrawStretchPic(x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader);
    RE_DrawStretchPic(x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader);

    RE_SetColor(NULL);
}

void UI_SetColor(const float* rgba)
{
    RE_SetColor(rgba);
}

/*
=================
UI_Refresh
=================
*/
void UI_Refresh(int32 realtime)
{
    uis.frametime = realtime - uis.realtime;
    uis.realtime = realtime;

    if (!(Key_GetCatcher() & KEYCATCH_UI)) {
        return;
    }

    if (uis.activemenu) {
        if (uis.activemenu->fullscreen) {
            // draw the background
            if (uis.activemenu->showlogo) {
                UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
            } else {
                UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackNoLogoShader);
            }
        }

        if (uis.activemenu->draw) {
            uis.activemenu->draw();
        } else {
            Menu_Draw(uis.activemenu);
        }

        if (uis.firstdraw) {
            UI_MouseEvent(0, 0);
            uis.firstdraw = false;
        }
    }

    // draw cursor
    UI_SetColor(NULL);
    UI_DrawHandlePic(uis.cursorx - 16, uis.cursory - 16, 32, 32, uis.cursor);

    // delay playing the enter sound until after the
    // menu has been drawn, to avoid delay while
    // caching images
    if (m_entersound) {
        S_StartLocalSound(menu_in_sound, CHAN_LOCAL_SOUND);
        m_entersound = false;
    }
}

bool UI_CursorInRect(int32 x, int32 y, int32 width, int32 height)
{
    if (uis.cursorx < x || uis.cursory < y || uis.cursorx > x + width || uis.cursory > y + height) {
        return false;
    }

    return true;
}
