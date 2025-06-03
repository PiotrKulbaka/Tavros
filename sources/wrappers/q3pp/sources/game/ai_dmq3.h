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
#pragma once

#include "ai_main.h"
#include <game/q_shared.h>
#include "be_ai_move.h"

// setup the deathmatch AI
void BotSetupDeathmatchAI();
// let the bot live within it's deathmatch AI net
void BotDeathmatchAI(bot_state_t* bs, float thinktime);
// free waypoints
void BotFreeWaypoints(bot_waypoint_t* wp);
// choose a weapon
void BotChooseWeapon(bot_state_t* bs);
// setup movement stuff
void BotSetupForMovement(bot_state_t* bs);
// update the inventory
void BotUpdateInventory(bot_state_t* bs);
// update the inventory during battle
void BotUpdateBattleInventory(bot_state_t* bs, int32 enemy);
// use holdable items during battle
void BotBattleUseItems(bot_state_t* bs);
// return true if the bot is dead
bool BotIsDead(bot_state_t* bs);
// returns true if the bot is in observer mode
bool BotIsObserver(bot_state_t* bs);
// returns true if the bot is in the intermission
bool BotIntermission(bot_state_t* bs);
// returns true if the bot is in lava or slime
bool BotInLavaOrSlime(bot_state_t* bs);
// returns true if the entity is dead
bool EntityIsDead(aas_entityinfo_t* entinfo);
// returns true if the entity is invisible
bool EntityIsInvisible(aas_entityinfo_t* entinfo);
// returns true if the entity is shooting
bool EntityIsShooting(aas_entityinfo_t* entinfo);
// set a user info key/value pair
void BotSetUserInfo(bot_state_t* bs, const char* key, const char* value);
// set the team status (offense, defense etc.)
void BotSetTeamStatus(bot_state_t* bs);
// returns the name of the client
const char* ClientName(int32 client, char* name, int32 size);
// returns an simplyfied client name
char* EasyClientName(int32 client, char* name, int32 size);
// returns the appropriate synonym context for the current game type and situation
int32 BotSynonymContext(bot_state_t* bs);
// set last ordered task
int32 BotSetLastOrderedTask(bot_state_t* bs);
// selection of goals for teamplay
void BotTeamGoals(bot_state_t* bs, int32 retreat);
// returns the aggression of the bot in the range [0, 100]
float BotAggression(bot_state_t* bs);
// returns true if the bot wants to retreat
int32 BotWantsToRetreat(bot_state_t* bs);
// returns true if the bot wants to chase
int32 BotWantsToChase(bot_state_t* bs);
// returns true if the bot can and wants to rocketjump
int32 BotCanAndWantsToRocketJump(bot_state_t* bs);
// returns true if the bot has a persistant powerup and a weapon
int32 BotHasPersistantPowerupAndWeapon(bot_state_t* bs);
// returns true if the bot wants to and goes camping
int32 BotWantsToCamp(bot_state_t* bs);
// the bot will perform attack movements
bot_moveresult_t BotAttackMove(bot_state_t* bs, int32 tfl);
// returns true if the bot and the entity are in the same team
int32 BotSameTeam(bot_state_t* bs, int32 entnum);
// returns true if teamplay is on
int32 TeamPlayIsOn();
// returns visible team mate flag carrier if available
int32 BotTeamFlagCarrierVisible(bot_state_t* bs);
// returns visible enemy flag carrier if available
int32 BotEnemyFlagCarrierVisible(bot_state_t* bs);
// returns true if within the field of vision for the given angles
bool InFieldOfVision(vec3_t viewangles, float fov, vec3_t angles);
// returns true and sets the .enemy field when an enemy is found
int32 BotFindEnemy(bot_state_t* bs, int32 curenemy);
// returns a roam goal
void BotRoamGoal(bot_state_t* bs, vec3_t goal);
// returns entity visibility in the range [0, 1]
float BotEntityVisible(int32 viewer, vec3_t eye, vec3_t viewangles, float fov, int32 ent);
// the bot will aim at the current enemy
void BotAimAtEnemy(bot_state_t* bs);
// check if the bot should attack
void BotCheckAttack(bot_state_t* bs);
// AI when the bot is blocked
void BotAIBlocked(bot_state_t* bs, bot_moveresult_t* moveresult, int32 activate);
// AI to predict obstacles
int32 BotAIPredictObstacles(bot_state_t* bs, bot_goal_t* goal);
// enable or disable the areas the blocking entity is in
void BotEnableActivateGoalAreas(bot_activategoal_t* activategoal, int32 enable);
// pop an activate goal from the stack
int32 BotPopFromActivateGoalStack(bot_state_t* bs);
// clear the activate goal stack
void BotClearActivateGoalStack(bot_state_t* bs);
// returns the team the bot is in
int32 BotTeam(bot_state_t* bs);
// retuns the opposite team of the bot
int32 BotOppositeTeam(bot_state_t* bs);
// returns the flag the bot is carrying (CTFFLAG_?)
int32 BotCTFCarryingFlag(bot_state_t* bs);
// remember the last ordered task
void BotRememberLastOrderedTask(bot_state_t* bs);
// set ctf goals (defend base, get enemy flag) during seek
void BotCTFSeekGoals(bot_state_t* bs);
// set ctf goals (defend base, get enemy flag) during retreat
void BotCTFRetreatGoals(bot_state_t* bs);
//
// get a random alternate route goal towards the given base
int32 BotGetAlternateRouteGoal(bot_state_t* bs, int32 base);
// returns either the alternate route goal or the given goal
bot_goal_t* BotAlternateRoute(bot_state_t* bs, bot_goal_t* goal);
// create a new waypoint
bot_waypoint_t* BotCreateWayPoint(char* name, vec3_t origin, int32 areanum);
// find a waypoint with the given name
bot_waypoint_t* BotFindWayPoint(bot_waypoint_t* waypoints, char* name);
// strstr but case insensitive
char* stristr(char* str, char* charset);
// returns the number of the client with the given name
int32 ClientFromName(char* name);
int32 ClientOnSameTeamFromName(bot_state_t* bs, char* name);
//
int32 BotPointAreaNum(vec3_t origin);
//
void BotMapScripts(bot_state_t* bs);

// ctf flags
#define CTF_FLAG_NONE     0
#define CTF_FLAG_RED      1
#define CTF_FLAG_BLUE     2
// CTF skins
#define CTF_SKIN_REDTEAM  "red"
#define CTF_SKIN_BLUETEAM "blue"

extern int32 gametype;   // game type
extern int32 maxclients; // maximum number of clients

extern cvar_t* bot_grapple;
extern cvar_t* bot_rocketjump;
extern cvar_t* bot_fastchat;
extern cvar_t* bot_nochat;
extern cvar_t* bot_testrchat;
extern cvar_t* bot_challenge;

extern bot_goal_t ctf_redflag;
extern bot_goal_t ctf_blueflag;
