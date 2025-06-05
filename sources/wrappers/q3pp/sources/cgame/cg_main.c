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
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "../client/client.h"

static tavros::core::logger logger("cg_main");

cg_t         cg;
cgs_t        cgs;
centity_t    cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t   cg_items[MAX_ITEMS];


cvar_t* cg_lagometer;
cvar_t* cg_synchronousClients;
cvar_t* cg_paused;
cvar_t* pmove_fixed;
cvar_t* pmove_msec;
cvar_t* cg_timescaleFadeEnd;
cvar_t* cg_timescaleFadeSpeed;
cvar_t* cg_timescale;

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars()
{
    char var[MAX_TOKEN_CHARS];

    cg_lagometer = Cvar_Get("cg_lagometer", "1", CVAR_ARCHIVE);
    cg_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
    cg_synchronousClients = Cvar_Get("g_synchronousClients", "0", 0);
    cg_timescaleFadeEnd = Cvar_Get("cg_timescaleFadeEnd", "1", 0);
    cg_timescaleFadeSpeed = Cvar_Get("cg_timescaleFadeSpeed", "0", 0);
    cg_timescale = Cvar_Get("timescale", "1", 0);

    // see if we are also running the server on this machine
    Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
    cgs.localServer = atoi(var);

    Cvar_Get("model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
}

int32 CG_CrosshairPlayer()
{
    if (cg.time > (cg.crosshairClientTime + 1000)) {
        return -1;
    }
    return cg.crosshairClientNum;
}

int32 CG_LastAttacker()
{
    if (!cg.attackerTime) {
        return -1;
    }
    return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Error(const char* msg, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, msg);
    vsprintf(text, msg, argptr);
    va_end(argptr);

    Com_Error(ERR_DROP, "%s", text);
}

/*
================
CG_Argv
================
*/
const char* CG_Argv(int32 arg)
{
    static char buffer[MAX_STRING_CHARS];
    Cmd_ArgvBuffer(arg, buffer, sizeof(buffer));
    return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int32 itemNum)
{
    gitem_t*    item;
    char        data[MAX_QPATH];
    const char *s, *start;
    int32       len;

    item = &bg_itemlist[itemNum];

    if (item->pickup_sound) {
        S_RegisterSound(item->pickup_sound, false);
    }

    // parse the space seperated precache string for other media
    s = item->sounds;
    if (!s || !s[0]) {
        return;
    }

    while (*s) {
        start = s;
        while (*s && *s != ' ') {
            s++;
        }

        len = s - start;
        if (len >= MAX_QPATH || len < 5) {
            CG_Error("PrecacheItem: %s has bad precache string", item->classname);
            return;
        }
        memcpy(data, start, len);
        data[len] = 0;
        if (*s) {
            s++;
        }

        if (!strcmp(data + len - 3, "wav")) {
            S_RegisterSound(data, false);
        }
    }
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds()
{
    int32       i;
    char        items[MAX_ITEMS + 1];
    char        name[MAX_QPATH];
    const char* soundName;

    cgs.media.oneMinuteSound = S_RegisterSound("sound/feedback/1_minute.wav", true);
    cgs.media.fiveMinuteSound = S_RegisterSound("sound/feedback/5_minute.wav", true);
    cgs.media.suddenDeathSound = S_RegisterSound("sound/feedback/sudden_death.wav", true);
    cgs.media.oneFragSound = S_RegisterSound("sound/feedback/1_frag.wav", true);
    cgs.media.twoFragSound = S_RegisterSound("sound/feedback/2_frags.wav", true);
    cgs.media.threeFragSound = S_RegisterSound("sound/feedback/3_frags.wav", true);
    cgs.media.count3Sound = S_RegisterSound("sound/feedback/three.wav", true);
    cgs.media.count2Sound = S_RegisterSound("sound/feedback/two.wav", true);
    cgs.media.count1Sound = S_RegisterSound("sound/feedback/one.wav", true);
    cgs.media.countFightSound = S_RegisterSound("sound/feedback/fight.wav", true);
    cgs.media.countPrepareSound = S_RegisterSound("sound/feedback/prepare.wav", true);

    if (cgs.gametype >= GT_TEAM) {
        cgs.media.captureAwardSound = S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", true);
        cgs.media.redLeadsSound = S_RegisterSound("sound/feedback/redleads.wav", true);
        cgs.media.blueLeadsSound = S_RegisterSound("sound/feedback/blueleads.wav", true);
        cgs.media.teamsTiedSound = S_RegisterSound("sound/feedback/teamstied.wav", true);
        cgs.media.hitTeamSound = S_RegisterSound("sound/feedback/hit_teammate.wav", true);

        cgs.media.redScoredSound = S_RegisterSound("sound/teamplay/voc_red_scores.wav", true);
        cgs.media.blueScoredSound = S_RegisterSound("sound/teamplay/voc_blue_scores.wav", true);

        cgs.media.captureYourTeamSound = S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", true);
        cgs.media.captureOpponentSound = S_RegisterSound("sound/teamplay/flagcapture_opponent.wav", true);

        cgs.media.returnYourTeamSound = S_RegisterSound("sound/teamplay/flagreturn_yourteam.wav", true);
        cgs.media.returnOpponentSound = S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", true);

        cgs.media.takenYourTeamSound = S_RegisterSound("sound/teamplay/flagtaken_yourteam.wav", true);
        cgs.media.takenOpponentSound = S_RegisterSound("sound/teamplay/flagtaken_opponent.wav", true);

        if (cgs.gametype == GT_CTF) {
            cgs.media.redFlagReturnedSound = S_RegisterSound("sound/teamplay/voc_red_returned.wav", true);
            cgs.media.blueFlagReturnedSound = S_RegisterSound("sound/teamplay/voc_blue_returned.wav", true);
            cgs.media.enemyTookYourFlagSound = S_RegisterSound("sound/teamplay/voc_enemy_flag.wav", true);
            cgs.media.yourTeamTookEnemyFlagSound = S_RegisterSound("sound/teamplay/voc_team_flag.wav", true);
        }

        cgs.media.youHaveFlagSound = S_RegisterSound("sound/teamplay/voc_you_flag.wav", true);
        cgs.media.holyShitSound = S_RegisterSound("sound/feedback/voc_holyshit.wav", true);
        cgs.media.neutralFlagReturnedSound = S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", true);
        cgs.media.yourTeamTookTheFlagSound = S_RegisterSound("sound/teamplay/voc_team_1flag.wav", true);
        cgs.media.enemyTookTheFlagSound = S_RegisterSound("sound/teamplay/voc_enemy_1flag.wav", true);
    }

    cgs.media.tracerSound = S_RegisterSound("sound/weapons/machinegun/buletby1.wav", false);
    cgs.media.selectSound = S_RegisterSound("sound/weapons/change.wav", false);
    cgs.media.wearOffSound = S_RegisterSound("sound/items/wearoff.wav", false);
    cgs.media.useNothingSound = S_RegisterSound("sound/items/use_nothing.wav", false);
    cgs.media.gibSound = S_RegisterSound("sound/player/gibsplt1.wav", false);
    cgs.media.gibBounce1Sound = S_RegisterSound("sound/player/gibimp1.wav", false);
    cgs.media.gibBounce2Sound = S_RegisterSound("sound/player/gibimp2.wav", false);
    cgs.media.gibBounce3Sound = S_RegisterSound("sound/player/gibimp3.wav", false);

    cgs.media.teleInSound = S_RegisterSound("sound/world/telein.wav", false);
    cgs.media.teleOutSound = S_RegisterSound("sound/world/teleout.wav", false);
    cgs.media.respawnSound = S_RegisterSound("sound/items/respawn1.wav", false);

    cgs.media.noAmmoSound = S_RegisterSound("sound/weapons/noammo.wav", false);

    cgs.media.talkSound = S_RegisterSound("sound/player/talk.wav", false);
    cgs.media.landSound = S_RegisterSound("sound/player/land1.wav", false);

    cgs.media.hitSound = S_RegisterSound("sound/feedback/hit.wav", false);

    cgs.media.impressiveSound = S_RegisterSound("sound/feedback/impressive.wav", true);
    cgs.media.excellentSound = S_RegisterSound("sound/feedback/excellent.wav", true);
    cgs.media.deniedSound = S_RegisterSound("sound/feedback/denied.wav", true);
    cgs.media.humiliationSound = S_RegisterSound("sound/feedback/humiliation.wav", true);
    cgs.media.assistSound = S_RegisterSound("sound/feedback/assist.wav", true);
    cgs.media.defendSound = S_RegisterSound("sound/feedback/defense.wav", true);

    cgs.media.takenLeadSound = S_RegisterSound("sound/feedback/takenlead.wav", true);
    cgs.media.tiedLeadSound = S_RegisterSound("sound/feedback/tiedlead.wav", true);
    cgs.media.lostLeadSound = S_RegisterSound("sound/feedback/lostlead.wav", true);

    cgs.media.watrInSound = S_RegisterSound("sound/player/watr_in.wav", false);
    cgs.media.watrOutSound = S_RegisterSound("sound/player/watr_out.wav", false);
    cgs.media.watrUnSound = S_RegisterSound("sound/player/watr_un.wav", false);

    cgs.media.jumpPadSound = S_RegisterSound("sound/world/jumppad.wav", false);

    for (i = 0; i < 4; i++) {
        Com_sprintf(name, sizeof(name), "sound/player/footsteps/step%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_NORMAL][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/boot%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_BOOT][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_FLESH][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/mech%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_MECH][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/energy%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_ENERGY][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/splash%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_SPLASH][i] = S_RegisterSound(name, false);

        Com_sprintf(name, sizeof(name), "sound/player/footsteps/clank%i.wav", i + 1);
        cgs.media.footsteps[FOOTSTEP_METAL][i] = S_RegisterSound(name, false);
    }

    // only register the items that the server says we need
    strcpy(items, CG_ConfigString(CS_ITEMS));

    for (i = 1; i < bg_numItems; i++) {
        CG_RegisterItemSounds(i);
    }

    for (i = 1; i < MAX_SOUNDS; i++) {
        soundName = CG_ConfigString(CS_SOUNDS + i);
        if (!soundName[0]) {
            break;
        }
        if (soundName[0] == '*') {
            continue; // custom sound
        }
        cgs.gameSounds[i] = S_RegisterSound(soundName, false);
    }

    // FIXME: only needed with item
    cgs.media.flightSound = S_RegisterSound("sound/items/flight.wav", false);
    cgs.media.medkitSound = S_RegisterSound("sound/items/use_medkit.wav", false);
    cgs.media.quadSound = S_RegisterSound("sound/items/damage3.wav", false);
    cgs.media.sfx_ric1 = S_RegisterSound("sound/weapons/machinegun/ric1.wav", false);
    cgs.media.sfx_ric2 = S_RegisterSound("sound/weapons/machinegun/ric2.wav", false);
    cgs.media.sfx_ric3 = S_RegisterSound("sound/weapons/machinegun/ric3.wav", false);
    cgs.media.sfx_railg = S_RegisterSound("sound/weapons/railgun/railgf1a.wav", false);
    cgs.media.sfx_rockexp = S_RegisterSound("sound/weapons/rocket/rocklx1a.wav", false);
    cgs.media.sfx_plasmaexp = S_RegisterSound("sound/weapons/plasma/plasmx1a.wav", false);

    cgs.media.regenSound = S_RegisterSound("sound/items/regen.wav", false);
    cgs.media.protectSound = S_RegisterSound("sound/items/protect3.wav", false);
    cgs.media.n_healthSound = S_RegisterSound("sound/items/n_health.wav", false);
    cgs.media.hgrenb1aSound = S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", false);
    cgs.media.hgrenb2aSound = S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", false);
}


//===================================================================================


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics()
{
    int32              i;
    char               items[MAX_ITEMS + 1];
    static const char* sb_nums[11] = {
        "gfx/2d/numbers/zero_32b",
        "gfx/2d/numbers/one_32b",
        "gfx/2d/numbers/two_32b",
        "gfx/2d/numbers/three_32b",
        "gfx/2d/numbers/four_32b",
        "gfx/2d/numbers/five_32b",
        "gfx/2d/numbers/six_32b",
        "gfx/2d/numbers/seven_32b",
        "gfx/2d/numbers/eight_32b",
        "gfx/2d/numbers/nine_32b",
        "gfx/2d/numbers/minus_32b",
    };

    // clear any references to old media
    memset(&cg.refdef, 0, sizeof(cg.refdef));
    RE_ClearScene();

    CG_LoadingString(cgs.mapname);

    RE_LoadWorldMap(cgs.mapname);

    // precache status bar pics
    CG_LoadingString("game media");

    for (i = 0; i < 11; i++) {
        cgs.media.numberShaders[i] = RE_RegisterShader(sb_nums[i]);
    }

    cgs.media.botSkillShaders[0] = RE_RegisterShader("menu/art/skill1.tga");
    cgs.media.botSkillShaders[1] = RE_RegisterShader("menu/art/skill2.tga");
    cgs.media.botSkillShaders[2] = RE_RegisterShader("menu/art/skill3.tga");
    cgs.media.botSkillShaders[3] = RE_RegisterShader("menu/art/skill4.tga");
    cgs.media.botSkillShaders[4] = RE_RegisterShader("menu/art/skill5.tga");

    cgs.media.viewBloodShader = RE_RegisterShader("viewBloodBlend");

    cgs.media.deferShader = RE_RegisterShaderNoMip("gfx/2d/defer.tga");

    cgs.media.scoreboardName = RE_RegisterShaderNoMip("menu/tab/name.tga");
    cgs.media.scoreboardPing = RE_RegisterShaderNoMip("menu/tab/ping.tga");
    cgs.media.scoreboardScore = RE_RegisterShaderNoMip("menu/tab/score.tga");
    cgs.media.scoreboardTime = RE_RegisterShaderNoMip("menu/tab/time.tga");

    cgs.media.smokePuffShader = RE_RegisterShader("smokePuff");
    cgs.media.smokePuffRageProShader = RE_RegisterShader("smokePuffRagePro");
    cgs.media.shotgunSmokePuffShader = RE_RegisterShader("shotgunSmokePuff");

    cgs.media.plasmaBallShader = RE_RegisterShader("sprites/plasma1");
    cgs.media.bloodTrailShader = RE_RegisterShader("bloodTrail");
    cgs.media.lagometerShader = RE_RegisterShader("lagometer");
    cgs.media.connectionShader = RE_RegisterShader("disconnected");

    cgs.media.waterBubbleShader = RE_RegisterShader("waterBubble");

    cgs.media.tracerShader = RE_RegisterShader("gfx/misc/tracer");
    cgs.media.selectShader = RE_RegisterShader("gfx/2d/select");
    cgs.media.crosshairShader = RE_RegisterShader("gfx/2d/crosshair");
    cgs.media.backTileShader = RE_RegisterShader("gfx/2d/backtile");
    cgs.media.noammoShader = RE_RegisterShader("icons/noammo");

    // powerup shaders
    cgs.media.quadShader = RE_RegisterShader("powerups/quad");
    cgs.media.quadWeaponShader = RE_RegisterShader("powerups/quadWeapon");
    cgs.media.battleSuitShader = RE_RegisterShader("powerups/battleSuit");
    cgs.media.battleWeaponShader = RE_RegisterShader("powerups/battleWeapon");
    cgs.media.invisShader = RE_RegisterShader("powerups/invisibility");
    cgs.media.regenShader = RE_RegisterShader("powerups/regen");
    cgs.media.hastePuffShader = RE_RegisterShader("hasteSmokePuff");

    if (cgs.gametype == GT_CTF) {
        cgs.media.redCubeModel = RE_RegisterModel("models/powerups/orb/r_orb.md3");
        cgs.media.blueCubeModel = RE_RegisterModel("models/powerups/orb/b_orb.md3");
        cgs.media.redCubeIcon = RE_RegisterShader("icons/skull_red");
        cgs.media.blueCubeIcon = RE_RegisterShader("icons/skull_blue");
    }

    if (cgs.gametype == GT_CTF) {
        cgs.media.redFlagModel = RE_RegisterModel("models/flags/r_flag.md3");
        cgs.media.blueFlagModel = RE_RegisterModel("models/flags/b_flag.md3");
        cgs.media.redFlagShader[0] = RE_RegisterShaderNoMip("icons/iconf_red1");
        cgs.media.redFlagShader[1] = RE_RegisterShaderNoMip("icons/iconf_red2");
        cgs.media.redFlagShader[2] = RE_RegisterShaderNoMip("icons/iconf_red3");
        cgs.media.blueFlagShader[0] = RE_RegisterShaderNoMip("icons/iconf_blu1");
        cgs.media.blueFlagShader[1] = RE_RegisterShaderNoMip("icons/iconf_blu2");
        cgs.media.blueFlagShader[2] = RE_RegisterShaderNoMip("icons/iconf_blu3");
    }

    if (cgs.gametype >= GT_TEAM) {
        cgs.media.friendShader = RE_RegisterShader("sprites/foe");
        cgs.media.redQuadShader = RE_RegisterShader("powerups/blueflag");
        cgs.media.teamStatusBar = RE_RegisterShader("gfx/2d/colorbar.tga");
    }

    cgs.media.armorModel = RE_RegisterModel("models/powerups/armor/armor_yel.md3");
    cgs.media.armorIcon = RE_RegisterShaderNoMip("icons/iconr_yellow");

    cgs.media.machinegunBrassModel = RE_RegisterModel("models/weapons2/shells/m_shell.md3");
    cgs.media.shotgunBrassModel = RE_RegisterModel("models/weapons2/shells/s_shell.md3");

    cgs.media.gibAbdomen = RE_RegisterModel("models/gibs/abdomen.md3");
    cgs.media.gibArm = RE_RegisterModel("models/gibs/arm.md3");
    cgs.media.gibChest = RE_RegisterModel("models/gibs/chest.md3");
    cgs.media.gibFist = RE_RegisterModel("models/gibs/fist.md3");
    cgs.media.gibFoot = RE_RegisterModel("models/gibs/foot.md3");
    cgs.media.gibForearm = RE_RegisterModel("models/gibs/forearm.md3");
    cgs.media.gibIntestine = RE_RegisterModel("models/gibs/intestine.md3");
    cgs.media.gibLeg = RE_RegisterModel("models/gibs/leg.md3");
    cgs.media.gibSkull = RE_RegisterModel("models/gibs/skull.md3");
    cgs.media.gibBrain = RE_RegisterModel("models/gibs/brain.md3");

    cgs.media.smoke2 = RE_RegisterModel("models/weapons2/shells/s_shell.md3");

    cgs.media.balloonShader = RE_RegisterShader("sprites/balloon3");

    cgs.media.bloodExplosionShader = RE_RegisterShader("bloodExplosion");

    cgs.media.bulletFlashModel = RE_RegisterModel("models/weaphits/bullet.md3");
    cgs.media.ringFlashModel = RE_RegisterModel("models/weaphits/ring02.md3");
    cgs.media.dishFlashModel = RE_RegisterModel("models/weaphits/boom01.md3");
    cgs.media.teleportEffectModel = RE_RegisterModel("models/misc/telep.md3");
    cgs.media.teleportEffectShader = RE_RegisterShader("teleportEffect");

    cgs.media.invulnerabilityPowerupModel = RE_RegisterModel("models/powerups/shield/shield.md3");
    cgs.media.medalImpressive = RE_RegisterShaderNoMip("medal_impressive");
    cgs.media.medalExcellent = RE_RegisterShaderNoMip("medal_excellent");
    cgs.media.medalGauntlet = RE_RegisterShaderNoMip("medal_gauntlet");
    cgs.media.medalDefend = RE_RegisterShaderNoMip("medal_defend");
    cgs.media.medalAssist = RE_RegisterShaderNoMip("medal_assist");
    cgs.media.medalCapture = RE_RegisterShaderNoMip("medal_capture");


    memset(cg_items, 0, sizeof(cg_items));
    memset(cg_weapons, 0, sizeof(cg_weapons));

    // only register the items that the server says we need
    strcpy(items, CG_ConfigString(CS_ITEMS));

    for (i = 1; i < bg_numItems; i++) {
        if (items[i] == '1') {
            CG_LoadingItem(i);
            CG_RegisterItemVisuals(i);
        }
    }

    // wall marks
    cgs.media.bulletMarkShader = RE_RegisterShader("gfx/damage/bullet_mrk");
    cgs.media.burnMarkShader = RE_RegisterShader("gfx/damage/burn_med_mrk");
    cgs.media.holeMarkShader = RE_RegisterShader("gfx/damage/hole_lg_mrk");
    cgs.media.energyMarkShader = RE_RegisterShader("gfx/damage/plasma_mrk");
    cgs.media.shadowMarkShader = RE_RegisterShader("markShadow");
    cgs.media.wakeMarkShader = RE_RegisterShader("wake");
    cgs.media.bloodMarkShader = RE_RegisterShader("bloodMark");

    // register the inline models
    cgs.numInlineModels = CM_NumInlineModels();
    for (i = 1; i < cgs.numInlineModels; i++) {
        char   name[10];
        vec3_t mins, maxs;
        int32  j;

        Com_sprintf(name, sizeof(name), "*%i", i);
        cgs.inlineDrawModel[i] = RE_RegisterModel(name);
        R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
        for (j = 0; j < 3; j++) {
            cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
        }
    }

    // register all the server specified models
    for (i = 1; i < MAX_MODELS; i++) {
        const char* modelName;

        modelName = CG_ConfigString(CS_MODELS + i);
        if (!modelName[0]) {
            break;
        }
        cgs.gameModels[i] = RE_RegisterModel(modelName);
    }

    CG_ClearParticles();
}


/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString()
{
    int32 i;
    cg.spectatorList[0] = 0;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR) {
            Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
        }
    }
    i = strlen(cg.spectatorList);
    if (i != cg.spectatorLen) {
        cg.spectatorLen = i;
        cg.spectatorWidth = -1;
    }
}


/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients()
{
    int32 i;

    CG_LoadingClient(cg.clientNum);
    CG_NewClientInfo(cg.clientNum);

    for (i = 0; i < MAX_CLIENTS; i++) {
        const char* clientInfo;

        if (cg.clientNum == i) {
            continue;
        }

        clientInfo = CG_ConfigString(CS_PLAYERS + i);
        if (!clientInfo[0]) {
            continue;
        }
        CG_LoadingClient(i);
        CG_NewClientInfo(i);
    }
    CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char* CG_ConfigString(int32 index)
{
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        CG_Error("CG_ConfigString: bad index: %i", index);
    }
    return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic()
{
    char* s;
    char  parm1[MAX_QPATH], parm2[MAX_QPATH];

    // start the background music
    s = (char*) CG_ConfigString(CS_MUSIC);
    Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
    Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

    S_StartBackgroundTrack(parm1, parm2);
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init(int32 serverMessageNum, int32 serverCommandSequence, int32 clientNum)
{
    const char* s;

    // clear everything
    memset(&cgs, 0, sizeof(cgs));
    memset(&cg, 0, sizeof(cg));
    memset(cg_entities, 0, sizeof(cg_entities));
    memset(cg_weapons, 0, sizeof(cg_weapons));
    memset(cg_items, 0, sizeof(cg_items));

    cg.clientNum = clientNum;

    cgs.processedSnapshotNum = serverMessageNum;
    cgs.serverCommandSequence = serverCommandSequence;

    // load a few needed things before we do any screen updates
    cgs.media.charsetShader = RE_RegisterShader("gfx/2d/bigchars");
    cgs.media.whiteShader = RE_RegisterShader("white");
    cgs.media.charsetProp = RE_RegisterShaderNoMip("menu/art/font1_prop.tga");
    cgs.media.charsetPropGlow = RE_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
    cgs.media.charsetPropB = RE_RegisterShaderNoMip("menu/art/font2_prop.tga");

    CG_RegisterCvars();

    CG_InitConsoleCommands();

    cg.weaponSelect = WP_MACHINEGUN;

    cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
    cgs.flagStatus = -1;
    // old servers

    // get the rendering configuration from the client system
    cgs.glconfig = cls.glconfig;
    cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
    cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

    // get the gamestate from the client system
    CL_GetGameState(&cgs.gameState);

    // check version
    s = CG_ConfigString(CS_GAME_VERSION);
    if (strcmp(s, GAME_VERSION)) {
        logger.error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
        // CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
    }

    s = CG_ConfigString(CS_LEVEL_START_TIME);
    cgs.levelStartTime = atoi(s);

    CG_ParseServerinfo();

    // load the new map
    CG_LoadingString("collision map");

    int32 checksum; // no used
    CM_LoadMap(cgs.mapname, true, &checksum);

    cg.loading = true; // force players to load instead of defer

    CG_LoadingString("sounds");

    CG_RegisterSounds();

    CG_LoadingString("graphics");

    CG_RegisterGraphics();

    CG_LoadingString("clients");

    CG_RegisterClients(); // if low on memory, some clients will be deferred

    cg.loading = false;   // future players will be deferred

    CG_InitLocalEntities();

    CG_InitMarkPolys();

    // remove the last loading update
    cg.infoScreenText[0] = 0;

    // Make sure we have update values (scores)
    CG_SetConfigValues();

    CG_StartMusic();

    CG_LoadingString("");

    CG_ShaderStateChanged();

    S_ClearLoopingSounds(true);
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown()
{
    // some mods may need to do cleanup work here,
    // like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int32 type)
{
}


void CG_KeyEvent(int32 key, bool down)
{
}

void CG_MouseEvent(int32 x, int32 y)
{
}
