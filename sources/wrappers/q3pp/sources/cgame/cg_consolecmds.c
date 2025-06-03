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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

static tavros::core::logger logger("cg_consolecmds");

void CG_TargetCommand_f()
{
    int32 targetNum;
    char  test[4];

    targetNum = CG_CrosshairPlayer();
    if (!targetNum) {
        return;
    }

    Cmd_ArgvBuffer(1, test, 4);
    Cbuf_AddText(va("gc %i %i", targetNum, atoi(test))); // Send console command
}


/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f()
{
    Cvar_Set("cg_viewsize", va("%i", (int32) (cg_viewsize->integer + 10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f()
{
    Cvar_Set("cg_viewsize", va("%i", (int32) (cg_viewsize->integer - 10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f()
{
    logger.info("(%i %i %i) : %i", (int32) cg.refdef.vieworg[0], (int32) cg.refdef.vieworg[1], (int32) cg.refdef.vieworg[2], (int32) cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f()
{
    if (cg.scoresRequestTime + 2000 < cg.time) {
        // the scores are more than two seconds out of data,
        // so request new ones
        cg.scoresRequestTime = cg.time;
        CL_AddReliableCommand("score");

        // leave the current scores up if they were already
        // displayed, but if this is the first hit, clear them out
        if (!cg.showScores) {
            cg.showScores = true;
            cg.numScores = 0;
        }
    } else {
        // show the cached contents even if they just pressed if it
        // is within two seconds
        cg.showScores = true;
    }
}

static void CG_ScoresUp_f()
{
    if (cg.showScores) {
        cg.showScores = false;
        cg.scoreFadeTime = cg.time;
    }
}

static void CG_TellTarget_f()
{
    int32 clientNum;
    char  command[128];
    char  message[128];

    clientNum = CG_CrosshairPlayer();
    if (clientNum == -1) {
        return;
    }

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "tell %i %s", clientNum, message);
    CL_AddReliableCommand(command);
}

static void CG_TellAttacker_f()
{
    int32 clientNum;
    char  command[128];
    char  message[128];

    clientNum = CG_LastAttacker();
    if (clientNum == -1) {
        return;
    }

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "tell %i %s", clientNum, message);
    CL_AddReliableCommand(command);
}

static void CG_VoiceTellTarget_f()
{
    int32 clientNum;
    char  command[128];
    char  message[128];

    clientNum = CG_CrosshairPlayer();
    if (clientNum == -1) {
        return;
    }

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
    CL_AddReliableCommand(command);
}

static void CG_VoiceTellAttacker_f()
{
    int32 clientNum;
    char  command[128];
    char  message[128];

    clientNum = CG_LastAttacker();
    if (clientNum == -1) {
        return;
    }

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "vtell %i %s", clientNum, message);
    CL_AddReliableCommand(command);
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f()
{
    char var[MAX_TOKEN_CHARS];

    Cvar_VariableStringBuffer("developer", var, sizeof(var));
    if (!atoi(var)) {
        return;
    }
    if (cg_cameraOrbit->value != 0) {
        Cvar_Set("cg_cameraOrbit", "0");
        Cvar_Set("cg_thirdPerson", "0");
    } else {
        Cvar_Set("cg_cameraOrbit", "5");
        Cvar_Set("cg_thirdPerson", "1");
        Cvar_Set("cg_thirdPersonAngle", "0");
        Cvar_Set("cg_thirdPersonRange", "100");
    }
}


typedef struct
{
    const char* cmd;
    void        (*function)();
} consoleCommand_t;

static consoleCommand_t commands[] = {
    {"testgun", CG_TestGun_f},
    {"testmodel", CG_TestModel_f},
    {"nextframe", CG_TestModelNextFrame_f},
    {"prevframe", CG_TestModelPrevFrame_f},
    {"nextskin", CG_TestModelNextSkin_f},
    {"prevskin", CG_TestModelPrevSkin_f},
    {"viewpos", CG_Viewpos_f},
    {"+scores", CG_ScoresDown_f},
    {"-scores", CG_ScoresUp_f},
    {"+zoom", CG_ZoomDown_f},
    {"-zoom", CG_ZoomUp_f},
    {"sizeup", CG_SizeUp_f},
    {"sizedown", CG_SizeDown_f},
    {"weapnext", CG_NextWeapon_f},
    {"weapprev", CG_PrevWeapon_f},
    {"weapon", CG_Weapon_f},
    {"tell_target", CG_TellTarget_f},
    {"tell_attacker", CG_TellAttacker_f},
    {"vtell_target", CG_VoiceTellTarget_f},
    {"vtell_attacker", CG_VoiceTellAttacker_f},
    {"tcmd", CG_TargetCommand_f},
    {"startOrbit", CG_StartOrbit_f},
    {"loaddeferred", CG_LoadDeferredPlayers}
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
bool CG_ConsoleCommand()
{
    const char* cmd;
    int32       i;

    cmd = CG_Argv(0);

    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (!Q_stricmp(cmd, commands[i].cmd)) {
            logger.debug("Execute command: '%s'", cmd);
            commands[i].function();
            return true;
        }
    }

    return false;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands()
{
    int32 i;

    for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        Cmd_AddCommand(commands[i].cmd, NULL);
    }

    //
    // the game server will interpret these commands, which will be automatically
    // forwarded to the server after they are not recognized locally
    //
    Cmd_AddCommand("kill", NULL);
    Cmd_AddCommand("say", NULL);
    Cmd_AddCommand("say_team", NULL);
    Cmd_AddCommand("tell", NULL);
    Cmd_AddCommand("vsay", NULL);
    Cmd_AddCommand("vsay_team", NULL);
    Cmd_AddCommand("vtell", NULL);
    Cmd_AddCommand("vtaunt", NULL);
    Cmd_AddCommand("vosay", NULL);
    Cmd_AddCommand("vosay_team", NULL);
    Cmd_AddCommand("votell", NULL);
    Cmd_AddCommand("give", NULL);
    Cmd_AddCommand("god", NULL);
    Cmd_AddCommand("notarget", NULL);
    Cmd_AddCommand("noclip", NULL);
    Cmd_AddCommand("team", NULL);
    Cmd_AddCommand("follow", NULL);
    Cmd_AddCommand("addbot", NULL);
    Cmd_AddCommand("setviewpos", NULL);
    Cmd_AddCommand("callvote", NULL);
    Cmd_AddCommand("vote", NULL);
    Cmd_AddCommand("callteamvote", NULL);
    Cmd_AddCommand("teamvote", NULL);
    Cmd_AddCommand("stats", NULL);
    Cmd_AddCommand("teamtask", NULL);
    Cmd_AddCommand("loaddefered", NULL); // spelled wrong, but not changing for demo
}
