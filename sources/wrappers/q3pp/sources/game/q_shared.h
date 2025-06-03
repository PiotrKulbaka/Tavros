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

#define Q3_VERSION   "Q3++ 1.33"

#define MAX_TEAMNAME 32

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>


#include <tavros/core/prelude.hpp>
#include <tavros/core/math.hpp>

#define QDECL __cdecl

static int16 BigShort(int16 l)
{
    return ((l & 0xff) << 8) + ((l >> 8) & 0xff);
}

#define PATH_SEP '\\'


//=============================================================
typedef int32 qhandle_t;
typedef int32 sfxHandle_t;
typedef int32 fileHandle_t;
typedef int32 clipHandle_t;

// angle indexes
#define PITCH             0 // up / down
#define YAW               1 // left / right
#define ROLL              2 // fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS  1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 1024 // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS   1024 // max length of an individual token

#define MAX_INFO_STRING   1024
#define MAX_INFO_KEY      1024
#define MAX_INFO_VALUE    1024

#define BIG_INFO_STRING   8192 // used for system info key only
#define BIG_INFO_KEY      8192
#define BIG_INFO_VALUE    8192


#define MAX_QPATH         64 // max length of a quake game pathname
#ifdef PATH_MAX
    #define MAX_OSPATH PATH_MAX
#else
    #define MAX_OSPATH 256 // max length of a filesystem pathname
#endif

#define MAX_NAME_LENGTH 32 // max length of a client name

#define MAX_SAY_TEXT    150

// paramters for command buffer stuffing
typedef enum
{
    EXEC_NOW,    // don't return until completed, a VM should NEVER use this,
                 // because some commands might cause the VM to be unloaded...
    EXEC_INSERT, // insert at current position, but don't run yet
    EXEC_APPEND  // add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES 32 // bit vector of area visibility

// parameters to the main Error routine
typedef enum
{
    ERR_FATAL,            // exit the entire game with a popup window
    ERR_DROP,             // print to console and disconnect from game
    ERR_SERVERDISCONNECT, // don't kill server
    ERR_DISCONNECT        // client disconnected from the server
} errorParm_t;


// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH        3
#define PROP_SPACE_WIDTH      8
#define PROP_HEIGHT           27
#define PROP_SMALL_SIZE_SCALE 0.75

#define BLINK_DIVISOR         200
#define PULSE_DIVISOR         75

#define UI_LEFT               0x00000000 // default
#define UI_CENTER             0x00000001
#define UI_RIGHT              0x00000002
#define UI_FORMATMASK         0x00000007
#define UI_SMALLFONT          0x00000010
#define UI_BIGFONT            0x00000020 // default
#define UI_GIANTFONT          0x00000040
#define UI_DROPSHADOW         0x00000800
#define UI_BLINK              0x00001000
#define UI_INVERSE            0x00002000
#define UI_PULSE              0x00004000

#ifdef DEBUG
    #define HUNK_DEBUG
#endif

typedef enum
{
    h_high,
    h_low
} ha_pref;

#ifdef HUNK_DEBUG
    #define Hunk_Alloc(size, preference) Hunk_AllocDebug(size, preference, #size, __FILE__, __LINE__)
void* Hunk_AllocDebug(int32 size, ha_pref preference, const char* label, const char* file, int32 line);
#else
void* Hunk_Alloc(int32 size, ha_pref preference);
#endif

#define Snd_Memset Com_Memset

void Com_Memset(void* dest, const int32 val, const size_t count);
void Com_Memcpy(void* dest, const void* src, const size_t count);

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

#ifndef M_PI
    #define M_PI 3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH       640
#define SCREEN_HEIGHT      480

#define TINYCHAR_WIDTH     (SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT    (SMALLCHAR_HEIGHT / 2)

#define SMALLCHAR_WIDTH    8
#define SMALLCHAR_HEIGHT   16

#define BIGCHAR_WIDTH      16
#define BIGCHAR_HEIGHT     16

#define GIANTCHAR_WIDTH    32
#define GIANTCHAR_HEIGHT   48

#define Q_COLOR_ESCAPE     '^'
#define Q_IsColorString(p) (p && *(p) == Q_COLOR_ESCAPE && *((p) + 1) && *((p) + 1) != Q_COLOR_ESCAPE)

#define COLOR_BLACK        '0'
#define COLOR_RED          '1'
#define COLOR_GREEN        '2'
#define COLOR_YELLOW       '3'
#define COLOR_BLUE         '4'
#define COLOR_CYAN         '5'
#define COLOR_MAGENTA      '6'
#define COLOR_WHITE        '7'
#define ColorIndex(c)      (((c) - '0') & 7)

#define S_COLOR_RED        "^1"
#define S_COLOR_GREEN      "^2"
#define S_COLOR_YELLOW     "^3"
#define S_COLOR_BLUE       "^4"
#define S_COLOR_CYAN       "^5"
#define S_COLOR_MAGENTA    "^6"
#define S_COLOR_WHITE      "^7"

extern vec4_t g_color_table[8];

#define DEG2RAD(a) (((a) * M_PI) / 180.0F)
#define RAD2DEG(a) (((a) * 180.0f) / M_PI)

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

float Q_fabs(float f);
float Q_rsqrt(float f); // reciprocal square root

int8 ClampChar(int32 i);

// this isn't a real cheap function to call!
int32 DirToByte(vec3_t dir);
void  ByteToDir(int32 b, vec3_t dir);

#define VectorClear(a)        ((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b)    ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define Vector4Copy(a, b)     ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

#define SnapVector(v)            \
    {                            \
        v[0] = ((int32) (v[0])); \
        v[1] = ((int32) (v[1])); \
        v[2] = ((int32) (v[2])); \
    }
// just in case you do't want to use the macros
vec_t DotProduct(const vec3_t v1, const vec3_t v2);
void  VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out);
void  VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out);
void  VectorCopy(const vec3_t in, vec3_t out);
void  VectorScale(const vec3_t in, float scale, vec3_t out);
void  VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

uint32 ColorBytes4(float r, float g, float b, float a);

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void  ClearBounds(vec3_t mins, vec3_t maxs);
void  AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);

int32 VectorCompare(const vec3_t v1, const vec3_t v2);
vec_t VectorLength(const vec3_t v);
vec_t VectorLengthSquared(const vec3_t v);
vec_t Distance(const vec3_t p1, const vec3_t p2);
vec_t DistanceSquared(const vec3_t p1, const vec3_t p2);
void  VectorNormalizeFast(vec3_t v);
void  VectorInverse(vec3_t v);
void  CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);

vec_t VectorNormalize(vec3_t v); // returns vector length
vec_t VectorNormalize2(const vec3_t v, vec3_t out);
void  VectorRotate(vec3_t in, vec3_t matrix[3], vec3_t out);

float Q_acos(float c);

float Q_random(int32* seed);
float Q_crandom(int32* seed);

#define random()  ((rand() & 0x7fff) / ((float) 0x7fff))
#define crandom() (2.0 * (random() - 0.5))

void vectoangles(const vec3_t value1, vec3_t angles);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

int32 BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* plane);

float AngleMod(float a);
float LerpAngle(float from, float to, float frac);
float AngleSubtract(float a1, float a2);
void  AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);

float AngleNormalize360(float angle);
float AngleNormalize180(float angle);

bool PlaneFromPoints(tavros::math::vec4& plane, const vec3_t a, const vec3_t b, const vec3_t c);
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
void RotateAroundDirection(vec3_t axis[3], float yaw);
// perpendicular vector could be replaced by this

int32 PlaneTypeForNormal(vec3_t normal);

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, const vec3_t src);


//=============================================

float Com_Clamp(float min, float max, float value);

char* COM_SkipPath(char* pathname);
void  COM_StripExtension(const char* in, char* out);
void  COM_DefaultExtension(char* path, int32 maxSize, const char* extension);

char* COM_Parse(char** data_p);
char* COM_ParseExt(char** data_p, bool allowLineBreak);
int32 COM_Compress(char* data_p);

#define MAX_TOKENLENGTH 1024

#ifndef TT_STRING
    // token types
    #define TT_STRING      1 // string
    #define TT_LITERAL     2 // literal
    #define TT_NUMBER      3 // number
    #define TT_NAME        4 // name
    #define TT_PUNCTUATION 5 // punctuation
#endif

typedef struct pc_token_s
{
    int32 type;
    int32 subtype;
    int32 intvalue;
    float floatvalue;
    char  string[MAX_TOKENLENGTH];
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void SkipBracedSection(char** program);
void SkipRestOfLine(char** data);

void QDECL Com_sprintf(char* dest, int32 size, const char* fmt, ...);


// mode parm for FS_FOpenFile
typedef enum
{
    FS_READ,
    FS_WRITE,
    FS_APPEND,
    FS_APPEND_SYNC
} fsMode_t;

typedef enum
{
    FS_SEEK_CUR,
    FS_SEEK_END,
    FS_SEEK_SET
} fsOrigin_t;

//=============================================

int32 Q_isprint(int32 c);
int32 Q_islower(int32 c);
int32 Q_isupper(int32 c);
int32 Q_isalpha(int32 c);

// portable case insensitive compare
int32 Q_stricmp(const char* s1, const char* s2);
int32 Q_strncmp(const char* s1, const char* s2, int32 n);
int32 Q_stricmpn(const char* s1, const char* s2, int32 n);
char* Q_strlwr(char* s1);
char* Q_strupr(char* s1);
char* Q_strrchr(const char* string, int32 c);

// buffer size safe library replacements
void Q_strncpyz(char* dest, const char* src, int32 destsize);
void Q_strcat(char* dest, int32 size, const char* src);

// strlen that discounts Quake color sequences
int32 Q_PrintStrlen(const char* string);
// removes color sequences from string
char* Q_CleanStr(char* string);

//=============================================
char* QDECL va(const char* format, ...);

//=============================================

//
// key / value info strings
//
char* Info_ValueForKey(const char* s, const char* key);
void  Info_RemoveKey(char* s, const char* key);
void  Info_SetValueForKey(char* s, const char* key, const char* value);
void  Info_SetValueForKey_Big(char* s, const char* key, const char* value);
bool  Info_Validate(const char* s);
void  Info_NextPair(const char** s, char* key, char* value);

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error(int32 level, const char* error, ...);


/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_ARCHIVE      1    // set to cause it to be saved to vars.rc
                               // used for system variables, not for player
                               // specific configurations
#define CVAR_USERINFO     2    // sent to server on connect or change
#define CVAR_SERVERINFO   4    // sent in response to front end requests
#define CVAR_SYSTEMINFO   8    // these cvars will be duplicated on all clients
#define CVAR_INIT         16   // don't allow change from console at all,
                               // but can be set from the command line
#define CVAR_LATCH        32   // will only change when C code next does
                               // a Cvar_Get(), so it can't be changed
                               // without proper initialization.  modified
                               // will be set, even though the value hasn't
                               // changed yet
#define CVAR_ROM          64   // display only, cannot be set by user at all
#define CVAR_USER_CREATED 128  // created by a set command
#define CVAR_TEMP         256  // can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT        512  // can not be changed if cheats are disabled
#define CVAR_NORESTART    1024 // do not clear when a cvar_restart is issued

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
    char*          name;
    char*          string;
    char*          resetString;   // cvar_restart will reset to this value
    char*          latchedString; // for CVAR_LATCH vars
    int32          flags;
    bool           modified;      // set each time the cvar is changed
    float          value;         // atof( string )
    int32          integer;       // atoi( string )
    struct cvar_s* next;
    struct cvar_s* hashNext;
} cvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// SURFACE FLAGS

#define CONTENTS_SOLID         1 // an eye is never valid in a solid
#define CONTENTS_LAVA          8
#define CONTENTS_SLIME         16
#define CONTENTS_WATER         32
#define CONTENTS_FOG           64
#define CONTENTS_AREAPORTAL    0x8000
#define CONTENTS_PLAYERCLIP    0x10000
#define CONTENTS_MONSTERCLIP   0x20000
// bot specific contents types
#define CONTENTS_TELEPORTER    0x40000
#define CONTENTS_JUMPPAD       0x80000
#define CONTENTS_CLUSTERPORTAL 0x100000
#define CONTENTS_DONOTENTER    0x200000
#define CONTENTS_BOTCLIP       0x400000
#define CONTENTS_MOVER         0x800000
#define CONTENTS_ORIGIN        0x1000000  // removed before bsping an entity
#define CONTENTS_BODY          0x2000000  // should never be on a brush, only in game
#define CONTENTS_CORPSE        0x4000000
#define CONTENTS_DETAIL        0x8000000  // brushes not used for the bsp
#define CONTENTS_STRUCTURAL    0x10000000 // brushes used for the bsp
#define CONTENTS_TRANSLUCENT   0x20000000 // don't consume surface fragments inside
#define CONTENTS_TRIGGER       0x40000000
#define CONTENTS_NODROP        0x80000000 // don't leave bodies or items (death fog, lava)

#define SURF_NODAMAGE          0x1        // never give falling damage
#define SURF_SLICK             0x2        // effects game physics
#define SURF_SKY               0x4        // lighting from environment map
#define SURF_LADDER            0x8
#define SURF_NOIMPACT          0x10       // don't make missile explosions
#define SURF_NOMARKS           0x20       // don't leave missile marks
#define SURF_FLESH             0x40       // make flesh sounds and effects
#define SURF_NODRAW            0x80       // don't generate a drawsurface at all
#define SURF_HINT              0x100      // make a primary bsp splitter
#define SURF_SKIP              0x200      // completely ignore, allowing non-closed brushes
#define SURF_NOLIGHTMAP        0x400      // surface doesn't need a lightmap
#define SURF_POINTLIGHT        0x800      // generate lighting info at vertexes
#define SURF_METALSTEPS        0x1000     // clanking footsteps
#define SURF_NOSTEPS           0x2000     // no footstep sounds
#define SURF_NONSOLID          0x4000     // don't collide against curves with this set
#define SURF_LIGHTFILTER       0x8000     // act as a light filter during q3map -light
#define SURF_ALPHASHADOW       0x10000    // do per-pixel light shadow casting in q3map
#define SURF_NODLIGHT          0x20000    // don't dlight even if solid (solid lava, skies)
#define SURF_DUST              0x40000    // leave a dust trail when walking on this surface


// plane types are used to speed some tests
// 0-2 are axial planes
#define PLANE_X                0
#define PLANE_Y                1
#define PLANE_Z                2
#define PLANE_NON_AXIAL        3

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
    vec3_t normal;
    float  dist;
    uint8  type;     // for fast side tests: 0,1,2 = axial, 3 = nonaxial
    uint8  signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
    uint8  pad[2];
} cplane_t;


// a trace is returned when a box is swept through the world
typedef struct
{
    bool     allsolid;     // if true, plane is not valid
    bool     startsolid;   // if true, the initial point was in a solid area
    float    fraction;     // time completed, 1.0 = didn't hit anything
    vec3_t   endpos;       // final position
    cplane_t plane;        // surface normal at impact, transformed to world space
    int32    surfaceFlags; // surface hit
    int32    contents;     // contents on other side of surface hit
    int32    entityNum;    // entity the contacted sirface is a part of
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD


// markfragments are returned by CM_MarkFragments()
typedef struct
{
    int32 firstPoint;
    int32 numPoints;
} markFragment_t;


typedef struct
{
    vec3_t origin;
    vec3_t axis[3];
} orientation_t;

//=====================================================================


// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE 0x0001
#define KEYCATCH_UI      0x0002
#define KEYCATCH_MESSAGE 0x0004
#define KEYCATCH_CGAME   0x0008


// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum
{
    CHAN_AUTO,
    CHAN_LOCAL, // menu sounds, etc
    CHAN_WEAPON,
    CHAN_VOICE,
    CHAN_ITEM,
    CHAN_BODY,
    CHAN_LOCAL_SOUND, // chat messages, etc
    CHAN_ANNOUNCER    // announcer voices, etc
} soundChannel_t;


/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define ANGLE2SHORT(x)         ((int32) ((x) * 65536 / 360) & 65535)
#define SHORT2ANGLE(x)         ((x) * (360.0 / 65536))

#define SNAPFLAG_RATE_DELAYED  1
#define SNAPFLAG_NOT_ACTIVE    2 // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT   4 // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_CLIENTS            64 // absolute limit
#define MAX_LOCATIONS          64

#define GENTITYNUM_BITS        10 // don't need to send any more
#define MAX_GENTITIES          (1 << GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE         (MAX_GENTITIES - 1)
#define ENTITYNUM_WORLD        (MAX_GENTITIES - 2)
#define ENTITYNUM_MAX_NORMAL   (MAX_GENTITIES - 2)


#define MAX_MODELS             256 // these are sent over the net as 8 bits
#define MAX_SOUNDS             256 // so they cannot be blindly increased


#define MAX_CONFIGSTRINGS      1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO          0 // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO          1 // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS 2 // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS    16000
typedef struct
{
    int32 stringOffsets[MAX_CONFIGSTRINGS];
    char  stringData[MAX_GAMESTATE_CHARS];
    int32 dataCount;
} gameState_t;

//=========================================================

// bit field limits
#define MAX_STATS              16
#define MAX_PERSISTANT         16
#define MAX_POWERUPS           16
#define MAX_WEAPONS            16

#define MAX_PS_EVENTS          2

#define PS_PMOVEFRAMECOUNTBITS 6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s
{
    int32 commandTime; // cmd->serverTime of last executed command
    int32 pm_type;
    int32 bobCycle;    // for view bobbing and footstep generation
    int32 pm_flags;    // ducked, jump_held, etc
    int32 pm_time;

    vec3_t origin;
    vec3_t velocity;
    int32  weaponTime;
    int32  gravity;
    int32  speed;
    int32  delta_angles[3]; // add to command angles to get view direction
                            // changed by spawns, rotating objects, and teleporters

    int32 groundEntityNum;  // ENTITYNUM_NONE = in air

    int32 legsTimer;        // don't change low priority animations until this runs out
    int32 legsAnim;         // mask off ANIM_TOGGLEBIT

    int32 torsoTimer;       // don't change low priority animations until this runs out
    int32 torsoAnim;        // mask off ANIM_TOGGLEBIT

    int32 movementDir;      // a number 0 to 7 that represents the reletive angle
                            // of movement to the view angle (axial and diagonals)
                            // when at rest, the value will remain unchanged
                            // used to twist the legs during strafing

    vec3_t grapplePoint;    // location of grapple to pull towards if PMF_GRAPPLE_PULL

    int32 eFlags;           // copied to entityState_t->eFlags

    int32 eventSequence;    // pmove generated events
    int32 events[MAX_PS_EVENTS];
    int32 eventParms[MAX_PS_EVENTS];

    int32 externalEvent; // events set on player from another source
    int32 externalEventParm;
    int32 externalEventTime;

    int32 clientNum; // ranges from 0 to MAX_CLIENTS-1
    int32 weapon;    // copied to entityState_t->weapon
    int32 weaponstate;

    vec3_t viewangles; // for fixed views
    int32  viewheight;

    // damage feedback
    int32 damageEvent; // when it changes, latch the other parms
    int32 damageYaw;
    int32 damagePitch;
    int32 damageCount;

    int32 stats[MAX_STATS];
    int32 persistant[MAX_PERSISTANT]; // stats that aren't cleared on death
    int32 powerups[MAX_POWERUPS];     // level.time that the powerup runs out
    int32 ammo[MAX_WEAPONS];

    int32 generic1;
    int32 loopSound;
    int32 jumppad_ent; // jumppad entity hit this frame

    // not communicated over the net at all
    int32 ping;             // server to game info for scoreboard
    int32 pmove_framecount; // FIXME: don't transmit over the network
    int32 jumppad_frame;
    int32 entityEventSequence;
} playerState_t;


//====================================================================


//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK       1
#define BUTTON_TALK         2 // displays talk balloon and disables actions
#define BUTTON_USE_HOLDABLE 4
#define BUTTON_GESTURE      8
#define BUTTON_WALKING      16 // walking can't just be infered from MOVE_RUN
                               // because a key pressed late in the frame will
                               // only generate a small move value for that frame
                               // walking will use different animations and
                               // won't generate footsteps
#define BUTTON_AFFIRMATIVE  32
#define BUTTON_NEGATIVE     64

#define BUTTON_GETFLAG      128
#define BUTTON_GUARDBASE    256
#define BUTTON_PATROL       512
#define BUTTON_FOLLOWME     1024

#define BUTTON_ANY          2048 // any key whatsoever

#define MOVE_RUN            120  // if forwardmove or rightmove are >= MOVE_RUN,
                                 // then BUTTON_WALKING should be set

// usercmd_t is sent to the server each client frame
typedef struct
{
    int32 serverTime;
    int32 angles[3];
    int32 buttons;
    uint8 weapon; // weapon
    int8  forwardmove, rightmove, upmove;
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL 0xffffff

typedef enum
{
    TR_STATIONARY,
    TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
    TR_LINEAR,
    TR_LINEAR_STOP,
    TR_SINE, // value = base + sin( time / duration ) * delta
    TR_GRAVITY
} trType_t;

typedef struct
{
    trType_t trType;
    int32    trTime;
    int32    trDuration; // if non 0, trTime + trDuration = stop time
    vec3_t   trBase;
    vec3_t   trDelta;    // velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s
{
    int32 number; // entity index
    int32 eType;  // entityType_t
    int32 eFlags;

    trajectory_t pos;  // for calculating position
    trajectory_t apos; // for calculating angles

    int32 time;
    int32 time2;

    vec3_t origin;
    vec3_t origin2;

    vec3_t angles;
    vec3_t angles2;

    int32 otherEntityNum; // shotgun sources, etc
    int32 otherEntityNum2;

    int32 groundEntityNum; // -1 = in air

    int32 constantLight;   // r + (g<<8) + (b<<16) + (intensity<<24)
    int32 loopSound;       // constantly loop this sound

    int32 modelindex;
    int32 modelindex2;
    int32 clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
    int32 frame;

    int32 solid; // for client side prediction, trap_linkentity sets this properly

    int32 event; // impulse events -- muzzle flashes, footsteps, etc
    int32 eventParm;

    // for players
    int32 powerups;  // bit flags
    int32 weapon;    // determines weapon and flash model, etc
    int32 legsAnim;  // mask off ANIM_TOGGLEBIT
    int32 torsoAnim; // mask off ANIM_TOGGLEBIT

    int32 generic1;
} entityState_t;

typedef enum
{
    CA_UNINITIALIZED,
    CA_DISCONNECTED, // not talking to a server
    CA_AUTHORIZING,  // not used any more, was checking cd key
    CA_CONNECTING,   // sending request packets to the server
    CA_CHALLENGING,  // sending challenge packets to the server
    CA_CONNECTED,    // netchan_t established, getting gamestate
    CA_LOADING,      // only during cgame initialization, never during main loop
    CA_PRIMED,       // got gamestate, waiting for first frame
    CA_ACTIVE,       // game views should be displayed
} connstate_t;

// font support

#define GLYPH_START     0
#define GLYPH_END       255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND   127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct
{
    int32     height;      // number of scan lines
    int32     top;         // top of glyph in buffer
    int32     bottom;      // bottom of glyph in buffer
    int32     pitch;       // width for copying
    int32     xSkip;       // x adjustment
    int32     imageWidth;  // width of actual image
    int32     imageHeight; // height of actual image
    float     s;           // x offset in image where glyph starts
    float     t;           // y offset in image where glyph starts
    float     s2;
    float     t2;
    qhandle_t glyph; // handle to the shader with the glyph
    char      shaderName[32];
} glyphInfo_t;

typedef struct
{
    glyphInfo_t glyphs[GLYPHS_PER_FONT];
    float       glyphScale;
    char        name[MAX_QPATH];
} fontInfo_t;

#define Square(x) ((x) * (x))

// real time
//=============================================


typedef struct
{
    int32 tm_sec;   /* seconds after the minute - [0,59] */
    int32 tm_min;   /* minutes after the hour - [0,59] */
    int32 tm_hour;  /* hours since midnight - [0,23] */
    int32 tm_mday;  /* day of the month - [1,31] */
    int32 tm_mon;   /* months since January - [0,11] */
    int32 tm_year;  /* years since 1900 */
    int32 tm_wday;  /* days since Sunday - [0,6] */
    int32 tm_yday;  /* days since January 1 - [0,365] */
    int32 tm_isdst; /* daylight savings time flag */
} qtime_t;


// server browser sources
// TTimo: AS_MPLAYER is no longer used
#define AS_LOCAL     0
#define AS_MPLAYER   1
#define AS_GLOBAL    2
#define AS_FAVORITES 3

typedef enum _flag_status
{
    FLAG_ATBASE = 0,
    FLAG_TAKEN, // CTF
    FLAG_DROPPED
} flagStatus_t;


#define MAX_GLOBAL_SERVERS       4096
#define MAX_OTHER_SERVERS        128
#define MAX_PINGREQUESTS         32
#define MAX_SERVERSTATUSREQUESTS 16

#define SAY_ALL                  0
#define SAY_TEAM                 1
#define SAY_TELL                 2
