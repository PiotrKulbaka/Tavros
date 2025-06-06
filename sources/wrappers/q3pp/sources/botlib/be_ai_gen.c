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

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "../game/be_ai_gen.h"

//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 GeneticSelection(int32 numranks, float* rankings)
{
    float sum, select;
    int32 i, index;

    sum = 0;
    for (i = 0; i < numranks; i++) {
        if (rankings[i] < 0) {
            continue;
        }
        sum += rankings[i];
    }
    if (sum > 0) {
        // select a bot where the ones with the higest rankings have
        // the highest chance of being selected
        select = random() * sum;
        for (i = 0; i < numranks; i++) {
            if (rankings[i] < 0) {
                continue;
            }
            sum -= rankings[i];
            if (sum <= 0) {
                return i;
            }
        }
    }
    // select a bot randomly
    index = random() * numranks;
    for (i = 0; i < numranks; i++) {
        if (rankings[index] >= 0) {
            return index;
        }
        index = (index + 1) % numranks;
    }
    return 0;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 GeneticParentsAndChildSelection(int32 numranks, float* ranks, int32* parent1, int32* parent2, int32* child)
{
    float rankings[256], max;
    int32 i;

    if (numranks > 256) {
        botimport.Print(PRT_WARNING, "GeneticParentsAndChildSelection: too many bots\n");
        *parent1 = *parent2 = *child = 0;
        return false;
    }
    for (max = 0, i = 0; i < numranks; i++) {
        if (ranks[i] < 0) {
            continue;
        }
        max++;
    }
    if (max < 3) {
        botimport.Print(PRT_WARNING, "GeneticParentsAndChildSelection: too few valid bots\n");
        *parent1 = *parent2 = *child = 0;
        return false;
    }
    Com_Memcpy(rankings, ranks, sizeof(float) * numranks);
    // select first parent
    *parent1 = GeneticSelection(numranks, rankings);
    rankings[*parent1] = -1;
    // select second parent
    *parent2 = GeneticSelection(numranks, rankings);
    rankings[*parent2] = -1;
    // reverse the rankings
    max = 0;
    for (i = 0; i < numranks; i++) {
        if (rankings[i] < 0) {
            continue;
        }
        if (rankings[i] > max) {
            max = rankings[i];
        }
    }
    for (i = 0; i < numranks; i++) {
        if (rankings[i] < 0) {
            continue;
        }
        rankings[i] = max - rankings[i];
    }
    // select child
    *child = GeneticSelection(numranks, rankings);
    return true;
}
