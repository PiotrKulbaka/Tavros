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
// cl_cgame.c  -- client system interaction with client game

#include "client.h"
#include "../game/botlib.h"

#include "../cgame/cg_public.h"

static tavros::core::logger logger("cl_cgame");

extern botlib_export_t* botlib_export;

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame()
{
    cls.keyCatchers &= ~KEYCATCH_CGAME;
    cls.cgameStarted = false;
    CG_Shutdown();
}

/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame()
{
    const char* info;
    const char* mapname;
    int32       t1, t2;

    t1 = Sys_Milliseconds();

    // put away the console
    Con_Close();

    // find the current mapname
    info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
    mapname = Info_ValueForKey(info, "mapname");
    Com_sprintf(cl.mapname, sizeof(cl.mapname), "maps/%s.bsp", mapname);

    cls.state = CA_LOADING;

    // init for this gamestate
    // use the lastExecutedServerCommand instead of the serverCommandSequence
    // otherwise server commands sent just before a gamestate are dropped
    CG_Init(clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum);

    // we will send a usercmd this frame, which
    // will cause the server to send us the first snapshot
    cls.state = CA_PRIMED;

    t2 = Sys_Milliseconds();

    logger.info("CL_InitCGame: %5.2f seconds", (t2 - t1) / 1000.0);

    // have the renderer touch all its images, so they are present
    // on the card even if the driver does deferred loading
    RE_EndRegistration();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
bool CL_GameCommand()
{
    return CG_ConsoleCommand();
}


/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering()
{
    CG_DrawActiveFrame(cl.serverTime, clc.demoplaying);
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME 500

void CL_AdjustTimeDelta()
{
    int32       resetTime;
    int32       newDelta;
    int32       deltaDelta;
    const char* info = "";

    cl.newSnapshots = false;

    // the delta never drifts when replaying a demo
    if (clc.demoplaying) {
        return;
    }

    // if the current time is WAY off, just correct to the current value
    if (com_sv_running->integer) {
        resetTime = 100;
    } else {
        resetTime = RESET_TIME;
    }

    newDelta = cl.snap.serverTime - cls.realtime;
    deltaDelta = abs(newDelta - cl.serverTimeDelta);

    if (deltaDelta > RESET_TIME) {
        cl.serverTimeDelta = newDelta;
        cl.oldServerTime = cl.snap.serverTime; // FIXME: is this a problem for cgame?
        cl.serverTime = cl.snap.serverTime;
        info = "<RESET> ";
    } else if (deltaDelta > 100) {
        // fast adjust, cut the difference in half
        info = "<FAST> ";
        cl.serverTimeDelta = (cl.serverTimeDelta + newDelta) >> 1;
    } else {
        // slow drift adjust, only move 1 or 2 msec

        // if any of the frames between this and the previous snapshot
        // had to be extrapolated, nudge our sense of time back a little
        // the granularity of +1 / -2 is too high for timescale modified frametimes
        if (com_timescale->value == 0 || com_timescale->value == 1) {
            if (cl.extrapolatedSnapshot) {
                cl.extrapolatedSnapshot = false;
                cl.serverTimeDelta -= 2;
            } else {
                // otherwise, move our sense of time forward to minimize total latency
                cl.serverTimeDelta++;
            }
        }
    }

    if (cl_showTimeDelta->integer) {
        logger.info("{}{} ", info, cl.serverTimeDelta);
    }
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot()
{
    // ignore snapshots that don't have entities
    if (cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE) {
        return;
    }
    cls.state = CA_ACTIVE;

    // set the timedelta so we are exactly on this first frame
    cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
    cl.oldServerTime = cl.snap.serverTime;

    clc.timeDemoBaseTime = cl.snap.serverTime;

    // if this is the first frame of active play,
    // execute the contents of activeAction now
    // this is to allow scripting a timedemo to start right
    // after loading
    if (cl_activeAction->string[0]) {
        Cbuf_AddText(cl_activeAction->string);
        Cvar_Set("activeAction", "");
    }
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime()
{
    // getting a valid frame message ends the connection process
    if (cls.state != CA_ACTIVE) {
        if (cls.state != CA_PRIMED) {
            return;
        }
        if (clc.demoplaying) {
            // we shouldn't get the first snapshot on the same frame
            // as the gamestate, because it causes a bad time skip
            if (!clc.firstDemoFrameSkipped) {
                clc.firstDemoFrameSkipped = true;
                return;
            }
            CL_ReadDemoMessage();
        }
        if (cl.newSnapshots) {
            cl.newSnapshots = false;
            CL_FirstSnapshot();
        }
        if (cls.state != CA_ACTIVE) {
            return;
        }
    }

    // if we have gotten to this point, cl.snap is guaranteed to be valid
    if (!cl.snap.valid) {
        Com_Error(ERR_DROP, "CL_SetCGameTime: !cl.snap.valid");
    }

    // allow pause in single player
    if (sv_paused->integer && cl_paused->integer && com_sv_running->integer) {
        // paused
        return;
    }

    if (cl.snap.serverTime < cl.oldFrameServerTime) {
        Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
    }
    cl.oldFrameServerTime = cl.snap.serverTime;


    // get our current view of time

    if (clc.demoplaying && cl_freezeDemo->integer) {
        // cl_freezeDemo is used to lock a demo in place for single frame advances

    } else {
        // cl_timeNudge is a user adjustable cvar that allows more
        // or less latency to be added in the interest of better
        // smoothness or better responsiveness.
        int32 tn;

        tn = cl_timeNudge->integer;
        if (tn < -30) {
            tn = -30;
        } else if (tn > 30) {
            tn = 30;
        }

        cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

        // guarantee that time will never flow backwards, even if
        // serverTimeDelta made an adjustment or cl_timeNudge was changed
        if (cl.serverTime < cl.oldServerTime) {
            cl.serverTime = cl.oldServerTime;
        }
        cl.oldServerTime = cl.serverTime;

        // note if we are almost past the latest frame (without timeNudge),
        // so we will try and adjust back a bit when the next snapshot arrives
        if (cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5) {
            cl.extrapolatedSnapshot = true;
        }
    }

    // if we have gotten new snapshots, drift serverTimeDelta
    // don't do this every frame, or a period of packet loss would
    // make a huge adjustment
    if (cl.newSnapshots) {
        CL_AdjustTimeDelta();
    }

    if (!clc.demoplaying) {
        return;
    }

    // if we are playing a demo back, we can just keep reading
    // messages from the demo file until the cgame definately
    // has valid snapshots to interpolate between

    // a timedemo will always use a deterministic set of time samples
    // no matter what speed machine it is run on,
    // while a normal demo may have different time samples
    // each time it is played back
    if (cl_timedemo->integer) {
        if (!clc.timeDemoStart) {
            clc.timeDemoStart = Sys_Milliseconds();
        }
        clc.timeDemoFrames++;
        cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
    }

    while (cl.serverTime >= cl.snap.serverTime) {
        // feed another messag, which should change
        // the contents of cl.snap
        CL_ReadDemoMessage();
        if (cls.state != CA_ACTIVE) {
            return; // end of demo
        }
    }
}


