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

int32 myftol(float f);

#define C0 0.4829629131445341
#define C1 0.8365163037378079
#define C2 0.2241438680420134
#define C3 -0.1294095225512604

void daub4(float b[], uint32 n, int32 isign)
{
    float  wksp[4097];
    float* a = b - 1; // numerical recipies so a[1] = b[0]

    uint32 nh, nh1, i, j;

    if (n < 4) {
        return;
    }

    nh1 = (nh = n >> 1) + 1;
    if (isign >= 0) {
        for (i = 1, j = 1; j <= n - 3; j += 2, i++) {
            wksp[i] = C0 * a[j] + C1 * a[j + 1] + C2 * a[j + 2] + C3 * a[j + 3];
            wksp[i + nh] = C3 * a[j] - C2 * a[j + 1] + C1 * a[j + 2] - C0 * a[j + 3];
        }
        wksp[i] = C0 * a[n - 1] + C1 * a[n] + C2 * a[1] + C3 * a[2];
        wksp[i + nh] = C3 * a[n - 1] - C2 * a[n] + C1 * a[1] - C0 * a[2];
    } else {
        wksp[1] = C2 * a[nh] + C1 * a[n] + C0 * a[1] + C3 * a[nh1];
        wksp[2] = C3 * a[nh] - C0 * a[n] + C1 * a[1] - C2 * a[nh1];
        for (i = 1, j = 3; i < nh; i++) {
            wksp[j++] = C2 * a[i] + C1 * a[i + nh] + C0 * a[i + 1] + C3 * a[i + nh1];
            wksp[j++] = C3 * a[i] - C0 * a[i + nh] + C1 * a[i + 1] - C2 * a[i + nh1];
        }
    }
    for (i = 1; i <= n; i++) {
        a[i] = wksp[i];
    }
}

void wt1(float a[], uint32 n, int32 isign)
{
    uint32 nn;
    int32  inverseStartLength = n / 4;
    if (n < inverseStartLength) {
        return;
    }
    if (isign >= 0) {
        for (nn = n; nn >= inverseStartLength; nn >>= 1) {
            daub4(a, nn, isign);
        }
    } else {
        for (nn = inverseStartLength; nn <= n; nn <<= 1) {
            daub4(a, nn, isign);
        }
    }
}

int16 mulawToShort[256];

void decodeWavelet(sndBuffer* chunk, int16* to)
{
    float  wksp[4097];
    int32  i;
    uint8* out;

    int32 size = chunk->size;

    out = (uint8*) chunk->sndChunk;
    for (i = 0; i < size; i++) {
        wksp[i] = mulawToShort[out[i]];
    }

    wt1(wksp, size, -1);

    if (!to) {
        return;
    }

    for (i = 0; i < size; i++) {
        to[i] = wksp[i];
    }
}
