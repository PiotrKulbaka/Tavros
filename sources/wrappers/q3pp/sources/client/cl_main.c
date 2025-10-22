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
// cl_main.c  -- client main loop

#include "client.h"
#include <limits.h>

static tavros::core::logger logger("cl_main");

cvar_t* cl_nodelta;
cvar_t* cl_debugMove;

cvar_t* cl_noprint;
cvar_t* cl_motd;

cvar_t* rcon_client_password;
cvar_t* rconAddress;

cvar_t* cl_timeout;
cvar_t* cl_maxpackets;
cvar_t* cl_packetdup;
cvar_t* cl_timeNudge;
cvar_t* cl_showTimeDelta;
cvar_t* cl_freezeDemo;

cvar_t* cl_shownet;
cvar_t* cl_showSend;
cvar_t* cl_timedemo;
cvar_t* cl_avidemo;
cvar_t* cl_forceavidemo;

cvar_t* cl_freelook;
cvar_t* cl_sensitivity;

cvar_t* cl_mouseAccel;
cvar_t* cl_showMouseRate;

cvar_t* m_pitch;
cvar_t* m_yaw;
cvar_t* m_forward;
cvar_t* m_side;
cvar_t* m_filter;

cvar_t* cl_activeAction;

cvar_t* cl_motdString;

cvar_t* cl_allowDownload;
cvar_t* cl_conXOffset;

cvar_t* cl_serverStatusResendTime;

clientActive_t     cl;
clientConnection_t clc;
clientStatic_t     cls;

ping_t cl_pinglist[MAX_PINGREQUESTS];

typedef struct serverStatus_s
{
    char     string[BIG_INFO_STRING];
    netadr_t address;
    int32    time, startTime;
    bool     pending;
    bool     print;
    bool     retrieved;
} serverStatus_t;

serverStatus_t cl_serverStatusList[MAX_SERVERSTATUSREQUESTS];
int32          serverStatusCount;

extern void SV_BotFrame(int32 time);
void        CL_CheckForResend();
void        CL_ShowIP_f();
void        CL_ServerStatus_f();
void        CL_ServerStatusResponse(netadr_t from, msg_t* msg);


/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand(const char* cmd)
{
    int32 index;

    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    if (clc.reliableSequence - clc.reliableAcknowledge > MAX_RELIABLE_COMMANDS) {
        Com_Error(ERR_DROP, "Client command overflow");
    }
    clc.reliableSequence++;
    index = clc.reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(clc.reliableCommands[index], cmd, sizeof(clc.reliableCommands[index]));
}

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage(msg_t* msg, int32 headerBytes)
{
    int32 len, swlen;

    // write the packet sequence
    len = clc.serverMessageSequence;
    swlen = (len);
    FS_Write(&swlen, 4, clc.demofile);

    // skip the packet sequencing information
    len = msg->cursize - headerBytes;
    swlen = (len);
    FS_Write(&swlen, 4, clc.demofile);
    FS_Write(msg->data + headerBytes, len, clc.demofile);
}


/*
====================
CL_StopRecording_f

stop recording a demo
====================
*/
void CL_StopRecord_f()
{
    int32 len;

    if (!clc.demorecording) {
        logger.info("Not recording a demo.");
        return;
    }

    // finish up
    len = -1;
    FS_Write(&len, 4, clc.demofile);
    FS_Write(&len, 4, clc.demofile);
    FS_FCloseFile(clc.demofile);
    clc.demofile = 0;
    clc.demorecording = false;
    clc.spDemoRecording = false;
    logger.info("Stopped demo.");
}

/*
==================
CL_DemoFilename
==================
*/
void CL_DemoFilename(int32 number, char* fileName)
{
    int32 a, b, c, d;

    if (number < 0 || number > 9999) {
        Com_sprintf(fileName, MAX_OSPATH, "demo9999.tga");
        return;
    }

    a = number / 1000;
    number -= a * 1000;
    b = number / 100;
    number -= b * 100;
    c = number / 10;
    number -= c * 10;
    d = number;

    Com_sprintf(fileName, MAX_OSPATH, "demo%i%i%i%i", a, b, c, d);
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static char demoName[MAX_QPATH]; // compiler bug workaround
void        CL_Record_f()
{
    char           name[MAX_OSPATH];
    uint8          bufData[MAX_MSGLEN];
    msg_t          buf;
    int32          i;
    int32          len;
    entityState_t* ent;
    entityState_t  nullstate;
    const char*    s;

    if (Cmd_Argc() > 2) {
        logger.info("record <demoname>");
        return;
    }

    if (clc.demorecording) {
        if (!clc.spDemoRecording) {
            logger.info("Already recording.");
        }
        return;
    }

    if (cls.state != CA_ACTIVE) {
        logger.info("You must be in a level to record.");
        return;
    }

    // sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
    if (!Cvar_VariableValue("g_synchronousClients")) {
        logger.warning("You should set 'g_synchronousClients 1' for smoother demo recording");
    }

    if (Cmd_Argc() == 2) {
        s = Cmd_Argv(1);
        Q_strncpyz(demoName, s, sizeof(demoName));
        Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION);
    } else {
        int32 number;

        // scan for a free demo name
        for (number = 0; number <= 9999; number++) {
            CL_DemoFilename(number, demoName);
            Com_sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION);

            len = FS_ReadFile(name, NULL);
            if (len <= 0) {
                break; // file doesn't exist
            }
        }
    }

    // open the demo file

    logger.info("recording to {}.", name);
    clc.demofile = FS_FOpenFileWrite(name);
    if (!clc.demofile) {
        logger.error("couldn't open.");
        return;
    }
    clc.demorecording = true;
    if (Cvar_VariableValue("ui_recordSPDemo")) {
        clc.spDemoRecording = true;
    } else {
        clc.spDemoRecording = false;
    }


    Q_strncpyz(clc.demoName, demoName, sizeof(clc.demoName));

    // don't start saving messages until a non-delta compressed message is received
    clc.demowaiting = true;

    // write out the gamestate message
    MSG_Init(&buf, bufData, sizeof(bufData));
    MSG_Bitstream(&buf);

    // NOTE, MRE: all server->client messages now acknowledge
    MSG_WriteLong(&buf, clc.reliableSequence);

    MSG_WriteByte(&buf, svc_gamestate);
    MSG_WriteLong(&buf, clc.serverCommandSequence);

    // configstrings
    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if (!cl.gameState.stringOffsets[i]) {
            continue;
        }
        s = cl.gameState.stringData + cl.gameState.stringOffsets[i];
        MSG_WriteByte(&buf, svc_configstring);
        MSG_WriteShort(&buf, i);
        MSG_WriteBigString(&buf, s);
    }

    // baselines
    Com_Memset(&nullstate, 0, sizeof(nullstate));
    for (i = 0; i < MAX_GENTITIES; i++) {
        ent = &cl.entityBaselines[i];
        if (!ent->number) {
            continue;
        }
        MSG_WriteByte(&buf, svc_baseline);
        MSG_WriteDeltaEntity(&buf, &nullstate, ent, true);
    }

    MSG_WriteByte(&buf, svc_EOF);

    // finished writing the gamestate stuff

    // write the client num
    MSG_WriteLong(&buf, clc.clientNum);
    // write the checksum feed
    MSG_WriteLong(&buf, clc.checksumFeed);

    // finished writing the client packet
    MSG_WriteByte(&buf, svc_EOF);

    // write it to the demo file
    len = (clc.serverMessageSequence - 1);
    FS_Write(&len, 4, clc.demofile);

    len = (buf.cursize);
    FS_Write(&len, 4, clc.demofile);
    FS_Write(buf.data, buf.cursize, clc.demofile);

    // the rest of the demo file will be copied from net messages
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

/*
=================
CL_DemoCompleted
=================
*/
void CL_DemoCompleted()
{
    if (cl_timedemo && cl_timedemo->integer) {
        int32 time;

        time = Sys_Milliseconds() - clc.timeDemoStart;
        if (time > 0) {
            logger.info("{} frames, %3.1f seconds: %3.1f fps", clc.timeDemoFrames, time / 1000.0, clc.timeDemoFrames * 1000.0 / time);
        }
    }

    CL_Disconnect(true);
    CL_NextDemo();
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage()
{
    int32 r;
    msg_t buf;
    uint8 bufData[MAX_MSGLEN];
    int32 s;

    if (!clc.demofile) {
        CL_DemoCompleted();
        return;
    }

    // get the sequence number
    r = FS_Read(&s, 4, clc.demofile);
    if (r != 4) {
        CL_DemoCompleted();
        return;
    }
    clc.serverMessageSequence = (s);

    // init the message
    MSG_Init(&buf, bufData, sizeof(bufData));

    // get the length
    r = FS_Read(&buf.cursize, 4, clc.demofile);
    if (r != 4) {
        CL_DemoCompleted();
        return;
    }
    buf.cursize = (buf.cursize);
    if (buf.cursize == -1) {
        CL_DemoCompleted();
        return;
    }
    if (buf.cursize > buf.maxsize) {
        Com_Error(ERR_DROP, "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN");
    }
    r = FS_Read(buf.data, buf.cursize, clc.demofile);
    if (r != buf.cursize) {
        logger.info("Demo file was truncated.");
        CL_DemoCompleted();
        return;
    }

    clc.lastPacketTime = cls.realtime;
    buf.readcount = 0;
    CL_ParseServerMessage(&buf);
}

/*
====================
CL_WalkDemoExt
====================
*/
static void CL_WalkDemoExt(const char* arg, char* name, int32* demofile)
{
    int32 i = 0;
    *demofile = 0;
    while (demo_protocols[i]) {
        Com_sprintf(name, MAX_OSPATH, "demos/%s.dm_%d", arg, demo_protocols[i]);
        FS_FOpenFileRead(name, demofile, true);
        if (*demofile) {
            logger.info("Demo file: {}", name);
            break;
        } else {
            logger.info("Not found: {}", name);
        }
        i++;
    }
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f()
{
    char        name[MAX_OSPATH];
    const char *arg, *ext_test;
    int32       protocol, i;
    char        retry[MAX_OSPATH];

    if (Cmd_Argc() != 2) {
        logger.info("playdemo <demoname>");
        return;
    }

    // make sure a local server is killed
    Cvar_Set("sv_killserver", "1");

    CL_Disconnect(true);

    // open the demo file
    arg = Cmd_Argv(1);

    // check for an extension .dm_?? (?? is protocol)
    ext_test = arg + strlen(arg) - 6;
    if ((strlen(arg) > 6) && (ext_test[0] == '.') && ((ext_test[1] == 'd') || (ext_test[1] == 'D')) && ((ext_test[2] == 'm') || (ext_test[2] == 'M')) && (ext_test[3] == '_')) {
        protocol = atoi(ext_test + 4);
        i = 0;
        while (demo_protocols[i]) {
            if (demo_protocols[i] == protocol) {
                break;
            }
            i++;
        }
        if (demo_protocols[i]) {
            Com_sprintf(name, sizeof(name), "demos/%s", arg);
            FS_FOpenFileRead(name, &clc.demofile, true);
        } else {
            logger.info("Protocol {} not supported for demos", protocol);
            Q_strncpyz(retry, arg, sizeof(retry));
            retry[strlen(retry) - 6] = 0;
            CL_WalkDemoExt(retry, name, &clc.demofile);
        }
    } else {
        CL_WalkDemoExt(arg, name, &clc.demofile);
    }

    if (!clc.demofile) {
        Com_Error(ERR_DROP, "couldn't open %s", name);
        return;
    }
    Q_strncpyz(clc.demoName, Cmd_Argv(1), sizeof(clc.demoName));

    Con_Close();

    cls.state = CA_CONNECTED;
    clc.demoplaying = true;
    Q_strncpyz(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

    // read demo messages until connected
    while (cls.state >= CA_CONNECTED && cls.state < CA_PRIMED) {
        CL_ReadDemoMessage();
    }
    // don't get the first snapshot this frame, to prevent the long
    // time from the gamestate load from messing causing a time skip
    clc.firstDemoFrameSkipped = false;
}


/*
====================
CL_StartDemoLoop

Closing the main menu will restart the demo loop
====================
*/
void CL_StartDemoLoop()
{
    // start the demo loop again
    Cbuf_AddText("d1\n");
    cls.keyCatchers = 0;
}

/*
==================
CL_NextDemo

Called when a demo finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo()
{
    char v[MAX_STRING_CHARS];

    Q_strncpyz(v, Cvar_VariableString("nextdemo"), sizeof(v));
    v[MAX_STRING_CHARS - 1] = 0;
    logger.debug("CL_NextDemo: {}", v);
    if (!v[0]) {
        return;
    }

    Cvar_Set("nextdemo", "");
    Cbuf_AddText(v);
    Cbuf_AddText("\n");
    Cbuf_Execute();
}


//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll()
{
    // clear sounds
    S_DisableSounds();
    // shutdown CGame
    CL_ShutdownCGame();
    // shutdown UI
    CL_ShutdownUI();

    // shutdown the renderer
    if (RE_Shutdown) {
        RE_Shutdown(false); // don't destroy window or context
    }

    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.rendererStarted = false;
    cls.soundRegistered = false;
}

/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory()
{
    // shutdown all the client stuff
    CL_ShutdownAll();

    // if not running a server clear the whole hunk
    if (!com_sv_running->integer) {
        // clear the whole hunk
        Hunk_Clear();
        // clear collision map data
        CM_ClearMap();
    } else {
        // clear all the client data on the hunk
        Hunk_ClearToMark();
    }

    CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading()
{
    if (!com_cl_running->integer) {
        return;
    }

    Con_Close();
    cls.keyCatchers = 0;

    // if we are already connected to the local host, stay connected
    if (cls.state >= CA_CONNECTED && !Q_stricmp(cls.servername, "localhost")) {
        cls.state = CA_CONNECTED; // so the connect screen is drawn
        Com_Memset(cls.updateInfoString, 0, sizeof(cls.updateInfoString));
        Com_Memset(clc.serverMessage, 0, sizeof(clc.serverMessage));
        Com_Memset(&cl.gameState, 0, sizeof(cl.gameState));
        clc.lastPacketSentTime = -9999;
        SCR_UpdateScreen();
    } else {
        // clear nextmap so the cinematic shutdown doesn't execute it
        Cvar_Set("nextmap", "");
        CL_Disconnect(true);
        Q_strncpyz(cls.servername, "localhost", sizeof(cls.servername));
        cls.state = CA_CHALLENGING; // so the connect screen is drawn
        cls.keyCatchers = 0;
        SCR_UpdateScreen();
        clc.connectTime = -RETRANSMIT_TIMEOUT;
        NET_StringToAdr(cls.servername, &clc.serverAddress);
        // we don't need a challenge on the localhost

        CL_CheckForResend();
    }
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState()
{
    //    S_StopAllSounds();

    Com_Memset(&cl, 0, sizeof(cl));
}


/*
=====================
CL_Disconnect

Called when a connection, demo is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(bool showMainMenu)
{
    if (!com_cl_running || !com_cl_running->integer) {
        return;
    }

    if (clc.demorecording) {
        CL_StopRecord_f();
    }

    if (clc.download) {
        FS_FCloseFile(clc.download);
        clc.download = 0;
    }
    *clc.downloadTempName = *clc.downloadName = 0;
    Cvar_Set("cl_downloadName", "");

    if (clc.demofile) {
        FS_FCloseFile(clc.demofile);
        clc.demofile = 0;
    }

    if (showMainMenu) {
        UI_SetActiveMenu(UIMENU_NONE);
    }

    S_ClearSoundBuffer();

    // send a disconnect message to the server
    // send it a few times in case one is dropped
    if (cls.state >= CA_CONNECTED) {
        CL_AddReliableCommand("disconnect");
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
    }

    CL_ClearState();

    // wipe the client connection
    Com_Memset(&clc, 0, sizeof(clc));

    cls.state = CA_DISCONNECTED;

    // allow cheats locally
    Cvar_Set("sv_cheats", "1");
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer(const char* string)
{
    const char* cmd;

    cmd = Cmd_Argv(0);

    // ignore key up commands
    if (cmd[0] == '-') {
        return;
    }

    if (clc.demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+') {
        logger.info("Unknown command '{}'", cmd);
        return;
    }

    if (Cmd_Argc() > 1) {
        CL_AddReliableCommand(string);
    } else {
        CL_AddReliableCommand(cmd);
    }
}

/*
===================
CL_RequestMotd

===================
*/
void CL_RequestMotd()
{
    char info[MAX_INFO_STRING];

    if (!cl_motd->integer) {
        return;
    }
    logger.info("Resolving {}", UPDATE_SERVER_NAME);
    if (!NET_StringToAdr(UPDATE_SERVER_NAME, &cls.updateServer)) {
        logger.info("Couldn't resolve address");
        return;
    }
    cls.updateServer.port = BigShort(PORT_UPDATE);
    logger.info("{} resolved to {}.{}.{}.{}:{}", UPDATE_SERVER_NAME, cls.updateServer.ip[0], cls.updateServer.ip[1], cls.updateServer.ip[2], cls.updateServer.ip[3], BigShort(cls.updateServer.port));

    info[0] = 0;
    // NOTE TTimo xoring against Com_Milliseconds, otherwise we may not have a true randomization
    // only srand I could catch before here is tr_noise.c l:26 srand(1001)
    // NOTE: the Com_Milliseconds xoring only affects the lower 16-bit word,
    //   but I decided it was enough randomization
    Com_sprintf(cls.updateChallenge, sizeof(cls.updateChallenge), "%i", ((rand() << 16) ^ rand()) ^ Com_Milliseconds());

    Info_SetValueForKey(info, "challenge", cls.updateChallenge);
    Info_SetValueForKey(info, "version", com_version->string);

    NET_OutOfBandPrint(NS_CLIENT, cls.updateServer, "getmotd \"%s\"\n", info);
}

/*
===================
CL_RequestAuthorization

Authorization server protocol
-----------------------------

All commands are text in Q3 out of band packets (leading 0xff 0xff 0xff 0xff).

Whenever the client tries to get a challenge from the server it wants to
connect to, it also blindly fires off a packet to the authorize server:


#OLD The authorize server returns a:
#OLD
#OLD keyAthorize <challenge> <accept | deny>
#OLD
#OLD A client will be accepted if is valid and it has not been used by any other IP
#OLD address in the last 15 minutes.


The server sends a:

getIpAuthorize <challenge> <ip>

The authorize server returns a:

ipAuthorize <challenge> <accept | deny | demo | unknown >

A client will be accepted if a valid was sent by that ip (only) in the last 15 minutes.
If no response is received from the authorize server after two tries, the client will be let
in anyway.
===================
*/
void CL_RequestAuthorization()
{
    cvar_t* fs;

    if (!cls.authorizeServer.port) {
        logger.info("Resolving {}", AUTHORIZE_SERVER_NAME);
        if (!NET_StringToAdr(AUTHORIZE_SERVER_NAME, &cls.authorizeServer)) {
            logger.info("Couldn't resolve address");
            return;
        }

        cls.authorizeServer.port = BigShort(PORT_AUTHORIZE);
        logger.info("{} resolved to {}.{}.{}.{}:{}", AUTHORIZE_SERVER_NAME, cls.authorizeServer.ip[0], cls.authorizeServer.ip[1], cls.authorizeServer.ip[2], cls.authorizeServer.ip[3], BigShort(cls.authorizeServer.port));
    }
    if (cls.authorizeServer.type == NA_BAD) {
        return;
    }

    fs = Cvar_Get("cl_anonymous", "0", CVAR_INIT | CVAR_SYSTEMINFO);
    NET_OutOfBandPrint(NS_CLIENT, cls.authorizeServer, va("getKeyAuthorize %i", fs->integer));
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f()
{
    if (cls.state != CA_ACTIVE || clc.demoplaying) {
        logger.info("Not connected to a server.");
        return;
    }

    // don't forward the first argument
    if (Cmd_Argc() > 1) {
        CL_AddReliableCommand(Cmd_Args());
    }
}

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f()
{
    int32 argc = Cmd_Argc();

    if (argc > 2) {
        char  buffer[1024];
        int32 i;

        strcpy(buffer, Cmd_Argv(1));
        strcat(buffer, "=");

        for (i = 2; i < argc; i++) {
            strcat(buffer, Cmd_Argv(i));
            strcat(buffer, " ");
        }

        _putenv(buffer);
    } else if (argc == 2) {
        char* env = getenv(Cmd_Argv(1));

        if (env) {
            logger.info("{}={}", Cmd_Argv(1), env);
        } else {
            logger.info("{} undefined", Cmd_Argv(1), env);
        }
    }
}


/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f()
{
    Cvar_Set("ui_singlePlayerActive", "0");
    if (cls.state != CA_DISCONNECTED) {
        Com_Error(ERR_DISCONNECT, "Disconnected from server");
    }
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f()
{
    if (!strlen(cls.servername) || !strcmp(cls.servername, "localhost")) {
        logger.info("Can't reconnect to localhost.");
        return;
    }
    Cvar_Set("ui_singlePlayerActive", "0");
    Cbuf_AddText(va("connect %s\n", cls.servername));
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f()
{
    const char* server;

    if (Cmd_Argc() != 2) {
        logger.info("usage: connect [server]");
        return;
    }

    Cvar_Set("ui_singlePlayerActive", "0");

    // fire a message off to the motd server
    CL_RequestMotd();

    // clear any previous "server full" type messages
    clc.serverMessage[0] = 0;

    server = Cmd_Argv(1);

    if (com_sv_running->integer && !strcmp(server, "localhost")) {
        // if running a local server, kill it
        SV_Shutdown("Server quit\n");
    }

    // make sure a local server is killed
    Cvar_Set("sv_killserver", "1");
    SV_Frame(0);

    CL_Disconnect(true);
    Con_Close();

    /* MrE: 2000-09-13: now called in CL_DownloadsComplete
    CL_FlushMemory( );
    */

    Q_strncpyz(cls.servername, server, sizeof(cls.servername));

    if (!NET_StringToAdr(cls.servername, &clc.serverAddress)) {
        logger.info("Bad server address");
        cls.state = CA_DISCONNECTED;
        return;
    }
    if (clc.serverAddress.port == 0) {
        clc.serverAddress.port = BigShort(PORT_SERVER);
    }
    logger.info("{} resolved to {}.{}.{}.{}:{}", cls.servername, clc.serverAddress.ip[0], clc.serverAddress.ip[1], clc.serverAddress.ip[2], clc.serverAddress.ip[3], BigShort(clc.serverAddress.port));

    // if we aren't playing on a lan, we need to authenticate
    // with the cd key
    if (NET_IsLocalAddress(clc.serverAddress)) {
        cls.state = CA_CHALLENGING;
    } else {
        cls.state = CA_CONNECTING;
    }

    cls.keyCatchers = 0;
    clc.connectTime = -99999; // CL_CheckForResend() will fire immediately
    clc.connectPacketCount = 0;

    // server connection string
    Cvar_Set("cl_currentServerAddress", server);
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f()
{
    char     message[1024];
    netadr_t to;

    if (!rcon_client_password->string) {
        logger.info("You must set 'rconpassword' before issuing an rcon command.");
        return;
    }

    message[0] = -1;
    message[1] = -1;
    message[2] = -1;
    message[3] = -1;
    message[4] = 0;

    strcat(message, "rcon ");

    strcat(message, rcon_client_password->string);
    strcat(message, " ");

    strcat(message, Cmd_Cmd() + 5);

    if (cls.state >= CA_CONNECTED) {
        to = clc.netchan.remoteAddress;
    } else {
        if (!strlen(rconAddress->string)) {
            logger.info("You must either be connected, or set the 'rconAddress' cvar to issue rcon commands.");
            return;
        }
        NET_StringToAdr(rconAddress->string, &to);
        if (to.port == 0) {
            to.port = BigShort(PORT_SERVER);
        }
    }

    NET_SendPacket(NS_CLIENT, strlen(message) + 1, message, to);
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums()
{
    const char* pChecksums;
    char        cMsg[MAX_INFO_VALUE];
    int32       i;

    // if we are pure we need to send back a command with our referenced pk3 checksums
    pChecksums = FS_ReferencedPakPureChecksums();

    // "cp"
    // "Yf"
    Com_sprintf(cMsg, sizeof(cMsg), "Yf ");
    Q_strcat(cMsg, sizeof(cMsg), va("%d ", cl.serverId));
    Q_strcat(cMsg, sizeof(cMsg), pChecksums);
    for (i = 0; i < 2; i++) {
        cMsg[i] += 10;
    }
    CL_AddReliableCommand(cMsg);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer()
{
    CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void CL_Vid_Restart_f()
{
    // don't let them loop during the restart
    S_StopAllSounds();
    // shutdown the UI
    CL_ShutdownUI();
    // shutdown the CGame
    CL_ShutdownCGame();
    // shutdown the renderer and clear the renderer interface
    RE_Shutdown(true);
    // client is no longer pure untill new checksums are sent
    CL_ResetPureClientAtServer();
    // clear pak references
    FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
    // reinitialize the filesystem if the game directory or checksum has changed
    FS_ConditionalRestart(clc.checksumFeed);

    cls.rendererStarted = false;
    cls.uiStarted = false;
    cls.cgameStarted = false;
    cls.soundRegistered = false;

    // unpause so the cgame definately gets a snapshot and renders a frame
    Cvar_Set("cl_paused", "0");

    // if not running a server clear the whole hunk
    if (!com_sv_running->integer) {
        // clear the whole hunk
        Hunk_Clear();
    } else {
        // clear all the client data on the hunk
        Hunk_ClearToMark();
    }

    // initialize the renderer interface
    Cvar_Set("cl_paused", "0");

    // startup all the client stuff
    CL_StartHunkUsers();

    // start the cgame if connected
    if (cls.state > CA_CONNECTED) {
        cls.cgameStarted = true;
        CL_InitCGame();
        // send pure checksums
        CL_SendPureChecksums();
    }
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f()
{
    S_Shutdown();
    S_Init();

    CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f()
{
    logger.info("Opened PK3 Names: {}", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f()
{
    logger.info("Referenced PK3 Names: {}", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f()
{
    int32 i;
    int32 ofs;

    if (cls.state != CA_ACTIVE) {
        logger.info("Not connected to a server.");
        return;
    }

    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        ofs = cl.gameState.stringOffsets[i];
        if (!ofs) {
            continue;
        }
        logger.info("%4i: {}", i, cl.gameState.stringData + ofs);
    }
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f()
{
    logger.info("--------- Client Information ---------");
    logger.info("state: {}", (uint32) cls.state);
    logger.info("Server: {}", cls.servername);
    logger.info("User info settings: {}", Cvar_InfoString(CVAR_USERINFO));
    logger.info("--------------------------------------");
}


//====================================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete()
{
    // if we downloaded files we need to restart the file system
    if (clc.downloadRestart) {
        clc.downloadRestart = false;

        FS_Restart(clc.checksumFeed); // We possibly downloaded a pak, restart the file system to load it

        // inform the server so we get new gamestate info
        CL_AddReliableCommand("donedl");

        // by sending the donedl command we request a new gamestate
        // so we don't want to load stuff yet
        return;
    }

    // let the client game init and load data
    cls.state = CA_LOADING;

    // Pump the loop, this may change gamestate!
    Com_EventLoop();

    // if the gamestate was changed by calling Com_EventLoop
    // then we loaded everything already and we don't want to do it again.
    if (cls.state != CA_LOADING) {
        return;
    }

    // flush client memory and start loading stuff
    // this will also (re)load the UI
    // if this is a local client then only the client part of the hunk
    // will be cleared, note that this is done after the hunk mark has been set
    CL_FlushMemory();

    // initialize the CGame
    cls.cgameStarted = true;
    CL_InitCGame();

    // set pure checksums
    CL_SendPureChecksums();

    CL_WritePacket();
    CL_WritePacket();
    CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/
void CL_BeginDownload(const char* localName, const char* remoteName)
{
    logger.debug("***** CL_BeginDownload *****");
    logger.debug("Localname: {}", localName);
    logger.debug("Remotename: {}", remoteName);
    logger.debug("****************************");

    Q_strncpyz(clc.downloadName, localName, sizeof(clc.downloadName));
    Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName);

    // Set so UI gets access to it
    Cvar_Set("cl_downloadName", remoteName);
    Cvar_Set("cl_downloadSize", "0");
    Cvar_Set("cl_downloadCount", "0");
    Cvar_SetValue("cl_downloadTime", cls.realtime);

    clc.downloadBlock = 0; // Starting new file
    clc.downloadCount = 0;

    CL_AddReliableCommand(va("download %s", remoteName));
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload()
{
    char* s;
    char *remoteName, *localName;

    // We are looking to start a download here
    if (*clc.downloadList) {
        s = clc.downloadList;

        // format is:
        //  @remotename@localname@remotename@localname, etc.

        if (*s == '@') {
            s++;
        }
        remoteName = s;

        if ((s = strchr(s, '@')) == NULL) {
            CL_DownloadsComplete();
            return;
        }

        *s++ = 0;
        localName = s;
        if ((s = strchr(s, '@')) != NULL) {
            *s++ = 0;
        } else {
            s = localName + strlen(localName); // point at the nul uint8
        }

        CL_BeginDownload(localName, remoteName);

        clc.downloadRestart = true;

        // move over the rest
        memmove(clc.downloadList, s, strlen(s) + 1);

        return;
    }

    CL_DownloadsComplete();
}

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads()
{
    char missingfiles[1024];

    if (!cl_allowDownload->integer) {
        // autodownload is disabled on the client
        // but it's possible that some referenced files on the server are missing
        if (FS_ComparePaks(missingfiles, sizeof(missingfiles), false)) {
            // NOTE TTimo I would rather have that printed as a modal message box
            //   but at this point while joining the game we don't know wether we will successfully join or not
            logger.info(
                "\nWARNING: You are missing some files referenced by the server:\n%s"
                "You might not be able to join the game\n"
                "Go to the setting menu to turn on autodownload, or get the file elsewhere",
                missingfiles
            );
        }
    } else if (FS_ComparePaks(clc.downloadList, sizeof(clc.downloadList), true)) {
        logger.info("Need paks: {}", clc.downloadList);

        if (*clc.downloadList) {
            // if autodownloading is not enabled on the server
            cls.state = CA_CONNECTED;
            CL_NextDownload();
            return;
        }
    }

    CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend()
{
    int32 port, i;
    char  info[MAX_INFO_STRING];
    char  data[MAX_INFO_STRING];

    // don't send anything if playing back a demo
    if (clc.demoplaying) {
        return;
    }

    // resend if we haven't gotten a reply yet
    if (cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING) {
        return;
    }

    if (cls.realtime - clc.connectTime < RETRANSMIT_TIMEOUT) {
        return;
    }

    clc.connectTime = cls.realtime; // for retransmit requests
    clc.connectPacketCount++;


    switch (cls.state) {
    case CA_CONNECTING:
        // requesting a challenge
        if (!Sys_IsLANAddress(clc.serverAddress)) {
            CL_RequestAuthorization();
        }
        NET_OutOfBandPrint(NS_CLIENT, clc.serverAddress, "getchallenge");
        break;

    case CA_CHALLENGING:
        // sending back the challenge
        port = Cvar_VariableValue("net_qport");

        Q_strncpyz(info, Cvar_InfoString(CVAR_USERINFO), sizeof(info));
        Info_SetValueForKey(info, "protocol", va("%i", PROTOCOL_VERSION));
        Info_SetValueForKey(info, "qport", va("%i", port));
        Info_SetValueForKey(info, "challenge", va("%i", clc.challenge));

        strcpy(data, "connect ");
        // TTimo adding " " around the userinfo string to avoid truncated userinfo on the server
        //   (Com_TokenizeString tokenizes around spaces)
        data[8] = '"';

        for (i = 0; i < strlen(info); i++) {
            data[9 + i] = info[i]; // + (clc.challenge)&0x3;
        }
        data[9 + i] = '"';
        data[10 + i] = 0;

        // NOTE TTimo don't forget to set the right data length!
        NET_OutOfBandData(NS_CLIENT, clc.serverAddress, (uint8*) &data[0], i + 10);
        // the most current userinfo has been sent, so watch for any
        // newer changes to userinfo variables
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        break;

    default:
        Com_Error(ERR_FATAL, "CL_CheckForResend: bad cls.state");
    }
}

/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket(netadr_t from)
{
    if (cls.state < CA_AUTHORIZING) {
        return;
    }

    // if not from our server, ignore it
    if (!NET_CompareAdr(from, clc.netchan.remoteAddress)) {
        return;
    }

    // if we have received packets within three seconds, ignore it
    // (it might be a malicious spoof)
    if (cls.realtime - clc.lastPacketTime < 3000) {
        return;
    }

    // drop the connection
    logger.info("Server disconnected for unknown reason");
    Cvar_Set("com_errorMessage", "Server disconnected for unknown reason\n");
    CL_Disconnect(true);
}


/*
===================
CL_MotdPacket

===================
*/
void CL_MotdPacket(netadr_t from)
{
    char*       challenge;
    const char* info;

    // if not from our server, ignore it
    if (!NET_CompareAdr(from, cls.updateServer)) {
        return;
    }

    info = Cmd_Argv(1);

    // check challenge
    challenge = Info_ValueForKey(info, "challenge");
    if (strcmp(challenge, cls.updateChallenge)) {
        return;
    }

    challenge = Info_ValueForKey(info, "motd");

    Q_strncpyz(cls.updateInfoString, info, sizeof(cls.updateInfoString));
    Cvar_Set("cl_motdString", challenge);
}

/*
===================
CL_InitServerInfo
===================
*/
void CL_InitServerInfo(serverInfo_t* server, serverAddress_t* address)
{
    server->adr.type = NA_IP;
    server->adr.ip[0] = address->ip[0];
    server->adr.ip[1] = address->ip[1];
    server->adr.ip[2] = address->ip[2];
    server->adr.ip[3] = address->ip[3];
    server->adr.port = address->port;
    server->clients = 0;
    server->hostName[0] = '\0';
    server->mapName[0] = '\0';
    server->maxClients = 0;
    server->maxPing = 0;
    server->minPing = 0;
    server->ping = -1;
    server->game[0] = '\0';
    server->gameType = 0;
    server->netType = 0;
}

#define MAX_SERVERSPERPACKET 256

/*
===================
CL_ServersResponsePacket
===================
*/
void CL_ServersResponsePacket(netadr_t from, msg_t* msg)
{
    int32           i, count, max, total;
    serverAddress_t addresses[MAX_SERVERSPERPACKET];
    int32           numservers;
    uint8*          buffptr;
    uint8*          buffend;

    logger.info("CL_ServersResponsePacket");

    if (cls.numglobalservers == -1) {
        // state to detect lack of servers or lack of response
        cls.numglobalservers = 0;
        cls.numGlobalServerAddresses = 0;
    }

    if (cls.nummplayerservers == -1) {
        cls.nummplayerservers = 0;
    }

    // parse through server response string
    numservers = 0;
    buffptr = msg->data;
    buffend = buffptr + msg->cursize;
    while (buffptr + 1 < buffend) {
        // advance to initial token
        do {
            if (*buffptr++ == '\\') {
                break;
            }
        } while (buffptr < buffend);

        if (buffptr >= buffend - 6) {
            break;
        }

        // parse out ip
        addresses[numservers].ip[0] = *buffptr++;
        addresses[numservers].ip[1] = *buffptr++;
        addresses[numservers].ip[2] = *buffptr++;
        addresses[numservers].ip[3] = *buffptr++;

        // parse out port
        addresses[numservers].port = (*buffptr++) << 8;
        addresses[numservers].port += *buffptr++;
        addresses[numservers].port = BigShort(addresses[numservers].port);

        // syntax check
        if (*buffptr != '\\') {
            break;
        }

        logger.debug("server: {} ip: {}.{}.{}.{}:{}", numservers, addresses[numservers].ip[0], addresses[numservers].ip[1], addresses[numservers].ip[2], addresses[numservers].ip[3], addresses[numservers].port);

        numservers++;
        if (numservers >= MAX_SERVERSPERPACKET) {
            break;
        }

        // parse out EOT
        if (buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T') {
            break;
        }
    }

    if (cls.masterNum == 0) {
        count = cls.numglobalservers;
        max = MAX_GLOBAL_SERVERS;
    } else {
        count = cls.nummplayerservers;
        max = MAX_OTHER_SERVERS;
    }

    for (i = 0; i < numservers && count < max; i++) {
        // build net address
        serverInfo_t* server = (cls.masterNum == 0) ? &cls.globalServers[count] : &cls.mplayerServers[count];

        CL_InitServerInfo(server, &addresses[i]);
        // advance to next slot
        count++;
    }

    // if getting the global list
    if (cls.masterNum == 0) {
        if (cls.numGlobalServerAddresses < MAX_GLOBAL_SERVERS) {
            // if we couldn't store the servers in the main list anymore
            for (; i < numservers && count >= max; i++) {
                serverAddress_t* addr;
                // just store the addresses in an additional list
                addr = &cls.globalServerAddresses[cls.numGlobalServerAddresses++];
                addr->ip[0] = addresses[i].ip[0];
                addr->ip[1] = addresses[i].ip[1];
                addr->ip[2] = addresses[i].ip[2];
                addr->ip[3] = addresses[i].ip[3];
                addr->port = addresses[i].port;
            }
        }
    }

    if (cls.masterNum == 0) {
        cls.numglobalservers = count;
        total = count + cls.numGlobalServerAddresses;
    } else {
        cls.nummplayerservers = count;
        total = count;
    }

    logger.info("{} servers parsed (total {})", numservers, total);
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket(netadr_t from, msg_t* msg)
{
    char*       s;
    const char* c;

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg); // skip the -1

    s = MSG_ReadStringLine(msg);

    Cmd_TokenizeString(s);

    c = Cmd_Argv(0);

    logger.debug("CL packet {}: {}", NET_AdrToString(from), c);

    // challenge from the server we are connecting to
    if (!Q_stricmp(c, "challengeResponse")) {
        if (cls.state != CA_CONNECTING) {
            logger.info("Unwanted challenge response received.  Ignored.");
        } else {
            // start sending challenge repsonse instead of challenge request packets
            clc.challenge = atoi(Cmd_Argv(1));
            cls.state = CA_CHALLENGING;
            clc.connectPacketCount = 0;
            clc.connectTime = -99999;

            // take this address as the new server address.  This allows
            // a server proxy to hand off connections to multiple servers
            clc.serverAddress = from;
            logger.debug("challengeResponse: {}", clc.challenge);
        }
        return;
    }

    // server connection
    if (!Q_stricmp(c, "connectResponse")) {
        if (cls.state >= CA_CONNECTED) {
            logger.info("Dup connect received.  Ignored.");
            return;
        }
        if (cls.state != CA_CHALLENGING) {
            logger.info("connectResponse packet while not connecting.  Ignored.");
            return;
        }
        if (!NET_CompareBaseAdr(from, clc.serverAddress)) {
            logger.info("connectResponse from a different address.  Ignored.");
            logger.info("{} should have been {}", NET_AdrToString(from), NET_AdrToString(clc.serverAddress));
            return;
        }
        Netchan_Setup(NS_CLIENT, &clc.netchan, from, Cvar_VariableValue("net_qport"));
        cls.state = CA_CONNECTED;
        clc.lastPacketSentTime = -9999; // send first packet immediately
        return;
    }

    // server responding to an info broadcast
    if (!Q_stricmp(c, "infoResponse")) {
        CL_ServerInfoPacket(from, msg);
        return;
    }

    // server responding to a get playerlist
    if (!Q_stricmp(c, "statusResponse")) {
        CL_ServerStatusResponse(from, msg);
        return;
    }

    // a disconnect message from the server, which will happen if the server
    // dropped the connection but it is still getting packets from us
    if (!Q_stricmp(c, "disconnect")) {
        CL_DisconnectPacket(from);
        return;
    }

    // echo request from server
    if (!Q_stricmp(c, "echo")) {
        NET_OutOfBandPrint(NS_CLIENT, from, "%s", Cmd_Argv(1));
        return;
    }

    // cd check
    if (!Q_stricmp(c, "keyAuthorize")) {
        // we don't use these now, so dump them on the floor
        return;
    }

    // global MOTD from id
    if (!Q_stricmp(c, "motd")) {
        CL_MotdPacket(from);
        return;
    }

    // echo request from server
    if (!Q_stricmp(c, "print")) {
        s = MSG_ReadString(msg);
        Q_strncpyz(clc.serverMessage, s, sizeof(clc.serverMessage));
        logger.info("{}", s);
        return;
    }

    // echo request from server
    if (!Q_strncmp(c, "getserversResponse", 18)) {
        CL_ServersResponsePacket(from, msg);
        return;
    }

    logger.debug("Unknown connectionless packet command.");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent(netadr_t from, msg_t* msg)
{
    int32 headerBytes;

    clc.lastPacketTime = cls.realtime;

    if (msg->cursize >= 4 && *(int32*) msg->data == -1) {
        CL_ConnectionlessPacket(from, msg);
        return;
    }

    if (cls.state < CA_CONNECTED) {
        return; // can't be a valid sequenced packet
    }

    if (msg->cursize < 4) {
        logger.info("{}: Runt packet", NET_AdrToString(from));
        return;
    }

    //
    // packet from server
    //
    if (!NET_CompareAdr(from, clc.netchan.remoteAddress)) {
        logger.debug("{}:sequenced packet without connection", NET_AdrToString(from));
        // FIXME: send a client disconnect?
        return;
    }

    if (!CL_Netchan_Process(&clc.netchan, msg)) {
        return; // out of order, duplicated, etc
    }

    // the header is different lengths for reliable and unreliable messages
    headerBytes = msg->readcount;

    // track the last message received so it can be returned in
    // client messages, allowing the server to detect a dropped
    // gamestate
    clc.serverMessageSequence = (*(int32*) msg->data);

    clc.lastPacketTime = cls.realtime;
    CL_ParseServerMessage(msg);

    //
    // we don't know if it is ok to save a demo message until
    // after we have parsed the frame
    //
    if (clc.demorecording && !clc.demowaiting) {
        CL_WriteDemoMessage(msg, headerBytes);
    }
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout()
{
    //
    // check timeout
    //
    if ((!cl_paused->integer || !sv_paused->integer)
        && cls.state >= CA_CONNECTED
        && cls.realtime - clc.lastPacketTime > cl_timeout->value * 1000) {
        if (++cl.timeoutcount > 5) { // timeoutcount saves debugger
            logger.info("Server connection timed out.");
            CL_Disconnect(true);
            return;
        }
    } else {
        cl.timeoutcount = 0;
    }
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo()
{
    // don't add reliable commands when not yet connected
    if (cls.state < CA_CHALLENGING) {
        return;
    }
    // don't overflow the reliable command buffer when paused
    if (cl_paused->integer) {
        return;
    }
    // send a reliable userinfo update if needed
    if (cvar_modifiedFlags & CVAR_USERINFO) {
        cvar_modifiedFlags &= ~CVAR_USERINFO;
        CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO)));
    }
}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int32 msec)
{
    if (!com_cl_running->integer) {
        return;
    }

    if (cls.state == CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_UI)
        && !com_sv_running->integer) {
        // if disconnected, bring up the menu
        S_StopAllSounds();
        UI_SetActiveMenu(UIMENU_MAIN);
    }

    // if recording an avi, lock to a fixed fps
    if (cl_avidemo->integer && msec) {
        // save the current screen
        if (cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
            Cbuf_ExecuteText(EXEC_NOW, "screenshot silent\n");
        }
        // fixed time for next frame'
        msec = (1000 / cl_avidemo->integer) * com_timescale->value;
        if (msec == 0) {
            msec = 1;
        }
    }

    // save the msec before checking pause
    cls.realFrametime = msec;

    // decide the simulation time
    cls.frametime = msec;

    cls.realtime += cls.frametime;

    if (cl_timegraph->integer) {
        SCR_DebugGraph(cls.realFrametime * 0.25, 0);
    }

    // see if we need to update any userinfo
    CL_CheckUserinfo();

    // if we haven't gotten a packet in a long time,
    // drop the connection
    CL_CheckTimeout();

    // send intentions now
    CL_SendCmd();

    // resend a connection request if necessary
    CL_CheckForResend();

    // decide on the serverTime to render
    CL_SetCGameTime();

    // update the screen
    SCR_UpdateScreen();

    // update audio
    S_Update();

    Con_RunConsole();

    cls.framecount++;
}


//============================================================================

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer()
{
    // this sets up the renderer and calls R_Init
    RE_BeginRegistration(&cls.glconfig);

    // load character sets
    cls.charSetShader = RE_RegisterShader("gfx/2d/bigchars");
    cls.whiteShader = RE_RegisterShader("white");
    cls.consoleShader = RE_RegisterShader("console");
    g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
    g_consoleField.widthInChars = g_console_field_width;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers()
{
    if (!com_cl_running) {
        return;
    }

    if (!com_cl_running->integer) {
        return;
    }

    if (!cls.rendererStarted) {
        cls.rendererStarted = true;
        CL_InitRenderer();
    }

    if (!cls.soundStarted) {
        cls.soundStarted = true;
        S_Init();
    }

    if (!cls.soundRegistered) {
        cls.soundRegistered = true;
        S_BeginRegistration();
    }

    if (!cls.uiStarted) {
        cls.uiStarted = true;
        CL_InitUI();
    }
}

//===========================================================================================


void CL_SetModel_f()
{
    const char* arg;
    char        name[256];

    arg = Cmd_Argv(1);
    if (arg[0]) {
        Cvar_Set("model", arg);
        Cvar_Set("headmodel", arg);
    } else {
        Cvar_VariableStringBuffer("model", name, sizeof(name));
        logger.info("model is set to {}", name);
    }
}

/*
====================
CL_Init
====================
*/
void CL_Init()
{
    logger.info("----- Client Initialization -----");

    Con_Init();

    CL_ClearState();

    cls.state = CA_DISCONNECTED; // no longer CA_UNINITIALIZED

    cls.realtime = 0;

    CL_InitInput();

    //
    // register our variables
    //
    cl_noprint = Cvar_Get("cl_noprint", "0", 0);
    cl_motd = Cvar_Get("cl_motd", "1", 0);

    cl_timeout = Cvar_Get("cl_timeout", "200", 0);

    cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
    cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
    cl_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
    cl_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
    cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
    rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
    cl_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);

    cl_timedemo = Cvar_Get("timedemo", "0", 0);
    cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
    cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

    rconAddress = Cvar_Get("rconAddress", "", 0);

    cl_yawspeed = Cvar_Get("cl_yawspeed", "140", CVAR_ARCHIVE);
    cl_pitchspeed = Cvar_Get("cl_pitchspeed", "140", CVAR_ARCHIVE);
    cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);

    cl_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
    cl_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

    cl_run = Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
    cl_sensitivity = Cvar_Get("sensitivity", "5", CVAR_ARCHIVE);
    cl_mouseAccel = Cvar_Get("cl_mouseAccel", "0", CVAR_ARCHIVE);
    cl_freelook = Cvar_Get("cl_freelook", "1", CVAR_ARCHIVE);

    cl_showMouseRate = Cvar_Get("cl_showmouserate", "0", 0);

    cl_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);

    cl_conXOffset = Cvar_Get("cl_conXOffset", "0", 0);

    cl_serverStatusResendTime = Cvar_Get("cl_serverStatusResendTime", "750", 0);

    m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
    m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
    m_forward = Cvar_Get("m_forward", "0.25", CVAR_ARCHIVE);
    m_side = Cvar_Get("m_side", "0.25", CVAR_ARCHIVE);

    m_filter = Cvar_Get("m_filter", "0", CVAR_ARCHIVE);

    cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

    Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);


    // userinfo
    Cvar_Get("name", "Player", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE);
    Cvar_Get("g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE);
    Cvar_Get("color1", "4", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("teamtask", "0", CVAR_USERINFO);
    Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
    Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

    Cvar_Get("password", "", CVAR_USERINFO);

    //
    // register our commands
    //
    Cmd_AddCommand("cmd", CL_ForwardToServer_f);
    Cmd_AddCommand("configstrings", CL_Configstrings_f);
    Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
    Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
    Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
    Cmd_AddCommand("disconnect", CL_Disconnect_f);
    Cmd_AddCommand("record", CL_Record_f);
    Cmd_AddCommand("demo", CL_PlayDemo_f);
    Cmd_AddCommand("stoprecord", CL_StopRecord_f);
    Cmd_AddCommand("connect", CL_Connect_f);
    Cmd_AddCommand("reconnect", CL_Reconnect_f);
    Cmd_AddCommand("localservers", CL_LocalServers_f);
    Cmd_AddCommand("globalservers", CL_GlobalServers_f);
    Cmd_AddCommand("rcon", CL_Rcon_f);
    Cmd_AddCommand("setenv", CL_Setenv_f);
    Cmd_AddCommand("ping", CL_Ping_f);
    Cmd_AddCommand("serverstatus", CL_ServerStatus_f);
    Cmd_AddCommand("showip", CL_ShowIP_f);
    Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
    Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);
    Cmd_AddCommand("model", CL_SetModel_f);
    // unpause so the cgame definately gets a snapshot and renders a frame
    Cvar_Set("cl_paused", "0");

    SCR_Init();

    Cbuf_Execute();

    Cvar_Set("cl_running", "1");

    logger.info("----- Client Initialization Complete -----");
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown()
{
    static bool recursive = false;

    logger.info("----- CL_Shutdown -----");

    if (recursive) {
        logger.info("recursive shutdown");
        return;
    }
    recursive = true;

    CL_Disconnect(true);

    S_Shutdown();
    RE_Shutdown(true);

    CL_ShutdownUI();

    Cmd_RemoveCommand("cmd");
    Cmd_RemoveCommand("configstrings");
    Cmd_RemoveCommand("userinfo");
    Cmd_RemoveCommand("snd_restart");
    Cmd_RemoveCommand("vid_restart");
    Cmd_RemoveCommand("disconnect");
    Cmd_RemoveCommand("record");
    Cmd_RemoveCommand("demo");
    Cmd_RemoveCommand("stoprecord");
    Cmd_RemoveCommand("connect");
    Cmd_RemoveCommand("localservers");
    Cmd_RemoveCommand("globalservers");
    Cmd_RemoveCommand("rcon");
    Cmd_RemoveCommand("setenv");
    Cmd_RemoveCommand("ping");
    Cmd_RemoveCommand("serverstatus");
    Cmd_RemoveCommand("showip");
    Cmd_RemoveCommand("model");

    Cvar_Set("cl_running", "0");

    recursive = false;

    Com_Memset(&cls, 0, sizeof(cls));

    logger.info("-----------------------");
}

static void CL_SetServerInfo(serverInfo_t* server, const char* info, int32 ping)
{
    if (server) {
        if (info) {
            server->clients = atoi(Info_ValueForKey(info, "clients"));
            Q_strncpyz(server->hostName, Info_ValueForKey(info, "hostname"), MAX_NAME_LENGTH);
            Q_strncpyz(server->mapName, Info_ValueForKey(info, "mapname"), MAX_NAME_LENGTH);
            server->maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));
            Q_strncpyz(server->game, Info_ValueForKey(info, "game"), MAX_NAME_LENGTH);
            server->gameType = atoi(Info_ValueForKey(info, "gametype"));
            server->netType = atoi(Info_ValueForKey(info, "nettype"));
            server->minPing = atoi(Info_ValueForKey(info, "minping"));
            server->maxPing = atoi(Info_ValueForKey(info, "maxping"));
            server->punkbuster = atoi(Info_ValueForKey(info, "punkbuster"));
        }
        server->ping = ping;
    }
}

static void CL_SetServerInfoByAddress(netadr_t from, const char* info, int32 ping)
{
    int32 i;

    for (i = 0; i < MAX_OTHER_SERVERS; i++) {
        if (NET_CompareAdr(from, cls.localServers[i].adr)) {
            CL_SetServerInfo(&cls.localServers[i], info, ping);
        }
    }

    for (i = 0; i < MAX_OTHER_SERVERS; i++) {
        if (NET_CompareAdr(from, cls.mplayerServers[i].adr)) {
            CL_SetServerInfo(&cls.mplayerServers[i], info, ping);
        }
    }

    for (i = 0; i < MAX_GLOBAL_SERVERS; i++) {
        if (NET_CompareAdr(from, cls.globalServers[i].adr)) {
            CL_SetServerInfo(&cls.globalServers[i], info, ping);
        }
    }

    for (i = 0; i < MAX_OTHER_SERVERS; i++) {
        if (NET_CompareAdr(from, cls.favoriteServers[i].adr)) {
            CL_SetServerInfo(&cls.favoriteServers[i], info, ping);
        }
    }
}

/*
===================
CL_ServerInfoPacket
===================
*/
void CL_ServerInfoPacket(netadr_t from, msg_t* msg)
{
    int32       i, type;
    char        info[MAX_INFO_STRING];
    const char* str;
    char*       infoString;
    int32       prot;

    infoString = MSG_ReadString(msg);

    // if this isn't the correct protocol version, ignore it
    prot = atoi(Info_ValueForKey(infoString, "protocol"));
    if (prot != PROTOCOL_VERSION) {
        logger.debug("Different protocol info packet: {}", infoString);
        return;
    }

    // iterate servers waiting for ping response
    for (i = 0; i < MAX_PINGREQUESTS; i++) {
        if (cl_pinglist[i].adr.port && !cl_pinglist[i].time && NET_CompareAdr(from, cl_pinglist[i].adr)) {
            // calc ping time
            cl_pinglist[i].time = cls.realtime - cl_pinglist[i].start + 1;
            logger.debug("ping time %dms from {}", cl_pinglist[i].time, NET_AdrToString(from));

            // save of info
            Q_strncpyz(cl_pinglist[i].info, infoString, sizeof(cl_pinglist[i].info));

            // tack on the net type
            // NOTE: make sure these types are in sync with the netnames strings in the UI
            switch (from.type) {
            case NA_BROADCAST:
            case NA_IP:
                str = "udp";
                type = 1;
                break;

            case NA_IPX:
            case NA_BROADCAST_IPX:
                str = "ipx";
                type = 2;
                break;

            default:
                str = "???";
                type = 0;
                break;
            }
            Info_SetValueForKey(cl_pinglist[i].info, "nettype", va("%d", type));
            CL_SetServerInfoByAddress(from, infoString, cl_pinglist[i].time);

            return;
        }
    }

    // if not just sent a local broadcast or pinging local servers
    if (cls.pingUpdateSource != AS_LOCAL) {
        return;
    }

    for (i = 0; i < MAX_OTHER_SERVERS; i++) {
        // empty slot
        if (cls.localServers[i].adr.port == 0) {
            break;
        }

        // avoid duplicate
        if (NET_CompareAdr(from, cls.localServers[i].adr)) {
            return;
        }
    }

    if (i == MAX_OTHER_SERVERS) {
        logger.debug("MAX_OTHER_SERVERS hit, dropping infoResponse");
        return;
    }

    // add this to the list
    cls.numlocalservers = i + 1;
    cls.localServers[i].adr = from;
    cls.localServers[i].clients = 0;
    cls.localServers[i].hostName[0] = '\0';
    cls.localServers[i].mapName[0] = '\0';
    cls.localServers[i].maxClients = 0;
    cls.localServers[i].maxPing = 0;
    cls.localServers[i].minPing = 0;
    cls.localServers[i].ping = -1;
    cls.localServers[i].game[0] = '\0';
    cls.localServers[i].gameType = 0;
    cls.localServers[i].netType = from.type;
    cls.localServers[i].punkbuster = 0;

    Q_strncpyz(info, MSG_ReadString(msg), MAX_INFO_STRING);
    if (strlen(info)) {
        if (info[strlen(info) - 1] != '\n') {
            strncat(info, "\n", sizeof(info));
        }
        logger.info("{}: {}", NET_AdrToString(from), info);
    }
}

/*
===================
CL_GetServerStatus
===================
*/
serverStatus_t* CL_GetServerStatus(netadr_t from)
{
    serverStatus_t* serverStatus;
    int32           i, oldest, oldestTime;

    serverStatus = NULL;
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if (NET_CompareAdr(from, cl_serverStatusList[i].address)) {
            return &cl_serverStatusList[i];
        }
    }
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if (cl_serverStatusList[i].retrieved) {
            return &cl_serverStatusList[i];
        }
    }
    oldest = -1;
    oldestTime = 0;
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if (oldest == -1 || cl_serverStatusList[i].startTime < oldestTime) {
            oldest = i;
            oldestTime = cl_serverStatusList[i].startTime;
        }
    }
    if (oldest != -1) {
        return &cl_serverStatusList[oldest];
    }
    serverStatusCount++;
    return &cl_serverStatusList[serverStatusCount & (MAX_SERVERSTATUSREQUESTS - 1)];
}

/*
===================
CL_ServerStatusResponse
===================
*/
void CL_ServerStatusResponse(netadr_t from, msg_t* msg)
{
    char*           s;
    char            info[MAX_INFO_STRING];
    int32           i, l, score, ping;
    int32           len;
    serverStatus_t* serverStatus;

    serverStatus = NULL;
    for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
        if (NET_CompareAdr(from, cl_serverStatusList[i].address)) {
            serverStatus = &cl_serverStatusList[i];
            break;
        }
    }
    // if we didn't request this server status
    if (!serverStatus) {
        return;
    }

    s = MSG_ReadStringLine(msg);

    len = 0;
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "%s", s);

    if (serverStatus->print) {
        logger.info("Server settings:");
        // print cvars
        while (*s) {
            for (i = 0; i < 2 && *s; i++) {
                if (*s == '\\') {
                    s++;
                }
                l = 0;
                while (*s) {
                    info[l++] = *s;
                    if (l >= MAX_INFO_STRING - 1) {
                        break;
                    }
                    s++;
                    if (*s == '\\') {
                        break;
                    }
                }
                info[l] = '\0';
                if (i) {
                    logger.info("{}", info);
                } else {
                    logger.info("%-24s", info);
                }
            }
        }
    }

    len = strlen(serverStatus->string);
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

    if (serverStatus->print) {
        logger.info("Players:");
        logger.info("num: score: ping: name:");
    }
    for (i = 0, s = MSG_ReadStringLine(msg); *s; s = MSG_ReadStringLine(msg), i++) {
        len = strlen(serverStatus->string);
        Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\%s", s);

        if (serverStatus->print) {
            score = ping = 0;
            sscanf(s, "%d %d", &score, &ping);
            s = strchr(s, ' ');
            if (s) {
                s = strchr(s + 1, ' ');
            }
            if (s) {
                s++;
            } else {
                s = (char*) "unknown";
            }
            logger.info("%-2d   %-3d    %-3d   {}", i, score, ping, s);
        }
    }
    len = strlen(serverStatus->string);
    Com_sprintf(&serverStatus->string[len], sizeof(serverStatus->string) - len, "\\");

    serverStatus->time = Com_Milliseconds();
    serverStatus->address = from;
    serverStatus->pending = false;
    if (serverStatus->print) {
        serverStatus->retrieved = true;
    }
}

/*
==================
CL_LocalServers_f
==================
*/
void CL_LocalServers_f()
{
    const char* message;
    int32       i, j;
    netadr_t    to;

    logger.info("Scanning for servers on the local network...");

    // reset the list, waiting for response
    cls.numlocalservers = 0;
    cls.pingUpdateSource = AS_LOCAL;

    for (i = 0; i < MAX_OTHER_SERVERS; i++) {
        bool b = cls.localServers[i].visible;
        Com_Memset(&cls.localServers[i], 0, sizeof(cls.localServers[i]));
        cls.localServers[i].visible = b;
    }
    Com_Memset(&to, 0, sizeof(to));

    // The 'xxx' in the message is a challenge that will be echoed back
    // by the server.  We don't care about that here, but master servers
    // can use that to prevent spoofed server responses from invalid ip
    message = "\377\377\377\377getinfo xxx";

    // send each message twice in case one is dropped
    for (i = 0; i < 2; i++) {
        // send a broadcast packet on each server port
        // we support multiple server ports so a single machine
        // can nicely run multiple servers
        for (j = 0; j < NUM_SERVER_PORTS; j++) {
            to.port = BigShort((int16) (PORT_SERVER + j));

            to.type = NA_BROADCAST;
            NET_SendPacket(NS_CLIENT, strlen(message), message, to);

            to.type = NA_BROADCAST_IPX;
            NET_SendPacket(NS_CLIENT, strlen(message), message, to);
        }
    }
}

/*
==================
CL_GlobalServers_f
==================
*/
void CL_GlobalServers_f()
{
    netadr_t to;
    int32    i;
    int32    count;
    char*    buffptr;
    char     command[1024];

    if (Cmd_Argc() < 3) {
        logger.info("usage: globalservers <master# 0-1> <protocol> [keywords]");
        return;
    }

    cls.masterNum = atoi(Cmd_Argv(1));

    logger.info("Requesting servers from the master...");

    // reset the list, waiting for response
    // -1 is used to distinguish a "no response"

    if (cls.masterNum == 1) {
        NET_StringToAdr(MASTER_SERVER_NAME, &to);
        cls.nummplayerservers = -1;
        cls.pingUpdateSource = AS_MPLAYER;
    } else {
        NET_StringToAdr(MASTER_SERVER_NAME, &to);
        cls.numglobalservers = -1;
        cls.pingUpdateSource = AS_GLOBAL;
    }
    to.type = NA_IP;
    to.port = BigShort(PORT_MASTER);

    sprintf(command, "getservers %s", Cmd_Argv(2));

    // tack on keywords
    buffptr = command + strlen(command);
    count = Cmd_Argc();
    for (i = 3; i < count; i++) {
        buffptr += sprintf(buffptr, " %s", Cmd_Argv(i));
    }

    NET_OutOfBandPrint(NS_SERVER, to, command);
}


/*
==================
CL_GetPing
==================
*/
void CL_GetPing(int32 n, char* buf, int32 buflen, int32* pingtime)
{
    const char* str;
    int32       time;
    int32       maxPing;

    if (!cl_pinglist[n].adr.port) {
        // empty slot
        buf[0] = '\0';
        *pingtime = 0;
        return;
    }

    str = NET_AdrToString(cl_pinglist[n].adr);
    Q_strncpyz(buf, str, buflen);

    time = cl_pinglist[n].time;
    if (!time) {
        // check for timeout
        time = cls.realtime - cl_pinglist[n].start;
        maxPing = Cvar_VariableIntegerValue("cl_maxPing");
        if (maxPing < 100) {
            maxPing = 100;
        }
        if (time < maxPing) {
            // not timed out yet
            time = 0;
        }
    }

    CL_SetServerInfoByAddress(cl_pinglist[n].adr, cl_pinglist[n].info, cl_pinglist[n].time);

    *pingtime = time;
}

/*
==================
CL_GetPingInfo
==================
*/
void CL_GetPingInfo(int32 n, char* buf, int32 buflen)
{
    if (!cl_pinglist[n].adr.port) {
        // empty slot
        if (buflen) {
            buf[0] = '\0';
        }
        return;
    }

    Q_strncpyz(buf, cl_pinglist[n].info, buflen);
}

/*
==================
CL_ClearPing
==================
*/
void CL_ClearPing(int32 n)
{
    if (n < 0 || n >= MAX_PINGREQUESTS) {
        return;
    }

    cl_pinglist[n].adr.port = 0;
}

/*
==================
CL_GetPingQueueCount
==================
*/
int32 CL_GetPingQueueCount()
{
    int32   i;
    int32   count;
    ping_t* pingptr;

    count = 0;
    pingptr = cl_pinglist;

    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        if (pingptr->adr.port) {
            count++;
        }
    }

    return (count);
}

/*
==================
CL_GetFreePing
==================
*/
ping_t* CL_GetFreePing()
{
    ping_t* pingptr;
    ping_t* best;
    int32   oldest;
    int32   i;
    int32   time;

    pingptr = cl_pinglist;
    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        // find free ping slot
        if (pingptr->adr.port) {
            if (!pingptr->time) {
                if (cls.realtime - pingptr->start < 500) {
                    // still waiting for response
                    continue;
                }
            } else if (pingptr->time < 500) {
                // results have not been queried
                continue;
            }
        }

        // clear it
        pingptr->adr.port = 0;
        return (pingptr);
    }

    // use oldest entry
    pingptr = cl_pinglist;
    best = cl_pinglist;
    oldest = INT_MIN;
    for (i = 0; i < MAX_PINGREQUESTS; i++, pingptr++) {
        // scan for oldest
        time = cls.realtime - pingptr->start;
        if (time > oldest) {
            oldest = time;
            best = pingptr;
        }
    }

    return (best);
}

/*
==================
CL_Ping_f
==================
*/
void CL_Ping_f()
{
    netadr_t    to;
    ping_t*     pingptr;
    const char* server;

    if (Cmd_Argc() != 2) {
        logger.info("usage: ping [server]");
        return;
    }

    Com_Memset(&to, 0, sizeof(netadr_t));

    server = Cmd_Argv(1);

    if (!NET_StringToAdr(server, &to)) {
        return;
    }

    pingptr = CL_GetFreePing();

    memcpy(&pingptr->adr, &to, sizeof(netadr_t));
    pingptr->start = cls.realtime;
    pingptr->time = 0;

    CL_SetServerInfoByAddress(pingptr->adr, NULL, 0);

    NET_OutOfBandPrint(NS_CLIENT, to, "getinfo xxx");
}

/*
==================
CL_ServerStatus_f
==================
*/
void CL_ServerStatus_f()
{
    netadr_t        to;
    const char*     server;
    serverStatus_t* serverStatus;

    Com_Memset(&to, 0, sizeof(netadr_t));

    if (Cmd_Argc() != 2) {
        if (cls.state != CA_ACTIVE || clc.demoplaying) {
            logger.info("Not connected to a server.");
            logger.info("Usage: serverstatus [server]");
            return;
        }
        server = cls.servername;
    } else {
        server = Cmd_Argv(1);
    }

    if (!NET_StringToAdr(server, &to)) {
        return;
    }

    NET_OutOfBandPrint(NS_CLIENT, to, "getstatus");

    serverStatus = CL_GetServerStatus(to);
    serverStatus->address = to;
    serverStatus->print = true;
    serverStatus->pending = true;
}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f()
{
    Sys_ShowIP();
}
