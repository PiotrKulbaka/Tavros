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

#include "g_local.h"

static tavros::core::logger logger("g_main");

level_locals_t level;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

cvar_t* g_gametype;
cvar_t* g_dmflags;
cvar_t* g_fraglimit;
cvar_t* g_timelimit;
cvar_t* g_capturelimit;
cvar_t* g_friendlyFire;
cvar_t* g_password;
cvar_t* g_maxclients;
cvar_t* g_maxGameClients;
cvar_t* g_dedicated;
cvar_t* g_speed;
cvar_t* g_gravity;
cvar_t* g_cheats;
cvar_t* g_knockback;
cvar_t* g_quadfactor;
cvar_t* g_forcerespawn;
cvar_t* g_inactivity;
cvar_t* g_debugDamage;
cvar_t* g_debugAlloc;
cvar_t* g_weaponRespawn;
cvar_t* g_weaponTeamRespawn;
cvar_t* g_motd;
cvar_t* g_synchronousClients;
cvar_t* g_warmup;
cvar_t* g_doWarmup;
cvar_t* g_restarted;
cvar_t* g_podiumDist;
cvar_t* g_podiumDrop;
cvar_t* g_allowVote;
cvar_t* g_teamAutoJoin;
cvar_t* g_teamForceBalance;
cvar_t* g_banIPs;
cvar_t* g_filterBan;
cvar_t* g_smoothClients;
cvar_t* g_rankings;
cvar_t* g_listEntity;

void CheckExitRules();

void QDECL G_Error(const char* fmt, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, fmt);
    vsprintf(text, fmt, argptr);
    va_end(argptr);

    Com_Error(ERR_DROP, "%s", text);
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams()
{
    gentity_t *e, *e2;
    int32      i, j;
    int32      c, c2;

    c = 0;
    c2 = 0;
    for (i = 1, e = g_entities + i; i < level.num_entities; i++, e++) {
        if (!e->inuse) {
            continue;
        }
        if (!e->team) {
            continue;
        }
        if (e->flags & FL_TEAMSLAVE) {
            continue;
        }
        e->teammaster = e;
        c++;
        c2++;
        for (j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++) {
            if (!e2->inuse) {
                continue;
            }
            if (!e2->team) {
                continue;
            }
            if (e2->flags & FL_TEAMSLAVE) {
                continue;
            }
            if (!strcmp(e->team, e2->team)) {
                c2++;
                e2->teamchain = e->teamchain;
                e->teamchain = e2;
                e2->teammaster = e;
                e2->flags |= FL_TEAMSLAVE;

                // make sure that targets only point at the master
                if (e2->targetname) {
                    e->targetname = e2->targetname;
                    e2->targetname = NULL;
                }
            }
        }
    }

    logger.info("%i teams with %i entities", c, c2);
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars()
{
    // don't override the cheat state set by the system
    g_cheats = Cvar_Get("sv_cheats", "", 0);

    // noset vars
    Cvar_Get("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_ROM);
    Cvar_Get("gamedate", __DATE__, CVAR_ROM);
    g_restarted = Cvar_Get("g_restarted", "0", CVAR_ROM);
    Cvar_Get("sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM);

    // latched vars
    g_gametype = Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH);

    g_maxclients = Cvar_Get("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE);
    g_maxGameClients = Cvar_Get("g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE);

    // change anytime vars
    g_dmflags = Cvar_Get("dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE);
    g_fraglimit = Cvar_Get("fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART);
    g_timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART);
    g_capturelimit = Cvar_Get("capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART);

    g_synchronousClients = Cvar_Get("g_synchronousClients", "1", CVAR_SYSTEMINFO);

    g_friendlyFire = Cvar_Get("g_friendlyFire", "0", CVAR_ARCHIVE);

    g_teamAutoJoin = Cvar_Get("g_teamAutoJoin", "0", CVAR_ARCHIVE);
    g_teamForceBalance = Cvar_Get("g_teamForceBalance", "0", CVAR_ARCHIVE);

    g_warmup = Cvar_Get("g_warmup", "20", CVAR_ARCHIVE);
    g_doWarmup = Cvar_Get("g_doWarmup", "0", 0);

    g_password = Cvar_Get("g_password", "", CVAR_USERINFO);

    g_banIPs = Cvar_Get("g_banIPs", "", CVAR_ARCHIVE);
    g_filterBan = Cvar_Get("g_filterBan", "1", CVAR_ARCHIVE);

    g_dedicated = Cvar_Get("dedicated", "0", 0);

    g_speed = Cvar_Get("g_speed", "320", 0);
    g_gravity = Cvar_Get("g_gravity", "800", 0);
    g_knockback = Cvar_Get("g_knockback", "1000", 0);
    g_quadfactor = Cvar_Get("g_quadfactor", "3", 0);
    g_weaponRespawn = Cvar_Get("g_weaponrespawn", "5", 0);
    g_weaponTeamRespawn = Cvar_Get("g_weaponTeamRespawn", "30", 0);
    g_forcerespawn = Cvar_Get("g_forcerespawn", "20", 0);
    g_inactivity = Cvar_Get("g_inactivity", "0", 0);
    g_debugDamage = Cvar_Get("g_debugDamage", "1", 0);
    g_debugAlloc = Cvar_Get("g_debugAlloc", "0", 0);
    g_motd = Cvar_Get("g_motd", "", 0);

    g_podiumDist = Cvar_Get("g_podiumDist", "80", 0);
    g_podiumDrop = Cvar_Get("g_podiumDrop", "70", 0);

    g_allowVote = Cvar_Get("g_allowVote", "1", CVAR_ARCHIVE);
    g_listEntity = Cvar_Get("g_listEntity", "0", 0);

    g_smoothClients = Cvar_Get("g_smoothClients", "1", 0);

    g_rankings = Cvar_Get("g_rankings", "0", 0);

    // check some things
    if (g_gametype->integer < 0 || g_gametype->integer >= GT_MAX_GAME_TYPE) {
        logger.info("g_gametype %i is out of range, defaulting to 0", g_gametype->integer);
        Cvar_Set("g_gametype", "0");
    }
}

static bool s_game_is_init = false;
/*
============
G_InitGame

============
*/
void G_InitGame(int32 levelTime, int32 randomSeed, int32 restart)
{
    int32 i;

    logger.info("------- Game Initialization -------");
    logger.info("gamename: %s", GAMEVERSION);
    logger.info("gamedate: %s", __DATE__);

    s_game_is_init = true;
    srand(randomSeed);

    G_RegisterCvars();

    G_ProcessIPBans();

    G_InitMemory();

    // set some level globals
    memset(&level, 0, sizeof(level));
    level.time = levelTime;
    level.startTime = levelTime;

    level.snd_fry = G_SoundIndex("sound/player/fry.wav"); // FIXME standing in lava / slime

    G_InitWorldSession();

    // initialize all entities for this game
    memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
    level.gentities = g_entities;

    // initialize all clients for this game
    level.maxclients = g_maxclients->integer;
    memset(g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]));
    level.clients = g_clients;

    // set client fields on player ents
    for (i = 0; i < level.maxclients; i++) {
        g_entities[i].client = level.clients + i;
    }

    // always leave room for the max number of clients,
    // even if they aren't all used, so numbers inside that
    // range are NEVER anything but clients
    level.num_entities = MAX_CLIENTS;

    // let the server system know where the entites are
    trap_LocateGameData(level.gentities, level.num_entities, sizeof(gentity_t), &level.clients[0].ps, sizeof(level.clients[0]));

    // reserve some spots for dead player bodies
    InitBodyQue();

    ClearRegisteredItems();

    // parse the key/value pairs and spawn gentities
    G_SpawnEntitiesFromString();

    // general initialization
    G_FindTeams();

    // make sure we have flags for CTF, etc
    if (g_gametype->integer >= GT_TEAM) {
        G_CheckTeamItems();
    }

    SaveRegisteredItems();

    logger.info("-----------------------------------");

    if (g_gametype->integer == GT_SINGLE_PLAYER) {
        G_ModelIndex(SP_PODIUM_MODEL);
        G_SoundIndex("sound/player/gurp1.wav");
        G_SoundIndex("sound/player/gurp2.wav");
    }

    if (Cvar_VariableIntegerValue("bot_enable")) {
        BotAISetup(restart);
        BotAILoadMap(restart);
        G_InitBots(restart);
    }
}


/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame(int32 restart)
{
    if (!s_game_is_init) {
        return;
    }
    s_game_is_init = false;
    logger.info("==== ShutdownGame ====");

    // write all the client session data so we can get it back
    G_WriteSessionData();

    if (Cvar_VariableIntegerValue("bot_enable")) {
        BotAIShutdown(restart);
    }
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer()
{
    int32      i;
    gclient_t* client;
    gclient_t* nextInLine;

    if (level.numPlayingClients >= 2) {
        return;
    }

    // never change during intermission
    if (level.intermissiontime) {
        return;
    }

    nextInLine = NULL;

    for (i = 0; i < level.maxclients; i++) {
        client = &level.clients[i];
        if (client->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (client->sess.sessionTeam != TEAM_SPECTATOR) {
            continue;
        }
        // never select the dedicated follow or scoreboard clients
        if (client->sess.spectatorState == SPECTATOR_SCOREBOARD || client->sess.spectatorClient < 0) {
            continue;
        }

        if (!nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime) {
            nextInLine = client;
        }
    }

    if (!nextInLine) {
        return;
    }

    level.warmupTime = -1;

    // set them to free-for-all team
    SetTeam(&g_entities[nextInLine - level.clients], "f");
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser()
{
    int32 clientNum;

    if (level.numPlayingClients != 2) {
        return;
    }

    clientNum = level.sortedClients[1];

    if (level.clients[clientNum].pers.connected != CON_CONNECTED) {
        return;
    }

    // make them a spectator
    SetTeam(&g_entities[clientNum], "s");
}

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores()
{
    int32 clientNum;

    clientNum = level.sortedClients[0];
    if (level.clients[clientNum].pers.connected == CON_CONNECTED) {
        level.clients[clientNum].sess.wins++;
        ClientUserinfoChanged(clientNum);
    }

    clientNum = level.sortedClients[1];
    if (level.clients[clientNum].pers.connected == CON_CONNECTED) {
        level.clients[clientNum].sess.losses++;
        ClientUserinfoChanged(clientNum);
    }
}

/*
=============
SortRanks

=============
*/
int32 QDECL SortRanks(const void* a, const void* b)
{
    gclient_t *ca, *cb;

    ca = &level.clients[*(int32*) a];
    cb = &level.clients[*(int32*) b];

    // sort special clients last
    if (ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0) {
        return 1;
    }
    if (cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0) {
        return -1;
    }

    // then connecting clients
    if (ca->pers.connected == CON_CONNECTING) {
        return 1;
    }
    if (cb->pers.connected == CON_CONNECTING) {
        return -1;
    }


    // then spectators
    if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
        if (ca->sess.spectatorTime < cb->sess.spectatorTime) {
            return -1;
        }
        if (ca->sess.spectatorTime > cb->sess.spectatorTime) {
            return 1;
        }
        return 0;
    }
    if (ca->sess.sessionTeam == TEAM_SPECTATOR) {
        return 1;
    }
    if (cb->sess.sessionTeam == TEAM_SPECTATOR) {
        return -1;
    }

    // then sort by score
    if (ca->ps.persistant[PERS_SCORE]
        > cb->ps.persistant[PERS_SCORE]) {
        return -1;
    }
    if (ca->ps.persistant[PERS_SCORE]
        < cb->ps.persistant[PERS_SCORE]) {
        return 1;
    }
    return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks()
{
    int32      i;
    int32      rank;
    int32      score;
    int32      newScore;
    gclient_t* cl;

    level.follow1 = -1;
    level.follow2 = -1;
    level.numConnectedClients = 0;
    level.numNonSpectatorClients = 0;
    level.numPlayingClients = 0;
    level.numVotingClients = 0; // don't count bots
    for (i = 0; i < TEAM_NUM_TEAMS; i++) {
        level.numteamVotingClients[i] = 0;
    }
    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].pers.connected != CON_DISCONNECTED) {
            level.sortedClients[level.numConnectedClients] = i;
            level.numConnectedClients++;

            if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR) {
                level.numNonSpectatorClients++;

                // decide if this should be auto-followed
                if (level.clients[i].pers.connected == CON_CONNECTED) {
                    level.numPlayingClients++;
                    if (!(g_entities[i].r.svFlags & SVF_BOT)) {
                        level.numVotingClients++;
                        if (level.clients[i].sess.sessionTeam == TEAM_RED) {
                            level.numteamVotingClients[0]++;
                        } else if (level.clients[i].sess.sessionTeam == TEAM_BLUE) {
                            level.numteamVotingClients[1]++;
                        }
                    }
                    if (level.follow1 == -1) {
                        level.follow1 = i;
                    } else if (level.follow2 == -1) {
                        level.follow2 = i;
                    }
                }
            }
        }
    }

    qsort(level.sortedClients, level.numConnectedClients, sizeof(level.sortedClients[0]), SortRanks);

    // set the rank value for all clients that are connected and not spectators
    if (g_gametype->integer >= GT_TEAM) {
        // in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
        for (i = 0; i < level.numConnectedClients; i++) {
            cl = &level.clients[level.sortedClients[i]];
            if (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]) {
                cl->ps.persistant[PERS_RANK] = 2;
            } else if (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
                cl->ps.persistant[PERS_RANK] = 0;
            } else {
                cl->ps.persistant[PERS_RANK] = 1;
            }
        }
    } else {
        rank = -1;
        score = 0;
        for (i = 0; i < level.numPlayingClients; i++) {
            cl = &level.clients[level.sortedClients[i]];
            newScore = cl->ps.persistant[PERS_SCORE];
            if (i == 0 || newScore != score) {
                rank = i;
                // assume we aren't tied until the next client is checked
                level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank;
            } else {
                // we are tied with the previous client
                level.clients[level.sortedClients[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
                level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
            }
            score = newScore;
            if (g_gametype->integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1) {
                level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
            }
        }
    }

    // set the CS_SCORES1/2 configstrings, which will be visible to everyone
    if (g_gametype->integer >= GT_TEAM) {
        trap_SetConfigstring(CS_SCORES1, va("%i", level.teamScores[TEAM_RED]));
        trap_SetConfigstring(CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]));
    } else {
        if (level.numConnectedClients == 0) {
            trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
            trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
        } else if (level.numConnectedClients == 1) {
            trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
            trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
        } else {
            trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE]));
            trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE]));
        }
    }

    // see if it is time to end the level
    CheckExitRules();

    // if we are at the intermission, send the new info to everyone
    if (level.intermissiontime) {
        SendScoreboardMessageToAllClients();
    }
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients()
{
    int32 i;

    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].pers.connected == CON_CONNECTED) {
            DeathmatchScoreboardMessage(g_entities + i);
        }
    }
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission(gentity_t* ent)
{
    // take out of follow mode if needed
    if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
        StopFollowing(ent);
    }


    // move to the spot
    VectorCopy(level.intermission_origin, ent->s.origin);
    VectorCopy(level.intermission_origin, ent->client->ps.origin);
    VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
    ent->client->ps.pm_type = PM_INTERMISSION;

    // clean up powerup info
    memset(ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups));

    ent->client->ps.eFlags = 0;
    ent->s.eFlags = 0;
    ent->s.eType = ET_GENERAL;
    ent->s.modelindex = 0;
    ent->s.loopSound = 0;
    ent->s.event = 0;
    ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint()
{
    gentity_t *ent, *target;
    vec3_t     dir;

    // find the intermission spot
    ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
    if (!ent) { // the map creator forgot to put in an intermission point...
        SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle);
    } else {
        VectorCopy(ent->s.origin, level.intermission_origin);
        VectorCopy(ent->s.angles, level.intermission_angle);
        // if it has a target, look towards it
        if (ent->target) {
            target = G_PickTarget(ent->target);
            if (target) {
                VectorSubtract(target->s.origin, level.intermission_origin, dir);
                vectoangles(dir, level.intermission_angle);
            }
        }
    }
}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission()
{
    int32      i;
    gentity_t* client;

    if (level.intermissiontime) {
        return; // already active
    }

    // if in tournement mode, change the wins / losses
    if (g_gametype->integer == GT_TOURNAMENT) {
        AdjustTournamentScores();
    }

    level.intermissiontime = level.time;
    FindIntermissionPoint();

    // if single player game
    if (g_gametype->integer == GT_SINGLE_PLAYER) {
        UpdateTournamentInfo();
        SpawnModelsOnVictoryPads();
    }

    // move all clients to the intermission point
    for (i = 0; i < level.maxclients; i++) {
        client = g_entities + i;
        if (!client->inuse) {
            continue;
        }
        // respawn if dead
        if (client->health <= 0) {
            respawn(client);
        }
        MoveClientToIntermission(client);
    }

    // send the current scoring to all clients
    SendScoreboardMessageToAllClients();
}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel()
{
    int32      i;
    gclient_t* cl;

    // bot interbreeding
    BotInterbreedEndMatch();

    // if we are running a tournement map, kick the loser to spectator status,
    // which will automatically grab the next spectator and restart
    if (g_gametype->integer == GT_TOURNAMENT) {
        if (!level.restarted) {
            RemoveTournamentLoser();
            Cbuf_ExecuteText(EXEC_APPEND, "map_restart 0\n");
            level.restarted = true;
            level.changemap = NULL;
            level.intermissiontime = 0;
        }
        return;
    }


    Cbuf_ExecuteText(EXEC_APPEND, "vstr nextmap\n");
    level.changemap = NULL;
    level.intermissiontime = 0;

    // reset all the scores so we don't enter the intermission again
    level.teamScores[TEAM_RED] = 0;
    level.teamScores[TEAM_BLUE] = 0;
    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        cl->ps.persistant[PERS_SCORE] = 0;
    }

    // we need to do this here before chaning to CON_CONNECTING
    G_WriteSessionData();

    // change all client states to connecting, so the early players into the
    // next level will know the others aren't done reconnecting
    for (i = 0; i < g_maxclients->integer; i++) {
        if (level.clients[i].pers.connected == CON_CONNECTED) {
            level.clients[i].pers.connected = CON_CONNECTING;
        }
    }
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit(const char* string)
{
    int32      i, numSorted;
    gclient_t* cl;

    logger.info("Exit: %s", string);

    level.intermissionQueued = level.time;

    // this will keep the clients from playing any voice sounds
    // that will get cut off when the queued intermission starts
    trap_SetConfigstring(CS_INTERMISSION, "1");

    // don't send more than 32 scores (FIXME?)
    numSorted = level.numConnectedClients;
    if (numSorted > 32) {
        numSorted = 32;
    }

    if (g_gametype->integer >= GT_TEAM) {
        logger.info("red:%i  blue:%i\n", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]);
    }

    for (i = 0; i < numSorted; i++) {
        int32 ping;

        cl = &level.clients[level.sortedClients[i]];

        if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
            continue;
        }
        if (cl->pers.connected == CON_CONNECTING) {
            continue;
        }

        ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

        logger.info("score: %i  ping: %i  client: %i %s", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i], cl->pers.netname);
    }
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit()
{
    int32      ready, notReady;
    int32      i;
    gclient_t* cl;
    int32      readyMask;

    if (g_gametype->integer == GT_SINGLE_PLAYER) {
        return;
    }

    // see which players are ready
    ready = 0;
    notReady = 0;
    readyMask = 0;
    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) {
            continue;
        }

        if (cl->readyToExit) {
            ready++;
            if (i < 16) {
                readyMask |= 1 << i;
            }
        } else {
            notReady++;
        }
    }

    // copy the readyMask to each player's stats so
    // it can be displayed on the scoreboard
    for (i = 0; i < g_maxclients->integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
    }

    // never exit in less than five seconds
    if (level.time < level.intermissiontime + 5000) {
        return;
    }

    // if nobody wants to go, clear timer
    if (!ready) {
        level.readyToExit = false;
        return;
    }

    // if everyone wants to go, go now
    if (!notReady) {
        ExitLevel();
        return;
    }

    // the first person to ready starts the ten second timeout
    if (!level.readyToExit) {
        level.readyToExit = true;
        level.exitTime = level.time;
    }

    // if we have waited ten seconds since at least one player
    // wanted to exit, go ahead
    if (level.time < level.exitTime + 10000) {
        return;
    }

    ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
bool ScoreIsTied()
{
    int32 a, b;

    if (level.numPlayingClients < 2) {
        return false;
    }

    if (g_gametype->integer >= GT_TEAM) {
        return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
    }

    a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
    b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

    return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules()
{
    int32      i;
    gclient_t* cl;
    // if at the intermission, wait for all non-bots to
    // signal ready, then go to next level
    if (level.intermissiontime) {
        CheckIntermissionExit();
        return;
    }

    if (level.intermissionQueued) {
        if (level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME) {
            level.intermissionQueued = 0;
            BeginIntermission();
        }
        return;
    }

    // check for sudden death
    if (ScoreIsTied()) {
        // always wait for sudden death
        return;
    }

    if (g_timelimit->integer && !level.warmupTime) {
        if (level.time - level.startTime >= g_timelimit->integer * 60000) {
            trap_SendServerCommand(-1, "print \"Timelimit hit.\n\"");
            LogExit("Timelimit hit.");
            return;
        }
    }

    if (level.numPlayingClients < 2) {
        return;
    }

    if (g_gametype->integer < GT_CTF && g_fraglimit->integer) {
        if (level.teamScores[TEAM_RED] >= g_fraglimit->integer) {
            trap_SendServerCommand(-1, "print \"Red hit the fraglimit.\n\"");
            LogExit("Fraglimit hit.");
            return;
        }

        if (level.teamScores[TEAM_BLUE] >= g_fraglimit->integer) {
            trap_SendServerCommand(-1, "print \"Blue hit the fraglimit.\n\"");
            LogExit("Fraglimit hit.");
            return;
        }

        for (i = 0; i < g_maxclients->integer; i++) {
            cl = level.clients + i;
            if (cl->pers.connected != CON_CONNECTED) {
                continue;
            }
            if (cl->sess.sessionTeam != TEAM_FREE) {
                continue;
            }

            if (cl->ps.persistant[PERS_SCORE] >= g_fraglimit->integer) {
                LogExit("Fraglimit hit.");
                trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"", cl->pers.netname));
                return;
            }
        }
    }

    if (g_gametype->integer >= GT_CTF && g_capturelimit->integer) {
        if (level.teamScores[TEAM_RED] >= g_capturelimit->integer) {
            trap_SendServerCommand(-1, "print \"Red hit the capturelimit.\n\"");
            LogExit("Capturelimit hit.");
            return;
        }

        if (level.teamScores[TEAM_BLUE] >= g_capturelimit->integer) {
            trap_SendServerCommand(-1, "print \"Blue hit the capturelimit.\n\"");
            LogExit("Capturelimit hit.");
            return;
        }
    }
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
static void CheckTournament()
{
    // check because we run 3 game frames before calling Connect and/or ClientBegin
    // for clients on a map_restart
    if (level.numPlayingClients == 0) {
        return;
    }

    if (g_gametype->integer == GT_TOURNAMENT) {
        // pull in a spectator if needed
        if (level.numPlayingClients < 2) {
            AddTournamentPlayer();
        }

        // if we don't have two players, go back to "waiting for players"
        if (level.numPlayingClients != 2) {
            if (level.warmupTime != -1) {
                level.warmupTime = -1;
                trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
                logger.info("Warmup:");
            }
            return;
        }

        if (level.warmupTime == 0) {
            return;
        }

        // if the warmup is changed at the console, restart it
        if (g_warmup->modified) {
            level.warmupTime = -1;
            g_warmup->modified = false;
        }

        // if all players have arrived, start the countdown
        if (level.warmupTime < 0) {
            if (level.numPlayingClients == 2) {
                // fudge by -1 to account for extra delays
                level.warmupTime = level.time + (g_warmup->integer - 1) * 1000;
                trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
            }
            return;
        }

        // if the warmup time has counted down, restart
        if (level.time > level.warmupTime) {
            level.warmupTime += 10000;
            Cvar_Set("g_restarted", "1");
            Cbuf_ExecuteText(EXEC_APPEND, "map_restart 0\n");
            level.restarted = true;
            return;
        }
    } else if (g_gametype->integer != GT_SINGLE_PLAYER && level.warmupTime != 0) {
        int32 counts[TEAM_NUM_TEAMS];
        bool  notEnough = false;

        if (g_gametype->integer > GT_TEAM) {
            counts[TEAM_BLUE] = TeamCount(-1, TEAM_BLUE);
            counts[TEAM_RED] = TeamCount(-1, TEAM_RED);

            if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
                notEnough = true;
            }
        } else if (level.numPlayingClients < 2) {
            notEnough = true;
        }

        if (notEnough) {
            if (level.warmupTime != -1) {
                level.warmupTime = -1;
                trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
                logger.info("Warmup:");
            }
            return; // still waiting for team members
        }

        if (level.warmupTime == 0) {
            return;
        }

        // if the warmup is changed at the console, restart it
        if (g_warmup->modified) {
            level.warmupTime = -1;
            g_warmup->modified = false;
        }

        // if all players have arrived, start the countdown
        if (level.warmupTime < 0) {
            // fudge by -1 to account for extra delays
            level.warmupTime = level.time + (g_warmup->integer - 1) * 1000;
            trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
            return;
        }

        // if the warmup time has counted down, restart
        if (level.time > level.warmupTime) {
            level.warmupTime += 10000;
            Cvar_Set("g_restarted", "1");
            Cbuf_ExecuteText(EXEC_APPEND, "map_restart 0\n");
            level.restarted = true;
            return;
        }
    }
}


/*
==================
CheckVote
==================
*/
static void CheckVote()
{
    if (level.voteExecuteTime && level.voteExecuteTime < level.time) {
        level.voteExecuteTime = 0;
        Cbuf_ExecuteText(EXEC_APPEND, va("%s\n", level.voteString));
    }
    if (!level.voteTime) {
        return;
    }
    if (level.time - level.voteTime >= VOTE_TIME) {
        trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
    } else {
        // ATVI Q3 1.32 Patch #9, WNF
        if (level.voteYes > level.numVotingClients / 2) {
            // execute the command, then remove the vote
            trap_SendServerCommand(-1, "print \"Vote passed.\n\"");
            level.voteExecuteTime = level.time + 3000;
        } else if (level.voteNo >= level.numVotingClients / 2) {
            // same behavior as a timeout
            trap_SendServerCommand(-1, "print \"Vote failed.\n\"");
        } else {
            // still waiting for a majority
            return;
        }
    }
    level.voteTime = 0;
    trap_SetConfigstring(CS_VOTE_TIME, "");
}

/*
==================
PrintTeam
==================
*/
static void PrintTeam(int32 team, char* message)
{
    int32 i;

    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].sess.sessionTeam != team) {
            continue;
        }
        trap_SendServerCommand(i, message);
    }
}

/*
==================
SetLeader
==================
*/
void SetLeader(int32 team, int32 client)
{
    int32 i;

    if (level.clients[client].pers.connected == CON_DISCONNECTED) {
        PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname));
        return;
    }
    if (level.clients[client].sess.sessionTeam != team) {
        PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname));
        return;
    }
    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].sess.sessionTeam != team) {
            continue;
        }
        if (level.clients[i].sess.teamLeader) {
            level.clients[i].sess.teamLeader = false;
            ClientUserinfoChanged(i);
        }
    }
    level.clients[client].sess.teamLeader = true;
    ClientUserinfoChanged(client);
    PrintTeam(team, va("print \"%s is the new team leader\n\"", level.clients[client].pers.netname));
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader(int32 team)
{
    int32 i;

    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].sess.sessionTeam != team) {
            continue;
        }
        if (level.clients[i].sess.teamLeader) {
            break;
        }
    }
    if (i >= level.maxclients) {
        for (i = 0; i < level.maxclients; i++) {
            if (level.clients[i].sess.sessionTeam != team) {
                continue;
            }
            if (!(g_entities[i].r.svFlags & SVF_BOT)) {
                level.clients[i].sess.teamLeader = true;
                break;
            }
        }
        for (i = 0; i < level.maxclients; i++) {
            if (level.clients[i].sess.sessionTeam != team) {
                continue;
            }
            level.clients[i].sess.teamLeader = true;
            break;
        }
    }
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote(int32 team)
{
    int32 cs_offset;

    if (team == TEAM_RED) {
        cs_offset = 0;
    } else if (team == TEAM_BLUE) {
        cs_offset = 1;
    } else {
        return;
    }

    if (!level.teamVoteTime[cs_offset]) {
        return;
    }
    if (level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME) {
        trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
    } else {
        if (level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset] / 2) {
            // execute the command, then remove the vote
            trap_SendServerCommand(-1, "print \"Team vote passed.\n\"");
            //
            if (!Q_strncmp("leader", level.teamVoteString[cs_offset], 6)) {
                // set the team leader
                SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
            } else {
                Cbuf_ExecuteText(EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset]));
            }
        } else if (level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset] / 2) {
            // same behavior as a timeout
            trap_SendServerCommand(-1, "print \"Team vote failed.\n\"");
        } else {
            // still waiting for a majority
            return;
        }
    }
    level.teamVoteTime[cs_offset] = 0;
    trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "");
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t* ent)
{
    float thinktime;

    thinktime = ent->nextthink;
    if (thinktime <= 0) {
        return;
    }
    if (thinktime > level.time) {
        return;
    }

    ent->nextthink = 0;
    if (!ent->think) {
        G_Error("NULL ent->think");
    }
    ent->think(ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame(int32 levelTime)
{
    int32      i;
    gentity_t* ent;
    int32      msec;
    int32      start, end;

    // if we are waiting for the level to restart, do nothing
    if (level.restarted) {
        return;
    }

    level.framenum++;
    level.previousTime = level.time;
    level.time = levelTime;
    msec = level.time - level.previousTime;

    //
    // go through all allocated objects
    //
    start = Sys_Milliseconds();
    ent = &g_entities[0];
    for (i = 0; i < level.num_entities; i++, ent++) {
        if (!ent->inuse) {
            continue;
        }

        // clear events that are too old
        if (level.time - ent->eventTime > EVENT_VALID_MSEC) {
            if (ent->s.event) {
                ent->s.event = 0; // &= EV_EVENT_BITS;
                if (ent->client) {
                    ent->client->ps.externalEvent = 0;
                    // predicted events should never be set to zero
                    // ent->client->ps.events[0] = 0;
                    // ent->client->ps.events[1] = 0;
                }
            }
            if (ent->freeAfterEvent) {
                // tempEntities or dropped items completely go away after their event
                G_FreeEntity(ent);
                continue;
            } else if (ent->unlinkAfterEvent) {
                // items that will respawn will hide themselves after their pickup event
                ent->unlinkAfterEvent = false;
                trap_UnlinkEntity(ent);
            }
        }

        // temporary entities don't think
        if (ent->freeAfterEvent) {
            continue;
        }

        if (!ent->r.linked && ent->neverFree) {
            continue;
        }

        if (ent->s.eType == ET_MISSILE) {
            G_RunMissile(ent);
            continue;
        }

        if (ent->s.eType == ET_ITEM || ent->physicsObject) {
            G_RunItem(ent);
            continue;
        }

        if (ent->s.eType == ET_MOVER) {
            G_RunMover(ent);
            continue;
        }

        if (i < MAX_CLIENTS) {
            G_RunClient(ent);
            continue;
        }

        G_RunThink(ent);
    }
    end = Sys_Milliseconds();

    start = Sys_Milliseconds();
    // perform final fixups on the players
    ent = &g_entities[0];
    for (i = 0; i < level.maxclients; i++, ent++) {
        if (ent->inuse) {
            ClientEndFrame(ent);
        }
    }
    end = Sys_Milliseconds();

    // see if it is time to do a tournement restart
    CheckTournament();

    // see if it is time to end the level
    CheckExitRules();

    // update to team status?
    CheckTeamStatus();

    // cancel vote if timed out
    CheckVote();

    // check team votes
    CheckTeamVote(TEAM_RED);
    CheckTeamVote(TEAM_BLUE);

    if (g_listEntity->integer) {
        for (i = 0; i < MAX_GENTITIES; i++) {
            logger.info("%4i: %s", i, g_entities[i].classname);
        }
        Cvar_Set("g_listEntity", "0");
    }
}
