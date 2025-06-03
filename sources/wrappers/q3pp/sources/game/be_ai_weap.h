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

// projectile flags
#define PFL_WINDOWDAMAGE   1 // projectile damages through window
#define PFL_RETURN         2 // set when projectile returns to owner
// weapon flags
#define WFL_FIRERELEASED   1 // set when projectile is fired with key-up event
// damage types
#define DAMAGETYPE_IMPACT  1 // damage on impact
#define DAMAGETYPE_RADIAL  2 // radial damage
#define DAMAGETYPE_VISIBLE 4 // damage to all entities visible to the projectile

typedef struct projectileinfo_s
{
    char  name[MAX_STRINGFIELD];
    char  model[MAX_STRINGFIELD];
    int32 flags;
    float gravity;
    int32 damage;
    float radius;
    int32 visdamage;
    int32 damagetype;
    int32 healthinc;
    float push;
    float detonation;
    float bounce;
    float bouncefric;
    float bouncestop;
} projectileinfo_t;

typedef struct weaponinfo_s
{
    int32            valid;  // true if the weapon info is valid
    int32            number; // number of the weapon
    char             name[MAX_STRINGFIELD];
    char             model[MAX_STRINGFIELD];
    int32            level;
    int32            weaponindex;
    int32            flags;
    char             projectile[MAX_STRINGFIELD];
    int32            numprojectiles;
    float            hspread;
    float            vspread;
    float            speed;
    float            acceleration;
    vec3_t           recoil;
    vec3_t           offset;
    vec3_t           angleoffset;
    float            extrazvelocity;
    int32            ammoamount;
    int32            ammoindex;
    float            activate;
    float            reload;
    float            spinup;
    float            spindown;
    projectileinfo_t proj; // pointer to the used projectile
} weaponinfo_t;

// setup the weapon AI
int32 BotSetupWeaponAI();
// shut down the weapon AI
void BotShutdownWeaponAI();
// returns the best weapon to fight with
int32 BotChooseBestFightWeapon(int32 weaponstate, int32* inventory);
// returns the information of the current weapon
void BotGetWeaponInfo(int32 weaponstate, int32 weapon, weaponinfo_t* weaponinfo);
// loads the weapon weights
int32 BotLoadWeaponWeights(int32 weaponstate, char* filename);
// returns a handle to a newly allocated weapon state
int32 BotAllocWeaponState();
// frees the weapon state
void BotFreeWeaponState(int32 weaponstate);
// resets the whole weapon state
void BotResetWeaponState(int32 weaponstate);
