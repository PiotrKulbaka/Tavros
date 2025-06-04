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

#include "../game/q_shared.h"
#include "../cgame/tr_types.h"
// NOTE: include the ui_public.h from the new UI
#include "../q3_ui/ui_public.h"
#include "../q3_ui/keycodes.h"
#include "../game/bg_public.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"

//
// ui_qmenu.c
//

#define SLIDER_RANGE           10
#define MAX_EDIT_LINE          256

#define MAX_MENUDEPTH          8
#define MAX_MENUITEMS          64

#define MTYPE_NULL             0
#define MTYPE_SLIDER           1
#define MTYPE_ACTION           2
#define MTYPE_SPINCONTROL      3
#define MTYPE_FIELD            4
#define MTYPE_RADIOBUTTON      5
#define MTYPE_BITMAP           6
#define MTYPE_TEXT             7
#define MTYPE_SCROLLLIST       8
#define MTYPE_PTEXT            9
#define MTYPE_BTEXT            10

#define QMF_BLINK              0x00000001
#define QMF_SMALLFONT          0x00000002
#define QMF_LEFT_JUSTIFY       0x00000004
#define QMF_CENTER_JUSTIFY     0x00000008
#define QMF_RIGHT_JUSTIFY      0x00000010
#define QMF_NUMBERSONLY        0x00000020 // edit field is only numbers
#define QMF_HIGHLIGHT          0x00000040
#define QMF_HIGHLIGHT_IF_FOCUS 0x00000080 // steady focus
#define QMF_PULSEIFFOCUS       0x00000100 // pulse if focus
#define QMF_HASMOUSEFOCUS      0x00000200
#define QMF_NOONOFFTEXT        0x00000400
#define QMF_MOUSEONLY          0x00000800 // only mouse input allowed
#define QMF_HIDDEN             0x00001000 // skips drawing
#define QMF_GRAYED             0x00002000 // grays and disables
#define QMF_INACTIVE           0x00004000 // disables any input
#define QMF_NODEFAULTINIT      0x00008000 // skip default initialization
#define QMF_OWNERDRAW          0x00010000
#define QMF_PULSE              0x00020000
#define QMF_LOWERCASE          0x00040000 // edit field is all lower case
#define QMF_UPPERCASE          0x00080000 // edit field is all upper case
#define QMF_SILENT             0x00100000

// callback notifications
#define QM_GOTFOCUS            1
#define QM_LOSTFOCUS           2
#define QM_ACTIVATED           3

#define ART_FRAME              "menu/art/cut_frame"

typedef struct _tag_menuframework
{
    int32 cursor;
    int32 cursor_prev;

    int32 nitems;
    void* items[MAX_MENUITEMS];

    void        (*draw)();
    sfxHandle_t (*key)(int32 key);

    bool wrapAround;
    bool fullscreen;
    bool showlogo;
} menuframework_s;

typedef struct
{
    int32            type;
    const char*      name;
    int32            id;
    int32            x, y;
    int32            left;
    int32            top;
    int32            right;
    int32            bottom;
    menuframework_s* parent;
    int32            menuPosition;
    uint32           flags;

    void (*callback)(void* self, int32 event);
    void (*statusbar)(void* self);
    void (*ownerdraw)(void* self);
} menucommon_s;

typedef struct
{
    int32 cursor;
    int32 scroll;
    int32 widthInChars;
    char  buffer[MAX_EDIT_LINE];
    int32 maxchars;
} mfield_t;

typedef struct
{
    menucommon_s generic;
    mfield_t     field;
} menufield_s;

typedef struct
{
    menucommon_s generic;

    float minvalue;
    float maxvalue;
    float curvalue;

    float range;
} menuslider_s;

typedef struct
{
    menucommon_s generic;

    int32 oldvalue;
    int32 curvalue;
    int32 numitems;
    int32 top;

    const char** itemnames;

    int32 width;
    int32 height;
    int32 columns;
    int32 seperation;
} menulist_s;

typedef struct
{
    menucommon_s generic;
} menuaction_s;

typedef struct
{
    menucommon_s generic;
    int32        curvalue;
} menuradiobutton_s;

typedef struct
{
    menucommon_s generic;
    const char*  focuspic;
    const char*  errorpic;
    qhandle_t    shader;
    qhandle_t    focusshader;
    int32        width;
    int32        height;
    float*       focuscolor;
} menubitmap_s;

typedef struct
{
    menucommon_s generic;
    const char*  string;
    int32        style;
    float*       color;
} menutext_s;

extern void        Menu_Cache();
extern void        Menu_AddItem(menuframework_s* menu, void* item);
extern void        Menu_AdjustCursor(menuframework_s* menu, int32 dir);
extern void        Menu_Draw(menuframework_s* menu);
extern void*       Menu_ItemAtCursor(menuframework_s* m);
extern sfxHandle_t Menu_ActivateItem(menuframework_s* s, menucommon_s* item);
extern void        Menu_SetCursor(menuframework_s* s, int32 cursor);
extern void        Menu_SetCursorToItem(menuframework_s* m, void* ptr);
extern sfxHandle_t Menu_DefaultKey(menuframework_s* s, int32 key);
extern void        Bitmap_Init(menubitmap_s* b);
extern void        Bitmap_Draw(menubitmap_s* b);
extern void        ScrollList_Draw(menulist_s* l);
extern sfxHandle_t ScrollList_Key(menulist_s* l, int32 key);
extern sfxHandle_t menu_in_sound;
extern sfxHandle_t menu_move_sound;
extern sfxHandle_t menu_out_sound;
extern sfxHandle_t menu_buzz_sound;
extern sfxHandle_t menu_null_sound;
extern sfxHandle_t weaponChangeSound;
extern vec4_t      menu_text_color;
extern vec4_t      color_black;
extern vec4_t      color_white;
extern vec4_t      color_yellow;
extern vec4_t      color_orange;
extern vec4_t      color_red;
extern vec4_t      listbar_color;
extern vec4_t      text_color_disabled;
extern vec4_t      text_color_normal;
extern vec4_t      text_color_highlight;

extern const char* ui_medalNames[];
extern const char* ui_medalPicNames[];
extern const char* ui_medalSounds[];

//
// ui_mfield.c
//
extern void        MField_Clear(mfield_t* edit);
extern void        MField_KeyDownEvent(mfield_t* edit, int32 key);
extern void        MField_CharEvent(mfield_t* edit, int32 ch);
extern void        MField_Draw(mfield_t* edit, int32 x, int32 y, int32 style, vec4_t color);
extern void        MenuField_Init(menufield_s* m);
extern void        MenuField_Draw(menufield_s* f);
extern sfxHandle_t MenuField_Key(menufield_s* m, int32* key);

//
// ui_menu.c
//
extern void MainMenu_Cache();
extern void UI_MainMenu();
extern void UI_RegisterCvars();

//
// ui_ingame.c
//
extern void InGame_Cache();
extern void UI_InGameMenu();

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache();
extern void UI_ConfirmMenu(const char* question, void (*draw)(), void (*action)(bool result));
extern void UI_ConfirmMenu_Style(const char* question, int32 style, void (*draw)(), void (*action)(bool result));
extern void UI_Message(const char** lines);

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache();
extern void UI_SetupMenu();

//
// ui_team.c
//
extern void UI_TeamMainMenu();
extern void TeamMain_Cache();

//
// ui_controls2.c
//
extern void UI_ControlsMenu();
extern void Controls_Cache();

//
// ui_demo2.c
//
extern void UI_DemosMenu();
extern void Demos_Cache();

//
// ui_playermodel.c
//
extern void UI_PlayerModelMenu();
extern void PlayerModel_Cache();

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu();
extern void PlayerSettings_Cache();

//
// ui_preferences.c
//
extern void UI_PreferencesMenu();
extern void Preferences_Cache();

//
// ui_specifyserver.c
//
extern void UI_SpecifyServerMenu();
extern void SpecifyServer_Cache();

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu();
extern void ArenaServers_Cache();

//
// ui_startserver.c
//
extern void UI_StartServerMenu(bool multiplayer);
extern void StartServer_Cache();
extern void ServerOptions_Cache();
extern void UI_BotSelectMenu(char* bot);
extern void UI_BotSelectMenu_Cache();

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu();
extern void ServerInfo_Cache();

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu();
extern void GraphicsOptions_Cache();

//
// ui_players.c
//

// FIXME ripped from cg_local.h
typedef struct
{
    int32 oldFrame;
    int32 oldFrameTime; // time when ->oldFrame was exactly on

    int32 frame;
    int32 frameTime; // time when ->frame will be exactly on

    float backlerp;

    float yawAngle;
    bool  yawing;
    float pitchAngle;
    bool  pitching;

    int32        animationNumber; // may include ANIM_TOGGLEBIT
    animation_t* animation;
    int32        animationTime;   // time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct
{
    // model info
    qhandle_t   legsModel;
    qhandle_t   legsSkin;
    lerpFrame_t legs;

    qhandle_t   torsoModel;
    qhandle_t   torsoSkin;
    lerpFrame_t torso;

    qhandle_t headModel;
    qhandle_t headSkin;

    animation_t animations[MAX_ANIMATIONS];

    qhandle_t weaponModel;
    qhandle_t barrelModel;
    qhandle_t flashModel;
    vec3_t    flashDlightColor;
    int32     muzzleFlashTime;

    // currently in use drawing parms
    vec3_t   viewAngles;
    vec3_t   moveAngles;
    weapon_t currentWeapon;
    int32    legsAnim;
    int32    torsoAnim;

    // animation vars
    weapon_t weapon;
    weapon_t lastWeapon;
    weapon_t pendingWeapon;
    int32    weaponTimer;
    int32    pendingLegsAnim;
    int32    torsoAnimationTimer;

    int32 pendingTorsoAnim;
    int32 legsAnimationTimer;

    bool chat;
    bool newModel;

    bool  barrelSpinning;
    float barrelAngle;
    int32 barrelTime;

    int32 realWeapon;
} playerInfo_t;

void UI_DrawPlayer(float x, float y, float w, float h, playerInfo_t* pi, int32 time);
void UI_PlayerInfo_SetModel(playerInfo_t* pi, const char* model);
void UI_PlayerInfo_SetInfo(playerInfo_t* pi, int32 legsAnim, int32 torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, bool chat);
bool UI_RegisterClientModelname(playerInfo_t* pi, const char* modelSkinName);

//
// ui_atoms.c
//
typedef struct
{
    int32            frametime;
    int32            realtime;
    int32            cursorx;
    int32            cursory;
    int32            menusp;
    menuframework_s* activemenu;
    menuframework_s* stack[MAX_MENUDEPTH];
    glconfig_t       glconfig;
    qhandle_t        whiteShader;
    qhandle_t        menuBackShader;
    qhandle_t        menuBackNoLogoShader;
    qhandle_t        charset;
    qhandle_t        charsetProp;
    qhandle_t        charsetPropGlow;
    qhandle_t        charsetPropB;
    qhandle_t        cursor;
    qhandle_t        rb_on;
    qhandle_t        rb_off;
    float            scale;
    float            bias;
    bool             firstdraw;
} uiStatic_t;

extern float      UI_ClampCvar(float min, float max, float value);
extern void       UI_DrawNamedPic(float x, float y, float width, float height, const char* picname);
extern void       UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader);
extern void       UI_FillRect(float x, float y, float width, float height, const float* color);
extern void       UI_DrawRect(float x, float y, float width, float height, const float* color);
extern void       UI_SetColor(const float* rgba);
extern void       UI_DrawBannerString(int32 x, int32 y, const char* str, int32 style, vec4_t color);
extern float      UI_ProportionalSizeScale(int32 style);
extern void       UI_DrawProportionalString(int32 x, int32 y, const char* str, int32 style, vec4_t color);
int32             UI_ProportionalStringWidth(const char* str);
extern void       UI_DrawProportionalString_AutoWrapped(int32 x, int32 ystart, int32 xmax, int32 ystep, const char* str, int32 style, vec4_t color);
extern void       UI_DrawString(int32 x, int32 y, const char* str, int32 style, vec4_t color);
extern void       UI_DrawChar(int32 x, int32 y, int32 ch, int32 style, vec4_t color);
extern bool       UI_CursorInRect(int32 x, int32 y, int32 width, int32 height);
extern void       UI_AdjustFrom640(float* x, float* y, float* w, float* h);
extern void       UI_PushMenu(menuframework_s* menu);
extern void       UI_PopMenu();
extern void       UI_ForceMenuOff();
extern char*      UI_Argv(int32 arg);
extern char*      UI_Cvar_VariableString(const char* var_name);
extern bool       m_entersound;
extern uiStatic_t uis;

//
// ui_spLevel.c
//
void UI_SPLevelMenu_Cache();
void UI_SPLevelMenu();
void UI_SPLevelMenu_f();
void UI_SPLevelMenu_ReInit();

//
// ui_spArena.c
//
void UI_SPArena_Start(const char* arenaInfo);

//
// ui_spPostgame.c
//
void UI_SPPostgameMenu_Cache();
void UI_SPPostgameMenu_f();

//
// ui_spSkill.c
//
void UI_SPSkillMenu(const char* arenaInfo);
void UI_SPSkillMenu_Cache();

//
// ui_syscalls.c
//
void  __UI_GetClientState(uiClientState_t* state);
int32 __UI_GetConfigString(int32 index, char* buff, int32 buffsize);

//
// ui_addbots.c
//
void UI_AddBots_Cache();
void UI_AddBotsMenu();

//
// ui_removebots.c
//
void UI_RemoveBots_Cache();
void UI_RemoveBotsMenu();

//
// ui_teamorders.c
//
extern void UI_TeamOrdersMenu();
extern void UI_TeamOrdersMenu_f();
extern void UI_TeamOrdersMenu_Cache();

//
// ui_display.c
//
void UI_DisplayOptionsMenu_Cache();
void UI_DisplayOptionsMenu();

//
// ui_sound.c
//
void UI_SoundOptionsMenu_Cache();
void UI_SoundOptionsMenu();

//
// ui_network.c
//
void UI_NetworkOptionsMenu_Cache();
void UI_NetworkOptionsMenu();

//
// ui_gameinfo.c
//
typedef enum
{
    AWARD_ACCURACY,
    AWARD_IMPRESSIVE,
    AWARD_EXCELLENT,
    AWARD_GAUNTLET,
    AWARD_FRAGS,
    AWARD_PERFECT
} awardType_t;

const char* UI_GetArenaInfoByNumber(int32 num);
const char* UI_GetArenaInfoByMap(const char* map);
const char* UI_GetSpecialArenaInfo(const char* tag);
int32       UI_GetNumArenas();
int32       UI_GetNumSPArenas();
int32       UI_GetNumSPTiers();

char* UI_GetBotInfoByNumber(int32 num);
char* UI_GetBotInfoByName(const char* name);
int32 UI_GetNumBots();

void  UI_GetBestScore(int32 level, int32* score, int32* skill);
void  UI_SetBestScore(int32 level, int32 score);
int32 UI_TierCompleted(int32 levelWon);
int32 UI_GetCurrentGame();
void  UI_NewGame();
void  UI_LogAwardData(int32 award, int32 data);
int32 UI_GetAwardLevel(int32 award);

void UI_SPUnlock_f();
void UI_SPUnlockMedals_f();

void UI_InitGameinfo();
