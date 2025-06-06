/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
*/

#include "snd_local.h"


/* Intel ADPCM step variation table */
static int32 indexTable[16] = {
    -1,
    -1,
    -1,
    -1,
    2,
    4,
    6,
    8,
    -1,
    -1,
    -1,
    -1,
    2,
    4,
    6,
    8,
};

static int32 stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


void S_AdpcmEncode(int16 indata[], char outdata[], int32 len, struct adpcm_state* state)
{
    int16* inp;          /* Input buffer pointer */
    int8*  outp;         /* output buffer pointer */
    int32  val;          /* Current input sample value */
    int32  sign;         /* Current adpcm sign bit */
    int32  delta;        /* Current adpcm output value */
    int32  diff;         /* Difference between val and sample */
    int32  step;         /* Stepsize */
    int32  valpred;      /* Predicted output value */
    int32  vpdiff;       /* Current change to valpred */
    int32  index;        /* Current step change index */
    int32  outputbuffer; /* place to keep previous 4-bit value */
    int32  bufferstep;   /* toggle between outputbuffer/output */

    outp = (int8*) outdata;
    inp = indata;

    valpred = state->sample;
    index = state->index;
    step = stepsizeTable[index];

    outputbuffer = 0; // quiet a compiler warning
    bufferstep = 1;

    for (; len > 0; len--) {
        val = *inp++;

        /* Step 1 - compute difference with previous value */
        diff = val - valpred;
        sign = (diff < 0) ? 8 : 0;
        if (sign) {
            diff = (-diff);
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this is
        ** that even if you have fast mul/div hardware you cannot put it to
        ** good use since the fixup would be too expensive.
        */
        delta = 0;
        vpdiff = (step >> 3);

        if (diff >= step) {
            delta = 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            delta |= 1;
            vpdiff += step;
        }

        /* Step 3 - Update previous value */
        if (sign) {
            valpred -= vpdiff;
        } else {
            valpred += vpdiff;
        }

        /* Step 4 - Clamp previous value to 16 bits */
        if (valpred > 32767) {
            valpred = 32767;
        } else if (valpred < -32768) {
            valpred = -32768;
        }

        /* Step 5 - Assemble value, update index and step values */
        delta |= sign;

        index += indexTable[delta];
        if (index < 0) {
            index = 0;
        }
        if (index > 88) {
            index = 88;
        }
        step = stepsizeTable[index];

        /* Step 6 - Output value */
        if (bufferstep) {
            outputbuffer = (delta << 4) & 0xf0;
        } else {
            *outp++ = (delta & 0x0f) | outputbuffer;
        }
        bufferstep = !bufferstep;
    }

    /* Output last step, if needed */
    if (!bufferstep) {
        *outp++ = outputbuffer;
    }

    state->sample = valpred;
    state->index = index;
}


/* static */ void S_AdpcmDecode(const char indata[], int16* outdata, int32 len, struct adpcm_state* state)
{
    int8* inp;         /* Input buffer pointer */
    int32 outp;        /* output buffer pointer */
    int32 sign;        /* Current adpcm sign bit */
    int32 delta;       /* Current adpcm output value */
    int32 step;        /* Stepsize */
    int32 valpred;     /* Predicted value */
    int32 vpdiff;      /* Current change to valpred */
    int32 index;       /* Current step change index */
    int32 inputbuffer; /* place to keep next 4-bit value */
    int32 bufferstep;  /* toggle between inputbuffer/input */

    outp = 0;
    inp = (int8*) indata;

    valpred = state->sample;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 0;
    inputbuffer = 0; // quiet a compiler warning
    for (; len > 0; len--) {
        /* Step 1 - get the delta value */
        if (bufferstep) {
            delta = inputbuffer & 0xf;
        } else {
            inputbuffer = *inp++;
            delta = (inputbuffer >> 4) & 0xf;
        }
        bufferstep = !bufferstep;

        /* Step 2 - Find new index value (for later) */
        index += indexTable[delta];
        if (index < 0) {
            index = 0;
        }
        if (index > 88) {
            index = 88;
        }

        /* Step 3 - Separate sign and magnitude */
        sign = delta & 8;
        delta = delta & 7;

        /* Step 4 - Compute difference and new predicted value */
        /*
        ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
        ** in adpcm_coder.
        */
        vpdiff = step >> 3;
        if (delta & 4) {
            vpdiff += step;
        }
        if (delta & 2) {
            vpdiff += step >> 1;
        }
        if (delta & 1) {
            vpdiff += step >> 2;
        }

        if (sign) {
            valpred -= vpdiff;
        } else {
            valpred += vpdiff;
        }

        /* Step 5 - clamp output value */
        if (valpred > 32767) {
            valpred = 32767;
        } else if (valpred < -32768) {
            valpred = -32768;
        }

        /* Step 6 - Update step value */
        step = stepsizeTable[index];

        /* Step 7 - Output value */
        outdata[outp] = valpred;
        outp++;
    }

    state->sample = valpred;
    state->index = index;
}

/*
====================
S_AdpcmGetSamples
====================
*/
void S_AdpcmGetSamples(sndBuffer* chunk, int16* to)
{
    adpcm_state_t state;
    uint8*        out;

    // get the starting state from the block header
    state.index = chunk->adpcm.index;
    state.sample = chunk->adpcm.sample;

    out = (uint8*) chunk->sndChunk;
    // get samples
    S_AdpcmDecode((const char*) out, to, SND_CHUNK_SIZE_BYTE * 2, &state);
}


/*
====================
S_AdpcmEncodeSound
====================
*/
void S_AdpcmEncodeSound(sfx_t* sfx, int16* samples)
{
    adpcm_state_t state;
    int32         inOffset;
    int32         count;
    int32         n;
    sndBuffer *   newchunk, *chunk;
    uint8*        out;

    inOffset = 0;
    count = sfx->soundLength;
    state.index = 0;
    state.sample = samples[0];

    chunk = NULL;
    while (count) {
        n = count;
        if (n > SND_CHUNK_SIZE_BYTE * 2) {
            n = SND_CHUNK_SIZE_BYTE * 2;
        }

        newchunk = SND_malloc();
        if (sfx->soundData == NULL) {
            sfx->soundData = newchunk;
        } else {
            chunk->next = newchunk;
        }
        chunk = newchunk;

        // output the header
        chunk->adpcm.index = state.index;
        chunk->adpcm.sample = state.sample;

        out = (uint8*) chunk->sndChunk;

        // encode the samples
        S_AdpcmEncode(samples + inOffset, (char*) out, n, &state);

        inOffset += n;
        count -= n;
    }
}
