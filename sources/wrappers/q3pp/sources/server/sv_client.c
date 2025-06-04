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
// sv_client.c -- server code for dealing with clients

#include "server.h"

static tavros::core::logger logger("sv_client");

static void SV_CloseDownload(client_t* cl);

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that ip.
=================
*/
void SV_GetChallenge(netadr_t from)
{
    int32        i;
    int32        oldest;
    int32        oldestTime;
    challenge_t* challenge;

    // ignore if we are in single player
    if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")) {
        return;
    }

    oldest = 0;
    oldestTime = 0x7fffffff;

    // see if we already have a challenge for this ip
    challenge = &svs.challenges[0];
    for (i = 0; i < MAX_CHALLENGES; i++, challenge++) {
        if (!challenge->connected && NET_CompareAdr(from, challenge->adr)) {
            break;
        }
        if (challenge->time < oldestTime) {
            oldestTime = challenge->time;
            oldest = i;
        }
    }

    if (i == MAX_CHALLENGES) {
        // this is the first time this client has asked for a challenge
        challenge = &svs.challenges[oldest];

        challenge->challenge = ((rand() << 16) ^ rand()) ^ svs.time;
        challenge->adr = from;
        challenge->firstTime = svs.time;
        challenge->time = svs.time;
        challenge->connected = false;
        i = oldest;
    }

    // if they are on a lan address, send the challengeResponse immediately
    if (Sys_IsLANAddress(from)) {
        challenge->pingTime = svs.time;
        NET_OutOfBandPrint(NS_SERVER, from, "challengeResponse %i", challenge->challenge);
        return;
    }

    // look up the authorize server's IP
    if (!svs.authorizeAddress.ip[0] && svs.authorizeAddress.type != NA_BAD) {
        logger.info("Resolving %s", AUTHORIZE_SERVER_NAME);
        if (!NET_StringToAdr(AUTHORIZE_SERVER_NAME, &svs.authorizeAddress)) {
            logger.info("Couldn't resolve address");
            return;
        }
        svs.authorizeAddress.port = BigShort(PORT_AUTHORIZE);
        logger.info("%s resolved to %i.%i.%i.%i:%i", AUTHORIZE_SERVER_NAME, svs.authorizeAddress.ip[0], svs.authorizeAddress.ip[1], svs.authorizeAddress.ip[2], svs.authorizeAddress.ip[3], BigShort(svs.authorizeAddress.port));
    }

    // if they have been challenging for a long time and we
    // haven't heard anything from the authorize server, go ahead and
    // let them in, assuming the id server is down
    if (svs.time - challenge->firstTime > AUTHORIZE_TIMEOUT) {
        logger.debug("authorize server timed out");

        challenge->pingTime = svs.time;
        NET_OutOfBandPrint(NS_SERVER, challenge->adr, "challengeResponse %i", challenge->challenge);
        return;
    }

    // otherwise send their ip to the authorize server
    if (svs.authorizeAddress.type != NA_BAD) {
        cvar_t* fs;
        char    game[1024];

        logger.debug("sending getIpAuthorize for %s", NET_AdrToString(from));

        strcpy(game, BASEGAME);
        fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
        if (fs && fs->string[0] != 0) {
            strcpy(game, fs->string);
        }

        // the 0 is for backwards compatibility with obsolete sv_allowanonymous flags
        // getIpAuthorize <challenge> <IP> <game> 0 <auth-flag>
        NET_OutOfBandPrint(NS_SERVER, svs.authorizeAddress, "getIpAuthorize %i %i.%i.%i.%i %s 0 %s", svs.challenges[i].challenge, from.ip[0], from.ip[1], from.ip[2], from.ip[3], game, sv_strictAuth->string);
    }
}

/*
====================
SV_AuthorizeIpPacket

A packet has been returned from the authorize server.
If we have a challenge adr for that ip, send the
challengeResponse to it
====================
*/
void SV_AuthorizeIpPacket(netadr_t from)
{
    int32       challenge;
    int32       i;
    const char* s;
    const char* r;
    char        ret[1024];

    if (!NET_CompareBaseAdr(from, svs.authorizeAddress)) {
        logger.info("SV_AuthorizeIpPacket: not from authorize server");
        return;
    }

    challenge = atoi(Cmd_Argv(1));

    for (i = 0; i < MAX_CHALLENGES; i++) {
        if (svs.challenges[i].challenge == challenge) {
            break;
        }
    }
    if (i == MAX_CHALLENGES) {
        logger.info("SV_AuthorizeIpPacket: challenge not found");
        return;
    }

    // send a packet back to the original client
    svs.challenges[i].pingTime = svs.time;
    s = Cmd_Argv(2);
    r = Cmd_Argv(3); // reason

    if (!Q_stricmp(s, "accept")) {
        NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "challengeResponse %i", svs.challenges[i].challenge);
        return;
    }
    if (!Q_stricmp(s, "unknown")) {
        if (!r) {
            NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nAwaiting CD key authorization\n");
        } else {
            sprintf(ret, "print\n%s\n", r);
            NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, ret);
        }
        // clear the challenge record so it won't timeout and let them through
        Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
        return;
    }

    // authorization failed
    if (!r) {
        NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, "print\nSomeone is using this CD Key\n");
    } else {
        sprintf(ret, "print\n%s\n", r);
        NET_OutOfBandPrint(NS_SERVER, svs.challenges[i].adr, ret);
    }

    // clear the challenge record so it won't timeout and let them through
    Com_Memset(&svs.challenges[i], 0, sizeof(svs.challenges[i]));
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/

#define PB_MESSAGE                                                                            \
    "PunkBuster Anti-Cheat software must be installed "                                       \
    "and Enabled in order to join this server. An updated game patch can be downloaded from " \
    "www.idsoftware.com"

void SV_DirectConnect(netadr_t from)
{
    char            userinfo[MAX_INFO_STRING];
    int32           i;
    client_t *      cl, *newcl;
    client_t        temp;
    sharedEntity_t* ent;
    int32           clientNum;
    int32           version;
    int32           qport;
    int32           challenge;
    char*           password;
    int32           startIndex;
    char*           denied;
    int32           count;

    logger.debug("SVC_DirectConnect()");

    Q_strncpyz(userinfo, Cmd_Argv(1), sizeof(userinfo));

    version = atoi(Info_ValueForKey(userinfo, "protocol"));
    if (version != PROTOCOL_VERSION) {
        NET_OutOfBandPrint(NS_SERVER, from, "print\nServer uses protocol version %i.\n", PROTOCOL_VERSION);
        logger.debug("    rejected connect from version %i", version);
        return;
    }

    challenge = atoi(Info_ValueForKey(userinfo, "challenge"));
    qport = atoi(Info_ValueForKey(userinfo, "qport"));

    // quick reject
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (cl->state == CS_FREE) {
            continue;
        }
        if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress)
            && (cl->netchan.qport == qport
                || from.port == cl->netchan.remoteAddress.port)) {
            if ((svs.time - cl->lastConnectTime)
                < (sv_reconnectlimit->integer * 1000)) {
                logger.debug("%s:reconnect rejected : too soon", NET_AdrToString(from));
                return;
            }
            break;
        }
    }

    // see if the challenge is valid (LAN clients don't need to challenge)
    if (!NET_IsLocalAddress(from)) {
        int32 ping;

        for (i = 0; i < MAX_CHALLENGES; i++) {
            if (NET_CompareAdr(from, svs.challenges[i].adr)) {
                if (challenge == svs.challenges[i].challenge) {
                    break; // good
                }
            }
        }
        if (i == MAX_CHALLENGES) {
            NET_OutOfBandPrint(NS_SERVER, from, "print\nNo or bad challenge for address.\n");
            return;
        }
        // force the IP key/value pair so the game can filter based on ip
        Info_SetValueForKey(userinfo, "ip", NET_AdrToString(from));

        ping = svs.time - svs.challenges[i].pingTime;
        logger.info("Client %i connecting with %i challenge ping", i, ping);
        svs.challenges[i].connected = true;

        // never reject a LAN client based on ping
        if (!Sys_IsLANAddress(from)) {
            if (sv_minPing->value && ping < sv_minPing->value) {
                // don't let them keep trying until they get a big delay
                NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for high pings only\n");
                logger.debug("Client %i rejected on a too low ping", i);
                // reset the address otherwise their ping will keep increasing
                // with each connect message and they'd eventually be able to connect
                svs.challenges[i].adr.port = 0;
                return;
            }
            if (sv_maxPing->value && ping > sv_maxPing->value) {
                NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is for low pings only\n");
                logger.debug("Client %i rejected on a too high ping", i);
                return;
            }
        }
    } else {
        // force the "ip" info key to "localhost"
        Info_SetValueForKey(userinfo, "ip", "localhost");
    }

    newcl = &temp;
    Com_Memset(newcl, 0, sizeof(client_t));

    // if there is already a slot for this ip, reuse it
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (cl->state == CS_FREE) {
            continue;
        }
        if (NET_CompareBaseAdr(from, cl->netchan.remoteAddress)
            && (cl->netchan.qport == qport
                || from.port == cl->netchan.remoteAddress.port)) {
            logger.info("%s:reconnect", NET_AdrToString(from));
            newcl = cl;

            goto gotnewcl;
        }
    }

    // find a client slot
    // if "sv_privateClients" is set > 0, then that number
    // of client slots will be reserved for connections that
    // have "password" set to the value of "sv_privatePassword"
    // Info requests will report the maxclients as if the private
    // slots didn't exist, to prevent people from trying to connect
    // to a full server.
    // This is to allow us to reserve a couple slots here on our
    // servers so we can play without having to kick people.

    // check for privateClient password
    password = Info_ValueForKey(userinfo, "password");
    if (!strcmp(password, sv_privatePassword->string)) {
        startIndex = 0;
    } else {
        // skip past the reserved slots
        startIndex = sv_privateClients->integer;
    }

    newcl = NULL;
    for (i = startIndex; i < sv_maxclients->integer; i++) {
        cl = &svs.clients[i];
        if (cl->state == CS_FREE) {
            newcl = cl;
            break;
        }
    }

    if (!newcl) {
        if (NET_IsLocalAddress(from)) {
            count = 0;
            for (i = startIndex; i < sv_maxclients->integer; i++) {
                cl = &svs.clients[i];
                if (cl->netchan.remoteAddress.type == NA_BOT) {
                    count++;
                }
            }
            // if they're all bots
            if (count >= sv_maxclients->integer - startIndex) {
                SV_DropClient(&svs.clients[sv_maxclients->integer - 1], "only bots on server");
                newcl = &svs.clients[sv_maxclients->integer - 1];
            } else {
                Com_Error(ERR_FATAL, "server is full on local connect\n");
                return;
            }
        } else {
            NET_OutOfBandPrint(NS_SERVER, from, "print\nServer is full.\n");
            logger.debug("Rejected a connection.");
            return;
        }
    }

    // we got a newcl, so reset the reliableSequence and reliableAcknowledge
    cl->reliableAcknowledge = 0;
    cl->reliableSequence = 0;

gotnewcl:
    // build a new connection
    // accept the new client
    // this is the only place a client_t is ever initialized
    *newcl = temp;
    clientNum = newcl - svs.clients;
    ent = SV_GentityNum(clientNum);
    newcl->gentity = ent;

    // save the challenge
    newcl->challenge = challenge;

    // save the address
    Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport);
    // init the netchan queue
    newcl->netchan_end_queue = &newcl->netchan_start_queue;

    // save the userinfo
    Q_strncpyz(newcl->userinfo, userinfo, sizeof(newcl->userinfo));

    // get the game a chance to reject this connection or modify the userinfo
    denied = (char*) ClientConnect(clientNum, true, false); // firstTime = true
    if (denied) {
        NET_OutOfBandPrint(NS_SERVER, from, "print\n%s\n", denied);
        logger.debug("Game rejected a connection: %s.", denied);
        return;
    }

    SV_UserinfoChanged(newcl);

    // send the connect packet to the client
    NET_OutOfBandPrint(NS_SERVER, from, "connectResponse");

    logger.debug("Going from CS_FREE to CS_CONNECTED for %s", newcl->name);

    newcl->state = CS_CONNECTED;
    newcl->nextSnapshotTime = svs.time;
    newcl->lastPacketTime = svs.time;
    newcl->lastConnectTime = svs.time;

    // when we receive the first packet from the client, we will
    // notice that it is from a different serverid and that the
    // gamestate message was not just sent, forcing a retransmit
    newcl->gamestateMessageNum = -1;

    // if this was the first client on the server, or the last client
    // the server can hold, send a heartbeat to the master.
    count = 0;
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        if (svs.clients[i].state >= CS_CONNECTED) {
            count++;
        }
    }
    if (count == 1 || count == sv_maxclients->integer) {
        SV_Heartbeat_f();
    }
}


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalMessage() will handle that
=====================
*/
void SV_DropClient(client_t* drop, const char* reason)
{
    int32        i;
    challenge_t* challenge;

    if (drop->state == CS_ZOMBIE) {
        return; // already dropped
    }

    if (!drop->gentity || !(drop->gentity->r.svFlags & SVF_BOT)) {
        // see if we already have a challenge for this ip
        challenge = &svs.challenges[0];

        for (i = 0; i < MAX_CHALLENGES; i++, challenge++) {
            if (NET_CompareAdr(drop->netchan.remoteAddress, challenge->adr)) {
                challenge->connected = false;
                break;
            }
        }
    }

    // Kill any download
    SV_CloseDownload(drop);

    // tell everyone why they got dropped
    SV_SendServerCommand(NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason);

    logger.debug("Going to CS_ZOMBIE for %s", drop->name);
    drop->state = CS_ZOMBIE; // become free in a few seconds

    if (drop->download) {
        FS_FCloseFile(drop->download);
        drop->download = 0;
    }

    // call the prog function for removing a client
    // this will remove the body, among other things
    ClientDisconnect(drop - svs.clients);

    // add the disconnect command
    SV_SendServerCommand(drop, "disconnect \"%s\"", reason);

    if (drop->netchan.remoteAddress.type == NA_BOT) {
        SV_BotFreeClient(drop - svs.clients);
    }

    // nuke user info
    SV_SetUserinfo(drop - svs.clients, "");

    // if this was the last client on the server, send a heartbeat
    // to the master so it is known the server is empty
    // send a heartbeat now so the master will get up to date info
    // if there is already a slot for this ip, reuse it
    for (i = 0; i < sv_maxclients->integer; i++) {
        if (svs.clients[i].state >= CS_CONNECTED) {
            break;
        }
    }
    if (i == sv_maxclients->integer) {
        SV_Heartbeat_f();
    }
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState(client_t* client)
{
    int32          start;
    entityState_t *base, nullstate;
    msg_t          msg;
    uint8          msgBuffer[MAX_MSGLEN];

    logger.debug("SV_SendClientGameState() for %s", client->name);
    logger.debug("Going from CS_CONNECTED to CS_PRIMED for %s", client->name);
    client->state = CS_PRIMED;
    client->pureAuthentic = 0;
    client->gotCP = false;

    // when we receive the first packet from the client, we will
    // notice that it is from a different serverid and that the
    // gamestate message was not just sent, forcing a retransmit
    client->gamestateMessageNum = client->netchan.outgoingSequence;

    MSG_Init(&msg, msgBuffer, sizeof(msgBuffer));

    // NOTE, MRE: all server->client messages now acknowledge
    // let the client know which reliable clientCommands we have received
    MSG_WriteLong(&msg, client->lastClientCommand);

    // send any server commands waiting to be sent first.
    // we have to do this cause we send the client->reliableSequence
    // with a gamestate and it sets the clc.serverCommandSequence at
    // the client side
    SV_UpdateServerCommandsToClient(client, &msg);

    // send the gamestate
    MSG_WriteByte(&msg, svc_gamestate);
    MSG_WriteLong(&msg, client->reliableSequence);

    // write the configstrings
    for (start = 0; start < MAX_CONFIGSTRINGS; start++) {
        if (sv.configstrings[start][0]) {
            MSG_WriteByte(&msg, svc_configstring);
            MSG_WriteShort(&msg, start);
            MSG_WriteBigString(&msg, sv.configstrings[start]);
        }
    }

    // write the baselines
    Com_Memset(&nullstate, 0, sizeof(nullstate));
    for (start = 0; start < MAX_GENTITIES; start++) {
        base = &sv.svEntities[start].baseline;
        if (!base->number) {
            continue;
        }
        MSG_WriteByte(&msg, svc_baseline);
        MSG_WriteDeltaEntity(&msg, &nullstate, base, true);
    }

    MSG_WriteByte(&msg, svc_EOF);

    MSG_WriteLong(&msg, client - svs.clients);

    // write the checksum feed
    MSG_WriteLong(&msg, sv.checksumFeed);

    // deliver this to the client
    SV_SendMessageToClient(&msg, client);
}


/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld(client_t* client, usercmd_t* cmd)
{
    int32           clientNum;
    sharedEntity_t* ent;

    logger.debug("Going from CS_PRIMED to CS_ACTIVE for %s", client->name);
    client->state = CS_ACTIVE;

    // set up the entity for the client
    clientNum = client - svs.clients;
    ent = SV_GentityNum(clientNum);
    ent->s.number = clientNum;
    client->gentity = ent;

    client->deltaMessage = -1;
    client->nextSnapshotTime = svs.time; // generate a snapshot immediately
    client->lastUsercmd = *cmd;

    // call the game begin function
    ClientBegin(client - svs.clients);
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload(client_t* cl)
{
    int32 i;

    // EOF
    if (cl->download) {
        FS_FCloseFile(cl->download);
    }
    cl->download = 0;
    *cl->downloadName = 0;

    // Free the temporary buffer space
    for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++) {
        if (cl->downloadBlocks[i]) {
            Z_Free(cl->downloadBlocks[i]);
            cl->downloadBlocks[i] = NULL;
        }
    }
}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
void SV_StopDownload_f(client_t* cl)
{
    if (*cl->downloadName) {
        logger.debug("clientDownload: %d : file \"%s\" aborted", cl - svs.clients, cl->downloadName);
    }

    SV_CloseDownload(cl);
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
void SV_DoneDownload_f(client_t* cl)
{
    logger.debug("clientDownload: %s Done", cl->name);
    // resend the game state to update any clients that entered during the download
    SV_SendClientGameState(cl);
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
void SV_NextDownload_f(client_t* cl)
{
    int32 block = atoi(Cmd_Argv(1));

    if (block == cl->downloadClientBlock) {
        logger.debug("clientDownload: %d : client acknowledge of block %d", cl - svs.clients, block);

        // Find out if we are done.  A zero-length block indicates EOF
        if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0) {
            logger.info("clientDownload: %d : file \"%s\" completed", cl - svs.clients, cl->downloadName);
            SV_CloseDownload(cl);
            return;
        }

        cl->downloadSendTime = svs.time;
        cl->downloadClientBlock++;
        return;
    }
    // We aren't getting an acknowledge for the correct block, drop the client
    // FIXME: this is bad... the client will never parse the disconnect message
    //            because the cgame isn't loaded yet
    SV_DropClient(cl, "broken download");
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(client_t* cl)
{
    // Kill any existing download
    SV_CloseDownload(cl);

    // cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
    // the file itself
    Q_strncpyz(cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName));
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SV_WriteDownloadToClient(client_t* cl, msg_t* msg)
{
    int32 curindex;
    int32 rate;
    int32 blockspersnap;
    int32 idPack;
    char  errorMessage[1024];

    if (!*cl->downloadName) {
        return; // Nothing being downloaded
    }

    if (!cl->download) {
        // We open the file here

        logger.info("clientDownload: %d : begining \"%s\"", cl - svs.clients, cl->downloadName);

        idPack = FS_idPak(cl->downloadName, "baseq3");

        if (!sv_allowDownload->integer || idPack || (cl->downloadSize = FS_SV_FOpenFileRead(cl->downloadName, &cl->download)) <= 0) {
            // cannot auto-download file
            if (idPack) {
                logger.info("clientDownload: %d : \"%s\" cannot download id pk3 files", cl - svs.clients, cl->downloadName);
                Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload id pk3 file \"%s\"", cl->downloadName);
            } else if (!sv_allowDownload->integer) {
                logger.info("clientDownload: %d : \"%s\" download disabled", cl - svs.clients, cl->downloadName);
                if (sv_pure->integer) {
                    Com_sprintf(errorMessage, sizeof(errorMessage),
                                "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                                "You will need to get this file elsewhere before you "
                                "can connect to this pure server.\n",
                                cl->downloadName);
                } else {
                    Com_sprintf(errorMessage, sizeof(errorMessage),
                                "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                                "The server you are connecting to is not a pure server, "
                                "set autodownload to No in your settings and you might be "
                                "able to join the game anyway.\n",
                                cl->downloadName);
                }
            } else {
                // NOTE TTimo this is NOT supposed to happen unless bug in our filesystem scheme?
                //   if the pk3 is referenced, it must have been found somewhere in the filesystem
                logger.info("clientDownload: %d : \"%s\" file not found on server", cl - svs.clients, cl->downloadName);
                Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName);
            }
            MSG_WriteByte(msg, svc_download);
            MSG_WriteShort(msg, 0); // client is expecting block zero
            MSG_WriteLong(msg, -1); // illegal file size
            MSG_WriteString(msg, errorMessage);

            *cl->downloadName = 0;
            return;
        }

        // Init
        cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
        cl->downloadCount = 0;
        cl->downloadEOF = false;
    }

    // Perform any reads that we need to
    while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW && cl->downloadSize != cl->downloadCount) {
        curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

        if (!cl->downloadBlocks[curindex]) {
            cl->downloadBlocks[curindex] = (uint8*) Z_Malloc(MAX_DOWNLOAD_BLKSIZE);
        }

        cl->downloadBlockSize[curindex] = FS_Read(cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download);

        if (cl->downloadBlockSize[curindex] < 0) {
            // EOF right now
            cl->downloadCount = cl->downloadSize;
            break;
        }

        cl->downloadCount += cl->downloadBlockSize[curindex];

        // Load in next block
        cl->downloadCurrentBlock++;
    }

    // Check to see if we have eof condition and add the EOF block
    if (cl->downloadCount == cl->downloadSize && !cl->downloadEOF && cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW) {
        cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
        cl->downloadCurrentBlock++;

        cl->downloadEOF = true; // We have added the EOF block
    }

    // Loop up to window size times based on how many blocks we can fit in the
    // client snapMsec and rate

    // based on the rate, how many bytes can we fit in the snapMsec time of the client
    // normal rate / snapshotMsec calculation
    rate = cl->rate;
    if (sv_maxRate->integer) {
        if (sv_maxRate->integer < 1000) {
            Cvar_Set("sv_MaxRate", "1000");
        }
        if (sv_maxRate->integer < rate) {
            rate = sv_maxRate->integer;
        }
    }

    if (!rate) {
        blockspersnap = 1;
    } else {
        blockspersnap = ((rate * cl->snapshotMsec) / 1000 + MAX_DOWNLOAD_BLKSIZE) / MAX_DOWNLOAD_BLKSIZE;
    }

    if (blockspersnap < 0) {
        blockspersnap = 1;
    }

    while (blockspersnap--) {
        // Write out the next section of the file, if we have already reached our window,
        // automatically start retransmitting

        if (cl->downloadClientBlock == cl->downloadCurrentBlock) {
            return; // Nothing to transmit
        }

        if (cl->downloadXmitBlock == cl->downloadCurrentBlock) {
            // We have transmitted the complete window, should we start resending?

            // FIXME:  This uses a hardcoded one second timeout for lost blocks
            // the timeout should be based on client rate somehow
            if (svs.time - cl->downloadSendTime > 1000) {
                cl->downloadXmitBlock = cl->downloadClientBlock;
            } else {
                return;
            }
        }

        // Send current block
        curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

        MSG_WriteByte(msg, svc_download);
        MSG_WriteShort(msg, cl->downloadXmitBlock);

        // block zero is special, contains file size
        if (cl->downloadXmitBlock == 0) {
            MSG_WriteLong(msg, cl->downloadSize);
        }

        MSG_WriteShort(msg, cl->downloadBlockSize[curindex]);

        // Write the block
        if (cl->downloadBlockSize[curindex]) {
            MSG_WriteData(msg, cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex]);
        }

        logger.debug("clientDownload: %d : writing block %d", cl - svs.clients, cl->downloadXmitBlock);

        // Move on to the next block
        // It will get sent with next snap shot.  The rate will keep us in line.
        cl->downloadXmitBlock++;

        cl->downloadSendTime = svs.time;
    }
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f(client_t* cl)
{
    SV_DropClient(cl, "disconnected");
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void SV_VerifyPaks_f(client_t* cl)
{
    cl->gotCP = true;
    cl->pureAuthentic = 1;
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void SV_ResetPureClient_f(client_t* cl)
{
    cl->pureAuthentic = 0;
    cl->gotCP = false;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged(client_t* cl)
{
    char* val;
    int32 i;

    // name for C code
    Q_strncpyz(cl->name, Info_ValueForKey(cl->userinfo, "name"), sizeof(cl->name));

    // rate command

    // if the client is on the same subnet as the server and we aren't running an
    // internet public server, assume they don't need a rate choke
    if (Sys_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1) {
        cl->rate = 99999; // lans should not rate limit
    } else {
        val = Info_ValueForKey(cl->userinfo, "rate");
        if (strlen(val)) {
            i = atoi(val);
            cl->rate = i;
            if (cl->rate < 1000) {
                cl->rate = 1000;
            } else if (cl->rate > 90000) {
                cl->rate = 90000;
            }
        } else {
            cl->rate = 3000;
        }
    }
    val = Info_ValueForKey(cl->userinfo, "handicap");
    if (strlen(val)) {
        i = atoi(val);
        if (i <= 0 || i > 100 || strlen(val) > 4) {
            Info_SetValueForKey(cl->userinfo, "handicap", "100");
        }
    }

    // snaps command
    val = Info_ValueForKey(cl->userinfo, "snaps");
    if (strlen(val)) {
        i = atoi(val);
        if (i < 1) {
            i = 1;
        } else if (i > 30) {
            i = 30;
        }
        cl->snapshotMsec = 1000 / i;
    } else {
        cl->snapshotMsec = 50;
    }

    // TTimo
    // maintain the IP information
    // this is set in SV_DirectConnect (directly on the server, not transmitted), may be lost when client updates it's userinfo
    // the banning code relies on this being consistently present
    val = Info_ValueForKey(cl->userinfo, "ip");
    if (!val[0]) {
        if (!NET_IsLocalAddress(cl->netchan.remoteAddress)) {
            Info_SetValueForKey(cl->userinfo, "ip", NET_AdrToString(cl->netchan.remoteAddress));
        } else {
            // force the "ip" info key to "localhost" for local clients
            Info_SetValueForKey(cl->userinfo, "ip", "localhost");
        }
    }
}


/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f(client_t* cl)
{
    Q_strncpyz(cl->userinfo, Cmd_Argv(1), sizeof(cl->userinfo));

    SV_UserinfoChanged(cl);
    // call prog code to allow overrides
    ClientUserinfoChanged(cl - svs.clients);
}

typedef struct
{
    const char* name;
    void        (*func)(client_t* cl);
} ucmd_t;

static ucmd_t ucmds[] = {
    {"userinfo", SV_UpdateUserinfo_f},
    {"disconnect", SV_Disconnect_f},
    {"cp", SV_VerifyPaks_f},
    {"vdr", SV_ResetPureClient_f},
    {"download", SV_BeginDownload_f},
    {"nextdl", SV_NextDownload_f},
    {"stopdl", SV_StopDownload_f},
    {"donedl", SV_DoneDownload_f},

    {NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK)
{
    ucmd_t* u;
    bool    bProcessed = false;

    Cmd_TokenizeString(s);

    // see if it is a server level command
    for (u = ucmds; u->name; u++) {
        if (!strcmp(Cmd_Argv(0), u->name)) {
            u->func(cl);
            bProcessed = true;
            break;
        }
    }

    if (clientOK) {
        // pass unknown strings to the game
        if (!u->name && sv.state == SS_GAME) {
            ClientCommand(cl - svs.clients);
        }
    } else if (!bProcessed) {
        logger.debug("client text ignored for %s: %s", cl->name, Cmd_Argv(0));
    }
}

/*
===============
SV_ClientCommand
===============
*/
static bool SV_ClientCommand(client_t* cl, msg_t* msg)
{
    int32       seq;
    const char* s;
    bool        clientOk = true;

    seq = MSG_ReadLong(msg);
    s = MSG_ReadString(msg);

    // see if we have already executed it
    if (cl->lastClientCommand >= seq) {
        return true;
    }

    logger.debug("clientCommand: %s : %i : %s", cl->name, seq, s);

    // drop the connection if we have somehow lost commands
    if (seq > cl->lastClientCommand + 1) {
        logger.info("Client %s lost %i clientCommands", cl->name, seq - cl->lastClientCommand + 1);
        SV_DropClient(cl, "Lost reliable commands");
        return false;
    }

    // malicious users may try using too many string commands
    // to lag other players.  If we decide that we want to stall
    // the command, we will stop processing the rest of the packet,
    // including the usercmd.  This causes flooders to lag themselves
    // but not other people
    // We don't do this when the client hasn't been active yet since its
    // normal to spam a lot of commands when downloading
    if (!com_cl_running->integer && cl->state >= CS_ACTIVE && sv_floodProtect->integer && svs.time < cl->nextReliableTime) {
        // ignore any other text messages from this client but let them keep playing
        // TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
        clientOk = false;
    }

    // don't allow another command for one second
    cl->nextReliableTime = svs.time + 1000;

    SV_ExecuteClientCommand(cl, s, clientOk);

    cl->lastClientCommand = seq;
    Com_sprintf(cl->lastClientCommandString, sizeof(cl->lastClientCommandString), "%s", s);

    return true; // continue procesing
}


//==================================================================================


/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink(client_t* cl, usercmd_t* cmd)
{
    cl->lastUsercmd = *cmd;

    if (cl->state != CS_ACTIVE) {
        return; // may have been kicked during the last usercmd
    }

    ClientThink(cl - svs.clients);
}

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove(client_t* cl, msg_t* msg, bool delta)
{
    int32      i, key;
    int32      cmdCount;
    usercmd_t  nullcmd;
    usercmd_t  cmds[MAX_PACKET_USERCMDS];
    usercmd_t *cmd, *oldcmd;

    if (delta) {
        cl->deltaMessage = cl->messageAcknowledge;
    } else {
        cl->deltaMessage = -1;
    }

    cmdCount = MSG_ReadByte(msg);

    if (cmdCount < 1) {
        logger.info("cmdCount < 1");
        return;
    }

    if (cmdCount > MAX_PACKET_USERCMDS) {
        logger.info("cmdCount > MAX_PACKET_USERCMDS");
        return;
    }

    // use the checksum feed in the key
    key = sv.checksumFeed;
    // also use the message acknowledge
    key ^= cl->messageAcknowledge;
    // also use the last acknowledged server command in the key
    key ^= Com_HashKey(cl->reliableCommands[cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS - 1)], 32);

    Com_Memset(&nullcmd, 0, sizeof(nullcmd));
    oldcmd = &nullcmd;
    for (i = 0; i < cmdCount; i++) {
        cmd = &cmds[i];
        MSG_ReadDeltaUsercmdKey(msg, key, oldcmd, cmd);
        oldcmd = cmd;
    }

    // save time for ping calculation
    cl->frames[cl->messageAcknowledge & PACKET_MASK].messageAcked = svs.time;

    // TTimo
    // catch the no-cp-yet situation before SV_ClientEnterWorld
    // if CS_ACTIVE, then it's time to trigger a new gamestate emission
    // if not, then we are getting remaining parasite usermove commands, which we should ignore
    if (sv_pure->integer != 0 && cl->pureAuthentic == 0 && !cl->gotCP) {
        if (cl->state == CS_ACTIVE) {
            // we didn't get a cp yet, don't assume anything and just send the gamestate all over again
            logger.debug("%s: didn't get cp command, resending gamestate", cl->name, cl->state);
            SV_SendClientGameState(cl);
        }
        return;
    }

    // if this is the first usercmd we have received
    // this gamestate, put the client into the world
    if (cl->state == CS_PRIMED) {
        SV_ClientEnterWorld(cl, &cmds[0]);
        // the moves can be processed normaly
    }

    // a bad cp command was sent, drop the client
    if (sv_pure->integer != 0 && cl->pureAuthentic == 0) {
        SV_DropClient(cl, "Cannot validate pure client!");
        return;
    }

    if (cl->state != CS_ACTIVE) {
        cl->deltaMessage = -1;
        return;
    }

    // usually, the first couple commands will be duplicates
    // of ones we have previously received, but the servertimes
    // in the commands will cause them to be immediately discarded
    for (i = 0; i < cmdCount; i++) {
        // if this is a cmd from before a map_restart ignore it
        if (cmds[i].serverTime > cmds[cmdCount - 1].serverTime) {
            continue;
        }
        // extremely lagged or cmd from before a map_restart
        // if ( cmds[i].serverTime > svs.time + 3000 ) {
        //    continue;
        //}
        // don't execute if this is an old cmd which is already executed
        // these old cmds are included when cl_packetdup > 0
        if (cmds[i].serverTime <= cl->lastUsercmd.serverTime) {
            continue;
        }
        SV_ClientThink(cl, &cmds[i]);
    }
}


/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage(client_t* cl, msg_t* msg)
{
    int32 c;
    int32 serverId;

    MSG_Bitstream(msg);

    serverId = MSG_ReadLong(msg);
    cl->messageAcknowledge = MSG_ReadLong(msg);

    if (cl->messageAcknowledge < 0) {
        // usually only hackers create messages like this
        // it is more annoying for them to let them hanging
        SV_DropClient(cl, "DEBUG: illegible client message");
        return;
    }

    cl->reliableAcknowledge = MSG_ReadLong(msg);

    // NOTE: when the client message is fux0red the acknowledgement numbers
    // can be out of range, this could cause the server to send thousands of server
    // commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
    if (cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS) {
        // usually only hackers create messages like this
        // it is more annoying for them to let them hanging
        SV_DropClient(cl, "DEBUG: illegible client message");
        cl->reliableAcknowledge = cl->reliableSequence;
        return;
    }
    // if this is a usercmd from a previous gamestate,
    // ignore it or retransmit the current gamestate
    //
    // if the client was downloading, let it stay at whatever serverId and
    // gamestate it was at.  This allows it to keep downloading even when
    // the gamestate changes.  After the download is finished, we'll
    // notice and send it a new game state
    //
    // don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
    // but we still need to read the next message to move to next download or send gamestate
    // I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
    if (serverId != sv.serverId && !*cl->downloadName && !strstr(cl->lastClientCommandString, "nextdl")) {
        if (serverId >= sv.restartedServerId && serverId < sv.serverId) { // TTimo - use a comparison here to catch multiple map_restart
            // they just haven't caught the map_restart yet
            logger.debug("%s : ignoring pre map_restart / outdated client message", cl->name);
            return;
        }
        // if we can tell that the client has dropped the last
        // gamestate we sent them, resend it
        if (cl->messageAcknowledge > cl->gamestateMessageNum) {
            logger.debug("%s : dropped gamestate, resending", cl->name);
            SV_SendClientGameState(cl);
        }
        return;
    }

    // read optional clientCommand strings
    do {
        c = MSG_ReadByte(msg);
        if (c == clc_EOF) {
            break;
        }
        if (c != clc_clientCommand) {
            break;
        }
        if (!SV_ClientCommand(cl, msg)) {
            return; // we couldn't execute it because of the flood protection
        }
        if (cl->state == CS_ZOMBIE) {
            return; // disconnect command
        }
    } while (1);

    // read the usercmd_t
    if (c == clc_move) {
        SV_UserMove(cl, msg, true);
    } else if (c == clc_moveNoDelta) {
        SV_UserMove(cl, msg, false);
    } else if (c != clc_EOF) {
        logger.warning("bad command uint8 for client %i", cl - svs.clients);
    }
}
