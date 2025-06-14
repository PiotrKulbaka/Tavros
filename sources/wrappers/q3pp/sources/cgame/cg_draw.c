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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

int32 sortedTeamPlayers[TEAM_MAXOVERLAY];
int32 numSortedTeamPlayers;

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
static void CG_DrawField(int32 x, int32 y, int32 width, int32 value)
{
    char  num[16], *ptr;
    int32 l;
    int32 frame;

    if (width < 1) {
        return;
    }

    // draw number string
    if (width > 5) {
        width = 5;
    }

    switch (width) {
    case 1:
        value = value > 9 ? 9 : value;
        value = value < 0 ? 0 : value;
        break;
    case 2:
        value = value > 99 ? 99 : value;
        value = value < -9 ? -9 : value;
        break;
    case 3:
        value = value > 999 ? 999 : value;
        value = value < -99 ? -99 : value;
        break;
    case 4:
        value = value > 9999 ? 9999 : value;
        value = value < -999 ? -999 : value;
        break;
    }

    Com_sprintf(num, sizeof(num), "%i", value);
    l = strlen(num);
    if (l > width) {
        l = width;
    }
    x += 2 + CHAR_WIDTH * (width - l);

    ptr = num;
    while (*ptr && l) {
        if (*ptr == '-') {
            frame = STAT_MINUS;
        } else {
            frame = *ptr - '0';
        }

        CG_DrawPic(x, y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame]);
        x += CHAR_WIDTH;
        ptr++;
        l--;
    }
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles)
{
    refdef_t    refdef;
    refEntity_t ent;

    CG_AdjustFrom640(&x, &y, &w, &h);

    memset(&refdef, 0, sizeof(refdef));

    memset(&ent, 0, sizeof(ent));
    AnglesToAxis(angles, ent.axis);
    VectorCopy(origin, ent.origin);
    ent.hModel = model;
    ent.customSkin = skin;
    ent.renderfx = RF_NOSHADOW; // no stencil shadows

    refdef.rdflags = RDF_NOWORLDMODEL;

    AxisClear(refdef.viewaxis);

    refdef.fov_x = 30;
    refdef.fov_y = 30;

    refdef.x = x;
    refdef.y = y;
    refdef.width = w;
    refdef.height = h;

    refdef.time = cg.time;

    RE_ClearScene();
    RE_AddRefEntityToScene(&ent);
    RE_RenderScene(&refdef);
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead(float x, float y, float w, float h, int32 clientNum, vec3_t headAngles)
{
    clipHandle_t  cm;
    clientInfo_t* ci;
    float         len;
    vec3_t        origin;
    vec3_t        mins, maxs;

    ci = &cgs.clientinfo[clientNum];

    cm = ci->headModel;
    if (!cm) {
        return;
    }

    // offset the origin y and z to center the head
    R_ModelBounds(cm, mins, maxs);

    origin[2] = -0.5 * (mins[2] + maxs[2]);
    origin[1] = 0.5 * (mins[1] + maxs[1]);

    // calculate distance so the head nearly fills the box
    // assume heads are taller than wide
    len = 0.7 * (maxs[2] - mins[2]);
    origin[0] = len / 0.268; // len / tan( fov/2 )

    // allow per-model tweaking
    VectorAdd(origin, ci->headOffset, origin);

    CG_Draw3DModel(x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles);

    // if they are deferred, draw a cross out
    if (ci->deferred) {
        CG_DrawPic(x, y, w, h, cgs.media.deferShader);
    }
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int32 team)
{
    qhandle_t cm;
    float     len;
    vec3_t    origin, angles;
    vec3_t    mins, maxs;
    qhandle_t handle;


    // Draw 3d icons
    VectorClear(angles);

    cm = cgs.media.redFlagModel;

    // offset the origin y and z to center the flag
    R_ModelBounds(cm, mins, maxs);

    origin[2] = -0.5 * (mins[2] + maxs[2]);
    origin[1] = 0.5 * (mins[1] + maxs[1]);

    // calculate distance so the flag nearly fills the box
    // assume heads are taller than wide
    len = 0.5 * (maxs[2] - mins[2]);
    origin[0] = len / 0.268; // len / tan( fov/2 )

    angles[YAW] = 60 * sin(cg.time / 2000.0);
    ;

    if (team == TEAM_RED) {
        handle = cgs.media.redFlagModel;
    } else if (team == TEAM_BLUE) {
        handle = cgs.media.blueFlagModel;
    } else if (team == TEAM_FREE) {
        handle = cgs.media.neutralFlagModel;
    } else {
        return;
    }
    CG_Draw3DModel(x, y, w, h, handle, 0, origin, angles);
}

/*
================
CG_DrawStatusBarHead

================
*/
static void CG_DrawStatusBarHead(float x)
{
    vec3_t angles;
    float  size, stretch;
    float  frac;

    VectorClear(angles);

    if (cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME) {
        frac = (float) (cg.time - cg.damageTime) / DAMAGE_TIME;
        size = ICON_SIZE * 1.25 * (1.5 - frac * 0.5);

        stretch = size - ICON_SIZE * 1.25;
        // kick in the direction of damage
        x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

        cg.headStartYaw = 180 + cg.damageX * 45;

        cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
        cg.headEndPitch = 5 * cos(crandom() * M_PI);

        cg.headStartTime = cg.time;
        cg.headEndTime = cg.time + 100 + random() * 2000;
    } else {
        if (cg.time >= cg.headEndTime) {
            // select a new head angle
            cg.headStartYaw = cg.headEndYaw;
            cg.headStartPitch = cg.headEndPitch;
            cg.headStartTime = cg.headEndTime;
            cg.headEndTime = cg.time + 100 + random() * 2000;

            cg.headEndYaw = 180 + 20 * cos(crandom() * M_PI);
            cg.headEndPitch = 5 * cos(crandom() * M_PI);
        }

        size = ICON_SIZE * 1.25;
    }

    // if the server was frozen for a while we may have a bad head start time
    if (cg.headStartTime > cg.time) {
        cg.headStartTime = cg.time;
    }

    frac = (cg.time - cg.headStartTime) / (float) (cg.headEndTime - cg.headStartTime);
    frac = frac * frac * (3 - 2 * frac);
    angles[YAW] = cg.headStartYaw + (cg.headEndYaw - cg.headStartYaw) * frac;
    angles[PITCH] = cg.headStartPitch + (cg.headEndPitch - cg.headStartPitch) * frac;

    CG_DrawHead(x, 480 - size, size, size, cg.snap->ps.clientNum, angles);
}

/*
================
CG_DrawStatusBarFlag

================
*/
static void CG_DrawStatusBarFlag(float x, int32 team)
{
    CG_DrawFlagModel(x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team);
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground(int32 x, int32 y, int32 w, int32 h, float alpha, int32 team)
{
    tavros::math::vec4 color;

    if (team == TEAM_RED) {
        color = tavros::math::vec4(1.0, 0.0, 0.0, alpha);
    } else if (team == TEAM_BLUE) {
        color = tavros::math::vec4(0.0, 0.0, 1.0, alpha);
    } else {
        return;
    }
    RE_SetColor(color.data());
    CG_DrawPic(x, y, w, h, cgs.media.teamStatusBar);
    RE_SetColor(NULL);
}

/*
================
CG_DrawStatusBar

================
*/
static void CG_DrawStatusBar()
{
    int32              color;
    centity_t*         cent;
    playerState_t*     ps;
    int32              value;
    tavros::math::vec4 hcolor;
    vec3_t             angles;
    vec3_t             origin;
    static float       colors[4][4] = {
        //        { 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
        {1.0f, 0.69f, 0.0f, 1.0f}, // normal
        {1.0f, 0.2f, 0.2f, 1.0f},  // low health
        {0.5f, 0.5f, 0.5f, 1.0f},  // weapon firing
        {1.0f, 1.0f, 1.0f, 1.0f}
    };                             // health > 100

    // draw the team background
    CG_DrawTeamBackground(0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM]);

    cent = &cg_entities[cg.snap->ps.clientNum];
    ps = &cg.snap->ps;

    VectorClear(angles);

    // draw any 3D icons first, so the changes back to 2D are minimized
    if (cent->currentState.weapon && cg_weapons[cent->currentState.weapon].ammoModel) {
        origin[0] = 70;
        origin[1] = 0;
        origin[2] = 0;
        angles[YAW] = 90 + 20 * sin(cg.time / 1000.0);
        CG_Draw3DModel(CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cg_weapons[cent->currentState.weapon].ammoModel, 0, origin, angles);
    }

    CG_DrawStatusBarHead(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE);

    if (cg.predictedPlayerState.powerups[PW_REDFLAG]) {
        CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED);
    } else if (cg.predictedPlayerState.powerups[PW_BLUEFLAG]) {
        CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE);
    } else if (cg.predictedPlayerState.powerups[PW_NEUTRALFLAG]) {
        CG_DrawStatusBarFlag(185 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE);
    }

    if (ps->stats[STAT_ARMOR]) {
        origin[0] = 90;
        origin[1] = 0;
        origin[2] = -10;
        angles[YAW] = (cg.time & 2047) * 360 / 2048.0;
        CG_Draw3DModel(370 + CHAR_WIDTH * 3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorModel, 0, origin, angles);
    }

    //
    // ammo
    //
    if (cent->currentState.weapon) {
        value = ps->ammo[cent->currentState.weapon];
        if (value > -1) {
            if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
                && cg.predictedPlayerState.weaponTime > 100) {
                // draw as dark grey when reloading
                color = 2; // dark grey
            } else {
                if (value >= 0) {
                    color = 0; // green
                } else {
                    color = 1; // red
                }
            }
            RE_SetColor(colors[color]);

            CG_DrawField(0, 432, 3, value);
            RE_SetColor(NULL);
        }
    }

    //
    // health
    //
    value = ps->stats[STAT_HEALTH];
    if (value > 100) {
        RE_SetColor(colors[3]);     // white
    } else if (value > 25) {
        RE_SetColor(colors[0]);     // green
    } else if (value > 0) {
        color = (cg.time >> 8) & 1; // flash
        RE_SetColor(colors[color]);
    } else {
        RE_SetColor(colors[1]); // red
    }

    // stretch the health up when taking damage
    CG_DrawField(185, 432, 3, value);
    hcolor = CG_ColorForHealth();
    RE_SetColor(hcolor.data());


    //
    // armor
    //
    value = ps->stats[STAT_ARMOR];
    if (value > 0) {
        RE_SetColor(colors[0]);
        CG_DrawField(370, 432, 3, value);
        RE_SetColor(NULL);
    }
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores(float y)
{
    const char*        s;
    int32              s1, s2, score;
    int32              x, w;
    int32              v;
    tavros::math::vec4 color;
    float              y1;
    gitem_t*           item;

    s1 = cgs.scores1;
    s2 = cgs.scores2;

    y -= BIGCHAR_HEIGHT + 8;

    y1 = y;

    // draw from the right side to left
    if (cgs.gametype >= GT_TEAM) {
        x = 640;
        color = tavros::math::vec4(0.0f, 0.0f, 1.0f, 0.33f);
        s = va("%2i", s2);
        w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
        x -= w;
        CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
        if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
            CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
        }
        CG_DrawBigString(x + 4, y, s, 1.0F);

        if (cgs.gametype == GT_CTF) {
            // Display flag status
            item = BG_FindItemForPowerup(PW_BLUEFLAG);

            if (item) {
                y1 = y - BIGCHAR_HEIGHT - 8;
                if (cgs.blueflag >= 0 && cgs.blueflag <= 2) {
                    CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.blueFlagShader[cgs.blueflag]);
                }
            }
        }
        color = tavros::math::vec4(1.0f, 0.0f, 0.0f, 0.33f);
        s = va("%2i", s1);
        w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
        x -= w;
        CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
        if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
            CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
        }
        CG_DrawBigString(x + 4, y, s, 1.0F);

        if (cgs.gametype == GT_CTF) {
            // Display flag status
            item = BG_FindItemForPowerup(PW_REDFLAG);

            if (item) {
                y1 = y - BIGCHAR_HEIGHT - 8;
                if (cgs.redflag >= 0 && cgs.redflag <= 2) {
                    CG_DrawPic(x, y1 - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.redFlagShader[cgs.redflag]);
                }
            }
        }

        if (cgs.gametype >= GT_CTF) {
            v = cgs.capturelimit;
        } else {
            v = cgs.fraglimit;
        }
        if (v) {
            s = va("%2i", v);
            w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
            x -= w;
            CG_DrawBigString(x + 4, y, s, 1.0F);
        }

    } else {
        bool spectator;

        x = 640;
        score = cg.snap->ps.persistant[PERS_SCORE];
        spectator = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

        // always show your score in the second box if not in first place
        if (s1 != score) {
            s2 = score;
        }
        if (s2 != SCORE_NOT_PRESENT) {
            s = va("%2i", s2);
            w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
            x -= w;
            if (!spectator && score == s2 && score != s1) {
                color = tavros::math::vec4(1.0f, 0.0f, 0.0f, 0.33f);
                CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
                CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
            } else {
                color = tavros::math::vec4(0.5f, 0.5f, 0.5f, 0.33f);
                CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
            }
            CG_DrawBigString(x + 4, y, s, 1.0F);
        }

        // first place
        if (s1 != SCORE_NOT_PRESENT) {
            s = va("%2i", s1);
            w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
            x -= w;
            if (!spectator && score == s1) {
                color = tavros::math::vec4(0.0f, 0.0f, 1.0f, 0.33f);
                CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
                CG_DrawPic(x, y - 4, w, BIGCHAR_HEIGHT + 8, cgs.media.selectShader);
            } else {
                color = tavros::math::vec4(0.5f, 0.5f, 0.5f, 0.33f);
                CG_FillRect(x, y - 4, w, BIGCHAR_HEIGHT + 8, color.data());
            }
            CG_DrawBigString(x + 4, y, s, 1.0F);
        }

        if (cgs.fraglimit) {
            s = va("%2i", cgs.fraglimit);
            w = CG_DrawStrlen(s) * BIGCHAR_WIDTH + 8;
            x -= w;
            CG_DrawBigString(x + 4, y, s, 1.0F);
        }
    }

    return y1 - 8;
}

/*
================
CG_DrawPowerups
================
*/
static float CG_DrawPowerups(float y)
{
    int32          sorted[MAX_POWERUPS];
    int32          sortedTime[MAX_POWERUPS];
    int32          i, j, k;
    int32          active;
    playerState_t* ps;
    int32          t;
    gitem_t*       item;
    int32          x;
    int32          color;
    float          size;
    float          f;
    static float   colors[2][4] = {
        {0.2f, 1.0f, 0.2f, 1.0f},
        {1.0f, 0.2f, 0.2f, 1.0f}
    };

    ps = &cg.snap->ps;

    if (ps->stats[STAT_HEALTH] <= 0) {
        return y;
    }

    // sort the list by time remaining
    active = 0;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (!ps->powerups[i]) {
            continue;
        }
        t = ps->powerups[i] - cg.time;
        // ZOID--don't draw if the power up has unlimited time (999 seconds)
        // This is true of the CTF flags
        if (t < 0 || t > 999000) {
            continue;
        }

        // insert into the list
        for (j = 0; j < active; j++) {
            if (sortedTime[j] >= t) {
                for (k = active - 1; k >= j; k--) {
                    sorted[k + 1] = sorted[k];
                    sortedTime[k + 1] = sortedTime[k];
                }
                break;
            }
        }
        sorted[j] = i;
        sortedTime[j] = t;
        active++;
    }

    // draw the icons and timers
    x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
    for (i = 0; i < active; i++) {
        item = BG_FindItemForPowerup((powerup_t) sorted[i]);

        if (item) {
            color = 1;

            y -= ICON_SIZE;

            RE_SetColor(colors[color]);
            CG_DrawField(x, y, 2, sortedTime[i] / 1000);

            t = ps->powerups[sorted[i]];
            if (t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME) {
                RE_SetColor(NULL);
            } else {
                f = (float) (t - cg.time) / POWERUP_BLINK_TIME;
                f -= (int32) f;
                tavros::math::vec4 modulate(f);
                RE_SetColor(modulate.data());
            }

            if (cg.powerupActive == sorted[i] && cg.time - cg.powerupTime < PULSE_TIME) {
                f = 1.0 - (((float) cg.time - cg.powerupTime) / PULSE_TIME);
                size = ICON_SIZE * (1.0 + (PULSE_SCALE - 1.0) * f);
            } else {
                size = ICON_SIZE;
            }

            CG_DrawPic(640 - size, y + ICON_SIZE / 2 - size / 2, size, size, RE_RegisterShader(item->icon));
        }
    }
    RE_SetColor(NULL);

    return y;
}

/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight()
{
    float y;

    y = 480 - ICON_SIZE;

    y = CG_DrawScores(y);
    y = CG_DrawPowerups(y);
}

/*
===================
CG_DrawPickupItem
===================
*/
static int32 CG_DrawPickupItem(int32 y)
{
    if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
        return y;
    }

    y -= ICON_SIZE;

    auto value = cg.itemPickup;
    if (value) {
        auto fadeColor = CG_FadeColor(cg.itemPickupTime, 3000);
        if (fadeColor.x != 0.0f) {
            CG_RegisterItemVisuals(value);
            RE_SetColor(fadeColor.data());
            CG_DrawPic(8, y, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
            CG_DrawBigString(ICON_SIZE + 16, y + (ICON_SIZE / 2 - BIGCHAR_HEIGHT / 2), bg_itemlist[value].pickup_name, fadeColor.x);
            RE_SetColor(NULL);
        }
    }

    return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft()
{
    float y;

    y = 480 - ICON_SIZE;

    y = CG_DrawPickupItem(y);
}

/*
===================
CG_DrawHoldableItem
===================
*/
static void CG_DrawHoldableItem()
{
    int32 value;

    value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
    if (value) {
        CG_RegisterItemVisuals(value);
        CG_DrawPic(640 - ICON_SIZE, (SCREEN_HEIGHT - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, cg_items[value].icon);
    }
}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward()
{
    int32 i, count;
    float x, y;
    char  buf[32];

    auto fadeColor = CG_FadeColor(cg.rewardTime, REWARD_TIME);
    if (fadeColor.x != 0.0f) {
        if (cg.rewardStack > 0) {
            for (i = 0; i < cg.rewardStack; i++) {
                cg.rewardSound[i] = cg.rewardSound[i + 1];
                cg.rewardShader[i] = cg.rewardShader[i + 1];
                cg.rewardCount[i] = cg.rewardCount[i + 1];
            }
            cg.rewardTime = cg.time;
            cg.rewardStack--;
            fadeColor = CG_FadeColor(cg.rewardTime, REWARD_TIME);
            S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
        } else {
            return;
        }
    }

    RE_SetColor(fadeColor.data());

    if (cg.rewardCount[0] >= 10) {
        y = 56;
        x = 320 - ICON_SIZE / 2;
        CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
        Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
        x = (SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen(buf)) / 2;
        CG_DrawStringExt(x, y + ICON_SIZE, buf, fadeColor, false, true, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
    } else {
        count = cg.rewardCount[0];

        y = 56;
        x = 320 - count * ICON_SIZE / 2;
        for (i = 0; i < count; i++) {
            CG_DrawPic(x, y, ICON_SIZE - 4, ICON_SIZE - 4, cg.rewardShader[0]);
            x += ICON_SIZE;
        }
    }
    RE_SetColor(NULL);
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES 128


typedef struct
{
    int32 frameSamples[LAG_SAMPLES];
    int32 frameCount;
    int32 snapshotFlags[LAG_SAMPLES];
    int32 snapshotSamples[LAG_SAMPLES];
    int32 snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo()
{
    int32 offset;

    offset = cg.time - cg.latestSnapshotTime;
    lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
    lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t* snap)
{
    // dropped packet
    if (!snap) {
        lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
        lagometer.snapshotCount++;
        return;
    }

    // add this snapshot's info
    lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
    lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
    lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect()
{
    float       x, y;
    int32       cmdNum;
    usercmd_t   cmd;
    const char* s;
    int32       w; // bk010215 - FIXME char message[1024];

    // draw the phone jack if we are completely past our buffers
    cmdNum = CL_GetCurrentCmdNumber() - CMD_BACKUP + 1;
    CL_GetUserCmd(cmdNum, &cmd);
    if (cmd.serverTime <= cg.snap->ps.commandTime
        || cmd.serverTime > cg.time) { // special check for map_restart // bk 0102165 - FIXME
        return;
    }

    // also add text in center of screen
    s = "Connection Interrupted"; // bk 010215 - FIXME
    w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
    CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

    // blink the icon
    if ((cg.time >> 9) & 1) {
        return;
    }

    x = 640 - 48;
    y = 480 - 48;

    CG_DrawPic(x, y, 48, 48, RE_RegisterShader("gfx/2d/net.tga"));
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer()
{
    int32 a, x, y, i;
    float v;
    float ax, ay, aw, ah, mid, range;
    int32 color;
    float vscale;

    if (cgs.localServer) {
        CG_DrawDisconnect();
        return;
    }

    //
    // draw the graph
    //

    x = 640 - 48;
    y = 480 - 48;

    RE_SetColor(NULL);
    CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

    ax = x;
    ay = y;
    aw = 48;
    ah = 48;
    CG_AdjustFrom640(&ax, &ay, &aw, &ah);

    color = -1;
    range = ah / 3;
    mid = ay + range;

    vscale = range / MAX_LAGOMETER_RANGE;

    // draw the frame interpoalte / extrapolate graph
    for (a = 0; a < aw; a++) {
        i = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
        v = lagometer.frameSamples[i];
        v *= vscale;
        if (v > 0) {
            if (color != 1) {
                color = 1;
                RE_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)].data());
            }
            if (v > range) {
                v = range;
            }
            RE_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
        } else if (v < 0) {
            if (color != 2) {
                color = 2;
                RE_SetColor(g_color_table[ColorIndex(COLOR_BLUE)].data());
            }
            v = -v;
            if (v > range) {
                v = range;
            }
            RE_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
        }
    }

    // draw the snapshot latency / drop graph
    range = ah / 2;
    vscale = range / MAX_LAGOMETER_PING;

    for (a = 0; a < aw; a++) {
        i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
        v = lagometer.snapshotSamples[i];
        if (v > 0) {
            if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED) {
                if (color != 5) {
                    color = 5; // YELLOW for rate delay
                    RE_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)].data());
                }
            } else {
                if (color != 3) {
                    color = 3;
                    RE_SetColor(g_color_table[ColorIndex(COLOR_GREEN)].data());
                }
            }
            v = v * vscale;
            if (v > range) {
                v = range;
            }
            RE_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
        } else if (v < 0) {
            if (color != 4) {
                color = 4; // RED for dropped snapshots
                RE_SetColor(g_color_table[ColorIndex(COLOR_RED)].data());
            }
            RE_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
        }
    }

    RE_SetColor(NULL);

    if (cg_synchronousClients->integer) {
        CG_DrawBigString(ax, ay, "snc", 1.0);
    }

    CG_DrawDisconnect();
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint(const char* str, int32 y, int32 charWidth)
{
    char* s;

    Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

    cg.centerPrintTime = cg.time;
    cg.centerPrintY = y;
    cg.centerPrintCharWidth = charWidth;

    // count the number of lines for centering
    cg.centerPrintLines = 1;
    s = cg.centerPrint;
    while (*s) {
        if (*s == '\n') {
            cg.centerPrintLines++;
        }
        s++;
    }
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString()
{
    char* start;
    int32 l;
    int32 x, y, w;

    if (!cg.centerPrintTime) {
        return;
    }

    constexpr auto     time_to_show_central_messages = 3; // time to show messages to center screen
    tavros::math::vec4 fadeColor = CG_FadeColor(cg.centerPrintTime, 1000 * time_to_show_central_messages);
    if (fadeColor.x == 0.0f) {
        return;
    }

    RE_SetColor(fadeColor.data());

    start = cg.centerPrint;

    y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

    while (1) {
        char linebuffer[1024];

        for (l = 0; l < 50; l++) {
            if (!start[l] || start[l] == '\n') {
                break;
            }
            linebuffer[l] = start[l];
        }
        linebuffer[l] = 0;

        w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

        x = (SCREEN_WIDTH - w) / 2;

        CG_DrawStringExt(x, y, linebuffer, fadeColor, false, true, cg.centerPrintCharWidth, (int32) (cg.centerPrintCharWidth * 1.5), 0);

        y += cg.centerPrintCharWidth * 1.5;

        while (*start && (*start != '\n')) {
            start++;
        }
        if (!*start) {
            break;
        }
        start++;
    }

    RE_SetColor(NULL);
}


/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair()
{
    float w, h;
    float f;
    float x, y;
    int32 ca;

    if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
        return;
    }

    if (cg.renderingThirdPerson) {
        return;
    }

    // set color based on health
    auto hcolor = CG_ColorForHealth();
    RE_SetColor(hcolor.data());


    w = h = 20;

    // pulse the size of the crosshair when picking up items
    f = cg.time - cg.itemPickupBlendTime;
    if (f > 0 && f < ITEM_BLOB_TIME) {
        f /= ITEM_BLOB_TIME;
        w *= (1 + f);
        h *= (1 + f);
    }

    x = 0;
    y = 0;
    CG_AdjustFrom640(&x, &y, &w, &h);

    RE_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w), y + cg.refdef.y + 0.5 * (cg.refdef.height - h), w, h, 0, 0, 1, 1, cgs.media.crosshairShader);
}


/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity()
{
    trace_t trace;
    vec3_t  start, end;
    int32   content;

    VectorCopy(cg.refdef.vieworg, start);
    VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

    CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY);
    if (trace.entityNum >= MAX_CLIENTS) {
        return;
    }

    // if the player is in fog, don't show it
    content = CM_PointContents(trace.endpos, 0);
    if (content & CONTENTS_FOG) {
        return;
    }

    // if the player is invisible, don't show it
    if (cg_entities[trace.entityNum].currentState.powerups & (1 << PW_INVIS)) {
        return;
    }

    // update the fade timer
    cg.crosshairClientNum = trace.entityNum;
    cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames()
{
    char* name;
    float w;

    if (cg.renderingThirdPerson) {
        return;
    }

    // scan the known entities to see if the crosshair is sighted on one
    CG_ScanForCrosshairEntity();

    // draw the name of the player being looked at
    auto fadeColor = CG_FadeColor(cg.crosshairClientTime, 1000);
    if (fadeColor.x == 0.0f) {
        RE_SetColor(NULL);
        return;
    }

    name = cgs.clientinfo[cg.crosshairClientNum].name;
    w = CG_DrawStrlen(name) * BIGCHAR_WIDTH;
    CG_DrawBigString(320 - w / 2, 170, name, fadeColor.a * 0.5f);
    RE_SetColor(NULL);
}


/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator()
{
    CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
    if (cgs.gametype == GT_TOURNAMENT) {
        CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
    } else if (cgs.gametype >= GT_TEAM) {
        CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
    }
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote()
{
    char* s;
    int32 sec;

    if (!cgs.voteTime) {
        return;
    }

    // play a talk beep whenever it is modified
    if (cgs.voteModified) {
        cgs.voteModified = false;
        S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
    }

    sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
    if (sec < 0) {
        sec = 0;
    }

    s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
    CG_DrawSmallString(0, 58, s, 1.0F);
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote()
{
    char* s;
    int32 sec, cs_offset;

    if (cgs.clientinfo->team == TEAM_RED) {
        cs_offset = 0;
    } else if (cgs.clientinfo->team == TEAM_BLUE) {
        cs_offset = 1;
    } else {
        return;
    }

    if (!cgs.teamVoteTime[cs_offset]) {
        return;
    }

    // play a talk beep whenever it is modified
    if (cgs.teamVoteModified[cs_offset]) {
        cgs.teamVoteModified[cs_offset] = false;
        S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
    }

    sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
    if (sec < 0) {
        sec = 0;
    }
    s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset], cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
    CG_DrawSmallString(0, 90, s, 1.0F);
}


static bool CG_DrawScoreboard()
{
    return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission()
{
    if (cgs.gametype == GT_SINGLE_PLAYER) {
        CG_DrawCenterString();
        return;
    }
    cg.scoreFadeTime = cg.time;
    cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static bool CG_DrawFollow()
{
    float       x;
    const char* name;

    if (!(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
        return false;
    }
    CG_DrawBigString(320 - 9 * 8, 24, "following", 1.0F);

    name = cgs.clientinfo[cg.snap->ps.clientNum].name;

    x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(name));

    CG_DrawStringExt(x, 40, name, tavros::math::vec4(1.0f), true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);

    return true;
}


/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning()
{
    const char* s;
    int32       w;

    if (!cg.lowAmmoWarning) {
        return;
    }

    if (cg.lowAmmoWarning == 2) {
        s = "OUT OF AMMO";
    } else {
        s = "LOW AMMO WARNING";
    }
    w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
    CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup()
{
    int32         w;
    int32         sec;
    int32         i;
    float         scale;
    clientInfo_t *ci1, *ci2;
    int32         cw;
    const char*   s;

    sec = cg.warmup;
    if (!sec) {
        return;
    }

    if (sec < 0) {
        s = "Waiting for players";
        w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
        CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
        cg.warmupCount = 0;
        return;
    }

    if (cgs.gametype == GT_TOURNAMENT) {
        // find the two active players
        ci1 = NULL;
        ci2 = NULL;
        for (i = 0; i < cgs.maxclients; i++) {
            if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE) {
                if (!ci1) {
                    ci1 = &cgs.clientinfo[i];
                } else {
                    ci2 = &cgs.clientinfo[i];
                }
            }
        }

        if (ci1 && ci2) {
            s = va("%s vs %s", ci1->name, ci2->name);
            w = CG_DrawStrlen(s);
            if (w > 640 / GIANT_WIDTH) {
                cw = 640 / w;
            } else {
                cw = GIANT_WIDTH;
            }
            CG_DrawStringExt(320 - w * cw / 2, 20, s, tavros::math::vec4(1.0f), false, true, cw, (int32) (cw * 1.5f), 0);
        }
    } else {
        if (cgs.gametype == GT_FFA) {
            s = "Free For All";
        } else if (cgs.gametype == GT_TEAM) {
            s = "Team Deathmatch";
        } else if (cgs.gametype == GT_CTF) {
            s = "Capture the Flag";
        } else {
            s = "";
        }
        w = CG_DrawStrlen(s);
        if (w > 640 / GIANT_WIDTH) {
            cw = 640 / w;
        } else {
            cw = GIANT_WIDTH;
        }
        CG_DrawStringExt(320 - w * cw / 2, 25, s, tavros::math::vec4(1.0f), false, true, cw, (int32) (cw * 1.1f), 0);
    }

    sec = (sec - cg.time) / 1000;
    if (sec < 0) {
        cg.warmup = 0;
        sec = 0;
    }
    s = va("Starts in: %i", sec + 1);
    if (sec != cg.warmupCount) {
        cg.warmupCount = sec;
        switch (sec) {
        case 0:
            S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
            break;
        case 1:
            S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
            break;
        case 2:
            S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
            break;
        default:
            break;
        }
    }
    scale = 0.45f;
    switch (cg.warmupCount) {
    case 0:
        cw = 28;
        scale = 0.54f;
        break;
    case 1:
        cw = 24;
        scale = 0.51f;
        break;
    case 2:
        cw = 20;
        scale = 0.48f;
        break;
    default:
        cw = 16;
        scale = 0.45f;
        break;
    }

    w = CG_DrawStrlen(s);
    CG_DrawStringExt(320 - w * cw / 2, 70, s, tavros::math::vec4(1.0f), false, true, cw, (int32) (cw * 1.5), 0);
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D()
{
    if (cg.snap->ps.pm_type == PM_INTERMISSION) {
        CG_DrawIntermission();
        return;
    }

    if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
        CG_DrawSpectator();
        CG_DrawCrosshair();
        CG_DrawCrosshairNames();
    } else {
        // don't draw any status if dead or the scoreboard is being explicitly shown
        if (!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0) {
            CG_DrawStatusBar();
            CG_DrawAmmoWarning();
            CG_DrawCrosshair();
            CG_DrawCrosshairNames();
            CG_DrawWeaponSelect();
            CG_DrawHoldableItem();
            CG_DrawReward();
        }
    }

    CG_DrawVote();
    CG_DrawTeamVote();

    CG_DrawLagometer();

    CG_DrawLowerRight();
    CG_DrawLowerLeft();

    if (!CG_DrawFollow()) {
        CG_DrawWarmup();
    }

    // don't draw center string if scoreboard is up
    cg.scoreBoardShowing = CG_DrawScoreboard();
    if (!cg.scoreBoardShowing) {
        CG_DrawCenterString();
    }
}


static void CG_DrawTourneyScoreboard()
{
    CG_DrawOldTourneyScoreboard();
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive()
{
    // optionally draw the info screen instead
    if (!cg.snap) {
        CG_DrawInformation();
        return;
    }

    // optionally draw the tournement scoreboard instead
    if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_SCOREBOARD)) {
        CG_DrawTourneyScoreboard();
        return;
    }

    // clear around the rendered view if sized down
    CG_TileClear();

    // draw 3D view
    RE_RenderScene(&cg.refdef);

    // draw status bar and other floating elements
    CG_Draw2D();
}


