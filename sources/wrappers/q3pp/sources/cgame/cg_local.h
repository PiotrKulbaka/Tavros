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

#include "../game/q_shared.h"
#include "tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"

#include "../qcommon/qcommon.h"
#include "../client/client.h"


// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define POWERUP_BLINKS                 5

#define POWERUP_BLINK_TIME             1000
#define FADE_TIME                      200
#define PULSE_TIME                     200
#define DAMAGE_DEFLECT_TIME            100
#define DAMAGE_RETURN_TIME             400
#define DAMAGE_TIME                    500
#define LAND_DEFLECT_TIME              150
#define LAND_RETURN_TIME               300
#define STEP_TIME                      200
#define DUCK_TIME                      100
#define PAIN_TWITCH_TIME               200
#define WEAPON_SELECT_TIME             1400
#define ITEM_SCALEUP_TIME              1000
#define ZOOM_TIME                      150
#define ITEM_BLOB_TIME                 200
#define MUZZLE_FLASH_TIME              20
#define SINK_TIME                      1000 // time for fragments to sink into ground before going away
#define ATTACKER_HEAD_TIME             10000
#define REWARD_TIME                    3000

#define PULSE_SCALE                    1.5 // amount to scale up the icons when activating

#define MAX_STEP_CHANGE                32

#define MAX_VERTS_ON_POLY              10
#define MAX_MARK_POLYS                 256

#define STAT_MINUS                     10 // num frame for '-' stats digit

#define ICON_SIZE                      48
#define CHAR_WIDTH                     32
#define CHAR_HEIGHT                    48
#define TEXT_ICON_SPACE                4

#define TEAMCHAT_WIDTH                 80
#define TEAMCHAT_HEIGHT                8

// very large characters
#define GIANT_WIDTH                    32
#define GIANT_HEIGHT                   48

#define NUM_CROSSHAIRS                 10

#define TEAM_OVERLAY_MAXNAME_WIDTH     12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH 16

#define DEFAULT_MODEL                  "sarge"
#define DEFAULT_TEAM_MODEL             "sarge"
#define DEFAULT_TEAM_HEAD              "sarge"

#define DEFAULT_REDTEAM_NAME           "Stroggs"
#define DEFAULT_BLUETEAM_NAME          "Pagans"

typedef enum
{
    FOOTSTEP_NORMAL,
    FOOTSTEP_BOOT,
    FOOTSTEP_FLESH,
    FOOTSTEP_MECH,
    FOOTSTEP_ENERGY,
    FOOTSTEP_METAL,
    FOOTSTEP_SPLASH,

    FOOTSTEP_TOTAL
} footstep_t;

typedef enum
{
    IMPACTSOUND_DEFAULT,
    IMPACTSOUND_METAL,
    IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct
{
    int32 oldFrame;
    int32 oldFrameTime; // time when ->oldFrame was exactly on

    int32 frame;
    int32 frameTime; // time when ->frame will be exactly on

    float backlerp;

    float yawAngle;
    bool  yawing;
    float pitchAngle;
    bool  pitching;

    int32        animationNumber; // may include ANIM_TOGGLEBIT
    animation_t* animation;
    int32        animationTime;   // time when the first frame of the animation will be exact
} lerpFrame_t;


typedef struct
{
    lerpFrame_t legs, torso, flag;
    int32       painTime;
    int32       painDirection; // flip from 0 to 1
    int32       lightningFiring;

    // railgun trail spawning
    vec3_t railgunImpact;
    bool   railgunFlash;

    // machinegun spinning
    float barrelAngle;
    int32 barrelTime;
    bool  barrelSpinning;
} playerEntity_t;

//=================================================


// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s
{
    entityState_t currentState; // from cg.frame
    entityState_t nextState;    // from cg.nextFrame, if available
    bool          interpolate;  // true if next is valid to interpolate to
    bool          currentValid; // true if cg.frame holds this entity

    int32 muzzleFlashTime;      // move to playerEntity?
    int32 previousEvent;
    int32 teleportFlag;

    int32 trailTime; // so missile trails can handle dropped initial packets
    int32 dustTrailTime;
    int32 miscTime;

    int32 snapShotTime; // last time this entity was found in a snapshot

    playerEntity_t pe;

    int32  errorTime; // decay the error from this time
    vec3_t errorOrigin;
    vec3_t errorAngles;

    bool   extrapolated; // false if origin / angles is an interpolation
    vec3_t rawOrigin;
    vec3_t rawAngles;

    vec3_t beamEnd;

    // exact interpolated position of entity on this frame
    vec3_t lerpOrigin;
    vec3_t lerpAngles;
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s
{
    struct markPoly_s *prevMark, *nextMark;
    int32              time;
    qhandle_t          markShader;
    bool               alphaFade; // fade alpha instead of rgb
    float              color[4];
    poly_t             poly;
    polyVert_t         verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum
{
    LE_MARK,
    LE_EXPLOSION,
    LE_SPRITE_EXPLOSION,
    LE_FRAGMENT,
    LE_MOVE_SCALE_FADE,
    LE_FALL_SCALE_FADE,
    LE_FADE_RGB,
    LE_SCALE_FADE,
    LE_SCOREPLUM,
} leType_t;

typedef enum
{
    LEF_PUFF_DONT_SCALE = 0x0001, // do not scale size over time
    LEF_TUMBLE = 0x0002,          // tumble over time, used for ejecting shells
} leFlag_t;

typedef enum
{
    LEMT_NONE,
    LEMT_BURN,
    LEMT_BLOOD
} leMarkType_t; // fragment local entities can leave marks on walls

typedef enum
{
    LEBS_NONE,
    LEBS_BLOOD,
    LEBS_BRASS
} leBounceSoundType_t; // fragment local entities can make sounds on impacts

typedef struct localEntity_s
{
    struct localEntity_s *prev, *next;
    leType_t              leType;
    int32                 leFlags;

    int32 startTime;
    int32 endTime;
    int32 fadeInTime;

    float lifeRate; // 1.0 / (endTime - startTime)

    trajectory_t pos;
    trajectory_t angles;

    float bounceFactor; // 0.0 = no bounce, 1.0 = perfect

    float color[4];

    float radius;

    float  light;
    vec3_t lightColor;

    leMarkType_t        leMarkType; // mark to leave on fragment impact
    leBounceSoundType_t leBounceSoundType;

    refEntity_t refEntity;
} localEntity_t;

//======================================================================


typedef struct
{
    int32 client;
    int32 score;
    int32 ping;
    int32 time;
    int32 scoreFlags;
    int32 powerUps;
    int32 accuracy;
    int32 impressiveCount;
    int32 excellentCount;
    int32 guantletCount;
    int32 defendCount;
    int32 assistCount;
    int32 captures;
    bool  perfect;
    int32 team;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS 32

typedef struct
{
    bool infoValid;

    char   name[MAX_QPATH];
    team_t team;

    int32 botSkill; // 0 = not bot, 1-5 = bot

    vec3_t color1;
    vec3_t color2;

    int32 score;    // updated by score servercmds
    int32 location; // location index for team mode
    int32 health;   // you only get this info about your teammates
    int32 armor;
    int32 curWeapon;

    int32 handicap;
    int32 wins, losses; // in tourney mode

    int32 teamTask;     // task in teamplay (offence/defence)
    bool  teamLeader;   // true when this is a team leader

    int32 powerups;     // so can display quad/flag status

    int32 medkitUsageTime;
    int32 invulnerabilityStartTime;
    int32 invulnerabilityStopTime;

    int32 breathPuffTime;

    // when clientinfo is changed, the loading of models/skins/sounds
    // can be deferred until you are dead, to prevent hitches in
    // gameplay
    char modelName[MAX_QPATH];
    char skinName[MAX_QPATH];
    char headModelName[MAX_QPATH];
    char headSkinName[MAX_QPATH];
    char redTeam[MAX_TEAMNAME];
    char blueTeam[MAX_TEAMNAME];
    bool deferred;

    bool newAnims;         // true if using the new mission pack animations
    bool fixedlegs;        // true if legs yaw is always the same as torso yaw
    bool fixedtorso;       // true if torso never changes yaw

    vec3_t     headOffset; // move head in icon views
    footstep_t footsteps;
    gender_t   gender;     // from model

    qhandle_t legsModel;
    qhandle_t legsSkin;

    qhandle_t torsoModel;
    qhandle_t torsoSkin;

    qhandle_t headModel;
    qhandle_t headSkin;

    qhandle_t modelIcon;

    animation_t animations[MAX_TOTALANIMATIONS];

    sfxHandle_t sounds[MAX_CUSTOM_SOUNDS];
} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s
{
    bool     registered;
    gitem_t* item;

    qhandle_t handsModel; // the hands don't actually draw, they just position the weapon
    qhandle_t weaponModel;
    qhandle_t barrelModel;
    qhandle_t flashModel;

    vec3_t weaponMidpoint; // so it will rotate centered instead of by tag

    float       flashDlight;
    vec3_t      flashDlightColor;
    sfxHandle_t flashSound[4]; // fast firing weapons randomly choose

    qhandle_t weaponIcon;
    qhandle_t ammoIcon;

    qhandle_t ammoModel;

    qhandle_t   missileModel;
    sfxHandle_t missileSound;
    void        (*missileTrailFunc)(centity_t*, const struct weaponInfo_s* wi);
    float       missileDlight;
    vec3_t      missileDlightColor;
    int32       missileRenderfx;

    void (*ejectBrassFunc)(centity_t*);

    float trailRadius;
    float wiTrailTime;

    sfxHandle_t readySound;
    sfxHandle_t firingSound;
    bool        loopFireSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct
{
    bool      registered;
    qhandle_t models[MAX_ITEM_MODELS];
    qhandle_t icon;
} itemInfo_t;

#define MAX_SKULLTRAIL 10

typedef struct
{
    vec3_t positions[MAX_SKULLTRAIL];
    int32  numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK      10
#define MAX_SOUNDBUFFER      20

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS 16

typedef struct
{
    int32 clientFrame; // incremented each frame

    int32 clientNum;

    bool  demoPlayback;
    int32 deferredPlayerLoading;
    bool  loading;             // don't defer players at initial startup
    bool  intermissionStarted; // don't play voice rewards, because game will end shortly

    // there are only one or two snapshot_t that are relevent at a time
    int32 latestSnapshotNum;  // the number of snapshots the client system has received
    int32 latestSnapshotTime; // the time from latestSnapshotNum, so we don't need to read the snapshot yet

    snapshot_t* snap;         // cg.snap->serverTime <= cg.time
    snapshot_t* nextSnap;     // cg.nextSnap->serverTime > cg.time, or NULL
    snapshot_t  activeSnapshots[2];

    float frameInterpolation; // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

    bool thisFrameTeleport;
    bool nextFrameTeleport;

    int32 frametime;         // cg.time - cg.oldTime

    int32 time;              // this is the time value that the client
                             // is rendering at.
    int32 oldTime;           // time at last frame, used for missile trails and prediction checking

    int32 physicsTime;       // either cg.snap->time or cg.nextSnap->time

    int32 timelimitWarnings; // 5 min, 1 min, overtime
    int32 fraglimitWarnings;

    bool mapRestart;           // set on a map restart to set back the weapon

    bool renderingThirdPerson; // during deaths, chasecams, etc

    // prediction state
    bool          hyperspace; // true if prediction has hit a trigger_teleport
    playerState_t predictedPlayerState;
    centity_t     predictedPlayerEntity;
    bool          validPPS; // clear until the first call to CG_PredictPlayerState
    int32         predictedErrorTime;
    vec3_t        predictedError;

    int32 eventSequence;
    int32 predictableEvents[MAX_PREDICTED_EVENTS];

    float stepChange; // for stair up smoothing
    int32 stepTime;

    float duckChange; // for duck viewheight smoothing
    int32 duckTime;

    float landChange; // for landing hard
    int32 landTime;

    // input state sent to server
    int32 weaponSelect;

    // auto rotating items
    vec3_t autoAngles;
    vec3_t autoAxis[3];
    vec3_t autoAnglesFast;
    vec3_t autoAxisFast[3];

    // view rendering
    refdef_t refdef;
    vec3_t   refdefViewAngles; // will be converted to refdef.viewaxis

    // zoom key
    bool  zoomed;
    int32 zoomTime;
    float zoomSensitivity;

    // information screen text during loading
    char infoScreenText[MAX_STRING_CHARS];

    // scoreboard
    int32   scoresRequestTime;
    int32   numScores;
    int32   selectedScore;
    int32   teamScores[2];
    score_t scores[MAX_CLIENTS];
    bool    showScores;
    bool    scoreBoardShowing;
    int32   scoreFadeTime;
    char    killerName[MAX_NAME_LENGTH];
    char    spectatorList[MAX_STRING_CHARS]; // list of names
    int32   spectatorLen;                    // length of list
    float   spectatorWidth;                  // width in device units
    int32   spectatorTime;                   // next time to offset
    int32   spectatorPaintX;                 // current paint x
    int32   spectatorPaintX2;                // current paint x
    int32   spectatorOffset;                 // current offset from start
    int32   spectatorPaintLen;               // current offset from start

    // skull trails
    skulltrail_t skulltrails[MAX_CLIENTS];

    // centerprinting
    int32 centerPrintTime;
    int32 centerPrintCharWidth;
    int32 centerPrintY;
    char  centerPrint[1024];
    int32 centerPrintLines;

    // low ammo warning state
    int32 lowAmmoWarning; // 1 = low, 2 = empty

    // kill timers for carnage reward
    int32 lastKillTime;

    // crosshair client ID
    int32 crosshairClientNum;
    int32 crosshairClientTime;

    // powerup active flashing
    int32 powerupActive;
    int32 powerupTime;

    // attacking player
    int32 attackerTime;
    int32 voiceTime;

    // reward medals
    int32     rewardStack;
    int32     rewardTime;
    int32     rewardCount[MAX_REWARDSTACK];
    qhandle_t rewardShader[MAX_REWARDSTACK];
    qhandle_t rewardSound[MAX_REWARDSTACK];

    // sound buffer mainly for announcer sounds
    int32     soundBufferIn;
    int32     soundBufferOut;
    int32     soundTime;
    qhandle_t soundBuffer[MAX_SOUNDBUFFER];

    // for voice chat buffer
    int32 voiceChatTime;
    int32 voiceChatBufferIn;
    int32 voiceChatBufferOut;

    // warmup countdown
    int32 warmup;
    int32 warmupCount;

    //==========================

    int32 itemPickup;
    int32 itemPickupTime;
    int32 itemPickupBlendTime; // the pulse around the crosshair is timed seperately

    int32 weaponSelectTime;
    int32 weaponAnimation;
    int32 weaponAnimationTime;

    // blend blobs
    float damageTime;
    float damageX, damageY, damageValue;

    // status bar head
    float headYaw;
    float headEndPitch;
    float headEndYaw;
    int32 headEndTime;
    float headStartPitch;
    float headStartYaw;
    int32 headStartTime;

    // view movement
    float v_dmg_time;
    float v_dmg_pitch;
    float v_dmg_roll;

    vec3_t kick_angles; // weapon kicks
    vec3_t kick_origin;

    // temp working variables for player view
    float bobfracsin;
    int32 bobcycle;
    float xyspeed;
    int32 nextOrbitTime;

    // bool cameraMode;        // if rendering from a loaded camera


    // development tool
    refEntity_t testModelEntity;
    char        testModelName[MAX_QPATH];
    bool        testGun;

} cg_t;


// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t
typedef struct
{
    qhandle_t charsetShader;
    qhandle_t charsetProp;
    qhandle_t charsetPropGlow;
    qhandle_t charsetPropB;
    qhandle_t whiteShader;

    qhandle_t redCubeModel;
    qhandle_t blueCubeModel;
    qhandle_t redCubeIcon;
    qhandle_t blueCubeIcon;
    qhandle_t redFlagModel;
    qhandle_t blueFlagModel;
    qhandle_t neutralFlagModel;
    qhandle_t redFlagShader[3];
    qhandle_t blueFlagShader[3];
    qhandle_t flagShader[4];

    qhandle_t flagPoleModel;
    qhandle_t flagFlapModel;

    qhandle_t redFlagFlapSkin;
    qhandle_t blueFlagFlapSkin;
    qhandle_t neutralFlagFlapSkin;

    qhandle_t redFlagBaseModel;
    qhandle_t blueFlagBaseModel;
    qhandle_t neutralFlagBaseModel;

    qhandle_t armorModel;
    qhandle_t armorIcon;

    qhandle_t teamStatusBar;

    qhandle_t deferShader;

    // gib explosions
    qhandle_t gibAbdomen;
    qhandle_t gibArm;
    qhandle_t gibChest;
    qhandle_t gibFist;
    qhandle_t gibFoot;
    qhandle_t gibForearm;
    qhandle_t gibIntestine;
    qhandle_t gibLeg;
    qhandle_t gibSkull;
    qhandle_t gibBrain;

    qhandle_t smoke2;

    qhandle_t machinegunBrassModel;
    qhandle_t shotgunBrassModel;

    qhandle_t railRingsShader;
    qhandle_t railCoreShader;

    qhandle_t lightningShader;

    qhandle_t friendShader;

    qhandle_t balloonShader;
    qhandle_t connectionShader;

    qhandle_t selectShader;
    qhandle_t viewBloodShader;
    qhandle_t tracerShader;
    qhandle_t crosshairShader;
    qhandle_t lagometerShader;
    qhandle_t backTileShader;
    qhandle_t noammoShader;

    qhandle_t smokePuffShader;
    qhandle_t smokePuffRageProShader;
    qhandle_t shotgunSmokePuffShader;
    qhandle_t plasmaBallShader;
    qhandle_t waterBubbleShader;
    qhandle_t bloodTrailShader;

    qhandle_t numberShaders[11];

    qhandle_t shadowMarkShader;

    qhandle_t botSkillShaders[5];

    // wall mark shaders
    qhandle_t wakeMarkShader;
    qhandle_t bloodMarkShader;
    qhandle_t bulletMarkShader;
    qhandle_t burnMarkShader;
    qhandle_t holeMarkShader;
    qhandle_t energyMarkShader;

    // powerup shaders
    qhandle_t quadShader;
    qhandle_t redQuadShader;
    qhandle_t quadWeaponShader;
    qhandle_t invisShader;
    qhandle_t regenShader;
    qhandle_t battleSuitShader;
    qhandle_t battleWeaponShader;
    qhandle_t hastePuffShader;
    qhandle_t redKamikazeShader;
    qhandle_t blueKamikazeShader;

    // weapon effect models
    qhandle_t bulletFlashModel;
    qhandle_t ringFlashModel;
    qhandle_t dishFlashModel;
    qhandle_t lightningExplosionModel;

    // weapon effect shaders
    qhandle_t railExplosionShader;
    qhandle_t plasmaExplosionShader;
    qhandle_t bulletExplosionShader;
    qhandle_t rocketExplosionShader;
    qhandle_t grenadeExplosionShader;
    qhandle_t bfgExplosionShader;
    qhandle_t bloodExplosionShader;

    // special effects models
    qhandle_t teleportEffectModel;
    qhandle_t teleportEffectShader;

    qhandle_t invulnerabilityPowerupModel;

    // scoreboard headers
    qhandle_t scoreboardName;
    qhandle_t scoreboardPing;
    qhandle_t scoreboardScore;
    qhandle_t scoreboardTime;

    // medals shown during gameplay
    qhandle_t medalImpressive;
    qhandle_t medalExcellent;
    qhandle_t medalGauntlet;
    qhandle_t medalDefend;
    qhandle_t medalAssist;
    qhandle_t medalCapture;

    // sounds
    sfxHandle_t quadSound;
    sfxHandle_t tracerSound;
    sfxHandle_t selectSound;
    sfxHandle_t useNothingSound;
    sfxHandle_t wearOffSound;
    sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
    sfxHandle_t sfx_lghit1;
    sfxHandle_t sfx_lghit2;
    sfxHandle_t sfx_lghit3;
    sfxHandle_t sfx_ric1;
    sfxHandle_t sfx_ric2;
    sfxHandle_t sfx_ric3;
    sfxHandle_t sfx_railg;
    sfxHandle_t sfx_rockexp;
    sfxHandle_t sfx_plasmaexp;
    sfxHandle_t gibSound;
    sfxHandle_t gibBounce1Sound;
    sfxHandle_t gibBounce2Sound;
    sfxHandle_t gibBounce3Sound;
    sfxHandle_t teleInSound;
    sfxHandle_t teleOutSound;
    sfxHandle_t noAmmoSound;
    sfxHandle_t respawnSound;
    sfxHandle_t talkSound;
    sfxHandle_t landSound;
    sfxHandle_t fallSound;
    sfxHandle_t jumpPadSound;

    sfxHandle_t oneMinuteSound;
    sfxHandle_t fiveMinuteSound;
    sfxHandle_t suddenDeathSound;

    sfxHandle_t threeFragSound;
    sfxHandle_t twoFragSound;
    sfxHandle_t oneFragSound;

    sfxHandle_t hitSound;
    sfxHandle_t hitSoundHighArmor;
    sfxHandle_t hitSoundLowArmor;
    sfxHandle_t hitTeamSound;
    sfxHandle_t impressiveSound;
    sfxHandle_t excellentSound;
    sfxHandle_t deniedSound;
    sfxHandle_t humiliationSound;
    sfxHandle_t assistSound;
    sfxHandle_t defendSound;
    sfxHandle_t firstImpressiveSound;
    sfxHandle_t firstExcellentSound;
    sfxHandle_t firstHumiliationSound;

    sfxHandle_t takenLeadSound;
    sfxHandle_t tiedLeadSound;
    sfxHandle_t lostLeadSound;

    sfxHandle_t voteNow;
    sfxHandle_t votePassed;
    sfxHandle_t voteFailed;

    sfxHandle_t watrInSound;
    sfxHandle_t watrOutSound;
    sfxHandle_t watrUnSound;

    sfxHandle_t flightSound;
    sfxHandle_t medkitSound;

    sfxHandle_t weaponHoverSound;

    // teamplay sounds
    sfxHandle_t captureAwardSound;
    sfxHandle_t redScoredSound;
    sfxHandle_t blueScoredSound;
    sfxHandle_t redLeadsSound;
    sfxHandle_t blueLeadsSound;
    sfxHandle_t teamsTiedSound;

    sfxHandle_t captureYourTeamSound;
    sfxHandle_t captureOpponentSound;
    sfxHandle_t returnYourTeamSound;
    sfxHandle_t returnOpponentSound;
    sfxHandle_t takenYourTeamSound;
    sfxHandle_t takenOpponentSound;

    sfxHandle_t redFlagReturnedSound;
    sfxHandle_t blueFlagReturnedSound;
    sfxHandle_t neutralFlagReturnedSound;
    sfxHandle_t enemyTookYourFlagSound;
    sfxHandle_t enemyTookTheFlagSound;
    sfxHandle_t yourTeamTookEnemyFlagSound;
    sfxHandle_t yourTeamTookTheFlagSound;
    sfxHandle_t youHaveFlagSound;
    sfxHandle_t yourBaseIsUnderAttackSound;
    sfxHandle_t holyShitSound;

    // tournament sounds
    sfxHandle_t count3Sound;
    sfxHandle_t count2Sound;
    sfxHandle_t count1Sound;
    sfxHandle_t countFightSound;
    sfxHandle_t countPrepareSound;

    qhandle_t cursor;
    qhandle_t selectCursor;
    qhandle_t sizeCursor;

    sfxHandle_t regenSound;
    sfxHandle_t protectSound;
    sfxHandle_t n_healthSound;
    sfxHandle_t hgrenb1aSound;
    sfxHandle_t hgrenb2aSound;
    sfxHandle_t wstbimplSound;
    sfxHandle_t wstbimpmSound;
    sfxHandle_t wstbimpdSound;
    sfxHandle_t wstbactvSound;

} cgMedia_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct
{
    gameState_t gameState;    // gamestate from server
    glconfig_t  glconfig;     // rendering configuration
    float       screenXScale; // derived from glconfig
    float       screenYScale;
    float       screenXBias;

    int32 serverCommandSequence; // reliable command stream counter
    int32 processedSnapshotNum;  // the number of snapshots cgame has requested

    bool localServer;            // detected on startup by checking sv_running

    // parsed from serverinfo
    gametype_t gametype;
    int32      dmflags;
    int32      teamflags;
    int32      fraglimit;
    int32      capturelimit;
    int32      timelimit;
    int32      maxclients;
    char       mapname[MAX_QPATH];
    char       redTeam[MAX_QPATH];
    char       blueTeam[MAX_QPATH];

    int32 voteTime;
    int32 voteYes;
    int32 voteNo;
    bool  voteModified; // beep whenever changed
    char  voteString[MAX_STRING_TOKENS];

    int32 teamVoteTime[2];
    int32 teamVoteYes[2];
    int32 teamVoteNo[2];
    bool  teamVoteModified[2]; // beep whenever changed
    char  teamVoteString[2][MAX_STRING_TOKENS];

    int32 levelStartTime;

    int32 scores1, scores2;  // from configstrings
    int32 redflag, blueflag; // flag status from configstrings
    int32 flagStatus;

    bool newHud;

    //
    // locally derived information from gamestate
    //
    qhandle_t   gameModels[MAX_MODELS];
    sfxHandle_t gameSounds[MAX_SOUNDS];

    int32     numInlineModels;
    qhandle_t inlineDrawModel[MAX_MODELS];
    vec3_t    inlineModelMidpoints[MAX_MODELS];

    clientInfo_t clientinfo[MAX_CLIENTS];

    // teamchat width is *3 because of embedded color codes
    char  teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
    int32 teamChatMsgTimes[TEAMCHAT_HEIGHT];
    int32 teamChatPos;
    int32 teamLastChatPos;

    int32     cursorX;
    int32     cursorY;
    bool      eventHandling;
    bool      mouseCaptured;
    bool      sizingHud;
    void*     capturedItem;
    qhandle_t activeCursor;

    // orders
    int32 currentOrder;
    bool  orderPending;
    int32 orderTime;
    int32 currentVoiceClient;
    int32 acceptOrderTime;
    int32 acceptTask;
    int32 acceptLeader;
    char  acceptVoice[MAX_NAME_LENGTH];

    // media
    cgMedia_t media;

} cgs_t;

//==============================================================================

extern cgs_t        cgs;
extern cg_t         cg;
extern centity_t    cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t   cg_items[MAX_ITEMS];
extern markPoly_t   cg_markPolys[MAX_MARK_POLYS];

extern cvar_t* cg_runpitch;
extern cvar_t* cg_runroll;
extern cvar_t* cg_bobup;
extern cvar_t* cg_bobpitch;
extern cvar_t* cg_bobroll;
extern cvar_t* cg_swingSpeed;
extern cvar_t* cg_drawAmmoWarning;
extern cvar_t* cg_drawRewards;
extern cvar_t* cg_crosshairX;
extern cvar_t* cg_crosshairY;
extern cvar_t* cg_crosshairSize;
extern cvar_t* cg_crosshairHealth;
extern cvar_t* cg_drawStatus;
extern cvar_t* cg_draw2D;
extern cvar_t* cg_animSpeed;
extern cvar_t* cg_debugAnim;
extern cvar_t* cg_debugPosition;
extern cvar_t* cg_debugEvents;
extern cvar_t* cg_errorDecay;
extern cvar_t* cg_nopredict;
extern cvar_t* cg_gun_x;
extern cvar_t* cg_gun_y;
extern cvar_t* cg_gun_z;
extern cvar_t* cg_viewsize;
extern cvar_t* cg_fov;
extern cvar_t* cg_zoomFov;
extern cvar_t* cg_thirdPersonRange;
extern cvar_t* cg_thirdPersonAngle;
extern cvar_t* cg_thirdPerson;
extern cvar_t* cg_lagometer;
extern cvar_t* cg_synchronousClients;
extern cvar_t* cg_teamChatTime;
extern cvar_t* cg_teamChatHeight;
extern cvar_t* cg_stats;
extern cvar_t* cg_paused;
extern cvar_t* cg_predictItems;
extern cvar_t* cg_deferPlayers;
extern cvar_t* cg_drawFriend;
extern cvar_t* cg_teamChatsOnly;
extern cvar_t* cg_noVoiceChats;
extern cvar_t* cg_noVoiceText;
extern cvar_t* cg_scorePlum;
extern cvar_t* cg_smoothClients;
extern cvar_t* pmove_fixed;
extern cvar_t* pmove_msec;
extern cvar_t* cg_cameraOrbit;
extern cvar_t* cg_cameraOrbitDelay;
extern cvar_t* cg_timescaleFadeEnd;
extern cvar_t* cg_timescaleFadeSpeed;
extern cvar_t* cg_timescale;
extern cvar_t* cg_cameraMode;
extern cvar_t* cg_noTaunt;
extern cvar_t* cg_noProjectileTrail;
extern cvar_t* cg_trueLightning;

//
// cg_main.c
//
const char* CG_ConfigString(int32 index);
const char* CG_Argv(int32 arg);

void QDECL CG_Error(const char* msg, ...);

void CG_StartMusic();

void CG_BuildSpectatorString();


//
// cg_view.c
//
void CG_TestModel_f();
void CG_TestGun_f();
void CG_TestModelNextFrame_f();
void CG_TestModelPrevFrame_f();
void CG_TestModelNextSkin_f();
void CG_TestModelPrevSkin_f();
void CG_ZoomDown_f();
void CG_ZoomUp_f();
void CG_AddBufferedSound(sfxHandle_t sfx);


//
// cg_drawtools.c
//
void CG_AdjustFrom640(float* x, float* y, float* w, float* h);
void CG_FillRect(float x, float y, float width, float height, const float* color);
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void CG_DrawStringExt(int32 x, int32 y, const char* string, tavros::math::vec4 setColor, bool forceColor, bool shadow, int32 charWidth, int32 charHeight, int32 maxChars);
void CG_DrawBigString(int32 x, int32 y, const char* s, float alpha);
void CG_DrawBigStringColor(int32 x, int32 y, const char* s, tavros::math::vec4 color);
void CG_DrawSmallString(int32 x, int32 y, const char* s, float alpha);
void CG_DrawSmallStringColor(int32 x, int32 y, const char* s, tavros::math::vec4 color);

int32 CG_DrawStrlen(const char* str);

tavros::math::vec4 CG_FadeColor(int32 startMsec, int32 totalMsec);
void               CG_TileClear();
tavros::math::vec4 CG_ColorForHealth();
tavros::math::vec4 CG_GetColorForHealth(int32 health, int32 armor);

void UI_DrawProportionalString(int32 x, int32 y, const char* str, int32 style, tavros::math::vec4 color);


//
// cg_draw.c, cg_newDraw.c
//
extern int32 sortedTeamPlayers[TEAM_MAXOVERLAY];
extern int32 numSortedTeamPlayers;

void CG_AddLagometerFrameInfo();
void CG_AddLagometerSnapshotInfo(snapshot_t* snap);
void CG_CenterPrint(const char* str, int32 y, int32 charWidth);
void CG_DrawHead(float x, float y, float w, float h, int32 clientNum, vec3_t headAngles);
void CG_DrawActive();
void CG_DrawFlagModel(float x, float y, float w, float h, int32 team);
void CG_DrawTeamBackground(int32 x, int32 y, int32 w, int32 h, float alpha, int32 team);
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);

//
// cg_player.c
//
void        CG_Player(centity_t* cent);
void        CG_ResetPlayerEntity(centity_t* cent);
void        CG_AddRefEntityWithPowerups(refEntity_t* ent, entityState_t* state, int32 team);
void        CG_NewClientInfo(int32 clientNum);
sfxHandle_t CG_CustomSound(int32 clientNum, const char* soundName);

//
// cg_predict.c
//
void  CG_BuildSolidList();
int32 CG_PointContents(const vec3_t point, int32 passEntityNum);
void  CG_Trace(trace_t* result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int32 skipNumber, int32 mask);
void  CG_PredictPlayerState();
void  CG_LoadDeferredPlayers();


//
// cg_events.c
//
void        CG_CheckEvents(centity_t* cent);
const char* CG_PlaceString(int32 rank);
void        CG_EntityEvent(centity_t* cent, vec3_t position);
void        CG_PainEvent(centity_t* cent, int32 health);


//
// cg_ents.c
//
void CG_SetEntitySoundPosition(centity_t* cent);
void CG_AddPacketEntities();
void CG_Beam(centity_t* cent);
void CG_AdjustPositionForMover(const vec3_t in, int32 moverNum, int32 fromTime, int32 toTime, vec3_t out);

void CG_PositionEntityOnTag(refEntity_t* entity, const refEntity_t* parent, qhandle_t parentModel, const char* tagName);
void CG_PositionRotatedEntityOnTag(refEntity_t* entity, const refEntity_t* parent, qhandle_t parentModel, const char* tagName);


//
// cg_weapons.c
//
void CG_NextWeapon_f();
void CG_PrevWeapon_f();
void CG_Weapon_f();

void CG_RegisterWeapon(int32 weaponNum);
void CG_RegisterItemVisuals(int32 itemNum);

void CG_FireWeapon(centity_t* cent);
void CG_MissileHitWall(int32 weapon, int32 clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType);
void CG_MissileHitPlayer(int32 weapon, vec3_t origin, vec3_t dir, int32 entityNum);
void CG_ShotgunFire(entityState_t* es);
void CG_Bullet(vec3_t origin, int32 sourceEntityNum, vec3_t normal, bool flesh, int32 fleshEntityNum);

void CG_RailTrail(clientInfo_t* ci, vec3_t start, vec3_t end);
void CG_GrappleTrail(centity_t* ent, const weaponInfo_t* wi);
void CG_AddViewWeapon(playerState_t* ps);
void CG_AddPlayerWeapon(refEntity_t* parent, playerState_t* ps, centity_t* cent, int32 team);
void CG_DrawWeaponSelect();

void CG_OutOfAmmoChange(); // should this be in pmove?

//
// cg_marks.c
//
void CG_InitMarkPolys();
void CG_AddMarks();
void CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, bool alphaFade, float radius, bool temporary);

//
// cg_localents.c
//
void           CG_InitLocalEntities();
localEntity_t* CG_AllocLocalEntity();
void           CG_AddLocalEntities();

//
// cg_effects.c
//
localEntity_t* CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration, int32 startTime, int32 fadeInTime, int32 leFlags, qhandle_t hShader);
void           CG_BubbleTrail(vec3_t start, vec3_t end, float spacing);
void           CG_SpawnEffect(vec3_t org);

void CG_ScorePlum(int32 client, vec3_t org, int32 score);

void CG_GibPlayer(vec3_t playerOrigin);

void CG_Bleed(vec3_t origin, int32 entityNum);

localEntity_t* CG_MakeExplosion(vec3_t origin, vec3_t dir, qhandle_t hModel, qhandle_t shader, int32 msec, bool isSprite);

//
// cg_snapshot.c
//
void CG_ProcessSnapshots();

//
// cg_info.c
//
void CG_LoadingString(const char* s);
void CG_LoadingItem(int32 itemNum);
void CG_LoadingClient(int32 clientNum);
void CG_DrawInformation();

//
// cg_scoreboard.c
//
bool CG_DrawOldScoreboard();
void CG_DrawOldTourneyScoreboard();

//
// cg_consolecmds.c
//
void CG_InitConsoleCommands();

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands(int32 latestSequence);
void CG_ParseServerinfo();
void CG_SetConfigValues();
void CG_ShaderStateChanged();

//
// cg_playerstate.c
//
void CG_Respawn();
void CG_TransitionPlayerState(playerState_t* ps, playerState_t* ops);

//===============================================

void        CG_ClearParticles();
void        CG_AddParticles();
void        CG_ParticleExplosion(const char* animStr, vec3_t origin, vec3_t vel, int32 duration, int32 sizeStart, int32 sizeEnd);
extern bool initparticles;
