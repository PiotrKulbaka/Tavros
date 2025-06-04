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

// qcommon.h -- definitions common between client and server, but not game.orient ref modules
#include "../qcommon/cm_public.h"

#include <tavros/core/prelude.hpp>
#include <qcommon/qcommon.h>
#include <game/q_shared.h>

//============================================================================

//
// msg.c
//
typedef struct
{
    bool   allowoverflow; // if false, do a Com_Error
    bool   overflowed;    // set to true if the buffer size failed (with allowoverflow set)
    bool   oob;           // set to true if the buffer size failed (with allowoverflow set)
    uint8* data;
    int32  maxsize;
    int32  cursize;
    int32  readcount;
    int32  bit; // for bitwise reads and writes
} msg_t;

void MSG_Init(msg_t* buf, uint8* data, int32 length);
void MSG_InitOOB(msg_t* buf, uint8* data, int32 length);
void MSG_Clear(msg_t* buf);
void MSG_WriteData(msg_t* buf, const void* data, int32 length);
void MSG_Bitstream(msg_t* buf);

// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void MSG_Copy(msg_t* buf, uint8* data, int32 length, msg_t* src);

struct entityState_s;
struct playerState_s;

void MSG_WriteBits(msg_t* msg, int32 value, int32 bits);

void MSG_WriteByte(msg_t* sb, int32 c);
void MSG_WriteShort(msg_t* sb, int32 c);
void MSG_WriteLong(msg_t* sb, int32 c);
void MSG_WriteString(msg_t* sb, const char* s);
void MSG_WriteBigString(msg_t* sb, const char* s);

void MSG_BeginReadingOOB(msg_t* sb);

int32 MSG_ReadBits(msg_t* msg, int32 bits);

int32 MSG_ReadByte(msg_t* sb);
int32 MSG_ReadShort(msg_t* sb);
int32 MSG_ReadLong(msg_t* sb);
char* MSG_ReadString(msg_t* sb);
char* MSG_ReadBigString(msg_t* sb);
char* MSG_ReadStringLine(msg_t* sb);
void  MSG_ReadData(msg_t* sb, void* buffer, int32 size);

void MSG_WriteDeltaUsercmdKey(msg_t* msg, int32 key, usercmd_t* from, usercmd_t* to);
void MSG_ReadDeltaUsercmdKey(msg_t* msg, int32 key, usercmd_t* from, usercmd_t* to);

void MSG_WriteDeltaEntity(msg_t* msg, struct entityState_s* from, struct entityState_s* to, bool force);
void MSG_ReadDeltaEntity(msg_t* msg, entityState_t* from, entityState_t* to, int32 number);

void MSG_WriteDeltaPlayerstate(msg_t* msg, struct playerState_s* from, struct playerState_s* to);


void MSG_ReportChangeVectors_f();

//============================================================================

/*
==============================================================

NET

==============================================================
*/

#define PACKET_BACKUP         32 // number of old messages that must be kept on client and
                                 // server for delta comrpession and ping estimation
#define PACKET_MASK           (PACKET_BACKUP - 1)

#define MAX_PACKET_USERCMDS   32 // max number of usercmd_t in a packet

#define PORT_ANY              -1

#define MAX_RELIABLE_COMMANDS 64 // max string commands buffered for restransmit

typedef enum
{
    NA_BOT,
    NA_BAD, // an address lookup failed
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IPX,
    NA_BROADCAST_IPX
} netadrtype_t;

typedef enum
{
    NS_CLIENT,
    NS_SERVER
} netsrc_t;

typedef struct
{
    netadrtype_t type;

    uint8 ip[4];
    uint8 ipx[10];

    uint16 port;
} netadr_t;

void NET_Init();
void NET_Restart();
void NET_Config(bool enableNetworking);

void       NET_SendPacket(netsrc_t sock, int32 length, const void* data, netadr_t to);
void QDECL NET_OutOfBandPrint(netsrc_t net_socket, netadr_t adr, const char* format, ...);
void QDECL NET_OutOfBandData(netsrc_t sock, netadr_t adr, uint8* format, int32 len);

bool        NET_CompareAdr(netadr_t a, netadr_t b);
bool        NET_CompareBaseAdr(netadr_t a, netadr_t b);
bool        NET_IsLocalAddress(netadr_t adr);
const char* NET_AdrToString(netadr_t a);
bool        NET_StringToAdr(const char* s, netadr_t* a);
bool        NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, msg_t* net_message);
void        NET_Sleep(int32 msec);


#define MAX_MSGLEN           16384 // max length of a message, which may
                                   // be fragmented into multiple packets

#define MAX_DOWNLOAD_WINDOW  8     // max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE 2048  // 2048 uint8 block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct
{
    netsrc_t sock;

    int32 dropped; // between last packet and previous

    netadr_t remoteAddress;
    int32    qport; // qport value to write when transmitting

    // sequencing variables
    int32 incomingSequence;
    int32 outgoingSequence;

    // incoming fragment assembly buffer
    int32 fragmentSequence;
    int32 fragmentLength;
    uint8 fragmentBuffer[MAX_MSGLEN];

    // outgoing fragment buffer
    // we need to space out the sending of large fragmented messages
    bool  unsentFragments;
    int32 unsentFragmentStart;
    int32 unsentLength;
    uint8 unsentBuffer[MAX_MSGLEN];
} netchan_t;

void Netchan_Init(int32 qport);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int32 qport);

void Netchan_Transmit(netchan_t* chan, int32 length, const uint8* data);
void Netchan_TransmitNextFragment(netchan_t* chan);

bool Netchan_Process(netchan_t* chan, msg_t* msg);


/*
==============================================================

PROTOCOL

==============================================================
*/

#define PROTOCOL_VERSION 68
// 1.31 - 67

// maintain a list of compatible protocols for demo playing
// NOTE: that stuff only works with two digits protocols
extern int32 demo_protocols[];

#define UPDATE_SERVER_NAME "update.quake3arena.com"
// override on command line, config files etc.
#ifndef MASTER_SERVER_NAME
    #define MASTER_SERVER_NAME "master.quake3arena.com"
#endif
#ifndef AUTHORIZE_SERVER_NAME
    #define AUTHORIZE_SERVER_NAME "authorize.quake3arena.com"
#endif

#define PORT_MASTER 27950
#define PORT_UPDATE 27951
#ifndef PORT_AUTHORIZE
    #define PORT_AUTHORIZE 27952
#endif
#define PORT_SERVER      27960
#define NUM_SERVER_PORTS 4 // broadcast scan this many ports after
                           // PORT_SERVER so a single machine can
                           // run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e
{
    svc_bad,
    svc_nop,
    svc_gamestate,
    svc_configstring,  // [int16] [string] only in gamestate messages
    svc_baseline,      // only in gamestate messages
    svc_serverCommand, // [string] to be executed by client game module
    svc_download,      // [int16] size [size bytes]
    svc_snapshot,
    svc_EOF
};


//
// client to server
//
enum clc_ops_e
{
    clc_bad,
    clc_nop,
    clc_move,          // [[usercmd_t]
    clc_moveNoDelta,   // [[usercmd_t]
    clc_clientCommand, // [string] message
    clc_EOF
};

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but entire text
files can be execed.

*/

void Cbuf_Init();
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText(const char* text);
// Adds command text at the end of the buffer, does NOT add a final \n

void Cbuf_ExecuteText(int32 exec_when, const char* text);
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute();
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function, or current args will be destroyed.

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t)();

void Cmd_Init();

void Cmd_AddCommand(const char* cmd_name, xcommand_t function);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_clientCommand instead of executed locally

void Cmd_RemoveCommand(const char* cmd_name);

void Cmd_CommandCompletion(void (*callback)(const char* s));
// callback with each valid string

int32       Cmd_Argc();
const char* Cmd_Argv(int32 arg);
void        Cmd_ArgvBuffer(int32 arg, char* buffer, int32 bufferLength);
char*       Cmd_Args();
char*       Cmd_ArgsFrom(int32 arg);
void        Cmd_ArgsBuffer(char* buffer, int32 bufferLength);
char*       Cmd_Cmd();
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

void Cmd_TokenizeString(const char* text);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void Cmd_ExecuteString(const char* text);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console


/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold scalar or string variables that can be changed
or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder            prints the current value
r_draworder 0        sets the current value to 0
set r_draworder 0    as above, but creates the cvar if not present

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

The are also occasionally used to communicated information between different
modules of the program.

*/

cvar_t* Cvar_Get(const char* var_name, const char* value, int32 flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

void Cvar_Set(const char* var_name, const char* value);
// will create the variable with no flags if it doesn't exist

void Cvar_SetLatched(const char* var_name, const char* value);
// don't set the cvar immediately

void Cvar_SetValue(const char* var_name, float value);
// expands value to a string and calls Cvar_Set

float Cvar_VariableValue(const char* var_name);
int32 Cvar_VariableIntegerValue(const char* var_name);
// returns 0 if not defined or non numeric

const char* Cvar_VariableString(const char* var_name);
void        Cvar_VariableStringBuffer(const char* var_name, char* buffer, int32 bufsize);
// returns an empty string if not defined

void Cvar_CommandCompletion(void (*callback)(const char* s));
// callback with each valid string

void Cvar_Reset(const char* var_name);

void Cvar_SetCheatState();
// reset all testing vars to a safe value

bool Cvar_Command();
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void Cvar_WriteVariables(fileHandle_t f);
// writes lines containing "set variable value" for all variables
// with the archive flag set to true.

void Cvar_Init();

char* Cvar_InfoString(int32 bit);
char* Cvar_InfoString_Big(int32 bit);

void Cvar_Restart_f();

extern int32 cvar_modifiedFlags;
// whenever a cvar is modifed, its flags will be OR'd into this, so
// a single check can determine if any CVAR_USERINFO, CVAR_SERVERINFO,
// etc, variables have been modified since the last check.  The bit
// can then be cleared to allow another change detection.

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF   0x01
#define FS_UI_REF        0x02
#define FS_CGAME_REF     0x04
#define FS_QAGAME_REF    0x08
// number of id paks that will never be autodownloaded from baseq3
#define NUM_ID_PAKS      9

#define MAX_FILE_HANDLES 64

#define BASEGAME         "baseq3"

void FS_InitFilesystem();
void FS_Shutdown(bool closemfp);

bool FS_ConditionalRestart(int32 checksumFeed);
void FS_Restart(int32 checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

char** FS_ListFiles(const char* directory, const char* extension, int32* numfiles);
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void FS_FreeFileList(char** list);

bool FS_FileExists(const char* file);

int32 FS_GetFileList(const char* path, const char* extension, char* listbuf, int32 bufsize);
int32 FS_GetModList(char* listbuf, int32 bufsize);

fileHandle_t FS_FOpenFileWrite(const char* qpath);
// will properly create any needed paths and deal with seperater character issues

int32        FS_filelength(fileHandle_t f);
fileHandle_t FS_SV_FOpenFileWrite(const char* filename);
int32        FS_SV_FOpenFileRead(const char* filename, fileHandle_t* fp);
void         FS_SV_Rename(const char* from, const char* to);
int32        FS_FOpenFileRead(const char* qpath, fileHandle_t* file, bool uniqueFILE);
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int32 FS_FileIsInPAK(const char* filename, int32* pChecksum);
// returns 1 if a file is in the PAK file, otherwise -1

int32 FS_Write(const void* buffer, int32 len, fileHandle_t f);

int32 FS_Read2(void* buffer, int32 len, fileHandle_t f);
int32 FS_Read(void* buffer, int32 len, fileHandle_t f);
// properly handles partial reads and reads from other dlls

void FS_FCloseFile(fileHandle_t f);
// note: you can't just fclose from another DLL, due to MS libc issues

int32 FS_ReadFile(const char* qpath, void** buffer);
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 uint8 will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void FS_FreeFile(void* buffer);
// frees the memory returned by FS_ReadFile

void FS_WriteFile(const char* qpath, const void* buffer, int32 size);
// writes a complete file, creating any subdirectories needed

int32 FS_filelength(fileHandle_t f);
// doesn't work for files that are opened from a pack file

int32 FS_FTell(fileHandle_t f);
// where are we?

void FS_Flush(fileHandle_t f);

void QDECL FS_Printf(fileHandle_t f, const char* fmt, ...);
// like fprintf

int32 FS_FOpenFileByMode(const char* qpath, fileHandle_t* f, fsMode_t mode);
// opens a file for reading, writing, or appending depending on the value of mode

int32 FS_Seek(fileHandle_t f, int32 offset, int32 origin);
// seek on a file (doesn't work for zip files!!!!!!!!)

bool FS_FilenameCompare(const char* s1, const char* s2);

const char* FS_LoadedPakNames();
const char* FS_LoadedPakChecksums();
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char* FS_ReferencedPakNames();
const char* FS_ReferencedPakChecksums();
const char* FS_ReferencedPakPureChecksums();
// Returns a space separated string containing the checksums of all loaded
// AND referenced pk3 files. Servers with sv_pure set will get this string
// back from clients for pure validation

void FS_ClearPakReferences(int32 flags);
// clears referenced booleans on loaded pk3s

void FS_PureServerSetReferencedPaks(const char* pakSums, const char* pakNames);
void FS_PureServerSetLoadedPaks(const char* pakSums, const char* pakNames);
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

bool FS_idPak(const char* pak, const char* base);
bool FS_ComparePaks(char* neededpaks, int32 len, bool dlstring);

/*
==============================================================

Edit fields and command line history/completion

==============================================================
*/

#define MAX_EDIT_LINE 256
typedef struct
{
    int32 cursor;
    int32 scroll;
    int32 widthInChars;
    char  buffer[MAX_EDIT_LINE];
} field_t;

void Field_Clear(field_t* edit);
void Field_CompleteCommand(field_t* edit);

/*
==============================================================

MISC

==============================================================
*/

// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
#define Q_vsnprintf _vsnprintf

// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define MAXPRINTMSG 4096

char* CopyString(const char* in);

void       Com_BeginRedirect(char* buffer, int32 buffersize, void (*flush)(char*));
void       Com_EndRedirect();
void QDECL Com_Error(int32 code, const char* fmt, ...);
void       Com_Quit_f();
int32      Com_EventLoop();
int32      Com_Milliseconds();
uint32     Com_BlockChecksum(const void* buffer, int32 length);
uint32     Com_BlockChecksumKey(void* buffer, int32 length, int32 key);
int32      Com_HashKey(char* string, int32 maxlen);
int32      Com_Filter(const char* filter, char* name, int32 casesensitive);
int32      Com_FilterPath(const char* filter, const char* name, int32 casesensitive);
bool       Com_SafeMode();

void Com_StartupVariable(const char* match);
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

extern cvar_t* com_dedicated;
extern cvar_t* com_speeds;
extern cvar_t* com_timescale;
extern cvar_t* com_sv_running;
extern cvar_t* com_cl_running;
extern cvar_t* com_version;
extern cvar_t* com_cameraMode;

// both client and server must agree to pause
extern cvar_t* cl_paused;
extern cvar_t* sv_paused;

// com_speeds times
extern int32 time_game;
extern int32 time_frontend;
extern int32 time_backend; // renderer backend time

extern int32 com_frameTime;
extern int32 com_frameMsec;

extern bool com_errorEntered;

#ifdef DEBUG
    #define Z_TagMalloc(size, tag) Z_TagMallocDebug(size, tag, __FILE__, __LINE__)
    #define Z_Malloc(size)         Z_MallocDebug(size, __FILE__, __LINE__)
void* Z_TagMallocDebug(int32 size, const char* tag, const char* file, int32 line); // NOT 0 filled memory
void* Z_MallocDebug(int32 size, const char* file, int32 line);                     // returns 0 filled memory
#else
void* Z_TagMalloc(int32 size, const char* tag); // NOT 0 filled memory
void* Z_Malloc(int32 size);                     // returns 0 filled memory
#endif
void Z_Free(void* ptr);

void  Hunk_Clear();
void  Hunk_ClearToMark();
void  Hunk_SetMark();
bool  Hunk_CheckMark();
void  Hunk_ClearTempMemory();
void* Hunk_AllocateTempMemory(int32 size);
void  Hunk_FreeTempMemory(void* buf);

// commandLine should not include the executable name (argv[0])
void Com_Init(char* commandLine);
void Com_Frame();


/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands();
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init();
void CL_Disconnect(bool showMainMenu);
void CL_Shutdown();
void CL_Frame(int32 msec);
bool CL_GameCommand();
void CL_KeyEvent(int32 key, bool down, uint32 time);

void CL_CharEvent(int32 key);
// char events are for field typing, not game control

void CL_MouseEvent(int32 dx, int32 dy, int32 time);

void CL_PacketEvent(netadr_t from, msg_t* msg);

void CL_MapLoading();
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void CL_ForwardCommandToServer(const char* string);
// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_ShutdownAll();
// shutdown all the client stuff

void CL_FlushMemory();
// dump all memory on an error

void CL_StartHunkUsers();
// start all the client stuff using the hunk

void Key_WriteBindings(fileHandle_t f);
// for writing the config files

void S_ClearSoundBuffer();
// call before filesystem access

void SCR_DebugGraph(float value, int32 color); // FIXME: move logging to common?


//
// server interface
//
void SV_Init();
void SV_Shutdown(const char* finalmsg);
void SV_Frame(int32 msec);
void SV_PacketEvent(netadr_t from, msg_t* msg);
bool SV_GameCommand();


//
// UI interface
//
bool UI_GameCommand();

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

typedef enum
{
    SE_NONE = 0, // evTime is still valid
    SE_KEY,      // evValue is a key code, evValue2 is the down flag
    SE_CHAR,     // evValue is an ascii char
    SE_MOUSE,    // evValue and evValue2 are reletive signed x / y moves
    SE_CONSOLE,  // evPtr is a char*
    SE_PACKET    // evPtr is a netadr_t followed by data bytes to evPtrLength
} sysEventType_t;

typedef struct
{
    int32          evTime;
    sysEventType_t evType;
    int32          evValue, evValue2;
    int32          evPtrLength; // bytes of data pointed to by evPtr
    void*          evPtr;       // this must be manually freed if not NULL
} sysEvent_t;

sysEvent_t Sys_GetEvent();

void Sys_Init();

char* Sys_GetCurrentUser();

void QDECL Sys_Error(const char* error, ...);
void       Sys_Quit();
char*      Sys_GetClipboardData();

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int32 Sys_Milliseconds();

void Sys_SendPacket(int32 length, const void* data, netadr_t to);

bool Sys_StringToAdr(const char* s, netadr_t* a);
// Does NOT parse port numbers, only base addresses.

bool Sys_IsLANAddress(netadr_t adr);
void Sys_ShowIP();

void        Sys_Mkdir(const char* path);
const char* Sys_DefaultInstallPath();
const char* Sys_DefaultHomePath();

const char** Sys_ListFiles(const char* directory, const char* extension, const char* filter, int32* numfiles, bool wantsubs);
void         Sys_FreeFileList(char** list);

void SetPlaneSignbits(cplane_t* out);


/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */

#define NYT           HMAX /* NYT = Not Yet Transmitted */
#define INTERNAL_NODE (HMAX + 1)

typedef struct nodetype
{
    struct nodetype * left, *right, *parent; /* tree structure */
    struct nodetype * next, *prev;           /* doubly-linked list */
    struct nodetype** head;                  /* highest ranked node in block */
    int32             weight;
    int32             symbol;
} node_t;

#define HMAX 256 /* Maximum symbol */

typedef struct
{
    int32 blocNode;
    int32 blocPtrs;

    node_t*  tree;
    node_t*  lhead;
    node_t*  ltail;
    node_t*  loc[HMAX + 1];
    node_t** freelist;

    node_t  nodeList[768];
    node_t* nodePtrs[768];
} huff_t;

typedef struct
{
    huff_t compressor;
    huff_t decompressor;
} huffman_t;

void  Huff_Compress(msg_t* buf, int32 offset);
void  Huff_Decompress(msg_t* buf, int32 offset);
void  Huff_Init(huffman_t* huff);
void  Huff_addRef(huff_t* huff, uint8 ch);
int32 Huff_Receive(node_t* node, int32* ch, uint8* fin);
void  Huff_transmit(huff_t* huff, int32 ch, uint8* fout);
void  Huff_offsetReceive(node_t* node, int32* ch, uint8* fin, int32* offset);
void  Huff_offsetTransmit(huff_t* huff, int32 ch, uint8* fout, int32* offset);
void  Huff_putBit(int32 bit, uint8* fout, int32* offset);
int32 Huff_getBit(uint8* fout, int32* offset);

#define SV_ENCODE_START 4
#define SV_DECODE_START 12
#define CL_ENCODE_START 12
#define CL_DECODE_START 4
