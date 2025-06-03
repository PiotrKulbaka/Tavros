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

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "../game/botlib.h"
#include "be_interface.h"

#define MAX_USERMOVE           400
#define MAX_COMMANDARGUMENTS   10
#define ACTION_JUMPEDLASTFRAME 128

bot_input_t* botinputs;


void EA_Say(int32 client, char* str)
{
    botimport.BotClientCommand(client, va("say %s", str));
}


void EA_SayTeam(int32 client, char* str)
{
    botimport.BotClientCommand(client, va("say_team %s", str));
}

//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Gesture(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_GESTURE;
}


void EA_Command(int32 client, char* command)
{
    botimport.BotClientCommand(client, command);
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_SelectWeapon(int32 client, int32 weapon)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->weapon = weapon;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Attack(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_ATTACK;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Talk(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_TALK;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Use(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_USE;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Respawn(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_RESPAWN;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Jump(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    if (bi->actionflags & ACTION_JUMPEDLASTFRAME) {
        bi->actionflags &= ~ACTION_JUMP;
    } else {
        bi->actionflags |= ACTION_JUMP;
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_DelayedJump(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    if (bi->actionflags & ACTION_JUMPEDLASTFRAME) {
        bi->actionflags &= ~ACTION_DELAYEDJUMP;
    } else {
        bi->actionflags |= ACTION_DELAYEDJUMP;
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Crouch(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_CROUCH;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Walk(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_WALK;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Action(int32 client, int32 action)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= action;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveUp(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVEUP;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveDown(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVEDOWN;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveForward(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVEFORWARD;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveBack(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVEBACK;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveLeft(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVELEFT;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_MoveRight(int32 client)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    bi->actionflags |= ACTION_MOVERIGHT;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Move(int32 client, vec3_t dir, float speed)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    VectorCopy(dir, bi->dir);
    // cap speed
    if (speed > MAX_USERMOVE) {
        speed = MAX_USERMOVE;
    } else if (speed < -MAX_USERMOVE) {
        speed = -MAX_USERMOVE;
    }
    bi->speed = speed;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_View(int32 client, vec3_t viewangles)
{
    bot_input_t* bi;

    bi = &botinputs[client];

    VectorCopy(viewangles, bi->viewangles);
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_EndRegular(int32 client, float thinktime)
{
    /*
        bot_input_t *bi;
        int32 jumped = false;

        bi = &botinputs[client];

        bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

        bi->thinktime = thinktime;
        botimport.BotInput(client, bi);

        bi->thinktime = 0;
        VectorClear(bi->dir);
        bi->speed = 0;
        jumped = bi->actionflags & ACTION_JUMP;
        bi->actionflags = 0;
        if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
    */
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_GetInput(int32 client, float thinktime, bot_input_t* input)
{
    bot_input_t* bi;
    //    int32 jumped = false;

    bi = &botinputs[client];

    //    bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

    bi->thinktime = thinktime;
    Com_Memcpy(input, bi, sizeof(bot_input_t));

    /*
    bi->thinktime = 0;
    VectorClear(bi->dir);
    bi->speed = 0;
    jumped = bi->actionflags & ACTION_JUMP;
    bi->actionflags = 0;
    if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
    */
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_ResetInput(int32 client)
{
    bot_input_t* bi;
    int32        jumped = false;

    bi = &botinputs[client];
    bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

    bi->thinktime = 0;
    VectorClear(bi->dir);
    bi->speed = 0;
    jumped = bi->actionflags & ACTION_JUMP;
    bi->actionflags = 0;
    if (jumped) {
        bi->actionflags |= ACTION_JUMPEDLASTFRAME;
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 EA_Setup()
{
    // initialize the bot inputs
    botinputs = (bot_input_t*) GetClearedHunkMemory(
        botlibglobals.maxclients * sizeof(bot_input_t)
    );
    return BLERR_NOERROR;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void EA_Shutdown()
{
    FreeMemory(botinputs);
    botinputs = NULL;
}
