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
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER            10
#define ID_MULTIPLAYER             11
#define ID_SETUP                   12
#define ID_DEMOS                   13
#define ID_EXIT                    17

#define MAIN_BANNER_MODEL          "models/mapobjects/banner/banner5.md3"
#define MAIN_MENU_VERTICAL_SPACING 34


typedef struct
{
    menuframework_s menu;

    menutext_s singleplayer;
    menutext_s multiplayer;
    menutext_s setup;
    menutext_s demos;
    menutext_s exit;

    qhandle_t bannerModel;
} mainmenu_t;


static mainmenu_t s_main;

typedef struct
{
    menuframework_s menu;
    char            errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction(bool result)
{
    if (!result) {
        return;
    }
    UI_PopMenu();
    Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
}


/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent(void* ptr, int32 event)
{
    if (event != QM_ACTIVATED) {
        return;
    }

    switch (((menucommon_s*) ptr)->id) {
    case ID_SINGLEPLAYER:
        UI_SPLevelMenu();
        break;

    case ID_MULTIPLAYER:
        UI_ArenaServersMenu();
        break;

    case ID_SETUP:
        UI_SetupMenu();
        break;

    case ID_DEMOS:
        UI_DemosMenu();
        break;

    case ID_EXIT:
        UI_ConfirmMenu("EXIT GAME?", NULL, MainMenu_ExitAction);
        break;
    }
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache()
{
    s_main.bannerModel = RE_RegisterModel(MAIN_BANNER_MODEL);
}

sfxHandle_t ErrorMessage_Key(int32 key)
{
    Cvar_Set("com_errorMessage", "");
    UI_MainMenu();
    return (menu_null_sound);
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw()
{
    refdef_t           refdef;
    refEntity_t        ent;
    vec3_t             origin;
    vec3_t             angles;
    float              adjust;
    float              x, y, w, h;
    tavros::math::vec4 color = {0.5, 0, 0, 1};

    // setup the refdef

    memset(&refdef, 0, sizeof(refdef));

    refdef.rdflags = RDF_NOWORLDMODEL;

    AxisClear(refdef.viewaxis);

    x = 0;
    y = 0;
    w = 640;
    h = 120;
    UI_AdjustFrom640(&x, &y, &w, &h);
    refdef.x = x;
    refdef.y = y;
    refdef.width = w;
    refdef.height = h;

    adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
    refdef.fov_x = 60 + adjust;
    refdef.fov_y = 19.6875 + adjust;

    refdef.time = uis.realtime;

    origin[0] = 300;
    origin[1] = 0;
    origin[2] = -32;

    RE_ClearScene();

    // add the model

    memset(&ent, 0, sizeof(ent));

    adjust = 5.0 * sin((float) uis.realtime / 5000);
    VectorSet(angles, 0, 180 + adjust, 0);
    AnglesToAxis(angles, ent.axis);
    ent.hModel = s_main.bannerModel;
    VectorCopy(origin, ent.origin);
    VectorCopy(origin, ent.lightingOrigin);
    ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
    VectorCopy(ent.origin, ent.oldorigin);

    RE_AddRefEntityToScene(&ent);

    RE_RenderScene(&refdef);

    if (strlen(s_errorMessage.errorMessage)) {
        UI_DrawProportionalString_AutoWrapped(320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, menu_text_color);
    } else {
        // standard menu drawing
        Menu_Draw(&s_main.menu);
    }

    UI_DrawString(320, 450, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER | UI_SMALLFONT, color);
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu()
{
    int32 y;
    int32 style = UI_CENTER | UI_DROPSHADOW;

    Cvar_Set("sv_killserver", "1");

    memset(&s_main, 0, sizeof(mainmenu_t));
    memset(&s_errorMessage, 0, sizeof(errorMessage_t));

    // com_errorMessage would need that too
    MainMenu_Cache();

    Cvar_VariableStringBuffer("com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage));
    if (strlen(s_errorMessage.errorMessage)) {
        s_errorMessage.menu.draw = Main_MenuDraw;
        s_errorMessage.menu.key = ErrorMessage_Key;
        s_errorMessage.menu.fullscreen = true;
        s_errorMessage.menu.wrapAround = true;
        s_errorMessage.menu.showlogo = true;

        Key_SetCatcher(KEYCATCH_UI);
        uis.menusp = 0;
        UI_PushMenu(&s_errorMessage.menu);

        return;
    }

    s_main.menu.draw = Main_MenuDraw;
    s_main.menu.fullscreen = true;
    s_main.menu.wrapAround = true;
    s_main.menu.showlogo = true;

    y = 134;
    s_main.singleplayer.generic.type = MTYPE_PTEXT;
    s_main.singleplayer.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_main.singleplayer.generic.x = 320;
    s_main.singleplayer.generic.y = y;
    s_main.singleplayer.generic.id = ID_SINGLEPLAYER;
    s_main.singleplayer.generic.callback = Main_MenuEvent;
    s_main.singleplayer.string = "SINGLE PLAYER";
    s_main.singleplayer.color = color_red;
    s_main.singleplayer.style = style;

    y += MAIN_MENU_VERTICAL_SPACING;
    s_main.multiplayer.generic.type = MTYPE_PTEXT;
    s_main.multiplayer.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_main.multiplayer.generic.x = 320;
    s_main.multiplayer.generic.y = y;
    s_main.multiplayer.generic.id = ID_MULTIPLAYER;
    s_main.multiplayer.generic.callback = Main_MenuEvent;
    s_main.multiplayer.string = "MULTIPLAYER";
    s_main.multiplayer.color = color_red;
    s_main.multiplayer.style = style;

    y += MAIN_MENU_VERTICAL_SPACING;
    s_main.setup.generic.type = MTYPE_PTEXT;
    s_main.setup.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_main.setup.generic.x = 320;
    s_main.setup.generic.y = y;
    s_main.setup.generic.id = ID_SETUP;
    s_main.setup.generic.callback = Main_MenuEvent;
    s_main.setup.string = "SETUP";
    s_main.setup.color = color_red;
    s_main.setup.style = style;

    y += MAIN_MENU_VERTICAL_SPACING;
    s_main.demos.generic.type = MTYPE_PTEXT;
    s_main.demos.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_main.demos.generic.x = 320;
    s_main.demos.generic.y = y;
    s_main.demos.generic.id = ID_DEMOS;
    s_main.demos.generic.callback = Main_MenuEvent;
    s_main.demos.string = "DEMOS";
    s_main.demos.color = color_red;
    s_main.demos.style = style;

    y += MAIN_MENU_VERTICAL_SPACING;
    s_main.exit.generic.type = MTYPE_PTEXT;
    s_main.exit.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_main.exit.generic.x = 320;
    s_main.exit.generic.y = y;
    s_main.exit.generic.id = ID_EXIT;
    s_main.exit.generic.callback = Main_MenuEvent;
    s_main.exit.string = "EXIT";
    s_main.exit.color = color_red;
    s_main.exit.style = style;

    Menu_AddItem(&s_main.menu, &s_main.singleplayer);
    Menu_AddItem(&s_main.menu, &s_main.multiplayer);
    Menu_AddItem(&s_main.menu, &s_main.setup);
    Menu_AddItem(&s_main.menu, &s_main.demos);
    Menu_AddItem(&s_main.menu, &s_main.exit);

    Key_SetCatcher(KEYCATCH_UI);
    uis.menusp = 0;
    UI_PushMenu(&s_main.menu);
}
