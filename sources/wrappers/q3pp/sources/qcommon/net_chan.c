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
#include "qcommon.h"

static tavros::core::logger logger("net_chan");

/*

packet header
-------------
4    outgoing sequence.  high bit will be set if this is a fragmented message
[2    qport (only for client to server)]
[2    fragment start uint8]
[2    fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.

*/


#define MAX_PACKETLEN 1400 // max size of a network packet

#define FRAGMENT_SIZE (MAX_PACKETLEN - 100)
#define PACKET_HEADER 10 // two ints and a int16

#define FRAGMENT_BIT  (1 << 31)

cvar_t* showpackets;
cvar_t* showdrop;
cvar_t* qport;

static const char* netsrcString[2] = {
    "client",
    "server"
};

/*
===============
Netchan_Init

===============
*/
void Netchan_Init(int32 port)
{
    port &= 0xffff;
    showpackets = Cvar_Get("showpackets", "0", CVAR_TEMP);
    showdrop = Cvar_Get("showdrop", "0", CVAR_TEMP);
    qport = Cvar_Get("net_qport", va("%i", port), CVAR_INIT);
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int32 qport)
{
    Com_Memset(chan, 0, sizeof(*chan));

    chan->sock = sock;
    chan->remoteAddress = adr;
    chan->qport = qport;
    chan->incomingSequence = 0;
    chan->outgoingSequence = 1;
}

/*
=================
Netchan_TransmitNextFragment

Send one fragment of the current message
=================
*/
void Netchan_TransmitNextFragment(netchan_t* chan)
{
    msg_t send;
    uint8 send_buf[MAX_PACKETLEN];
    int32 fragmentLength;

    // write the packet header
    MSG_InitOOB(&send, send_buf, sizeof(send_buf)); // <-- only do the oob here

    MSG_WriteLong(&send, chan->outgoingSequence | FRAGMENT_BIT);

    // send the qport if we are a client
    if (chan->sock == NS_CLIENT) {
        MSG_WriteShort(&send, qport->integer);
    }

    // copy the reliable message to the packet first
    fragmentLength = FRAGMENT_SIZE;
    if (chan->unsentFragmentStart + fragmentLength > chan->unsentLength) {
        fragmentLength = chan->unsentLength - chan->unsentFragmentStart;
    }

    MSG_WriteShort(&send, chan->unsentFragmentStart);
    MSG_WriteShort(&send, fragmentLength);
    MSG_WriteData(&send, chan->unsentBuffer + chan->unsentFragmentStart, fragmentLength);

    // send the datagram
    NET_SendPacket(chan->sock, send.cursize, send.data, chan->remoteAddress);

    if (showpackets->integer) {
        logger.info("%s send %4i : s=%i fragment=%i,%i", netsrcString[chan->sock], send.cursize, chan->outgoingSequence, chan->unsentFragmentStart, fragmentLength);
    }

    chan->unsentFragmentStart += fragmentLength;

    // this exit condition is a little tricky, because a packet
    // that is exactly the fragment length still needs to send
    // a second packet of zero length so that the other side
    // can tell there aren't more to follow
    if (chan->unsentFragmentStart == chan->unsentLength && fragmentLength != FRAGMENT_SIZE) {
        chan->outgoingSequence++;
        chan->unsentFragments = false;
    }
}


/*
===============
Netchan_Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void Netchan_Transmit(netchan_t* chan, int32 length, const uint8* data)
{
    msg_t send;
    uint8 send_buf[MAX_PACKETLEN];

    if (length > MAX_MSGLEN) {
        Com_Error(ERR_DROP, "Netchan_Transmit: length = %i", length);
    }
    chan->unsentFragmentStart = 0;

    // fragment large reliable messages
    if (length >= FRAGMENT_SIZE) {
        chan->unsentFragments = true;
        chan->unsentLength = length;
        Com_Memcpy(chan->unsentBuffer, data, length);

        // only send the first fragment now
        Netchan_TransmitNextFragment(chan);

        return;
    }

    // write the packet header
    MSG_InitOOB(&send, send_buf, sizeof(send_buf));

    MSG_WriteLong(&send, chan->outgoingSequence);
    chan->outgoingSequence++;

    // send the qport if we are a client
    if (chan->sock == NS_CLIENT) {
        MSG_WriteShort(&send, qport->integer);
    }

    MSG_WriteData(&send, data, length);

    // send the datagram
    NET_SendPacket(chan->sock, send.cursize, send.data, chan->remoteAddress);

    if (showpackets->integer) {
        logger.info("%s send %4i : s=%i ack=%i", netsrcString[chan->sock], send.cursize, chan->outgoingSequence - 1, chan->incomingSequence);
    }
}

/*
=================
Netchan_Process

Returns false if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
bool Netchan_Process(netchan_t* chan, msg_t* msg)
{
    int32 sequence;
    int32 qport;
    int32 fragmentStart, fragmentLength;
    bool  fragmented;

    // XOR unscramble all data in the packet after the header
    //    Netchan_UnScramblePacket( msg );

    // get sequence numbers
    MSG_BeginReadingOOB(msg);
    sequence = MSG_ReadLong(msg);

    // check for fragment information
    if (sequence & FRAGMENT_BIT) {
        sequence &= ~FRAGMENT_BIT;
        fragmented = true;
    } else {
        fragmented = false;
    }

    // read the qport if we are a server
    if (chan->sock == NS_SERVER) {
        qport = MSG_ReadShort(msg);
    }

    // read the fragment information
    if (fragmented) {
        fragmentStart = MSG_ReadShort(msg);
        fragmentLength = MSG_ReadShort(msg);
    } else {
        fragmentStart = 0; // stop warning message
        fragmentLength = 0;
    }

    if (showpackets->integer) {
        if (fragmented) {
            logger.info("%s recv %4i : s=%i fragment=%i,%i", netsrcString[chan->sock], msg->cursize, sequence, fragmentStart, fragmentLength);
        } else {
            logger.info("%s recv %4i : s=%i", netsrcString[chan->sock], msg->cursize, sequence);
        }
    }

    //
    // discard out of order or duplicated packets
    //
    if (sequence <= chan->incomingSequence) {
        if (showdrop->integer || showpackets->integer) {
            logger.info("%s:Out of order packet %i at %i", NET_AdrToString(chan->remoteAddress), sequence, chan->incomingSequence);
        }
        return false;
    }

    //
    // dropped packets don't keep the message from being used
    //
    chan->dropped = sequence - (chan->incomingSequence + 1);
    if (chan->dropped > 0) {
        if (showdrop->integer || showpackets->integer) {
            logger.info("%s:Dropped %i packets at %i", NET_AdrToString(chan->remoteAddress), chan->dropped, sequence);
        }
    }


    //
    // if this is the final framgent of a reliable message,
    // bump incoming_reliable_sequence
    //
    if (fragmented) {
        // make sure we add the fragments in correct order
        // either a packet was dropped, or we received this one too soon
        // we don't reconstruct the fragments. we will wait till this fragment gets to us again
        // (NOTE: we could probably try to rebuild by out of order chunks if needed)
        if (sequence != chan->fragmentSequence) {
            chan->fragmentSequence = sequence;
            chan->fragmentLength = 0;
        }

        // if we missed a fragment, dump the message
        if (fragmentStart != chan->fragmentLength) {
            if (showdrop->integer || showpackets->integer) {
                logger.info("%s:Dropped a message fragment", NET_AdrToString(chan->remoteAddress), sequence);
            }
            // we can still keep the part that we have so far,
            // so we don't need to clear chan->fragmentLength
            return false;
        }

        // copy the fragment to the fragment buffer
        if (fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize || chan->fragmentLength + fragmentLength > sizeof(chan->fragmentBuffer)) {
            if (showdrop->integer || showpackets->integer) {
                logger.info("%s:illegal fragment length", NET_AdrToString(chan->remoteAddress));
            }
            return false;
        }

        Com_Memcpy(chan->fragmentBuffer + chan->fragmentLength, msg->data + msg->readcount, fragmentLength);

        chan->fragmentLength += fragmentLength;

        // if this wasn't the last fragment, don't process anything
        if (fragmentLength == FRAGMENT_SIZE) {
            return false;
        }

        if (chan->fragmentLength > msg->maxsize) {
            logger.info("%s:fragmentLength %i > msg->maxsize", NET_AdrToString(chan->remoteAddress), chan->fragmentLength);
            return false;
        }

        // copy the full message over the partial fragment

        // make sure the sequence number is still there
        *(int32*) msg->data = (sequence);

        Com_Memcpy(msg->data + 4, chan->fragmentBuffer, chan->fragmentLength);
        msg->cursize = chan->fragmentLength + 4;
        chan->fragmentLength = 0;
        msg->readcount = 4; // past the sequence number
        msg->bit = 32;      // past the sequence number

        // clients were not acking fragmented messages
        chan->incomingSequence = sequence;

        return true;
    }

    //
    // the message can now be read from the current message pointer
    //
    chan->incomingSequence = sequence;

    return true;
}


//==============================================================================

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr(netadr_t a, netadr_t b)
{
    if (a.type != b.type) {
        return false;
    }

    if (a.type == NA_LOOPBACK) {
        return true;
    }

    if (a.type == NA_IP) {
        if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3]) {
            return true;
        }
        return false;
    }

    if (a.type == NA_IPX) {
        if ((memcmp(a.ipx, b.ipx, 10) == 0)) {
            return true;
        }
        return false;
    }


    logger.info("NET_CompareBaseAdr: bad address type");
    return false;
}

const char* NET_AdrToString(netadr_t a)
{
    static char s[64];

    if (a.type == NA_LOOPBACK) {
        Com_sprintf(s, sizeof(s), "loopback");
    } else if (a.type == NA_BOT) {
        Com_sprintf(s, sizeof(s), "bot");
    } else if (a.type == NA_IP) {
        Com_sprintf(s, sizeof(s), "%i.%i.%i.%i:%hu", a.ip[0], a.ip[1], a.ip[2], a.ip[3], BigShort(a.port));
    } else {
        Com_sprintf(s, sizeof(s), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%hu", a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9], BigShort(a.port));
    }

    return s;
}


bool NET_CompareAdr(netadr_t a, netadr_t b)
{
    if (a.type != b.type) {
        return false;
    }

    if (a.type == NA_LOOPBACK) {
        return true;
    }

    if (a.type == NA_IP) {
        if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port) {
            return true;
        }
        return false;
    }

    if (a.type == NA_IPX) {
        if ((memcmp(a.ipx, b.ipx, 10) == 0) && a.port == b.port) {
            return true;
        }
        return false;
    }

    logger.info("NET_CompareAdr: bad address type");
    return false;
}


bool NET_IsLocalAddress(netadr_t adr)
{
    return adr.type == NA_LOOPBACK;
}


/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define MAX_LOOPBACK 16

typedef struct
{
    uint8 data[MAX_PACKETLEN];
    int32 datalen;
} loopmsg_t;

typedef struct
{
    loopmsg_t msgs[MAX_LOOPBACK];
    int32     get, send;
} loopback_t;

loopback_t loopbacks[2];


bool NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, msg_t* net_message)
{
    int32       i;
    loopback_t* loop;

    loop = &loopbacks[sock];

    if (loop->send - loop->get > MAX_LOOPBACK) {
        loop->get = loop->send - MAX_LOOPBACK;
    }

    if (loop->get >= loop->send) {
        return false;
    }

    i = loop->get & (MAX_LOOPBACK - 1);
    loop->get++;

    Com_Memcpy(net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
    net_message->cursize = loop->msgs[i].datalen;
    Com_Memset(net_from, 0, sizeof(*net_from));
    net_from->type = NA_LOOPBACK;
    return true;
}


void NET_SendLoopPacket(netsrc_t sock, int32 length, const void* data, netadr_t to)
{
    int32       i;
    loopback_t* loop;

    loop = &loopbacks[sock ^ 1];

    i = loop->send & (MAX_LOOPBACK - 1);
    loop->send++;

    Com_Memcpy(loop->msgs[i].data, data, length);
    loop->msgs[i].datalen = length;
}

//=============================================================================


void NET_SendPacket(netsrc_t sock, int32 length, const void* data, netadr_t to)
{
    // sequenced packets are shown in netchan, so just show oob
    if (showpackets->integer && *(int32*) data == -1) {
        logger.info("send packet %4i", length);
    }

    if (to.type == NA_LOOPBACK) {
        NET_SendLoopPacket(sock, length, data, to);
        return;
    }
    if (to.type == NA_BOT) {
        return;
    }
    if (to.type == NA_BAD) {
        return;
    }

    Sys_SendPacket(length, data, to);
}

/*
===============
NET_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void QDECL NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char* format, ...)
{
    va_list argptr;
    char    string[MAX_MSGLEN];


    // set the header
    string[0] = -1;
    string[1] = -1;
    string[2] = -1;
    string[3] = -1;

    va_start(argptr, format);
    vsprintf(string + 4, format, argptr);
    va_end(argptr);

    // send the datagram
    NET_SendPacket(sock, strlen(string), string, adr);
}

/*
===============
NET_OutOfBandPrint

Sends a data message in an out-of-band datagram (only used for "connect")
================
*/
void QDECL NET_OutOfBandData(netsrc_t sock, netadr_t adr, uint8* format, int32 len)
{
    uint8 string[MAX_MSGLEN * 2];
    int32 i;
    msg_t mbuf;

    // set the header
    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;

    for (i = 0; i < len; i++) {
        string[i + 4] = format[i];
    }

    mbuf.data = string;
    mbuf.cursize = len + 4;
    Huff_Compress(&mbuf, 12);
    // send the datagram
    NET_SendPacket(sock, mbuf.cursize, mbuf.data, adr);
}

/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
=============
*/
bool NET_StringToAdr(const char* s, netadr_t* a)
{
    bool  r;
    char  base[MAX_STRING_CHARS];
    char* port;

    if (!strcmp(s, "localhost")) {
        Com_Memset(a, 0, sizeof(*a));
        a->type = NA_LOOPBACK;
        return true;
    }

    // look for a port number
    Q_strncpyz(base, s, sizeof(base));
    port = strstr(base, ":");
    if (port) {
        *port = 0;
        port++;
    }

    r = Sys_StringToAdr(base, a);

    if (!r) {
        a->type = NA_BAD;
        return false;
    }

    // inet_addr returns this if out of range
    if (a->ip[0] == 255 && a->ip[1] == 255 && a->ip[2] == 255 && a->ip[3] == 255) {
        a->type = NA_BAD;
        return false;
    }

    if (port) {
        a->port = BigShort((int16) atoi(port));
    } else {
        a->port = BigShort(PORT_SERVER);
    }

    return true;
}

