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
// g_bot.c

#include "g_local.h"

static tavros::core::logger logger("g_bot");


static int32 g_numBots;
static char* g_botInfos[MAX_BOTS];


int32        g_numArenas;
static char* g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE      2000
#define BOT_BEGIN_DELAY_INCREMENT 1500

#define BOT_SPAWN_QUEUE_DEPTH     16

typedef struct
{
    int32 clientNum;
    int32 spawnTime;
} botSpawnQueue_t;

// static int32            botBeginDelay = 0;  // bk001206 - unused, init
static botSpawnQueue_t botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

cvar_t* bot_minplayers;

extern gentity_t* podium1;
extern gentity_t* podium2;
extern gentity_t* podium3;

/*
===============
G_ParseInfos
===============
*/
int32 G_ParseInfos(char* buf, int32 max, char* infos[])
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
        infos[count] = (char*) G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
        if (infos[count]) {
            strcpy(infos[count], info);
            count++;
        }
    }
    return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile(const char* filename)
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

    g_numArenas += G_ParseInfos(buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas]);
}

/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas()
{
    int32 numdirs;
    char  filename[128];
    char  dirlist[1024];
    char* dirptr;
    int32 i, n;
    int32 dirlen;

    g_numArenas = 0;

    G_LoadArenasFromFile("scripts/arenas.txt");

    // get all arenas from .arena files
    numdirs = FS_GetFileList("scripts", ".arena", dirlist, 1024);
    dirptr = dirlist;
    for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
        dirlen = strlen(dirptr);
        strcpy(filename, "scripts/");
        strcat(filename, dirptr);
        G_LoadArenasFromFile(filename);
    }
    logger.info("{} arenas parsed", g_numArenas);

    for (n = 0; n < g_numArenas; n++) {
        Info_SetValueForKey(g_arenaInfos[n], "num", va("%i", n));
    }
}


/*
===============
G_GetArenaInfoByNumber
===============
*/
const char* G_GetArenaInfoByMap(const char* map)
{
    int32 n;

    for (n = 0; n < g_numArenas; n++) {
        if (Q_stricmp(Info_ValueForKey(g_arenaInfos[n], "map"), map) == 0) {
            return g_arenaInfos[n];
        }
    }

    return NULL;
}


/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound(const char* modelAndSkin)
{
    char  model[MAX_QPATH];
    char* skin;

    Q_strncpyz(model, modelAndSkin, sizeof(model));
    skin = Q_strrchr(model, '/');
    if (skin) {
        *skin++ = '\0';
    } else {
        skin = model;
    }

    if (Q_stricmp(skin, "default") == 0) {
        skin = model;
    }

    Cbuf_ExecuteText(EXEC_APPEND, va("play sound/player/announce/%s.wav\n", skin));
}

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot(int32 team)
{
    int32       i, n, num;
    float       skill;
    char *      value, netname[36];
    const char* teamstr;
    gclient_t*  cl;

    num = 0;
    for (n = 0; n < g_numBots; n++) {
        value = Info_ValueForKey(g_botInfos[n], "name");
        //
        for (i = 0; i < g_maxclients->integer; i++) {
            cl = level.clients + i;
            if (cl->pers.connected != CON_CONNECTED) {
                continue;
            }
            if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT)) {
                continue;
            }
            if (team >= 0 && cl->sess.sessionTeam != team) {
                continue;
            }
            if (!Q_stricmp(value, cl->pers.netname)) {
                break;
            }
        }
        if (i >= g_maxclients->integer) {
            num++;
        }
    }
    num = random() * num;
    for (n = 0; n < g_numBots; n++) {
        value = Info_ValueForKey(g_botInfos[n], "name");
        //
        for (i = 0; i < g_maxclients->integer; i++) {
            cl = level.clients + i;
            if (cl->pers.connected != CON_CONNECTED) {
                continue;
            }
            if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT)) {
                continue;
            }
            if (team >= 0 && cl->sess.sessionTeam != team) {
                continue;
            }
            if (!Q_stricmp(value, cl->pers.netname)) {
                break;
            }
        }
        if (i >= g_maxclients->integer) {
            num--;
            if (num <= 0) {
                skill = Cvar_VariableValue("g_spSkill");
                if (team == TEAM_RED) {
                    teamstr = "red";
                } else if (team == TEAM_BLUE) {
                    teamstr = "blue";
                } else {
                    teamstr = "";
                }
                strncpy(netname, value, sizeof(netname) - 1);
                netname[sizeof(netname) - 1] = '\0';
                Q_CleanStr(netname);
                Cbuf_ExecuteText(EXEC_INSERT, va("addbot %s %f %s %i\n", netname, skill, teamstr, 0));
                return;
            }
        }
    }
}

/*
===============
G_RemoveRandomBot
===============
*/
int32 G_RemoveRandomBot(int32 team)
{
    int32      i;
    char       netname[36];
    gclient_t* cl;

    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT)) {
            continue;
        }
        if (team >= 0 && cl->sess.sessionTeam != team) {
            continue;
        }
        strcpy(netname, cl->pers.netname);
        Q_CleanStr(netname);
        Cbuf_ExecuteText(EXEC_INSERT, va("kick %s\n", netname));
        return true;
    }
    return false;
}

/*
===============
G_CountHumanPlayers
===============
*/
int32 G_CountHumanPlayers(int32 team)
{
    int32      i, num;
    gclient_t* cl;

    num = 0;
    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) {
            continue;
        }
        if (team >= 0 && cl->sess.sessionTeam != team) {
            continue;
        }
        num++;
    }
    return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int32 G_CountBotPlayers(int32 team)
{
    int32      i, n, num;
    gclient_t* cl;

    num = 0;
    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (!(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT)) {
            continue;
        }
        if (team >= 0 && cl->sess.sessionTeam != team) {
            continue;
        }
        num++;
    }
    for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
        if (!botSpawnQueue[n].spawnTime) {
            continue;
        }
        if (botSpawnQueue[n].spawnTime > level.time) {
            continue;
        }
        num++;
    }
    return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers()
{
    int32        minplayers;
    int32        humanplayers, botplayers;
    static int32 checkminimumplayers_time;

    if (level.intermissiontime) {
        return;
    }
    // only check once each 10 seconds
    if (checkminimumplayers_time > level.time - 10000) {
        return;
    }
    checkminimumplayers_time = level.time;
    minplayers = bot_minplayers->integer;
    if (minplayers <= 0) {
        return;
    }

    if (g_gametype->integer >= GT_TEAM) {
        if (minplayers >= g_maxclients->integer / 2) {
            minplayers = (g_maxclients->integer / 2) - 1;
        }

        humanplayers = G_CountHumanPlayers(TEAM_RED);
        botplayers = G_CountBotPlayers(TEAM_RED);
        //
        if (humanplayers + botplayers < minplayers) {
            G_AddRandomBot(TEAM_RED);
        } else if (humanplayers + botplayers > minplayers && botplayers) {
            G_RemoveRandomBot(TEAM_RED);
        }
        //
        humanplayers = G_CountHumanPlayers(TEAM_BLUE);
        botplayers = G_CountBotPlayers(TEAM_BLUE);
        //
        if (humanplayers + botplayers < minplayers) {
            G_AddRandomBot(TEAM_BLUE);
        } else if (humanplayers + botplayers > minplayers && botplayers) {
            G_RemoveRandomBot(TEAM_BLUE);
        }
    } else if (g_gametype->integer == GT_TOURNAMENT) {
        if (minplayers >= g_maxclients->integer) {
            minplayers = g_maxclients->integer - 1;
        }
        humanplayers = G_CountHumanPlayers(-1);
        botplayers = G_CountBotPlayers(-1);
        //
        if (humanplayers + botplayers < minplayers) {
            G_AddRandomBot(TEAM_FREE);
        } else if (humanplayers + botplayers > minplayers && botplayers) {
            // try to remove spectators first
            if (!G_RemoveRandomBot(TEAM_SPECTATOR)) {
                // just remove the bot that is playing
                G_RemoveRandomBot(-1);
            }
        }
    } else if (g_gametype->integer == GT_FFA) {
        if (minplayers >= g_maxclients->integer) {
            minplayers = g_maxclients->integer - 1;
        }
        humanplayers = G_CountHumanPlayers(TEAM_FREE);
        botplayers = G_CountBotPlayers(TEAM_FREE);
        //
        if (humanplayers + botplayers < minplayers) {
            G_AddRandomBot(TEAM_FREE);
        } else if (humanplayers + botplayers > minplayers && botplayers) {
            G_RemoveRandomBot(TEAM_FREE);
        }
    }
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn()
{
    int32 n;
    char  userinfo[MAX_INFO_VALUE];

    G_CheckMinimumPlayers();

    for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
        if (!botSpawnQueue[n].spawnTime) {
            continue;
        }
        if (botSpawnQueue[n].spawnTime > level.time) {
            continue;
        }
        ClientBegin(botSpawnQueue[n].clientNum);
        botSpawnQueue[n].spawnTime = 0;

        if (g_gametype->integer == GT_SINGLE_PLAYER) {
            trap_GetUserinfo(botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo));
            PlayerIntroSound(Info_ValueForKey(userinfo, "model"));
        }
    }
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue(int32 clientNum, int32 delay)
{
    int32 n;

    for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
        if (!botSpawnQueue[n].spawnTime) {
            botSpawnQueue[n].spawnTime = level.time + delay;
            botSpawnQueue[n].clientNum = clientNum;
            return;
        }
    }

    logger.warning("Unable to delay spawn");
    ClientBegin(clientNum);
}


/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin(int32 clientNum)
{
    int32 n;

    for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
        if (botSpawnQueue[n].clientNum == clientNum) {
            botSpawnQueue[n].spawnTime = 0;
            return;
        }
    }
}


/*
===============
G_BotConnect
===============
*/
bool G_BotConnect(int32 clientNum, bool restart)
{
    bot_settings_t settings;
    char           userinfo[MAX_INFO_STRING];

    trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

    Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));
    settings.skill = atof(Info_ValueForKey(userinfo, "skill"));
    Q_strncpyz(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));

    if (!BotAISetupClient(clientNum, &settings, restart)) {
        trap_DropClient(clientNum, "BotAISetupClient failed");
        return false;
    }

    return true;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot(const char* name, float skill, const char* team, int32 delay, char* altname)
{
    int32       clientNum;
    char*       botinfo;
    gentity_t*  bot;
    const char* key;
    const char* s;
    char*       botname;
    const char* model;
    const char* headmodel;
    char        userinfo[MAX_INFO_STRING];

    // get the botinfo from bots.txt
    botinfo = G_GetBotInfoByName(name);
    if (!botinfo) {
        logger.error("Bot '{}' not defined", name);
        return;
    }

    // create the bot's userinfo
    userinfo[0] = '\0';

    botname = Info_ValueForKey(botinfo, "funname");
    if (!botname[0]) {
        botname = Info_ValueForKey(botinfo, "name");
    }
    // check for an alternative name
    if (altname && altname[0]) {
        botname = altname;
    }
    Info_SetValueForKey(userinfo, "name", botname);
    Info_SetValueForKey(userinfo, "rate", "25000");
    Info_SetValueForKey(userinfo, "snaps", "20");
    Info_SetValueForKey(userinfo, "skill", va("%1.2f", skill));

    if (skill >= 1 && skill < 2) {
        Info_SetValueForKey(userinfo, "handicap", "50");
    } else if (skill >= 2 && skill < 3) {
        Info_SetValueForKey(userinfo, "handicap", "70");
    } else if (skill >= 3 && skill < 4) {
        Info_SetValueForKey(userinfo, "handicap", "90");
    }

    key = "model";
    model = Info_ValueForKey(botinfo, key);
    if (!*model) {
        model = "visor/default";
    }
    Info_SetValueForKey(userinfo, key, model);
    key = "team_model";
    Info_SetValueForKey(userinfo, key, model);

    key = "headmodel";
    headmodel = Info_ValueForKey(botinfo, key);
    if (!*headmodel) {
        headmodel = model;
    }
    Info_SetValueForKey(userinfo, key, headmodel);
    key = "team_headmodel";
    Info_SetValueForKey(userinfo, key, headmodel);

    key = "gender";
    s = Info_ValueForKey(botinfo, key);
    if (!*s) {
        s = "male";
    }
    Info_SetValueForKey(userinfo, "sex", s);

    key = "color1";
    s = Info_ValueForKey(botinfo, key);
    if (!*s) {
        s = "4";
    }
    Info_SetValueForKey(userinfo, key, s);

    key = "color2";
    s = Info_ValueForKey(botinfo, key);
    if (!*s) {
        s = "5";
    }
    Info_SetValueForKey(userinfo, key, s);

    s = Info_ValueForKey(botinfo, "aifile");
    if (!*s) {
        logger.error("Error: bot has no aifile specified");
        return;
    }

    // have the server allocate a client slot
    clientNum = trap_BotAllocateClient();
    if (clientNum == -1) {
        logger.error("Unable to add bot.  All player slots are in use.");
        logger.error("Start server with more 'open' slots (or check setting of sv_maxclients cvar).");
        return;
    }

    // initialize the bot settings
    if (!team || !*team) {
        if (g_gametype->integer >= GT_TEAM) {
            if (PickTeam(clientNum) == TEAM_RED) {
                team = "red";
            } else {
                team = "blue";
            }
        } else {
            team = "red";
        }
    }
    Info_SetValueForKey(userinfo, "characterfile", Info_ValueForKey(botinfo, "aifile"));
    Info_SetValueForKey(userinfo, "skill", va("%5.2f", skill));
    Info_SetValueForKey(userinfo, "team", team);

    bot = &g_entities[clientNum];
    bot->r.svFlags |= SVF_BOT;
    bot->inuse = true;

    // register the userinfo
    trap_SetUserinfo(clientNum, userinfo);

    // have it connect to the game as a normal client
    if (ClientConnect(clientNum, true, true)) {
        return;
    }

    if (delay == 0) {
        ClientBegin(clientNum);
        return;
    }

    AddBotToSpawnQueue(clientNum, delay);
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f()
{
    float skill;
    int32 delay;
    char  name[MAX_TOKEN_CHARS];
    char  altname[MAX_TOKEN_CHARS];
    char  string[MAX_TOKEN_CHARS];
    char  team[MAX_TOKEN_CHARS];

    // are bots enabled?
    if (!Cvar_VariableIntegerValue("bot_enable")) {
        return;
    }

    // name
    Cmd_ArgvBuffer(1, name, sizeof(name));
    if (!name[0]) {
        logger.info("Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]");
        return;
    }

    // skill
    Cmd_ArgvBuffer(2, string, sizeof(string));
    if (!string[0]) {
        skill = 4;
    } else {
        skill = atof(string);
    }

    // team
    Cmd_ArgvBuffer(3, team, sizeof(team));

    // delay
    Cmd_ArgvBuffer(4, string, sizeof(string));
    if (!string[0]) {
        delay = 0;
    } else {
        delay = atoi(string);
    }

    // alternative name
    Cmd_ArgvBuffer(5, altname, sizeof(altname));

    G_AddBot(name, skill, team, delay, altname);

    // if this was issued during gameplay and we are playing locally,
    // go ahead and load the bot's media immediately
    if (level.time - level.startTime > 1000 && Cvar_VariableIntegerValue("cl_running")) {
        trap_SendServerCommand(-1, "loaddefered\n"); // FIXME: spelled wrong, but not changing for demo
    }
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f()
{
    int32 i;
    char  name[MAX_TOKEN_CHARS];
    char  funname[MAX_TOKEN_CHARS];
    char  model[MAX_TOKEN_CHARS];
    char  aifile[MAX_TOKEN_CHARS];

    logger.info("^1name             model            aifile              funname");
    for (i = 0; i < g_numBots; i++) {
        strcpy(name, Info_ValueForKey(g_botInfos[i], "name"));
        if (!*name) {
            strcpy(name, "UnnamedPlayer");
        }
        strcpy(funname, Info_ValueForKey(g_botInfos[i], "funname"));
        if (!*funname) {
            strcpy(funname, "");
        }
        strcpy(model, Info_ValueForKey(g_botInfos[i], "model"));
        if (!*model) {
            strcpy(model, "visor/default");
        }
        strcpy(aifile, Info_ValueForKey(g_botInfos[i], "aifile"));
        if (!*aifile) {
            strcpy(aifile, "bots/default_c.c");
        }
        logger.info("%-16s %-16s %-20s %-20s", name, model, aifile, funname);
    }
}


/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots(char* botList, int32 baseDelay)
{
    char* bot;
    char* p;
    float skill;
    int32 delay;
    char  bots[MAX_INFO_VALUE];

    podium1 = NULL;
    podium2 = NULL;
    podium3 = NULL;

    skill = Cvar_VariableValue("g_spSkill");
    if (skill < 1) {
        Cvar_Set("g_spSkill", "1");
        skill = 1;
    } else if (skill > 5) {
        Cvar_Set("g_spSkill", "5");
        skill = 5;
    }

    Q_strncpyz(bots, botList, sizeof(bots));
    p = &bots[0];
    delay = baseDelay;
    while (*p) {
        // skip spaces
        while (*p && *p == ' ') {
            p++;
        }
        if (!p) {
            break;
        }

        // mark start of bot name
        bot = p;

        // skip until space of null
        while (*p && *p != ' ') {
            p++;
        }
        if (*p) {
            *p++ = 0;
        }

        // we must add the bot this way, calling G_AddBot directly at this stage
        // does "Bad Things"
        Cbuf_ExecuteText(EXEC_INSERT, va("addbot %s %f free %i\n", bot, skill, delay));

        delay += BOT_BEGIN_DELAY_INCREMENT;
    }
}


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile(const char* filename)
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
        logger.error("File too large: {} is {}, max allowed is {}", filename, len, MAX_BOTS_TEXT);
        FS_FCloseFile(f);
        return;
    }

    FS_Read2(buf, len, f);
    buf[len] = 0;
    FS_FCloseFile(f);

    g_numBots += G_ParseInfos(buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots]);
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots()
{
    int32 numdirs;
    char  filename[128];
    char  dirlist[1024];
    char* dirptr;
    int32 i;
    int32 dirlen;

    if (!Cvar_VariableIntegerValue("bot_enable")) {
        return;
    }

    g_numBots = 0;

    G_LoadBotsFromFile("scripts/bots.txt");

    // get all bots from .bot files
    numdirs = FS_GetFileList("scripts", ".bot", dirlist, 1024);
    dirptr = dirlist;
    for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
        dirlen = strlen(dirptr);
        strcpy(filename, "scripts/");
        strcat(filename, dirptr);
        G_LoadBotsFromFile(filename);
    }
    logger.info("{} bots parsed", g_numBots);
}

/*
===============
G_GetBotInfoByName
===============
*/
char* G_GetBotInfoByName(const char* name)
{
    int32 n;
    char* value;

    for (n = 0; n < g_numBots; n++) {
        value = Info_ValueForKey(g_botInfos[n], "name");
        if (!Q_stricmp(value, name)) {
            return g_botInfos[n];
        }
    }

    return NULL;
}

/*
===============
G_InitBots
===============
*/
void G_InitBots(bool restart)
{
    int32       fragLimit;
    int32       timeLimit;
    const char* arenainfo;
    char*       strValue;
    int32       basedelay;
    char        map[MAX_QPATH];
    char        serverinfo[MAX_INFO_STRING];

    G_LoadBots();
    G_LoadArenas();

    bot_minplayers = Cvar_Get("bot_minplayers", "0", CVAR_SERVERINFO);

    if (g_gametype->integer == GT_SINGLE_PLAYER) {
        trap_GetServerinfo(serverinfo, sizeof(serverinfo));
        Q_strncpyz(map, Info_ValueForKey(serverinfo, "mapname"), sizeof(map));
        arenainfo = G_GetArenaInfoByMap(map);
        if (!arenainfo) {
            return;
        }

        strValue = Info_ValueForKey(arenainfo, "fraglimit");
        fragLimit = atoi(strValue);
        if (fragLimit) {
            Cvar_Set("fraglimit", strValue);
        } else {
            Cvar_Set("fraglimit", "0");
        }

        strValue = Info_ValueForKey(arenainfo, "timelimit");
        timeLimit = atoi(strValue);
        if (timeLimit) {
            Cvar_Set("timelimit", strValue);
        } else {
            Cvar_Set("timelimit", "0");
        }

        if (!fragLimit && !timeLimit) {
            Cvar_Set("fraglimit", "10");
            Cvar_Set("timelimit", "0");
        }

        basedelay = BOT_BEGIN_DELAY_BASE;
        strValue = Info_ValueForKey(arenainfo, "special");
        if (Q_stricmp(strValue, "training") == 0) {
            basedelay += 10000;
        }

        if (!restart) {
            G_SpawnBots(Info_ValueForKey(arenainfo, "bots"), basedelay);
        }
    }
}
