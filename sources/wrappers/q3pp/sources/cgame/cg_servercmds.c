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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

static tavros::core::logger logger("cg_servercmds");

typedef struct
{
    const char* order;
    int32       taskNum;
} orderTask_t;

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores()
{
    int32 i, powerups;

    cg.numScores = atoi(CG_Argv(1));
    if (cg.numScores > MAX_CLIENTS) {
        cg.numScores = MAX_CLIENTS;
    }

    cg.teamScores[0] = atoi(CG_Argv(2));
    cg.teamScores[1] = atoi(CG_Argv(3));

    memset(cg.scores, 0, sizeof(cg.scores));
    for (i = 0; i < cg.numScores; i++) {
        //
        cg.scores[i].client = atoi(CG_Argv(i * 14 + 4));
        cg.scores[i].score = atoi(CG_Argv(i * 14 + 5));
        cg.scores[i].ping = atoi(CG_Argv(i * 14 + 6));
        cg.scores[i].time = atoi(CG_Argv(i * 14 + 7));
        cg.scores[i].scoreFlags = atoi(CG_Argv(i * 14 + 8));
        powerups = atoi(CG_Argv(i * 14 + 9));
        cg.scores[i].accuracy = atoi(CG_Argv(i * 14 + 10));
        cg.scores[i].impressiveCount = atoi(CG_Argv(i * 14 + 11));
        cg.scores[i].excellentCount = atoi(CG_Argv(i * 14 + 12));
        cg.scores[i].guantletCount = atoi(CG_Argv(i * 14 + 13));
        cg.scores[i].defendCount = atoi(CG_Argv(i * 14 + 14));
        cg.scores[i].assistCount = atoi(CG_Argv(i * 14 + 15));
        cg.scores[i].perfect = atoi(CG_Argv(i * 14 + 16));
        cg.scores[i].captures = atoi(CG_Argv(i * 14 + 17));

        if (cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS) {
            cg.scores[i].client = 0;
        }
        cgs.clientinfo[cg.scores[i].client].score = cg.scores[i].score;
        cgs.clientinfo[cg.scores[i].client].powerups = powerups;

        cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
    }
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo()
{
    int32 i;
    int32 client;

    numSortedTeamPlayers = atoi(CG_Argv(1));

    for (i = 0; i < numSortedTeamPlayers; i++) {
        client = atoi(CG_Argv(i * 6 + 2));

        sortedTeamPlayers[i] = client;

        cgs.clientinfo[client].location = atoi(CG_Argv(i * 6 + 3));
        cgs.clientinfo[client].health = atoi(CG_Argv(i * 6 + 4));
        cgs.clientinfo[client].armor = atoi(CG_Argv(i * 6 + 5));
        cgs.clientinfo[client].curWeapon = atoi(CG_Argv(i * 6 + 6));
        cgs.clientinfo[client].powerups = atoi(CG_Argv(i * 6 + 7));
    }
}


/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo()
{
    const char* info;
    char*       mapname;

    info = CG_ConfigString(CS_SERVERINFO);
    cgs.gametype = (gametype_t) atoi(Info_ValueForKey(info, "g_gametype"));
    Cvar_Set("g_gametype", va("%i", cgs.gametype));
    cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
    cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
    cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));
    cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
    cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
    cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
    mapname = Info_ValueForKey(info, "mapname");
    Com_sprintf(cgs.mapname, sizeof(cgs.mapname), "maps/%s.bsp", mapname);
    Q_strncpyz(cgs.redTeam, Info_ValueForKey(info, "g_redTeam"), sizeof(cgs.redTeam));
    Cvar_Set("g_redTeam", cgs.redTeam);
    Q_strncpyz(cgs.blueTeam, Info_ValueForKey(info, "g_blueTeam"), sizeof(cgs.blueTeam));
    Cvar_Set("g_blueTeam", cgs.blueTeam);
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup()
{
    const char* info;
    int32       warmup;

    info = CG_ConfigString(CS_WARMUP);

    warmup = atoi(info);
    cg.warmupCount = -1;

    if (warmup == 0 && cg.warmup) {
    } else if (warmup > 0 && cg.warmup <= 0) {
        S_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
    }

    cg.warmup = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues()
{
    const char* s;

    cgs.scores1 = atoi(CG_ConfigString(CS_SCORES1));
    cgs.scores2 = atoi(CG_ConfigString(CS_SCORES2));
    cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
    if (cgs.gametype == GT_CTF) {
        s = CG_ConfigString(CS_FLAGSTATUS);
        cgs.redflag = s[0] - '0';
        cgs.blueflag = s[1] - '0';
    }
    cg.warmup = atoi(CG_ConfigString(CS_WARMUP));
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged()
{
    char        originalShader[MAX_QPATH];
    char        newShader[MAX_QPATH];
    char        timeOffset[16];
    const char* o;
    const char *n, *t;

    o = CG_ConfigString(CS_SHADERSTATE);
    while (o && *o) {
        n = strstr(o, "=");
        if (n && *n) {
            strncpy(originalShader, o, n - o);
            originalShader[n - o] = 0;
            n++;
            t = strstr(n, ":");
            if (t && *t) {
                strncpy(newShader, n, t - n);
                newShader[t - n] = 0;
            } else {
                break;
            }
            t++;
            o = strstr(t, "@");
            if (o) {
                strncpy(timeOffset, t, o - t);
                timeOffset[o - t] = 0;
                o++;
                R_RemapShader(originalShader, newShader, timeOffset);
            }
        } else {
            break;
        }
    }
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified()
{
    const char* str;
    int32       num;

    num = atoi(CG_Argv(1));

    // get the gamestate from the client system, which will have the
    // new configstring already integrated
    CL_GetGameState(&cgs.gameState);

    // look up the individual string that was modified
    str = CG_ConfigString(num);

    // do something with it if necessary
    if (num == CS_MUSIC) {
        CG_StartMusic();
    } else if (num == CS_SERVERINFO) {
        CG_ParseServerinfo();
    } else if (num == CS_WARMUP) {
        CG_ParseWarmup();
    } else if (num == CS_SCORES1) {
        cgs.scores1 = atoi(str);
    } else if (num == CS_SCORES2) {
        cgs.scores2 = atoi(str);
    } else if (num == CS_LEVEL_START_TIME) {
        cgs.levelStartTime = atoi(str);
    } else if (num == CS_VOTE_TIME) {
        cgs.voteTime = atoi(str);
        cgs.voteModified = true;
    } else if (num == CS_VOTE_YES) {
        cgs.voteYes = atoi(str);
        cgs.voteModified = true;
    } else if (num == CS_VOTE_NO) {
        cgs.voteNo = atoi(str);
        cgs.voteModified = true;
    } else if (num == CS_VOTE_STRING) {
        Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
    } else if (num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
        cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_TIME] = true;
    } else if (num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
        cgs.teamVoteYes[num - CS_TEAMVOTE_YES] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_YES] = true;
    } else if (num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
        cgs.teamVoteNo[num - CS_TEAMVOTE_NO] = atoi(str);
        cgs.teamVoteModified[num - CS_TEAMVOTE_NO] = true;
    } else if (num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
        Q_strncpyz(cgs.teamVoteString[num - CS_TEAMVOTE_STRING], str, sizeof(cgs.teamVoteString));
    } else if (num == CS_INTERMISSION) {
        cg.intermissionStarted = atoi(str);
    } else if (num >= CS_MODELS && num < CS_MODELS + MAX_MODELS) {
        cgs.gameModels[num - CS_MODELS] = RE_RegisterModel(str);
    } else if (num >= CS_SOUNDS && num < CS_SOUNDS + MAX_MODELS) {
        if (str[0] != '*') { // player specific sounds don't register here
            cgs.gameSounds[num - CS_SOUNDS] = S_RegisterSound(str, false);
        }
    } else if (num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS) {
        CG_NewClientInfo(num - CS_PLAYERS);
        CG_BuildSpectatorString();
    } else if (num == CS_FLAGSTATUS) {
        if (cgs.gametype == GT_CTF) {
            // format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
            cgs.redflag = str[0] - '0';
            cgs.blueflag = str[1] - '0';
        }
    } else if (num == CS_SHADERSTATE) {
        CG_ShaderStateChanged();
    }
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart()
{
    logger.info("CG_MapRestart");

    CG_InitLocalEntities();
    CG_InitMarkPolys();
    CG_ClearParticles();

    // make sure the "3 frags left" warnings play again
    cg.fraglimitWarnings = 0;

    cg.timelimitWarnings = 0;

    cg.intermissionStarted = false;

    cgs.voteTime = 0;

    cg.mapRestart = true;

    CG_StartMusic();

    S_ClearLoopingSounds(true);

    // we really should clear more parts of cg here and stop sounds

    // play the "fight" sound if this is a restart without warmup
    if (cg.warmup == 0 /* && cgs.gametype == GT_TOURNAMENT */) {
        S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
        CG_CenterPrint("FIGHT!", 120, GIANTCHAR_WIDTH * 2);
    }
}

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar(char* text)
{
    int32 i, l;

    l = 0;
    for (i = 0; text[i]; i++) {
        if (text[i] == '\x19') {
            continue;
        }
        text[l++] = text[i];
    }
    text[l] = '\0';
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand()
{
    const char* cmd;
    char        text[MAX_SAY_TEXT];

    cmd = CG_Argv(0);

    if (!cmd[0]) {
        // server claimed the command
        return;
    }

    if (!strcmp(cmd, "cp")) {
        CG_CenterPrint(CG_Argv(1), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
        return;
    }

    if (!strcmp(cmd, "cs")) {
        CG_ConfigStringModified();
        return;
    }

    if (!strcmp(cmd, "print")) {
        logger.info("%s", CG_Argv(1));
        return;
    }

    if (!strcmp(cmd, "chat")) {
        S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
        Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
        CG_RemoveChatEscapeChar(text);
        logger.info("%s", text);
        return;
    }

    if (!strcmp(cmd, "tchat")) {
        S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
        Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
        CG_RemoveChatEscapeChar(text);
        logger.info("%s", text);
        return;
    }
    if (!strcmp(cmd, "vchat")) {
        return;
    }

    if (!strcmp(cmd, "vtchat")) {
        return;
    }

    if (!strcmp(cmd, "vtell")) {
        return;
    }

    if (!strcmp(cmd, "scores")) {
        CG_ParseScores();
        return;
    }

    if (!strcmp(cmd, "tinfo")) {
        CG_ParseTeamInfo();
        return;
    }

    if (!strcmp(cmd, "map_restart")) {
        CG_MapRestart();
        return;
    }

    if (Q_stricmp(cmd, "remapShader") == 0) {
        if (Cmd_Argc() == 4) {
            R_RemapShader(CG_Argv(1), CG_Argv(2), CG_Argv(3));
        }
    }

    // loaddeferred can be both a servercmd and a consolecmd
    if (!strcmp(cmd, "loaddefered")) { // FIXME: spelled wrong, but not changing for demo
        CG_LoadDeferredPlayers();
        return;
    }

    logger.info("Unknown client game command: %s", cmd);
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands(int32 latestSequence)
{
    while (cgs.serverCommandSequence < latestSequence) {
        if (CL_GetServerCommand(++cgs.serverCommandSequence)) {
            CG_ServerCommand();
        }
    }
}
