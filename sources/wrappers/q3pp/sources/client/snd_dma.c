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

#include "snd_local.h"
#include "client.h"

static tavros::core::logger logger("snd_dma");

void S_Play_f();
void S_SoundList_f();
void S_Music_f();

void S_Update_();
void S_StopAllSounds();
void S_UpdateBackgroundTrack();

static fileHandle_t s_backgroundFile;
static wavinfo_t    s_backgroundInfo;
// int32            s_nextWavChunk;
static int32 s_backgroundSamples;
static char  s_backgroundLoop[MAX_QPATH];
// static char        s_backgroundMusic[MAX_QPATH]; //TTimo: unused


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define SOUND_FULLVOLUME 80

#define SOUND_ATTENUATE  0.0008f

channel_t s_channels[MAX_CHANNELS];
channel_t loop_channels[MAX_CHANNELS];
int32     numLoopChannels;

static int32 s_soundStarted;
static bool  s_soundMuted;

dma_t dma;

static int32  listener_number;
static vec3_t listener_origin;
static vec3_t listener_axis[3];

int32 s_soundtime;   // sample PAIRS
int32 s_paintedtime; // sample PAIRS

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define MAX_SFX 4096
sfx_t s_knownSfx[MAX_SFX];
int32 s_numSfx = 0;

#define LOOP_HASH 128
static sfx_t* sfxHash[LOOP_HASH];

cvar_t* s_volume;
cvar_t* s_testsound;
cvar_t* s_khz;
cvar_t* s_show;
cvar_t* s_mixahead;
cvar_t* s_mixPreStep;
cvar_t* s_musicVolume;
cvar_t* s_separation;
cvar_t* s_doppler;

static loopSound_t loopSounds[MAX_GENTITIES];
static channel_t*  freelist = NULL;

int32                 s_rawend;
portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];


// ====================================================================
// User-setable variables
// ====================================================================


void S_SoundInfo_f()
{
    logger.info("----- Sound Info -----");
    if (!s_soundStarted) {
        logger.info("sound system not started");
    } else {
        if (s_soundMuted) {
            logger.info("sound system is muted");
        }

        logger.info("{:05d} stereo", dma.channels - 1);
        logger.info("{:05d} samples", dma.samples);
        logger.info("{:05d} samplebits", dma.samplebits);
        logger.info("{:05d} submission_chunk", dma.submission_chunk);
        logger.info("{:05d} speed", dma.speed);
        logger.info("{} dma buffer", (void*) dma.buffer);
        if (s_backgroundFile) {
            logger.info("Background file: {}", s_backgroundLoop);
        } else {
            logger.info("No background file.");
        }
    }
    logger.info("----------------------");
}


/*
================
S_Init
================
*/
void S_Init()
{
    cvar_t* cv;
    bool    r;

    logger.info("------- sound initialization -------");

    s_volume = Cvar_Get("s_volume", "0.8", CVAR_ARCHIVE);
    s_musicVolume = Cvar_Get("s_musicvolume", "0.25", CVAR_ARCHIVE);
    s_separation = Cvar_Get("s_separation", "0.5", CVAR_ARCHIVE);
    s_doppler = Cvar_Get("s_doppler", "1", CVAR_ARCHIVE);
    s_khz = Cvar_Get("s_khz", "22", CVAR_ARCHIVE);
    s_mixahead = Cvar_Get("s_mixahead", "0.2", CVAR_ARCHIVE);

    s_mixPreStep = Cvar_Get("s_mixPreStep", "0.05", CVAR_ARCHIVE);
    s_show = Cvar_Get("s_show", "0", CVAR_CHEAT);
    s_testsound = Cvar_Get("s_testsound", "0", CVAR_CHEAT);

    cv = Cvar_Get("s_initsound", "1", 0);
    if (!cv->integer) {
        logger.info("not initializing.");
        logger.info("------------------------------------");
        return;
    }

    Cmd_AddCommand("play", S_Play_f);
    Cmd_AddCommand("music", S_Music_f);
    Cmd_AddCommand("s_list", S_SoundList_f);
    Cmd_AddCommand("s_info", S_SoundInfo_f);
    Cmd_AddCommand("s_stop", S_StopAllSounds);

    r = SNDDMA_Init();
    logger.info("------------------------------------");

    if (r) {
        s_soundStarted = 1;
        s_soundMuted = 1;
        //        s_numSfx = 0;

        Com_Memset(sfxHash, 0, sizeof(sfx_t*) * LOOP_HASH);

        s_soundtime = 0;
        s_paintedtime = 0;

        S_StopAllSounds();

        S_SoundInfo_f();
    }
}


void S_ChannelFree(channel_t* v)
{
    v->thesfx = NULL;
    *(channel_t**) v = freelist;
    freelist = (channel_t*) v;
}

channel_t* S_ChannelMalloc()
{
    channel_t* v;
    if (freelist == NULL) {
        return NULL;
    }
    v = freelist;
    freelist = *(channel_t**) freelist;
    v->allocTime = Com_Milliseconds();
    return v;
}

void S_ChannelSetup()
{
    channel_t *p, *q;

    // clear all the sounds so they don't
    Com_Memset(s_channels, 0, sizeof(s_channels));

    p = s_channels;
    ;
    q = p + MAX_CHANNELS;
    while (--q > p) {
        *(channel_t**) q = q - 1;
    }

    *(channel_t**) q = NULL;
    freelist = p + MAX_CHANNELS - 1;
    logger.debug("Channel memory manager started");
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown()
{
    if (!s_soundStarted) {
        return;
    }

    SNDDMA_Shutdown();

    s_soundStarted = 0;

    Cmd_RemoveCommand("play");
    Cmd_RemoveCommand("music");
    Cmd_RemoveCommand("stopsound");
    Cmd_RemoveCommand("soundlist");
    Cmd_RemoveCommand("soundinfo");
}


// =======================================================================
// Load a sound
// =======================================================================

/*
================
return a hash value for the sfx name
================
*/
static int32 S_HashSFXName(const char* name)
{
    int32 i;
    int32 hash;
    char  letter;

    hash = 0;
    i = 0;
    while (name[i] != '\0') {
        letter = tolower(name[i]);
        if (letter == '.') {
            break; // don't include extension
        }
        if (letter == '\\') {
            letter = '/'; // damn path names
        }
        hash += (int32) (letter) * (i + 119);
        i++;
    }
    hash &= (LOOP_HASH - 1);
    return hash;
}

/*
==================
S_FindName

Will allocate a new sfx if it isn't found
==================
*/
static sfx_t* S_FindName(const char* name)
{
    int32 i;
    int32 hash;

    sfx_t* sfx;

    if (!name) {
        Com_Error(ERR_FATAL, "S_FindName: NULL\n");
    }
    if (!name[0]) {
        Com_Error(ERR_FATAL, "S_FindName: empty name\n");
    }

    if (strlen(name) >= MAX_QPATH) {
        Com_Error(ERR_FATAL, "Sound name too int32: %s", name);
    }

    hash = S_HashSFXName(name);

    sfx = sfxHash[hash];
    // see if already loaded
    while (sfx) {
        if (!Q_stricmp(sfx->soundName, name)) {
            return sfx;
        }
        sfx = sfx->next;
    }

    // find a free sfx
    for (i = 0; i < s_numSfx; i++) {
        if (!s_knownSfx[i].soundName[0]) {
            break;
        }
    }

    if (i == s_numSfx) {
        if (s_numSfx == MAX_SFX) {
            Com_Error(ERR_FATAL, "S_FindName: out of sfx_t");
        }
        s_numSfx++;
    }

    sfx = &s_knownSfx[i];
    Com_Memset(sfx, 0, sizeof(*sfx));
    strcpy(sfx->soundName, name);

    sfx->next = sfxHash[hash];
    sfxHash[hash] = sfx;

    return sfx;
}

/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_DisableSounds()
{
    S_StopAllSounds();
    s_soundMuted = true;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration()
{
    s_soundMuted = false; // we can play again

    if (s_numSfx == 0) {
        SND_setup();

        s_numSfx = 0;
        Com_Memset(s_knownSfx, 0, sizeof(s_knownSfx));
        Com_Memset(sfxHash, 0, sizeof(sfx_t*) * LOOP_HASH);

        S_RegisterSound("sound/feedback/hit.wav", false); // changed to a sound in baseq3
    }
}


/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t S_RegisterSound(const char* name, bool compressed)
{
    sfx_t* sfx;

    compressed = false;
    if (!s_soundStarted) {
        return 0;
    }

    if (strlen(name) >= MAX_QPATH) {
        logger.info("Sound name exceeds MAX_QPATH");
        return 0;
    }

    sfx = S_FindName(name);
    if (sfx->soundData) {
        if (sfx->defaultSound) {
            logger.warning("Could not find {} - using default", sfx->soundName);
            return 0;
        }
        return sfx - s_knownSfx;
    }

    sfx->inMemory = false;
    sfx->soundCompressed = compressed;

    S_memoryLoad(sfx);

    if (sfx->defaultSound) {
        logger.warning("Could not find {} - using default", sfx->soundName);
        return 0;
    }

    return sfx - s_knownSfx;
}

void S_memoryLoad(sfx_t* sfx)
{
    // load the sound file
    if (!S_LoadSound(sfx)) {
        sfx->defaultSound = true;
    }
    sfx->inMemory = true;
}

//=============================================================================

/*
=================
S_SpatializeOrigin

Used for spatializing s_channels
=================
*/
void S_SpatializeOrigin(vec3_t origin, int32 master_vol, int32* left_vol, int32* right_vol)
{
    vec_t  dot;
    vec_t  dist;
    vec_t  lscale, rscale, scale;
    vec3_t source_vec;
    vec3_t vec;

    const float dist_mult = SOUND_ATTENUATE;

    // calculate stereo seperation and distance attenuation
    VectorSubtract(origin, listener_origin, source_vec);

    dist = VectorNormalize(source_vec);
    dist -= SOUND_FULLVOLUME;
    if (dist < 0) {
        dist = 0;      // close enough to be at full volume
    }
    dist *= dist_mult; // different attenuation levels

    VectorRotate(source_vec, listener_axis, vec);

    dot = -vec[1];

    if (dma.channels == 1) { // no attenuation = no spatialization
        rscale = 1.0;
        lscale = 1.0;
    } else {
        rscale = 0.5 * (1.0 + dot);
        lscale = 0.5 * (1.0 - dot);
        // rscale = s_separation->value + ( 1.0 - s_separation->value ) * dot;
        // lscale = s_separation->value - ( 1.0 - s_separation->value ) * dot;
        if (rscale < 0) {
            rscale = 0;
        }
        if (lscale < 0) {
            lscale = 0;
        }
    }

    // add in distance effect
    scale = (1.0 - dist) * rscale;
    *right_vol = (master_vol * scale);
    if (*right_vol < 0) {
        *right_vol = 0;
    }

    scale = (1.0 - dist) * lscale;
    *left_vol = (master_vol * scale);
    if (*left_vol < 0) {
        *left_vol = 0;
    }
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(vec3_t origin, int32 entityNum, int32 entchannel, sfxHandle_t sfxHandle)
{
    channel_t* ch;
    sfx_t*     sfx;
    int32      i, oldest, chosen, time;
    int32      inplay, allowed;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    if (!origin && (entityNum < 0 || entityNum > MAX_GENTITIES)) {
        Com_Error(ERR_DROP, "S_StartSound: bad entitynum %i", entityNum);
    }

    if (sfxHandle < 0 || sfxHandle >= s_numSfx) {
        logger.info("S_StartSound: handle {} out of range", sfxHandle);
        return;
    }

    sfx = &s_knownSfx[sfxHandle];

    if (!sfx->inMemory) {
        S_memoryLoad(sfx);
    }

    if (s_show->integer == 1) {
        logger.info("{} : {}", s_paintedtime, sfx->soundName);
    }

    time = Com_Milliseconds();

    allowed = 4;
    if (entityNum == listener_number) {
        allowed = 8;
    }

    ch = s_channels;
    inplay = 0;
    for (i = 0; i < MAX_CHANNELS; i++, ch++) {
        if (ch[i].entnum == entityNum && ch[i].thesfx == sfx) {
            if (time - ch[i].allocTime < 50) {
                return;
            }
            inplay++;
        }
    }

    if (inplay > allowed) {
        return;
    }

    sfx->lastTimeUsed = time;

    ch = S_ChannelMalloc(); // entityNum, entchannel);
    if (!ch) {
        ch = s_channels;

        oldest = sfx->lastTimeUsed;
        chosen = -1;
        for (i = 0; i < MAX_CHANNELS; i++, ch++) {
            if (ch->entnum != listener_number && ch->entnum == entityNum && ch->allocTime < oldest && ch->entchannel != CHAN_ANNOUNCER) {
                oldest = ch->allocTime;
                chosen = i;
            }
        }
        if (chosen == -1) {
            ch = s_channels;
            for (i = 0; i < MAX_CHANNELS; i++, ch++) {
                if (ch->entnum != listener_number && ch->allocTime < oldest && ch->entchannel != CHAN_ANNOUNCER) {
                    oldest = ch->allocTime;
                    chosen = i;
                }
            }
            if (chosen == -1) {
                if (ch->entnum == listener_number) {
                    for (i = 0; i < MAX_CHANNELS; i++, ch++) {
                        if (ch->allocTime < oldest) {
                            oldest = ch->allocTime;
                            chosen = i;
                        }
                    }
                }
                if (chosen == -1) {
                    logger.info("dropping sound");
                    return;
                }
            }
        }
        ch = &s_channels[chosen];
        ch->allocTime = sfx->lastTimeUsed;
    }

    if (origin) {
        VectorCopy(origin, ch->origin);
        ch->fixed_origin = true;
    } else {
        ch->fixed_origin = false;
    }

    ch->master_vol = 127;
    ch->entnum = entityNum;
    ch->thesfx = sfx;
    ch->startSample = START_SAMPLE_IMMEDIATE;
    ch->entchannel = entchannel;
    ch->leftvol = ch->master_vol;  // these will get calced at next spatialize
    ch->rightvol = ch->master_vol; // unless the game isn't running
    ch->doppler = false;
}


/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound(sfxHandle_t sfxHandle, int32 channelNum)
{
    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    if (sfxHandle < 0 || sfxHandle >= s_numSfx) {
        logger.info("S_StartLocalSound: handle {} out of range", sfxHandle);
        return;
    }

    S_StartSound(NULL, listener_number, channelNum, sfxHandle);
}


/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_ClearSoundBuffer()
{
    int32 clear;

    if (!s_soundStarted) {
        return;
    }

    // stop looping sounds
    Com_Memset(loopSounds, 0, MAX_GENTITIES * sizeof(loopSound_t));
    Com_Memset(loop_channels, 0, MAX_CHANNELS * sizeof(channel_t));
    numLoopChannels = 0;

    S_ChannelSetup();

    s_rawend = 0;

    if (dma.samplebits == 8) {
        clear = 0x80;
    } else {
        clear = 0;
    }

    SNDDMA_BeginPainting();
    if (dma.buffer) {
        // TTimo: due to a particular bug workaround in linu_x sound code,
        //   have to optionally use a custom C implementation of Com_Memset
        //   not affecting win32, we have #define Snd_Memset Com_Memset
        Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits / 8);
    }
    SNDDMA_Submit();
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds()
{
    if (!s_soundStarted) {
        return;
    }

    // stop the background music
    S_StopBackgroundTrack();

    S_ClearSoundBuffer();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

void S_StopLoopingSound(int32 entityNum)
{
    loopSounds[entityNum].active = false;
    //    loopSounds[entityNum].sfx = 0;
    loopSounds[entityNum].kill = false;
}

/*
==================
S_ClearLoopingSounds

==================
*/
void S_ClearLoopingSounds(bool killall)
{
    int32 i;
    for (i = 0; i < MAX_GENTITIES; i++) {
        if (killall || loopSounds[i].kill || (loopSounds[i].sfx && loopSounds[i].sfx->soundLength == 0)) {
            loopSounds[i].kill = false;
            S_StopLoopingSound(i);
        }
    }
    numLoopChannels = 0;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddLoopingSound(int32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle)
{
    sfx_t* sfx;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    if (sfxHandle < 0 || sfxHandle >= s_numSfx) {
        logger.info("S_AddLoopingSound: handle {} out of range", sfxHandle);
        return;
    }

    sfx = &s_knownSfx[sfxHandle];

    if (!sfx->inMemory) {
        S_memoryLoad(sfx);
    }

    if (!sfx->soundLength) {
        Com_Error(ERR_DROP, "%s has length 0", sfx->soundName);
    }

    VectorCopy(origin, loopSounds[entityNum].origin);
    VectorCopy(velocity, loopSounds[entityNum].velocity);
    loopSounds[entityNum].active = true;
    loopSounds[entityNum].kill = true;
    loopSounds[entityNum].doppler = false;
    loopSounds[entityNum].oldDopplerScale = 1.0;
    loopSounds[entityNum].dopplerScale = 1.0;
    loopSounds[entityNum].sfx = sfx;

    if (s_doppler->integer && VectorLengthSquared(velocity) > 0.0) {
        vec3_t out;
        float  lena, lenb;

        loopSounds[entityNum].doppler = true;
        lena = DistanceSquared(loopSounds[listener_number].origin, loopSounds[entityNum].origin);
        VectorAdd(loopSounds[entityNum].origin, loopSounds[entityNum].velocity, out);
        lenb = DistanceSquared(loopSounds[listener_number].origin, out);
        if ((loopSounds[entityNum].framenum + 1) != cls.framecount) {
            loopSounds[entityNum].oldDopplerScale = 1.0;
        } else {
            loopSounds[entityNum].oldDopplerScale = loopSounds[entityNum].dopplerScale;
        }
        loopSounds[entityNum].dopplerScale = lenb / (lena * 100);
        if (loopSounds[entityNum].dopplerScale <= 1.0) {
            loopSounds[entityNum].doppler = false; // don't bother doing the math
        }
    }

    loopSounds[entityNum].framenum = cls.framecount;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddRealLoopingSound(int32 entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle)
{
    sfx_t* sfx;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    if (sfxHandle < 0 || sfxHandle >= s_numSfx) {
        logger.info("S_AddRealLoopingSound: handle {} out of range", sfxHandle);
        return;
    }

    sfx = &s_knownSfx[sfxHandle];

    if (!sfx->inMemory) {
        S_memoryLoad(sfx);
    }

    if (!sfx->soundLength) {
        Com_Error(ERR_DROP, "%s has length 0", sfx->soundName);
    }
    VectorCopy(origin, loopSounds[entityNum].origin);
    VectorCopy(velocity, loopSounds[entityNum].velocity);
    loopSounds[entityNum].sfx = sfx;
    loopSounds[entityNum].active = true;
    loopSounds[entityNum].kill = false;
    loopSounds[entityNum].doppler = false;
}


/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds()
{
    int32        i, j, time;
    int32        left_total, right_total, left, right;
    channel_t*   ch;
    loopSound_t *loop, *loop2;
    static int32 loopFrame;


    numLoopChannels = 0;

    time = Com_Milliseconds();

    loopFrame++;
    for (i = 0; i < MAX_GENTITIES; i++) {
        loop = &loopSounds[i];
        if (!loop->active || loop->mergeFrame == loopFrame) {
            continue; // already merged into an earlier sound
        }

        if (loop->kill) {
            S_SpatializeOrigin(loop->origin, 127, &left_total, &right_total); // 3d
        } else {
            S_SpatializeOrigin(loop->origin, 90, &left_total, &right_total);  // sphere
        }

        loop->sfx->lastTimeUsed = time;

        for (j = (i + 1); j < MAX_GENTITIES; j++) {
            loop2 = &loopSounds[j];
            if (!loop2->active || loop2->doppler || loop2->sfx != loop->sfx) {
                continue;
            }
            loop2->mergeFrame = loopFrame;

            if (loop2->kill) {
                S_SpatializeOrigin(loop2->origin, 127, &left, &right); // 3d
            } else {
                S_SpatializeOrigin(loop2->origin, 90, &left, &right);  // sphere
            }

            loop2->sfx->lastTimeUsed = time;
            left_total += left;
            right_total += right;
        }
        if (left_total == 0 && right_total == 0) {
            continue; // not audible
        }

        // allocate a channel
        ch = &loop_channels[numLoopChannels];

        if (left_total > 255) {
            left_total = 255;
        }
        if (right_total > 255) {
            right_total = 255;
        }

        ch->master_vol = 127;
        ch->leftvol = left_total;
        ch->rightvol = right_total;
        ch->thesfx = loop->sfx;
        ch->doppler = loop->doppler;
        ch->dopplerScale = loop->dopplerScale;
        ch->oldDopplerScale = loop->oldDopplerScale;
        numLoopChannels++;
        if (numLoopChannels == MAX_CHANNELS) {
            return;
        }
    }
}

//=============================================================================

/*
=================
S_ByteSwapRawSamples

If raw data has been loaded in little endien binary form, this must be done.
If raw data was calculated, as with ADPCM, this should not be called.
=================
*/
void S_ByteSwapRawSamples(int32 samples, int32 width, int32 s_channels, const uint8* data)
{
    int32 i;

    if (width != 2) {
        return;
    }
    if ((256) == 256) {
        return;
    }

    if (s_channels == 2) {
        samples <<= 1;
    }
    for (i = 0; i < samples; i++) {
        ((int16*) data)[i] = (((int16*) data)[i]);
    }
}

/*
============
S_RawSamples

Music streaming
============
*/
void S_RawSamples(int32 samples, int32 rate, int32 width, int32 s_channels, const uint8* data, float volume)
{
    int32 i;
    int32 src, dst;
    float scale;
    int32 intVolume;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    intVolume = 256 * volume;

    if (s_rawend < s_soundtime) {
        logger.debug("S_RawSamples: resetting minimum: {} < {}", s_rawend, s_soundtime);
        s_rawend = s_soundtime;
    }

    scale = (float) rate / dma.speed;

    if (s_channels == 2 && width == 2) {
        if (scale == 1.0) { // optimized case
            for (i = 0; i < samples; i++) {
                dst = s_rawend & (MAX_RAW_SAMPLES - 1);
                s_rawend++;
                s_rawsamples[dst].left = ((int16*) data)[i * 2] * intVolume;
                s_rawsamples[dst].right = ((int16*) data)[i * 2 + 1] * intVolume;
            }
        } else {
            for (i = 0;; i++) {
                src = i * scale;
                if (src >= samples) {
                    break;
                }
                dst = s_rawend & (MAX_RAW_SAMPLES - 1);
                s_rawend++;
                s_rawsamples[dst].left = ((int16*) data)[src * 2] * intVolume;
                s_rawsamples[dst].right = ((int16*) data)[src * 2 + 1] * intVolume;
            }
        }
    } else if (s_channels == 1 && width == 2) {
        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples) {
                break;
            }
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = ((int16*) data)[src] * intVolume;
            s_rawsamples[dst].right = ((int16*) data)[src] * intVolume;
        }
    } else if (s_channels == 2 && width == 1) {
        intVolume *= 256;

        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples) {
                break;
            }
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = ((char*) data)[src * 2] * intVolume;
            s_rawsamples[dst].right = ((char*) data)[src * 2 + 1] * intVolume;
        }
    } else if (s_channels == 1 && width == 1) {
        intVolume *= 256;

        for (i = 0;; i++) {
            src = i * scale;
            if (src >= samples) {
                break;
            }
            dst = s_rawend & (MAX_RAW_SAMPLES - 1);
            s_rawend++;
            s_rawsamples[dst].left = (((uint8*) data)[src] - 128) * intVolume;
            s_rawsamples[dst].right = (((uint8*) data)[src] - 128) * intVolume;
        }
    }

    if (s_rawend > s_soundtime + MAX_RAW_SAMPLES) {
        logger.debug("S_RawSamples: overflowed {} > {}", s_rawend, s_soundtime);
    }
}

//=============================================================================

/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_UpdateEntityPosition(int32 entityNum, const vec3_t origin)
{
    if (entityNum < 0 || entityNum > MAX_GENTITIES) {
        Com_Error(ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum);
    }
    VectorCopy(origin, loopSounds[entityNum].origin);
}


/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize(int32 entityNum, const vec3_t head, vec3_t axis[3], int32 inwater)
{
    int32      i;
    channel_t* ch;
    vec3_t     origin;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    listener_number = entityNum;
    VectorCopy(head, listener_origin);
    VectorCopy(axis[0], listener_axis[0]);
    VectorCopy(axis[1], listener_axis[1]);
    VectorCopy(axis[2], listener_axis[2]);

    // update spatialization for dynamic sounds
    ch = s_channels;
    for (i = 0; i < MAX_CHANNELS; i++, ch++) {
        if (!ch->thesfx) {
            continue;
        }
        // anything coming from the view entity will always be full volume
        if (ch->entnum == listener_number) {
            ch->leftvol = ch->master_vol;
            ch->rightvol = ch->master_vol;
        } else {
            if (ch->fixed_origin) {
                VectorCopy(ch->origin, origin);
            } else {
                VectorCopy(loopSounds[ch->entnum].origin, origin);
            }

            S_SpatializeOrigin(origin, ch->master_vol, &ch->leftvol, &ch->rightvol);
        }
    }

    // add loopsounds
    S_AddLoopSounds();
}


/*
========================
S_ScanChannelStarts

Returns true if any new sounds were started since the last mix
========================
*/
bool S_ScanChannelStarts()
{
    channel_t* ch;
    int32      i;
    bool       newSamples;

    newSamples = false;
    ch = s_channels;

    for (i = 0; i < MAX_CHANNELS; i++, ch++) {
        if (!ch->thesfx) {
            continue;
        }
        // if this channel was just started this frame,
        // set the sample count to it begins mixing
        // into the very first sample
        if (ch->startSample == START_SAMPLE_IMMEDIATE) {
            ch->startSample = s_paintedtime;
            newSamples = true;
            continue;
        }

        // if it is completely finished by now, clear it
        if (ch->startSample + (ch->thesfx->soundLength) <= s_paintedtime) {
            S_ChannelFree(ch);
        }
    }

    return newSamples;
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update()
{
    int32      i;
    int32      total;
    channel_t* ch;

    if (!s_soundStarted || s_soundMuted) {
        logger.debug("not started or muted");
        return;
    }

    //
    // debugging output
    //
    if (s_show->integer == 2) {
        total = 0;
        ch = s_channels;
        for (i = 0; i < MAX_CHANNELS; i++, ch++) {
            if (ch->thesfx && (ch->leftvol || ch->rightvol)) {
                logger.debug("{} {} {}", ch->leftvol, ch->rightvol, ch->thesfx->soundName);
                total++;
            }
        }

        logger.debug("----({})---- painted: {}", total, s_paintedtime);
    }

    // add raw data from streamed samples
    S_UpdateBackgroundTrack();

    // mix some sound
    S_Update_();
}

void S_GetSoundtime()
{
    int32        samplepos;
    static int32 buffers;
    static int32 oldsamplepos;
    int32        fullsamples;

    fullsamples = dma.samples / dma.channels;

    // it is possible to miscount buffers if it has wrapped twice between
    // calls to S_Update.  Oh well.
    samplepos = SNDDMA_GetDMAPos();
    if (samplepos < oldsamplepos) {
        buffers++;                        // buffer wrapped

        if (s_paintedtime > 0x40000000) { // time to chop things off to avoid 32 bit limits
            buffers = 0;
            s_paintedtime = fullsamples;
            S_StopAllSounds();
        }
    }
    oldsamplepos = samplepos;

    s_soundtime = buffers * fullsamples + samplepos / dma.channels;

    if (dma.submission_chunk < 256) {
        s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
    } else {
        s_paintedtime = s_soundtime + dma.submission_chunk;
    }
}


void S_Update_()
{
    uint32       endtime;
    int32        samps;
    static float lastTime = 0.0f;
    float        ma, op;
    float        thisTime, sane;
    static int32 ot = -1;

    if (!s_soundStarted || s_soundMuted) {
        return;
    }

    thisTime = Com_Milliseconds();

    // Updates s_soundtime
    S_GetSoundtime();

    if (s_soundtime == ot) {
        return;
    }
    ot = s_soundtime;

    // clear any sound effects that end before the current time,
    // and start any new sounds
    S_ScanChannelStarts();

    sane = thisTime - lastTime;
    if (sane < 11) {
        sane = 11; // 85hz
    }

    ma = s_mixahead->value * dma.speed;
    op = s_mixPreStep->value + sane * dma.speed * 0.01;

    if (op < ma) {
        ma = op;
    }

    // mix ahead of current position
    endtime = s_soundtime + ma;

    // mix to an even submission block size
    endtime = (endtime + dma.submission_chunk - 1)
            & ~(dma.submission_chunk - 1);

    // never mix more than the complete buffer
    samps = dma.samples >> (dma.channels - 1);
    if (endtime - s_soundtime > samps) {
        endtime = s_soundtime + samps;
    }


    SNDDMA_BeginPainting();

    S_PaintChannels(endtime);

    SNDDMA_Submit();

    lastTime = thisTime;
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play_f()
{
    int32       i;
    sfxHandle_t h;
    char        name[256];

    i = 1;
    while (i < Cmd_Argc()) {
        if (!Q_strrchr(Cmd_Argv(i), '.')) {
            Com_sprintf(name, sizeof(name), "%s.wav", Cmd_Argv(1));
        } else {
            Q_strncpyz(name, Cmd_Argv(i), sizeof(name));
        }
        h = S_RegisterSound(name, false);
        if (h) {
            S_StartLocalSound(h, CHAN_LOCAL_SOUND);
        }
        i++;
    }
}

void S_Music_f()
{
    int32 c;

    c = Cmd_Argc();

    if (c == 2) {
        S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(1));
        s_backgroundLoop[0] = 0;
    } else if (c == 3) {
        S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(2));
    } else {
        logger.info("music <musicfile> [loopfile]");
        return;
    }
}

void S_SoundList_f()
{
    int32  i;
    sfx_t* sfx;
    int32  size, total;
    char   type[4][16];
    char   mem[2][16];

    strcpy(type[0], "16bit");
    strcpy(type[1], "adpcm");
    strcpy(type[2], "daub4");
    strcpy(type[3], "mulaw");
    strcpy(mem[0], "paged out");
    strcpy(mem[1], "resident ");
    total = 0;
    for (sfx = s_knownSfx, i = 0; i < s_numSfx; i++, sfx++) {
        size = sfx->soundLength;
        total += size;
        logger.info("%6i[{}] : {}[{}]", size, type[sfx->soundCompressionMethod], sfx->soundName, mem[sfx->inMemory]);
    }
    logger.info("Total resident: {}", total);
    S_DisplayFreeMemory();
}


/*
===============================================================================

background music functions

===============================================================================
*/

int32 FGetLittleLong(fileHandle_t f)
{
    int32 v;

    FS_Read(&v, sizeof(v), f);

    return (v);
}

int32 FGetLittleShort(fileHandle_t f)
{
    int16 v;

    FS_Read(&v, sizeof(v), f);

    return (v);
}

// returns the length of the data in the chunk, or 0 if not found
int32 S_FindWavChunk(fileHandle_t f, const char* chunk)
{
    char  name[5];
    int32 len;
    int32 r;

    name[4] = 0;
    len = 0;
    r = FS_Read(name, 4, f);
    if (r != 4) {
        return 0;
    }
    len = FGetLittleLong(f);
    if (len < 0 || len > 0xfffffff) {
        len = 0;
        return 0;
    }
    len = (len + 1) & ~1; // pad to word boundary
                          //    s_nextWavChunk += len + 8;

    if (strcmp(name, chunk)) {
        return 0;
    }

    return len;
}

/*
======================
S_StopBackgroundTrack
======================
*/
void S_StopBackgroundTrack()
{
    if (!s_backgroundFile) {
        return;
    }
    FS_FCloseFile(s_backgroundFile);
    s_backgroundFile = 0;
    s_rawend = 0;
}

/*
======================
S_StartBackgroundTrack
======================
*/
void S_StartBackgroundTrack(const char* intro, const char* loop)
{
    int32 len;
    char  dump[16];
    char  name[MAX_QPATH];

    if (!intro) {
        intro = "";
    }
    if (!loop || !loop[0]) {
        loop = intro;
    }
    logger.debug("S_StartBackgroundTrack( {}, {} )", intro, loop);

    Q_strncpyz(name, intro, sizeof(name) - 4);
    COM_DefaultExtension(name, sizeof(name), ".wav");

    if (!intro[0]) {
        return;
    }

    Q_strncpyz(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

    // close the background track, but DON'T reset s_rawend
    // if restarting the same back ground track
    if (s_backgroundFile) {
        FS_FCloseFile(s_backgroundFile);
        s_backgroundFile = 0;
    }

    //
    // open up a wav file and get all the info
    //
    FS_FOpenFileRead(name, &s_backgroundFile, true);
    if (!s_backgroundFile) {
        logger.warning("couldn't open music file {}", name);
        return;
    }

    // skip the riff wav header

    FS_Read(dump, 12, s_backgroundFile);

    if (!S_FindWavChunk(s_backgroundFile, "fmt ")) {
        logger.info("No fmt chunk in {}", name);
        FS_FCloseFile(s_backgroundFile);
        s_backgroundFile = 0;
        return;
    }

    // save name for soundinfo
    s_backgroundInfo.format = FGetLittleShort(s_backgroundFile);
    s_backgroundInfo.channels = FGetLittleShort(s_backgroundFile);
    s_backgroundInfo.rate = FGetLittleLong(s_backgroundFile);
    FGetLittleLong(s_backgroundFile);
    FGetLittleShort(s_backgroundFile);
    s_backgroundInfo.width = FGetLittleShort(s_backgroundFile) / 8;

    if (s_backgroundInfo.format != WAV_FORMAT_PCM) {
        FS_FCloseFile(s_backgroundFile);
        s_backgroundFile = 0;
        logger.info("Not a microsoft PCM format wav: {}", name);
        return;
    }

    if (s_backgroundInfo.channels != 2 || s_backgroundInfo.rate != 22050) {
        logger.warning("Music file {} is not 22k stereo", name);
    }

    if ((len = S_FindWavChunk(s_backgroundFile, "data")) == 0) {
        FS_FCloseFile(s_backgroundFile);
        s_backgroundFile = 0;
        logger.info("No data chunk in {}", name);
        return;
    }

    s_backgroundInfo.samples = len / (s_backgroundInfo.width * s_backgroundInfo.channels);

    s_backgroundSamples = s_backgroundInfo.samples;
}

/*
======================
S_UpdateBackgroundTrack
======================
*/
void S_UpdateBackgroundTrack()
{
    int32        bufferSamples;
    int32        fileSamples;
    uint8        raw[30000]; // just enough to fit in a mac stack frame
    int32        fileBytes;
    int32        r;
    static float musicVolume = 0.5f;

    if (!s_backgroundFile) {
        return;
    }

    // graeme see if this is OK
    musicVolume = (musicVolume + (s_musicVolume->value * 2)) / 4.0f;

    // don't bother playing anything if musicvolume is 0
    if (musicVolume <= 0) {
        return;
    }

    // see how many samples should be copied into the raw buffer
    if (s_rawend < s_soundtime) {
        s_rawend = s_soundtime;
    }

    while (s_rawend < s_soundtime + MAX_RAW_SAMPLES) {
        bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);

        // decide how much data needs to be read from the file
        fileSamples = bufferSamples * s_backgroundInfo.rate / dma.speed;

        // don't try and read past the end of the file
        if (fileSamples > s_backgroundSamples) {
            fileSamples = s_backgroundSamples;
        }

        // our max buffer size
        fileBytes = fileSamples * (s_backgroundInfo.width * s_backgroundInfo.channels);
        if (fileBytes > sizeof(raw)) {
            fileBytes = sizeof(raw);
            fileSamples = fileBytes / (s_backgroundInfo.width * s_backgroundInfo.channels);
        }

        r = FS_Read(raw, fileBytes, s_backgroundFile);
        if (r != fileBytes) {
            logger.info("StreamedRead failure on music track");
            S_StopBackgroundTrack();
            return;
        }

        // uint8 swap if needed
        S_ByteSwapRawSamples(fileSamples, s_backgroundInfo.width, s_backgroundInfo.channels, raw);

        // add to raw buffer
        S_RawSamples(fileSamples, s_backgroundInfo.rate, s_backgroundInfo.width, s_backgroundInfo.channels, raw, musicVolume);

        s_backgroundSamples -= fileSamples;
        if (!s_backgroundSamples) {
            // loop
            if (s_backgroundLoop[0]) {
                FS_FCloseFile(s_backgroundFile);
                s_backgroundFile = 0;
                S_StartBackgroundTrack(s_backgroundLoop, s_backgroundLoop);
                if (!s_backgroundFile) {
                    return; // loop failed to restart
                }
            } else {
                s_backgroundFile = 0;
                return;
            }
        }
    }
}


/*
======================
S_FreeOldestSound
======================
*/

void S_FreeOldestSound()
{
    int32      i, oldest, used;
    sfx_t*     sfx;
    sndBuffer *buffer, *nbuffer;

    oldest = Com_Milliseconds();
    used = 0;

    for (i = 1; i < s_numSfx; i++) {
        sfx = &s_knownSfx[i];
        if (sfx->inMemory && sfx->lastTimeUsed < oldest) {
            used = i;
            oldest = sfx->lastTimeUsed;
        }
    }

    sfx = &s_knownSfx[used];

    logger.debug("S_FreeOldestSound: freeing sound {}", sfx->soundName);

    buffer = sfx->soundData;
    while (buffer != NULL) {
        nbuffer = buffer->next;
        SND_free(buffer);
        buffer = nbuffer;
    }
    sfx->inMemory = false;
    sfx->soundData = NULL;
}
