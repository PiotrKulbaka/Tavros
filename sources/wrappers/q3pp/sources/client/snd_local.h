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
// snd_local.h -- private sound definations


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define PAINTBUFFER_SIZE     4096                 // this is in samples

#define SND_CHUNK_SIZE       1024                 // samples
#define SND_CHUNK_SIZE_FLOAT (SND_CHUNK_SIZE / 2) // floats
#define SND_CHUNK_SIZE_BYTE  (SND_CHUNK_SIZE * 2) // floats

typedef struct
{
    int32 left; // the final values will be clamped to +/- 0x00ffff00 and shifted down
    int32 right;
} portable_samplepair_t;

typedef struct adpcm_state
{
    int16 sample; /* Previous output value */
    char  index;  /* Index into stepsize table */
} adpcm_state_t;

typedef struct sndBuffer_s
{
    int16               sndChunk[SND_CHUNK_SIZE];
    struct sndBuffer_s* next;
    int32               size;
    adpcm_state_t       adpcm;
} sndBuffer;

typedef struct sfx_s
{
    sndBuffer*    soundData;
    bool          defaultSound;    // couldn't be loaded, so use buzz
    bool          inMemory;        // not in Memory
    bool          soundCompressed; // not in Memory
    int32         soundCompressionMethod;
    int32         soundLength;
    char          soundName[MAX_QPATH];
    int32         lastTimeUsed;
    struct sfx_s* next;
} sfx_t;

typedef struct
{
    int32  channels;
    int32  samples;          // mono samples in buffer
    int32  submission_chunk; // don't mix less than this #
    int32  samplebits;
    int32  speed;
    uint8* buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE 0x7fffffff

typedef struct loopSound_s
{
    vec3_t origin;
    vec3_t velocity;
    sfx_t* sfx;
    int32  mergeFrame;
    bool   active;
    bool   kill;
    bool   doppler;
    float  dopplerScale;
    float  oldDopplerScale;
    int32  framenum;
} loopSound_t;

typedef struct
{
    int32  allocTime;
    int32  startSample; // START_SAMPLE_IMMEDIATE = set immediately on next mix
    int32  entnum;      // to allow overriding a specific sound
    int32  entchannel;  // to allow overriding a specific sound
    int32  leftvol;     // 0-255 volume after spatialization
    int32  rightvol;    // 0-255 volume after spatialization
    int32  master_vol;  // 0-255 volume before spatialization
    float  dopplerScale;
    float  oldDopplerScale;
    vec3_t origin;       // only use if fixed_origin is set
    bool   fixed_origin; // use origin instead of fetching entnum's origin
    sfx_t* thesfx;       // sfx structure
    bool   doppler;
} channel_t;


#define WAV_FORMAT_PCM 1


typedef struct
{
    int32 format;
    int32 rate;
    int32 width;
    int32 channels;
    int32 samples;
    int32 dataofs; // chunk starts this many bytes from file start
} wavinfo_t;


/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init();

// gets the current DMA position
int32 SNDDMA_GetDMAPos();

// shutdown the DMA xfer.
void SNDDMA_Shutdown();

void SNDDMA_BeginPainting();

void SNDDMA_Submit();

//====================================================================

#define MAX_CHANNELS 96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern int32     numLoopChannels;

extern int32 s_paintedtime;
extern int32 s_rawend;
extern dma_t dma;

#define MAX_RAW_SAMPLES 16384
extern portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];

extern cvar_t* s_volume;
extern cvar_t* s_khz;
extern cvar_t* s_mixahead;

extern cvar_t* s_testsound;
extern cvar_t* s_separation;

bool S_LoadSound(sfx_t* sfx);

void       SND_free(sndBuffer* v);
sndBuffer* SND_malloc();
void       SND_setup();

void S_PaintChannels(int32 endtime);

void S_memoryLoad(sfx_t* sfx);

// adpcm functions
void S_AdpcmEncodeSound(sfx_t* sfx, int16* samples);
void S_AdpcmGetSamples(sndBuffer* chunk, int16* to);

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN     127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound();

#define NXStream uint8

void decodeWavelet(sndBuffer* stream, int16* packets);

extern int16 mulawToShort[256];

extern int16* sfxScratchBuffer;
extern sfx_t* sfxScratchPointer;
extern int32  sfxScratchIndex;

