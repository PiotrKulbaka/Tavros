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

#include "g_local.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_ea.h"
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_gen.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
//
#include "chars.h" //characteristics
#include "inv.h"   //indexes into the inventory
#include "syn.h"   //synonyms
#include "match.h" //string matching types and vars

#define TIME_BETWEENCHATTING 25


/*
==================
BotNumActivePlayers
==================
*/
int32 BotNumActivePlayers()
{
    int32        i, num;
    char         buf[MAX_INFO_STRING];
    static int32 maxclients;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    num = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        //
        num++;
    }
    return num;
}

/*
==================
BotIsFirstInRankings
==================
*/
int32 BotIsFirstInRankings(bot_state_t* bs)
{
    int32         i, score;
    char          buf[MAX_INFO_STRING];
    static int32  maxclients;
    playerState_t ps;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    score = bs->cur_ps.persistant[PERS_SCORE];
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        //
        BotAI_GetClientState(i, &ps);
        if (score < ps.persistant[PERS_SCORE]) {
            return false;
        }
    }
    return true;
}

/*
==================
BotIsLastInRankings
==================
*/
int32 BotIsLastInRankings(bot_state_t* bs)
{
    int32         i, score;
    char          buf[MAX_INFO_STRING];
    static int32  maxclients;
    playerState_t ps;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    score = bs->cur_ps.persistant[PERS_SCORE];
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        //
        BotAI_GetClientState(i, &ps);
        if (score > ps.persistant[PERS_SCORE]) {
            return false;
        }
    }
    return true;
}

/*
==================
BotFirstClientInRankings
==================
*/
char* BotFirstClientInRankings()
{
    int32         i, bestscore, bestclient;
    char          buf[MAX_INFO_STRING];
    static char   name[32];
    static int32  maxclients;
    playerState_t ps;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    bestscore = -999999;
    bestclient = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        //
        BotAI_GetClientState(i, &ps);
        if (ps.persistant[PERS_SCORE] > bestscore) {
            bestscore = ps.persistant[PERS_SCORE];
            bestclient = i;
        }
    }
    EasyClientName(bestclient, name, 32);
    return name;
}

/*
==================
BotLastClientInRankings
==================
*/
char* BotLastClientInRankings()
{
    int32         i, worstscore, bestclient;
    char          buf[MAX_INFO_STRING];
    static char   name[32];
    static int32  maxclients;
    playerState_t ps;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    worstscore = 999999;
    bestclient = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        //
        BotAI_GetClientState(i, &ps);
        if (ps.persistant[PERS_SCORE] < worstscore) {
            worstscore = ps.persistant[PERS_SCORE];
            bestclient = i;
        }
    }
    EasyClientName(bestclient, name, 32);
    return name;
}

/*
==================
BotRandomOpponentName
==================
*/
char* BotRandomOpponentName(bot_state_t* bs)
{
    int32        i, count;
    char         buf[MAX_INFO_STRING];
    int32        opponents[MAX_CLIENTS], numopponents;
    static int32 maxclients;
    static char  name[32];

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    numopponents = 0;
    opponents[0] = 0;
    for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
        if (i == bs->client) {
            continue;
        }
        //
        trap_GetConfigstring(CS_PLAYERS + i, buf, sizeof(buf));
        // if no config string or no name
        if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) {
            continue;
        }
        // skip spectators
        if (atoi(Info_ValueForKey(buf, "t")) == TEAM_SPECTATOR) {
            continue;
        }
        // skip team mates
        if (BotSameTeam(bs, i)) {
            continue;
        }
        //
        opponents[numopponents] = i;
        numopponents++;
    }
    count = random() * numopponents;
    for (i = 0; i < numopponents; i++) {
        count--;
        if (count <= 0) {
            EasyClientName(opponents[i], name, sizeof(name));
            return name;
        }
    }
    EasyClientName(opponents[0], name, sizeof(name));
    return name;
}

/*
==================
BotMapTitle
==================
*/

char* BotMapTitle()
{
    char        info[1024];
    static char mapname[128];

    trap_GetServerinfo(info, sizeof(info));

    strncpy(mapname, Info_ValueForKey(info, "mapname"), sizeof(mapname) - 1);
    mapname[sizeof(mapname) - 1] = '\0';

    return mapname;
}


/*
==================
BotWeaponNameForMeansOfDeath
==================
*/

static const char* BotWeaponNameForMeansOfDeath(int32 mod)
{
    switch (mod) {
    case MOD_SHOTGUN:
        return "Shotgun";
    case MOD_GAUNTLET:
        return "Gauntlet";
    case MOD_MACHINEGUN:
        return "Machinegun";
    case MOD_GRENADE:
    case MOD_GRENADE_SPLASH:
        return "Grenade Launcher";
    case MOD_ROCKET:
    case MOD_ROCKET_SPLASH:
        return "Rocket Launcher";
    case MOD_PLASMA:
    case MOD_PLASMA_SPLASH:
        return "Plasmagun";
    case MOD_RAILGUN:
        return "Railgun";
    case MOD_LIGHTNING:
        return "Lightning Gun";
    case MOD_BFG:
    case MOD_BFG_SPLASH:
        return "BFG10K";
    case MOD_GRAPPLE:
        return "Grapple";
    default:
        return "[unknown weapon]";
    }
}

/*
==================
BotRandomWeaponName
==================
*/
static const char* BotRandomWeaponName()
{
    int32 rnd;
    rnd = random() * 8.9;
    switch (rnd) {
    case 0:
        return "Gauntlet";
    case 1:
        return "Shotgun";
    case 2:
        return "Machinegun";
    case 3:
        return "Grenade Launcher";
    case 4:
        return "Rocket Launcher";
    case 5:
        return "Plasmagun";
    case 6:
        return "Railgun";
    case 7:
        return "Lightning Gun";
    default:
        return "BFG10K";
    }
}

/*
==================
BotVisibleEnemies
==================
*/
int32 BotVisibleEnemies(bot_state_t* bs)
{
    float            vis;
    int32            i;
    aas_entityinfo_t entinfo;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (i == bs->client) {
            continue;
        }
        //
        BotEntityInfo(i, &entinfo);
        //
        if (!entinfo.valid) {
            continue;
        }
        // if the enemy isn't dead and the enemy isn't the bot self
        if (EntityIsDead(&entinfo) || entinfo.number == bs->entitynum) {
            continue;
        }
        // if the enemy is invisible and not shooting
        if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
            continue;
        }
        // if on the same team
        if (BotSameTeam(bs, i)) {
            continue;
        }
        // check if the enemy is visible
        vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
        if (vis > 0) {
            return true;
        }
    }
    return false;
}

/*
==================
BotValidChatPosition
==================
*/
int32 BotValidChatPosition(bot_state_t* bs)
{
    vec3_t      point, start, end, mins, maxs;
    bsp_trace_t trace;

    // if the bot is dead all positions are valid
    if (BotIsDead(bs)) {
        return true;
    }
    // never start chatting with a powerup
    if (bs->inventory[INVENTORY_QUAD] || bs->inventory[INVENTORY_HASTE] || bs->inventory[INVENTORY_INVISIBILITY] || bs->inventory[INVENTORY_REGEN] || bs->inventory[INVENTORY_FLIGHT]) {
        return false;
    }
    // must be on the ground
    // if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) return false;
    // do not chat if in lava or slime
    VectorCopy(bs->origin, point);
    point[2] -= 24;
    if (trap_PointContents(point, bs->entitynum) & (CONTENTS_LAVA | CONTENTS_SLIME)) {
        return false;
    }
    // do not chat if under water
    VectorCopy(bs->origin, point);
    point[2] += 32;
    if (trap_PointContents(point, bs->entitynum) & MASK_WATER) {
        return false;
    }
    // must be standing on the world entity
    VectorCopy(bs->origin, start);
    VectorCopy(bs->origin, end);
    start[2] += 1;
    end[2] -= 10;
    trap_AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, mins, maxs);
    BotAI_Trace(&trace, start, mins, maxs, end, bs->client, MASK_SOLID);
    if (trace.ent != ENTITYNUM_WORLD) {
        return false;
    }
    // the bot is in a position where it can chat
    return true;
}

/*
==================
BotChat_EnterGame
==================
*/
int32 BotChat_EnterGame(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1);
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    BotAI_BotInitialChat(bs, "game_enter", EasyClientName(bs->client, name, 32), // 0
                         BotRandomOpponentName(bs),                              // 1
                         "[invalid var]",                                        // 2
                         "[invalid var]",                                        // 3
                         BotMapTitle(),                                          // 4
                         NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_ExitGame
==================
*/
int32 BotChat_ExitGame(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1);
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    //
    BotAI_BotInitialChat(bs, "game_exit", EasyClientName(bs->client, name, 32), // 0
                         BotRandomOpponentName(bs),                             // 1
                         "[invalid var]",                                       // 2
                         "[invalid var]",                                       // 3
                         BotMapTitle(),                                         // 4
                         NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_StartLevel
==================
*/
int32 BotChat_StartLevel(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (BotIsObserver(bs)) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        trap_EA_Command(bs->client, "vtaunt");
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1);
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    BotAI_BotInitialChat(bs, "level_start", EasyClientName(bs->client, name, 32), // 0
                         NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_EndLevel
==================
*/
int32 BotChat_EndLevel(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (BotIsObserver(bs)) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    // teamplay
    if (TeamPlayIsOn()) {
        if (BotIsFirstInRankings(bs)) {
            trap_EA_Command(bs->client, "vtaunt");
        }
        return true;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1);
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    //
    if (BotIsFirstInRankings(bs)) {
        BotAI_BotInitialChat(bs, "level_end_victory", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                                     // 1
                             "[invalid var]",                                               // 2
                             BotLastClientInRankings(),                                     // 3
                             BotMapTitle(),                                                 // 4
                             NULL);
    } else if (BotIsLastInRankings(bs)) {
        BotAI_BotInitialChat(bs, "level_end_lose", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                                  // 1
                             BotFirstClientInRankings(),                                 // 2
                             "[invalid var]",                                            // 3
                             BotMapTitle(),                                              // 4
                             NULL);
    } else {
        BotAI_BotInitialChat(bs, "level_end", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                             // 1
                             BotFirstClientInRankings(),                            // 2
                             BotLastClientInRankings(),                             // 3
                             BotMapTitle(),                                         // 4
                             NULL);
    }
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_Death
==================
*/
int32 BotChat_Death(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_DEATH, 0, 1);
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chatting is off
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    //
    if (bs->lastkilledby >= 0 && bs->lastkilledby < MAX_CLIENTS) {
        EasyClientName(bs->lastkilledby, name, 32);
    } else {
        strcpy(name, "[world]");
    }
    //
    if (TeamPlayIsOn() && BotSameTeam(bs, bs->lastkilledby)) {
        if (bs->lastkilledby == bs->client) {
            return false;
        }
        BotAI_BotInitialChat(bs, "death_teammate", name, NULL);
        bs->chatto = CHAT_TEAM;
    } else {
        // teamplay
        if (TeamPlayIsOn()) {
            trap_EA_Command(bs->client, "vtaunt");
            return true;
        }
        //
        if (bs->botdeathtype == MOD_WATER) {
            BotAI_BotInitialChat(bs, "death_drown", BotRandomOpponentName(bs), NULL);
        } else if (bs->botdeathtype == MOD_SLIME) {
            BotAI_BotInitialChat(bs, "death_slime", BotRandomOpponentName(bs), NULL);
        } else if (bs->botdeathtype == MOD_LAVA) {
            BotAI_BotInitialChat(bs, "death_lava", BotRandomOpponentName(bs), NULL);
        } else if (bs->botdeathtype == MOD_FALLING) {
            BotAI_BotInitialChat(bs, "death_cratered", BotRandomOpponentName(bs), NULL);
        } else if (bs->botsuicide || // all other suicides by own weapon
                   bs->botdeathtype == MOD_CRUSH || bs->botdeathtype == MOD_SUICIDE || bs->botdeathtype == MOD_TARGET_LASER || bs->botdeathtype == MOD_TRIGGER_HURT || bs->botdeathtype == MOD_UNKNOWN) {
            BotAI_BotInitialChat(bs, "death_suicide", BotRandomOpponentName(bs), NULL);
        } else if (bs->botdeathtype == MOD_TELEFRAG) {
            BotAI_BotInitialChat(bs, "death_telefrag", name, NULL);
        } else {
            if ((bs->botdeathtype == MOD_GAUNTLET || bs->botdeathtype == MOD_RAILGUN || bs->botdeathtype == MOD_BFG || bs->botdeathtype == MOD_BFG_SPLASH) && random() < 0.5) {
                if (bs->botdeathtype == MOD_GAUNTLET) {
                    BotAI_BotInitialChat(bs, "death_gauntlet",
                                         name,                                           // 0
                                         BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                                         NULL);
                } else if (bs->botdeathtype == MOD_RAILGUN) {
                    BotAI_BotInitialChat(bs, "death_rail",
                                         name,                                           // 0
                                         BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                                         NULL);
                } else {
                    BotAI_BotInitialChat(bs, "death_bfg",
                                         name,                                           // 0
                                         BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                                         NULL);
                }
            }
            // choose between insult and praise
            else if (random() < trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1)) {
                BotAI_BotInitialChat(bs, "death_insult",
                                     name,                                           // 0
                                     BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                                     NULL);
            } else {
                BotAI_BotInitialChat(bs, "death_praise",
                                     name,                                           // 0
                                     BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                                     NULL);
            }
        }
        bs->chatto = CHAT_ALL;
    }
    bs->lastchat_time = FloatTime();
    return true;
}

/*
==================
BotChat_Kill
==================
*/
int32 BotChat_Kill(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_KILL, 0, 1);
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chat is off
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (bs->lastkilledplayer == bs->client) {
        return false;
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    if (BotVisibleEnemies(bs)) {
        return false;
    }
    //
    EasyClientName(bs->lastkilledplayer, name, 32);
    //
    bs->chatto = CHAT_ALL;
    if (TeamPlayIsOn() && BotSameTeam(bs, bs->lastkilledplayer)) {
        BotAI_BotInitialChat(bs, "kill_teammate", name, NULL);
        bs->chatto = CHAT_TEAM;
    } else {
        // don't chat in teamplay
        if (TeamPlayIsOn()) {
            trap_EA_Command(bs->client, "vtaunt");
            return false; // don't wait
        }
        //
        if (bs->enemydeathtype == MOD_GAUNTLET) {
            BotAI_BotInitialChat(bs, "kill_gauntlet", name, NULL);
        } else if (bs->enemydeathtype == MOD_RAILGUN) {
            BotAI_BotInitialChat(bs, "kill_rail", name, NULL);
        } else if (bs->enemydeathtype == MOD_TELEFRAG) {
            BotAI_BotInitialChat(bs, "kill_telefrag", name, NULL);
        }

        // choose between insult and praise
        else if (random() < trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1)) {
            BotAI_BotInitialChat(bs, "kill_insult", name, NULL);
        } else {
            BotAI_BotInitialChat(bs, "kill_praise", name, NULL);
        }
    }
    bs->lastchat_time = FloatTime();
    return true;
}

/*
==================
BotChat_EnemySuicide
==================
*/
int32 BotChat_EnemySuicide(bot_state_t* bs)
{
    char  name[32];
    float rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    //
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_KILL, 0, 1);
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chat is off
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    if (BotVisibleEnemies(bs)) {
        return false;
    }
    //
    if (bs->enemy >= 0) {
        EasyClientName(bs->enemy, name, 32);
    } else {
        strcpy(name, "");
    }
    BotAI_BotInitialChat(bs, "enemy_suicide", name, NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_HitTalking
==================
*/
int32 BotChat_HitTalking(bot_state_t* bs)
{
    char        name[32];
    const char* weap;
    int32       lasthurt_client;
    float       rnd;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    lasthurt_client = g_entities[bs->client].client->lasthurt_client;
    if (!lasthurt_client) {
        return false;
    }
    if (lasthurt_client == bs->client) {
        return false;
    }
    //
    if (lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS) {
        return false;
    }
    //
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITTALKING, 0, 1);
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chat is off
    if (!bot_fastchat->integer) {
        if (random() > rnd * 0.5) {
            return false;
        }
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    ClientName(g_entities[bs->client].client->lasthurt_client, name, sizeof(name));
    weap = BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_client);
    //
    BotAI_BotInitialChat(bs, "hit_talking", name, weap, NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_HitNoDeath
==================
*/
int32 BotChat_HitNoDeath(bot_state_t* bs)
{
    char             name[32];
    const char*      weap;
    float            rnd;
    int32            lasthurt_client;
    aas_entityinfo_t entinfo;

    lasthurt_client = g_entities[bs->client].client->lasthurt_client;
    if (!lasthurt_client) {
        return false;
    }
    if (lasthurt_client == bs->client) {
        return false;
    }
    //
    if (lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS) {
        return false;
    }
    //
    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITNODEATH, 0, 1);
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chat is off
    if (!bot_fastchat->integer) {
        if (random() > rnd * 0.5) {
            return false;
        }
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    if (BotVisibleEnemies(bs)) {
        return false;
    }
    //
    BotEntityInfo(bs->enemy, &entinfo);
    if (EntityIsShooting(&entinfo)) {
        return false;
    }
    //
    ClientName(lasthurt_client, name, sizeof(name));
    weap = BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_mod);
    //
    BotAI_BotInitialChat(bs, "hit_nodeath", name, weap, NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_HitNoKill
==================
*/
int32 BotChat_HitNoKill(bot_state_t* bs)
{
    char             name[32];
    const char*      weap;
    float            rnd;
    aas_entityinfo_t entinfo;

    if (bot_nochat->integer) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_HITNOKILL, 0, 1);
    // don't chat in teamplay
    if (TeamPlayIsOn()) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // if fast chat is off
    if (!bot_fastchat->integer) {
        if (random() > rnd * 0.5) {
            return false;
        }
    }
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    if (BotVisibleEnemies(bs)) {
        return false;
    }
    //
    BotEntityInfo(bs->enemy, &entinfo);
    if (EntityIsShooting(&entinfo)) {
        return false;
    }
    //
    ClientName(bs->enemy, name, sizeof(name));
    weap = BotWeaponNameForMeansOfDeath(g_entities[bs->enemy].client->lasthurt_mod);
    //
    BotAI_BotInitialChat(bs, "hit_nokill", name, weap, NULL);
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChat_Random
==================
*/
int32 BotChat_Random(bot_state_t* bs)
{
    float rnd;
    char  name[32];

    if (bot_nochat->integer) {
        return false;
    }
    if (BotIsObserver(bs)) {
        return false;
    }
    if (bs->lastchat_time > FloatTime() - TIME_BETWEENCHATTING) {
        return false;
    }
    // don't chat in tournament mode
    if (gametype == GT_TOURNAMENT) {
        return false;
    }
    // don't chat when doing something important :)
    if (bs->ltgtype == LTG_TEAMHELP || bs->ltgtype == LTG_TEAMACCOMPANY || bs->ltgtype == LTG_RUSHBASE) {
        return false;
    }
    //
    rnd = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_RANDOM, 0, 1);
    if (random() > bs->thinktime * 0.1) {
        return false;
    }
    if (!bot_fastchat->integer) {
        if (random() > rnd) {
            return false;
        }
        if (random() > 0.25) {
            return false;
        }
    }
    if (BotNumActivePlayers() <= 1) {
        return false;
    }
    //
    if (!BotValidChatPosition(bs)) {
        return false;
    }
    //
    if (BotVisibleEnemies(bs)) {
        return false;
    }
    //
    if (bs->lastkilledplayer == bs->client) {
        strcpy(name, BotRandomOpponentName(bs));
    } else {
        EasyClientName(bs->lastkilledplayer, name, sizeof(name));
    }
    if (TeamPlayIsOn()) {
        trap_EA_Command(bs->client, "vtaunt");
        return false; // don't wait
    }
    //
    if (random() < trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CHAT_MISC, 0, 1)) {
        BotAI_BotInitialChat(bs, "random_misc",
                             BotRandomOpponentName(bs), // 0
                             name,                      // 1
                             "[invalid var]",           // 2
                             "[invalid var]",           // 3
                             BotMapTitle(),             // 4
                             BotRandomWeaponName(),     // 5
                             NULL);
    } else {
        BotAI_BotInitialChat(bs, "random_insult",
                             BotRandomOpponentName(bs), // 0
                             name,                      // 1
                             "[invalid var]",           // 2
                             "[invalid var]",           // 3
                             BotMapTitle(),             // 4
                             BotRandomWeaponName(),     // 5
                             NULL);
    }
    bs->lastchat_time = FloatTime();
    bs->chatto = CHAT_ALL;
    return true;
}

/*
==================
BotChatTime
==================
*/
float BotChatTime(bot_state_t* bs)
{
    int32 cpm;

    cpm = trap_Characteristic_BInteger(bs->character, CHARACTERISTIC_CHAT_CPM, 1, 4000);

    return 2.0;
}

/*
==================
BotChatTest
==================
*/
void BotChatTest(bot_state_t* bs)
{
    char        name[32];
    const char* weap;
    int32       num, i;

    num = trap_BotNumInitialChats(bs->cs, "game_enter");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "game_enter", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                              // 1
                             "[invalid var]",                                        // 2
                             "[invalid var]",                                        // 3
                             BotMapTitle(),                                          // 4
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "game_exit");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "game_exit", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                             // 1
                             "[invalid var]",                                       // 2
                             "[invalid var]",                                       // 3
                             BotMapTitle(),                                         // 4
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "level_start");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "level_start", EasyClientName(bs->client, name, 32), // 0
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "level_end_victory");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "level_end_victory", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                                     // 1
                             BotFirstClientInRankings(),                                    // 2
                             BotLastClientInRankings(),                                     // 3
                             BotMapTitle(),                                                 // 4
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "level_end_lose");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "level_end_lose", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                                  // 1
                             BotFirstClientInRankings(),                                 // 2
                             BotLastClientInRankings(),                                  // 3
                             BotMapTitle(),                                              // 4
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "level_end");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "level_end", EasyClientName(bs->client, name, 32), // 0
                             BotRandomOpponentName(bs),                             // 1
                             BotFirstClientInRankings(),                            // 2
                             BotLastClientInRankings(),                             // 3
                             BotMapTitle(),                                         // 4
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    EasyClientName(bs->lastkilledby, name, sizeof(name));
    num = trap_BotNumInitialChats(bs->cs, "death_drown");
    for (i = 0; i < num; i++) {
        //
        BotAI_BotInitialChat(bs, "death_drown", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_slime");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_slime", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_lava");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_lava", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_cratered");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_cratered", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_suicide");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_suicide", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_telefrag");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_telefrag", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_gauntlet");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_gauntlet",
                             name,                                           // 0
                             BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_rail");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_rail",
                             name,                                           // 0
                             BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_bfg");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_bfg",
                             name,                                           // 0
                             BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_insult");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_insult",
                             name,                                           // 0
                             BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "death_praise");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "death_praise",
                             name,                                           // 0
                             BotWeaponNameForMeansOfDeath(bs->botdeathtype), // 1
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    //
    EasyClientName(bs->lastkilledplayer, name, 32);
    //
    num = trap_BotNumInitialChats(bs->cs, "kill_gauntlet");
    for (i = 0; i < num; i++) {
        //
        BotAI_BotInitialChat(bs, "kill_gauntlet", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "kill_rail");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "kill_rail", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "kill_telefrag");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "kill_telefrag", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "kill_insult");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "kill_insult", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "kill_praise");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "kill_praise", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "enemy_suicide");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "enemy_suicide", name, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    ClientName(g_entities[bs->client].client->lasthurt_client, name, sizeof(name));
    weap = BotWeaponNameForMeansOfDeath(g_entities[bs->client].client->lasthurt_client);
    num = trap_BotNumInitialChats(bs->cs, "hit_talking");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "hit_talking", name, weap, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "hit_nodeath");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "hit_nodeath", name, weap, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "hit_nokill");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "hit_nokill", name, weap, NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    //
    if (bs->lastkilledplayer == bs->client) {
        strcpy(name, BotRandomOpponentName(bs));
    } else {
        EasyClientName(bs->lastkilledplayer, name, sizeof(name));
    }
    //
    num = trap_BotNumInitialChats(bs->cs, "random_misc");
    for (i = 0; i < num; i++) {
        //
        BotAI_BotInitialChat(bs, "random_misc",
                             BotRandomOpponentName(bs), // 0
                             name,                      // 1
                             "[invalid var]",           // 2
                             "[invalid var]",           // 3
                             BotMapTitle(),             // 4
                             BotRandomWeaponName(),     // 5
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
    num = trap_BotNumInitialChats(bs->cs, "random_insult");
    for (i = 0; i < num; i++) {
        BotAI_BotInitialChat(bs, "random_insult",
                             BotRandomOpponentName(bs), // 0
                             name,                      // 1
                             "[invalid var]",           // 2
                             "[invalid var]",           // 3
                             BotMapTitle(),             // 4
                             BotRandomWeaponName(),     // 5
                             NULL);
        trap_BotEnterChat(bs->cs, 0, CHAT_ALL);
    }
}
