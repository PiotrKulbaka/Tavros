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

GAME OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_FRAMEL        "menu/art/frame2_l"
#define ART_FRAMER        "menu/art/frame1_r"
#define ART_BACK0         "menu/art/back_0"
#define ART_BACK1         "menu/art/back_1"

#define PREFERENCES_X_POS 360

#define ID_ALLOWDOWNLOAD  137
#define ID_BACK           138


typedef struct
{
    menuframework_s menu;

    menutext_s   banner;
    menubitmap_s framel;
    menubitmap_s framer;

    menuradiobutton_s allowdownload;
    menubitmap_s      back;
} preferences_t;

static preferences_t s_preferences;

static void Preferences_SetMenuItems()
{
    s_preferences.allowdownload.curvalue = Cvar_VariableValue("cl_allowDownload") != 0;
}


static void Preferences_Event(void* ptr, int32 notification)
{
    if (notification != QM_ACTIVATED) {
        return;
    }

    switch (((menucommon_s*) ptr)->id) {
    case ID_ALLOWDOWNLOAD:
        Cvar_SetValue("cl_allowDownload", s_preferences.allowdownload.curvalue);
        Cvar_SetValue("sv_allowDownload", s_preferences.allowdownload.curvalue);
        break;

    case ID_BACK:
        UI_PopMenu();
        break;
    }
}

static void Preferences_MenuInit()
{
    int32 y;

    memset(&s_preferences, 0, sizeof(preferences_t));

    Preferences_Cache();

    s_preferences.menu.wrapAround = true;
    s_preferences.menu.fullscreen = true;

    s_preferences.banner.generic.type = MTYPE_BTEXT;
    s_preferences.banner.generic.x = 320;
    s_preferences.banner.generic.y = 16;
    s_preferences.banner.string = "GAME OPTIONS";
    s_preferences.banner.color = color_white;
    s_preferences.banner.style = UI_CENTER;

    s_preferences.framel.generic.type = MTYPE_BITMAP;
    s_preferences.framel.generic.name = ART_FRAMEL;
    s_preferences.framel.generic.flags = QMF_INACTIVE;
    s_preferences.framel.generic.x = 0;
    s_preferences.framel.generic.y = 78;
    s_preferences.framel.width = 256;
    s_preferences.framel.height = 329;

    s_preferences.framer.generic.type = MTYPE_BITMAP;
    s_preferences.framer.generic.name = ART_FRAMER;
    s_preferences.framer.generic.flags = QMF_INACTIVE;
    s_preferences.framer.generic.x = 376;
    s_preferences.framer.generic.y = 76;
    s_preferences.framer.width = 256;
    s_preferences.framer.height = 334;

    y = 144;

    y += BIGCHAR_HEIGHT + 2;
    s_preferences.allowdownload.generic.type = MTYPE_RADIOBUTTON;
    s_preferences.allowdownload.generic.name = "Automatic Downloading:";
    s_preferences.allowdownload.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
    s_preferences.allowdownload.generic.callback = Preferences_Event;
    s_preferences.allowdownload.generic.id = ID_ALLOWDOWNLOAD;
    s_preferences.allowdownload.generic.x = PREFERENCES_X_POS;
    s_preferences.allowdownload.generic.y = y;

    y += BIGCHAR_HEIGHT + 2;
    s_preferences.back.generic.type = MTYPE_BITMAP;
    s_preferences.back.generic.name = ART_BACK0;
    s_preferences.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_preferences.back.generic.callback = Preferences_Event;
    s_preferences.back.generic.id = ID_BACK;
    s_preferences.back.generic.x = 0;
    s_preferences.back.generic.y = 480 - 64;
    s_preferences.back.width = 128;
    s_preferences.back.height = 64;
    s_preferences.back.focuspic = ART_BACK1;

    Menu_AddItem(&s_preferences.menu, &s_preferences.banner);
    Menu_AddItem(&s_preferences.menu, &s_preferences.framel);
    Menu_AddItem(&s_preferences.menu, &s_preferences.framer);

    Menu_AddItem(&s_preferences.menu, &s_preferences.allowdownload);

    Menu_AddItem(&s_preferences.menu, &s_preferences.back);

    Preferences_SetMenuItems();
}


/*
===============
Preferences_Cache
===============
*/
void Preferences_Cache()
{
    RE_RegisterShaderNoMip(ART_FRAMEL);
    RE_RegisterShaderNoMip(ART_FRAMER);
    RE_RegisterShaderNoMip(ART_BACK0);
    RE_RegisterShaderNoMip(ART_BACK1);
}


/*
===============
UI_PreferencesMenu
===============
*/
void UI_PreferencesMenu()
{
    Preferences_MenuInit();
    UI_PushMenu(&s_preferences.menu);
}
