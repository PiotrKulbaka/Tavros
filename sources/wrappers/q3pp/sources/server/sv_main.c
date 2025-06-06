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

#include "server.h"

static tavros::core::logger logger("sv_main");

serverStatic_t svs;         // persistant server info
server_t       sv;          // local server

cvar_t* sv_fps;             // time rate for running non-clients
cvar_t* sv_timeout;         // seconds without any message
cvar_t* sv_zombietime;      // seconds to sink messages after disconnect
cvar_t* sv_rconPassword;    // password for remote server commands
cvar_t* sv_privatePassword; // password for the privateClient slots
cvar_t* sv_allowDownload;
cvar_t* sv_maxclients;

cvar_t* sv_privateClients;             // number of clients reserved for password
cvar_t* sv_hostname;
cvar_t* sv_master[MAX_MASTER_SERVERS]; // master server ip address
cvar_t* sv_reconnectlimit;             // minimum seconds between connect messages
cvar_t* sv_showloss;                   // report when usercmds are lost
cvar_t* sv_padPackets;                 // add nop bytes to messages
cvar_t* sv_killserver;                 // menu system can set to 1 to shut server down
cvar_t* sv_mapname;
cvar_t* sv_mapChecksum;
cvar_t* sv_serverid;
cvar_t* sv_maxRate;
cvar_t* sv_minPing;
cvar_t* sv_maxPing;
cvar_t* sv_gametype;
cvar_t* sv_pure;
cvar_t* sv_floodProtect;
cvar_t* sv_lanForceRate; // dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t* sv_strictAuth;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
===============
SV_ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
char* SV_ExpandNewlines(char* in)
{
    static char string[1024];
    int32       l;

    l = 0;
    while (*in && l < sizeof(string) - 3) {
        if (*in == '\n') {
            string[l++] = '\\';
            string[l++] = 'n';
        } else {
            string[l++] = *in;
        }
        in++;
    }
    string[l] = 0;

    return string;
}

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand(client_t* client, const char* cmd)
{
    int32 index, i;

    // this is very ugly but it's also a waste to for instance send multiple config string updates
    // for the same config string index in one snapshot

    client->reliableSequence++;
    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    // we check == instead of >= so a broadcast print added by SV_DropClient()
    // doesn't cause a recursive drop client
    if (client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1) {
        logger.info("===== pending server commands =====");
        for (i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++) {
            logger.info("cmd %5d: %s", i, client->reliableCommands[i & (MAX_RELIABLE_COMMANDS - 1)]);
        }
        logger.info("cmd %5d: %s", i, cmd);
        SV_DropClient(client, "Server command overflow");
        return;
    }
    index = client->reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(client->reliableCommands[index], cmd, sizeof(client->reliableCommands[index]));
}


/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A NULL client will broadcast to all clients
=================
*/
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...)
{
    va_list   argptr;
    uint8     message[MAX_MSGLEN];
    client_t* client;
    int32     j;

    va_start(argptr, fmt);
    Q_vsnprintf((char*) message, sizeof(message), fmt, argptr);
    va_end(argptr);

    if (cl != NULL) {
        SV_AddServerCommand(cl, (char*) message);
        return;
    }

    // hack to echo broadcast prints to console
    if (com_dedicated->integer && !strncmp((char*) message, "print", 5)) {
        logger.info("broadcast: %s", SV_ExpandNewlines((char*) message));
    }

    // send the data to all relevent clients
    for (j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++) {
        if (client->state < CS_PRIMED) {
            continue;
        }
        SV_AddServerCommand(client, (char*) message);
    }
}


/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
#define HEARTBEAT_MSEC 300 * 1000
#define HEARTBEAT_GAME "QuakeArena-1"
void SV_MasterHeartbeat()
{
    static netadr_t adr[MAX_MASTER_SERVERS];
    int32           i;

    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if (!com_dedicated || com_dedicated->integer != 2) {
        return; // only dedicated servers send heartbeats
    }

    // if not time yet, don't send anything
    if (svs.time < svs.nextHeartbeatTime) {
        return;
    }
    svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;


    // send to group masters
    for (i = 0; i < MAX_MASTER_SERVERS; i++) {
        if (!sv_master[i]->string[0]) {
            continue;
        }

        // see if we haven't already resolved the name
        // resolving usually causes hitches on win95, so only
        // do it when needed
        if (sv_master[i]->modified) {
            sv_master[i]->modified = false;

            logger.info("Resolving %s", sv_master[i]->string);
            if (!NET_StringToAdr(sv_master[i]->string, &adr[i])) {
                // if the address failed to resolve, clear it
                // so we don't take repeated dns hits
                logger.info("Couldn't resolve address: %s", sv_master[i]->string);
                Cvar_Set(sv_master[i]->name, "");
                sv_master[i]->modified = false;
                continue;
            }
            if (!strstr(":", sv_master[i]->string)) {
                adr[i].port = BigShort(PORT_MASTER);
            }
            logger.info("%s resolved to %i.%i.%i.%i:%i", sv_master[i]->string, adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3], BigShort(adr[i].port));
        }


        logger.info("Sending heartbeat to %s", sv_master[i]->string);
        // this command should be changed if the server info / status format
        // ever incompatably changes
        NET_OutOfBandPrint(NS_SERVER, adr[i], "heartbeat %s\n", HEARTBEAT_GAME);
    }
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown()
{
    // send a hearbeat right now
    svs.nextHeartbeatTime = -9999;
    SV_MasterHeartbeat();

    // send it again to minimize chance of drops
    svs.nextHeartbeatTime = -9999;
    SV_MasterHeartbeat();

    // when the master tries to poll the server, it won't respond, so
    // it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status(netadr_t from)
{
    char           player[1024];
    char           status[MAX_MSGLEN];
    int32          i;
    client_t*      cl;
    playerState_t* ps;
    int32          statusLength;
    int32          playerLength;
    char           infostring[MAX_INFO_STRING];

    // ignore if we are in single player
    if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER) {
        return;
    }

    strcpy(infostring, Cvar_InfoString(CVAR_SERVERINFO));

    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));

    status[0] = 0;
    statusLength = 0;

    for (i = 0; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];
        if (cl->state >= CS_CONNECTED) {
            ps = SV_GameClientNum(i);
            Com_sprintf(player, sizeof(player), "%i %i \"%s\"\n", ps->persistant[PERS_SCORE], cl->ping, cl->name);
            playerLength = strlen(player);
            if (statusLength + playerLength >= sizeof(status)) {
                break; // can't hold any more
            }
            strcpy(status + statusLength, player);
            statusLength += playerLength;
        }
    }

    NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

/*
================
SVC_Info

Responds with a int16 info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info(netadr_t from)
{
    int32       i, count;
    const char* gamedir;
    char        infostring[MAX_INFO_STRING];

    // ignore if we are in single player
    if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")) {
        return;
    }

    // don't count privateclients
    count = 0;
    for (i = sv_privateClients->integer; i < sv_maxclients->integer; i++) {
        if (svs.clients[i].state >= CS_CONNECTED) {
            count++;
        }
    }

    infostring[0] = 0;

    // echo back the parameter to status. so servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));

    Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION));
    Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
    Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
    Info_SetValueForKey(infostring, "clients", va("%i", count));
    Info_SetValueForKey(infostring, "sv_maxclients", va("%i", sv_maxclients->integer - sv_privateClients->integer));
    Info_SetValueForKey(infostring, "gametype", va("%i", sv_gametype->integer));
    Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer));

    if (sv_minPing->integer) {
        Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer));
    }
    if (sv_maxPing->integer) {
        Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer));
    }
    gamedir = Cvar_VariableString("fs_game");
    if (*gamedir) {
        Info_SetValueForKey(infostring, "game", gamedir);
    }

    NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

/*
================
SVC_FlushRedirect

================
*/
void SV_FlushRedirect(char* outputbuf)
{
    NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand(netadr_t from, msg_t* msg)
{
    bool   valid;
    uint32 time;
    char   remaining[1024];
    // TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
    // (OOB messages are the bottleneck here)
#define SV_OUTPUTBUF_LENGTH (1024 - 16)
    char          sv_outputbuf[SV_OUTPUTBUF_LENGTH];
    static uint32 lasttime = 0;
    char*         cmd_aux;

    time = Com_Milliseconds();
    if (time < (lasttime + 500)) {
        return;
    }
    lasttime = time;

    if (!strlen(sv_rconPassword->string) || strcmp(Cmd_Argv(1), sv_rconPassword->string)) {
        valid = false;
        logger.info("Bad rcon from %s:\n%s", NET_AdrToString(from), Cmd_Argv(2));
    } else {
        valid = true;
        logger.info("Rcon from %s:\n%s", NET_AdrToString(from), Cmd_Argv(2));
    }

    // start redirecting all print outputs to the packet
    svs.redirectAddress = from;
    Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

    if (!strlen(sv_rconPassword->string)) {
        logger.info("No rconpassword set on the server.");
    } else if (!valid) {
        logger.info("Bad rconpassword.");
    } else {
        remaining[0] = 0;

        // get the command directly, "rcon <pass> <command>" to avoid quoting issues
        // extract the command by walking
        // since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
        cmd_aux = Cmd_Cmd();
        cmd_aux += 4;
        while (cmd_aux[0] == ' ') {
            cmd_aux++;
        }
        while (cmd_aux[0] && cmd_aux[0] != ' ') { // password
            cmd_aux++;
        }
        while (cmd_aux[0] == ' ') {
            cmd_aux++;
        }

        Q_strcat(remaining, sizeof(remaining), cmd_aux);

        Cmd_ExecuteString(remaining);
    }

    Com_EndRedirect();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket(netadr_t from, msg_t* msg)
{
    const char* s;
    const char* c;

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg); // skip the -1 marker

    if (!Q_strncmp("connect", (const char*) &msg->data[4], 7)) {
        Huff_Decompress(msg, 12);
    }

    s = MSG_ReadStringLine(msg);
    Cmd_TokenizeString(s);

    c = Cmd_Argv(0);
    logger.debug("SV packet %s : %s", NET_AdrToString(from), c);

    if (!Q_stricmp(c, "getstatus")) {
        SVC_Status(from);
    } else if (!Q_stricmp(c, "getinfo")) {
        SVC_Info(from);
    } else if (!Q_stricmp(c, "getchallenge")) {
        SV_GetChallenge(from);
    } else if (!Q_stricmp(c, "connect")) {
        SV_DirectConnect(from);
    } else if (!Q_stricmp(c, "ipAuthorize")) {
        SV_AuthorizeIpPacket(from);
    } else if (!Q_stricmp(c, "rcon")) {
        SVC_RemoteCommand(from, msg);
    } else if (!Q_stricmp(c, "disconnect")) {
        // if a client starts up a local server, we may see some spurious
        // server disconnect messages when their new server sees our final
        // sequenced messages to the old client
    } else {
        logger.debug("bad connectionless packet from %s:\n%s", NET_AdrToString(from), s);
    }
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent(netadr_t from, msg_t* msg)
{
    int32     i;
    client_t* cl;
    int32     qport;

    // check for connectionless packet (0xffffffff) first
    if (msg->cursize >= 4 && *(int32*) msg->data == -1) {
        SV_ConnectionlessPacket(from, msg);
        return;
    }

    // read the qport out of the message so we can fix up
    // stupid address translating routers
    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg); // sequence number
    qport = MSG_ReadShort(msg) & 0xffff;

    // find which client the message is from
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (cl->state == CS_FREE) {
            continue;
        }
        if (!NET_CompareBaseAdr(from, cl->netchan.remoteAddress)) {
            continue;
        }
        // it is possible to have multiple clients from a single IP
        // address, so they are differentiated by the qport variable
        if (cl->netchan.qport != qport) {
            continue;
        }

        // the IP port can't be used to differentiate them, because
        // some address translating routers periodically change UDP
        // port assignments
        if (cl->netchan.remoteAddress.port != from.port) {
            logger.info("SV_PacketEvent: fixing up a translated port");
            cl->netchan.remoteAddress.port = from.port;
        }

        // make sure it is a valid, in sequence packet
        if (SV_Netchan_Process(cl, msg)) {
            // zombie clients still need to do the Netchan_Process
            // to make sure they don't need to retransmit the final
            // reliable message, but they don't do any other processing
            if (cl->state != CS_ZOMBIE) {
                cl->lastPacketTime = svs.time; // don't timeout
                SV_ExecuteClientMessage(cl, msg);
            }
        }
        return;
    }

    // if we received a sequenced packet from an address we don't recognize,
    // send an out of band disconnect packet to it
    NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}


/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings()
{
    int32          i, j;
    client_t*      cl;
    int32          total, count;
    int32          delta;
    playerState_t* ps;

    for (i = 0; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];
        if (cl->state != CS_ACTIVE) {
            cl->ping = 999;
            continue;
        }
        if (!cl->gentity) {
            cl->ping = 999;
            continue;
        }
        if (cl->gentity->r.svFlags & SVF_BOT) {
            cl->ping = 0;
            continue;
        }

        total = 0;
        count = 0;
        for (j = 0; j < PACKET_BACKUP; j++) {
            if (cl->frames[j].messageAcked <= 0) {
                continue;
            }
            delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
            count++;
            total += delta;
        }
        if (!count) {
            cl->ping = 999;
        } else {
            cl->ping = total / count;
            if (cl->ping > 999) {
                cl->ping = 999;
            }
        }

        // let the game dll know about the ping
        ps = SV_GameClientNum(i);
        ps->ping = cl->ping;
    }
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts()
{
    int32     i;
    client_t* cl;
    int32     droppoint;
    int32     zombiepoint;

    droppoint = svs.time - 1000 * sv_timeout->integer;
    zombiepoint = svs.time - 1000 * sv_zombietime->integer;

    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        // message times may be wrong across a changelevel
        if (cl->lastPacketTime > svs.time) {
            cl->lastPacketTime = svs.time;
        }

        if (cl->state == CS_ZOMBIE
            && cl->lastPacketTime < zombiepoint) {
            // using the client id cause the cl->name is empty at this point
            logger.debug("Going from CS_ZOMBIE to CS_FREE for client %d", i);
            cl->state = CS_FREE; // can now be reused
            continue;
        }
        if (cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint) {
            // wait several frames so a debugger session doesn't
            // cause a timeout
            if (++cl->timeoutCount > 5) {
                SV_DropClient(cl, "timed out");
                cl->state = CS_FREE; // don't bother with zombie state
            }
        } else {
            cl->timeoutCount = 0;
        }
    }
}


/*
==================
SV_CheckPaused
==================
*/
bool SV_CheckPaused()
{
    int32     count;
    client_t* cl;
    int32     i;

    if (!cl_paused->integer) {
        return false;
    }

    // only pause if there is just a single client connected
    count = 0;
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT) {
            count++;
        }
    }

    if (count > 1) {
        // don't pause
        if (sv_paused->integer) {
            Cvar_Set("sv_paused", "0");
        }
        return false;
    }

    if (!sv_paused->integer) {
        Cvar_Set("sv_paused", "1");
    }
    return true;
}

/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame(int32 msec)
{
    int32 frameMsec;
    int32 startTime;

    // the menu kills the server with this cvar
    if (sv_killserver->integer) {
        SV_Shutdown("Server was killed.\n");
        Cvar_Set("sv_killserver", "0");
        return;
    }

    if (!com_sv_running->integer) {
        return;
    }

    // allow pause if only the local client is connected
    if (SV_CheckPaused()) {
        return;
    }

    // if it isn't time for the next frame, do nothing
    if (sv_fps->integer < 1) {
        Cvar_Set("sv_fps", "10");
    }
    frameMsec = 1000 / sv_fps->integer;

    sv.timeResidual += msec;

    if (!com_dedicated->integer) {
        SV_BotFrame(svs.time + sv.timeResidual);
    }

    if (com_dedicated->integer && sv.timeResidual < frameMsec) {
        // NET_Sleep will give the OS time slices until either get a packet
        // or time enough for a server frame has gone by
        NET_Sleep(frameMsec - sv.timeResidual);
        return;
    }

    // if time is about to hit the 32nd bit, kick all clients
    // and clear sv.time, rather
    // than checking for negative time wraparound everywhere.
    // 2giga-milliseconds = 23 days, so it won't be too often
    if (svs.time > 0x70000000) {
        SV_Shutdown("Restarting server due to time wrapping");
        Cbuf_AddText("vstr nextmap\n");
        return;
    }
    // this can happen considerably earlier when lots of clients play and the map doesn't change
    if (svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities) {
        SV_Shutdown("Restarting server due to numSnapshotEntities wrapping");
        Cbuf_AddText("vstr nextmap\n");
        return;
    }

    if (sv.restartTime && svs.time >= sv.restartTime) {
        sv.restartTime = 0;
        Cbuf_AddText("map_restart 0\n");
        return;
    }

    // update infostrings if anything has been changed
    if (cvar_modifiedFlags & CVAR_SERVERINFO) {
        SV_SetConfigstring(CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO));
        cvar_modifiedFlags &= ~CVAR_SERVERINFO;
    }
    if (cvar_modifiedFlags & CVAR_SYSTEMINFO) {
        SV_SetConfigstring(CS_SYSTEMINFO, Cvar_InfoString_Big(CVAR_SYSTEMINFO));
        cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    }

    if (com_speeds->integer) {
        startTime = Sys_Milliseconds();
    } else {
        startTime = 0; // quite a compiler warning
    }

    // update ping based on the all received frames
    SV_CalcPings();

    if (com_dedicated->integer) {
        SV_BotFrame(svs.time);
    }

    // run the game simulation in chunks
    while (sv.timeResidual >= frameMsec) {
        sv.timeResidual -= frameMsec;
        svs.time += frameMsec;

        // let everything in the world think and move
        G_RunFrame(svs.time);
    }

    if (com_speeds->integer) {
        time_game = Sys_Milliseconds() - startTime;
    }

    // check timeouts
    SV_CheckTimeouts();

    // send messages back to the clients
    SV_SendClientMessages();

    // send a heartbeat to the master if needed
    SV_MasterHeartbeat();
}

//============================================================================

