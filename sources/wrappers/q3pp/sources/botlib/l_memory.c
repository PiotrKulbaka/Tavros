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
#include "../game/botlib.h"
#include "l_log.h"
#include "be_interface.h"

// #define MEMDEBUG
// #define MEMORYMANEGER

#define MEM_ID  0x12345678l
#define HUNK_ID 0x87654321l

int32 allocatedmemory;
int32 totalmemorysize;
int32 numblocks;

#ifdef MEMORYMANEGER

typedef struct memoryblock_s
{
    uint32 id;
    void*  ptr;
    int32  size;
    #ifdef MEMDEBUG
    char* label;
    char* file;
    int32 line;
    #endif // MEMDEBUG
    struct memoryblock_s *prev, *next;
} memoryblock_t;

memoryblock_t* memory;

//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void LinkMemoryBlock(memoryblock_t* block)
{
    block->prev = NULL;
    block->next = memory;
    if (memory) {
        memory->prev = block;
    }
    memory = block;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void UnlinkMemoryBlock(memoryblock_t* block)
{
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        memory = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetMemory(uint32 size)
    #endif // MEMDEBUG
{
    void*          ptr;
    memoryblock_t* block;
    assert(botimport.GetMemory); // bk001129 - was NULL'ed
    ptr = botimport.GetMemory(size + sizeof(memoryblock_t));
    block = (memoryblock_t*) ptr;
    block->id = MEM_ID;
    block->ptr = (char*) ptr + sizeof(memoryblock_t);
    block->size = size + sizeof(memoryblock_t);
    #ifdef MEMDEBUG
    block->label = label;
    block->file = file;
    block->line = line;
    #endif // MEMDEBUG
    LinkMemoryBlock(block);
    allocatedmemory += block->size;
    totalmemorysize += block->size + sizeof(memoryblock_t);
    numblocks++;
    return block->ptr;
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetClearedMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetClearedMemory(uint32 size)
    #endif // MEMDEBUG
{
    void* ptr;
    #ifdef MEMDEBUG
    ptr = GetMemoryDebug(size, label, file, line);
    #else
    ptr = GetMemory(size);
    #endif // MEMDEBUG
    Com_Memset(ptr, 0, size);
    return ptr;
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetHunkMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetHunkMemory(uint32 size)
    #endif // MEMDEBUG
{
    void*          ptr;
    memoryblock_t* block;

    ptr = botimport.HunkAlloc(size + sizeof(memoryblock_t));
    block = (memoryblock_t*) ptr;
    block->id = HUNK_ID;
    block->ptr = (char*) ptr + sizeof(memoryblock_t);
    block->size = size + sizeof(memoryblock_t);
    #ifdef MEMDEBUG
    block->label = label;
    block->file = file;
    block->line = line;
    #endif // MEMDEBUG
    LinkMemoryBlock(block);
    allocatedmemory += block->size;
    totalmemorysize += block->size + sizeof(memoryblock_t);
    numblocks++;
    return block->ptr;
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetClearedHunkMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetClearedHunkMemory(uint32 size)
    #endif // MEMDEBUG
{
    void* ptr;
    #ifdef MEMDEBUG
    ptr = GetHunkMemoryDebug(size, label, file, line);
    #else
    ptr = GetHunkMemory(size);
    #endif // MEMDEBUG
    Com_Memset(ptr, 0, size);
    return ptr;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
memoryblock_t* BlockFromPointer(void* ptr, char* str)
{
    memoryblock_t* block;

    if (!ptr) {
    #ifdef MEMDEBUG
        // char *crash = (char *) NULL;
        // crash[0] = 1;
        botimport.Print(PRT_FATAL, "%s: NULL pointer\n", str);
    #endif // MEMDEBUG
        return NULL;
    }
    block = (memoryblock_t*) ((char*) ptr - sizeof(memoryblock_t));
    if (block->id != MEM_ID && block->id != HUNK_ID) {
        botimport.Print(PRT_FATAL, "%s: invalid memory block\n", str);
        return NULL;
    }
    if (block->ptr != ptr) {
        botimport.Print(PRT_FATAL, "%s: memory block pointer invalid\n", str);
        return NULL;
    }
    return block;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void FreeMemory(void* ptr)
{
    memoryblock_t* block;

    block = BlockFromPointer(ptr, "FreeMemory");
    if (!block) {
        return;
    }
    UnlinkMemoryBlock(block);
    allocatedmemory -= block->size;
    totalmemorysize -= block->size + sizeof(memoryblock_t);
    numblocks--;
    //
    if (block->id == MEM_ID) {
        botimport.FreeMemory(block);
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
int32 MemoryByteSize(void* ptr)
{
    memoryblock_t* block;

    block = BlockFromPointer(ptr, "MemoryByteSize");
    if (!block) {
        return 0;
    }
    return block->size;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void PrintUsedMemorySize()
{
    botimport.Print(PRT_MESSAGE, "total allocated memory: %d KB\n", allocatedmemory >> 10);
    botimport.Print(PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10);
    botimport.Print(PRT_MESSAGE, "total memory blocks: %d\n", numblocks);
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void PrintMemoryLabels()
{
    memoryblock_t* block;
    int32          i;

    PrintUsedMemorySize();
    i = 0;
    Log_Write("============= Botlib memory log ==============\r\n");
    Log_Write("\r\n");
    for (block = memory; block; block = block->next) {
    #ifdef MEMDEBUG
        if (block->id == HUNK_ID) {
            Log_Write("%6d, hunk %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
        } else {
            Log_Write("%6d,      %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
        }
    #endif // MEMDEBUG
        i++;
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void DumpMemory()
{
    memoryblock_t* block;

    for (block = memory; block; block = memory) {
        FreeMemory(block->ptr);
    }
    totalmemorysize = 0;
    allocatedmemory = 0;
}

#else

    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetMemory(uint32 size)
    #endif // MEMDEBUG
{
    void*   ptr;
    uint32* memid;

    ptr = botimport.GetMemory(size + sizeof(uint32));
    if (!ptr) {
        return NULL;
    }
    memid = (uint32*) ptr;
    *memid = MEM_ID;
    return (uint32*) ((char*) ptr + sizeof(uint32));
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetClearedMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetClearedMemory(uint32 size)
    #endif // MEMDEBUG
{
    void* ptr;
    #ifdef MEMDEBUG
    ptr = GetMemoryDebug(size, label, file, line);
    #else
    ptr = GetMemory(size);
    #endif // MEMDEBUG
    Com_Memset(ptr, 0, size);
    return ptr;
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetHunkMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetHunkMemory(uint32 size)
    #endif // MEMDEBUG
{
    void*   ptr;
    uint32* memid;

    ptr = botimport.HunkAlloc(size + sizeof(uint32));
    if (!ptr) {
        return NULL;
    }
    memid = (uint32*) ptr;
    *memid = HUNK_ID;
    return (uint32*) ((char*) ptr + sizeof(uint32));
}
    //===========================================================================
    //
    // Parameter:            -
    // Returns:                -
    // Changes Globals:        -
    //===========================================================================
    #ifdef MEMDEBUG
void* GetClearedHunkMemoryDebug(uint32 size, char* label, char* file, int32 line)
    #else
void* GetClearedHunkMemory(uint32 size)
    #endif // MEMDEBUG
{
    void* ptr;
    #ifdef MEMDEBUG
    ptr = GetHunkMemoryDebug(size, label, file, line);
    #else
    ptr = GetHunkMemory(size);
    #endif // MEMDEBUG
    Com_Memset(ptr, 0, size);
    return ptr;
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void FreeMemory(void* ptr)
{
    uint32* memid;

    memid = (uint32*) ((char*) ptr - sizeof(uint32));

    if (*memid == MEM_ID) {
        botimport.FreeMemory(memid);
    }
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void PrintUsedMemorySize()
{
}
//===========================================================================
//
// Parameter:            -
// Returns:                -
// Changes Globals:        -
//===========================================================================
void PrintMemoryLabels()
{
}

#endif
