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
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static tavros::core::logger logger("cg_predict");

static pmove_t cg_pmove;

static int32      cg_numSolidEntities;
static centity_t* cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static int32      cg_numTriggerEntities;
static centity_t* cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList()
{
    int32          i;
    centity_t*     cent;
    snapshot_t*    snap;
    entityState_t* ent;

    cg_numSolidEntities = 0;
    cg_numTriggerEntities = 0;

    if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport) {
        snap = cg.nextSnap;
    } else {
        snap = cg.snap;
    }

    for (i = 0; i < snap->numEntities; i++) {
        cent = &cg_entities[snap->entities[i].number];
        ent = &cent->currentState;

        if (ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER) {
            cg_triggerEntities[cg_numTriggerEntities] = cent;
            cg_numTriggerEntities++;
            continue;
        }

        if (cent->nextState.solid) {
            cg_solidEntities[cg_numSolidEntities] = cent;
            cg_numSolidEntities++;
            continue;
        }
    }
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int32 skipNumber, int32 mask, trace_t* tr)
{
    int32          i, x, zd, zu;
    trace_t        trace;
    entityState_t* ent;
    clipHandle_t   cmodel;
    vec3_t         bmins, bmaxs;
    vec3_t         origin, angles;
    centity_t*     cent;

    for (i = 0; i < cg_numSolidEntities; i++) {
        cent = cg_solidEntities[i];
        ent = &cent->currentState;

        if (ent->number == skipNumber) {
            continue;
        }

        if (ent->solid == SOLID_BMODEL) {
            // special value for bmodel
            cmodel = CM_InlineModel(ent->modelindex);
            VectorCopy(cent->lerpAngles, angles);
            BG_EvaluateTrajectory(&cent->currentState.pos, cg.physicsTime, origin);
        } else {
            // encoded bbox
            x = (ent->solid & 255);
            zd = ((ent->solid >> 8) & 255);
            zu = ((ent->solid >> 16) & 255) - 32;

            bmins[0] = bmins[1] = -x;
            bmaxs[0] = bmaxs[1] = x;
            bmins[2] = -zd;
            bmaxs[2] = zu;

            cmodel = CM_TempBoxModel(bmins, bmaxs, false);
            VectorCopy(vec3_origin, angles);
            VectorCopy(cent->lerpOrigin, origin);
        }


        CM_TransformedBoxTrace(&trace, start, end, (float*) mins, (float*) maxs, cmodel, mask, origin, angles, false);

        if (trace.allsolid || trace.fraction < tr->fraction) {
            trace.entityNum = ent->number;
            *tr = trace;
        } else if (trace.startsolid) {
            tr->startsolid = true;
        }
        if (tr->allsolid) {
            return;
        }
    }
}

/*
================
CG_Trace
================
*/
void CG_Trace(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int32 skipNumber, int32 mask)
{
    trace_t t;

    CM_BoxTrace(&t, start, end, (float*) mins, (float*) maxs, 0, mask, false);
    t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
    // check all other solid models
    CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, &t);

    *result = t;
}

/*
================
CG_PointContents
================
*/
int32 CG_PointContents(const vec3_t point, int32 passEntityNum)
{
    int32          i;
    entityState_t* ent;
    centity_t*     cent;
    clipHandle_t   cmodel;
    int32          contents;

    contents = CM_PointContents(point, 0);

    for (i = 0; i < cg_numSolidEntities; i++) {
        cent = cg_solidEntities[i];

        ent = &cent->currentState;

        if (ent->number == passEntityNum) {
            continue;
        }

        if (ent->solid != SOLID_BMODEL) { // special value for bmodel
            continue;
        }

        cmodel = CM_InlineModel(ent->modelindex);
        if (!cmodel) {
            continue;
        }

        contents |= CM_TransformedPointContents(point, cmodel, ent->origin, ent->angles);
    }

    return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState(bool grabAngles)
{
    float          f;
    int32          i;
    playerState_t* out;
    snapshot_t *   prev, *next;

    out = &cg.predictedPlayerState;
    prev = cg.snap;
    next = cg.nextSnap;

    *out = cg.snap->ps;

    // if we are still allowing local input, int16 circuit the view angles
    if (grabAngles) {
        usercmd_t cmd;
        int32     cmdNum;

        cmdNum = CL_GetCurrentCmdNumber();
        CL_GetUserCmd(cmdNum, &cmd);

        PM_UpdateViewAngles(out, &cmd);
    }

    // if the next frame is a teleport, we can't lerp to it
    if (cg.nextFrameTeleport) {
        return;
    }

    if (!next || next->serverTime <= prev->serverTime) {
        return;
    }

    f = (float) (cg.time - prev->serverTime) / (next->serverTime - prev->serverTime);

    i = next->ps.bobCycle;
    if (i < prev->ps.bobCycle) {
        i += 256; // handle wraparound
    }
    out->bobCycle = prev->ps.bobCycle + f * (i - prev->ps.bobCycle);

    for (i = 0; i < 3; i++) {
        out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i]);
        if (!grabAngles) {
            out->viewangles[i] = LerpAngle(
                prev->ps.viewangles[i], next->ps.viewangles[i], f
            );
        }
        out->velocity[i] = prev->ps.velocity[i] + f * (next->ps.velocity[i] - prev->ps.velocity[i]);
    }
}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem(centity_t* cent)
{
    gitem_t* item;

    if (!BG_PlayerTouchesItem(&cg.predictedPlayerState, &cent->currentState, cg.time)) {
        return;
    }

    // never pick an item up twice in a prediction
    if (cent->miscTime == cg.time) {
        return;
    }

    if (!BG_CanItemBeGrabbed(cgs.gametype, &cent->currentState, &cg.predictedPlayerState)) {
        return; // can't hold it
    }

    item = &bg_itemlist[cent->currentState.modelindex];

    // Special case for flags.
    // We don't predict touching our own flag
    if (cgs.gametype == GT_CTF) {
        if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED && item->giTag == PW_REDFLAG) {
            return;
        }
        if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE && item->giTag == PW_BLUEFLAG) {
            return;
        }
    }

    // grab it
    BG_AddPredictableEventToPlayerstate(EV_ITEM_PICKUP, cent->currentState.modelindex, &cg.predictedPlayerState);

    // remove it from the frame so it won't be drawn
    cent->currentState.eFlags |= EF_NODRAW;

    // don't touch it again this prediction
    cent->miscTime = cg.time;

    // if its a weapon, give them some predicted ammo so the autoswitch will work
    if (item->giType == IT_WEAPON) {
        cg.predictedPlayerState.stats[STAT_WEAPONS] |= 1 << item->giTag;
        if (!cg.predictedPlayerState.ammo[item->giTag]) {
            cg.predictedPlayerState.ammo[item->giTag] = 1;
        }
    }
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction()
{
    int32          i;
    trace_t        trace;
    entityState_t* ent;
    clipHandle_t   cmodel;
    centity_t*     cent;
    bool           spectator;

    // dead clients don't activate triggers
    if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
        return;
    }

    spectator = (cg.predictedPlayerState.pm_type == PM_SPECTATOR);

    if (cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator) {
        return;
    }

    for (i = 0; i < cg_numTriggerEntities; i++) {
        cent = cg_triggerEntities[i];
        ent = &cent->currentState;

        if (ent->eType == ET_ITEM && !spectator) {
            CG_TouchItem(cent);
            continue;
        }

        if (ent->solid != SOLID_BMODEL) {
            continue;
        }

        cmodel = CM_InlineModel(ent->modelindex);
        if (!cmodel) {
            continue;
        }

        CM_BoxTrace(&trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, cg_pmove.mins, cg_pmove.maxs, cmodel, -1, false);

        if (!trace.startsolid) {
            continue;
        }

        if (ent->eType == ET_TELEPORT_TRIGGER) {
            cg.hyperspace = true;
        } else if (ent->eType == ET_PUSH_TRIGGER) {
            BG_TouchJumpPad(&cg.predictedPlayerState, ent);
        }
    }

    // if we didn't touch a jump pad this pmove frame
    if (cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount) {
        cg.predictedPlayerState.jumppad_frame = 0;
        cg.predictedPlayerState.jumppad_ent = 0;
    }
}


/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState()
{
    int32         cmdNum, current;
    playerState_t oldPlayerState;
    bool          moved;
    usercmd_t     oldestCmd;
    usercmd_t     latestCmd;

    cg.hyperspace = false; // will be set if touching a trigger_teleport

    // if this is the first frame we must guarantee
    // predictedPlayerState is valid even if there is some
    // other error condition
    if (!cg.validPPS) {
        cg.validPPS = true;
        cg.predictedPlayerState = cg.snap->ps;
    }


    // demo playback just copies the moves
    if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {
        CG_InterpolatePlayerState(false);
        return;
    }

    // non-predicting local movement will grab the latest angles
    if (cg_synchronousClients->integer) {
        CG_InterpolatePlayerState(true);
        return;
    }

    // prepare for pmove
    cg_pmove.ps = &cg.predictedPlayerState;
    cg_pmove.trace = CG_Trace;
    cg_pmove.pointcontents = CG_PointContents;
    if (cg_pmove.ps->pm_type == PM_DEAD) {
        cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
    } else {
        cg_pmove.tracemask = MASK_PLAYERSOLID;
    }
    if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
        cg_pmove.tracemask &= ~CONTENTS_BODY; // spectators can fly through bodies
    }
    cg_pmove.noFootsteps = (cgs.dmflags & DF_NO_FOOTSTEPS) > 0;

    // save the state before the pmove so we can detect transitions
    oldPlayerState = cg.predictedPlayerState;

    current = CL_GetCurrentCmdNumber();

    // if we don't have the commands right after the snapshot, we
    // can't accurately predict a current position, so just freeze at
    // the last good position we had
    cmdNum = current - CMD_BACKUP + 1;
    CL_GetUserCmd(cmdNum, &oldestCmd);
    if (oldestCmd.serverTime > cg.snap->ps.commandTime
        && oldestCmd.serverTime < cg.time) { // special check for map_restart
        return;
    }

    // get the latest command so we can know which commands are from previous map_restarts
    CL_GetUserCmd(current, &latestCmd);

    // get the most recent information we have, even if
    // the server time is beyond our current cg.time,
    // because predicted player positions are going to
    // be ahead of everything else anyway
    if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport) {
        cg.predictedPlayerState = cg.nextSnap->ps;
        cg.physicsTime = cg.nextSnap->serverTime;
    } else {
        cg.predictedPlayerState = cg.snap->ps;
        cg.physicsTime = cg.snap->serverTime;
    }

    cg_pmove.pmove_fixed = 0;
    cg_pmove.pmove_msec = 8;

    // run cmds
    moved = false;
    for (cmdNum = current - CMD_BACKUP + 1; cmdNum <= current; cmdNum++) {
        // get the command
        CL_GetUserCmd(cmdNum, &cg_pmove.cmd);

        if (cg_pmove.pmove_fixed) {
            PM_UpdateViewAngles(cg_pmove.ps, &cg_pmove.cmd);
        }

        // don't do anything if the time is before the snapshot player time
        if (cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime) {
            continue;
        }

        // don't do anything if the command was from a previous map_restart
        if (cg_pmove.cmd.serverTime > latestCmd.serverTime) {
            continue;
        }

        // check for a prediction error from last frame
        // on a lan, this will often be the exact value
        // from the snapshot, but on a wan we will have
        // to predict several commands to get to the point
        // we want to compare
        if (cg.predictedPlayerState.commandTime == oldPlayerState.commandTime) {
            vec3_t delta;
            float  len;

            if (cg.thisFrameTeleport) {
                // a teleport will not cause an error decay
                VectorClear(cg.predictedError);
                cg.thisFrameTeleport = false;
            } else {
                vec3_t adjusted;
                CG_AdjustPositionForMover(cg.predictedPlayerState.origin, cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted);

                VectorSubtract(oldPlayerState.origin, adjusted, delta);
                len = VectorLength(delta);
                if (len > 0.1) {
                    int32 t;
                    float f;

                    t = cg.time - cg.predictedErrorTime;
                    f = (200 - t) / 200;
                    if (f < 0) {
                        f = 0;
                    }
                    VectorScale(cg.predictedError, f, cg.predictedError);

                    VectorAdd(delta, cg.predictedError, cg.predictedError);
                    cg.predictedErrorTime = cg.oldTime;
                }
            }
        }

        // don't predict gauntlet firing, which is only supposed to happen
        // when it actually inflicts damage
        cg_pmove.gauntletHit = false;

        if (cg_pmove.pmove_fixed) {
            cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + 8 - 1) / 8) * 8;
        }

        Pmove(&cg_pmove);

        moved = true;

        // add push trigger movement effects
        CG_TouchTriggerPrediction();

        // check for predictable events that changed from previous predictions
    }

    if (!moved) {
        return;
    }

    // adjust for the movement of the groundentity
    CG_AdjustPositionForMover(cg.predictedPlayerState.origin, cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.time, cg.predictedPlayerState.origin);

    // fire events and other transition triggered things
    CG_TransitionPlayerState(&cg.predictedPlayerState, &oldPlayerState);
}


