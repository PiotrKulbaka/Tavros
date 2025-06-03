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
// client.h -- primary header for client

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../q3_renderer/tr_public.h"
#include "../q3_ui/ui_public.h"
#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"

#define RETRANSMIT_TIMEOUT 3000 // time between connection packet retransmits


// snapshots are a view of the server at a given time
typedef struct
{
    bool  valid;                        // cleared if delta parsing was invalid
    int32 snapFlags;                    // rate delayed and dropped commands

    int32 serverTime;                   // server time the message is valid for (in msec)

    int32 messageNum;                   // copied from netchan->incoming_sequence
    int32 deltaNum;                     // messageNum the delta is from
    int32 ping;                         // time from when cmdNum-1 was sent to time packet was reeceived
    uint8 areamask[MAX_MAP_AREA_BYTES]; // portalarea visibility bits

    int32         cmdNum;               // the next cmdNum the server is expecting
    playerState_t ps;                   // complete information about the current player at this time

    int32 numEntities;                  // all of the entities that need to be presented
    int32 parseEntitiesNum;             // at the time of this snapshot

    int32 serverCommandNum;             // execute all commands up to this before
                                        // making the snapshot current
} clSnapshot_t;


/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct
{
    int32 p_cmdNumber;  // cl.cmdNumber when packet was sent
    int32 p_serverTime; // usercmd->serverTime when packet was sent
    int32 p_realtime;   // cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define MAX_PARSE_ENTITIES 2048

extern int32 g_console_field_width;

typedef struct
{
    int32 timeoutcount;             // it requres several frames in a timeout condition
                                    // to disconnect, preventing debugging breaks from
                                    // causing immediate disconnects on continue
    clSnapshot_t snap;              // latest received from server

    int32 serverTime;               // may be paused during play
    int32 oldServerTime;            // to prevent time from flowing bakcwards
    int32 oldFrameServerTime;       // to check tournament restarts
    int32 serverTimeDelta;          // cl.serverTime = cls.realtime + cl.serverTimeDelta
                                    // this value changes as net lag varies
    bool extrapolatedSnapshot;      // set if any cgame frame has been forced to extrapolate
                                    // cleared when CL_AdjustTimeDelta looks at it
    bool newSnapshots;              // set on parse of any valid packet

    gameState_t gameState;          // configstrings
    char        mapname[MAX_QPATH]; // extracted from CS_SERVERINFO

    int32 parseEntitiesNum;         // index (not anded off) into cl_parse_entities[]

    int32 mouseDx[2], mouseDy[2];   // added to by mouse events
    int32 mouseIndex;

    // cgame communicates a few values to the client system
    int32 cgameUserCmdValue; // current weapon to add to usercmd_t
    float cgameSensitivity;

    // cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
    // properly generated command
    usercmd_t cmds[CMD_BACKUP];            // each mesage will send several old cmds
    int32     cmdNumber;                   // incremented each frame, because multiple
                                           // frames may need to be packed into a single packet

    outPacket_t outPackets[PACKET_BACKUP]; // information about each packet we have sent out

    // the client maintains its own idea of view angles, which are
    // sent to the server each frame.  It is cleared to 0 upon entering each level.
    // the server sends a delta each frame which is added to the locally
    // tracked view angles to account for standing on rotating objects,
    // and teleport direction changes
    vec3_t viewangles;

    int32 serverId; // included in each client message so the server
                    // can tell if it is for a prior map_restart
    // big stuff at end of structure so most offsets are 15 bits or less
    clSnapshot_t snapshots[PACKET_BACKUP];

    entityState_t entityBaselines[MAX_GENTITIES]; // for delta compression when not in previous frame

    entityState_t parseEntities[MAX_PARSE_ENTITIES];
} clientActive_t;

extern clientActive_t cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/


typedef struct
{
    int32 clientNum;
    int32 lastPacketSentTime; // for retransmits during connection
    int32 lastPacketTime;     // for timeouts

    netadr_t serverAddress;
    int32    connectTime;                      // for connection retransmits
    int32    connectPacketCount;               // for display on connection dialog
    char     serverMessage[MAX_STRING_TOKENS]; // for display on connection dialog

    int32 challenge;                           // from the server to use for connecting
    int32 checksumFeed;                        // from the server for checksum calculations

    // these are our reliable messages that go to the server
    int32 reliableSequence;
    int32 reliableAcknowledge; // the last one the server has executed
    char  reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

    // server message (unreliable) and command (reliable) sequence
    // numbers are NOT cleared at level changes, but continue to
    // increase as long as the connection is valid

    // message sequence is used by both the network layer and the
    // delta compression layer
    int32 serverMessageSequence;

    // reliable messages received from server
    int32 serverCommandSequence;
    int32 lastExecutedServerCommand; // last server command grabbed or executed with CL_GetServerCommand
    char  serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

    // file transfer from server
    fileHandle_t download;
    char         downloadTempName[MAX_OSPATH];
    char         downloadName[MAX_OSPATH];
    int32        downloadNumber;
    int32        downloadBlock;                 // block we are waiting for
    int32        downloadCount;                 // how many bytes we got
    int32        downloadSize;                  // how many bytes we got
    char         downloadList[MAX_INFO_STRING]; // list of paks we need to download
    bool         downloadRestart;               // if true, we need to do another FS_Restart because we downloaded a pak

    // demo information
    char         demoName[MAX_QPATH];
    bool         spDemoRecording;
    bool         demorecording;
    bool         demoplaying;
    bool         demowaiting; // don't record until a non-delta message is received
    bool         firstDemoFrameSkipped;
    fileHandle_t demofile;

    int32 timeDemoFrames;   // counter of rendered frames
    int32 timeDemoStart;    // cls.realtime before first frame
    int32 timeDemoBaseTime; // each frame will be at this time + frameNum * 50

    // big stuff at end of structure so most offsets are 15 bits or less
    netchan_t netchan;
} clientConnection_t;

extern clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct
{
    netadr_t adr;
    int32    start;
    int32    time;
    char     info[MAX_INFO_STRING];
} ping_t;

typedef struct
{
    netadr_t adr;
    char     hostName[MAX_NAME_LENGTH];
    char     mapName[MAX_NAME_LENGTH];
    char     game[MAX_NAME_LENGTH];
    int32    netType;
    int32    gameType;
    int32    clients;
    int32    maxClients;
    int32    minPing;
    int32    maxPing;
    int32    ping;
    bool     visible;
    int32    punkbuster;
} serverInfo_t;

typedef struct
{
    uint8  ip[4];
    uint16 port;
} serverAddress_t;

typedef struct
{
    connstate_t state;           // connection status
    int32       keyCatchers;     // bit flags

    char servername[MAX_OSPATH]; // name of server from original connect (used by reconnect)

    // when the server clears the hunk, all of these must be restarted
    bool rendererStarted;
    bool soundStarted;
    bool soundRegistered;
    bool uiStarted;
    bool cgameStarted;

    int32 framecount;
    int32 frametime;     // msec since last frame

    int32 realtime;      // ignores pause
    int32 realFrametime; // ignoring pause, so console always works

    int32        numlocalservers;
    serverInfo_t localServers[MAX_OTHER_SERVERS];

    int32        numglobalservers;
    serverInfo_t globalServers[MAX_GLOBAL_SERVERS];
    // additional global servers
    int32           numGlobalServerAddresses;
    serverAddress_t globalServerAddresses[MAX_GLOBAL_SERVERS];

    int32        numfavoriteservers;
    serverInfo_t favoriteServers[MAX_OTHER_SERVERS];

    int32        nummplayerservers;
    serverInfo_t mplayerServers[MAX_OTHER_SERVERS];

    int32 pingUpdateSource; // source currently pinging or updating

    int32 masterNum;

    // update server info
    netadr_t updateServer;
    char     updateChallenge[MAX_TOKEN_CHARS];
    char     updateInfoString[MAX_INFO_STRING];

    netadr_t authorizeServer;

    // rendering info
    glconfig_t glconfig;
    qhandle_t  charSetShader;
    qhandle_t  whiteShader;
    qhandle_t  consoleShader;
} clientStatic_t;

extern clientStatic_t cls;

//
// cvars
//
extern cvar_t* cl_nodelta;
extern cvar_t* cl_debugMove;
extern cvar_t* cl_noprint;
extern cvar_t* cl_timegraph;
extern cvar_t* cl_maxpackets;
extern cvar_t* cl_packetdup;
extern cvar_t* cl_shownet;
extern cvar_t* cl_showSend;
extern cvar_t* cl_timeNudge;
extern cvar_t* cl_showTimeDelta;
extern cvar_t* cl_freezeDemo;

extern cvar_t* cl_yawspeed;
extern cvar_t* cl_pitchspeed;
extern cvar_t* cl_run;
extern cvar_t* cl_anglespeedkey;

extern cvar_t* cl_sensitivity;
extern cvar_t* cl_freelook;

extern cvar_t* cl_mouseAccel;
extern cvar_t* cl_showMouseRate;

extern cvar_t* m_pitch;
extern cvar_t* m_yaw;
extern cvar_t* m_forward;
extern cvar_t* m_side;
extern cvar_t* m_filter;

extern cvar_t* cl_timedemo;

extern cvar_t* cl_activeAction;

extern cvar_t* cl_allowDownload;
extern cvar_t* cl_conXOffset;

//=================================================

//
// cl_main
//

void CL_Init();
void CL_FlushMemory();
void CL_ShutdownAll();
void CL_AddReliableCommand(const char* cmd);

void CL_StartHunkUsers();

void CL_Disconnect_f();
void CL_Vid_Restart_f();
void CL_Snd_Restart_f();
void CL_StartDemoLoop();
void CL_NextDemo();
void CL_ReadDemoMessage();

void CL_InitDownloads();
void CL_NextDownload();

void  CL_GetPing(int32 n, char* buf, int32 buflen, int32* pingtime);
void  CL_GetPingInfo(int32 n, char* buf, int32 buflen);
void  CL_ClearPing(int32 n);
int32 CL_GetPingQueueCount();

//
// cl_input
//
typedef struct
{
    int32  down[2];    // key nums holding it down
    uint32 downtime;   // msec timestamp
    uint32 msec;       // msec down this frame if both a down and up happened
    bool   active;     // current state
    bool   wasPressed; // set when down, not cleared when up
} kbutton_t;

extern kbutton_t in_strafe;
extern kbutton_t in_speed;

void CL_InitInput();
void CL_SendCmd();
void CL_ClearState();

void CL_WritePacket();
void IN_CenterView();

float       CL_KeyState(kbutton_t* key);
const char* Key_KeynumToString(int32 keynum);

//
// cl_parse.c
//

void CL_SystemInfoChanged();
void CL_ParseServerMessage(msg_t* msg);

//====================================================================

void CL_ServerInfoPacket(netadr_t from, msg_t* msg);
void CL_LocalServers_f();
void CL_GlobalServers_f();
void CL_Ping_f();

//
// console
//
void Con_CheckResize();
void Con_Init();
void Con_Clear_f();
void Con_ToggleConsole_f();
void Con_DrawNotify();
void Con_RunConsole();
void Con_DrawConsole();
void Con_PageUp();
void Con_PageDown();
void Con_Top();
void Con_Bottom();
void Con_Close();


//
// cl_scrn.c
//
void SCR_Init();
void SCR_UpdateScreen();

void SCR_DebugGraph(float value, int32 color);

void SCR_AdjustFrom640(float* x, float* y, float* w, float* h);
void SCR_FillRect(float x, float y, float width, float height, const float* color);
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader);

void SCR_DrawBigString(int32 x, int32 y, const char* s, float alpha); // draws a string with embedded color control characters with fade
void SCR_DrawSmallStringExt(int32 x, int32 y, const char* string, float* setColor, bool forceColor);
void SCR_DrawSmallChar(int32 x, int32 y, int32 ch);

//
// cl_cgame.c
//
void CL_InitCGame();
void CL_ShutdownCGame();
bool CL_GameCommand();
void CL_CGameRendering();
void CL_SetCGameTime();
void CL_FirstSnapshot();

//
// cl_ui.c
//
void  CL_InitUI();
void  CL_ShutdownUI();
bool  CL_UIIsInit();
int32 Key_GetCatcher();
void  Key_SetCatcher(int32 catcher);

//
// cl_net_chan.c
//
void CL_Netchan_Transmit(netchan_t* chan, msg_t* msg); // int32 length, const uint8 *data );
void CL_Netchan_TransmitNextFragment(netchan_t* chan);
bool CL_Netchan_Process(netchan_t* chan, msg_t* msg);


void  CL_GetGameState(gameState_t* gs);
void  CL_GetCurrentSnapshotNumber(int32* snapshotNumber, int32* serverTime);
bool  CL_GetSnapshot(int32 snapshotNumber, snapshot_t* snapshot);
bool  CL_GetServerCommand(int32 serverCommandNumber);
int32 CL_GetCurrentCmdNumber();
bool  CL_GetUserCmd(int32 cmdNumber, usercmd_t* ucmd);
void  CL_SetUserCmdValue(int32 userCmdValue, float sensitivityScale);
