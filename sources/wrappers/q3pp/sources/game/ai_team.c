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
#include "ai_team.h"
#include "ai_vcmd.h"

#include "match.h"

// ctf task preferences for a client
typedef struct bot_ctftaskpreference_s
{
    char  name[36];
    int32 preference;
} bot_ctftaskpreference_t;

bot_ctftaskpreference_t ctftaskpreferences[MAX_CLIENTS];


/*
==================
BotValidTeamLeader
==================
*/
int32 BotValidTeamLeader(bot_state_t* bs)
{
    if (!strlen(bs->teamleader)) {
        return false;
    }
    if (ClientFromName(bs->teamleader) == -1) {
        return false;
    }
    return true;
}

/*
==================
BotNumTeamMates
==================
*/
int32 BotNumTeamMates(bot_state_t* bs)
{
    int32        i, numplayers;
    char         buf[MAX_INFO_STRING];
    static int32 maxclients;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    numplayers = 0;
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
        if (BotSameTeam(bs, i)) {
            numplayers++;
        }
    }
    return numplayers;
}

/*
==================
BotClientTravelTimeToGoal
==================
*/
int32 BotClientTravelTimeToGoal(int32 client, bot_goal_t* goal)
{
    playerState_t ps;
    int32         areanum;

    BotAI_GetClientState(client, &ps);
    areanum = BotPointAreaNum(ps.origin);
    if (!areanum) {
        return 1;
    }
    return trap_AAS_AreaTravelTimeToGoalArea(areanum, ps.origin, goal->areanum, TFL_DEFAULT);
}

/*
==================
BotSortTeamMatesByBaseTravelTime
==================
*/
int32 BotSortTeamMatesByBaseTravelTime(bot_state_t* bs, int32* teammates, int32 maxteammates)
{
    int32        i, j, k, numteammates, traveltime;
    char         buf[MAX_INFO_STRING];
    static int32 maxclients;
    int32        traveltimes[MAX_CLIENTS];
    bot_goal_t*  goal = NULL;

    if (gametype == GT_CTF || gametype == GT_1FCTF) {
        if (BotTeam(bs) == TEAM_RED) {
            goal = &ctf_redflag;
        } else {
            goal = &ctf_blueflag;
        }
    }

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    numteammates = 0;
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
        if (BotSameTeam(bs, i)) {
            //
            traveltime = BotClientTravelTimeToGoal(i, goal);
            //
            for (j = 0; j < numteammates; j++) {
                if (traveltime < traveltimes[j]) {
                    for (k = numteammates; k > j; k--) {
                        traveltimes[k] = traveltimes[k - 1];
                        teammates[k] = teammates[k - 1];
                    }
                    break;
                }
            }
            traveltimes[j] = traveltime;
            teammates[j] = i;
            numteammates++;
            if (numteammates >= maxteammates) {
                break;
            }
        }
    }
    return numteammates;
}

/*
==================
BotSetTeamMateTaskPreference
==================
*/
void BotSetTeamMateTaskPreference(bot_state_t* bs, int32 teammate, int32 preference)
{
    char teammatename[MAX_NETNAME];

    ctftaskpreferences[teammate].preference = preference;
    ClientName(teammate, teammatename, sizeof(teammatename));
    strcpy(ctftaskpreferences[teammate].name, teammatename);
}

/*
==================
BotGetTeamMateTaskPreference
==================
*/
int32 BotGetTeamMateTaskPreference(bot_state_t* bs, int32 teammate)
{
    char teammatename[MAX_NETNAME];

    if (!ctftaskpreferences[teammate].preference) {
        return 0;
    }
    ClientName(teammate, teammatename, sizeof(teammatename));
    if (Q_stricmp(teammatename, ctftaskpreferences[teammate].name)) {
        return 0;
    }
    return ctftaskpreferences[teammate].preference;
}

/*
==================
BotSortTeamMatesByTaskPreference
==================
*/
int32 BotSortTeamMatesByTaskPreference(bot_state_t* bs, int32* teammates, int32 numteammates)
{
    int32 defenders[MAX_CLIENTS], numdefenders;
    int32 attackers[MAX_CLIENTS], numattackers;
    int32 roamers[MAX_CLIENTS], numroamers;
    int32 i, preference;

    numdefenders = numattackers = numroamers = 0;
    for (i = 0; i < numteammates; i++) {
        preference = BotGetTeamMateTaskPreference(bs, teammates[i]);
        if (preference & TEAMTP_DEFENDER) {
            defenders[numdefenders++] = teammates[i];
        } else if (preference & TEAMTP_ATTACKER) {
            attackers[numattackers++] = teammates[i];
        } else {
            roamers[numroamers++] = teammates[i];
        }
    }
    numteammates = 0;
    // defenders at the front of the list
    memcpy(&teammates[numteammates], defenders, numdefenders * sizeof(int32));
    numteammates += numdefenders;
    // roamers in the middle
    memcpy(&teammates[numteammates], roamers, numroamers * sizeof(int32));
    numteammates += numroamers;
    // attacker in the back of the list
    memcpy(&teammates[numteammates], attackers, numattackers * sizeof(int32));
    numteammates += numattackers;

    return numteammates;
}

/*
==================
BotSayTeamOrders
==================
*/
void BotSayTeamOrderAlways(bot_state_t* bs, int32 toclient)
{
    char teamchat[MAX_MESSAGE_SIZE];
    char buf[MAX_MESSAGE_SIZE];
    char name[MAX_NETNAME];

    // if the bot is talking to itself
    if (bs->client == toclient) {
        // don't show the message just put it in the console message queue
        trap_BotGetChatMessage(bs->cs, buf, sizeof(buf));
        ClientName(bs->client, name, sizeof(name));
        Com_sprintf(teamchat, sizeof(teamchat), EC "(%s" EC ")" EC ": %s", name, buf);
        trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, teamchat);
    } else {
        trap_BotEnterChat(bs->cs, toclient, CHAT_TELL);
    }
}

/*
==================
BotSayTeamOrders
==================
*/
void BotSayTeamOrder(bot_state_t* bs, int32 toclient)
{
    BotSayTeamOrderAlways(bs, toclient);
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_BothFlagsNotAtBase(bot_state_t* bs)
{
    int32 numteammates, defenders, attackers, i, other;
    int32 teammates[MAX_CLIENTS];
    char  name[MAX_NETNAME], carriername[MAX_NETNAME];

    numteammates = BotSortTeamMatesByBaseTravelTime(bs, teammates, sizeof(teammates));
    BotSortTeamMatesByTaskPreference(bs, teammates, numteammates);
    // different orders based on the number of team mates
    switch (bs->numteammates) {
    case 1:
        break;
    case 2: {
        // tell the one not carrying the flag to attack the enemy base
        if (teammates[0] != bs->flagcarrier) {
            other = teammates[0];
        } else {
            other = teammates[1];
        }
        ClientName(other, name, sizeof(name));
        BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
        BotSayTeamOrder(bs, other);
        break;
    }
    case 3: {
        // tell the one closest to the base not carrying the flag to accompany the flag carrier
        if (teammates[0] != bs->flagcarrier) {
            other = teammates[0];
        } else {
            other = teammates[1];
        }
        ClientName(other, name, sizeof(name));
        if (bs->flagcarrier != -1) {
            ClientName(bs->flagcarrier, carriername, sizeof(carriername));
            if (bs->flagcarrier == bs->client) {
                BotAI_BotInitialChat(bs, "cmd_accompanyme", name, NULL);
            } else {
                BotAI_BotInitialChat(bs, "cmd_accompany", name, carriername, NULL);
            }
        } else {
            //
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
        }
        BotSayTeamOrder(bs, other);
        // tell the one furthest from the the base not carrying the flag to get the enemy flag
        if (teammates[2] != bs->flagcarrier) {
            other = teammates[2];
        } else {
            other = teammates[1];
        }
        ClientName(other, name, sizeof(name));
        BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
        BotSayTeamOrder(bs, other);
        break;
    }
    default: {
        defenders = (int32) (float) numteammates * 0.4 + 0.5;
        if (defenders > 4) {
            defenders = 4;
        }
        attackers = (int32) (float) numteammates * 0.5 + 0.5;
        if (attackers > 5) {
            attackers = 5;
        }
        if (bs->flagcarrier != -1) {
            ClientName(bs->flagcarrier, carriername, sizeof(carriername));
            for (i = 0; i < defenders; i++) {
                //
                if (teammates[i] == bs->flagcarrier) {
                    continue;
                }
                //
                ClientName(teammates[i], name, sizeof(name));
                if (bs->flagcarrier == bs->client) {
                    BotAI_BotInitialChat(bs, "cmd_accompanyme", name, NULL);
                } else {
                    BotAI_BotInitialChat(bs, "cmd_accompany", name, carriername, NULL);
                }
                BotSayTeamOrder(bs, teammates[i]);
            }
        } else {
            for (i = 0; i < defenders; i++) {
                //
                if (teammates[i] == bs->flagcarrier) {
                    continue;
                }
                //
                ClientName(teammates[i], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[i]);
            }
        }
        for (i = 0; i < attackers; i++) {
            //
            if (teammates[numteammates - i - 1] == bs->flagcarrier) {
                continue;
            }
            //
            ClientName(teammates[numteammates - i - 1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
        }
        //
        break;
    }
    }
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_FlagNotAtBase(bot_state_t* bs)
{
    int32 numteammates, defenders, attackers, i;
    int32 teammates[MAX_CLIENTS];
    char  name[MAX_NETNAME];

    numteammates = BotSortTeamMatesByBaseTravelTime(bs, teammates, sizeof(teammates));
    BotSortTeamMatesByTaskPreference(bs, teammates, numteammates);
    // passive strategy
    if (!(bs->ctfstrategy & CTFS_AGRESSIVE)) {
        // different orders based on the number of team mates
        switch (bs->numteammates) {
        case 1:
            break;
        case 2: {
            // both will go for the enemy flag
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            //
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            break;
        }
        case 3: {
            // keep one near the base for when the flag is returned
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            // the other two get the flag
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            //
            ClientName(teammates[2], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[2]);
            break;
        }
        default: {
            // keep some people near the base for when the flag is returned
            defenders = (int32) (float) numteammates * 0.3 + 0.5;
            if (defenders > 3) {
                defenders = 3;
            }
            attackers = (int32) (float) numteammates * 0.7 + 0.5;
            if (attackers > 6) {
                attackers = 6;
            }
            for (i = 0; i < defenders; i++) {
                //
                ClientName(teammates[i], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
                BotSayTeamOrder(bs, teammates[i]);
            }
            for (i = 0; i < attackers; i++) {
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
            //
            break;
        }
        }
    } else {
        // different orders based on the number of team mates
        switch (bs->numteammates) {
        case 1:
            break;
        case 2: {
            // both will go for the enemy flag
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            //
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            break;
        }
        case 3: {
            // everyone go for the flag
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            //
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            //
            ClientName(teammates[2], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[2]);
            break;
        }
        default: {
            // keep some people near the base for when the flag is returned
            defenders = (int32) (float) numteammates * 0.2 + 0.5;
            if (defenders > 2) {
                defenders = 2;
            }
            attackers = (int32) (float) numteammates * 0.7 + 0.5;
            if (attackers > 7) {
                attackers = 7;
            }
            for (i = 0; i < defenders; i++) {
                //
                ClientName(teammates[i], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
                BotSayTeamOrder(bs, teammates[i]);
            }
            for (i = 0; i < attackers; i++) {
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
            //
            break;
        }
        }
    }
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_EnemyFlagNotAtBase(bot_state_t* bs)
{
    int32 numteammates, defenders, attackers, i, other;
    int32 teammates[MAX_CLIENTS];
    char  name[MAX_NETNAME], carriername[MAX_NETNAME];

    numteammates = BotSortTeamMatesByBaseTravelTime(bs, teammates, sizeof(teammates));
    BotSortTeamMatesByTaskPreference(bs, teammates, numteammates);
    // different orders based on the number of team mates
    switch (numteammates) {
    case 1:
        break;
    case 2: {
        // tell the one not carrying the flag to defend the base
        if (teammates[0] == bs->flagcarrier) {
            other = teammates[1];
        } else {
            other = teammates[0];
        }
        ClientName(other, name, sizeof(name));
        BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
        BotSayTeamOrder(bs, other);
        break;
    }
    case 3: {
        // tell the one closest to the base not carrying the flag to defend the base
        if (teammates[0] != bs->flagcarrier) {
            other = teammates[0];
        } else {
            other = teammates[1];
        }
        ClientName(other, name, sizeof(name));
        BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
        BotSayTeamOrder(bs, other);
        // tell the other also to defend the base
        if (teammates[2] != bs->flagcarrier) {
            other = teammates[2];
        } else {
            other = teammates[1];
        }
        ClientName(other, name, sizeof(name));
        BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
        BotSayTeamOrder(bs, other);
        break;
    }
    default: {
        // 60% will defend the base
        defenders = (int32) (float) numteammates * 0.6 + 0.5;
        if (defenders > 6) {
            defenders = 6;
        }
        // 30% accompanies the flag carrier
        attackers = (int32) (float) numteammates * 0.3 + 0.5;
        if (attackers > 3) {
            attackers = 3;
        }
        for (i = 0; i < defenders; i++) {
            //
            if (teammates[i] == bs->flagcarrier) {
                continue;
            }
            ClientName(teammates[i], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[i]);
        }
        // if we have a flag carrier
        if (bs->flagcarrier != -1) {
            ClientName(bs->flagcarrier, carriername, sizeof(carriername));
            for (i = 0; i < attackers; i++) {
                //
                if (teammates[numteammates - i - 1] == bs->flagcarrier) {
                    continue;
                }
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                if (bs->flagcarrier == bs->client) {
                    BotAI_BotInitialChat(bs, "cmd_accompanyme", name, NULL);
                } else {
                    BotAI_BotInitialChat(bs, "cmd_accompany", name, carriername, NULL);
                }
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
        } else {
            for (i = 0; i < attackers; i++) {
                //
                if (teammates[numteammates - i - 1] == bs->flagcarrier) {
                    continue;
                }
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
        }
        //
        break;
    }
    }
}


/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders_BothFlagsAtBase(bot_state_t* bs)
{
    int32 numteammates, defenders, attackers, i;
    int32 teammates[MAX_CLIENTS];
    char  name[MAX_NETNAME];

    // sort team mates by travel time to base
    numteammates = BotSortTeamMatesByBaseTravelTime(bs, teammates, sizeof(teammates));
    // sort team mates by CTF preference
    BotSortTeamMatesByTaskPreference(bs, teammates, numteammates);
    // passive strategy
    if (!(bs->ctfstrategy & CTFS_AGRESSIVE)) {
        // different orders based on the number of team mates
        switch (numteammates) {
        case 1:
            break;
        case 2: {
            // the one closest to the base will defend the base
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            // the other will get the flag
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            break;
        }
        case 3: {
            // the one closest to the base will defend the base
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            // the second one closest to the base will defend the base
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            // the other will get the flag
            ClientName(teammates[2], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[2]);
            break;
        }
        default: {
            defenders = (int32) (float) numteammates * 0.5 + 0.5;
            if (defenders > 5) {
                defenders = 5;
            }
            attackers = (int32) (float) numteammates * 0.4 + 0.5;
            if (attackers > 4) {
                attackers = 4;
            }
            for (i = 0; i < defenders; i++) {
                //
                ClientName(teammates[i], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
                BotSayTeamOrder(bs, teammates[i]);
            }
            for (i = 0; i < attackers; i++) {
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
            //
            break;
        }
        }
    } else {
        // different orders based on the number of team mates
        switch (numteammates) {
        case 1:
            break;
        case 2: {
            // the one closest to the base will defend the base
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            // the other will get the flag
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            break;
        }
        case 3: {
            // the one closest to the base will defend the base
            ClientName(teammates[0], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
            BotSayTeamOrder(bs, teammates[0]);
            // the others should go for the enemy flag
            ClientName(teammates[1], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[1]);
            //
            ClientName(teammates[2], name, sizeof(name));
            BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
            BotSayTeamOrder(bs, teammates[2]);
            break;
        }
        default: {
            defenders = (int32) (float) numteammates * 0.4 + 0.5;
            if (defenders > 4) {
                defenders = 4;
            }
            attackers = (int32) (float) numteammates * 0.5 + 0.5;
            if (attackers > 5) {
                attackers = 5;
            }
            for (i = 0; i < defenders; i++) {
                //
                ClientName(teammates[i], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_defendbase", name, NULL);
                BotSayTeamOrder(bs, teammates[i]);
            }
            for (i = 0; i < attackers; i++) {
                //
                ClientName(teammates[numteammates - i - 1], name, sizeof(name));
                BotAI_BotInitialChat(bs, "cmd_getflag", name, NULL);
                BotSayTeamOrder(bs, teammates[numteammates - i - 1]);
            }
            //
            break;
        }
        }
    }
}

/*
==================
BotCTFOrders
==================
*/
void BotCTFOrders(bot_state_t* bs)
{
    int32 flagstatus;

    //
    if (BotTeam(bs) == TEAM_RED) {
        flagstatus = bs->redflagstatus * 2 + bs->blueflagstatus;
    } else {
        flagstatus = bs->blueflagstatus * 2 + bs->redflagstatus;
    }
    //
    switch (flagstatus) {
    case 0:
        BotCTFOrders_BothFlagsAtBase(bs);
        break;
    case 1:
        BotCTFOrders_EnemyFlagNotAtBase(bs);
        break;
    case 2:
        BotCTFOrders_FlagNotAtBase(bs);
        break;
    case 3:
        BotCTFOrders_BothFlagsNotAtBase(bs);
        break;
    }
}


/*
==================
BotCreateGroup
==================
*/
void BotCreateGroup(bot_state_t* bs, int32* teammates, int32 groupsize)
{
    char  name[MAX_NETNAME], leadername[MAX_NETNAME];
    int32 i;

    // the others in the group will follow the teammates[0]
    ClientName(teammates[0], leadername, sizeof(leadername));
    for (i = 1; i < groupsize; i++) {
        ClientName(teammates[i], name, sizeof(name));
        if (teammates[0] == bs->client) {
            BotAI_BotInitialChat(bs, "cmd_accompanyme", name, NULL);
        } else {
            BotAI_BotInitialChat(bs, "cmd_accompany", name, leadername, NULL);
        }
        BotSayTeamOrderAlways(bs, teammates[i]);
    }
}

/*
==================
BotTeamOrders

  FIXME: defend key areas?
==================
*/
void BotTeamOrders(bot_state_t* bs)
{
    int32        teammates[MAX_CLIENTS];
    int32        numteammates, i;
    char         buf[MAX_INFO_STRING];
    static int32 maxclients;

    if (!maxclients) {
        maxclients = Cvar_VariableIntegerValue("sv_maxclients");
    }

    numteammates = 0;
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
        if (BotSameTeam(bs, i)) {
            teammates[numteammates] = i;
            numteammates++;
        }
    }
    //
    switch (numteammates) {
    case 1:
        break;
    case 2: {
        // nothing special
        break;
    }
    case 3: {
        // have one follow another and one free roaming
        BotCreateGroup(bs, teammates, 2);
        break;
    }
    case 4: {
        BotCreateGroup(bs, teammates, 2);     // a group of 2
        BotCreateGroup(bs, &teammates[2], 2); // a group of 2
        break;
    }
    case 5: {
        BotCreateGroup(bs, teammates, 2);     // a group of 2
        BotCreateGroup(bs, &teammates[2], 3); // a group of 3
        break;
    }
    default: {
        if (numteammates <= 10) {
            for (i = 0; i < numteammates / 2; i++) {
                BotCreateGroup(bs, &teammates[i * 2], 2); // groups of 2
            }
        }
        break;
    }
    }
}

/*
==================
FindHumanTeamLeader
==================
*/
int32 FindHumanTeamLeader(bot_state_t* bs)
{
    int32 i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (g_entities[i].inuse) {
            // if this player is not a bot
            if (!(g_entities[i].r.svFlags & SVF_BOT)) {
                // if this player is ok with being the leader
                if (!notleader[i]) {
                    // if this player is on the same team
                    if (BotSameTeam(bs, i)) {
                        ClientName(i, bs->teamleader, sizeof(bs->teamleader));
                        // if not yet ordered to do anything
                        if (!BotSetLastOrderedTask(bs)) {
                            // go on defense by default
                            BotVoiceChat_Defend(bs, i, SAY_TELL);
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/*
==================
BotTeamAI
==================
*/
void BotTeamAI(bot_state_t* bs)
{
    int32 numteammates;
    char  netname[MAX_NETNAME];

    //
    if (gametype < GT_TEAM) {
        return;
    }
    // make sure we've got a valid team leader
    if (!BotValidTeamLeader(bs)) {
        //
        if (!FindHumanTeamLeader(bs)) {
            //
            if (!bs->askteamleader_time && !bs->becometeamleader_time) {
                if (bs->entergame_time + 10 > FloatTime()) {
                    bs->askteamleader_time = FloatTime() + 5 + random() * 10;
                } else {
                    bs->becometeamleader_time = FloatTime() + 5 + random() * 10;
                }
            }
            if (bs->askteamleader_time && bs->askteamleader_time < FloatTime()) {
                // if asked for a team leader and no response
                BotAI_BotInitialChat(bs, "whoisteamleader", NULL);
                trap_BotEnterChat(bs->cs, 0, CHAT_TEAM);
                bs->askteamleader_time = 0;
                bs->becometeamleader_time = FloatTime() + 8 + random() * 10;
            }
            if (bs->becometeamleader_time && bs->becometeamleader_time < FloatTime()) {
                BotAI_BotInitialChat(bs, "iamteamleader", NULL);
                trap_BotEnterChat(bs->cs, 0, CHAT_TEAM);
                ClientName(bs->client, netname, sizeof(netname));
                strncpy(bs->teamleader, netname, sizeof(bs->teamleader));
                bs->teamleader[sizeof(bs->teamleader)] = '\0';
                bs->becometeamleader_time = 0;
            }
            return;
        }
    }
    bs->askteamleader_time = 0;
    bs->becometeamleader_time = 0;

    // return if this bot is NOT the team leader
    ClientName(bs->client, netname, sizeof(netname));
    if (Q_stricmp(netname, bs->teamleader) != 0) {
        return;
    }
    //
    numteammates = BotNumTeamMates(bs);
    // give orders
    switch (gametype) {
    case GT_TEAM: {
        if (bs->numteammates != numteammates || bs->forceorders) {
            bs->teamgiveorders_time = FloatTime();
            bs->numteammates = numteammates;
            bs->forceorders = false;
        }
        // if it's time to give orders
        if (bs->teamgiveorders_time && bs->teamgiveorders_time < FloatTime() - 5) {
            BotTeamOrders(bs);
            // give orders again after 120 seconds
            bs->teamgiveorders_time = FloatTime() + 120;
        }
        break;
    }
    case GT_CTF: {
        // if the number of team mates changed or the flag status changed
        // or someone wants to know what to do
        if (bs->numteammates != numteammates || bs->flagstatuschanged || bs->forceorders) {
            bs->teamgiveorders_time = FloatTime();
            bs->numteammates = numteammates;
            bs->flagstatuschanged = false;
            bs->forceorders = false;
        }
        // if there were no flag captures the last 3 minutes
        if (bs->lastflagcapture_time < FloatTime() - 240) {
            bs->lastflagcapture_time = FloatTime();
            // randomly change the CTF strategy
            if (random() < 0.4) {
                bs->ctfstrategy ^= CTFS_AGRESSIVE;
                bs->teamgiveorders_time = FloatTime();
            }
        }
        // if it's time to give orders
        if (bs->teamgiveorders_time && bs->teamgiveorders_time < FloatTime() - 3) {
            BotCTFOrders(bs);
            //
            bs->teamgiveorders_time = 0;
        }
        break;
    }
    }
}

