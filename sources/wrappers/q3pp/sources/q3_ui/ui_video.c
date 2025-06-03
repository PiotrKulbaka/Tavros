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
#include "ui_local.h"

void GraphicsOptions_MenuInit();

/*
=======================================================================

GRAPHICS OPTIONS MENU

=======================================================================
*/

#define GRAPHICSOPTIONS_FRAMEL  "menu/art/frame2_l"
#define GRAPHICSOPTIONS_FRAMER  "menu/art/frame1_r"
#define GRAPHICSOPTIONS_BACK0   "menu/art/back_0"
#define GRAPHICSOPTIONS_BACK1   "menu/art/back_1"
#define GRAPHICSOPTIONS_ACCEPT0 "menu/art/accept_0"
#define GRAPHICSOPTIONS_ACCEPT1 "menu/art/accept_1"

#define ID_BACK2                101
#define ID_FULLSCREEN           102
#define ID_LIST                 103
#define ID_MODE                 104
#define ID_GRAPHICS             106
#define ID_DISPLAY              107
#define ID_SOUND                108
#define ID_NETWORK              109

typedef struct
{
    menuframework_s menu;

    menutext_s   banner;
    menubitmap_s framel;
    menubitmap_s framer;

    menutext_s graphics;
    menutext_s display;
    menutext_s sound;
    menutext_s network;

    menulist_s list;
    menulist_s geometry;

    menubitmap_s apply;
    menubitmap_s back;
} graphicsoptions_t;

typedef struct
{
    int32 mode;
    int32 lighting;
    int32 geometry;
    bool  extensions;
} InitialVideoOptions_s;

static InitialVideoOptions_s s_ivo;
static graphicsoptions_t     s_graphicsoptions;

static InitialVideoOptions_s s_ivo_templates[] =
    {
        {4, 0, 1, true},
        {3, 0, 1, true},
        {2, 0, 0, true},
        {2, 1, 0, true},
        {3, 0, 1, true}
};

#define NUM_IVO_TEMPLATES (sizeof(s_ivo_templates) / sizeof(s_ivo_templates[0]))

/*
=================
GraphicsOptions_GetInitialVideo
=================
*/
static void GraphicsOptions_GetInitialVideo()
{
    s_ivo.geometry = s_graphicsoptions.geometry.curvalue;
}

/*
=================
GraphicsOptions_CheckConfig
=================
*/
static void GraphicsOptions_CheckConfig()
{
    int32 i;

    for (i = 0; i < NUM_IVO_TEMPLATES; i++) {
        if (s_ivo_templates[i].geometry != s_graphicsoptions.geometry.curvalue) {
            continue;
        }
        s_graphicsoptions.list.curvalue = i;
        return;
    }
    s_graphicsoptions.list.curvalue = 4;
}

/*
=================
GraphicsOptions_UpdateMenuItems
=================
*/
static void GraphicsOptions_UpdateMenuItems()
{
    s_graphicsoptions.apply.generic.flags |= QMF_HIDDEN | QMF_INACTIVE;

    if (s_ivo.geometry != s_graphicsoptions.geometry.curvalue) {
        s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN | QMF_INACTIVE);
    }

    GraphicsOptions_CheckConfig();
}

/*
=================
GraphicsOptions_ApplyChanges
=================
*/
static void GraphicsOptions_ApplyChanges(void* unused, int32 notification)
{
    if (notification != QM_ACTIVATED) {
        return;
    }

    if (s_graphicsoptions.geometry.curvalue == 2) {
        Cvar_SetValue("r_lodBias", 0);
        Cvar_SetValue("r_subdivisions", 4);
    } else if (s_graphicsoptions.geometry.curvalue == 1) {
        Cvar_SetValue("r_lodBias", 1);
        Cvar_SetValue("r_subdivisions", 12);
    } else {
        Cvar_SetValue("r_lodBias", 1);
        Cvar_SetValue("r_subdivisions", 20);
    }

    Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
}

/*
=================
GraphicsOptions_Event
=================
*/
static void GraphicsOptions_Event(void* ptr, int32 event)
{
    InitialVideoOptions_s* ivo;

    if (event != QM_ACTIVATED) {
        return;
    }

    switch (((menucommon_s*) ptr)->id) {
    case ID_MODE:
        break;

    case ID_LIST:
        ivo = &s_ivo_templates[s_graphicsoptions.list.curvalue];

        s_graphicsoptions.geometry.curvalue = ivo->geometry;
        break;

    case ID_BACK2:
        UI_PopMenu();
        break;

    case ID_GRAPHICS:
        break;

    case ID_DISPLAY:
        UI_PopMenu();
        UI_DisplayOptionsMenu();
        break;

    case ID_SOUND:
        UI_PopMenu();
        UI_SoundOptionsMenu();
        break;

    case ID_NETWORK:
        UI_PopMenu();
        UI_NetworkOptionsMenu();
        break;
    }
}

/*
================
GraphicsOptions_MenuDraw
================
*/
void GraphicsOptions_MenuDraw()
{
    // APSFIX - rework this
    GraphicsOptions_UpdateMenuItems();

    Menu_Draw(&s_graphicsoptions.menu);
}

/*
=================
GraphicsOptions_SetMenuItems
=================
*/
static void GraphicsOptions_SetMenuItems()
{
    if (Cvar_VariableValue("r_lodBias") > 0) {
        if (Cvar_VariableValue("r_subdivisions") >= 20) {
            s_graphicsoptions.geometry.curvalue = 0;
        } else {
            s_graphicsoptions.geometry.curvalue = 1;
        }
    } else {
        s_graphicsoptions.geometry.curvalue = 2;
    }
}

/*
================
GraphicsOptions_MenuInit
================
*/
void GraphicsOptions_MenuInit()
{
    static const char* tq_names[] =
        {
            "Default",
            "16 bit",
            "32 bit",
            0
        };

    static const char* s_graphics_options_names[] =
        {
            "High Quality",
            "Normal",
            "Fast",
            "Fastest",
            "Custom",
            0
        };

    static const char* lighting_names[] =
        {
            "Lightmap",
            "Vertex",
            0
        };

    static const char* resolutions[] =
        {
            "320x240",
            "400x300",
            "512x384",
            "640x480",
            "800x600",
            "960x720",
            "1024x768",
            "1152x864",
            "1280x1024",
            "1600x1200",
            "1366x768",
            "856x480 wide screen",
            0
        };

    static const char* quality_names[] =
        {
            "Low",
            "Medium",
            "High",
            0
        };
    static const char* enabled_names[] =
        {
            "Off",
            "On",
            0
        };

    int32 y;

    // zero set all our globals
    memset(&s_graphicsoptions, 0, sizeof(graphicsoptions_t));

    GraphicsOptions_Cache();

    s_graphicsoptions.menu.wrapAround = true;
    s_graphicsoptions.menu.fullscreen = true;
    s_graphicsoptions.menu.draw = GraphicsOptions_MenuDraw;

    s_graphicsoptions.banner.generic.type = MTYPE_BTEXT;
    s_graphicsoptions.banner.generic.x = 320;
    s_graphicsoptions.banner.generic.y = 16;
    s_graphicsoptions.banner.string = "SYSTEM SETUP";
    s_graphicsoptions.banner.color = color_white;
    s_graphicsoptions.banner.style = UI_CENTER;

    s_graphicsoptions.framel.generic.type = MTYPE_BITMAP;
    s_graphicsoptions.framel.generic.name = GRAPHICSOPTIONS_FRAMEL;
    s_graphicsoptions.framel.generic.flags = QMF_INACTIVE;
    s_graphicsoptions.framel.generic.x = 0;
    s_graphicsoptions.framel.generic.y = 78;
    s_graphicsoptions.framel.width = 256;
    s_graphicsoptions.framel.height = 329;

    s_graphicsoptions.framer.generic.type = MTYPE_BITMAP;
    s_graphicsoptions.framer.generic.name = GRAPHICSOPTIONS_FRAMER;
    s_graphicsoptions.framer.generic.flags = QMF_INACTIVE;
    s_graphicsoptions.framer.generic.x = 376;
    s_graphicsoptions.framer.generic.y = 76;
    s_graphicsoptions.framer.width = 256;
    s_graphicsoptions.framer.height = 334;

    s_graphicsoptions.graphics.generic.type = MTYPE_PTEXT;
    s_graphicsoptions.graphics.generic.flags = QMF_RIGHT_JUSTIFY;
    s_graphicsoptions.graphics.generic.id = ID_GRAPHICS;
    s_graphicsoptions.graphics.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.graphics.generic.x = 216;
    s_graphicsoptions.graphics.generic.y = 240 - 2 * PROP_HEIGHT;
    s_graphicsoptions.graphics.string = "GRAPHICS";
    s_graphicsoptions.graphics.style = UI_RIGHT;
    s_graphicsoptions.graphics.color = color_red;

    s_graphicsoptions.display.generic.type = MTYPE_PTEXT;
    s_graphicsoptions.display.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_graphicsoptions.display.generic.id = ID_DISPLAY;
    s_graphicsoptions.display.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.display.generic.x = 216;
    s_graphicsoptions.display.generic.y = 240 - PROP_HEIGHT;
    s_graphicsoptions.display.string = "DISPLAY";
    s_graphicsoptions.display.style = UI_RIGHT;
    s_graphicsoptions.display.color = color_red;

    s_graphicsoptions.sound.generic.type = MTYPE_PTEXT;
    s_graphicsoptions.sound.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_graphicsoptions.sound.generic.id = ID_SOUND;
    s_graphicsoptions.sound.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.sound.generic.x = 216;
    s_graphicsoptions.sound.generic.y = 240;
    s_graphicsoptions.sound.string = "SOUND";
    s_graphicsoptions.sound.style = UI_RIGHT;
    s_graphicsoptions.sound.color = color_red;

    s_graphicsoptions.network.generic.type = MTYPE_PTEXT;
    s_graphicsoptions.network.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_graphicsoptions.network.generic.id = ID_NETWORK;
    s_graphicsoptions.network.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.network.generic.x = 216;
    s_graphicsoptions.network.generic.y = 240 + PROP_HEIGHT;
    s_graphicsoptions.network.string = "NETWORK";
    s_graphicsoptions.network.style = UI_RIGHT;
    s_graphicsoptions.network.color = color_red;

    y = 240 - 6 * (BIGCHAR_HEIGHT + 2);
    s_graphicsoptions.list.generic.type = MTYPE_SPINCONTROL;
    s_graphicsoptions.list.generic.name = "Graphics Settings:";
    s_graphicsoptions.list.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
    s_graphicsoptions.list.generic.x = 400;
    s_graphicsoptions.list.generic.y = y;
    s_graphicsoptions.list.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.list.generic.id = ID_LIST;
    s_graphicsoptions.list.itemnames = s_graphics_options_names;
    y += 2 * (BIGCHAR_HEIGHT + 2);

    // references/modifies "r_lodBias" & "subdivisions"
    s_graphicsoptions.geometry.generic.type = MTYPE_SPINCONTROL;
    s_graphicsoptions.geometry.generic.name = "Geometric Detail:";
    s_graphicsoptions.geometry.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
    s_graphicsoptions.geometry.generic.x = 400;
    s_graphicsoptions.geometry.generic.y = y;
    s_graphicsoptions.geometry.itemnames = quality_names;
    y += BIGCHAR_HEIGHT + 2;

    s_graphicsoptions.back.generic.type = MTYPE_BITMAP;
    s_graphicsoptions.back.generic.name = GRAPHICSOPTIONS_BACK0;
    s_graphicsoptions.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_graphicsoptions.back.generic.callback = GraphicsOptions_Event;
    s_graphicsoptions.back.generic.id = ID_BACK2;
    s_graphicsoptions.back.generic.x = 0;
    s_graphicsoptions.back.generic.y = 480 - 64;
    s_graphicsoptions.back.width = 128;
    s_graphicsoptions.back.height = 64;
    s_graphicsoptions.back.focuspic = GRAPHICSOPTIONS_BACK1;

    s_graphicsoptions.apply.generic.type = MTYPE_BITMAP;
    s_graphicsoptions.apply.generic.name = GRAPHICSOPTIONS_ACCEPT0;
    s_graphicsoptions.apply.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_HIDDEN | QMF_INACTIVE;
    s_graphicsoptions.apply.generic.callback = GraphicsOptions_ApplyChanges;
    s_graphicsoptions.apply.generic.x = 640;
    s_graphicsoptions.apply.generic.y = 480 - 64;
    s_graphicsoptions.apply.width = 128;
    s_graphicsoptions.apply.height = 64;
    s_graphicsoptions.apply.focuspic = GRAPHICSOPTIONS_ACCEPT1;

    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.banner);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.framel);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.framer);

    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.graphics);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.display);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.sound);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.network);

    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.list);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.geometry);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.back);
    Menu_AddItem(&s_graphicsoptions.menu, (void*) &s_graphicsoptions.apply);

    GraphicsOptions_SetMenuItems();
    GraphicsOptions_GetInitialVideo();
}


/*
=================
GraphicsOptions_Cache
=================
*/
void GraphicsOptions_Cache()
{
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_FRAMEL);
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_FRAMER);
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_BACK0);
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_BACK1);
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_ACCEPT0);
    RE_RegisterShaderNoMip(GRAPHICSOPTIONS_ACCEPT1);
}


/*
=================
UI_GraphicsOptionsMenu
=================
*/
void UI_GraphicsOptionsMenu()
{
    GraphicsOptions_MenuInit();
    UI_PushMenu(&s_graphicsoptions.menu);
    Menu_SetCursorToItem(&s_graphicsoptions.menu, &s_graphicsoptions.graphics);
}

