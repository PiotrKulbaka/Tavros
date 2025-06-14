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
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail(vec3_t start, vec3_t end, float spacing)
{
    vec3_t move;
    vec3_t vec;
    float  len;
    int32  i;

    VectorCopy(start, move);
    VectorSubtract(end, start, vec);
    len = VectorNormalize(vec);

    // advance a random amount first
    i = rand() % (int32) spacing;
    VectorMA(move, i, vec, move);

    VectorScale(vec, spacing, vec);

    for (; i < len; i += spacing) {
        localEntity_t* le;
        refEntity_t*   re;

        le = CG_AllocLocalEntity();
        le->leFlags = LEF_PUFF_DONT_SCALE;
        le->leType = LE_MOVE_SCALE_FADE;
        le->startTime = cg.time;
        le->endTime = cg.time + 1000 + random() * 250;
        le->lifeRate = 1.0 / (le->endTime - le->startTime);

        re = &le->refEntity;
        re->shaderTime = cg.time / 1000.0f;

        re->reType = RT_SPRITE;
        re->rotation = 0;
        re->radius = 3;
        re->customShader = cgs.media.waterBubbleShader;
        re->shaderRGBA[0] = 0xff;
        re->shaderRGBA[1] = 0xff;
        re->shaderRGBA[2] = 0xff;
        re->shaderRGBA[3] = 0xff;

        le->color[3] = 1.0;

        le->pos.trType = TR_LINEAR;
        le->pos.trTime = cg.time;
        VectorCopy(move, le->pos.trBase);
        le->pos.trDelta[0] = crandom() * 5;
        le->pos.trDelta[1] = crandom() * 5;
        le->pos.trDelta[2] = crandom() * 5 + 6;

        VectorAdd(move, vec, move);
    }
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t* CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration, int32 startTime, int32 fadeInTime, int32 leFlags, qhandle_t hShader)
{
    static int32   seed = 0x92;
    localEntity_t* le;
    refEntity_t*   re;
    //    int32 fadeInTime = startTime + duration / 2;

    le = CG_AllocLocalEntity();
    le->leFlags = leFlags;
    le->radius = radius;

    re = &le->refEntity;
    re->rotation = Q_random(&seed) * 360;
    re->radius = radius;
    re->shaderTime = startTime / 1000.0f;

    le->leType = LE_MOVE_SCALE_FADE;
    le->startTime = startTime;
    le->fadeInTime = fadeInTime;
    le->endTime = startTime + duration;
    if (fadeInTime > startTime) {
        le->lifeRate = 1.0 / (le->endTime - le->fadeInTime);
    } else {
        le->lifeRate = 1.0 / (le->endTime - le->startTime);
    }
    le->color[0] = r;
    le->color[1] = g;
    le->color[2] = b;
    le->color[3] = a;


    le->pos.trType = TR_LINEAR;
    le->pos.trTime = startTime;
    VectorCopy(vel, le->pos.trDelta);
    VectorCopy(p, le->pos.trBase);

    VectorCopy(p, re->origin);
    re->customShader = hShader;

    // rage pro can't alpha fade, so use a different shader
    re->shaderRGBA[0] = le->color[0] * 0xff;
    re->shaderRGBA[1] = le->color[1] * 0xff;
    re->shaderRGBA[2] = le->color[2] * 0xff;
    re->shaderRGBA[3] = 0xff;

    re->reType = RT_SPRITE;
    re->radius = le->radius;

    return le;
}

/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect(vec3_t org)
{
    localEntity_t* le;
    refEntity_t*   re;

    le = CG_AllocLocalEntity();
    le->leFlags = 0;
    le->leType = LE_FADE_RGB;
    le->startTime = cg.time;
    le->endTime = cg.time + 500;
    le->lifeRate = 1.0 / (le->endTime - le->startTime);

    le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

    re = &le->refEntity;

    re->reType = RT_MODEL;
    re->shaderTime = cg.time / 1000.0f;

    re->customShader = cgs.media.teleportEffectShader;

    re->hModel = cgs.media.teleportEffectModel;
    AxisClear(re->axis);

    VectorCopy(org, re->origin);
    re->origin[2] -= 24;
}

/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum(int32 client, vec3_t org, int32 score)
{
    localEntity_t* le;
    refEntity_t*   re;
    vec3_t         angles;
    static vec3_t  lastPos;

    // only visualize for the client that scored
    if (client != cg.predictedPlayerState.clientNum) {
        return;
    }

    le = CG_AllocLocalEntity();
    le->leFlags = 0;
    le->leType = LE_SCOREPLUM;
    le->startTime = cg.time;
    le->endTime = cg.time + 4000;
    le->lifeRate = 1.0 / (le->endTime - le->startTime);


    le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
    le->radius = score;

    VectorCopy(org, le->pos.trBase);
    if (org[2] >= lastPos[2] - 20 && org[2] <= lastPos[2] + 20) {
        le->pos.trBase[2] -= 20;
    }

    VectorCopy(org, lastPos);


    re = &le->refEntity;

    re->reType = RT_SPRITE;
    re->radius = 16;

    VectorClear(angles);
    AnglesToAxis(angles, re->axis);
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t* CG_MakeExplosion(vec3_t origin, vec3_t dir, qhandle_t hModel, qhandle_t shader, int32 msec, bool isSprite)
{
    float          ang;
    localEntity_t* ex;
    int32          offset;
    vec3_t         tmpVec, newOrigin;

    if (msec <= 0) {
        CG_Error("CG_MakeExplosion: msec = %i", msec);
    }

    // skew the time a bit so they aren't all in sync
    offset = rand() & 63;

    ex = CG_AllocLocalEntity();
    if (isSprite) {
        ex->leType = LE_SPRITE_EXPLOSION;

        // randomly rotate sprite orientation
        ex->refEntity.rotation = rand() % 360;
        VectorScale(dir, 16, tmpVec);
        VectorAdd(tmpVec, origin, newOrigin);
    } else {
        ex->leType = LE_EXPLOSION;
        VectorCopy(origin, newOrigin);

        // set axis with random rotate
        if (!dir) {
            AxisClear(ex->refEntity.axis);
        } else {
            ang = rand() % 360;
            VectorCopy(dir, ex->refEntity.axis[0]);
            RotateAroundDirection(ex->refEntity.axis, ang);
        }
    }

    ex->startTime = cg.time - offset;
    ex->endTime = ex->startTime + msec;

    // bias the time so all shader effects start correctly
    ex->refEntity.shaderTime = ex->startTime / 1000.0f;

    ex->refEntity.hModel = hModel;
    ex->refEntity.customShader = shader;

    // set origin
    VectorCopy(newOrigin, ex->refEntity.origin);
    VectorCopy(newOrigin, ex->refEntity.oldorigin);

    ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

    return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed(vec3_t origin, int32 entityNum)
{
    localEntity_t* ex;

    ex = CG_AllocLocalEntity();
    ex->leType = LE_EXPLOSION;

    ex->startTime = cg.time;
    ex->endTime = ex->startTime + 500;

    VectorCopy(origin, ex->refEntity.origin);
    ex->refEntity.reType = RT_SPRITE;
    ex->refEntity.rotation = rand() % 360;
    ex->refEntity.radius = 24;

    ex->refEntity.customShader = cgs.media.bloodExplosionShader;

    // don't show player's own blood in view
    if (entityNum == cg.snap->ps.clientNum) {
        ex->refEntity.renderfx |= RF_THIRD_PERSON;
    }
}


/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchGib(vec3_t origin, vec3_t velocity, qhandle_t hModel)
{
    localEntity_t* le;
    refEntity_t*   re;

    le = CG_AllocLocalEntity();
    re = &le->refEntity;

    le->leType = LE_FRAGMENT;
    le->startTime = cg.time;
    le->endTime = le->startTime + 5000 + random() * 3000;

    VectorCopy(origin, re->origin);
    AxisCopy(axisDefault, re->axis);
    re->hModel = hModel;

    le->pos.trType = TR_GRAVITY;
    VectorCopy(origin, le->pos.trBase);
    VectorCopy(velocity, le->pos.trDelta);
    le->pos.trTime = cg.time;

    le->bounceFactor = 0.6f;

    le->leBounceSoundType = LEBS_BLOOD;
    le->leMarkType = LEMT_BLOOD;
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define GIB_VELOCITY 250
#define GIB_JUMP     250
void CG_GibPlayer(vec3_t playerOrigin)
{
    vec3_t origin, velocity;

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    if (rand() & 1) {
        CG_LaunchGib(origin, velocity, cgs.media.gibSkull);
    } else {
        CG_LaunchGib(origin, velocity, cgs.media.gibBrain);
    }

    // show gibs below

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibAbdomen);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibArm);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibChest);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibFist);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibFoot);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibForearm);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibIntestine);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibLeg);

    VectorCopy(playerOrigin, origin);
    velocity[0] = crandom() * GIB_VELOCITY;
    velocity[1] = crandom() * GIB_VELOCITY;
    velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
    CG_LaunchGib(origin, velocity, cgs.media.gibLeg);
}
