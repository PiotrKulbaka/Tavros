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
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"

static tavros::core::logger logger("cg_scoreboard");

#define SCOREBOARD_X         (0)

#define SB_HEADER            86
#define SB_TOP               (SB_HEADER + 32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR         420

#define SB_NORMAL_HEIGHT     40
#define SB_INTER_HEIGHT      16 // interleaved height

#define SB_MAXCLIENTS_NORMAL ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER  ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved


#define SB_LEFT_BOTICON_X    (SCOREBOARD_X + 0)
#define SB_LEFT_HEAD_X       (SCOREBOARD_X + 32)
#define SB_RIGHT_BOTICON_X   (SCOREBOARD_X + 64)
#define SB_RIGHT_HEAD_X      (SCOREBOARD_X + 96)
// Normal
#define SB_BOTICON_X         (SCOREBOARD_X + 32)
#define SB_HEAD_X            (SCOREBOARD_X + 64)

#define SB_SCORELINE_X       112

#define SB_RATING_WIDTH      (6 * BIGCHAR_WIDTH)                       // width 6
#define SB_SCORE_X           (SB_SCORELINE_X + BIGCHAR_WIDTH)          // width 6
#define SB_RATING_X          (SB_SCORELINE_X + 6 * BIGCHAR_WIDTH)      // width 6
#define SB_PING_X            (SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8) // width 5
#define SB_TIME_X            (SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8) // width 5
#define SB_NAME_X            (SB_SCORELINE_X + 22 * BIGCHAR_WIDTH)     // width 15

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//    0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//
//  wins/losses are drawn on bot icon now

static bool localClient; // true if local client has been displayed


/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore(int32 y, score_t* score, tavros::math::vec4 color, float fade, bool largeFormat)
{
    char          string[1024];
    vec3_t        headAngles;
    clientInfo_t* ci;
    int32         iconx, headx;

    if (score->client < 0 || score->client >= cgs.maxclients) {
        logger.info("Bad score->client: %i", score->client);
        return;
    }

    ci = &cgs.clientinfo[score->client];

    iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
    headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

    // draw the handicap or bot skill marker (unless player has flag)
    if (ci->powerups & (1 << PW_NEUTRALFLAG)) {
        if (largeFormat) {
            CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_FREE);
        } else {
            CG_DrawFlagModel(iconx, y, 16, 16, TEAM_FREE);
        }
    } else if (ci->powerups & (1 << PW_REDFLAG)) {
        if (largeFormat) {
            CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_RED);
        } else {
            CG_DrawFlagModel(iconx, y, 16, 16, TEAM_RED);
        }
    } else if (ci->powerups & (1 << PW_BLUEFLAG)) {
        if (largeFormat) {
            CG_DrawFlagModel(iconx, y - (32 - BIGCHAR_HEIGHT) / 2, 32, 32, TEAM_BLUE);
        } else {
            CG_DrawFlagModel(iconx, y, 16, 16, TEAM_BLUE);
        }
    } else {
        if (ci->handicap < 100) {
            Com_sprintf(string, sizeof(string), "%i", ci->handicap);
            if (cgs.gametype == GT_TOURNAMENT) {
                CG_DrawSmallStringColor(iconx, y - SMALLCHAR_HEIGHT / 2, string, color);
            } else {
                CG_DrawSmallStringColor(iconx, y, string, color);
            }
        }

        // draw the wins / losses
        if (cgs.gametype == GT_TOURNAMENT) {
            Com_sprintf(string, sizeof(string), "%i/%i", ci->wins, ci->losses);
            if (ci->handicap < 100 && !ci->botSkill) {
                CG_DrawSmallStringColor(iconx, y + SMALLCHAR_HEIGHT / 2, string, color);
            } else {
                CG_DrawSmallStringColor(iconx, y, string, color);
            }
        }
    }

    // draw the face
    VectorClear(headAngles);
    headAngles[YAW] = 180;
    if (largeFormat) {
        CG_DrawHead(headx, y - (ICON_SIZE - BIGCHAR_HEIGHT) / 2, ICON_SIZE, ICON_SIZE, score->client, headAngles);
    } else {
        CG_DrawHead(headx, y, 16, 16, score->client, headAngles);
    }

    // draw the score line
    if (score->ping == -1) {
        Com_sprintf(string, sizeof(string), " connecting    %s", ci->name);
    } else if (ci->team == TEAM_SPECTATOR) {
        Com_sprintf(string, sizeof(string), " SPECT %3i %4i %s", score->ping, score->time, ci->name);
    } else {
        Com_sprintf(string, sizeof(string), "%5i %4i %4i %s", score->score, score->ping, score->time, ci->name);
    }

    // highlight your position
    if (score->client == cg.snap->ps.clientNum) {
        float hcolor[4];
        int32 rank;

        localClient = true;

        if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
            || cgs.gametype >= GT_TEAM) {
            rank = -1;
        } else {
            rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
        }
        if (rank == 0) {
            hcolor[0] = 0;
            hcolor[1] = 0;
            hcolor[2] = 0.7f;
        } else if (rank == 1) {
            hcolor[0] = 0.7f;
            hcolor[1] = 0;
            hcolor[2] = 0;
        } else if (rank == 2) {
            hcolor[0] = 0.7f;
            hcolor[1] = 0.7f;
            hcolor[2] = 0;
        } else {
            hcolor[0] = 0.7f;
            hcolor[1] = 0.7f;
            hcolor[2] = 0.7f;
        }

        hcolor[3] = fade * 0.7;
        CG_FillRect(SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2), y, 640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT + 1, hcolor);
    }

    CG_DrawBigString(SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade);

    // add the "ready" marker for intermission exiting
    if (cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client)) {
        CG_DrawBigStringColor(iconx, y, "READY", color);
    }
}

/*
=================
CG_TeamScoreboard
=================
*/
static int32 CG_TeamScoreboard(int32 y, team_t team, float fade, int32 maxClients, int32 lineHeight)
{
    int32              i;
    score_t*           score;
    tavros::math::vec4 color(1.0f, 1.0, 1.0f, fade);
    int32              count;
    clientInfo_t*      ci;

    count = 0;
    for (i = 0; i < cg.numScores && count < maxClients; i++) {
        score = &cg.scores[i];
        ci = &cgs.clientinfo[score->client];

        if (team != ci->team) {
            continue;
        }

        CG_DrawClientScore(y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT);

        count++;
    }

    return count;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
bool CG_DrawOldScoreboard()
{
    int32              x, y, w, i, n1, n2;
    float              fade;
    tavros::math::vec4 fadeColor;
    char*              s;
    int32              maxClients;
    int32              lineHeight;
    int32              topBorderSize, bottomBorderSize;

    // don't draw amuthing if the menu or console is up
    if (cg_paused->integer) {
        cg.deferredPlayerLoading = 0;
        return false;
    }

    if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
        cg.deferredPlayerLoading = 0;
        return false;
    }

    // don't draw scoreboard during death while warmup up
    if (cg.warmup && !cg.showScores) {
        return false;
    }

    if (cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
        tavros::math::vec4 white = {1.0, 1.0, 1.0, 1.0};
        fade = 1.0;
        fadeColor = white;
    } else {
        fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

        if (tavros::math::almost_equal(fadeColor, tavros::math::vec4(0.0f))) {
            // next time scoreboard comes up, don't print killer
            cg.deferredPlayerLoading = 0;
            cg.killerName[0] = 0;
            return false;
        }
        fade = fadeColor.r;
    }


    // fragged by ... line
    if (cg.killerName[0]) {
        s = va("Fragged by %s", cg.killerName);
        w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
        x = (SCREEN_WIDTH - w) / 2;
        y = 40;
        CG_DrawBigString(x, y, s, fade);
    }

    // current rank
    if (cgs.gametype < GT_TEAM) {
        if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
            s = va("%s place with %i", CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1), cg.snap->ps.persistant[PERS_SCORE]);
            w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
            x = (SCREEN_WIDTH - w) / 2;
            y = 60;
            CG_DrawBigString(x, y, s, fade);
        }
    } else {
        if (cg.teamScores[0] == cg.teamScores[1]) {
            s = va("Teams are tied at %i", cg.teamScores[0]);
        } else if (cg.teamScores[0] >= cg.teamScores[1]) {
            s = va("Red leads %i to %i", cg.teamScores[0], cg.teamScores[1]);
        } else {
            s = va("Blue leads %i to %i", cg.teamScores[1], cg.teamScores[0]);
        }

        w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
        x = (SCREEN_WIDTH - w) / 2;
        y = 60;
        CG_DrawBigString(x, y, s, fade);
    }

    // scoreboard
    y = SB_HEADER;

    CG_DrawPic(SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore);
    CG_DrawPic(SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing);
    CG_DrawPic(SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime);
    CG_DrawPic(SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName);

    y = SB_TOP;

    // If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
    if (cg.numScores > SB_MAXCLIENTS_NORMAL) {
        maxClients = SB_MAXCLIENTS_INTER;
        lineHeight = SB_INTER_HEIGHT;
        topBorderSize = 8;
        bottomBorderSize = 16;
    } else {
        maxClients = SB_MAXCLIENTS_NORMAL;
        lineHeight = SB_NORMAL_HEIGHT;
        topBorderSize = 16;
        bottomBorderSize = 16;
    }

    localClient = false;

    if (cgs.gametype >= GT_TEAM) {
        //
        // teamplay scoreboard
        //
        y += lineHeight / 2;

        if (cg.teamScores[0] >= cg.teamScores[1]) {
            n1 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
            CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
            y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
            maxClients -= n1;
            n2 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
            CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
            y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
            maxClients -= n2;
        } else {
            n1 = CG_TeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight);
            CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
            y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
            maxClients -= n1;
            n2 = CG_TeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight);
            CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
            y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
            maxClients -= n2;
        }
        n1 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight);
        y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

    } else {
        //
        // free for all scoreboard
        //
        n1 = CG_TeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight);
        y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
        n2 = CG_TeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight);
        y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
    }

    if (!localClient) {
        // draw local client at the bottom
        for (i = 0; i < cg.numScores; i++) {
            if (cg.scores[i].client == cg.snap->ps.clientNum) {
                CG_DrawClientScore(y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT);
                break;
            }
        }
    }

    // load any models that have been deferred
    if (++cg.deferredPlayerLoading > 10) {
        CG_LoadDeferredPlayers();
    }

    return true;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine(float y, const char* string)
{
    float              x;
    tavros::math::vec4 color(1.0f);
    x = 0.5 * (640 - GIANT_WIDTH * CG_DrawStrlen(string));
    CG_DrawStringExt(x, y, string, color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard()
{
    const char*        s;
    tavros::math::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
    int32              min, tens, ones;
    clientInfo_t*      ci;
    int32              y;
    int32              i;

    // request more scores regularly
    if (cg.scoresRequestTime + 2000 < cg.time) {
        cg.scoresRequestTime = cg.time;
        CL_AddReliableCommand("score");
    }

    // draw the dialog background
    CG_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color.data());

    // print the mesage of the day
    s = CG_ConfigString(CS_MOTD);
    if (!s[0]) {
        s = "Scoreboard";
    }

    // print optional title
    CG_CenterGiantLine(8, s);

    // print server time
    ones = cg.time / 1000;
    min = ones / 60;
    ones %= 60;
    tens = ones / 10;
    ones %= 10;
    s = va("%i:%i%i", min, tens, ones);

    CG_CenterGiantLine(64, s);


    // print the two scores

    y = 160;
    if (cgs.gametype >= GT_TEAM) {
        //
        // teamplay scoreboard
        //
        CG_DrawStringExt(8, y, "Red Team", color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
        s = va("%i", cg.teamScores[0]);
        CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);

        y += 64;

        CG_DrawStringExt(8, y, "Blue Team", color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
        s = va("%i", cg.teamScores[1]);
        CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
    } else {
        //
        // free for all scoreboard
        //
        for (i = 0; i < MAX_CLIENTS; i++) {
            ci = &cgs.clientinfo[i];
            if (!ci->infoValid) {
                continue;
            }
            if (ci->team != TEAM_FREE) {
                continue;
            }

            CG_DrawStringExt(8, y, ci->name, color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
            s = va("%i", ci->score);
            CG_DrawStringExt(632 - GIANT_WIDTH * strlen(s), y, s, color, true, true, GIANT_WIDTH, GIANT_HEIGHT, 0);
            y += 64;
        }
    }
}

