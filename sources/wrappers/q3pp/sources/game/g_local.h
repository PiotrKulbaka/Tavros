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

// g_local.h -- local definitions for game module

#include "q_shared.h"
#include "bg_public.h"
#include "g_public.h"

#include "../qcommon/qcommon.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION                "baseq3"

#define BODY_QUEUE_SIZE            8

#define INFINITE                   1000000

#define FRAMETIME                  100 // msec
#define CARNAGE_REWARD_TIME        3000
#define REWARD_SPRITE_TIME         2000

#define INTERMISSION_DELAY_TIME    1000
#define SP_INTERMISSION_DELAY_TIME 5000

// gentity->flags
#define FL_GODMODE                 0x00000010
#define FL_NOTARGET                0x00000020
#define FL_TEAMSLAVE               0x00000400 // not the first on the team
#define FL_NO_KNOCKBACK            0x00000800
#define FL_DROPPED_ITEM            0x00001000
#define FL_NO_BOTS                 0x00002000 // spawn point not for bot use
#define FL_NO_HUMANS               0x00004000 // spawn point just for bots
#define FL_FORCE_GESTURE           0x00008000 // force gesture on client

// movers are things like doors, plats, buttons, etc
typedef enum
{
    MOVER_POS1,
    MOVER_POS2,
    MOVER_1TO2,
    MOVER_2TO1
} moverState_t;

#define SP_PODIUM_MODEL "models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s
{
    entityState_t  s; // communicated by server to clients
    entityShared_t r; // shared by both the server system and game

    // DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
    // EXPECTS THE FIELDS IN THAT ORDER!
    //================================

    struct gclient_s* client; // NULL if not a client

    bool inuse;

    const char* classname;  // set in QuakeEd
    int32       spawnflags; // set in QuakeEd

    bool neverFree;         // if true, FreeEntity will only unlink
                            // bodyque uses this

    int32 flags;            // FL_* variables

    char* model;
    char* model2;
    int32 freetime;  // level.time when the object was freed

    int32 eventTime; // events will be cleared EVENT_VALID_MSEC after set
    bool  freeAfterEvent;
    bool  unlinkAfterEvent;

    bool physicsObject;  // if true, it can be pushed by movers and fall off edges
                         // all game items are physicsObjects,
    float physicsBounce; // 1.0 = continuous bounce, 0.0 = no bounce
    int32 clipmask;      // brushes with this content value will be collided against
                         // when moving.  items and corpses do not collide against
                         // players, for instance

    // movers
    moverState_t moverState;
    int32        soundPos1;
    int32        sound1to2;
    int32        sound2to1;
    int32        soundPos2;
    int32        soundLoop;
    gentity_t*   parent;
    gentity_t*   nextTrain;
    gentity_t*   prevTrain;
    vec3_t       pos1, pos2;

    char* message;

    int32 timestamp;  // body queue sinking, etc

    float      angle; // set in editor, -1 = up, -2 = down
    char*      target;
    char*      targetname;
    char*      team;
    char*      targetShaderName;
    char*      targetShaderNewName;
    gentity_t* target_ent;

    float  speed;
    vec3_t movedir;

    int32 nextthink;
    void  (*think)(gentity_t* self);
    void  (*reached)(gentity_t* self); // movers call this when hitting endpoint
    void  (*blocked)(gentity_t* self, gentity_t* other);
    void  (*touch)(gentity_t* self, gentity_t* other, trace_t* trace);
    void  (*use)(gentity_t* self, gentity_t* other, gentity_t* activator);
    void  (*pain)(gentity_t* self, gentity_t* attacker, int32 damage);
    void  (*die)(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int32 damage, int32 mod);

    int32 pain_debounce_time;
    int32 fly_sound_debounce_time; // wind tunnel
    int32 last_move_time;

    int32 health;

    bool takedamage;

    int32 damage;
    int32 splashDamage; // quad will increase this without increasing radius
    int32 splashRadius;
    int32 methodOfDeath;
    int32 splashMethodOfDeath;

    int32 count;

    gentity_t* chain;
    gentity_t* enemy;
    gentity_t* activator;
    gentity_t* teamchain;  // next entity in team
    gentity_t* teammaster; // master of the team

    int32 watertype;
    int32 waterlevel;

    int32 noise_index;

    // timing variables
    float wait;
    float random;

    gitem_t* item; // for bonus items
};


typedef enum
{
    CON_DISCONNECTED,
    CON_CONNECTING,
    CON_CONNECTED
} clientConnected_t;

typedef enum
{
    SPECTATOR_NOT,
    SPECTATOR_FREE,
    SPECTATOR_FOLLOW,
    SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum
{
    TEAM_BEGIN, // Beginning a team game, spawn at base
    TEAM_ACTIVE // Now actively playing
} playerTeamStateState_t;

typedef struct
{
    playerTeamStateState_t state;

    int32 location;

    int32 captures;
    int32 basedefense;
    int32 carrierdefense;
    int32 flagrecovery;
    int32 fragcarrier;
    int32 assists;

    float lasthurtcarrier;
    float lastreturnedflag;
    float flagsince;
    float lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1 -1
#define FOLLOW_ACTIVE2 -2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct
{
    team_t           sessionTeam;
    int32            spectatorTime;   // for determining next-in-line to play
    spectatorState_t spectatorState;
    int32            spectatorClient; // for chasecam and follow mode
    int32            wins, losses;    // tournament stats
    bool             teamLeader;      // true when this client is a team leader
} clientSession_t;

//
#define MAX_NETNAME    36
#define MAX_VOTE_COUNT 3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct
{
    clientConnected_t connected;
    usercmd_t         cmd;               // we would lose angles if not persistant
    bool              localClient;       // true if "ip" info key is "localhost"
    bool              initialSpawn;      // the first spawn should be at a cool location
    bool              predictItemPickup; // based on cg_predictItems userinfo
    bool              pmoveFixed;        //
    char              netname[MAX_NETNAME];
    int32             maxHealth;         // for handicapping
    int32             enterTime;         // level.time the client entered the game
    playerTeamState_t teamState;         // status in teamplay games
    int32             voteCount;         // to prevent people from constantly calling votes
    int32             teamVoteCount;     // to prevent people from constantly calling votes
    bool              teamInfo;          // send team overlay updates?
} clientPersistant_t;


// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s
{
    // ps MUST be the first element, because the server expects it
    playerState_t ps; // communicated by server to clients

    // the rest of the structure is private to game
    clientPersistant_t pers;
    clientSession_t    sess;

    bool readyToExit; // wishes to leave the intermission

    bool noclip;

    int32 lastCmdTime; // level.time of last usercmd_t, for EF_CONNECTION
                       // we can't just use pers.lastCommand.time, because
                       // of the g_sycronousclients case
    int32 buttons;
    int32 oldbuttons;
    int32 latched_buttons;

    vec3_t oldOrigin;

    // sum up damage over an entire frame, so
    // shotgun blasts give a single big kick
    int32  damage_armor;     // damage absorbed by armor
    int32  damage_blood;     // damage taken out of health
    int32  damage_knockback; // impact damage
    vec3_t damage_from;      // origin for vector calculation
    bool   damage_fromWorld; // if true, don't use the damage_from vector

    int32 accurateCount;     // for "impressive" reward sound

    int32 accuracy_shots;    // total number of shots
    int32 accuracy_hits;     // total number of hits

    //
    int32 lastkilled_client; // last client that this client killed
    int32 lasthurt_client;   // last client that damaged this client
    int32 lasthurt_mod;      // type of damage the client did

    // timers
    int32 respawnTime;       // can respawn when time > this, force after g_forcerespwan
    int32 inactivityTime;    // kick players when time > this
    bool  inactivityWarning; // true if the five seoond warning has been given
    int32 rewardTime;        // clear the EF_AWARD_IMPRESSIVE, etc when time > this

    int32 airOutTime;

    int32 lastKillTime;   // for multiple kill rewards

    bool       fireHeld;  // used for hook
    gentity_t* hook;      // grapple hook if out

    int32 switchTeamTime; // time the player switched teams

    // timeResidual is used to handle events that happen every second
    // like health / armor countdowns and regeneration
    int32 timeResidual;
    char* areabits;
};


//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS       64
#define MAX_SPAWN_VARS_CHARS 4096

typedef struct
{
    struct gclient_s* clients; // [maxclients]

    struct gentity_s* gentities;
    int32             gentitySize;
    int32             num_entities; // current number, <= MAX_GENTITIES

    int32 warmupTime;               // restart match at this time

    // store latched cvars here that we want to get at often
    int32 maxclients;

    int32 framenum;
    int32 time;         // in msec
    int32 previousTime; // so movers can back up when blocked

    int32 startTime;    // level.time the map was started

    int32 teamScores[TEAM_NUM_TEAMS];
    int32 lastTeamLocationTime; // last time of client team location update

    bool newSession;            // don't use any old session data, because
                                // we changed gametype

    bool restarted;             // waiting for a map_restart to fire

    int32 numConnectedClients;
    int32 numNonSpectatorClients;     // includes connecting clients
    int32 numPlayingClients;          // connected, non-spectators
    int32 sortedClients[MAX_CLIENTS]; // sorted by score
    int32 follow1, follow2;           // clientNums for auto-follow spectators

    int32 snd_fry;                    // sound index for standing in lava

    // voting state
    char  voteString[MAX_STRING_CHARS];
    char  voteDisplayString[MAX_STRING_CHARS];
    int32 voteTime;        // level.time vote was called
    int32 voteExecuteTime; // time the vote is executed
    int32 voteYes;
    int32 voteNo;
    int32 numVotingClients; // set by CalculateRanks

    // team voting state
    char  teamVoteString[2][MAX_STRING_CHARS];
    int32 teamVoteTime[2]; // level.time vote was called
    int32 teamVoteYes[2];
    int32 teamVoteNo[2];
    int32 numteamVotingClients[2]; // set by CalculateRanks

    // spawn variables
    bool  spawning;                     // the G_Spawn*() functions are valid
    int32 numSpawnVars;
    char* spawnVars[MAX_SPAWN_VARS][2]; // key / value pairs
    int32 numSpawnVarChars;
    char  spawnVarChars[MAX_SPAWN_VARS_CHARS];

    // intermission state
    int32 intermissionQueued;   // intermission was qualified, but
                                // wait INTERMISSION_DELAY_TIME before
                                // actually going there so the last
                                // frag can be watched.  Disable future
                                // kills during this delay
    int32  intermissiontime;    // time the intermission was started
    char*  changemap;
    bool   readyToExit;         // at least one client wants to exit
    int32  exitTime;
    vec3_t intermission_origin; // also used for spectator spawns
    vec3_t intermission_angle;

    bool       locationLinked; // target_locations get linked
    gentity_t* locationHead;   // head of the location list
    int32      bodyQueIndex;   // dead bodies
    gentity_t* bodyQue[BODY_QUEUE_SIZE];
} level_locals_t;


//
// g_spawn.c
//
bool G_SpawnString(const char* key, const char* defaultString, char** out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
bool  G_SpawnFloat(const char* key, const char* defaultString, float* out);
bool  G_SpawnInt(const char* key, const char* defaultString, int32* out);
bool  G_SpawnVector(const char* key, const char* defaultString, float* out);
void  G_SpawnEntitiesFromString();
char* G_NewString(const char* string);

//
// g_cmds.c
//
void Cmd_Score_f(gentity_t* ent);
void StopFollowing(gentity_t* ent);
void BroadcastTeamChange(gclient_t* client, int32 oldTeam);
void SetTeam(gentity_t* ent, const char* s);
void Cmd_FollowCycle_f(gentity_t* ent, int32 dir);

//
// g_items.c
//
void G_CheckTeamItems();
void G_RunItem(gentity_t* ent);
void RespawnItem(gentity_t* ent);

gentity_t* Drop_Item(gentity_t* ent, gitem_t* item, float angle);
gentity_t* LaunchItem(gitem_t* item, vec3_t origin, vec3_t velocity);
void       G_SpawnItem(gentity_t* ent, gitem_t* item);
void       FinishSpawningItem(gentity_t* ent);
void       Add_Ammo(gentity_t* ent, int32 weapon, int32 count);
void       Touch_Item(gentity_t* ent, gentity_t* other, trace_t* trace);

void ClearRegisteredItems();
void RegisterItem(gitem_t* item);
void SaveRegisteredItems();

//
// g_utils.c
//
int32      G_ModelIndex(const char* name);
int32      G_SoundIndex(const char* name);
void       G_TeamCommand(team_t team, char* cmd);
void       G_KillBox(gentity_t* ent);
gentity_t* G_Find(gentity_t* from, int32 fieldofs, const char* match);
gentity_t* G_PickTarget(char* targetname);
void       G_UseTargets(gentity_t* ent, gentity_t* activator);
void       G_SetMovedir(vec3_t angles, vec3_t movedir);

void       G_InitGentity(gentity_t* e);
gentity_t* G_Spawn();
gentity_t* G_TempEntity(vec3_t origin, int32 event);
void       G_Sound(gentity_t* ent, int32 channel, int32 soundIndex);
void       G_FreeEntity(gentity_t* e);

void G_TouchTriggers(gentity_t* ent);

char* vtos(const vec3_t v);

float vectoyaw(const vec3_t vec);

void        G_AddPredictableEvent(gentity_t* ent, int32 event, int32 eventParm);
void        G_AddEvent(gentity_t* ent, int32 event, int32 eventParm);
void        G_SetOrigin(gentity_t* ent, vec3_t origin);
void        AddRemap(const char* oldShader, const char* newShader, float timeOffset);
const char* BuildShaderStateConfig();

//
// g_combat.c
//
bool CanDamage(gentity_t* targ, vec3_t origin);
void G_Damage(gentity_t* targ, gentity_t* inflictor, gentity_t* attacker, vec3_t dir, vec3_t point, int32 damage, int32 dflags, int32 mod);
bool G_RadiusDamage(vec3_t origin, gentity_t* attacker, float damage, float radius, gentity_t* ignore, int32 mod);
void body_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int32 damage, int32 meansOfDeath);
void TossClientItems(gentity_t* self);

// damage flags
#define DAMAGE_RADIUS        0x00000001 // damage was indirect
#define DAMAGE_NO_ARMOR      0x00000002 // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK  0x00000004 // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION 0x00000008 // armor, shields, invulnerability, and godmode have no effect

//
// g_missile.c
//
void G_RunMissile(gentity_t* ent);

gentity_t* fire_plasma(gentity_t* self, vec3_t start, vec3_t aimdir);
gentity_t* fire_grenade(gentity_t* self, vec3_t start, vec3_t aimdir);
gentity_t* fire_rocket(gentity_t* self, vec3_t start, vec3_t dir);
gentity_t* fire_bfg(gentity_t* self, vec3_t start, vec3_t dir);
gentity_t* fire_grapple(gentity_t* self, vec3_t start, vec3_t dir);


//
// g_mover.c
//
void G_RunMover(gentity_t* ent);
void Touch_DoorTrigger(gentity_t* ent, gentity_t* other, trace_t* trace);

//
// g_trigger.c
//
void trigger_teleporter_touch(gentity_t* self, gentity_t* other, trace_t* trace);


//
// g_misc.c
//
void TeleportPlayer(gentity_t* player, vec3_t origin, vec3_t angles);


//
// g_weapon.c
//
bool LogAccuracyHit(gentity_t* target, gentity_t* attacker);
void CalcMuzzlePoint(gentity_t* ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);
bool CheckGauntletAttack(gentity_t* ent);
void Weapon_HookFree(gentity_t* ent);
void Weapon_HookThink(gentity_t* ent);


//
// g_client.c
//
team_t     TeamCount(int32 ignoreClientNum, int32 team);
int32      TeamLeader(int32 team);
team_t     PickTeam(int32 ignoreClientNum);
void       SetClientViewAngle(gentity_t* ent, vec3_t angle);
gentity_t* SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles);
void       CopyToBodyQue(gentity_t* ent);
void       respawn(gentity_t* ent);
void       BeginIntermission();
void       InitBodyQue();
void       ClientSpawn(gentity_t* ent);
void       player_die(gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int32 damage, int32 mod);
void       AddScore(gentity_t* ent, vec3_t origin, int32 score);
void       CalculateRanks();
bool       SpotWouldTelefrag(gentity_t* spot);

//
// g_svcmds.c
//
void G_ProcessIPBans();
bool G_FilterPacket(char* from);

//
// g_weapon.c
//
void FireWeapon(gentity_t* ent);

//
// p_hud.c
//
void MoveClientToIntermission(gentity_t* client);
void DeathmatchScoreboardMessage(gentity_t* client);

//
// g_cmds.c
//

//
// g_pweapon.c
//


//
// g_main.c
//
void       FindIntermissionPoint();
void       SetLeader(int32 team, int32 client);
void       CheckTeamLeader(int32 team);
void       G_RunThink(gentity_t* ent);
void       SendScoreboardMessageToAllClients();
void QDECL G_Error(const char* fmt, ...);

//
// g_active.c
//
void ClientEndFrame(gentity_t* ent);
void G_RunClient(gentity_t* ent);

//
// g_team.c
//
bool OnSameTeam(gentity_t* ent1, gentity_t* ent2);
void Team_CheckDroppedItem(gentity_t* dropped);

//
// g_mem.c
//
void* G_Alloc(int32 size);
void  G_InitMemory();
void  Svcmd_GameMem_f();

//
// g_session.c
//
void G_ReadSessionData(gclient_t* client);
void G_InitSessionData(gclient_t* client, char* userinfo);

void G_InitWorldSession();
void G_WriteSessionData();

//
// g_arenas.c
//
void UpdateTournamentInfo();
void SpawnModelsOnVictoryPads();
void Svcmd_AbortPodium_f();

//
// g_bot.c
//
void  G_InitBots(bool restart);
char* G_GetBotInfoByName(const char* name);
void  G_CheckBotSpawn();
void  G_RemoveQueuedBotBegin(int32 clientNum);
bool  G_BotConnect(int32 clientNum, bool restart);
void  Svcmd_AddBot_f();
void  Svcmd_BotList_f();
void  BotInterbreedEndMatch();

// ai_main.c
#define MAX_FILEPATH 144

// bot settings
typedef struct bot_settings_s
{
    char  characterfile[MAX_FILEPATH];
    float skill;
    char  team[MAX_FILEPATH];
} bot_settings_t;

int32 BotAISetup(int32 restart);
int32 BotAIShutdown(int32 restart);
int32 BotAILoadMap(int32 restart);
int32 BotAISetupClient(int32 client, struct bot_settings_s* settings, bool restart);
int32 BotAIShutdownClient(int32 client, bool restart);
void  BotTestAAS(vec3_t origin);

#include "g_team.h" // teamplay specific stuff


extern level_locals_t level;
extern gentity_t      g_entities[MAX_GENTITIES];

#define FOFS(x) ((int32) & (((gentity_t*) 0)->x))

extern cvar_t* g_gametype;
extern cvar_t* g_dedicated;
extern cvar_t* g_cheats;
extern cvar_t* g_maxclients;     // allow this many total, including spectators
extern cvar_t* g_maxGameClients; // allow this many active
extern cvar_t* g_restarted;

extern cvar_t* g_dmflags;
extern cvar_t* g_fraglimit;
extern cvar_t* g_timelimit;
extern cvar_t* g_capturelimit;
extern cvar_t* g_friendlyFire;
extern cvar_t* g_password;
extern cvar_t* g_gravity;
extern cvar_t* g_speed;
extern cvar_t* g_knockback;
extern cvar_t* g_quadfactor;
extern cvar_t* g_forcerespawn;
extern cvar_t* g_inactivity;
extern cvar_t* g_debugAlloc;
extern cvar_t* g_debugDamage;
extern cvar_t* g_weaponRespawn;
extern cvar_t* g_weaponTeamRespawn;
extern cvar_t* g_synchronousClients;
extern cvar_t* g_motd;
extern cvar_t* g_warmup;
extern cvar_t* g_doWarmup;
extern cvar_t* g_allowVote;
extern cvar_t* g_teamAutoJoin;
extern cvar_t* g_teamForceBalance;
extern cvar_t* g_banIPs;
extern cvar_t* g_filterBan;
extern cvar_t* g_smoothClients;
extern cvar_t* pmove_fixed;
extern cvar_t* pmove_msec;
extern cvar_t* g_rankings;

void  trap_LocateGameData(gentity_t* gEnts, int32 numGEntities, int32 sizeofGEntity_t, playerState_t* gameClients, int32 sizeofGameClient);
void  trap_DropClient(int32 clientNum, const char* reason);
void  trap_SendServerCommand(int32 clientNum, const char* text);
void  trap_SetConfigstring(int32 num, const char* string);
void  trap_GetConfigstring(int32 num, char* buffer, int32 bufferSize);
void  trap_GetUserinfo(int32 num, char* buffer, int32 bufferSize);
void  trap_SetUserinfo(int32 num, const char* buffer);
void  trap_GetServerinfo(char* buffer, int32 bufferSize);
void  trap_SetBrushModel(gentity_t* ent, const char* name);
void  trap_Trace(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int32 passEntityNum, int32 contentmask);
int32 trap_PointContents(const vec3_t point, int32 passEntityNum);
bool  trap_InPVS(const vec3_t p1, const vec3_t p2);
void  trap_AdjustAreaPortalState(gentity_t* ent, bool open);
void  trap_LinkEntity(gentity_t* ent);
void  trap_UnlinkEntity(gentity_t* ent);
int32 trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int32* entityList, int32 maxcount);
bool  trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t* ent);
int32 trap_BotAllocateClient();
void  trap_GetUsercmd(int32 clientNum, usercmd_t* cmd);
bool  trap_GetEntityToken(char* buffer, int32 bufferSize);

int32 trap_BotLibSetup();
int32 trap_BotLibShutdown();
int32 trap_BotLibVarSet(const char* var_name, const char* value);
int32 trap_BotLibStartFrame(float time);
int32 trap_BotLibLoadMap(const char* mapname);
int32 trap_BotLibUpdateEntity(int32 ent, void /* struct bot_updateentity_s */* bue);

int32 trap_BotGetSnapshotEntity(int32 clientNum, int32 sequence);
int32 trap_BotGetServerCommand(int32 clientNum, char* message, int32 size);
void  trap_BotUserCommand(int32 client, usercmd_t* ucmd);

int32 trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int32* areas, int32 maxareas);
int32 trap_AAS_AreaInfo(int32 areanum, void /* struct aas_areainfo_s */* info);
void  trap_AAS_EntityInfo(int32 entnum, void /* struct aas_entityinfo_s */* info);

int32 trap_AAS_Initialized();
void  trap_AAS_PresenceTypeBoundingBox(int32 presencetype, vec3_t mins, vec3_t maxs);
float trap_AAS_Time();

int32 trap_AAS_PointAreaNum(vec3_t point);
int32 trap_AAS_TraceAreas(vec3_t start, vec3_t end, int32* areas, vec3_t* points, int32 maxareas);

int32 trap_AAS_PointContents(vec3_t point);
int32 trap_AAS_NextBSPEntity(int32 ent);
int32 trap_AAS_ValueForBSPEpairKey(int32 ent, const char* key, char* value, int32 size);
int32 trap_AAS_VectorForBSPEpairKey(int32 ent, const char* key, vec3_t v);
int32 trap_AAS_FloatForBSPEpairKey(int32 ent, const char* key, float* value);
int32 trap_AAS_IntForBSPEpairKey(int32 ent, const char* key, int32* value);

int32 trap_AAS_AreaReachability(int32 areanum);

int32 trap_AAS_AreaTravelTimeToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags);
int32 trap_AAS_EnableRoutingArea(int32 areanum, int32 enable);
int32 trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/* route, int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32 maxareas, int32 maxtime, int32 stopevent, int32 stopcontents, int32 stoptfl, int32 stopareanum);

int32 trap_AAS_Swimming(vec3_t origin);
int32 trap_AAS_PredictClientMovement(void /* aas_clientmove_s */* move, int32 entnum, vec3_t origin, int32 presencetype, int32 onground, vec3_t velocity, vec3_t cmdmove, int32 cmdframes, int32 maxframes, float frametime, int32 stopevent, int32 stopareanum, int32 visualize);

void trap_EA_SayTeam(int32 client, const char* str);
void trap_EA_Command(int32 client, const char* command);

void trap_EA_Action(int32 client, int32 action);
void trap_EA_Gesture(int32 client);
void trap_EA_Talk(int32 client);
void trap_EA_Attack(int32 client);
void trap_EA_Use(int32 client);
void trap_EA_Respawn(int32 client);
void trap_EA_Crouch(int32 client);
void trap_EA_SelectWeapon(int32 client, int32 weapon);
void trap_EA_View(int32 client, vec3_t viewangles);

void trap_EA_GetInput(int32 client, float thinktime, void /* struct bot_input_s */* input);
void trap_EA_ResetInput(int32 client);


int32 trap_BotLoadCharacter(char* charfile, float skill);
void  trap_BotFreeCharacter(int32 character);
float trap_Characteristic_BFloat(int32 character, int32 index, float min, float max);
int32 trap_Characteristic_BInteger(int32 character, int32 index, int32 min, int32 max);
void  trap_Characteristic_String(int32 character, int32 index, char* buf, int32 size);

int32 trap_BotAllocChatState();
void  trap_BotFreeChatState(int32 handle);
void  trap_BotQueueConsoleMessage(int32 chatstate, int32 type, char* message);
void  trap_BotRemoveConsoleMessage(int32 chatstate, int32 handle);
int32 trap_BotNextConsoleMessage(int32 chatstate, void /* struct bot_consolemessage_s */* cm);
int32 trap_BotNumConsoleMessages(int32 chatstate);
void  trap_BotInitialChat(int32 chatstate, const char* type, int32 mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
int32 trap_BotNumInitialChats(int32 chatstate, const char* type);
int32 trap_BotReplyChat(int32 chatstate, char* message, int32 mcontext, int32 vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
void  trap_BotEnterChat(int32 chatstate, int32 client, int32 sendto);
void  trap_BotGetChatMessage(int32 chatstate, char* buf, int32 size);
int32 trap_BotFindMatch(char* str, void /* struct bot_match_s */* match, uint32 context);
void  trap_BotMatchVariable(void /* struct bot_match_s */* match, int32 variable, char* buf, int32 size);
void  trap_UnifyWhiteSpaces(char* string);
void  trap_BotReplaceSynonyms(char* string, uint32 context);
int32 trap_BotLoadChatFile(int32 chatstate, char* chatfile, char* chatname);
void  trap_BotSetChatGender(int32 chatstate, int32 gender);
void  trap_BotSetChatName(int32 chatstate, char* name, int32 client);
void  trap_BotResetGoalState(int32 goalstate);
void  trap_BotRemoveFromAvoidGoals(int32 goalstate, int32 number);
void  trap_BotResetAvoidGoals(int32 goalstate);
void  trap_BotPushGoal(int32 goalstate, void /* struct bot_goal_s */* goal);
void  trap_BotPopGoal(int32 goalstate);
void  trap_BotEmptyGoalStack(int32 goalstate);
void  trap_BotDumpAvoidGoals(int32 goalstate);
void  trap_BotDumpGoalStack(int32 goalstate);
void  trap_BotGoalName(int32 number, char* name, int32 size);
int32 trap_BotGetTopGoal(int32 goalstate, void /* struct bot_goal_s */* goal);
int32 trap_BotGetSecondGoal(int32 goalstate, void /* struct bot_goal_s */* goal);
int32 trap_BotChooseLTGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags);
int32 trap_BotChooseNBGItem(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags, void /* struct bot_goal_s */* ltg, float maxtime);
int32 trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */* goal);
int32 trap_BotItemGoalInVisButNotVisible(int32 viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */* goal);
int32 trap_BotGetNextCampSpotGoal(int32 num, void /* struct bot_goal_s */* goal);
int32 trap_BotGetLevelItemGoal(int32 index, const char* classname, void /* struct bot_goal_s */* goal);
void  trap_BotSetAvoidGoalTime(int32 goalstate, int32 number, float avoidtime);
void  trap_BotUpdateEntityItems();
int32 trap_BotLoadItemWeights(int32 goalstate, char* filename);
void  trap_BotInterbreedGoalFuzzyLogic(int32 parent1, int32 parent2, int32 child);
void  trap_BotSaveGoalFuzzyLogic(int32 goalstate, char* filename);
void  trap_BotMutateGoalFuzzyLogic(int32 goalstate, float range);
int32 trap_BotAllocGoalState(int32 state);
void  trap_BotFreeGoalState(int32 handle);

void  trap_BotResetMoveState(int32 movestate);
void  trap_BotMoveToGoal(void /* struct bot_moveresult_s */* result, int32 movestate, void /* struct bot_goal_s */* goal, int32 travelflags);
int32 trap_BotMoveInDirection(int32 movestate, vec3_t dir, float speed, int32 type);
void  trap_BotResetAvoidReach(int32 movestate);
void  trap_BotResetLastAvoidReach(int32 movestate);
int32 trap_BotMovementViewTarget(int32 movestate, void /* struct bot_goal_s */* goal, int32 travelflags, float lookahead, vec3_t target);
int32 trap_BotPredictVisiblePosition(vec3_t origin, int32 areanum, void /* struct bot_goal_s */* goal, int32 travelflags, vec3_t target);
int32 trap_BotAllocMoveState();
void  trap_BotFreeMoveState(int32 handle);
void  trap_BotInitMoveState(int32 handle, void /* struct bot_initmove_s */* initmove);
void  trap_BotAddAvoidSpot(int32 movestate, vec3_t origin, float radius, int32 type);

int32 trap_BotChooseBestFightWeapon(int32 weaponstate, int32* inventory);
void  trap_BotGetWeaponInfo(int32 weaponstate, int32 weapon, void /* struct weaponinfo_s */* weaponinfo);
int32 trap_BotLoadWeaponWeights(int32 weaponstate, char* filename);
int32 trap_BotAllocWeaponState();
void  trap_BotFreeWeaponState(int32 weaponstate);
void  trap_BotResetWeaponState(int32 weaponstate);

int32 trap_GeneticParentsAndChildSelection(int32 numranks, float* ranks, int32* parent1, int32* parent2, int32* child);

// sv_game.c
void SV_LocateGameData(sharedEntity_t* gEnts, int32 numGEntities, int32 sizeofGEntity_t, playerState_t* clients, int32 sizeofGameClient);
void SV_GameDropClient(int32 clientNum, const char* reason);
void SV_GameSendServerCommand(int32 clientNum, const char* text);
void SV_GetServerinfo(char* buffer, int32 bufferSize);
void SV_SetBrushModel(sharedEntity_t* ent, const char* name);
void SV_AdjustAreaPortalState(sharedEntity_t* ent, bool open);
bool SV_EntityContact(vec3_t mins, vec3_t maxs, const sharedEntity_t* gEnt, int32 capsule);
void SV_GetUsercmd(int32 clientNum, usercmd_t* cmd);
