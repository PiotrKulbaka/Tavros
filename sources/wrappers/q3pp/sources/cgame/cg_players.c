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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

static tavros::core::logger logger("cg_players");

static const char* cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
    "*death1.wav",
    "*death2.wav",
    "*death3.wav",
    "*jump1.wav",
    "*pain25_1.wav",
    "*pain50_1.wav",
    "*pain75_1.wav",
    "*pain100_1.wav",
    "*falling1.wav",
    "*gasp.wav",
    "*drown.wav",
    "*fall1.wav",
    "*taunt.wav"
};


/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound(int32 clientNum, const char* soundName)
{
    clientInfo_t* ci;
    int32         i;

    if (soundName[0] != '*') {
        return S_RegisterSound(soundName, false);
    }

    if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
        clientNum = 0;
    }
    ci = &cgs.clientinfo[clientNum];

    for (i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i]; i++) {
        if (!strcmp(soundName, cg_customSoundNames[i])) {
            return ci->sounds[i];
        }
    }

    CG_Error("Unknown custom sound: %s", soundName);
    return 0;
}


/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc
======================
*/
static bool CG_ParseAnimationFile(const char* filename, clientInfo_t* ci)
{
    char *       text_p, *prev;
    int32        len;
    int32        i;
    char*        token;
    float        fps;
    int32        skip;
    char         text[20000];
    fileHandle_t f;
    animation_t* animations;

    animations = ci->animations;

    // load the file
    len = FS_FOpenFileByMode(filename, &f, FS_READ);
    if (len <= 0) {
        return false;
    }
    if (len >= sizeof(text) - 1) {
        logger.error("File %s too long", filename);
        return false;
    }
    FS_Read2(text, len, f);
    text[len] = 0;
    FS_FCloseFile(f);

    // parse the text
    text_p = text;
    skip = 0; // quite the compiler warning

    ci->footsteps = FOOTSTEP_NORMAL;
    VectorClear(ci->headOffset);
    ci->gender = GENDER_MALE;
    ci->fixedlegs = false;
    ci->fixedtorso = false;

    // read optional parameters
    while (1) {
        prev = text_p; // so we can unget
        token = COM_Parse(&text_p);
        if (!token) {
            break;
        }
        if (!Q_stricmp(token, "footsteps")) {
            token = COM_Parse(&text_p);
            if (!token) {
                break;
            }
            if (!Q_stricmp(token, "default") || !Q_stricmp(token, "normal")) {
                ci->footsteps = FOOTSTEP_NORMAL;
            } else if (!Q_stricmp(token, "boot")) {
                ci->footsteps = FOOTSTEP_BOOT;
            } else if (!Q_stricmp(token, "flesh")) {
                ci->footsteps = FOOTSTEP_FLESH;
            } else if (!Q_stricmp(token, "mech")) {
                ci->footsteps = FOOTSTEP_MECH;
            } else if (!Q_stricmp(token, "energy")) {
                ci->footsteps = FOOTSTEP_ENERGY;
            } else {
                logger.info("Bad footsteps parm in %s: %s", filename, token);
            }
            continue;
        } else if (!Q_stricmp(token, "headoffset")) {
            for (i = 0; i < 3; i++) {
                token = COM_Parse(&text_p);
                if (!token) {
                    break;
                }
                ci->headOffset[i] = atof(token);
            }
            continue;
        } else if (!Q_stricmp(token, "sex")) {
            token = COM_Parse(&text_p);
            if (!token) {
                break;
            }
            if (token[0] == 'f' || token[0] == 'F') {
                ci->gender = GENDER_FEMALE;
            } else if (token[0] == 'n' || token[0] == 'N') {
                ci->gender = GENDER_NEUTER;
            } else {
                ci->gender = GENDER_MALE;
            }
            continue;
        } else if (!Q_stricmp(token, "fixedlegs")) {
            ci->fixedlegs = true;
            continue;
        } else if (!Q_stricmp(token, "fixedtorso")) {
            ci->fixedtorso = true;
            continue;
        }

        // if it is a number, start parsing animations
        if (token[0] >= '0' && token[0] <= '9') {
            text_p = prev; // unget the token
            break;
        }
        logger.warning("unknown token '%s' is %s", token, filename);
    }

    // read information for each frame
    for (i = 0; i < MAX_ANIMATIONS; i++) {
        token = COM_Parse(&text_p);
        if (!*token) {
            if (i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE) {
                animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
                animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
                animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
                animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
                animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
                animations[i].reversed = false;
                animations[i].flipflop = false;
                continue;
            }
            break;
        }
        animations[i].firstFrame = atoi(token);
        // leg only frames are adjusted to not count the upper body only frames
        if (i == LEGS_WALKCR) {
            skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
        }
        if (i >= LEGS_WALKCR && i < TORSO_GETFLAG) {
            animations[i].firstFrame -= skip;
        }

        token = COM_Parse(&text_p);
        if (!*token) {
            break;
        }
        animations[i].numFrames = atoi(token);

        animations[i].reversed = false;
        animations[i].flipflop = false;
        // if numFrames is negative the animation is reversed
        if (animations[i].numFrames < 0) {
            animations[i].numFrames = -animations[i].numFrames;
            animations[i].reversed = true;
        }

        token = COM_Parse(&text_p);
        if (!*token) {
            break;
        }
        animations[i].loopFrames = atoi(token);

        token = COM_Parse(&text_p);
        if (!*token) {
            break;
        }
        fps = atof(token);
        if (fps == 0) {
            fps = 1;
        }
        animations[i].frameLerp = 1000 / fps;
        animations[i].initialLerp = 1000 / fps;
    }

    if (i != MAX_ANIMATIONS) {
        logger.error("Error parsing animation file: %s", filename);
        return false;
    }

    // crouch backward animation
    memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
    animations[LEGS_BACKCR].reversed = true;
    // walk backward animation
    memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
    animations[LEGS_BACKWALK].reversed = true;
    // flag moving fast
    animations[FLAG_RUN].firstFrame = 0;
    animations[FLAG_RUN].numFrames = 16;
    animations[FLAG_RUN].loopFrames = 16;
    animations[FLAG_RUN].frameLerp = 1000 / 15;
    animations[FLAG_RUN].initialLerp = 1000 / 15;
    animations[FLAG_RUN].reversed = false;
    // flag not moving or moving slowly
    animations[FLAG_STAND].firstFrame = 16;
    animations[FLAG_STAND].numFrames = 5;
    animations[FLAG_STAND].loopFrames = 0;
    animations[FLAG_STAND].frameLerp = 1000 / 20;
    animations[FLAG_STAND].initialLerp = 1000 / 20;
    animations[FLAG_STAND].reversed = false;
    // flag speeding up
    animations[FLAG_STAND2RUN].firstFrame = 16;
    animations[FLAG_STAND2RUN].numFrames = 5;
    animations[FLAG_STAND2RUN].loopFrames = 1;
    animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
    animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
    animations[FLAG_STAND2RUN].reversed = true;
    //
    // new anims changes
    //
    //    animations[TORSO_GETFLAG].flipflop = true;
    //    animations[TORSO_GUARDBASE].flipflop = true;
    //    animations[TORSO_PATROL].flipflop = true;
    //    animations[TORSO_AFFIRMATIVE].flipflop = true;
    //    animations[TORSO_NEGATIVE].flipflop = true;
    //
    return true;
}

/*
==========================
CG_FileExists
==========================
*/
static bool CG_FileExists(const char* filename)
{
    int32 len;

    len = FS_FOpenFileByMode(filename, 0, FS_READ);
    if (len > 0) {
        return true;
    }
    return false;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static bool CG_FindClientModelFile(char* filename, int32 length, clientInfo_t* ci, const char* teamName, const char* modelName, const char* skinName, const char* base, const char* ext)
{
    const char *team, *charactersFolder;
    int32       i;

    if (cgs.gametype >= GT_TEAM) {
        switch (ci->team) {
        case TEAM_BLUE: {
            team = "blue";
            break;
        }
        default: {
            team = "red";
            break;
        }
        }
    } else {
        team = "default";
    }
    charactersFolder = "";
    while (1) {
        for (i = 0; i < 2; i++) {
            if (i == 0 && teamName && *teamName) {
                //                                "models/players/characters/james/stroggs/lower_lily_red.skin"
                Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext);
            } else {
                //                                "models/players/characters/james/lower_lily_red.skin"
                Com_sprintf(filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext);
            }
            if (CG_FileExists(filename)) {
                return true;
            }
            if (cgs.gametype >= GT_TEAM) {
                if (i == 0 && teamName && *teamName) {
                    //                                "models/players/characters/james/stroggs/lower_red.skin"
                    Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext);
                } else {
                    //                                "models/players/characters/james/lower_red.skin"
                    Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext);
                }
            } else {
                if (i == 0 && teamName && *teamName) {
                    //                                "models/players/characters/james/stroggs/lower_lily.skin"
                    Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext);
                } else {
                    //                                "models/players/characters/james/lower_lily.skin"
                    Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext);
                }
            }
            if (CG_FileExists(filename)) {
                return true;
            }
            if (!teamName || !*teamName) {
                break;
            }
        }
        // if tried the heads folder first
        if (charactersFolder[0]) {
            break;
        }
        charactersFolder = "characters/";
    }

    return false;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static bool CG_FindClientHeadFile(char* filename, int32 length, clientInfo_t* ci, const char* teamName, const char* headModelName, const char* headSkinName, const char* base, const char* ext)
{
    const char *team, *headsFolder;
    int32       i;

    if (cgs.gametype >= GT_TEAM) {
        switch (ci->team) {
        case TEAM_BLUE: {
            team = "blue";
            break;
        }
        default: {
            team = "red";
            break;
        }
        }
    } else {
        team = "default";
    }

    if (headModelName[0] == '*') {
        headsFolder = "heads/";
        headModelName++;
    } else {
        headsFolder = "";
    }
    while (1) {
        for (i = 0; i < 2; i++) {
            if (i == 0 && teamName && *teamName) {
                Com_sprintf(filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext);
            } else {
                Com_sprintf(filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext);
            }
            if (CG_FileExists(filename)) {
                return true;
            }
            if (cgs.gametype >= GT_TEAM) {
                if (i == 0 && teamName && *teamName) {
                    Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, team, ext);
                } else {
                    Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, team, ext);
                }
            } else {
                if (i == 0 && teamName && *teamName) {
                    Com_sprintf(filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext);
                } else {
                    Com_sprintf(filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext);
                }
            }
            if (CG_FileExists(filename)) {
                return true;
            }
            if (!teamName || !*teamName) {
                break;
            }
        }
        // if tried the heads folder first
        if (headsFolder[0]) {
            break;
        }
        headsFolder = "heads/";
    }

    return false;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static bool CG_RegisterClientSkin(clientInfo_t* ci, const char* teamName, const char* modelName, const char* skinName, const char* headModelName, const char* headSkinName)
{
    char filename[MAX_QPATH];
    if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinName, "lower", "skin")) {
        ci->legsSkin = RE_RegisterSkin(filename);
    }
    if (!ci->legsSkin) {
        logger.warning("Leg skin load failure: %s", filename);
    }

    if (CG_FindClientModelFile(filename, sizeof(filename), ci, teamName, modelName, skinName, "upper", "skin")) {
        ci->torsoSkin = RE_RegisterSkin(filename);
    }
    if (!ci->torsoSkin) {
        logger.warning("Torso skin load failure: %s", filename);
    }

    if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headModelName, headSkinName, "head", "skin")) {
        ci->headSkin = RE_RegisterSkin(filename);
    }
    if (!ci->headSkin) {
        logger.warning("Head skin load failure: %s", filename);
    }

    // if any skins failed to load
    if (!ci->legsSkin || !ci->torsoSkin || !ci->headSkin) {
        return false;
    }
    return true;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static bool CG_RegisterClientModelname(clientInfo_t* ci, const char* modelName, const char* skinName, const char* headModelName, const char* headSkinName, const char* teamName)
{
    char        filename[MAX_QPATH * 2];
    const char* headName;
    char        newTeamName[MAX_QPATH * 2];

    if (headModelName[0] == '\0') {
        headName = modelName;
    } else {
        headName = headModelName;
    }
    Com_sprintf(filename, sizeof(filename), "models/players/%s/lower.md3", modelName);
    ci->legsModel = RE_RegisterModel(filename);
    if (!ci->legsModel) {
        Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/lower.md3", modelName);
        ci->legsModel = RE_RegisterModel(filename);
        if (!ci->legsModel) {
            logger.warning("Failed to load model file %s", filename);
            return false;
        }
    }

    Com_sprintf(filename, sizeof(filename), "models/players/%s/upper.md3", modelName);
    ci->torsoModel = RE_RegisterModel(filename);
    if (!ci->torsoModel) {
        Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/upper.md3", modelName);
        ci->torsoModel = RE_RegisterModel(filename);
        if (!ci->torsoModel) {
            logger.warning("Failed to load model file %s", filename);
            return false;
        }
    }

    if (headName[0] == '*') {
        Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1]);
    } else {
        Com_sprintf(filename, sizeof(filename), "models/players/%s/head.md3", headName);
    }
    ci->headModel = RE_RegisterModel(filename);
    // if the head model could not be found and we didn't load from the heads folder try to load from there
    if (!ci->headModel && headName[0] != '*') {
        Com_sprintf(filename, sizeof(filename), "models/players/heads/%s/%s.md3", headModelName, headModelName);
        ci->headModel = RE_RegisterModel(filename);
    }
    if (!ci->headModel) {
        logger.warning("Failed to load model file %s", filename);
        return false;
    }

    // if any skins failed to load, return failure
    if (!CG_RegisterClientSkin(ci, teamName, modelName, skinName, headName, headSkinName)) {
        if (teamName && *teamName) {
            logger.warning("Failed to load skin file: %s : %s : %s, %s : %s", teamName, modelName, skinName, headName, headSkinName);
            if (ci->team == TEAM_BLUE) {
                Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
            } else {
                Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
            }
            if (!CG_RegisterClientSkin(ci, newTeamName, modelName, skinName, headName, headSkinName)) {
                logger.warning("Failed to load skin file: %s : %s : %s, %s : %s", newTeamName, modelName, skinName, headName, headSkinName);
                return false;
            }
        } else {
            logger.warning("Failed to load skin file: %s : %s, %s : %s", modelName, skinName, headName, headSkinName);
            return false;
        }
    }

    // load the animations
    Com_sprintf(filename, sizeof(filename), "models/players/%s/animation.cfg", modelName);
    if (!CG_ParseAnimationFile(filename, ci)) {
        Com_sprintf(filename, sizeof(filename), "models/players/characters/%s/animation.cfg", modelName);
        if (!CG_ParseAnimationFile(filename, ci)) {
            logger.warning("Failed to load animation file %s", filename);
            return false;
        }
    }

    if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "skin")) {
        ci->modelIcon = RE_RegisterShaderNoMip(filename);
    } else if (CG_FindClientHeadFile(filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "tga")) {
        ci->modelIcon = RE_RegisterShaderNoMip(filename);
    }

    if (!ci->modelIcon) {
        return false;
    }

    return true;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString(const char* v, vec3_t color)
{
    int32 val;

    VectorClear(color);

    val = atoi(v);

    if (val < 1 || val > 7) {
        VectorSet(color, 1, 1, 1);
        return;
    }

    if (val & 1) {
        color[2] = 1.0f;
    }
    if (val & 2) {
        color[1] = 1.0f;
    }
    if (val & 4) {
        color[0] = 1.0f;
    }
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo(clientInfo_t* ci)
{
    const char *dir, *fallback;
    int32       i, modelloaded;
    const char* s;
    int32       clientNum;
    char        teamname[MAX_QPATH];

    teamname[0] = 0;

    modelloaded = true;
    if (!CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname)) {
        // fall back to default team name
        if (cgs.gametype >= GT_TEAM) {
            // keep skin name
            if (ci->team == TEAM_BLUE) {
                Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname));
            } else {
                Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname));
            }
            if (!CG_RegisterClientModelname(ci, DEFAULT_TEAM_MODEL, ci->skinName, DEFAULT_TEAM_HEAD, ci->skinName, teamname)) {
                CG_Error("DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinName);
            }
        } else {
            if (!CG_RegisterClientModelname(ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname)) {
                CG_Error("DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL);
            }
        }
        modelloaded = false;
    }

    ci->newAnims = false;
    if (ci->torsoModel) {
        orientation_t tag;
        // if the torso model has the "tag_flag"
        if (R_LerpTag(&tag, ci->torsoModel, 0, 0, 1, "tag_flag")) {
            ci->newAnims = true;
        }
    }

    // sounds
    dir = ci->modelName;
    fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

    for (i = 0; i < MAX_CUSTOM_SOUNDS; i++) {
        s = cg_customSoundNames[i];
        if (!s) {
            break;
        }
        ci->sounds[i] = 0;
        // if the model didn't load use the sounds of the default model
        if (modelloaded) {
            ci->sounds[i] = S_RegisterSound(va("sound/player/%s/%s", dir, s + 1), false);
        }
        if (!ci->sounds[i]) {
            ci->sounds[i] = S_RegisterSound(va("sound/player/%s/%s", fallback, s + 1), false);
        }
    }

    ci->deferred = false;

    // reset any existing players and bodies, because they might be in bad
    // frames for this new model
    clientNum = ci - cgs.clientinfo;
    for (i = 0; i < MAX_GENTITIES; i++) {
        if (cg_entities[i].currentState.clientNum == clientNum
            && cg_entities[i].currentState.eType == ET_PLAYER) {
            CG_ResetPlayerEntity(&cg_entities[i]);
        }
    }
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel(clientInfo_t* from, clientInfo_t* to)
{
    VectorCopy(from->headOffset, to->headOffset);
    to->footsteps = from->footsteps;
    to->gender = from->gender;

    to->legsModel = from->legsModel;
    to->legsSkin = from->legsSkin;
    to->torsoModel = from->torsoModel;
    to->torsoSkin = from->torsoSkin;
    to->headModel = from->headModel;
    to->headSkin = from->headSkin;
    to->modelIcon = from->modelIcon;

    to->newAnims = from->newAnims;

    memcpy(to->animations, from->animations, sizeof(to->animations));
    memcpy(to->sounds, from->sounds, sizeof(to->sounds));
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static bool CG_ScanForExistingClientInfo(clientInfo_t* ci)
{
    int32         i;
    clientInfo_t* match;

    for (i = 0; i < cgs.maxclients; i++) {
        match = &cgs.clientinfo[i];
        if (!match->infoValid) {
            continue;
        }
        if (match->deferred) {
            continue;
        }
        if (!Q_stricmp(ci->modelName, match->modelName)
            && !Q_stricmp(ci->skinName, match->skinName)
            && !Q_stricmp(ci->headModelName, match->headModelName)
            && !Q_stricmp(ci->headSkinName, match->headSkinName)
            && !Q_stricmp(ci->blueTeam, match->blueTeam)
            && !Q_stricmp(ci->redTeam, match->redTeam)
            && (cgs.gametype < GT_TEAM || ci->team == match->team)) {
            // this clientinfo is identical, so use it's handles

            ci->deferred = false;

            CG_CopyClientInfoModel(match, ci);

            return true;
        }
    }

    // nothing matches, so defer the load
    return false;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo(clientInfo_t* ci)
{
    int32         i;
    clientInfo_t* match;

    // if someone else is already the same models and skins we
    // can just load the client info
    for (i = 0; i < cgs.maxclients; i++) {
        match = &cgs.clientinfo[i];
        if (!match->infoValid || match->deferred) {
            continue;
        }
        if (Q_stricmp(ci->skinName, match->skinName) || Q_stricmp(ci->modelName, match->modelName) ||
            //             Q_stricmp( ci->headModelName, match->headModelName ) ||
            //             Q_stricmp( ci->headSkinName, match->headSkinName ) ||
            (cgs.gametype >= GT_TEAM && ci->team != match->team)) {
            continue;
        }
        // just load the real info cause it uses the same models and skins
        CG_LoadClientInfo(ci);
        return;
    }

    // if we are in teamplay, only grab a model if the skin is correct
    if (cgs.gametype >= GT_TEAM) {
        for (i = 0; i < cgs.maxclients; i++) {
            match = &cgs.clientinfo[i];
            if (!match->infoValid || match->deferred) {
                continue;
            }
            if (Q_stricmp(ci->skinName, match->skinName) || (cgs.gametype >= GT_TEAM && ci->team != match->team)) {
                continue;
            }
            ci->deferred = true;
            CG_CopyClientInfoModel(match, ci);
            return;
        }
        // load the full model, because we don't ever want to show
        // an improper team skin.  This will cause a hitch for the first
        // player, when the second enters.  Combat shouldn't be going on
        // yet, so it shouldn't matter
        CG_LoadClientInfo(ci);
        return;
    }

    // find the first valid clientinfo and grab its stuff
    for (i = 0; i < cgs.maxclients; i++) {
        match = &cgs.clientinfo[i];
        if (!match->infoValid) {
            continue;
        }

        ci->deferred = true;
        CG_CopyClientInfoModel(match, ci);
        return;
    }

    // we should never get here...
    logger.error("CG_SetDeferredClientInfo: no valid clients!");

    CG_LoadClientInfo(ci);
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo(int32 clientNum)
{
    clientInfo_t* ci;
    clientInfo_t  newInfo;
    const char*   configstring;
    const char*   v;
    const char*   slash;

    ci = &cgs.clientinfo[clientNum];

    configstring = CG_ConfigString(clientNum + CS_PLAYERS);
    if (!configstring[0]) {
        memset(ci, 0, sizeof(*ci));
        return; // player just left
    }

    // build into a temp buffer so the defer checks can use
    // the old value
    memset(&newInfo, 0, sizeof(newInfo));

    // isolate the player's name
    v = Info_ValueForKey(configstring, "n");
    Q_strncpyz(newInfo.name, v, sizeof(newInfo.name));

    // colors
    v = Info_ValueForKey(configstring, "c1");
    CG_ColorFromString(v, newInfo.color1);

    v = Info_ValueForKey(configstring, "c2");
    CG_ColorFromString(v, newInfo.color2);

    // bot skill
    v = Info_ValueForKey(configstring, "skill");
    newInfo.botSkill = atoi(v);

    // handicap
    v = Info_ValueForKey(configstring, "hc");
    newInfo.handicap = atoi(v);

    // wins
    v = Info_ValueForKey(configstring, "w");
    newInfo.wins = atoi(v);

    // losses
    v = Info_ValueForKey(configstring, "l");
    newInfo.losses = atoi(v);

    // team
    v = Info_ValueForKey(configstring, "t");
    newInfo.team = (team_t) atoi(v);

    // team task
    v = Info_ValueForKey(configstring, "tt");
    newInfo.teamTask = atoi(v);

    // team leader
    v = Info_ValueForKey(configstring, "tl");
    newInfo.teamLeader = atoi(v);

    v = Info_ValueForKey(configstring, "g_redteam");
    Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

    v = Info_ValueForKey(configstring, "g_blueteam");
    Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

    // model
    v = Info_ValueForKey(configstring, "model");

    Q_strncpyz(newInfo.modelName, v, sizeof(newInfo.modelName));

    slash = strchr(newInfo.modelName, '/');
    if (!slash) {
        // modelName didn not include a skin name
        Q_strncpyz(newInfo.skinName, "default", sizeof(newInfo.skinName));
    } else {
        Q_strncpyz(newInfo.skinName, slash + 1, sizeof(newInfo.skinName));
        // truncate modelName
        *((char*) slash) = 0;
    }


    // head model
    v = Info_ValueForKey(configstring, "hmodel");

    Q_strncpyz(newInfo.headModelName, v, sizeof(newInfo.headModelName));

    slash = strchr(newInfo.headModelName, '/');
    if (!slash) {
        // modelName didn not include a skin name
        Q_strncpyz(newInfo.headSkinName, "default", sizeof(newInfo.headSkinName));
    } else {
        Q_strncpyz(newInfo.headSkinName, slash + 1, sizeof(newInfo.headSkinName));
        // truncate modelName
        *(char*) slash = 0;
    }

    // scan for an existing clientinfo that matches this modelname
    // so we can avoid loading checks if possible
    if (!CG_ScanForExistingClientInfo(&newInfo)) {
        // if we are defering loads, just have it pick the first valid
        if (!cg.loading) {
            // keep whatever they had if it won't violate team skins
            CG_SetDeferredClientInfo(&newInfo);
        } else {
            CG_LoadClientInfo(&newInfo);
        }
    }

    // replace whatever was there with the new one
    newInfo.infoValid = true;
    *ci = newInfo;
}


/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers()
{
    int32         i;
    clientInfo_t* ci;

    // scan for a deferred player to load
    for (i = 0, ci = cgs.clientinfo; i < cgs.maxclients; i++, ci++) {
        if (ci->infoValid && ci->deferred) {
            CG_LoadClientInfo(ci);
        }
    }
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(clientInfo_t* ci, lerpFrame_t* lf, int32 newAnimation)
{
    animation_t* anim;

    lf->animationNumber = newAnimation;
    newAnimation &= ~ANIM_TOGGLEBIT;

    if (newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS) {
        CG_Error("Bad animation number: %i", newAnimation);
    }

    anim = &ci->animations[newAnimation];

    lf->animation = anim;
    lf->animationTime = lf->frameTime + anim->initialLerp;
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int32 newAnimation, float speedScale)
{
    int32        f, numFrames;
    animation_t* anim;

    // see if the animation sequence is switching
    if (newAnimation != lf->animationNumber || !lf->animation) {
        CG_SetLerpFrameAnimation(ci, lf, newAnimation);
    }

    // if we have passed the current frame, move it to
    // oldFrame and calculate a new frame
    if (cg.time >= lf->frameTime) {
        lf->oldFrame = lf->frame;
        lf->oldFrameTime = lf->frameTime;

        // get the next frame based on the animation
        anim = lf->animation;
        if (!anim->frameLerp) {
            return; // shouldn't happen
        }
        if (cg.time < lf->animationTime) {
            lf->frameTime = lf->animationTime; // initial lerp
        } else {
            lf->frameTime = lf->oldFrameTime + anim->frameLerp;
        }
        f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
        f *= speedScale; // adjust for haste, etc

        numFrames = anim->numFrames;
        if (anim->flipflop) {
            numFrames *= 2;
        }
        if (f >= numFrames) {
            f -= numFrames;
            if (anim->loopFrames) {
                f %= anim->loopFrames;
                f += anim->numFrames - anim->loopFrames;
            } else {
                f = numFrames - 1;
                // the animation is stuck at the end, so it
                // can immediately transition to another sequence
                lf->frameTime = cg.time;
            }
        }
        if (anim->reversed) {
            lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
        } else if (anim->flipflop && f >= anim->numFrames) {
            lf->frame = anim->firstFrame + anim->numFrames - 1 - (f % anim->numFrames);
        } else {
            lf->frame = anim->firstFrame + f;
        }
        if (cg.time > lf->frameTime) {
            lf->frameTime = cg.time;
        }
    }

    if (lf->frameTime > cg.time + 200) {
        lf->frameTime = cg.time;
    }

    if (lf->oldFrameTime > cg.time) {
        lf->oldFrameTime = cg.time;
    }
    // calculate current lerp value
    if (lf->frameTime == lf->oldFrameTime) {
        lf->backlerp = 0;
    } else {
        lf->backlerp = 1.0 - (float) (cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
    }
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(clientInfo_t* ci, lerpFrame_t* lf, int32 animationNumber)
{
    lf->frameTime = lf->oldFrameTime = cg.time;
    CG_SetLerpFrameAnimation(ci, lf, animationNumber);
    lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t* cent, int32* legsOld, int32* legs, float* legsBackLerp, int32* torsoOld, int32* torso, float* torsoBackLerp)
{
    clientInfo_t* ci;
    int32         clientNum;
    float         speedScale;

    clientNum = cent->currentState.clientNum;

    if (cent->currentState.powerups & (1 << PW_HASTE)) {
        speedScale = 1.5;
    } else {
        speedScale = 1;
    }

    ci = &cgs.clientinfo[clientNum];

    // do the shuffle turn frames locally
    if (cent->pe.legs.yawing && (cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE) {
        CG_RunLerpFrame(ci, &cent->pe.legs, LEGS_TURN, speedScale);
    } else {
        CG_RunLerpFrame(ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale);
    }

    *legsOld = cent->pe.legs.oldFrame;
    *legs = cent->pe.legs.frame;
    *legsBackLerp = cent->pe.legs.backlerp;

    CG_RunLerpFrame(ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale);

    *torsoOld = cent->pe.torso.oldFrame;
    *torso = cent->pe.torso.frame;
    *torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance, float speed, float* angle, bool* swinging)
{
    float swing;
    float move;
    float scale;

    if (!*swinging) {
        // see if a swing should be started
        swing = AngleSubtract(*angle, destination);
        if (swing > swingTolerance || swing < -swingTolerance) {
            *swinging = true;
        }
    }

    if (!*swinging) {
        return;
    }

    // modify the speed depending on the delta
    // so it doesn't seem so linear
    swing = AngleSubtract(destination, *angle);
    scale = fabs(swing);
    if (scale < swingTolerance * 0.5) {
        scale = 0.5;
    } else if (scale < swingTolerance) {
        scale = 1.0;
    } else {
        scale = 2.0;
    }

    // swing towards the destination angle
    if (swing >= 0) {
        move = cg.frametime * scale * speed;
        if (move >= swing) {
            move = swing;
            *swinging = false;
        }
        *angle = AngleMod(*angle + move);
    } else if (swing < 0) {
        move = cg.frametime * scale * -speed;
        if (move <= swing) {
            move = swing;
            *swinging = false;
        }
        *angle = AngleMod(*angle + move);
    }

    // clamp to no more than tolerance
    swing = AngleSubtract(destination, *angle);
    if (swing > clampTolerance) {
        *angle = AngleMod(destination - (clampTolerance - 1));
    } else if (swing < -clampTolerance) {
        *angle = AngleMod(destination + (clampTolerance - 1));
    }
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch(centity_t* cent, vec3_t torsoAngles)
{
    int32 t;
    float f;

    t = cg.time - cent->pe.painTime;
    if (t >= PAIN_TWITCH_TIME) {
        return;
    }

    f = 1.0 - (float) t / PAIN_TWITCH_TIME;

    if (cent->pe.painDirection) {
        torsoAngles[ROLL] += 20 * f;
    } else {
        torsoAngles[ROLL] -= 20 * f;
    }
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles(centity_t* cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
    vec3_t        legsAngles, torsoAngles, headAngles;
    float         dest;
    static int32  movementOffsets[8] = {0, 22, 45, -22, 0, 22, -45, -22};
    vec3_t        velocity;
    float         speed;
    int32         dir, clientNum;
    clientInfo_t* ci;

    VectorCopy(cent->lerpAngles, headAngles);
    headAngles[YAW] = AngleMod(headAngles[YAW]);
    VectorClear(legsAngles);
    VectorClear(torsoAngles);

    // --------- yaw -------------

    // allow yaw to drift a bit
    if ((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE
        || (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND) {
        // if not standing still, always point all in the same direction
        cent->pe.torso.yawing = true;   // always center
        cent->pe.torso.pitching = true; // always center
        cent->pe.legs.yawing = true;    // always center
    }

    // adjust legs for movement dir
    if (cent->currentState.eFlags & EF_DEAD) {
        // don't let dead bodies twitch
        dir = 0;
    } else {
        dir = cent->currentState.angles2[YAW];
        if (dir < 0 || dir > 7) {
            CG_Error("Bad player movement angle");
        }
    }
    legsAngles[YAW] = headAngles[YAW] + movementOffsets[dir];
    torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[dir];

    // torso
    CG_SwingAngles(torsoAngles[YAW], 25, 90, 0.3, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);
    CG_SwingAngles(legsAngles[YAW], 40, 90, 0.3, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);

    torsoAngles[YAW] = cent->pe.torso.yawAngle;
    legsAngles[YAW] = cent->pe.legs.yawAngle;


    // --------- pitch -------------

    // only show a fraction of the pitch angle in the torso
    if (headAngles[PITCH] > 180) {
        dest = (-360 + headAngles[PITCH]) * 0.75f;
    } else {
        dest = headAngles[PITCH] * 0.75f;
    }
    CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
    torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

    //
    clientNum = cent->currentState.clientNum;
    if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
        ci = &cgs.clientinfo[clientNum];
        if (ci->fixedtorso) {
            torsoAngles[PITCH] = 0.0f;
        }
    }

    // --------- roll -------------


    // lean towards the direction of travel
    VectorCopy(cent->currentState.pos.trDelta, velocity);
    speed = VectorNormalize(velocity);
    if (speed) {
        vec3_t axis[3];
        float  side;

        speed *= 0.05f;

        AnglesToAxis(legsAngles, axis);
        side = speed * DotProduct(velocity, axis[1]);
        legsAngles[ROLL] -= side;

        side = speed * DotProduct(velocity, axis[0]);
        legsAngles[PITCH] += side;
    }

    //
    clientNum = cent->currentState.clientNum;
    if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
        ci = &cgs.clientinfo[clientNum];
        if (ci->fixedlegs) {
            legsAngles[YAW] = torsoAngles[YAW];
            legsAngles[PITCH] = 0.0f;
            legsAngles[ROLL] = 0.0f;
        }
    }

    // pain twitch
    CG_AddPainTwitch(cent, torsoAngles);

    // pull the angles back out of the hierarchial chain
    AnglesSubtract(headAngles, torsoAngles, headAngles);
    AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
    AnglesToAxis(legsAngles, legs);
    AnglesToAxis(torsoAngles, torso);
    AnglesToAxis(headAngles, head);
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail(centity_t* cent)
{
    localEntity_t* smoke;
    vec3_t         origin;
    int32          anim;

    if (cent->trailTime > cg.time) {
        return;
    }
    anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
    if (anim != LEGS_RUN && anim != LEGS_BACK) {
        return;
    }

    cent->trailTime += 100;
    if (cent->trailTime < cg.time) {
        cent->trailTime = cg.time;
    }

    VectorCopy(cent->lerpOrigin, origin);
    origin[2] -= 16;

    smoke = CG_SmokePuff(origin, vec3_origin, 8, 1, 1, 1, 1, 500, cg.time, 0, 0, cgs.media.hastePuffShader);

    // use the optimized local entity add
    smoke->leType = LE_SCALE_FADE;
}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem(centity_t* cent, qhandle_t hModel)
{
    refEntity_t ent;
    vec3_t      angles;
    vec3_t      axis[3];

    VectorCopy(cent->lerpAngles, angles);
    angles[PITCH] = 0;
    angles[ROLL] = 0;
    AnglesToAxis(angles, axis);

    memset(&ent, 0, sizeof(ent));
    VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
    ent.origin[2] += 16;
    angles[YAW] += 90;
    AnglesToAxis(angles, ent.axis);

    ent.hModel = hModel;
    RE_AddRefEntityToScene(&ent);
}


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag(centity_t* cent, qhandle_t hSkin, refEntity_t* torso)
{
    clientInfo_t* ci;
    refEntity_t   pole;
    refEntity_t   flag;
    vec3_t        angles, dir;
    int32         legsAnim, flagAnim, updateangles;
    float         angle, d;

    // show the flag pole model
    memset(&pole, 0, sizeof(pole));
    pole.hModel = cgs.media.flagPoleModel;
    VectorCopy(torso->lightingOrigin, pole.lightingOrigin);
    pole.renderfx = torso->renderfx;
    CG_PositionEntityOnTag(&pole, torso, torso->hModel, "tag_flag");
    RE_AddRefEntityToScene(&pole);

    // show the flag model
    memset(&flag, 0, sizeof(flag));
    flag.hModel = cgs.media.flagFlapModel;
    flag.customSkin = hSkin;
    VectorCopy(torso->lightingOrigin, flag.lightingOrigin);
    flag.renderfx = torso->renderfx;

    VectorClear(angles);

    updateangles = false;
    legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
    if (legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR) {
        flagAnim = FLAG_STAND;
    } else if (legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR) {
        flagAnim = FLAG_STAND;
        updateangles = true;
    } else {
        flagAnim = FLAG_RUN;
        updateangles = true;
    }

    if (updateangles) {
        VectorCopy(cent->currentState.pos.trDelta, dir);
        // add gravity
        dir[2] += 100;
        VectorNormalize(dir);
        d = DotProduct(pole.axis[2], dir);
        // if there is anough movement orthogonal to the flag pole
        if (fabs(d) < 0.9) {
            //
            d = DotProduct(pole.axis[0], dir);
            if (d > 1.0f) {
                d = 1.0f;
            } else if (d < -1.0f) {
                d = -1.0f;
            }
            angle = acos(d);

            d = DotProduct(pole.axis[1], dir);
            if (d < 0) {
                angles[YAW] = 360 - angle * 180 / M_PI;
            } else {
                angles[YAW] = angle * 180 / M_PI;
            }
            if (angles[YAW] < 0) {
                angles[YAW] += 360;
            }
            if (angles[YAW] > 360) {
                angles[YAW] -= 360;
            }

            // vectoangles( cent->currentState.pos.trDelta, tmpangles );
            // angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
            //  change the yaw angle
            CG_SwingAngles(angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing);
        }

        /*
        d = DotProduct(pole.axis[2], dir);
        angle = Q_acos(d);

        d = DotProduct(pole.axis[1], dir);
        if (d < 0) {
            angle = 360 - angle * 180 / M_PI;
        }
        else {
            angle = angle * 180 / M_PI;
        }
        if (angle > 340 && angle < 20) {
            flagAnim = FLAG_RUNUP;
        }
        if (angle > 160 && angle < 200) {
            flagAnim = FLAG_RUNDOWN;
        }
        */
    }

    // set the yaw angle
    angles[YAW] = cent->pe.flag.yawAngle;
    // lerp the flag animation frames
    ci = &cgs.clientinfo[cent->currentState.clientNum];
    CG_RunLerpFrame(ci, &cent->pe.flag, flagAnim, 1);
    flag.oldframe = cent->pe.flag.oldFrame;
    flag.frame = cent->pe.flag.frame;
    flag.backlerp = cent->pe.flag.backlerp;

    AnglesToAxis(angles, flag.axis);
    CG_PositionRotatedEntityOnTag(&flag, &pole, pole.hModel, "tag_flag");

    RE_AddRefEntityToScene(&flag);
}

/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups(centity_t* cent, refEntity_t* torso)
{
    int32         powerups;
    clientInfo_t* ci;

    powerups = cent->currentState.powerups;
    if (!powerups) {
        return;
    }

    // quad gives a dlight
    if (powerups & (1 << PW_QUAD)) {
        RE_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1);
    }

    // flight plays a looped sound
    if (powerups & (1 << PW_FLIGHT)) {
        S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound);
    }

    ci = &cgs.clientinfo[cent->currentState.clientNum];
    // redflag
    if (powerups & (1 << PW_REDFLAG)) {
        if (ci->newAnims) {
            CG_PlayerFlag(cent, cgs.media.redFlagFlapSkin, torso);
        } else {
            CG_TrailItem(cent, cgs.media.redFlagModel);
        }
        RE_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 0.2f, 0.2f);
    }

    // blueflag
    if (powerups & (1 << PW_BLUEFLAG)) {
        if (ci->newAnims) {
            CG_PlayerFlag(cent, cgs.media.blueFlagFlapSkin, torso);
        } else {
            CG_TrailItem(cent, cgs.media.blueFlagModel);
        }
        RE_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 0.2f, 0.2f, 1.0);
    }

    // neutralflag
    if (powerups & (1 << PW_NEUTRALFLAG)) {
        if (ci->newAnims) {
            CG_PlayerFlag(cent, cgs.media.neutralFlagFlapSkin, torso);
        } else {
            CG_TrailItem(cent, cgs.media.neutralFlagModel);
        }
        RE_AddLightToScene(cent->lerpOrigin, 200 + (rand() & 31), 1.0, 1.0, 1.0);
    }

    // haste leaves smoke trails
    if (powerups & (1 << PW_HASTE)) {
        CG_HasteTrail(cent);
    }
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite(centity_t* cent, qhandle_t shader)
{
    int32       rf;
    refEntity_t ent;

    if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
        rf = RF_THIRD_PERSON; // only show in mirrors
    } else {
        rf = 0;
    }

    memset(&ent, 0, sizeof(ent));
    VectorCopy(cent->lerpOrigin, ent.origin);
    ent.origin[2] += 48;
    ent.reType = RT_SPRITE;
    ent.customShader = shader;
    ent.radius = 10;
    ent.renderfx = rf;
    ent.shaderRGBA[0] = 255;
    ent.shaderRGBA[1] = 255;
    ent.shaderRGBA[2] = 255;
    ent.shaderRGBA[3] = 255;
    RE_AddRefEntityToScene(&ent);
}


/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(centity_t* cent)
{
    int32 team;

    if (cent->currentState.eFlags & EF_CONNECTION) {
        CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
        return;
    }

    if (cent->currentState.eFlags & EF_TALK) {
        CG_PlayerFloatSprite(cent, cgs.media.balloonShader);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_IMPRESSIVE) {
        CG_PlayerFloatSprite(cent, cgs.media.medalImpressive);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_EXCELLENT) {
        CG_PlayerFloatSprite(cent, cgs.media.medalExcellent);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_GAUNTLET) {
        CG_PlayerFloatSprite(cent, cgs.media.medalGauntlet);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_DEFEND) {
        CG_PlayerFloatSprite(cent, cgs.media.medalDefend);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_ASSIST) {
        CG_PlayerFloatSprite(cent, cgs.media.medalAssist);
        return;
    }

    if (cent->currentState.eFlags & EF_AWARD_CAP) {
        CG_PlayerFloatSprite(cent, cgs.media.medalCapture);
        return;
    }

    team = cgs.clientinfo[cent->currentState.clientNum].team;
    if (!(cent->currentState.eFlags & EF_DEAD) && cg.snap->ps.persistant[PERS_TEAM] == team && cgs.gametype >= GT_TEAM) {
        CG_PlayerFloatSprite(cent, cgs.media.friendShader);
        return;
    }
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE 128
static bool CG_PlayerShadow(centity_t* cent)
{
    vec3_t  end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
    trace_t trace;
    float   alpha;

    // no shadows when invisible
    if (cent->currentState.powerups & (1 << PW_INVIS)) {
        return false;
    }

    // send a trace down from the player to the ground
    VectorCopy(cent->lerpOrigin, end);
    end[2] -= SHADOW_DISTANCE;

    CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, false);

    // no shadow if too high
    if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid) {
        return false;
    }

    // fade the shadow out with height
    alpha = 1.0 - trace.fraction;

    // bk0101022 - hack / FPE - bogus planes?
    // assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )

    // add the mark as a temporary, so it goes directly to the renderer
    // without taking a spot in the cg_marks array
    CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, cent->pe.legs.yawAngle, alpha, alpha, alpha, 1, false, 24, true);

    return true;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash(centity_t* cent)
{
    vec3_t     start, end;
    trace_t    trace;
    int32      contents;
    polyVert_t verts[4];

    VectorCopy(cent->lerpOrigin, end);
    end[2] -= 24;

    // if the feet aren't in liquid, don't make a mark
    // this won't handle moving water brushes, but they wouldn't draw right anyway...
    contents = CM_PointContents(end, 0);
    if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
        return;
    }

    VectorCopy(cent->lerpOrigin, start);
    start[2] += 32;

    // if the head isn't out of liquid, don't make a mark
    contents = CM_PointContents(start, 0);
    if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
        return;
    }

    // trace down to find the surface
    CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA), false);

    if (trace.fraction == 1.0) {
        return;
    }

    // create a mark polygon
    VectorCopy(trace.endpos, verts[0].xyz);
    verts[0].xyz[0] -= 32;
    verts[0].xyz[1] -= 32;
    verts[0].st[0] = 0;
    verts[0].st[1] = 0;
    verts[0].modulateU8[0] = 255;
    verts[0].modulateU8[1] = 255;
    verts[0].modulateU8[2] = 255;
    verts[0].modulateU8[3] = 255;

    VectorCopy(trace.endpos, verts[1].xyz);
    verts[1].xyz[0] -= 32;
    verts[1].xyz[1] += 32;
    verts[1].st[0] = 0;
    verts[1].st[1] = 1;
    verts[1].modulateU8[0] = 255;
    verts[1].modulateU8[1] = 255;
    verts[1].modulateU8[2] = 255;
    verts[1].modulateU8[3] = 255;

    VectorCopy(trace.endpos, verts[2].xyz);
    verts[2].xyz[0] += 32;
    verts[2].xyz[1] += 32;
    verts[2].st[0] = 1;
    verts[2].st[1] = 1;
    verts[2].modulateU8[0] = 255;
    verts[2].modulateU8[1] = 255;
    verts[2].modulateU8[2] = 255;
    verts[2].modulateU8[3] = 255;

    VectorCopy(trace.endpos, verts[3].xyz);
    verts[3].xyz[0] += 32;
    verts[3].xyz[1] -= 32;
    verts[3].st[0] = 1;
    verts[3].st[1] = 0;
    verts[3].modulateU8[0] = 255;
    verts[3].modulateU8[1] = 255;
    verts[3].modulateU8[2] = 255;
    verts[3].modulateU8[3] = 255;

    RE_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts, 1);
}


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups(refEntity_t* ent, entityState_t* state, int32 team)
{
    if (state->powerups & (1 << PW_INVIS)) {
        ent->customShader = cgs.media.invisShader;
        RE_AddRefEntityToScene(ent);
    } else {
        /*
        if ( state->eFlags & EF_KAMIKAZE ) {
            if (team == TEAM_BLUE)
                ent->customShader = cgs.media.blueKamikazeShader;
            else
                ent->customShader = cgs.media.redKamikazeShader;
            RE_AddRefEntityToScene( ent );
        }
        else {*/
        RE_AddRefEntityToScene(ent);
        //}

        if (state->powerups & (1 << PW_QUAD)) {
            if (team == TEAM_RED) {
                ent->customShader = cgs.media.redQuadShader;
            } else {
                ent->customShader = cgs.media.quadShader;
            }
            RE_AddRefEntityToScene(ent);
        }
        if (state->powerups & (1 << PW_REGEN)) {
            if (((cg.time / 100) % 10) == 1) {
                ent->customShader = cgs.media.regenShader;
                RE_AddRefEntityToScene(ent);
            }
        }
        if (state->powerups & (1 << PW_BATTLESUIT)) {
            ent->customShader = cgs.media.battleSuitShader;
            RE_AddRefEntityToScene(ent);
        }
    }
}

/*
===============
CG_Player
===============
*/
void CG_Player(centity_t* cent)
{
    clientInfo_t* ci;
    refEntity_t   legs;
    refEntity_t   torso;
    refEntity_t   head;
    int32         clientNum;
    int32         renderfx;

    // the client number is stored in clientNum.  It can't be derived
    // from the entity number, because a single client may have
    // multiple corpses on the level using the same clientinfo
    clientNum = cent->currentState.clientNum;
    if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
        CG_Error("Bad clientNum on player entity");
    }
    ci = &cgs.clientinfo[clientNum];

    // it is possible to see corpses from disconnected players that may
    // not have valid clientinfo
    if (!ci->infoValid) {
        return;
    }

    // get the player model information
    renderfx = 0;
    if (cent->currentState.number == cg.snap->ps.clientNum) {
        if (!cg.renderingThirdPerson) {
            renderfx = RF_THIRD_PERSON; // only draw in mirrors
        }
    }


    memset(&legs, 0, sizeof(legs));
    memset(&torso, 0, sizeof(torso));
    memset(&head, 0, sizeof(head));

    // get the rotation information
    CG_PlayerAngles(cent, legs.axis, torso.axis, head.axis);

    // get the animation state (after rotation, to allow feet shuffle)
    CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp, &torso.oldframe, &torso.frame, &torso.backlerp);

    // add the talk baloon or disconnect icon
    CG_PlayerSprites(cent);

    // add the shadow
    CG_PlayerShadow(cent);

    // add a water splash if partially in and out of water
    CG_PlayerSplash(cent);

    renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all
    //
    // add the legs
    //
    legs.hModel = ci->legsModel;
    legs.customSkin = ci->legsSkin;

    VectorCopy(cent->lerpOrigin, legs.origin);

    VectorCopy(cent->lerpOrigin, legs.lightingOrigin);
    legs.renderfx = renderfx;
    VectorCopy(legs.origin, legs.oldorigin); // don't positionally lerp at all

    CG_AddRefEntityWithPowerups(&legs, &cent->currentState, ci->team);

    // if the model failed, allow the default nullmodel to be displayed
    if (!legs.hModel) {
        return;
    }

    //
    // add the torso
    //
    torso.hModel = ci->torsoModel;
    if (!torso.hModel) {
        return;
    }

    torso.customSkin = ci->torsoSkin;

    VectorCopy(cent->lerpOrigin, torso.lightingOrigin);

    CG_PositionRotatedEntityOnTag(&torso, &legs, ci->legsModel, "tag_torso");

    torso.renderfx = renderfx;

    CG_AddRefEntityWithPowerups(&torso, &cent->currentState, ci->team);

    //
    // add the head
    //
    head.hModel = ci->headModel;
    if (!head.hModel) {
        return;
    }
    head.customSkin = ci->headSkin;

    VectorCopy(cent->lerpOrigin, head.lightingOrigin);

    CG_PositionRotatedEntityOnTag(&head, &torso, ci->torsoModel, "tag_head");

    head.renderfx = renderfx;

    CG_AddRefEntityWithPowerups(&head, &cent->currentState, ci->team);

    //
    // add the gun / barrel / flash
    //
    CG_AddPlayerWeapon(&torso, NULL, cent, ci->team);

    // add powerups floating behind the player
    CG_PlayerPowerups(cent, &torso);
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t* cent)
{
    cent->errorTime = -99999; // guarantee no error decay added
    cent->extrapolated = false;

    CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, cent->currentState.legsAnim);
    CG_ClearLerpFrame(&cgs.clientinfo[cent->currentState.clientNum], &cent->pe.torso, cent->currentState.torsoAnim);

    BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
    BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

    VectorCopy(cent->lerpOrigin, cent->rawOrigin);
    VectorCopy(cent->lerpAngles, cent->rawAngles);

    memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
    cent->pe.legs.yawAngle = cent->rawAngles[YAW];
    cent->pe.legs.yawing = false;
    cent->pe.legs.pitchAngle = 0;
    cent->pe.legs.pitching = false;

    memset(&cent->pe.torso, 0, sizeof(cent->pe.legs));
    cent->pe.torso.yawAngle = cent->rawAngles[YAW];
    cent->pe.torso.yawing = false;
    cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
    cent->pe.torso.pitching = false;
}

