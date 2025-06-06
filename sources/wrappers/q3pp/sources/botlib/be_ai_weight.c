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
#include "l_libvar.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_ai_weight.h"

#define MAX_INVENTORYVALUE 999999

#define MAX_WEIGHT_FILES   128
weightconfig_t* weightFileList[MAX_WEIGHT_FILES];


int32 ReadValue(source_t* source, float* value)
{
    token_t token;

    if (!PC_ExpectAnyToken(source, &token)) {
        return false;
    }
    if (!strcmp(token.string, "-")) {
        SourceWarning(source, "negative value set to zero\n");
        if (!PC_ExpectTokenType(source, TT_NUMBER, 0, &token)) {
            return false;
        }
    }
    if (token.type != TT_NUMBER) {
        SourceError(source, "invalid return value %s\n", token.string);
        return false;
    }
    *value = token.floatvalue;
    return true;
}

int32 ReadFuzzyWeight(source_t* source, fuzzyseperator_t* fs)
{
    if (PC_CheckTokenString(source, "balance")) {
        fs->type = WT_BALANCE;
        if (!PC_ExpectTokenString(source, "(")) {
            return false;
        }
        if (!ReadValue(source, &fs->weight)) {
            return false;
        }
        if (!PC_ExpectTokenString(source, ",")) {
            return false;
        }
        if (!ReadValue(source, &fs->minweight)) {
            return false;
        }
        if (!PC_ExpectTokenString(source, ",")) {
            return false;
        }
        if (!ReadValue(source, &fs->maxweight)) {
            return false;
        }
        if (!PC_ExpectTokenString(source, ")")) {
            return false;
        }
    } else {
        fs->type = 0;
        if (!ReadValue(source, &fs->weight)) {
            return false;
        }
        fs->minweight = fs->weight;
        fs->maxweight = fs->weight;
    }
    if (!PC_ExpectTokenString(source, ";")) {
        return false;
    }
    return true;
}

void FreeFuzzySeperators_r(fuzzyseperator_t* fs)
{
    if (!fs) {
        return;
    }
    if (fs->child) {
        FreeFuzzySeperators_r(fs->child);
    }
    if (fs->next) {
        FreeFuzzySeperators_r(fs->next);
    }
    FreeMemory(fs);
}

void FreeWeightConfig2(weightconfig_t* config)
{
    int32 i;

    for (i = 0; i < config->numweights; i++) {
        FreeFuzzySeperators_r(config->weights[i].firstseperator);
        if (config->weights[i].name) {
            FreeMemory(config->weights[i].name);
        }
    }
    FreeMemory(config);
}

void FreeWeightConfig(weightconfig_t* config)
{
    if (!LibVarGetValue("bot_reloadcharacters")) {
        return;
    }
    FreeWeightConfig2(config);
}

fuzzyseperator_t* ReadFuzzySeperators_r(source_t* source)
{
    int32             newindent, index, def, founddefault;
    token_t           token;
    fuzzyseperator_t *fs, *lastfs, *firstfs;

    founddefault = false;
    firstfs = NULL;
    lastfs = NULL;
    if (!PC_ExpectTokenString(source, "(")) {
        return NULL;
    }
    if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token)) {
        return NULL;
    }
    index = token.intvalue;
    if (!PC_ExpectTokenString(source, ")")) {
        return NULL;
    }
    if (!PC_ExpectTokenString(source, "{")) {
        return NULL;
    }
    if (!PC_ExpectAnyToken(source, &token)) {
        return NULL;
    }
    do {
        def = !strcmp(token.string, "default");
        if (def || !strcmp(token.string, "case")) {
            fs = (fuzzyseperator_t*) GetClearedMemory(sizeof(fuzzyseperator_t));
            fs->index = index;
            if (lastfs) {
                lastfs->next = fs;
            } else {
                firstfs = fs;
            }
            lastfs = fs;
            if (def) {
                if (founddefault) {
                    SourceError(source, "switch already has a default\n");
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
                fs->value = MAX_INVENTORYVALUE;
                founddefault = true;
            } else {
                if (!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token)) {
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
                fs->value = token.intvalue;
            }
            if (!PC_ExpectTokenString(source, ":") || !PC_ExpectAnyToken(source, &token)) {
                FreeFuzzySeperators_r(firstfs);
                return NULL;
            }
            newindent = false;
            if (!strcmp(token.string, "{")) {
                newindent = true;
                if (!PC_ExpectAnyToken(source, &token)) {
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
            }
            if (!strcmp(token.string, "return")) {
                if (!ReadFuzzyWeight(source, fs)) {
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
            } else if (!strcmp(token.string, "switch")) {
                fs->child = ReadFuzzySeperators_r(source);
                if (!fs->child) {
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
            } else {
                SourceError(source, "invalid name %s\n", token.string);
                return NULL;
            }
            if (newindent) {
                if (!PC_ExpectTokenString(source, "}")) {
                    FreeFuzzySeperators_r(firstfs);
                    return NULL;
                }
            }
        } else {
            FreeFuzzySeperators_r(firstfs);
            SourceError(source, "invalid name %s\n", token.string);
            return NULL;
        }
        if (!PC_ExpectAnyToken(source, &token)) {
            FreeFuzzySeperators_r(firstfs);
            return NULL;
        }
    } while (strcmp(token.string, "}"));
    //
    if (!founddefault) {
        SourceWarning(source, "switch without default\n");
        fs = (fuzzyseperator_t*) GetClearedMemory(sizeof(fuzzyseperator_t));
        fs->index = index;
        fs->value = MAX_INVENTORYVALUE;
        fs->weight = 0;
        fs->next = NULL;
        fs->child = NULL;
        if (lastfs) {
            lastfs->next = fs;
        } else {
            firstfs = fs;
        }
        lastfs = fs;
    }
    //
    return firstfs;
}

weightconfig_t* ReadWeightConfig(char* filename)
{
    int32             newindent, avail = 0, n;
    token_t           token;
    source_t*         source;
    fuzzyseperator_t* fs;
    weightconfig_t*   config = NULL;
#ifdef DEBUG
    int32 starttime;

    starttime = Sys_MilliSeconds();
#endif // DEBUG

    if (!LibVarGetValue("bot_reloadcharacters")) {
        avail = -1;
        for (n = 0; n < MAX_WEIGHT_FILES; n++) {
            config = weightFileList[n];
            if (!config) {
                if (avail == -1) {
                    avail = n;
                }
                continue;
            }
            if (strcmp(filename, config->filename) == 0) {
                // botimport.Print( PRT_MESSAGE, "retained %s\n", filename );
                return config;
            }
        }

        if (avail == -1) {
            botimport.Print(PRT_ERROR, "weightFileList was full trying to load %s\n", filename);
            return NULL;
        }
    }

    PC_SetBaseFolder(BOTFILESBASEFOLDER);
    source = LoadSourceFile(filename);
    if (!source) {
        botimport.Print(PRT_ERROR, "counldn't load %s\n", filename);
        return NULL;
    }
    //
    config = (weightconfig_t*) GetClearedMemory(sizeof(weightconfig_t));
    config->numweights = 0;
    Q_strncpyz(config->filename, filename, sizeof(config->filename));
    // parse the item config file
    while (PC_ReadToken(source, &token)) {
        if (!strcmp(token.string, "weight")) {
            if (config->numweights >= MAX_WEIGHTS) {
                SourceWarning(source, "too many fuzzy weights\n");
                break;
            }
            if (!PC_ExpectTokenType(source, TT_STRING, 0, &token)) {
                FreeWeightConfig(config);
                FreeSource(source);
                return NULL;
            }
            StripDoubleQuotes(token.string);
            config->weights[config->numweights].name = (char*) GetClearedMemory(strlen(token.string) + 1);
            strcpy(config->weights[config->numweights].name, token.string);
            if (!PC_ExpectAnyToken(source, &token)) {
                FreeWeightConfig(config);
                FreeSource(source);
                return NULL;
            }
            newindent = false;
            if (!strcmp(token.string, "{")) {
                newindent = true;
                if (!PC_ExpectAnyToken(source, &token)) {
                    FreeWeightConfig(config);
                    FreeSource(source);
                    return NULL;
                }
            }
            if (!strcmp(token.string, "switch")) {
                fs = ReadFuzzySeperators_r(source);
                if (!fs) {
                    FreeWeightConfig(config);
                    FreeSource(source);
                    return NULL;
                }
                config->weights[config->numweights].firstseperator = fs;
            } else if (!strcmp(token.string, "return")) {
                fs = (fuzzyseperator_t*) GetClearedMemory(sizeof(fuzzyseperator_t));
                fs->index = 0;
                fs->value = MAX_INVENTORYVALUE;
                fs->next = NULL;
                fs->child = NULL;
                if (!ReadFuzzyWeight(source, fs)) {
                    FreeMemory(fs);
                    FreeWeightConfig(config);
                    FreeSource(source);
                    return NULL;
                }
                config->weights[config->numweights].firstseperator = fs;
            } else {
                SourceError(source, "invalid name %s\n", token.string);
                FreeWeightConfig(config);
                FreeSource(source);
                return NULL;
            }
            if (newindent) {
                if (!PC_ExpectTokenString(source, "}")) {
                    FreeWeightConfig(config);
                    FreeSource(source);
                    return NULL;
                }
            }
            config->numweights++;
        } else {
            SourceError(source, "invalid name %s\n", token.string);
            FreeWeightConfig(config);
            FreeSource(source);
            return NULL;
        }
    }
    // free the source at the end of a pass
    FreeSource(source);
    // if the file was located in a pak file
    botimport.Print(PRT_MESSAGE, "loaded %s\n", filename);
#ifdef DEBUG
    if (bot_developer) {
        botimport.Print(PRT_MESSAGE, "weights loaded in %d msec\n", Sys_MilliSeconds() - starttime);
    }
#endif // DEBUG
    //
    if (!LibVarGetValue("bot_reloadcharacters")) {
        weightFileList[avail] = config;
    }
    //
    return config;
}

int32 FindFuzzyWeight(weightconfig_t* wc, char* name)
{
    int32 i;

    for (i = 0; i < wc->numweights; i++) {
        if (!strcmp(wc->weights[i].name, name)) {
            return i;
        }
    }
    return -1;
}

float FuzzyWeight_r(int32* inventory, fuzzyseperator_t* fs)
{
    float scale, w1, w2;

    if (inventory[fs->index] < fs->value) {
        if (fs->child) {
            return FuzzyWeight_r(inventory, fs->child);
        } else {
            return fs->weight;
        }
    } else if (fs->next) {
        if (inventory[fs->index] < fs->next->value) {
            // first weight
            if (fs->child) {
                w1 = FuzzyWeight_r(inventory, fs->child);
            } else {
                w1 = fs->weight;
            }
            // second weight
            if (fs->next->child) {
                w2 = FuzzyWeight_r(inventory, fs->next->child);
            } else {
                w2 = fs->next->weight;
            }
            // the scale factor
            scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
            // scale between the two weights
            return scale * w1 + (1 - scale) * w2;
        }
        return FuzzyWeight_r(inventory, fs->next);
    }
    return fs->weight;
}

float FuzzyWeightUndecided_r(int32* inventory, fuzzyseperator_t* fs)
{
    float scale, w1, w2;

    if (inventory[fs->index] < fs->value) {
        if (fs->child) {
            return FuzzyWeightUndecided_r(inventory, fs->child);
        } else {
            return fs->minweight + random() * (fs->maxweight - fs->minweight);
        }
    } else if (fs->next) {
        if (inventory[fs->index] < fs->next->value) {
            // first weight
            if (fs->child) {
                w1 = FuzzyWeightUndecided_r(inventory, fs->child);
            } else {
                w1 = fs->minweight + random() * (fs->maxweight - fs->minweight);
            }
            // second weight
            if (fs->next->child) {
                w2 = FuzzyWeight_r(inventory, fs->next->child);
            } else {
                w2 = fs->next->minweight + random() * (fs->next->maxweight - fs->next->minweight);
            }
            // the scale factor
            scale = (inventory[fs->index] - fs->value) / (fs->next->value - fs->value);
            // scale between the two weights
            return scale * w1 + (1 - scale) * w2;
        }
        return FuzzyWeightUndecided_r(inventory, fs->next);
    }
    return fs->weight;
}

float FuzzyWeight(int32* inventory, weightconfig_t* wc, int32 weightnum)
{
    return FuzzyWeight_r(inventory, wc->weights[weightnum].firstseperator);
}

float FuzzyWeightUndecided(int32* inventory, weightconfig_t* wc, int32 weightnum)
{
    return FuzzyWeightUndecided_r(inventory, wc->weights[weightnum].firstseperator);
}

void EvolveFuzzySeperator_r(fuzzyseperator_t* fs)
{
    if (fs->child) {
        EvolveFuzzySeperator_r(fs->child);
    } else if (fs->type == WT_BALANCE) {
        // every once in a while an evolution leap occurs, mutation
        if (random() < 0.01) {
            fs->weight += crandom() * (fs->maxweight - fs->minweight);
        } else {
            fs->weight += crandom() * (fs->maxweight - fs->minweight) * 0.5;
        }
        // modify bounds if necesary because of mutation
        if (fs->weight < fs->minweight) {
            fs->minweight = fs->weight;
        } else if (fs->weight > fs->maxweight) {
            fs->maxweight = fs->weight;
        }
    }
    if (fs->next) {
        EvolveFuzzySeperator_r(fs->next);
    }
}

void EvolveWeightConfig(weightconfig_t* config)
{
    int32 i;

    for (i = 0; i < config->numweights; i++) {
        EvolveFuzzySeperator_r(config->weights[i].firstseperator);
    }
}

void ScaleFuzzySeperator_r(fuzzyseperator_t* fs, float scale)
{
    if (fs->child) {
        ScaleFuzzySeperator_r(fs->child, scale);
    } else if (fs->type == WT_BALANCE) {
        //
        fs->weight = (fs->maxweight + fs->minweight) * scale;
        // get the weight between bounds
        if (fs->weight < fs->minweight) {
            fs->weight = fs->minweight;
        } else if (fs->weight > fs->maxweight) {
            fs->weight = fs->maxweight;
        }
    }
    if (fs->next) {
        ScaleFuzzySeperator_r(fs->next, scale);
    }
}

void ScaleFuzzySeperatorBalanceRange_r(fuzzyseperator_t* fs, float scale)
{
    if (fs->child) {
        ScaleFuzzySeperatorBalanceRange_r(fs->child, scale);
    } else if (fs->type == WT_BALANCE) {
        float mid = (fs->minweight + fs->maxweight) * 0.5;
        // get the weight between bounds
        fs->maxweight = mid + (fs->maxweight - mid) * scale;
        fs->minweight = mid + (fs->minweight - mid) * scale;
        if (fs->maxweight < fs->minweight) {
            fs->maxweight = fs->minweight;
        }
    }
    if (fs->next) {
        ScaleFuzzySeperatorBalanceRange_r(fs->next, scale);
    }
}

int32 InterbreedFuzzySeperator_r(fuzzyseperator_t* fs1, fuzzyseperator_t* fs2, fuzzyseperator_t* fsout)
{
    if (fs1->child) {
        if (!fs2->child || !fsout->child) {
            botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal child\n");
            return false;
        }
        if (!InterbreedFuzzySeperator_r(fs2->child, fs2->child, fsout->child)) {
            return false;
        }
    } else if (fs1->type == WT_BALANCE) {
        if (fs2->type != WT_BALANCE || fsout->type != WT_BALANCE) {
            botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal balance\n");
            return false;
        }
        fsout->weight = (fs1->weight + fs2->weight) / 2;
        if (fsout->weight > fsout->maxweight) {
            fsout->maxweight = fsout->weight;
        }
        if (fsout->weight > fsout->minweight) {
            fsout->minweight = fsout->weight;
        }
    }
    if (fs1->next) {
        if (!fs2->next || !fsout->next) {
            botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal next\n");
            return false;
        }
        if (!InterbreedFuzzySeperator_r(fs1->next, fs2->next, fsout->next)) {
            return false;
        }
    }
    return true;
}

// config1 and config2 are interbreeded and stored in configout
void InterbreedWeightConfigs(weightconfig_t* config1, weightconfig_t* config2, weightconfig_t* configout)
{
    int32 i;

    if (config1->numweights != config2->numweights || config1->numweights != configout->numweights) {
        botimport.Print(PRT_ERROR, "cannot interbreed weight configs, unequal numweights\n");
        return;
    }
    for (i = 0; i < config1->numweights; i++) {
        InterbreedFuzzySeperator_r(config1->weights[i].firstseperator, config2->weights[i].firstseperator, configout->weights[i].firstseperator);
    }
}

void BotShutdownWeights()
{
    int32 i;

    for (i = 0; i < MAX_WEIGHT_FILES; i++) {
        if (weightFileList[i]) {
            FreeWeightConfig2(weightFileList[i]);
            weightFileList[i] = NULL;
        }
    }
}
