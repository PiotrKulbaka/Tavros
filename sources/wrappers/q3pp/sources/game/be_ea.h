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

// ClientCommand elementary actions
void EA_Say(int32 client, char* str);
void EA_SayTeam(int32 client, char* str);
void EA_Command(int32 client, char* command);

void EA_Action(int32 client, int32 action);
void EA_Crouch(int32 client);
void EA_Walk(int32 client);
void EA_MoveUp(int32 client);
void EA_MoveDown(int32 client);
void EA_MoveForward(int32 client);
void EA_MoveBack(int32 client);
void EA_MoveLeft(int32 client);
void EA_MoveRight(int32 client);
void EA_Attack(int32 client);
void EA_Respawn(int32 client);
void EA_Talk(int32 client);
void EA_Gesture(int32 client);
void EA_Use(int32 client);

// regular elementary actions
void EA_SelectWeapon(int32 client, int32 weapon);
void EA_Jump(int32 client);
void EA_DelayedJump(int32 client);
void EA_Move(int32 client, vec3_t dir, float speed);
void EA_View(int32 client, vec3_t viewangles);

// send regular input to the server
void EA_EndRegular(int32 client, float thinktime);
void EA_GetInput(int32 client, float thinktime, bot_input_t* input);
void EA_ResetInput(int32 client);
// setup and shutdown routines
int32 EA_Setup();
void  EA_Shutdown();
