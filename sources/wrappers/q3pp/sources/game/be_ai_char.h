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

#include "ai_main.h"

// loads a bot character from a file
int32 BotLoadCharacter(char* charfile, float skill);
// frees a bot character
void BotFreeCharacter(int32 character);
// returns a float characteristic
float Characteristic_Float(int32 character, int32 index);
// returns a bounded float characteristic
float Characteristic_BFloat(int32 character, int32 index, float min, float max);
// returns an integer characteristic
int32 Characteristic_Integer(int32 character, int32 index);
// returns a bounded integer characteristic
int32 Characteristic_BInteger(int32 character, int32 index, int32 min, int32 max);
// returns a string characteristic
void Characteristic_String(int32 character, int32 index, char* buf, int32 size);
// free cached bot characters
void BotShutdownCharacters();
