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

#define MAX_AVOIDGOALS 256
#define MAX_GOALSTACK  8

#define GFL_NONE       0
#define GFL_ITEM       1
#define GFL_ROAM       2
#define GFL_DROPPED    4


// reset the whole goal state, but keep the item weights
void BotResetGoalState(int32 goalstate);
// reset avoid goals
void BotResetAvoidGoals(int32 goalstate);
// remove the goal with the given number from the avoid goals
void BotRemoveFromAvoidGoals(int32 goalstate, int32 number);
// push a goal onto the goal stack
void BotPushGoal(int32 goalstate, bot_goal_t* goal);
// pop a goal from the goal stack
void BotPopGoal(int32 goalstate);
// empty the bot's goal stack
void BotEmptyGoalStack(int32 goalstate);
// dump the avoid goals
void BotDumpAvoidGoals(int32 goalstate);
// dump the goal stack
void BotDumpGoalStack(int32 goalstate);
// get the name name of the goal with the given number
void BotGoalName(int32 number, char* name, int32 size);
// get the top goal from the stack
int32 BotGetTopGoal(int32 goalstate, bot_goal_t* goal);
// get the second goal on the stack
int32 BotGetSecondGoal(int32 goalstate, bot_goal_t* goal);
// choose the best long term goal item for the bot
int32 BotChooseLTGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags);
// choose the best nearby goal item for the bot
// the item may not be further away from the current bot position than maxtime
// also the travel time from the nearby goal towards the long term goal may not
// be larger than the travel time towards the long term goal from the current bot position
int32 BotChooseNBGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags, bot_goal_t* ltg, float maxtime);
// returns true if the bot touches the goal
int32 BotTouchingGoal(vec3_t origin, bot_goal_t* goal);
// returns true if the goal should be visible but isn't
int32 BotItemGoalInVisButNotVisible(int32 viewer, vec3_t eye, vec3_t viewangles, bot_goal_t* goal);
// search for a goal for the given classname, the index can be used
// as a start point for the search when multiple goals are available with that same classname
int32 BotGetLevelItemGoal(int32 index, char* classname, bot_goal_t* goal);
// get the next camp spot in the map
int32 BotGetNextCampSpotGoal(int32 num, bot_goal_t* goal);
// get the map location with the given name
int32 BotGetMapLocationGoal(char* name, bot_goal_t* goal);
// returns the avoid goal time
float BotAvoidGoalTime(int32 goalstate, int32 number);
// set the avoid goal time
void BotSetAvoidGoalTime(int32 goalstate, int32 number, float avoidtime);
// initializes the items in the level
void BotInitLevelItems();
// regularly update dynamic entity items (dropped weapons, flags etc.)
void BotUpdateEntityItems();
// interbreed the goal fuzzy logic
void BotInterbreedGoalFuzzyLogic(int32 parent1, int32 parent2, int32 child);
// save the goal fuzzy logic to disk
void BotSaveGoalFuzzyLogic(int32 goalstate, char* filename);
// mutate the goal fuzzy logic
void BotMutateGoalFuzzyLogic(int32 goalstate, float range);
// loads item weights for the bot
int32 BotLoadItemWeights(int32 goalstate, char* filename);
// frees the item weights of the bot
void BotFreeItemWeights(int32 goalstate);
// returns the handle of a newly allocated goal state
int32 BotAllocGoalState(int32 client);
// free the given goal state
void BotFreeGoalState(int32 handle);
// setup the goal AI
int32 BotSetupGoalAI();
// shut down the goal AI
void BotShutdownGoalAI();
