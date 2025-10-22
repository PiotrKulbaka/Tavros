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
// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include <setjmp.h>

#include <winsock.h>

static tavros::core::logger logger("common");

int32 demo_protocols[] =
    {66, 67, 68, 0};

jmp_buf abortframe; // an ERR_DROP occured, exit the entire frame

cvar_t* com_speeds;
cvar_t* com_dedicated;
cvar_t* com_timescale;
cvar_t* com_fixedtime;
cvar_t* com_timedemo;
cvar_t* com_sv_running;
cvar_t* com_cl_running;
cvar_t* com_showtrace;
cvar_t* com_version;
cvar_t* cl_paused;
cvar_t* sv_paused;
cvar_t* com_cameraMode;

// com_speeds times
int32 time_game;
int32 time_frontend; // renderer frontend time
int32 time_backend;  // renderer backend time

int32 com_frameTime;
int32 com_frameMsec;
int32 com_frameNumber;

bool com_errorEntered;
bool com_fullyInitialized;

char com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f();

//============================================================================

static char* rd_buffer;
static int32 rd_buffersize;
static void  (*rd_flush)(char* buffer);

void Com_BeginRedirect(char* buffer, int32 buffersize, void (*flush)(char*))
{
    if (!buffer || !buffersize || !flush) {
        return;
    }
    rd_buffer = buffer;
    rd_buffersize = buffersize;
    rd_flush = flush;

    *rd_buffer = 0;
}

void Com_EndRedirect()
{
    if (rd_flush) {
        rd_flush(rd_buffer);
    }

    rd_buffer = NULL;
    rd_buffersize = 0;
    rd_flush = NULL;
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void QDECL Com_Error(int32 code, const char* fmt, ...)
{
    va_list      argptr;
    static int32 lastErrorTime;
    static int32 errorCount;
    int32        currentTime;

    if (code != ERR_DISCONNECT) {
        TAV_ASSERT(false);
    }

    // make sure we can get at our local stuff
    FS_PureServerSetLoadedPaks("", "");

    // if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
    currentTime = Sys_Milliseconds();
    if (currentTime - lastErrorTime < 100) {
        if (++errorCount > 3) {
            code = ERR_FATAL;
        }
    } else {
        errorCount = 0;
    }
    lastErrorTime = currentTime;

    if (com_errorEntered) {
        Sys_Error("recursive error after: %s", com_errorMessage);
    }
    com_errorEntered = true;

    va_start(argptr, fmt);
    vsprintf(com_errorMessage, fmt, argptr);
    va_end(argptr);

    if (code != ERR_DISCONNECT) {
        Cvar_Set("com_errorMessage", com_errorMessage);
    }

    if (code == ERR_SERVERDISCONNECT) {
        CL_Disconnect(true);
        CL_FlushMemory();
        com_errorEntered = false;
        longjmp(abortframe, -1);
    } else if (code == ERR_DROP || code == ERR_DISCONNECT) {
        logger.info("********************\nERROR: {}\n********************\n", com_errorMessage);
        SV_Shutdown(va("Server crashed: %s\n", com_errorMessage));
        CL_Disconnect(true);
        CL_FlushMemory();
        com_errorEntered = false;
        longjmp(abortframe, -1);
    } else {
        CL_Shutdown();
        SV_Shutdown(va("Server fatal crashed: %s\n", com_errorMessage));
    }

    Sys_Error("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f()
{
    // don't try to shutdown if we are in a recursive error
    if (!com_errorEntered) {
        SV_Shutdown("Server quit\n");
        CL_Shutdown();
        FS_Shutdown(true);
    }
    Sys_Quit();
}


/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES 32
int32 com_numConsoleLines;
char* com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine(char* commandLine)
{
    int32 inq = 0;
    com_consoleLines[0] = commandLine;
    com_numConsoleLines = 1;

    while (*commandLine) {
        if (*commandLine == '"') {
            inq = !inq;
        }
        // look for a + seperating character
        // if commandLine came from a file, we might have real line seperators
        if ((*commandLine == '+' && !inq) || *commandLine == '\n' || *commandLine == '\r') {
            if (com_numConsoleLines == MAX_CONSOLE_LINES) {
                return;
            }
            com_consoleLines[com_numConsoleLines] = commandLine + 1;
            com_numConsoleLines++;
            *commandLine = 0;
        }
        commandLine++;
    }
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of q3config.cfg
===================
*/
bool Com_SafeMode()
{
    int32 i;

    for (i = 0; i < com_numConsoleLines; i++) {
        Cmd_TokenizeString(com_consoleLines[i]);
        if (!Q_stricmp(Cmd_Argv(0), "safe")
            || !Q_stricmp(Cmd_Argv(0), "cvar_restart")) {
            com_consoleLines[i][0] = 0;
            return true;
        }
    }
    return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable(const char* match)
{
    int32       i;
    const char* s;
    cvar_t*     cv;

    for (i = 0; i < com_numConsoleLines; i++) {
        Cmd_TokenizeString(com_consoleLines[i]);
        if (strcmp(Cmd_Argv(0), "set")) {
            continue;
        }

        s = Cmd_Argv(1);
        if (!match || !strcmp(s, match)) {
            Cvar_Set(s, Cmd_Argv(2));
            cv = Cvar_Get(s, "", 0);
            cv->flags |= CVAR_USER_CREATED;
            //            com_consoleLines[i] = 0;
        }
    }
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
bool Com_AddStartupCommands()
{
    int32 i;
    bool  added;

    added = false;
    // quote every token, so args with semicolons can work
    for (i = 0; i < com_numConsoleLines; i++) {
        if (!com_consoleLines[i] || !com_consoleLines[i][0]) {
            continue;
        }

        // set commands won't override menu startup
        if (Q_stricmpn(com_consoleLines[i], "set", 3)) {
            added = true;
        }
        Cbuf_AddText(com_consoleLines[i]);
        Cbuf_AddText("\n");
    }

    return added;
}


//============================================================================
/*
============
Com_StringContains
============
*/
char* Com_StringContains(char* str1, char* str2, int32 casesensitive)
{
    int32 len, i, j;

    len = strlen(str1) - strlen(str2);
    for (i = 0; i <= len; i++, str1++) {
        for (j = 0; str2[j]; j++) {
            if (casesensitive) {
                if (str1[j] != str2[j]) {
                    break;
                }
            } else {
                if (toupper(str1[j]) != toupper(str2[j])) {
                    break;
                }
            }
        }
        if (!str2[j]) {
            return str1;
        }
    }
    return NULL;
}

/*
============
Com_Filter
============
*/
int32 Com_Filter(const char* filter, char* name, int32 casesensitive)
{
    char  buf[MAX_TOKEN_CHARS];
    char* ptr;
    int32 i, found;

    while (*filter) {
        if (*filter == '*') {
            filter++;
            for (i = 0; *filter; i++) {
                if (*filter == '*' || *filter == '?') {
                    break;
                }
                buf[i] = *filter;
                filter++;
            }
            buf[i] = '\0';
            if (strlen(buf)) {
                ptr = Com_StringContains(name, buf, casesensitive);
                if (!ptr) {
                    return false;
                }
                name = ptr + strlen(buf);
            }
        } else if (*filter == '?') {
            filter++;
            name++;
        } else if (*filter == '[' && *(filter + 1) == '[') {
            filter++;
        } else if (*filter == '[') {
            filter++;
            found = false;
            while (*filter && !found) {
                if (*filter == ']' && *(filter + 1) != ']') {
                    break;
                }
                if (*(filter + 1) == '-' && *(filter + 2) && (*(filter + 2) != ']' || *(filter + 3) == ']')) {
                    if (casesensitive) {
                        if (*name >= *filter && *name <= *(filter + 2)) {
                            found = true;
                        }
                    } else {
                        if (toupper(*name) >= toupper(*filter) && toupper(*name) <= toupper(*(filter + 2))) {
                            found = true;
                        }
                    }
                    filter += 3;
                } else {
                    if (casesensitive) {
                        if (*filter == *name) {
                            found = true;
                        }
                    } else {
                        if (toupper(*filter) == toupper(*name)) {
                            found = true;
                        }
                    }
                    filter++;
                }
            }
            if (!found) {
                return false;
            }
            while (*filter) {
                if (*filter == ']' && *(filter + 1) != ']') {
                    break;
                }
                filter++;
            }
            filter++;
            name++;
        } else {
            if (casesensitive) {
                if (*filter != *name) {
                    return false;
                }
            } else {
                if (toupper(*filter) != toupper(*name)) {
                    return false;
                }
            }
            filter++;
            name++;
        }
    }
    return true;
}

/*
============
Com_FilterPath
============
*/
int32 Com_FilterPath(const char* filter, const char* name, int32 casesensitive)
{
    int32 i;
    char  new_filter[MAX_QPATH];
    char  new_name[MAX_QPATH];

    for (i = 0; i < MAX_QPATH - 1 && filter[i]; i++) {
        if (filter[i] == '\\' || filter[i] == ':') {
            new_filter[i] = '/';
        } else {
            new_filter[i] = filter[i];
        }
    }
    new_filter[i] = '\0';
    for (i = 0; i < MAX_QPATH - 1 && name[i]; i++) {
        if (name[i] == '\\' || name[i] == ':') {
            new_name[i] = '/';
        } else {
            new_name[i] = name[i];
        }
    }
    new_name[i] = '\0';
    return Com_Filter(new_filter, new_name, casesensitive);
}

/*
============
Com_HashKey
============
*/
int32 Com_HashKey(char* string, int32 maxlen)
{
    int32 register hash, i;

    hash = 0;
    for (i = 0; i < maxlen && string[i] != '\0'; i++) {
        hash += string[i] * (119 + i);
    }
    hash = (hash ^ (hash >> 10) ^ (hash >> 20));
    return hash;
}

/*
==============================================================================
                        ZONE MEMORY ALLOCATION
==============================================================================
*/

#include <memory>
#include <tavros/core/memory/mallocator.hpp>

std::unique_ptr<tavros::core::allocator> zallocator;

static void Z_Init()
{
    zallocator = std::make_unique<tavros::core::mallocator>();
}

void* Z_TagMalloc(int32 size, const char* tag)
{
    return zallocator->allocate(size, 8, tag);
}

void Z_Free(void* ptr)
{
    zallocator->deallocate(ptr);
}

void* Z_Malloc(int32 size)
{
    return Z_TagMalloc(size, "general");
}

/*
========================
CopyString

 NOTE:    never write over the memory CopyString returns because
        memory from a memstatic_t might be returned
========================
*/
char* CopyString(const char* in)
{
    char* out;
    out = (char*) Z_Malloc(strlen(in) + 1);
    strcpy(out, in);
    return out;
}

/*
==============================================================================
                         HUNK MEMORY ALLOCATION
==============================================================================
*/

std::unique_ptr<tavros::core::allocator> hallocator_marked;
std::unique_ptr<tavros::core::allocator> hallocator_other;
tavros::core::allocator*                 hallocator = nullptr;
std::unique_ptr<tavros::core::allocator> hallocator_temp;

/*
=================
Com_InitHunkMemory
=================
*/
static void Com_InitHunkMemory()
{
    hallocator_marked = std::make_unique<tavros::core::mallocator>();
    hallocator_other = std::make_unique<tavros::core::mallocator>();
    hallocator_temp = std::make_unique<tavros::core::mallocator>();
    hallocator = hallocator_marked.get();
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark()
{
    hallocator = hallocator_other.get();
}

/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark()
{
    hallocator_other->clear();
}

/*
=================
Hunk_CheckMark
=================
*/
bool Hunk_CheckMark()
{
    return hallocator == hallocator_other.get();
}

void CL_ShutdownCGame();
void CL_ShutdownUI();
void SV_ShutdownGameProgs();

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void Hunk_Clear()
{
    CL_ShutdownCGame();
    CL_ShutdownUI();
    SV_ShutdownGameProgs();

    hallocator = hallocator_marked.get();
    hallocator_marked->clear();
    hallocator_other->clear();
    hallocator_temp->clear();

    logger.info("Hunk_Clear: reset the hunk ok");
}

/*
=================
Hunk_Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
#ifdef HUNK_DEBUG
void* Hunk_AllocDebug(int32 size, ha_pref preference, const char* label, const char* file, int32 line)
{
#else
void* Hunk_Alloc(int32 size, ha_pref preference)
{
#endif
    if (hallocator == nullptr) {
        Com_Error(ERR_FATAL, "Hunk_Alloc: Hunk memory system not initialized");
    }
    return hallocator->allocate(size, 8);
}

/*
=================
Hunk_AllocateTempMemory
=================
*/
void* Hunk_AllocateTempMemory(int32 size)
{
    if (hallocator_temp) {
        return hallocator_temp->allocate(size, 8);
    }
    return zallocator->allocate(size, 8);
}

/*
==================
Hunk_FreeTempMemory
==================
*/
void Hunk_FreeTempMemory(void* buf)
{
    if (hallocator_temp) {
        return hallocator_temp->deallocate(buf);
    }
    zallocator->deallocate(buf);
}


/*
=================
Hunk_ClearTempMemory
=================
*/
void Hunk_ClearTempMemory()
{
    if (hallocator_temp) {
        hallocator_temp->clear();
    }
}

/*
===================================================================

EVENTS

===================================================================
*/

// bk001129 - here we go again: upped from 64
// FIXME TTimo blunt upping from 256 to 1024
#define MAX_PUSHED_EVENTS 1024
// bk001129 - init, also static
static int32 com_pushedEventsHead = 0;
static int32 com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent()
{
    // clear the static buffer array
    // this requires SE_NONE to be accepted as a valid but NOP event
    memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
    // reset counters while we are at it
    // beware: GetEvent might still return an SE_NONE from the buffer
    com_pushedEventsHead = 0;
    com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent(sysEvent_t* event)
{
    sysEvent_t*  ev;
    static int32 printedWarning = 0; // bk001129 - init, bk001204 - explicit int32

    ev = &com_pushedEvents[com_pushedEventsHead & (MAX_PUSHED_EVENTS - 1)];

    if (com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS) {
        // don't print the warning constantly, or it can give time for more...
        if (!printedWarning) {
            printedWarning = true;
            logger.warning("Com_PushEvent overflow");
        }

        if (ev->evPtr) {
            Z_Free(ev->evPtr);
        }
        com_pushedEventsTail++;
    } else {
        printedWarning = false;
    }

    *ev = *event;
    com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t Com_GetEvent()
{
    if (com_pushedEventsHead > com_pushedEventsTail) {
        com_pushedEventsTail++;
        return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
    }
    return Sys_GetEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket(netadr_t* evFrom, msg_t* buf)
{
    int32 t1, t2, msec;

    t1 = 0;

    if (com_speeds->integer) {
        t1 = Sys_Milliseconds();
    }

    SV_PacketEvent(*evFrom, buf);

    if (com_speeds->integer) {
        t2 = Sys_Milliseconds();
        msec = t2 - t1;
        if (com_speeds->integer == 3) {
            logger.info("SV_PacketEvent time: {}", msec);
        }
    }
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int32 Com_EventLoop()
{
    sysEvent_t ev;
    netadr_t   evFrom;
    uint8      bufData[MAX_MSGLEN];
    msg_t      buf;

    MSG_Init(&buf, bufData, sizeof(bufData));

    while (1) {
        ev = Com_GetEvent();

        // if no more events are available
        if (ev.evType == SE_NONE) {
            // manually send packet events for the loopback channel
            while (NET_GetLoopPacket(NS_CLIENT, &evFrom, &buf)) {
                CL_PacketEvent(evFrom, &buf);
            }

            while (NET_GetLoopPacket(NS_SERVER, &evFrom, &buf)) {
                // if the server just shut down, flush the events
                if (com_sv_running->integer) {
                    Com_RunAndTimeServerPacket(&evFrom, &buf);
                }
            }

            return ev.evTime;
        }


        switch (ev.evType) {
        default:
            // bk001129 - was ev.evTime
            Com_Error(ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType);
            break;
        case SE_NONE:
            break;
        case SE_KEY:
            CL_KeyEvent(ev.evValue, ev.evValue2, ev.evTime);
            break;
        case SE_CHAR:
            CL_CharEvent(ev.evValue);
            break;
        case SE_MOUSE:
            CL_MouseEvent(ev.evValue, ev.evValue2, ev.evTime);
            break;
        case SE_CONSOLE:
            Cbuf_AddText((char*) ev.evPtr);
            Cbuf_AddText("\n");
            break;
        case SE_PACKET:

            evFrom = *(netadr_t*) ev.evPtr;
            buf.cursize = ev.evPtrLength - sizeof(evFrom);

            // we must copy the contents of the message out, because
            // the event buffers are only large enough to hold the
            // exact payload, but channel messages need to be large
            // enough to hold fragment reassembly
            if ((uint32) buf.cursize > buf.maxsize) {
                logger.info("Com_EventLoop: oversize packet");
                continue;
            }
            Com_Memcpy(buf.data, (uint8*) ((netadr_t*) ev.evPtr + 1), buf.cursize);
            if (com_sv_running->integer) {
                Com_RunAndTimeServerPacket(&evFrom, &buf);
            } else {
                CL_PacketEvent(evFrom, &buf);
            }
            break;
        }

        // free any block data
        if (ev.evPtr) {
            Z_Free(ev.evPtr);
        }
    }

    return 0; // never reached
}

/*
================
Com_Milliseconds

Can be used for profiling
================
*/
int32 Com_Milliseconds()
{
    sysEvent_t ev;

    // get events and push them until we get a null event with the current time
    do {
        ev = Sys_GetEvent();
        if (ev.evType != SE_NONE) {
            Com_PushEvent(&ev);
        }
    } while (ev.evType != SE_NONE);

    return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f()
{
    if (Cmd_Argc() > 1) {
        Com_Error(ERR_DROP, "Testing drop error");
    } else {
        Com_Error(ERR_FATAL, "Testing fatal error");
    }
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f()
{
    float s;
    int32 start, now;

    if (Cmd_Argc() != 2) {
        logger.info("freeze <seconds>");
        return;
    }
    s = atof(Cmd_Argv(1));

    start = Com_Milliseconds();

    while (1) {
        now = Com_Milliseconds();
        if ((now - start) * 0.001 > s) {
            break;
        }
    }
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f()
{
    *(int32*) 0 = 0x12345678;
}

/*
=================
Com_Init
=================
*/
void Com_Init(char* commandLine)
{
    char* s;

    logger.info("{} {}", Q3_VERSION, __DATE__);

    if (setjmp(abortframe)) {
        Sys_Error("Error during initialization");
    }

    // bk001129 - do this before anything else decides to push events
    Com_InitPushEvent();

    Z_Init();
    Cvar_Init();

    // prepare enough of the subsystems to handle
    // cvar and command buffer management
    Com_ParseCommandLine(commandLine);

    //    Swap_Init ();
    Cbuf_Init();

    Cmd_Init();

    // override anything from the config files with command line args
    Com_StartupVariable(NULL);

    // get the developer cvar set as early as possible
    Com_StartupVariable("developer");

    // done early so bind command exists
    CL_InitKeyCommands();

    FS_InitFilesystem();

    Cbuf_AddText("exec default.cfg\n");

    // skip the q3config.cfg if "safe" is on the command line
    if (!Com_SafeMode()) {
        Cbuf_AddText("exec q3config.cfg\n");
    }

    Cbuf_AddText("exec autoexec.cfg\n");

    Cbuf_Execute();

    // override anything from the config files with command line args
    Com_StartupVariable(NULL);

    // get dedicated here for proper hunk megs initialization
    com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH);

    // allocate the stack based hunk allocator
    Com_InitHunkMemory();

    // if any archived cvars are modified after this, we will trigger a writing
    // of the config file
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;

    //
    // init commands and vars
    //
    com_timescale = Cvar_Get("timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO);
    com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
    com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
    com_speeds = Cvar_Get("com_speeds", "0", 0);
    com_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
    com_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

    cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
    sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
    com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
    com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);

    Cmd_AddCommand("error", Com_Error_f);
    Cmd_AddCommand("crash", Com_Crash_f);
    Cmd_AddCommand("freeze", Com_Freeze_f);

    Cmd_AddCommand("quit", Com_Quit_f);
    Cmd_AddCommand("changeVectors", MSG_ReportChangeVectors_f);
    Cmd_AddCommand("writeconfig", Com_WriteConfig_f);

    s = va("%s %s", Q3_VERSION, __DATE__);
    com_version = Cvar_Get("version", s, CVAR_ROM | CVAR_SERVERINFO);

    Sys_Init();
    Netchan_Init(Com_Milliseconds() & 0xffff); // pick a port value that should be nice and random

    SV_Init();

    com_dedicated->modified = false;
    if (!com_dedicated->integer) {
        CL_Init();
    }

    // set com_frameTime so that if a map is started on the
    // command line it will still be able to count on com_frameTime
    // being random enough for a serverid
    com_frameTime = Com_Milliseconds();

    // add + commands from command line
    if (!Com_AddStartupCommands()) {
        // if the user didn't give any commands, run default action
        if (!com_dedicated->integer) {
            Cvar_Set("nextmap", "");
        }
    }

    CL_StartHunkUsers();

    // make sure single player is off by default
    Cvar_Set("ui_singlePlayerActive", "0");

    com_fullyInitialized = true;
    logger.info("--- Common Initialization Complete ---");
}

//==================================================================

void Com_WriteConfigToFile(const char* filename)
{
    fileHandle_t f;

    f = FS_FOpenFileWrite(filename);
    if (!f) {
        logger.info("Couldn't write {}.", filename);
        return;
    }

    FS_Printf(f, "// generated by quake, do not modify\n");
    Key_WriteBindings(f);
    Cvar_WriteVariables(f);
    FS_FCloseFile(f);
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration()
{
    // if we are quiting without fully initializing, make sure
    // we don't write out anything
    if (!com_fullyInitialized) {
        return;
    }

    if (!(cvar_modifiedFlags & CVAR_ARCHIVE)) {
        return;
    }
    cvar_modifiedFlags &= ~CVAR_ARCHIVE;

    Com_WriteConfigToFile("q3config.cfg");
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f()
{
    char filename[MAX_QPATH];

    if (Cmd_Argc() != 2) {
        logger.info("Usage: writeconfig <filename>");
        return;
    }

    Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
    COM_DefaultExtension(filename, sizeof(filename), ".cfg");
    logger.info("Writing {}.", filename);
    Com_WriteConfigToFile(filename);
}

/*
================
Com_ModifyMsec
================
*/
int32 Com_ModifyMsec(int32 msec)
{
    int32 clampTime;

    //
    // modify time for debugging values
    //
    if (com_fixedtime->integer) {
        msec = com_fixedtime->integer;
    } else if (com_timescale->value) {
        msec *= com_timescale->value;
    } else if (com_cameraMode->integer) {
        msec *= com_timescale->value;
    }

    // don't let it scale below 1 msec
    if (msec < 1 && com_timescale->value) {
        msec = 1;
    }

    if (com_dedicated->integer) {
        // dedicated servers don't want to clamp for a much longer
        // period, because it would mess up all the client's views
        // of time.
        if (msec > 500) {
            logger.info("Hitch warning: {} msec frame time", msec);
        }
        clampTime = 5000;
    } else if (!com_sv_running->integer) {
        // clients of remote servers do not want to clamp time, because
        // it would skew their view of the server's time temporarily
        clampTime = 5000;
    } else {
        // for local single player gaming
        // we may want to clamp the time to prevent players from
        // flying off edges when something hitches.
        clampTime = 200;
    }

    if (msec > clampTime) {
        msec = clampTime;
    }

    return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame()
{
    int32        msec;
    static int32 lastTime;
    int32        key;

    int32 timeBeforeFirstEvents;
    int32 timeBeforeServer;
    int32 timeBeforeEvents;
    int32 timeBeforeClient;
    int32 timeAfter;


    if (setjmp(abortframe)) {
        return; // an ERR_DROP was thrown
    }

    // bk001204 - init to zero.
    //  also:  might be clobbered by `longjmp' or `vfork'
    timeBeforeFirstEvents = 0;
    timeBeforeServer = 0;
    timeBeforeEvents = 0;
    timeBeforeClient = 0;
    timeAfter = 0;


    // old net chan encryption key
    key = 0x87243987;

    // write config file if anything changed
    Com_WriteConfiguration();

    //
    // main event loop
    //
    if (com_speeds->integer) {
        timeBeforeFirstEvents = Sys_Milliseconds();
    }

    // we may want to spin here if things are going too fast

    do {
        com_frameTime = Com_EventLoop();
        if (lastTime > com_frameTime) {
            lastTime = com_frameTime; // possible on first frame
        }
        msec = com_frameTime - lastTime;
        if (msec < 1) {
            Sleep(0);
        }
    } while (msec < 1);
    Cbuf_Execute();

    lastTime = com_frameTime;

    // mess with msec if needed
    com_frameMsec = msec;
    msec = Com_ModifyMsec(msec);

    //
    // server side
    //
    if (com_speeds->integer) {
        timeBeforeServer = Sys_Milliseconds();
    }

    SV_Frame(msec);

    // if "dedicated" has been modified, start up
    // or shut down the client system.
    // Do this after the server may have started,
    // but before the client tries to auto-connect
    if (com_dedicated->modified) {
        // get the latched value
        Cvar_Get("dedicated", "0", 0);
        com_dedicated->modified = false;
        if (!com_dedicated->integer) {
            CL_Init();
        } else {
            CL_Shutdown();
        }
    }

    //
    // client system
    //
    if (!com_dedicated->integer) {
        //
        // run event loop a second time to get server to client packets
        // without a frame of latency
        //
        if (com_speeds->integer) {
            timeBeforeEvents = Sys_Milliseconds();
        }
        Com_EventLoop();
        Cbuf_Execute();


        //
        // client side
        //
        if (com_speeds->integer) {
            timeBeforeClient = Sys_Milliseconds();
        }

        CL_Frame(msec);

        if (com_speeds->integer) {
            timeAfter = Sys_Milliseconds();
        }
    }

    //
    // report timing information
    //
    if (com_speeds->integer) {
        int32 all, sv, ev, cl;

        all = timeAfter - timeBeforeServer;
        sv = timeBeforeEvents - timeBeforeServer;
        ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
        cl = timeAfter - timeBeforeClient;
        sv -= time_game;
        cl -= time_frontend + time_backend;

        logger.info("frame:{} all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i", com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend);
    }

    //
    // trace optimization tracking
    //
    if (com_showtrace->integer) {
        extern int32 c_traces, c_brush_traces, c_patch_traces;
        extern int32 c_pointcontents;

        logger.info("%4i traces  (%ib %ip) %4i points", c_traces, c_brush_traces, c_patch_traces, c_pointcontents);
        c_traces = 0;
        c_brush_traces = 0;
        c_patch_traces = 0;
        c_pointcontents = 0;
    }

    // old net chan encryption key
    key = lastTime * 0x87243987;

    com_frameNumber++;
}

void Com_Memcpy(void* dest, const void* src, const size_t count)
{
    memcpy(dest, src, count);
}

void Com_Memset(void* dest, const int32 val, const size_t count)
{
    memset(dest, val, count);
}

//------------------------------------------------------------------------


/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int32 i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

    This should go in q_math but it is too late to add new traps
    to game and ui
=====================
*/
float Q_acos(float c)
{
    float angle;

    angle = acos(c);

    if (angle > M_PI) {
        return (float) M_PI;
    }
    if (angle < -M_PI) {
        return (float) M_PI;
    }
    return angle;
}

/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear(field_t* edit)
{
    memset(edit->buffer, 0, MAX_EDIT_LINE);
    edit->cursor = 0;
    edit->scroll = 0;
}

static const char* completionString;
static char        shortestMatch[MAX_TOKEN_CHARS];
static int32       matchCount;
// field we are working on, passed to Field_CompleteCommand (&g_consoleCommand for instance)
static field_t* completionField;

/*
===============
FindMatches

===============
*/
static void FindMatches(const char* s)
{
    int32 i;

    if (Q_stricmpn(s, completionString, strlen(completionString))) {
        return;
    }
    matchCount++;
    if (matchCount == 1) {
        Q_strncpyz(shortestMatch, s, sizeof(shortestMatch));
        return;
    }

    // cut shortestMatch to the amount common with s
    for (i = 0; s[i]; i++) {
        if (tolower(shortestMatch[i]) != tolower(s[i])) {
            shortestMatch[i] = 0;
        }
    }
}

/*
===============
PrintMatches

===============
*/
static void PrintMatches(const char* s)
{
    if (!Q_stricmpn(s, shortestMatch, strlen(shortestMatch))) {
        logger.info("    {}", s);
    }
}

static void keyConcatArgs()
{
    int32       i;
    const char* arg;

    for (i = 1; i < Cmd_Argc(); i++) {
        Q_strcat(completionField->buffer, sizeof(completionField->buffer), " ");
        arg = Cmd_Argv(i);
        while (*arg) {
            if (*arg == ' ') {
                Q_strcat(completionField->buffer, sizeof(completionField->buffer), "\"");
                break;
            }
            arg++;
        }
        Q_strcat(completionField->buffer, sizeof(completionField->buffer), Cmd_Argv(i));
        if (*arg == ' ') {
            Q_strcat(completionField->buffer, sizeof(completionField->buffer), "\"");
        }
    }
}

static void ConcatRemaining(const char* src, const char* start)
{
    const char* str = strstr(src, start);
    if (!str) {
        keyConcatArgs();
        return;
    }

    str += strlen(start);
    Q_strcat(completionField->buffer, sizeof(completionField->buffer), str);
}

/*
===============
Field_CompleteCommand

perform Tab expansion
NOTE TTimo this was originally client code only
  moved to common code when writing tty console for *nix dedicated server
===============
*/
void Field_CompleteCommand(field_t* field)
{
    field_t temp;

    completionField = field;

    // only look at the first token for completion purposes
    Cmd_TokenizeString(completionField->buffer);

    completionString = Cmd_Argv(0);
    if (completionString[0] == '\\' || completionString[0] == '/') {
        completionString++;
    }
    matchCount = 0;
    shortestMatch[0] = 0;

    if (strlen(completionString) == 0) {
        return;
    }

    Cmd_CommandCompletion(FindMatches);
    Cvar_CommandCompletion(FindMatches);

    if (matchCount == 0) {
        return; // no matches
    }

    Com_Memcpy(&temp, completionField, sizeof(field_t));

    if (matchCount == 1) {
        Com_sprintf(completionField->buffer, sizeof(completionField->buffer), "\\%s", shortestMatch);
        if (Cmd_Argc() == 1) {
            Q_strcat(completionField->buffer, sizeof(completionField->buffer), " ");
        } else {
            ConcatRemaining(temp.buffer, completionString);
        }
        completionField->cursor = strlen(completionField->buffer);
        return;
    }

    // multiple matches, complete to shortest
    Com_sprintf(completionField->buffer, sizeof(completionField->buffer), "\\%s", shortestMatch);
    completionField->cursor = strlen(completionField->buffer);
    ConcatRemaining(temp.buffer, completionString);

    logger.info("]{}", completionField->buffer);

    // run through again, printing matches
    Cmd_CommandCompletion(PrintMatches);
    Cvar_CommandCompletion(PrintMatches);
}
