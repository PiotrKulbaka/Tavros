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

static tavros::core::logger logger("snd_mem");

#define DEF_COMSOUNDMEGS "8"

/*
===============================================================================

memory management

===============================================================================
*/

static sndBuffer* buffer = NULL;
static sndBuffer* freelist = NULL;
static int32      inUse = 0;
static int32      totalInUse = 0;

int16* sfxScratchBuffer = NULL;
sfx_t* sfxScratchPointer = NULL;
int32  sfxScratchIndex = 0;

void SND_free(sndBuffer* v)
{
    *(sndBuffer**) v = freelist;
    freelist = (sndBuffer*) v;
    inUse += sizeof(sndBuffer);
}

sndBuffer* SND_malloc()
{
    sndBuffer* v;
redo:
    if (freelist == NULL) {
        S_FreeOldestSound();
        goto redo;
    }

    inUse -= sizeof(sndBuffer);
    totalInUse += sizeof(sndBuffer);

    v = freelist;
    freelist = *(sndBuffer**) freelist;
    v->next = NULL;
    return v;
}

void SND_setup()
{
    sndBuffer *p, *q;
    cvar_t*    cv;
    int32      scs;

    cv = Cvar_Get("com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE);

    scs = (cv->integer * 1536);

    buffer = (sndBuffer*) malloc(scs * sizeof(sndBuffer));
    // allocate the stack based hunk allocator
    sfxScratchBuffer = (int16*) malloc(SND_CHUNK_SIZE * sizeof(int16) * 4); // Hunk_Alloc(SND_CHUNK_SIZE * sizeof(int16) * 4);
    sfxScratchPointer = NULL;

    inUse = scs * sizeof(sndBuffer);
    p = buffer;
    ;
    q = p + scs;
    while (--q > p) {
        *(sndBuffer**) q = q - 1;
    }

    *(sndBuffer**) q = NULL;
    freelist = p + scs - 1;

    logger.info("Sound memory manager started");
}

/*
===============================================================================

WAV loading

===============================================================================
*/

static uint8* data_p;
static uint8* iff_end;
static uint8* last_chunk;
static uint8* iff_data;
static int32  iff_chunk_len;

static int16 GetLittleShort()
{
    int16 val = 0;
    val = *data_p;
    val = val + (*(data_p + 1) << 8);
    data_p += 2;
    return val;
}

static int32 GetLittleLong()
{
    int32 val = 0;
    val = *data_p;
    val = val + (*(data_p + 1) << 8);
    val = val + (*(data_p + 2) << 16);
    val = val + (*(data_p + 3) << 24);
    data_p += 4;
    return val;
}

static void FindNextChunk(const char* name)
{
    while (1) {
        data_p = last_chunk;

        if (data_p >= iff_end) { // didn't find the chunk
            data_p = NULL;
            return;
        }

        data_p += 4;
        iff_chunk_len = GetLittleLong();
        if (iff_chunk_len < 0) {
            data_p = NULL;
            return;
        }
        data_p -= 8;
        last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
        if (!strncmp((char*) data_p, name, 4)) {
            return;
        }
    }
}

static void FindChunk(const char* name)
{
    last_chunk = iff_data;
    FindNextChunk(name);
}

/*
============
GetWavinfo
============
*/
static wavinfo_t GetWavinfo(char* name, uint8* wav, int32 wavlength)
{
    wavinfo_t info;

    Com_Memset(&info, 0, sizeof(info));

    if (!wav) {
        return info;
    }

    iff_data = wav;
    iff_end = wav + wavlength;

    // find "RIFF" chunk
    FindChunk("RIFF");
    if (!(data_p && !strncmp((char*) data_p + 8, "WAVE", 4))) {
        logger.info("Missing RIFF/WAVE chunks");
        return info;
    }

    // get "fmt " chunk
    iff_data = data_p + 12;
    // DumpChunks ();

    FindChunk("fmt ");
    if (!data_p) {
        logger.info("Missing fmt chunk");
        return info;
    }
    data_p += 8;
    info.format = GetLittleShort();
    info.channels = GetLittleShort();
    info.rate = GetLittleLong();
    data_p += 4 + 2;
    info.width = GetLittleShort() / 8;

    if (info.format != 1) {
        logger.info("Microsoft PCM format only");
        return info;
    }


    // find data chunk
    FindChunk("data");
    if (!data_p) {
        logger.info("Missing data chunk");
        return info;
    }

    data_p += 4;
    info.samples = GetLittleLong() / info.width;
    info.dataofs = data_p - wav;

    return info;
}


/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static void ResampleSfx(sfx_t* sfx, int32 inrate, int32 inwidth, uint8* data, bool compressed)
{
    int32      outcount;
    int32      srcsample;
    float      stepscale;
    int32      i;
    int32      sample, samplefrac, fracstep;
    int32      part;
    sndBuffer* chunk;

    stepscale = (float) inrate / dma.speed; // this is usually 0.5, 1, or 2

    outcount = sfx->soundLength / stepscale;
    sfx->soundLength = outcount;

    samplefrac = 0;
    fracstep = stepscale * 256;
    chunk = sfx->soundData;

    for (i = 0; i < outcount; i++) {
        srcsample = samplefrac >> 8;
        samplefrac += fracstep;
        if (inwidth == 2) {
            sample = (((int16*) data)[srcsample]);
        } else {
            sample = (int32) ((uint8) (data[srcsample]) - 128) << 8;
        }
        part = (i & (SND_CHUNK_SIZE - 1));
        if (part == 0) {
            sndBuffer* newchunk;
            newchunk = SND_malloc();
            if (chunk == NULL) {
                sfx->soundData = newchunk;
            } else {
                chunk->next = newchunk;
            }
            chunk = newchunk;
        }

        chunk->sndChunk[part] = sample;
    }
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static int32 ResampleSfxRaw(int16* sfx, int32 inrate, int32 inwidth, int32 samples, uint8* data)
{
    int32 outcount;
    int32 srcsample;
    float stepscale;
    int32 i;
    int32 sample, samplefrac, fracstep;

    stepscale = (float) inrate / dma.speed; // this is usually 0.5, 1, or 2

    outcount = samples / stepscale;

    samplefrac = 0;
    fracstep = stepscale * 256;

    for (i = 0; i < outcount; i++) {
        srcsample = samplefrac >> 8;
        samplefrac += fracstep;
        if (inwidth == 2) {
            sample = (((int16*) data)[srcsample]);
        } else {
            sample = (int32) ((uint8) (data[srcsample]) - 128) << 8;
        }
        sfx[i] = sample;
    }
    return outcount;
}


//=============================================================================

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
bool S_LoadSound(sfx_t* sfx)
{
    uint8*    data;
    int16*    samples;
    wavinfo_t info;
    int32     size;

    // player specific sounds are never directly loaded
    if (sfx->soundName[0] == '*') {
        return false;
    }

    // load it in
    size = FS_ReadFile(sfx->soundName, (void**) &data);
    if (!data) {
        return false;
    }

    info = GetWavinfo(sfx->soundName, data, size);
    if (info.channels != 1) {
        logger.info("%s is a stereo wav file", sfx->soundName);
        FS_FreeFile(data);
        return false;
    }

    if (info.width == 1) {
        logger.debug("%s is a 8 bit wav file", sfx->soundName);
    }

    if (info.rate != 22050) {
        logger.debug("%s is not a 22kHz wav file\n", sfx->soundName);
    }

    samples = (int16*) Hunk_AllocateTempMemory(info.samples * sizeof(int16) * 2);

    sfx->lastTimeUsed = Com_Milliseconds() + 1;

    // each of these compression schemes works just fine
    // but the 16bit quality is much nicer and with a local
    // install assured we can rely upon the sound memory
    // manager to do the right thing for us and page
    // sound in as needed

    if (sfx->soundCompressed) {
        sfx->soundCompressionMethod = 1;
        sfx->soundData = NULL;
        sfx->soundLength = ResampleSfxRaw(samples, info.rate, info.width, info.samples, (data + info.dataofs));
        S_AdpcmEncodeSound(sfx, samples);
    } else {
        sfx->soundCompressionMethod = 0;
        sfx->soundLength = info.samples;
        sfx->soundData = NULL;
        ResampleSfx(sfx, info.rate, info.width, data + info.dataofs, false);
    }

    Hunk_FreeTempMemory(samples);
    FS_FreeFile(data);

    return true;
}

void S_DisplayFreeMemory()
{
    logger.info("%d bytes free sound buffer memory, %d total used", inUse, totalInUse);
}
