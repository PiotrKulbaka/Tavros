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
// bg_misc.c -- both games misc functions, all completely stateless

#include "q_shared.h"
#include "bg_public.h"

static tavros::core::logger logger("bg_misc");

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"    override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t bg_itemlist[] =
    {
        {NULL,
         NULL,
         {NULL,
          NULL,
          0, 0},
         /* icon */ NULL,
         /* pickup */ NULL,
         0,
         (itemType_t) 0,
         0,
         /* precache */ "",
         /* sounds */ ""}, // leave index 0 alone

        //
        // ARMOR
        //

        /*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_armor_shard",
            "sound/misc/ar1_pkup.wav",
            {"models/powerups/armor/shard.md3",
             "models/powerups/armor/shard_sphere.md3",
             0, 0},
            /* icon */ "icons/iconr_shard",
            /* pickup */ "Armor Shard",
            5,
            IT_ARMOR,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_armor_combat",
            "sound/misc/ar2_pkup.wav",
            {"models/powerups/armor/armor_yel.md3",
             0, 0, 0},
            /* icon */ "icons/iconr_yellow",
            /* pickup */ "Armor",
            50,
            IT_ARMOR,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_armor_body",
            "sound/misc/ar2_pkup.wav",
            {"models/powerups/armor/armor_red.md3",
             0, 0, 0},
            /* icon */ "icons/iconr_red",
            /* pickup */ "Heavy Armor",
            100,
            IT_ARMOR,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        //
        // health
        //
        /*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_health_small",
            "sound/items/s_health.wav",
            {"models/powerups/health/small_cross.md3",
             "models/powerups/health/small_sphere.md3",
             0, 0},
            /* icon */ "icons/iconh_green",
            /* pickup */ "5 Health",
            5,
            IT_HEALTH,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_health",
            "sound/items/n_health.wav",
            {"models/powerups/health/medium_cross.md3",
             "models/powerups/health/medium_sphere.md3",
             0, 0},
            /* icon */ "icons/iconh_yellow",
            /* pickup */ "25 Health",
            25,
            IT_HEALTH,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_health_large",
            "sound/items/l_health.wav",
            {"models/powerups/health/large_cross.md3",
             "models/powerups/health/large_sphere.md3",
             0, 0},
            /* icon */ "icons/iconh_red",
            /* pickup */ "50 Health",
            50,
            IT_HEALTH,
            0,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_health_mega",
            "sound/items/m_health.wav",
            {"models/powerups/health/mega_cross.md3",
             "models/powerups/health/mega_sphere.md3",
             0, 0},
            /* icon */ "icons/iconh_mega",
            /* pickup */ "Mega Health",
            100,
            IT_HEALTH,
            0,
            /* precache */ "",
            /* sounds */ ""
        },


        //
        // WEAPONS
        //

        /*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_gauntlet",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/gauntlet/gauntlet.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_gauntlet",
            /* pickup */ "Gauntlet",
            0,
            IT_WEAPON,
            WP_GAUNTLET,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_shotgun",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/shotgun/shotgun.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_shotgun",
            /* pickup */ "Shotgun",
            10,
            IT_WEAPON,
            WP_SHOTGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_machinegun",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/machinegun/machinegun.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_machinegun",
            /* pickup */ "Machinegun",
            40,
            IT_WEAPON,
            WP_MACHINEGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_grenadelauncher",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/grenadel/grenadel.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_grenade",
            /* pickup */ "Grenade Launcher",
            10,
            IT_WEAPON,
            WP_GRENADE_LAUNCHER,
            /* precache */ "",
            /* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"
        },

        /*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_rocketlauncher",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/rocketl/rocketl.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_rocket",
            /* pickup */ "Rocket Launcher",
            10,
            IT_WEAPON,
            WP_ROCKET_LAUNCHER,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_lightning",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/lightning/lightning.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_lightning",
            /* pickup */ "Lightning Gun",
            100,
            IT_WEAPON,
            WP_LIGHTNING,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_railgun",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/railgun/railgun.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_railgun",
            /* pickup */ "Railgun",
            10,
            IT_WEAPON,
            WP_RAILGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_plasmagun",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/plasma/plasma.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_plasma",
            /* pickup */ "Plasma Gun",
            50,
            IT_WEAPON,
            WP_PLASMAGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_bfg",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/bfg/bfg.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_bfg",
            /* pickup */ "BFG10K",
            20,
            IT_WEAPON,
            WP_BFG,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "weapon_grapplinghook",
            "sound/misc/w_pkup.wav",
            {"models/weapons2/grapple/grapple.md3",
             0, 0, 0},
            /* icon */ "icons/iconw_grapple",
            /* pickup */ "Grappling Hook",
            0,
            IT_WEAPON,
            WP_GRAPPLING_HOOK,
            /* precache */ "",
            /* sounds */ ""
        },

        //
        // AMMO ITEMS
        //

        /*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_shells",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/shotgunam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_shotgun",
            /* pickup */ "Shells",
            10,
            IT_AMMO,
            WP_SHOTGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_bullets",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/machinegunam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_machinegun",
            /* pickup */ "Bullets",
            50,
            IT_AMMO,
            WP_MACHINEGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_grenades",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/grenadeam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_grenade",
            /* pickup */ "Grenades",
            5,
            IT_AMMO,
            WP_GRENADE_LAUNCHER,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_cells",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/plasmaam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_plasma",
            /* pickup */ "Cells",
            30,
            IT_AMMO,
            WP_PLASMAGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_lightning",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/lightningam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_lightning",
            /* pickup */ "Lightning",
            60,
            IT_AMMO,
            WP_LIGHTNING,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_rockets",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/rocketam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_rocket",
            /* pickup */ "Rockets",
            5,
            IT_AMMO,
            WP_ROCKET_LAUNCHER,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_slugs",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/railgunam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_railgun",
            /* pickup */ "Slugs",
            10,
            IT_AMMO,
            WP_RAILGUN,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "ammo_bfg",
            "sound/misc/am_pkup.wav",
            {"models/powerups/ammo/bfgam.md3",
             0, 0, 0},
            /* icon */ "icons/icona_bfg",
            /* pickup */ "Bfg Ammo",
            15,
            IT_AMMO,
            WP_BFG,
            /* precache */ "",
            /* sounds */ ""
        },

        //
        // HOLDABLE ITEMS
        //
        /*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "holdable_teleporter",
            "sound/items/holdable.wav",
            {"models/powerups/holdable/teleporter.md3",
             0, 0, 0},
            /* icon */ "icons/teleporter",
            /* pickup */ "Personal Teleporter",
            60,
            IT_HOLDABLE,
            HI_TELEPORTER,
            /* precache */ "",
            /* sounds */ ""
        },
        /*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "holdable_medkit",
            "sound/items/holdable.wav",
            {"models/powerups/holdable/medkit.md3",
             "models/powerups/holdable/medkit_sphere.md3",
             0, 0},
            /* icon */ "icons/medkit",
            /* pickup */ "Medkit",
            60,
            IT_HOLDABLE,
            HI_MEDKIT,
            /* precache */ "",
            /* sounds */ "sound/items/use_medkit.wav"
        },

        //
        // POWERUP ITEMS
        //
        /*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_quad",
            "sound/items/quaddamage.wav",
            {"models/powerups/instant/quad.md3",
             "models/powerups/instant/quad_ring.md3",
             0, 0},
            /* icon */ "icons/quad",
            /* pickup */ "Quad Damage",
            30,
            IT_POWERUP,
            PW_QUAD,
            /* precache */ "",
            /* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
        },

        /*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_enviro",
            "sound/items/protect.wav",
            {"models/powerups/instant/enviro.md3",
             "models/powerups/instant/enviro_ring.md3",
             0, 0},
            /* icon */ "icons/envirosuit",
            /* pickup */ "Battle Suit",
            30,
            IT_POWERUP,
            PW_BATTLESUIT,
            /* precache */ "",
            /* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
        },

        /*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_haste",
            "sound/items/haste.wav",
            {"models/powerups/instant/haste.md3",
             "models/powerups/instant/haste_ring.md3",
             0, 0},
            /* icon */ "icons/haste",
            /* pickup */ "Speed",
            30,
            IT_POWERUP,
            PW_HASTE,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_invis",
            "sound/items/invisibility.wav",
            {"models/powerups/instant/invis.md3",
             "models/powerups/instant/invis_ring.md3",
             0, 0},
            /* icon */ "icons/invis",
            /* pickup */ "Invisibility",
            30,
            IT_POWERUP,
            PW_INVIS,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_regen",
            "sound/items/regeneration.wav",
            {"models/powerups/instant/regen.md3",
             "models/powerups/instant/regen_ring.md3",
             0, 0},
            /* icon */ "icons/regen",
            /* pickup */ "Regeneration",
            30,
            IT_POWERUP,
            PW_REGEN,
            /* precache */ "",
            /* sounds */ "sound/items/regen.wav"
        },

        /*QUAKED item_flight (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
         */
        {
            "item_flight",
            "sound/items/flight.wav",
            {"models/powerups/instant/flight.md3",
             "models/powerups/instant/flight_ring.md3",
             0, 0},
            /* icon */ "icons/flight",
            /* pickup */ "Flight",
            60,
            IT_POWERUP,
            PW_FLIGHT,
            /* precache */ "",
            /* sounds */ "sound/items/flight.wav"
        },

        /*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
        Only in CTF games
        */
        {
            "team_CTF_redflag",
            NULL,
            {"models/flags/r_flag.md3",
             0, 0, 0},
            /* icon */ "icons/iconf_red1",
            /* pickup */ "Red Flag",
            0,
            IT_TEAM,
            PW_REDFLAG,
            /* precache */ "",
            /* sounds */ ""
        },

        /*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
        Only in CTF games
        */
        {
            "team_CTF_blueflag",
            NULL,
            {"models/flags/b_flag.md3",
             0, 0, 0},
            /* icon */ "icons/iconf_blu1",
            /* pickup */ "Blue Flag",
            0,
            IT_TEAM,
            PW_BLUEFLAG,
            /* precache */ "",
            /* sounds */ ""
        },

        // end of list marker
        {NULL}
};

int32 bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t* BG_FindItemForPowerup(powerup_t pw)
{
    int32 i;

    for (i = 0; i < bg_numItems; i++) {
        if ((bg_itemlist[i].giType == IT_POWERUP || bg_itemlist[i].giType == IT_TEAM || bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) && bg_itemlist[i].giTag == pw) {
            return &bg_itemlist[i];
        }
    }

    return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t* BG_FindItemForHoldable(holdable_t pw)
{
    int32 i;

    for (i = 0; i < bg_numItems; i++) {
        if (bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw) {
            return &bg_itemlist[i];
        }
    }

    Com_Error(ERR_DROP, "HoldableItem not found");

    return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t* BG_FindItemForWeapon(weapon_t weapon)
{
    gitem_t* it;

    for (it = bg_itemlist + 1; it->classname; it++) {
        if (it->giType == IT_WEAPON && it->giTag == weapon) {
            return it;
        }
    }

    Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
    return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t* BG_FindItem(const char* pickupName)
{
    gitem_t* it;

    for (it = bg_itemlist + 1; it->classname; it++) {
        if (!Q_stricmp(it->pickup_name, pickupName)) {
            return it;
        }
    }

    return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
bool BG_PlayerTouchesItem(playerState_t* ps, entityState_t* item, int32 atTime)
{
    vec3_t origin;

    BG_EvaluateTrajectory(&item->pos, atTime, origin);

    // we are ignoring ducked differences here
    if (ps->origin[0] - origin[0] > 44
        || ps->origin[0] - origin[0] < -50
        || ps->origin[1] - origin[1] > 36
        || ps->origin[1] - origin[1] < -36
        || ps->origin[2] - origin[2] > 36
        || ps->origin[2] - origin[2] < -36) {
        return false;
    }

    return true;
}


/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
bool BG_CanItemBeGrabbed(int32 gametype, const entityState_t* ent, const playerState_t* ps)
{
    gitem_t* item;

    if (ent->modelindex < 1 || ent->modelindex >= bg_numItems) {
        Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
    }

    item = &bg_itemlist[ent->modelindex];

    switch (item->giType) {
    case IT_WEAPON:
        return true; // weapons are always picked up

    case IT_AMMO:
        if (ps->ammo[item->giTag] >= 200) {
            return false; // can't hold any more
        }
        return true;

    case IT_ARMOR:
        if (ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2) {
            return false;
        }
        return true;

    case IT_HEALTH:
        // small and mega healths will go over the max, otherwise
        // don't pick up if already at max
        if (item->quantity == 5 || item->quantity == 100) {
            if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2) {
                return false;
            }
            return true;
        }

        if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH]) {
            return false;
        }
        return true;

    case IT_POWERUP:
        return true; // powerups are always picked up

    case IT_TEAM:    // team items, such as flags
        if (gametype == GT_CTF) {
            // ent->modelindex2 is non-zero on items if they are dropped
            // we need to know this because we can pick up our dropped flag (and return it)
            // but we can't pick up our flag at base
            if (ps->persistant[PERS_TEAM] == TEAM_RED) {
                if (item->giTag == PW_BLUEFLAG || (item->giTag == PW_REDFLAG && ent->modelindex2) || (item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG])) {
                    return true;
                }
            } else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
                if (item->giTag == PW_REDFLAG || (item->giTag == PW_BLUEFLAG && ent->modelindex2) || (item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG])) {
                    return true;
                }
            }
        }
        return false;

    case IT_HOLDABLE:
        // can only hold one item at a time
        if (ps->stats[STAT_HOLDABLE_ITEM]) {
            return false;
        }
        return true;

    case IT_BAD:
        Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD");
    default:
        logger.info("BG_CanItemBeGrabbed: unknown enum %d", item->giType);
        break;
    }

    return false;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory(const trajectory_t* tr, int32 atTime, vec3_t result)
{
    float deltaTime;
    float phase;

    switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
        VectorCopy(tr->trBase, result);
        break;
    case TR_LINEAR:
        deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
        VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
        break;
    case TR_SINE:
        deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
        phase = sin(deltaTime * M_PI * 2);
        VectorMA(tr->trBase, phase, tr->trDelta, result);
        break;
    case TR_LINEAR_STOP:
        if (atTime > tr->trTime + tr->trDuration) {
            atTime = tr->trTime + tr->trDuration;
        }
        deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
        if (deltaTime < 0) {
            deltaTime = 0;
        }
        VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
        break;
    case TR_GRAVITY:
        deltaTime = (atTime - tr->trTime) * 0.001;                  // milliseconds to seconds
        VectorMA(tr->trBase, deltaTime, tr->trDelta, result);
        result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
        break;
    default:
        Com_Error(ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime);
        break;
    }
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta(const trajectory_t* tr, int32 atTime, vec3_t result)
{
    float deltaTime;
    float phase;

    switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
        VectorClear(result);
        break;
    case TR_LINEAR:
        VectorCopy(tr->trDelta, result);
        break;
    case TR_SINE:
        deltaTime = (atTime - tr->trTime) / (float) tr->trDuration;
        phase = cos(deltaTime * M_PI * 2); // derivative of sin = cos
        phase *= 0.5;
        VectorScale(tr->trDelta, phase, result);
        break;
    case TR_LINEAR_STOP:
        if (atTime > tr->trTime + tr->trDuration) {
            VectorClear(result);
            return;
        }
        VectorCopy(tr->trDelta, result);
        break;
    case TR_GRAVITY:
        deltaTime = (atTime - tr->trTime) * 0.001; // milliseconds to seconds
        VectorCopy(tr->trDelta, result);
        result[2] -= DEFAULT_GRAVITY * deltaTime;  // FIXME: local gravity...
        break;
    default:
        Com_Error(ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime);
        break;
    }
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void BG_AddPredictableEventToPlayerstate(int32 newEvent, int32 eventParm, playerState_t* ps)
{
    ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
    ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
    ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad(playerState_t* ps, entityState_t* jumppad)
{
    vec3_t angles;
    float  p;
    int32  effectNum;

    // spectators don't use jump pads
    if (ps->pm_type != PM_NORMAL) {
        return;
    }

    // flying characters don't hit bounce pads
    if (ps->powerups[PW_FLIGHT]) {
        return;
    }

    // if we didn't hit this same jumppad the previous frame
    // then don't play the event sound again if we are in a fat trigger
    if (ps->jumppad_ent != jumppad->number) {
        vectoangles(jumppad->origin2, angles);
        p = fabs(AngleNormalize180(angles[PITCH]));
        if (p < 45) {
            effectNum = 0;
        } else {
            effectNum = 1;
        }
        BG_AddPredictableEventToPlayerstate(EV_JUMP_PAD, effectNum, ps);
    }
    // remember hitting this jumppad this frame
    ps->jumppad_ent = jumppad->number;
    ps->jumppad_frame = ps->pmove_framecount;
    // give the player the velocity from the jumppad
    VectorCopy(jumppad->origin2, ps->velocity);
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState(playerState_t* ps, entityState_t* s, bool snap)
{
    int32 i;

    if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR) {
        s->eType = ET_INVISIBLE;
    } else if (ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
        s->eType = ET_INVISIBLE;
    } else {
        s->eType = ET_PLAYER;
    }

    s->number = ps->clientNum;

    s->pos.trType = TR_INTERPOLATE;
    VectorCopy(ps->origin, s->pos.trBase);
    if (snap) {
        SnapVector(s->pos.trBase);
    }
    // set the trDelta for flag direction
    VectorCopy(ps->velocity, s->pos.trDelta);

    s->apos.trType = TR_INTERPOLATE;
    VectorCopy(ps->viewangles, s->apos.trBase);
    if (snap) {
        SnapVector(s->apos.trBase);
    }

    s->angles2[YAW] = ps->movementDir;
    s->legsAnim = ps->legsAnim;
    s->torsoAnim = ps->torsoAnim;
    s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
                                  // so corpses can also reference the proper config
    s->eFlags = ps->eFlags;
    if (ps->stats[STAT_HEALTH] <= 0) {
        s->eFlags |= EF_DEAD;
    } else {
        s->eFlags &= ~EF_DEAD;
    }

    if (ps->externalEvent) {
        s->event = ps->externalEvent;
        s->eventParm = ps->externalEventParm;
    } else if (ps->entityEventSequence < ps->eventSequence) {
        int32 seq;

        if (ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
            ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
        }
        seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
        s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
        s->eventParm = ps->eventParms[seq];
        ps->entityEventSequence++;
    }

    s->weapon = ps->weapon;
    s->groundEntityNum = ps->groundEntityNum;

    s->powerups = 0;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (ps->powerups[i]) {
            s->powerups |= 1 << i;
        }
    }

    s->loopSound = ps->loopSound;
    s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate(playerState_t* ps, entityState_t* s, int32 time, bool snap)
{
    int32 i;

    if (ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR) {
        s->eType = ET_INVISIBLE;
    } else if (ps->stats[STAT_HEALTH] <= GIB_HEALTH) {
        s->eType = ET_INVISIBLE;
    } else {
        s->eType = ET_PLAYER;
    }

    s->number = ps->clientNum;

    s->pos.trType = TR_LINEAR_STOP;
    VectorCopy(ps->origin, s->pos.trBase);
    if (snap) {
        SnapVector(s->pos.trBase);
    }
    // set the trDelta for flag direction and linear prediction
    VectorCopy(ps->velocity, s->pos.trDelta);
    // set the time for linear prediction
    s->pos.trTime = time;
    // set maximum extra polation time
    s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

    s->apos.trType = TR_INTERPOLATE;
    VectorCopy(ps->viewangles, s->apos.trBase);
    if (snap) {
        SnapVector(s->apos.trBase);
    }

    s->angles2[YAW] = ps->movementDir;
    s->legsAnim = ps->legsAnim;
    s->torsoAnim = ps->torsoAnim;
    s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
                                  // so corpses can also reference the proper config
    s->eFlags = ps->eFlags;
    if (ps->stats[STAT_HEALTH] <= 0) {
        s->eFlags |= EF_DEAD;
    } else {
        s->eFlags &= ~EF_DEAD;
    }

    if (ps->externalEvent) {
        s->event = ps->externalEvent;
        s->eventParm = ps->externalEventParm;
    } else if (ps->entityEventSequence < ps->eventSequence) {
        int32 seq;

        if (ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
            ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
        }
        seq = ps->entityEventSequence & (MAX_PS_EVENTS - 1);
        s->event = ps->events[seq] | ((ps->entityEventSequence & 3) << 8);
        s->eventParm = ps->eventParms[seq];
        ps->entityEventSequence++;
    }

    s->weapon = ps->weapon;
    s->groundEntityNum = ps->groundEntityNum;

    s->powerups = 0;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (ps->powerups[i]) {
            s->powerups |= 1 << i;
        }
    }

    s->loopSound = ps->loopSound;
    s->generic1 = ps->generic1;
}
