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
#include "../server/server.h"

#include "../game/botlib.h"
extern botlib_export_t* botlib_export;

void trap_LocateGameData(gentity_t* gEnts, int32 numGEntities, int32 sizeofGEntity_t, playerState_t* clients, int32 sizeofGClient)
{
    // the game needs to let the server system know where and how big the gentities
    // are, so it can look at them directly without going through an interface
    SV_LocateGameData((sharedEntity_t*) gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient);
}

void trap_DropClient(int32 clientNum, const char* reason)
{
    SV_GameDropClient(clientNum, reason);
}

void trap_SendServerCommand(int32 clientNum, const char* text)
{
    // reliably sends a command string to be interpreted by the given
    // client.  If clientNum is -1, it will be sent to all clients
    SV_GameSendServerCommand(clientNum, text);
}

void trap_SetConfigstring(int32 num, const char* string)
{
    // config strings hold all the index strings, and various other information
    // that is reliably communicated to all clients
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    // All confgstrings are cleared at each level start.
    SV_SetConfigstring(num, string);
}

void trap_GetConfigstring(int32 num, char* buffer, int32 bufferSize)
{
    SV_GetConfigstring(num, buffer, bufferSize);
}

void trap_GetUserinfo(int32 num, char* buffer, int32 bufferSize)
{
    // userinfo strings are maintained by the server system, so they
    // are persistant across level loads, while all other game visible
    // data is completely reset
    SV_GetUserinfo(num, buffer, bufferSize);
}

void trap_SetUserinfo(int32 num, const char* buffer)
{
    SV_SetUserinfo(num, buffer);
}

void trap_GetServerinfo(char* buffer, int32 bufferSize)
{
    // the serverinfo info string has all the cvars visible to server browsers
    SV_GetServerinfo(buffer, bufferSize);
}

void trap_SetBrushModel(gentity_t* ent, const char* name)
{
    // sets mins and maxs based on the brushmodel name
    SV_SetBrushModel((sharedEntity_t*) ent, name);
}

void trap_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int32 passEntityNum, int32 contentmask)
{
    // collision detection against all linked entities
    SV_Trace(results, start, (float*) mins, (float*) maxs, end, passEntityNum, contentmask, /*int32 capsule*/ false);
}

int32 trap_PointContents(const vec3_t point, int32 passEntityNum)
{
    // point contents against all linked entities
    return SV_PointContents(point, passEntityNum);
}


bool trap_InPVS(const vec3_t p1, const vec3_t p2)
{
    return SV_inPVS(p1, p2);
}

void trap_AdjustAreaPortalState(gentity_t* ent, bool open)
{
    SV_AdjustAreaPortalState((sharedEntity_t*) ent, open);
}

void trap_LinkEntity(gentity_t* ent)
{
    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    SV_LinkEntity((sharedEntity_t*) ent);
}

void trap_UnlinkEntity(gentity_t* ent)
{
    // call before removing an interactive entity
    SV_UnlinkEntity((sharedEntity_t*) ent);
}

int32 trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int32* list, int32 maxcount)
{
    // EntitiesInBox will return brush models based on their bounding box,
    // so exact determination must still be done with EntityContact
    return SV_AreaEntities(mins, maxs, list, maxcount);
}

bool trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t* ent)
{
    // perform an exact check against inline brush models of non-square shape
    return SV_EntityContact((float*) mins, (float*) maxs, (const sharedEntity_t*) ent, /*int32 capsule*/ false);
}

int32 trap_BotAllocateClient()
{
    // access for bots to get and free a server client
    return SV_BotAllocateClient();
}

void trap_GetUsercmd(int32 clientNum, usercmd_t* cmd)
{
    SV_GetUsercmd(clientNum, cmd);
}

bool trap_GetEntityToken(char* buffer, int32 bufferSize)
{
    // Retrieves the next string token from the entity spawn text, returning
    // false when all tokens have been parsed.
    // This should only be done at GAME_INIT time.
    const char* s;
    s = COM_Parse(&sv.entityParsePoint);
    Q_strncpyz(buffer, s, bufferSize);
    return sv.entityParsePoint && s[0];
}

// BotLib traps start here
int32 trap_BotLibSetup()
{
    return SV_BotLibSetup();
}

int32 trap_BotLibShutdown()
{
    return SV_BotLibShutdown();
}

int32 trap_BotLibVarSet(const char* var_name, const char* value)
{
    return botlib_export->BotLibVarSet(var_name, value);
}

int32 trap_BotLibStartFrame(float time)
{
    return botlib_export->BotLibStartFrame(time);
}

int32 trap_BotLibLoadMap(const char* mapname)
{
    return botlib_export->BotLibLoadMap(mapname);
}

int32 trap_BotLibUpdateEntity(int32 ent, void /* struct bot_updateentity_s */* bue)
{
    return botlib_export->BotLibUpdateEntity(ent, (bot_entitystate_t*) bue);
}

int32 trap_BotGetSnapshotEntity(int32 clientNum, int32 sequence)
{
    return SV_BotGetSnapshotEntity(clientNum, sequence);
}

int32 trap_BotGetServerCommand(int32 clientNum, char* message, int32 size)
{
    return SV_BotGetConsoleMessage(clientNum, message, size);
}

void trap_BotUserCommand(int32 clientNum, usercmd_t* ucmd)
{
    SV_ClientThink(&svs.clients[clientNum], ucmd);
}

void trap_AAS_EntityInfo(int32 entnum, void /* struct aas_entityinfo_s */* info)
{
    botlib_export->aas.AAS_EntityInfo(entnum, (struct aas_entityinfo_s*) info);
}

int32 trap_AAS_Initialized()
{
    return botlib_export->aas.AAS_Initialized();
}

void trap_AAS_PresenceTypeBoundingBox(int32 presencetype, vec3_t mins, vec3_t maxs)
{
    botlib_export->aas.AAS_PresenceTypeBoundingBox(presencetype, mins, maxs);
}

float trap_AAS_Time()
{
    return botlib_export->aas.AAS_Time();
}

int32 trap_AAS_PointAreaNum(vec3_t point)
{
    return botlib_export->aas.AAS_PointAreaNum(point);
}

int32 trap_AAS_TraceAreas(vec3_t start, vec3_t end, int32* areas, vec3_t* points, int32 maxareas)
{
    return botlib_export->aas.AAS_TraceAreas(start, end, areas, points, maxareas);
}

int32 trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int32* areas, int32 maxareas)
{
    return botlib_export->aas.AAS_BBoxAreas(absmins, absmaxs, areas, maxareas);
}

int32 trap_AAS_AreaInfo(int32 areanum, void /* struct aas_areainfo_s */* info)
{
    return botlib_export->aas.AAS_AreaInfo(areanum, (struct aas_areainfo_s*) info);
}

int32 trap_AAS_PointContents(vec3_t point)
{
    return botlib_export->aas.AAS_PointContents(point);
}

int32 trap_AAS_NextBSPEntity(int32 ent)
{
    return botlib_export->aas.AAS_NextBSPEntity(ent);
}

int32 trap_AAS_ValueForBSPEpairKey(int32 ent, const char* key, char* value, int32 size)
{
    return botlib_export->aas.AAS_ValueForBSPEpairKey(ent, key, value, size);
}

int32 trap_AAS_VectorForBSPEpairKey(int32 ent, const char* key, vec3_t v)
{
    return botlib_export->aas.AAS_VectorForBSPEpairKey(ent, key, v);
}

int32 trap_AAS_FloatForBSPEpairKey(int32 ent, const char* key, float* value)
{
    return botlib_export->aas.AAS_FloatForBSPEpairKey(ent, key, value);
}

int32 trap_AAS_IntForBSPEpairKey(int32 ent, const char* key, int32* value)
{
    return botlib_export->aas.AAS_IntForBSPEpairKey(ent, key, value);
}

int32 trap_AAS_AreaReachability(int32 areanum)
{
    return botlib_export->aas.AAS_AreaReachability(areanum);
}

int32 trap_AAS_AreaTravelTimeToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags)
{
    return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(areanum, origin, goalareanum, travelflags);
}

int32 trap_AAS_EnableRoutingArea(int32 areanum, int32 enable)
{
    return botlib_export->aas.AAS_EnableRoutingArea(areanum, enable);
}

int32 trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/* route, int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32 maxareas, int32 maxtime, int32 stopevent, int32 stopcontents, int32 stoptfl, int32 stopareanum)
{
    return botlib_export->aas.AAS_PredictRoute((struct aas_predictroute_s*) route, areanum, origin, goalareanum, travelflags, maxareas, maxtime, stopevent, stopcontents, stoptfl, stopareanum);
}

int32 trap_AAS_Swimming(vec3_t origin)
{
    return botlib_export->aas.AAS_Swimming(origin);
}

int32 trap_AAS_PredictClientMovement(void /* struct aas_clientmove_s */* move, int32 entnum, vec3_t origin, int32 presencetype, int32 onground, vec3_t velocity, vec3_t cmdmove, int32 cmdframes, int32 maxframes, float frametime, int32 stopevent, int32 stopareanum, int32 visualize)
{
    return botlib_export->aas.AAS_PredictClientMovement((aas_clientmove_s*) move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize);
}

void trap_EA_SayTeam(int32 client, const char* str)
{
    botlib_export->ea.EA_SayTeam(client, (char*) str);
}

void trap_EA_Command(int32 client, const char* command)
{
    botlib_export->ea.EA_Command(client, (char*) command);
}

void trap_EA_Action(int32 client, int32 action)
{
    botlib_export->ea.EA_Action(client, action);
}

void trap_EA_Gesture(int32 client)
{
    botlib_export->ea.EA_Gesture(client);
}

void trap_EA_Talk(int32 client)
{
    botlib_export->ea.EA_Talk(client);
}

void trap_EA_Attack(int32 client)
{
    botlib_export->ea.EA_Attack(client);
}

void trap_EA_Use(int32 client)
{
    botlib_export->ea.EA_Use(client);
}

void trap_EA_Respawn(int32 client)
{
    botlib_export->ea.EA_Respawn(client);
}

void trap_EA_Crouch(int32 client)
{
    botlib_export->ea.EA_Crouch(client);
}

void trap_EA_SelectWeapon(int32 client, int32 weapon)
{
    botlib_export->ea.EA_SelectWeapon(client, weapon);
}

void trap_EA_View(int32 client, vec3_t viewangles)
{
    botlib_export->ea.EA_View(client, viewangles);
}

void trap_EA_GetInput(int32 client, float thinktime, void /* struct bot_input_s */* input)
{
    botlib_export->ea.EA_GetInput(client, thinktime, (bot_input_s*) input);
}

void trap_EA_ResetInput(int32 client)
{
    botlib_export->ea.EA_ResetInput(client);
}

int32 trap_BotLoadCharacter(char* charfile, float skill)
{
    return botlib_export->ai.BotLoadCharacter(charfile, skill);
}

void trap_BotFreeCharacter(int32 character)
{
    botlib_export->ai.BotFreeCharacter(character);
}

float trap_Characteristic_BFloat(int32 character, int32 index, float min, float max)
{
    return botlib_export->ai.Characteristic_BFloat(character, index, min, max);
}

int32 trap_Characteristic_BInteger(int32 character, int32 index, int32 min, int32 max)
{
    return botlib_export->ai.Characteristic_BInteger(character, index, min, max);
}

void trap_Characteristic_String(int32 character, int32 index, char* buf, int32 size)
{
    botlib_export->ai.Characteristic_String(character, index, buf, size);
}

int32 trap_BotAllocChatState()
{
    return botlib_export->ai.BotAllocChatState();
}

void trap_BotFreeChatState(int32 handle)
{
    botlib_export->ai.BotFreeChatState(handle);
}

void trap_BotQueueConsoleMessage(int32 chatstate, int32 type, char* message)
{
    botlib_export->ai.BotQueueConsoleMessage(chatstate, type, message);
}

void trap_BotRemoveConsoleMessage(int32 chatstate, int32 handle)
{
    botlib_export->ai.BotRemoveConsoleMessage(chatstate, handle);
}

int32 trap_BotNextConsoleMessage(int32 chatstate, void /* struct bot_consolemessage_s */* cm)
{
    return botlib_export->ai.BotNextConsoleMessage(chatstate, (struct bot_consolemessage_s*) cm);
}

int32 trap_BotNumConsoleMessages(int32 chatstate)
{
    return botlib_export->ai.BotNumConsoleMessages(chatstate);
}

void trap_BotInitialChat(int32 chatstate, const char* type, int32 mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
    botlib_export->ai.BotInitialChat(chatstate, (char*) type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}

int32 trap_BotNumInitialChats(int32 chatstate, const char* type)
{
    return botlib_export->ai.BotNumInitialChats(chatstate, (char*) type);
}

int32 trap_BotReplyChat(int32 chatstate, char* message, int32 mcontext, int32 vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
    return botlib_export->ai.BotReplyChat(chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7);
}

void trap_BotEnterChat(int32 chatstate, int32 client, int32 sendto)
{
    botlib_export->ai.BotEnterChat(chatstate, client, sendto);
}

void trap_BotGetChatMessage(int32 chatstate, char* buf, int32 size)
{
    botlib_export->ai.BotGetChatMessage(chatstate, buf, size);
}

int32 trap_BotFindMatch(char* str, void /* struct bot_match_s */* match, uint32 context)
{
    return botlib_export->ai.BotFindMatch(str, (bot_match_s*) match, context);
}

void trap_BotMatchVariable(void /* struct bot_match_s */* match, int32 variable, char* buf, int32 size)
{
    botlib_export->ai.BotMatchVariable((struct bot_match_s*) match, variable, buf, size);
}

void trap_UnifyWhiteSpaces(char* string)
{
    botlib_export->ai.UnifyWhiteSpaces(string);
}

void trap_BotReplaceSynonyms(char* string, uint32 context)
{
    botlib_export->ai.BotReplaceSynonyms(string, context);
}

int32 trap_BotLoadChatFile(int32 chatstate, char* chatfile, char* chatname)
{
    return botlib_export->ai.BotLoadChatFile(chatstate, chatfile, chatname);
}

void trap_BotSetChatGender(int32 chatstate, int32 gender)
{
    botlib_export->ai.BotSetChatGender(chatstate, gender);
}

void trap_BotSetChatName(int32 chatstate, char* name, int32 client)
{
    botlib_export->ai.BotSetChatName(chatstate, name, client);
}

void trap_BotResetGoalState(int32 goalstate)
{
    botlib_export->ai.BotResetGoalState(goalstate);
}

void trap_BotResetAvoidGoals(int32 goalstate)
{
    botlib_export->ai.BotResetAvoidGoals(goalstate);
}

void trap_BotRemoveFromAvoidGoals(int32 goalstate, int32 number)
{
    botlib_export->ai.BotRemoveFromAvoidGoals(goalstate, number);
}

void trap_BotPushGoal(int32 goalstate, void /* struct bot_goal_s */* goal)
{
    botlib_export->ai.BotPushGoal(goalstate, (struct bot_goal_s*) goal);
}

void trap_BotPopGoal(int32 goalstate)
{
    botlib_export->ai.BotPopGoal(goalstate);
}

void trap_BotEmptyGoalStack(int32 goalstate)
{
    botlib_export->ai.BotEmptyGoalStack(goalstate);
}

void trap_BotDumpAvoidGoals(int32 goalstate)
{
    botlib_export->ai.BotDumpAvoidGoals(goalstate);
}

void trap_BotDumpGoalStack(int32 goalstate)
{
    botlib_export->ai.BotDumpGoalStack(goalstate);
}

void trap_BotGoalName(int32 number, char* name, int32 size)
{
    botlib_export->ai.BotGoalName(number, name, size);
}

int32 trap_BotGetTopGoal(int32 goalstate, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotGetTopGoal(goalstate, (bot_goal_s*) goal);
}

int32 trap_BotGetSecondGoal(int32 goalstate, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotGetSecondGoal(goalstate, (struct bot_goal_s*) goal);
}

int32 trap_BotChooseLTGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags)
{
    return botlib_export->ai.BotChooseLTGItem(goalstate, origin, inventory, travelflags);
}

int32 trap_BotChooseNBGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags, void /* struct bot_goal_s */* ltg, float maxtime)
{
    return botlib_export->ai.BotChooseNBGItem(goalstate, origin, inventory, travelflags, (bot_goal_s*) ltg, maxtime);
}

int32 trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotTouchingGoal(origin, (struct bot_goal_s*) goal);
}

int32 trap_BotItemGoalInVisButNotVisible(int32 viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotItemGoalInVisButNotVisible(viewer, eye, viewangles, (bot_goal_s*) goal);
}

int32 trap_BotGetLevelItemGoal(int32 index, const char* classname, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotGetLevelItemGoal(index, (char*) classname, (bot_goal_s*) goal);
}

int32 trap_BotGetNextCampSpotGoal(int32 num, void /* struct bot_goal_s */* goal)
{
    return botlib_export->ai.BotGetNextCampSpotGoal(num, (struct bot_goal_s*) goal);
}

void trap_BotSetAvoidGoalTime(int32 goalstate, int32 number, float avoidtime)
{
    botlib_export->ai.BotSetAvoidGoalTime(goalstate, number, avoidtime);
}

void trap_BotUpdateEntityItems()
{
    botlib_export->ai.BotUpdateEntityItems();
}

int32 trap_BotLoadItemWeights(int32 goalstate, char* filename)
{
    return botlib_export->ai.BotLoadItemWeights(goalstate, filename);
}

void trap_BotInterbreedGoalFuzzyLogic(int32 parent1, int32 parent2, int32 child)
{
    botlib_export->ai.BotInterbreedGoalFuzzyLogic(parent1, parent2, child);
}

void trap_BotSaveGoalFuzzyLogic(int32 goalstate, char* filename)
{
    botlib_export->ai.BotSaveGoalFuzzyLogic(goalstate, filename);
}

void trap_BotMutateGoalFuzzyLogic(int32 goalstate, float range)
{
    botlib_export->ai.BotMutateGoalFuzzyLogic(goalstate, range);
}

int32 trap_BotAllocGoalState(int32 state)
{
    return botlib_export->ai.BotAllocGoalState(state);
}

void trap_BotFreeGoalState(int32 handle)
{
    botlib_export->ai.BotFreeGoalState(handle);
}

void trap_BotResetMoveState(int32 movestate)
{
    botlib_export->ai.BotResetMoveState(movestate);
}

void trap_BotAddAvoidSpot(int32 movestate, vec3_t origin, float radius, int32 type)
{
    botlib_export->ai.BotAddAvoidSpot(movestate, origin, radius, type);
}

void trap_BotMoveToGoal(void /* struct bot_moveresult_s */* result, int32 movestate, void /* struct bot_goal_s */* goal, int32 travelflags)
{
    botlib_export->ai.BotMoveToGoal((bot_moveresult_s*) result, movestate, (bot_goal_s*) goal, travelflags);
}

int32 trap_BotMoveInDirection(int32 movestate, vec3_t dir, float speed, int32 type)
{
    return botlib_export->ai.BotMoveInDirection(movestate, dir, speed, type);
}

void trap_BotResetAvoidReach(int32 movestate)
{
    botlib_export->ai.BotResetAvoidReach(movestate);
}

void trap_BotResetLastAvoidReach(int32 movestate)
{
    botlib_export->ai.BotResetLastAvoidReach(movestate);
}

int32 trap_BotMovementViewTarget(int32 movestate, void /* struct bot_goal_s */* goal, int32 travelflags, float lookahead, vec3_t target)
{
    return botlib_export->ai.BotMovementViewTarget(movestate, (bot_goal_s*) goal, travelflags, lookahead, target);
}

int32 trap_BotPredictVisiblePosition(vec3_t origin, int32 areanum, void /* struct bot_goal_s */* goal, int32 travelflags, vec3_t target)
{
    return botlib_export->ai.BotPredictVisiblePosition(origin, areanum, (bot_goal_s*) goal, travelflags, target);
}

int32 trap_BotAllocMoveState()
{
    return botlib_export->ai.BotAllocMoveState();
}

void trap_BotFreeMoveState(int32 handle)
{
    botlib_export->ai.BotFreeMoveState(handle);
}

void trap_BotInitMoveState(int32 handle, void /* struct bot_initmove_s */* initmove)
{
    botlib_export->ai.BotInitMoveState(handle, (bot_initmove_s*) initmove);
}

int32 trap_BotChooseBestFightWeapon(int32 weaponstate, int32* inventory)
{
    return botlib_export->ai.BotChooseBestFightWeapon(weaponstate, inventory);
}

void trap_BotGetWeaponInfo(int32 weaponstate, int32 weapon, void /* struct weaponinfo_s */* weaponinfo)
{
    botlib_export->ai.BotGetWeaponInfo(weaponstate, weapon, (struct weaponinfo_s*) weaponinfo);
}

int32 trap_BotLoadWeaponWeights(int32 weaponstate, char* filename)
{
    return botlib_export->ai.BotLoadWeaponWeights(weaponstate, filename);
}

int32 trap_BotAllocWeaponState()
{
    return botlib_export->ai.BotAllocWeaponState();
}

void trap_BotFreeWeaponState(int32 weaponstate)
{
    botlib_export->ai.BotFreeWeaponState(weaponstate);
}

void trap_BotResetWeaponState(int32 weaponstate)
{
    botlib_export->ai.BotResetWeaponState(weaponstate);
}

int32 trap_GeneticParentsAndChildSelection(int32 numranks, float* ranks, int32* parent1, int32* parent2, int32* child)
{
    return botlib_export->ai.GeneticParentsAndChildSelection(numranks, ranks, parent1, parent2, child);
}
