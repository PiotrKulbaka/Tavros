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
//
// gameinfo.c
//

#include "ui_local.h"

#include <tavros/core/memory/mallocator.hpp>
#include <memory>

static tavros::core::logger                              logger("ui_gameinfo");
static tavros::core::unique_ptr<tavros::core::allocator> s_allocator;

//
// arena and bot info
//

int32        ui_numBots;
static char* ui_botInfos[MAX_BOTS];

static int32 ui_numArenas;
static char* ui_arenaInfos[MAX_ARENAS];

static int32 ui_numSinglePlayerArenas;
static int32 ui_numSpecialSinglePlayerArenas;

/*
===============
UI_Alloc
===============
*/
static void* UI_Alloc(int32 size)
{
    return s_allocator->allocate(size, 8);
}

/*
===============
UI_InitMemory
===============
*/
static void UI_InitMemory()
{
    if (!s_allocator) {
        s_allocator = tavros::core::make_unique<tavros::core::mallocator>();
    } else {
        s_allocator->clear();
    }
}

/*
===============
UI_ParseInfos
===============
*/
int32 UI_ParseInfos(char* buf, int32 max, char* infos[])
{
    char* token;
    int32 count;
    char  key[MAX_TOKEN_CHARS];
    char  info[MAX_INFO_STRING];

    count = 0;

    while (1) {
        token = COM_Parse(&buf);
        if (!token[0]) {
            break;
        }
        if (strcmp(token, "{")) {
            logger.warning("Missing {{ in info file");
            break;
        }

        if (count == max) {
            logger.warning("Max infos exceeded");
            break;
        }

        info[0] = '\0';
        while (1) {
            token = COM_ParseExt(&buf, true);
            if (!token[0]) {
                logger.warning("Unexpected end of info file");
                break;
            }
            if (!strcmp(token, "}")) {
                break;
            }
            Q_strncpyz(key, token, sizeof(key));

            token = COM_ParseExt(&buf, false);
            if (!token[0]) {
                strcpy(token, "<NULL>");
            }
            Info_SetValueForKey(info, key, token);
        }
        // NOTE: extra space for arena number
        infos[count] = (char*) UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
        if (infos[count]) {
            strcpy(infos[count], info);
            count++;
        }
    }
    return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile(const char* filename)
{
    int32        len;
    fileHandle_t f;
    char         buf[MAX_ARENAS_TEXT];

    len = FS_FOpenFileByMode(filename, &f, FS_READ);
    if (!f) {
        logger.error("File not found: {}", filename);
        return;
    }
    if (len >= MAX_ARENAS_TEXT) {
        logger.error("File too large: {} is {}, max allowed is {}", filename, len, MAX_ARENAS_TEXT);
        FS_FCloseFile(f);
        return;
    }

    FS_Read2(buf, len, f);
    buf[len] = 0;
    FS_FCloseFile(f);

    ui_numArenas += UI_ParseInfos(buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas]);
}

/*
===============
UI_LoadArenas
===============
*/
static void UI_LoadArenas()
{
    int32 numdirs;
    char  filename[128];
    char  dirlist[1024];
    char* dirptr;
    int32 i, n;
    int32 dirlen;
    char* type;
    char* tag;
    int32 singlePlayerNum, specialNum, otherNum;

    ui_numArenas = 0;

    UI_LoadArenasFromFile("scripts/arenas.txt");

    // get all arenas from .arena files
    numdirs = FS_GetFileList("scripts", ".arena", dirlist, 1024);
    dirptr = dirlist;
    for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
        dirlen = strlen(dirptr);
        strcpy(filename, "scripts/");
        strcat(filename, dirptr);
        UI_LoadArenasFromFile(filename);
    }
    logger.info("{} arenas parsed", ui_numArenas);

    // set initial numbers
    for (n = 0; n < ui_numArenas; n++) {
        Info_SetValueForKey(ui_arenaInfos[n], "num", va("%i", n));
    }

    // go through and count single players levels
    ui_numSinglePlayerArenas = 0;
    ui_numSpecialSinglePlayerArenas = 0;
    for (n = 0; n < ui_numArenas; n++) {
        // determine type
        type = Info_ValueForKey(ui_arenaInfos[n], "type");

        // if no type specified, it will be treated as "ffa"
        if (!*type) {
            continue;
        }

        if (strstr(type, "single")) {
            // check for special single player arenas (training, final)
            tag = Info_ValueForKey(ui_arenaInfos[n], "special");
            if (*tag) {
                ui_numSpecialSinglePlayerArenas++;
                continue;
            }

            ui_numSinglePlayerArenas++;
        }
    }

    n = ui_numSinglePlayerArenas % ARENAS_PER_TIER;
    if (n != 0) {
        ui_numSinglePlayerArenas -= n;
        logger.error("{} arenas ignored to make count divisible by {}", n, ARENAS_PER_TIER);
    }

    // go through once more and assign number to the levels
    singlePlayerNum = 0;
    specialNum = singlePlayerNum + ui_numSinglePlayerArenas;
    otherNum = specialNum + ui_numSpecialSinglePlayerArenas;
    for (n = 0; n < ui_numArenas; n++) {
        // determine type
        type = Info_ValueForKey(ui_arenaInfos[n], "type");

        // if no type specified, it will be treated as "ffa"
        if (*type) {
            if (strstr(type, "single")) {
                // check for special single player arenas (training, final)
                tag = Info_ValueForKey(ui_arenaInfos[n], "special");
                if (*tag) {
                    Info_SetValueForKey(ui_arenaInfos[n], "num", va("%i", specialNum++));
                    continue;
                }

                Info_SetValueForKey(ui_arenaInfos[n], "num", va("%i", singlePlayerNum++));
                continue;
            }
        }

        Info_SetValueForKey(ui_arenaInfos[n], "num", va("%i", otherNum++));
    }
}

/*
===============
UI_GetArenaInfoByNumber
===============
*/
const char* UI_GetArenaInfoByNumber(int32 num)
{
    int32 n;
    char* value;

    if (num < 0 || num >= ui_numArenas) {
        logger.error("Invalid arena number: {}", num);
        return NULL;
    }

    for (n = 0; n < ui_numArenas; n++) {
        value = Info_ValueForKey(ui_arenaInfos[n], "num");
        if (*value && atoi(value) == num) {
            return ui_arenaInfos[n];
        }
    }

    return NULL;
}


/*
===============
UI_GetArenaInfoByNumber
===============
*/
const char* UI_GetArenaInfoByMap(const char* map)
{
    int32 n;

    for (n = 0; n < ui_numArenas; n++) {
        if (Q_stricmp(Info_ValueForKey(ui_arenaInfos[n], "map"), map) == 0) {
            return ui_arenaInfos[n];
        }
    }

    return NULL;
}


/*
===============
UI_GetSpecialArenaInfo
===============
*/
const char* UI_GetSpecialArenaInfo(const char* tag)
{
    int32 n;

    for (n = 0; n < ui_numArenas; n++) {
        if (Q_stricmp(Info_ValueForKey(ui_arenaInfos[n], "special"), tag) == 0) {
            return ui_arenaInfos[n];
        }
    }

    return NULL;
}

/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile(const char* filename)
{
    int32        len;
    fileHandle_t f;
    char         buf[MAX_BOTS_TEXT];

    len = FS_FOpenFileByMode(filename, &f, FS_READ);
    if (!f) {
        logger.error("File not found: {}", filename);
        return;
    }
    if (len >= MAX_BOTS_TEXT) {
        logger.error("file too large: {} is {}, max allowed is {}", filename, len, MAX_BOTS_TEXT);
        FS_FCloseFile(f);
        return;
    }

    FS_Read2(buf, len, f);
    buf[len] = 0;
    FS_FCloseFile(f);

    ui_numBots += UI_ParseInfos(buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots]);
}

/*
===============
UI_LoadBots
===============
*/
static void UI_LoadBots()
{
    int32 numdirs;
    char  filename[128];
    char  dirlist[1024];
    char* dirptr;
    int32 i;
    int32 dirlen;

    ui_numBots = 0;

    UI_LoadBotsFromFile("scripts/bots.txt");

    // get all bots from .bot files
    numdirs = FS_GetFileList("scripts", ".bot", dirlist, 1024);
    dirptr = dirlist;
    for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
        dirlen = strlen(dirptr);
        strcpy(filename, "scripts/");
        strcat(filename, dirptr);
        UI_LoadBotsFromFile(filename);
    }
    logger.info("{} bots parsed", ui_numBots);
}


/*
===============
UI_GetBotInfoByNumber
===============
*/
char* UI_GetBotInfoByNumber(int32 num)
{
    if (num < 0 || num >= ui_numBots) {
        logger.error("Invalid bot number: {}", num);
        return NULL;
    }
    return ui_botInfos[num];
}


/*
===============
UI_GetBotInfoByName
===============
*/
char* UI_GetBotInfoByName(const char* name)
{
    int32 n;
    char* value;

    for (n = 0; n < ui_numBots; n++) {
        value = Info_ValueForKey(ui_botInfos[n], "name");
        if (!Q_stricmp(value, name)) {
            return ui_botInfos[n];
        }
    }

    return NULL;
}


//
// single player game info
//

/*
===============
UI_GetBestScore

Returns the player's best finish on a given level, 0 if the have not played the level
===============
*/
void UI_GetBestScore(int32 level, int32* score, int32* skill)
{
    int32 n;
    int32 skillScore;
    int32 bestScore;
    int32 bestScoreSkill;
    char  arenaKey[16];
    char  scores[MAX_INFO_VALUE];

    if (!score || !skill) {
        return;
    }

    if (level < 0 || level > ui_numArenas) {
        return;
    }

    bestScore = 0;
    bestScoreSkill = 0;

    for (n = 1; n <= 5; n++) {
        Cvar_VariableStringBuffer(va("g_spScores%i", n), scores, MAX_INFO_VALUE);

        Com_sprintf(arenaKey, sizeof(arenaKey), "l%i", level);
        skillScore = atoi(Info_ValueForKey(scores, arenaKey));

        if (skillScore < 1 || skillScore > 8) {
            continue;
        }

        if (!bestScore || skillScore <= bestScore) {
            bestScore = skillScore;
            bestScoreSkill = n;
        }
    }

    *score = bestScore;
    *skill = bestScoreSkill;
}


/*
===============
UI_SetBestScore

Set the player's best finish for a level
===============
*/
void UI_SetBestScore(int32 level, int32 score)
{
    int32 skill;
    int32 oldScore;
    char  arenaKey[16];
    char  scores[MAX_INFO_VALUE];

    // validate score
    if (score < 1 || score > 8) {
        return;
    }

    // validate skill
    skill = (int32) Cvar_VariableValue("g_spSkill");
    if (skill < 1 || skill > 5) {
        return;
    }

    // get scores
    Cvar_VariableStringBuffer(va("g_spScores%i", skill), scores, MAX_INFO_VALUE);

    // see if this is better
    Com_sprintf(arenaKey, sizeof(arenaKey), "l%i", level);
    oldScore = atoi(Info_ValueForKey(scores, arenaKey));
    if (oldScore && oldScore <= score) {
        return;
    }

    // update scores
    Info_SetValueForKey(scores, arenaKey, va("%i", score));
    Cvar_Set(va("g_spScores%i", skill), scores);
}


/*
===============
UI_LogAwardData
===============
*/
void UI_LogAwardData(int32 award, int32 data)
{
    char  key[16];
    char  awardData[MAX_INFO_VALUE];
    int32 oldValue;

    if (data == 0) {
        return;
    }

    if (award > AWARD_PERFECT) {
        logger.error("Bad award {} in UI_LogAwardData", award);
        return;
    }

    Cvar_VariableStringBuffer("g_spAwards", awardData, sizeof(awardData));

    Com_sprintf(key, sizeof(key), "a%i", award);
    oldValue = atoi(Info_ValueForKey(awardData, key));

    Info_SetValueForKey(awardData, key, va("%i", oldValue + data));
    Cvar_Set("g_spAwards", awardData);
}


/*
===============
UI_GetAwardLevel
===============
*/
int32 UI_GetAwardLevel(int32 award)
{
    char key[16];
    char awardData[MAX_INFO_VALUE];

    Cvar_VariableStringBuffer("g_spAwards", awardData, sizeof(awardData));

    Com_sprintf(key, sizeof(key), "a%i", award);
    return atoi(Info_ValueForKey(awardData, key));
}


/*
===============
UI_TierCompleted
===============
*/
int32 UI_TierCompleted(int32 levelWon)
{
    int32       level;
    int32       n;
    int32       tier;
    int32       score;
    int32       skill;
    const char* info;

    tier = levelWon / ARENAS_PER_TIER;
    level = tier * ARENAS_PER_TIER;

    if (tier == UI_GetNumSPTiers()) {
        info = UI_GetSpecialArenaInfo("training");
        if (levelWon == atoi(Info_ValueForKey(info, "num"))) {
            return 0;
        }
        info = UI_GetSpecialArenaInfo("final");
        if (!info || levelWon == atoi(Info_ValueForKey(info, "num"))) {
            return tier + 1;
        }
        return -1;
    }

    for (n = 0; n < ARENAS_PER_TIER; n++, level++) {
        UI_GetBestScore(level, &score, &skill);
        if (score != 1) {
            return -1;
        }
    }
    return tier + 1;
}

/*
===============
UI_GetCurrentGame

Returns the next level the player has not won
===============
*/
int32 UI_GetCurrentGame()
{
    int32       level;
    int32       rank;
    int32       skill;
    const char* info;

    info = UI_GetSpecialArenaInfo("training");
    if (info) {
        level = atoi(Info_ValueForKey(info, "num"));
        UI_GetBestScore(level, &rank, &skill);
        if (!rank || rank > 1) {
            return level;
        }
    }

    for (level = 0; level < ui_numSinglePlayerArenas; level++) {
        UI_GetBestScore(level, &rank, &skill);
        if (!rank || rank > 1) {
            return level;
        }
    }

    info = UI_GetSpecialArenaInfo("final");
    if (!info) {
        return -1;
    }
    return atoi(Info_ValueForKey(info, "num"));
}


/*
===============
UI_NewGame

Clears the scores and sets the difficutly level
===============
*/
void UI_NewGame()
{
    Cvar_Set("g_spScores1", "");
    Cvar_Set("g_spScores2", "");
    Cvar_Set("g_spScores3", "");
    Cvar_Set("g_spScores4", "");
    Cvar_Set("g_spScores5", "");
    Cvar_Set("g_spAwards", "");
}


/*
===============
UI_GetNumArenas
===============
*/
int32 UI_GetNumArenas()
{
    return ui_numArenas;
}


/*
===============
UI_GetNumSPArenas
===============
*/
int32 UI_GetNumSPArenas()
{
    return ui_numSinglePlayerArenas;
}


/*
===============
UI_GetNumSPTiers
===============
*/
int32 UI_GetNumSPTiers()
{
    return ui_numSinglePlayerArenas / ARENAS_PER_TIER;
}


/*
===============
UI_GetNumBots
===============
*/
int32 UI_GetNumBots()
{
    return ui_numBots;
}


/*
===============
UI_SPUnlock_f
===============
*/
void UI_SPUnlock_f()
{
    char  arenaKey[16];
    char  scores[MAX_INFO_VALUE];
    int32 level;
    int32 tier;

    // get scores for skill 1
    Cvar_VariableStringBuffer("g_spScores1", scores, MAX_INFO_VALUE);

    // update scores
    for (level = 0; level < ui_numSinglePlayerArenas + ui_numSpecialSinglePlayerArenas; level++) {
        Com_sprintf(arenaKey, sizeof(arenaKey), "l%i", level);
        Info_SetValueForKey(scores, arenaKey, "1");
    }
    Cvar_Set("g_spScores1", scores);

    logger.info("All levels unlocked at skill level 1");

    UI_SPLevelMenu_ReInit();
}


/*
===============
UI_SPUnlockMedals_f
===============
*/
void UI_SPUnlockMedals_f()
{
    int32 n;
    char  key[16];
    char  awardData[MAX_INFO_VALUE];

    Cvar_VariableStringBuffer("g_spAwards", awardData, MAX_INFO_VALUE);

    for (n = 0; n < 6; n++) {
        Com_sprintf(key, sizeof(key), "a%i", n);
        Info_SetValueForKey(awardData, key, "100");
    }

    Cvar_Set("g_spAwards", awardData);

    logger.info("All levels unlocked at 100");
}


/*
===============
UI_InitGameinfo
===============
*/
void UI_InitGameinfo()
{
    UI_InitMemory();
    UI_LoadArenas();
    UI_LoadBots();
}
