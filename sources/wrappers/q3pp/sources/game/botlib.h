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


#include <tavros/core/types.hpp>
#include <game/q_shared.h>

#define BOTLIB_API_VERSION 2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct aas_areainfo_s;
struct aas_altroutegoal_s;
struct aas_predictroute_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;

#define BOTFILESBASEFOLDER            "botfiles"
// debug line colors
#define LINECOLOR_NONE                -1
#define LINECOLOR_RED                 1 // 0xf2f2f0f0L
#define LINECOLOR_GREEN               2 // 0xd0d1d2d3L
#define LINECOLOR_BLUE                3 // 0xf3f3f1f1L
#define LINECOLOR_YELLOW              4 // 0xdcdddedfL
#define LINECOLOR_ORANGE              5 // 0xe0e1e2e3L

// Print types
#define PRT_MESSAGE                   1
#define PRT_WARNING                   2
#define PRT_ERROR                     3
#define PRT_FATAL                     4
#define PRT_EXIT                      5

// console message types
#define CMS_NORMAL                    0
#define CMS_CHAT                      1

// botlib error codes
#define BLERR_NOERROR                 0  // no error
#define BLERR_LIBRARYNOTSETUP         1  // library not setup
#define BLERR_INVALIDENTITYNUMBER     2  // invalid entity number
#define BLERR_NOAASFILE               3  // no AAS file available
#define BLERR_CANNOTOPENAASFILE       4  // cannot open AAS file
#define BLERR_WRONGAASFILEID          5  // incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION     6  // incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP       7  // cannot read AAS file lump
#define BLERR_CANNOTLOADICHAT         8  // cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS   9  // cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG    10 // cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS 11 // cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG  12 // cannot load weapon config

// action flags
#define ACTION_ATTACK                 0x0000001
#define ACTION_USE                    0x0000002
#define ACTION_RESPAWN                0x0000008
#define ACTION_JUMP                   0x0000010
#define ACTION_MOVEUP                 0x0000020
#define ACTION_CROUCH                 0x0000080
#define ACTION_MOVEDOWN               0x0000100
#define ACTION_MOVEFORWARD            0x0000200
#define ACTION_MOVEBACK               0x0000800
#define ACTION_MOVELEFT               0x0001000
#define ACTION_MOVERIGHT              0x0002000
#define ACTION_DELAYEDJUMP            0x0008000
#define ACTION_TALK                   0x0010000
#define ACTION_GESTURE                0x0020000
#define ACTION_WALK                   0x0080000
#define ACTION_AFFIRMATIVE            0x0100000
#define ACTION_NEGATIVE               0x0200000
#define ACTION_GETFLAG                0x0800000
#define ACTION_GUARDBASE              0x1000000
#define ACTION_PATROL                 0x2000000
#define ACTION_FOLLOWME               0x8000000

// the bot input, will be converted to an usercmd_t
typedef struct bot_input_s
{
    float  thinktime;   // time since last output (in seconds)
    vec3_t dir;         // movement direction
    float  speed;       // speed in the range [0, 400]
    vec3_t viewangles;  // the view angles
    int32  actionflags; // one of the ACTION_? flags
    int32  weapon;      // weapon to use
} bot_input_t;

#ifndef BSPTRACE

    #define BSPTRACE

// bsp_trace_t hit surface
typedef struct bsp_surface_s
{
    char  name[16];
    int32 flags;
    int32 value;
} bsp_surface_t;

// remove the bsp_trace_s structure definition l8r on
// a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
    bool          allsolid;   // if true, plane is not valid
    bool          startsolid; // if true, the initial point was in a solid area
    float         fraction;   // time completed, 1.0 = didn't hit anything
    vec3_t        endpos;     // final position
    cplane_t      plane;      // surface normal at impact
    float         exp_dist;   // expanded plane distance
    int32         sidenum;    // number of the brush side hit
    bsp_surface_t surface;    // the hit point surface
    int32         contents;   // contents on other side of surface hit
    int32         ent;        // number of entity hit
} bsp_trace_t;

#endif // BSPTRACE

// entity state
typedef struct bot_entitystate_s
{
    int32  type;        // entity type
    int32  flags;       // entity flags
    vec3_t origin;      // origin of the entity
    vec3_t angles;      // angles of the model
    vec3_t old_origin;  // for lerping
    vec3_t mins;        // bounding box minimums
    vec3_t maxs;        // bounding box maximums
    int32  groundent;   // ground entity
    int32  solid;       // solid type
    int32  modelindex;  // model used
    int32  modelindex2; // weapons, CTF flags, etc
    int32  frame;       // model frame number
    int32  event;       // impulse events -- muzzle flashes, footsteps, etc
    int32  eventParm;   // even parameter
    int32  powerups;    // bit flags
    int32  weapon;      // determines weapon and flash model, etc
    int32  legsAnim;    // mask off ANIM_TOGGLEBIT
    int32  torsoAnim;   // mask off ANIM_TOGGLEBIT
} bot_entitystate_t;

// bot AI library exported functions
typedef struct botlib_import_s
{
    // print messages from the bot library
    void(QDECL* Print)(int32 type, const char* fmt, ...);
    // trace a bbox through the world
    void (*Trace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int32 passent, int32 contentmask);
    // trace a bbox against a specific entity
    void (*EntityTrace)(bsp_trace_t* trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int32 entnum, int32 contentmask);
    // retrieve the contents at the given point
    int32 (*PointContents)(vec3_t point);
    // retrieve the BSP entity data lump
    char* (*BSPEntityData)();
    //
    void (*BSPModelMinsMaxsOrigin)(int32 modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
    // send a bot client command
    void (*BotClientCommand)(int32 client, char* command);
    // memory allocation
    void* (*GetMemory)(int32 size); // allocate from Zone
    void  (*FreeMemory)(void* ptr); // free memory from Zone
    void* (*HunkAlloc)(int32 size); // allocate from hunk
    // file system access
    int32 (*FS_FOpenFile)(const char* qpath, fileHandle_t* file, fsMode_t mode);
    int32 (*FS_Read)(void* buffer, int32 len, fileHandle_t f);
    int32 (*FS_Write)(const void* buffer, int32 len, fileHandle_t f);
    void  (*FS_FCloseFile)(fileHandle_t f);
    int32 (*FS_Seek)(fileHandle_t f, int32 offset, int32 origin);
    // debug visualisation stuff
    int32 (*DebugLineCreate)();
    void  (*DebugLineDelete)(int32 line);
    void  (*DebugLineShow)(int32 line, vec3_t start, vec3_t end, int32 color);
    //
    int32 (*DebugPolygonCreate)(int32 color, int32 numPoints, vec3_t* points);
    void  (*DebugPolygonDelete)(int32 id);
} botlib_import_t;

typedef struct aas_export_s
{
    //-----------------------------------
    // be_aas_entity.h
    //-----------------------------------
    void (*AAS_EntityInfo)(int32 entnum, struct aas_entityinfo_s* info);
    //-----------------------------------
    // be_aas_main.h
    //-----------------------------------
    int32 (*AAS_Initialized)();
    void  (*AAS_PresenceTypeBoundingBox)(int32 presencetype, vec3_t mins, vec3_t maxs);
    float (*AAS_Time)();
    //--------------------------------------------
    // be_aas_sample.c
    //--------------------------------------------
    int32 (*AAS_PointAreaNum)(vec3_t point);
    int32 (*AAS_PointReachabilityAreaIndex)(vec3_t point);
    int32 (*AAS_TraceAreas)(vec3_t start, vec3_t end, int32* areas, vec3_t* points, int32 maxareas);
    int32 (*AAS_BBoxAreas)(vec3_t absmins, vec3_t absmaxs, int32* areas, int32 maxareas);
    int32 (*AAS_AreaInfo)(int32 areanum, struct aas_areainfo_s* info);
    //--------------------------------------------
    // be_aas_bspq3.c
    //--------------------------------------------
    int32 (*AAS_PointContents)(vec3_t point);
    int32 (*AAS_NextBSPEntity)(int32 ent);
    int32 (*AAS_ValueForBSPEpairKey)(int32 ent, const char* key, char* value, int32 size);
    int32 (*AAS_VectorForBSPEpairKey)(int32 ent, const char* key, vec3_t v);
    int32 (*AAS_FloatForBSPEpairKey)(int32 ent, const char* key, float* value);
    int32 (*AAS_IntForBSPEpairKey)(int32 ent, const char* key, int32* value);
    //--------------------------------------------
    // be_aas_reach.c
    //--------------------------------------------
    int32 (*AAS_AreaReachability)(int32 areanum);
    //--------------------------------------------
    // be_aas_route.c
    //--------------------------------------------
    int32 (*AAS_AreaTravelTimeToGoalArea)(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags);
    int32 (*AAS_EnableRoutingArea)(int32 areanum, int32 enable);
    int32 (*AAS_PredictRoute)(struct aas_predictroute_s* route, int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32 maxareas, int32 maxtime, int32 stopevent, int32 stopcontents, int32 stoptfl, int32 stopareanum);
    //--------------------------------------------
    // be_aas_altroute.c
    //--------------------------------------------
    int32 (*AAS_AlternativeRouteGoals)(vec3_t start, int32 startareanum, vec3_t goal, int32 goalareanum, int32 travelflags, struct aas_altroutegoal_s* altroutegoals, int32 maxaltroutegoals, int32 type);
    //--------------------------------------------
    // be_aas_move.c
    //--------------------------------------------
    int32 (*AAS_Swimming)(vec3_t origin);
    int32 (*AAS_PredictClientMovement)(struct aas_clientmove_s* move, int32 entnum, vec3_t origin, int32 presencetype, int32 onground, vec3_t velocity, vec3_t cmdmove, int32 cmdframes, int32 maxframes, float frametime, int32 stopevent, int32 stopareanum, int32 visualize);
} aas_export_t;

typedef struct ea_export_s
{
    // ClientCommand elementary actions
    void (*EA_Command)(int32 client, char* command);
    void (*EA_Say)(int32 client, char* str);
    void (*EA_SayTeam)(int32 client, char* str);
    //
    void (*EA_Action)(int32 client, int32 action);
    void (*EA_Gesture)(int32 client);
    void (*EA_Talk)(int32 client);
    void (*EA_Attack)(int32 client);
    void (*EA_Use)(int32 client);
    void (*EA_Respawn)(int32 client);
    void (*EA_MoveUp)(int32 client);
    void (*EA_MoveDown)(int32 client);
    void (*EA_MoveForward)(int32 client);
    void (*EA_MoveBack)(int32 client);
    void (*EA_MoveLeft)(int32 client);
    void (*EA_MoveRight)(int32 client);
    void (*EA_Crouch)(int32 client);

    void (*EA_SelectWeapon)(int32 client, int32 weapon);
    void (*EA_Jump)(int32 client);
    void (*EA_DelayedJump)(int32 client);
    void (*EA_Move)(int32 client, vec3_t dir, float speed);
    void (*EA_View)(int32 client, vec3_t viewangles);
    // send regular input to the server
    void (*EA_EndRegular)(int32 client, float thinktime);
    void (*EA_GetInput)(int32 client, float thinktime, bot_input_t* input);
    void (*EA_ResetInput)(int32 client);
} ea_export_t;

typedef struct ai_export_s
{
    //-----------------------------------
    // be_ai_char.h
    //-----------------------------------
    int32 (*BotLoadCharacter)(char* charfile, float skill);
    void  (*BotFreeCharacter)(int32 character);
    float (*Characteristic_Float)(int32 character, int32 index);
    float (*Characteristic_BFloat)(int32 character, int32 index, float min, float max);
    int32 (*Characteristic_Integer)(int32 character, int32 index);
    int32 (*Characteristic_BInteger)(int32 character, int32 index, int32 min, int32 max);
    void  (*Characteristic_String)(int32 character, int32 index, char* buf, int32 size);
    //-----------------------------------
    // be_ai_chat.h
    //-----------------------------------
    int32 (*BotAllocChatState)();
    void  (*BotFreeChatState)(int32 handle);
    void  (*BotQueueConsoleMessage)(int32 chatstate, int32 type, char* message);
    void  (*BotRemoveConsoleMessage)(int32 chatstate, int32 handle);
    int32 (*BotNextConsoleMessage)(int32 chatstate, struct bot_consolemessage_s* cm);
    int32 (*BotNumConsoleMessages)(int32 chatstate);
    void  (*BotInitialChat)(int32 chatstate, char* type, int32 mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
    int32 (*BotNumInitialChats)(int32 chatstate, char* type);
    int32 (*BotReplyChat)(int32 chatstate, char* message, int32 mcontext, int32 vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
    int32 (*BotChatLength)(int32 chatstate);
    void  (*BotEnterChat)(int32 chatstate, int32 client, int32 sendto);
    void  (*BotGetChatMessage)(int32 chatstate, char* buf, int32 size);
    int32 (*StringContains)(char* str1, char* str2, int32 casesensitive);
    int32 (*BotFindMatch)(char* str, struct bot_match_s* match, uint32 context);
    void  (*BotMatchVariable)(struct bot_match_s* match, int32 variable, char* buf, int32 size);
    void  (*UnifyWhiteSpaces)(char* string);
    void  (*BotReplaceSynonyms)(char* string, uint32 context);
    int32 (*BotLoadChatFile)(int32 chatstate, char* chatfile, char* chatname);
    void  (*BotSetChatGender)(int32 chatstate, int32 gender);
    void  (*BotSetChatName)(int32 chatstate, char* name, int32 client);
    //-----------------------------------
    // be_ai_goal.h
    //-----------------------------------
    void  (*BotResetGoalState)(int32 goalstate);
    void  (*BotResetAvoidGoals)(int32 goalstate);
    void  (*BotRemoveFromAvoidGoals)(int32 goalstate, int32 number);
    void  (*BotPushGoal)(int32 goalstate, struct bot_goal_s* goal);
    void  (*BotPopGoal)(int32 goalstate);
    void  (*BotEmptyGoalStack)(int32 goalstate);
    void  (*BotDumpAvoidGoals)(int32 goalstate);
    void  (*BotDumpGoalStack)(int32 goalstate);
    void  (*BotGoalName)(int32 number, char* name, int32 size);
    int32 (*BotGetTopGoal)(int32 goalstate, struct bot_goal_s* goal);
    int32 (*BotGetSecondGoal)(int32 goalstate, struct bot_goal_s* goal);
    int32 (*BotChooseLTGItem)(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags);
    int32 (*BotChooseNBGItem)(int32 goalstate, vec3_t origin, int32* inventory, int32 travelflags, struct bot_goal_s* ltg, float maxtime);
    int32 (*BotTouchingGoal)(vec3_t origin, struct bot_goal_s* goal);
    int32 (*BotItemGoalInVisButNotVisible)(int32 viewer, vec3_t eye, vec3_t viewangles, struct bot_goal_s* goal);
    int32 (*BotGetLevelItemGoal)(int32 index, char* classname, struct bot_goal_s* goal);
    int32 (*BotGetNextCampSpotGoal)(int32 num, struct bot_goal_s* goal);
    int32 (*BotGetMapLocationGoal)(char* name, struct bot_goal_s* goal);
    float (*BotAvoidGoalTime)(int32 goalstate, int32 number);
    void  (*BotSetAvoidGoalTime)(int32 goalstate, int32 number, float avoidtime);
    void  (*BotInitLevelItems)();
    void  (*BotUpdateEntityItems)();
    int32 (*BotLoadItemWeights)(int32 goalstate, char* filename);
    void  (*BotFreeItemWeights)(int32 goalstate);
    void  (*BotInterbreedGoalFuzzyLogic)(int32 parent1, int32 parent2, int32 child);
    void  (*BotSaveGoalFuzzyLogic)(int32 goalstate, char* filename);
    void  (*BotMutateGoalFuzzyLogic)(int32 goalstate, float range);
    int32 (*BotAllocGoalState)(int32 client);
    void  (*BotFreeGoalState)(int32 handle);
    //-----------------------------------
    // be_ai_move.h
    //-----------------------------------
    void  (*BotResetMoveState)(int32 movestate);
    void  (*BotMoveToGoal)(struct bot_moveresult_s* result, int32 movestate, struct bot_goal_s* goal, int32 travelflags);
    int32 (*BotMoveInDirection)(int32 movestate, vec3_t dir, float speed, int32 type);
    void  (*BotResetAvoidReach)(int32 movestate);
    void  (*BotResetLastAvoidReach)(int32 movestate);
    int32 (*BotReachabilityArea)(vec3_t origin, int32 testground);
    int32 (*BotMovementViewTarget)(int32 movestate, struct bot_goal_s* goal, int32 travelflags, float lookahead, vec3_t target);
    int32 (*BotPredictVisiblePosition)(vec3_t origin, int32 areanum, struct bot_goal_s* goal, int32 travelflags, vec3_t target);
    int32 (*BotAllocMoveState)();
    void  (*BotFreeMoveState)(int32 handle);
    void  (*BotInitMoveState)(int32 handle, struct bot_initmove_s* initmove);
    void  (*BotAddAvoidSpot)(int32 movestate, vec3_t origin, float radius, int32 type);
    //-----------------------------------
    // be_ai_weap.h
    //-----------------------------------
    int32 (*BotChooseBestFightWeapon)(int32 weaponstate, int32* inventory);
    void  (*BotGetWeaponInfo)(int32 weaponstate, int32 weapon, struct weaponinfo_s* weaponinfo);
    int32 (*BotLoadWeaponWeights)(int32 weaponstate, char* filename);
    int32 (*BotAllocWeaponState)();
    void  (*BotFreeWeaponState)(int32 weaponstate);
    void  (*BotResetWeaponState)(int32 weaponstate);
    //-----------------------------------
    // be_ai_gen.h
    //-----------------------------------
    int32 (*GeneticParentsAndChildSelection)(int32 numranks, float* ranks, int32* parent1, int32* parent2, int32* child);
} ai_export_t;

// bot AI library imported functions
typedef struct botlib_export_s
{
    // Area Awareness System functions
    aas_export_t aas;
    // Elementary Action functions
    ea_export_t ea;
    // AI functions
    ai_export_t ai;
    // setup the bot library, returns BLERR_
    int32 (*BotLibSetup)();
    // shutdown the bot library, returns BLERR_
    int32 (*BotLibShutdown)();
    // sets a library variable returns BLERR_
    int32 (*BotLibVarSet)(const char* var_name, const char* value);

    // start a frame in the bot library
    int32 (*BotLibStartFrame)(float time);
    // load a new map in the bot library
    int32 (*BotLibLoadMap)(const char* mapname);
    // entity updates
    int32 (*BotLibUpdateEntity)(int32 ent, bot_entitystate_t* state);
    // just for testing
    int32 (*Test)(int32 parm0, char* parm1, vec3_t parm2, vec3_t parm3);
} botlib_export_t;

// linking of bot library
botlib_export_t* GetBotLibAPI(int32 apiVersion, botlib_import_t* import);

/* Library variables:

name:                        default:            module(s):            description:

"basedir"                    ""                    l_utils.c            base directory
"gamedir"                    ""                    l_utils.c            game directory
"cddir"                        ""                    l_utils.c            CD directory

"log"                        "0"                    l_log.c                enable/disable creating a log file
"maxclients"                "4"                    be_interface.c        maximum number of clients
"maxentities"                "1024"                be_interface.c        maximum number of entities
"bot_developer"                "0"                    be_interface.c        bot developer mode

"phys_friction"                "6"                    be_aas_move.c        ground friction
"phys_stopspeed"            "100"                be_aas_move.c        stop speed
"phys_gravity"                "800"                be_aas_move.c        gravity value
"phys_waterfriction"        "1"                    be_aas_move.c        water friction
"phys_watergravity"            "400"                be_aas_move.c        gravity in water
"phys_maxvelocity"            "320"                be_aas_move.c        maximum velocity
"phys_maxwalkvelocity"        "320"                be_aas_move.c        maximum walk velocity
"phys_maxcrouchvelocity"    "100"                be_aas_move.c        maximum crouch velocity
"phys_maxswimvelocity"        "150"                be_aas_move.c        maximum swim velocity
"phys_walkaccelerate"        "10"                be_aas_move.c        walk acceleration
"phys_airaccelerate"        "1"                    be_aas_move.c        air acceleration
"phys_swimaccelerate"        "4"                    be_aas_move.c        swim acceleration
"phys_maxstep"                "18"                be_aas_move.c        maximum step height
"phys_maxsteepness"            "0.7"                be_aas_move.c        maximum floor steepness
"phys_maxbarrier"            "32"                be_aas_move.c        maximum barrier height
"phys_maxwaterjump"            "19"                be_aas_move.c        maximum waterjump height
"phys_jumpvel"                "270"                be_aas_move.c        jump z velocity
"phys_falldelta5"            "40"                be_aas_move.c
"phys_falldelta10"            "60"                be_aas_move.c
"rs_waterjump"                "400"                be_aas_move.c
"rs_teleport"                "50"                be_aas_move.c
"rs_barrierjump"            "100"                be_aas_move.c
"rs_startcrouch"            "300"                be_aas_move.c
"rs_startgrapple"            "500"                be_aas_move.c
"rs_startwalkoffledge"        "70"                be_aas_move.c
"rs_startjump"                "300"                be_aas_move.c
"rs_rocketjump"                "500"                be_aas_move.c
"rs_bfgjump"                "500"                be_aas_move.c
"rs_jumppad"                "250"                be_aas_move.c
"rs_aircontrolledjumppad"    "300"                be_aas_move.c
"rs_funcbob"                "300"                be_aas_move.c
"rs_startelevator"            "50"                be_aas_move.c
"rs_falldamage5"            "300"                be_aas_move.c
"rs_falldamage10"            "500"                be_aas_move.c
"rs_maxjumpfallheight"        "450"                be_aas_move.c

"max_aaslinks"                "4096"                be_aas_sample.c        maximum links in the AAS
"max_routingcache"            "4096"                be_aas_route.c        maximum routing cache size in KB
"forceclustering"            "0"                    be_aas_main.c        force recalculation of clusters
"forcereachability"            "0"                    be_aas_main.c        force recalculation of reachabilities
"forcewrite"                "0"                    be_aas_main.c        force writing of aas file
"aasoptimize"                "0"                    be_aas_main.c        enable aas optimization
"sv_mapChecksum"            "0"                    be_aas_main.c        BSP file checksum
"bot_visualizejumppads"        "0"                    be_aas_reach.c        visualize jump pads

"bot_reloadcharacters"        "0"                    -                    reload bot character files
"ai_gametype"                "0"                    be_ai_goal.c        game type
"droppedweight"                "1000"                be_ai_goal.c        additional dropped item weight
"weapindex_rocketlauncher"    "5"                    be_ai_move.c        rl weapon index for rocket jumping
"weapindex_bfg10k"            "9"                    be_ai_move.c        bfg weapon index for bfg jumping
"weapindex_grapple"            "10"                be_ai_move.c        grapple weapon index for grappling
"entitytypemissile"            "3"                    be_ai_move.c        ET_MISSILE
"offhandgrapple"            "0"                    be_ai_move.c        enable off hand grapple hook
"cmd_grappleon"                "grappleon"            be_ai_move.c        command to activate off hand grapple
"cmd_grappleoff"            "grappleoff"        be_ai_move.c        command to deactivate off hand grapple
"itemconfig"                "items.c"            be_ai_goal.c        item configuration file
"weaponconfig"                "weapons.c"            be_ai_weap.c        weapon configuration file
"synfile"                    "syn.c"                be_ai_chat.c        file with synonyms
"rndfile"                    "rnd.c"                be_ai_chat.c        file with random strings
"matchfile"                    "match.c"            be_ai_chat.c        file with match strings
"nochat"                    "0"                    be_ai_chat.c        disable chats
"max_messages"                "1024"                be_ai_chat.c        console message heap size
"max_weaponinfo"            "32"                be_ai_weap.c        maximum number of weapon info
"max_projectileinfo"        "32"                be_ai_weap.c        maximum number of projectile info
"max_iteminfo"                "256"                be_ai_goal.c        maximum number of item info
"max_levelitems"            "256"                be_ai_goal.c        maximum number of level items

*/

