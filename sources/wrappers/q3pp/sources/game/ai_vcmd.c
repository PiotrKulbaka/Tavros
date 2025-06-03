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
//
#include "chars.h" //characteristics
#include "inv.h"   //indexes into the inventory
#include "syn.h"   //synonyms
#include "match.h" //string matching types and vars

/*
==================
BotVoiceChat_Defend
==================
*/
void BotVoiceChat_Defend(bot_state_t* bs, int32 client, int32 mode)
{
    if (gametype == GT_CTF) {
        //
        switch (BotTeam(bs)) {
        case TEAM_RED:
            memcpy(&bs->teamgoal, &ctf_redflag, sizeof(bot_goal_t));
            break;
        case TEAM_BLUE:
            memcpy(&bs->teamgoal, &ctf_blueflag, sizeof(bot_goal_t));
            break;
        default:
            return;
        }
    } else {
        return;
    }
    //
    bs->decisionmaker = client;
    bs->ordered = true;
    bs->order_time = FloatTime();
    // set the time to send a message to the team mates
    bs->teammessage_time = FloatTime() + 2 * random();
    // set the ltg type
    bs->ltgtype = LTG_DEFENDKEYAREA;
    // get the team goal time
    bs->teamgoal_time = FloatTime() + TEAM_DEFENDKEYAREA_TIME;
    // away from defending
    bs->defendaway_time = 0;
    //
    BotSetTeamStatus(bs);
    // remember last ordered task
    BotRememberLastOrderedTask(bs);
#ifdef DEBUG
    BotPrintTeamGoal(bs);
#endif // DEBUG
}
