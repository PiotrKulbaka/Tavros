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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "snd_local.h"

static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int32                 snd_vol;

// bk001119 - these not static, required by unix/snd_mixa.s
int32* snd_p;
int32  snd_linear_count;
int16* snd_out;

void S_WriteLinearBlastStereo16()
{
    int32 i;
    int32 val;

    for (i = 0; i < snd_linear_count; i += 2) {
        val = snd_p[i] >> 8;
        if (val > 0x7fff) {
            snd_out[i] = 0x7fff;
        } else if (val < -32768) {
            snd_out[i] = -32768;
        } else {
            snd_out[i] = val;
        }

        val = snd_p[i + 1] >> 8;
        if (val > 0x7fff) {
            snd_out[i + 1] = 0x7fff;
        } else if (val < -32768) {
            snd_out[i + 1] = -32768;
        } else {
            snd_out[i + 1] = val;
        }
    }
}

void S_TransferStereo16(uint32* pbuf, int32 endtime)
{
    int32 lpos;
    int32 ls_paintedtime;

    snd_p = (int32*) paintbuffer;
    ls_paintedtime = s_paintedtime;

    while (ls_paintedtime < endtime) {
        // handle recirculating buffer issues
        lpos = ls_paintedtime & ((dma.samples >> 1) - 1);

        snd_out = (int16*) pbuf + (lpos << 1);

        snd_linear_count = (dma.samples >> 1) - lpos;
        if (ls_paintedtime + snd_linear_count > endtime) {
            snd_linear_count = endtime - ls_paintedtime;
        }

        snd_linear_count <<= 1;

        // write a linear blast of samples
        S_WriteLinearBlastStereo16();

        snd_p += snd_linear_count;
        ls_paintedtime += (snd_linear_count >> 1);
    }
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer(int32 endtime)
{
    int32   out_idx;
    int32   count;
    int32   out_mask;
    int32*  p;
    int32   step;
    int32   val;
    uint32* pbuf;

    pbuf = (uint32*) dma.buffer;


    if (s_testsound->integer) {
        int32 i;
        int32 count;

        // write a fixed sine wave
        count = (endtime - s_paintedtime);
        for (i = 0; i < count; i++) {
            paintbuffer[i].left = paintbuffer[i].right = sin((s_paintedtime + i) * 0.1) * 20000 * 256;
        }
    }


    if (dma.samplebits == 16 && dma.channels == 2) { // optimized case
        S_TransferStereo16(pbuf, endtime);
    } else {                                         // general case
        p = (int32*) paintbuffer;
        count = (endtime - s_paintedtime) * dma.channels;
        out_mask = dma.samples - 1;
        out_idx = s_paintedtime * dma.channels & out_mask;
        step = 3 - dma.channels;

        if (dma.samplebits == 16) {
            int16* out = (int16*) pbuf;
            while (count--) {
                val = *p >> 8;
                p += step;
                if (val > 0x7fff) {
                    val = 0x7fff;
                } else if (val < -32768) {
                    val = -32768;
                }
                out[out_idx] = val;
                out_idx = (out_idx + 1) & out_mask;
            }
        } else if (dma.samplebits == 8) {
            uint8* out = (uint8*) pbuf;
            while (count--) {
                val = *p >> 8;
                p += step;
                if (val > 0x7fff) {
                    val = 0x7fff;
                } else if (val < -32768) {
                    val = -32768;
                }
                out[out_idx] = (val >> 8) + 128;
                out_idx = (out_idx + 1) & out_mask;
            }
        }
    }
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void S_PaintChannelFrom16(channel_t* ch, const sfx_t* sc, int32 count, int32 sampleOffset, int32 bufferOffset)
{
    int32                  data, aoff, boff;
    int32                  leftvol, rightvol;
    int32                  i, j;
    portable_samplepair_t* samp;
    sndBuffer*             chunk;
    int16*                 samples;
    float                  ooff, fdata, fdiv, fleftvol, frightvol;

    samp = &paintbuffer[bufferOffset];

    if (ch->doppler) {
        sampleOffset = sampleOffset * ch->oldDopplerScale;
    }

    chunk = sc->soundData;
    while (sampleOffset >= SND_CHUNK_SIZE) {
        chunk = chunk->next;
        sampleOffset -= SND_CHUNK_SIZE;
        if (!chunk) {
            chunk = sc->soundData;
        }
    }

    if (!ch->doppler || ch->dopplerScale == 1.0f) {
        leftvol = ch->leftvol * snd_vol;
        rightvol = ch->rightvol * snd_vol;
        samples = chunk->sndChunk;

        for (i = 0; i < count; i++) {
            data = samples[sampleOffset++];
            samp[i].left += (data * leftvol) >> 8;
            samp[i].right += (data * rightvol) >> 8;

            if (sampleOffset == SND_CHUNK_SIZE) {
                chunk = chunk->next;
                samples = chunk->sndChunk;
                sampleOffset = 0;
            }
        }

    } else {
        fleftvol = ch->leftvol * snd_vol;
        frightvol = ch->rightvol * snd_vol;

        ooff = sampleOffset;
        samples = chunk->sndChunk;


        for (i = 0; i < count; i++) {
            aoff = ooff;
            ooff = ooff + ch->dopplerScale;
            boff = ooff;
            fdata = 0;
            for (j = aoff; j < boff; j++) {
                if (j == SND_CHUNK_SIZE) {
                    chunk = chunk->next;
                    if (!chunk) {
                        chunk = sc->soundData;
                    }
                    samples = chunk->sndChunk;
                    ooff -= SND_CHUNK_SIZE;
                }
                fdata += samples[j & (SND_CHUNK_SIZE - 1)];
            }
            fdiv = 256 * (boff - aoff);
            samp[i].left += (fdata * fleftvol) / fdiv;
            samp[i].right += (fdata * frightvol) / fdiv;
        }
    }
}

void S_PaintChannelFromWavelet(channel_t* ch, sfx_t* sc, int32 count, int32 sampleOffset, int32 bufferOffset)
{
    int32                  data;
    int32                  leftvol, rightvol;
    int32                  i;
    portable_samplepair_t* samp;
    sndBuffer*             chunk;
    int16*                 samples;

    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;

    i = 0;
    samp = &paintbuffer[bufferOffset];
    chunk = sc->soundData;
    while (sampleOffset >= (SND_CHUNK_SIZE_FLOAT * 4)) {
        chunk = chunk->next;
        sampleOffset -= (SND_CHUNK_SIZE_FLOAT * 4);
        i++;
    }

    if (i != sfxScratchIndex || sfxScratchPointer != sc) {
        S_AdpcmGetSamples(chunk, sfxScratchBuffer);
        sfxScratchIndex = i;
        sfxScratchPointer = sc;
    }

    samples = sfxScratchBuffer;

    for (i = 0; i < count; i++) {
        data = samples[sampleOffset++];
        samp[i].left += (data * leftvol) >> 8;
        samp[i].right += (data * rightvol) >> 8;

        if (sampleOffset == SND_CHUNK_SIZE * 2) {
            chunk = chunk->next;
            decodeWavelet(chunk, sfxScratchBuffer);
            sfxScratchIndex++;
            sampleOffset = 0;
        }
    }
}

void S_PaintChannelFromADPCM(channel_t* ch, sfx_t* sc, int32 count, int32 sampleOffset, int32 bufferOffset)
{
    int32                  data;
    int32                  leftvol, rightvol;
    int32                  i;
    portable_samplepair_t* samp;
    sndBuffer*             chunk;
    int16*                 samples;

    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;

    i = 0;
    samp = &paintbuffer[bufferOffset];
    chunk = sc->soundData;

    if (ch->doppler) {
        sampleOffset = sampleOffset * ch->oldDopplerScale;
    }

    while (sampleOffset >= (SND_CHUNK_SIZE * 4)) {
        chunk = chunk->next;
        sampleOffset -= (SND_CHUNK_SIZE * 4);
        i++;
    }

    if (i != sfxScratchIndex || sfxScratchPointer != sc) {
        S_AdpcmGetSamples(chunk, sfxScratchBuffer);
        sfxScratchIndex = i;
        sfxScratchPointer = sc;
    }

    samples = sfxScratchBuffer;

    for (i = 0; i < count; i++) {
        data = samples[sampleOffset++];
        samp[i].left += (data * leftvol) >> 8;
        samp[i].right += (data * rightvol) >> 8;

        if (sampleOffset == SND_CHUNK_SIZE * 4) {
            chunk = chunk->next;
            S_AdpcmGetSamples(chunk, sfxScratchBuffer);
            sampleOffset = 0;
            sfxScratchIndex++;
        }
    }
}

void S_PaintChannelFromMuLaw(channel_t* ch, sfx_t* sc, int32 count, int32 sampleOffset, int32 bufferOffset)
{
    int32                  data;
    int32                  leftvol, rightvol;
    int32                  i;
    portable_samplepair_t* samp;
    sndBuffer*             chunk;
    uint8*                 samples;
    float                  ooff;

    leftvol = ch->leftvol * snd_vol;
    rightvol = ch->rightvol * snd_vol;

    samp = &paintbuffer[bufferOffset];
    chunk = sc->soundData;
    while (sampleOffset >= (SND_CHUNK_SIZE * 2)) {
        chunk = chunk->next;
        sampleOffset -= (SND_CHUNK_SIZE * 2);
        if (!chunk) {
            chunk = sc->soundData;
        }
    }

    if (!ch->doppler) {
        samples = (uint8*) chunk->sndChunk + sampleOffset;
        for (i = 0; i < count; i++) {
            data = mulawToShort[*samples];
            samp[i].left += (data * leftvol) >> 8;
            samp[i].right += (data * rightvol) >> 8;
            samples++;
            if (samples == (uint8*) chunk->sndChunk + (SND_CHUNK_SIZE * 2)) {
                chunk = chunk->next;
                samples = (uint8*) chunk->sndChunk;
            }
        }
    } else {
        ooff = sampleOffset;
        samples = (uint8*) chunk->sndChunk;
        for (i = 0; i < count; i++) {
            data = mulawToShort[samples[(int32) (ooff)]];
            ooff = ooff + ch->dopplerScale;
            samp[i].left += (data * leftvol) >> 8;
            samp[i].right += (data * rightvol) >> 8;
            if (ooff >= SND_CHUNK_SIZE * 2) {
                chunk = chunk->next;
                if (!chunk) {
                    chunk = sc->soundData;
                }
                samples = (uint8*) chunk->sndChunk;
                ooff = 0.0;
            }
        }
    }
}

/*
===================
S_PaintChannels
===================
*/
void S_PaintChannels(int32 endtime)
{
    int32      i;
    int32      end;
    channel_t* ch;
    sfx_t*     sc;
    int32      ltime, count;
    int32      sampleOffset;


    snd_vol = s_volume->value * 255;

    while (s_paintedtime < endtime) {
        // if paintbuffer is smaller than DMA buffer
        // we may need to fill it multiple times
        end = endtime;
        if (endtime - s_paintedtime > PAINTBUFFER_SIZE) {
            end = s_paintedtime + PAINTBUFFER_SIZE;
        }

        // clear the paint buffer to either music or zeros
        if (s_rawend < s_paintedtime) {
            Com_Memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
        } else {
            // copy from the streaming sound source
            int32 s;
            int32 stop;

            stop = (end < s_rawend) ? end : s_rawend;

            for (i = s_paintedtime; i < stop; i++) {
                s = i & (MAX_RAW_SAMPLES - 1);
                paintbuffer[i - s_paintedtime] = s_rawsamples[s];
            }
            for (; i < end; i++) {
                paintbuffer[i - s_paintedtime].left =
                    paintbuffer[i - s_paintedtime].right = 0;
            }
        }

        // paint in the channels.
        ch = s_channels;
        for (i = 0; i < MAX_CHANNELS; i++, ch++) {
            if (!ch->thesfx || (ch->leftvol < 0.25 && ch->rightvol < 0.25)) {
                continue;
            }

            ltime = s_paintedtime;
            sc = ch->thesfx;

            sampleOffset = ltime - ch->startSample;
            count = end - ltime;
            if (sampleOffset + count > sc->soundLength) {
                count = sc->soundLength - sampleOffset;
            }

            if (count > 0) {
                if (sc->soundCompressionMethod == 1) {
                    S_PaintChannelFromADPCM(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                } else if (sc->soundCompressionMethod == 2) {
                    S_PaintChannelFromWavelet(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                } else if (sc->soundCompressionMethod == 3) {
                    S_PaintChannelFromMuLaw(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                } else {
                    S_PaintChannelFrom16(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                }
            }
        }

        // paint in the looped channels.
        ch = loop_channels;
        for (i = 0; i < numLoopChannels; i++, ch++) {
            if (!ch->thesfx || (!ch->leftvol && !ch->rightvol)) {
                continue;
            }

            ltime = s_paintedtime;
            sc = ch->thesfx;

            if (sc->soundData == NULL || sc->soundLength == 0) {
                continue;
            }
            // we might have to make two passes if it
            // is a looping sound effect and the end of
            // the sample is hit
            do {
                sampleOffset = (ltime % sc->soundLength);

                count = end - ltime;
                if (sampleOffset + count > sc->soundLength) {
                    count = sc->soundLength - sampleOffset;
                }

                if (count > 0) {
                    if (sc->soundCompressionMethod == 1) {
                        S_PaintChannelFromADPCM(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                    } else if (sc->soundCompressionMethod == 2) {
                        S_PaintChannelFromWavelet(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                    } else if (sc->soundCompressionMethod == 3) {
                        S_PaintChannelFromMuLaw(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                    } else {
                        S_PaintChannelFrom16(ch, sc, count, sampleOffset, ltime - s_paintedtime);
                    }
                    ltime += count;
                }
            } while (ltime < end);
        }

        // transfer out according to DMA format
        S_TransferPaintBuffer(end);
        s_paintedtime = end;
    }
}
