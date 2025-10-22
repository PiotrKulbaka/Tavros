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
// net_wins.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

static tavros::core::logger logger("win_net");

static WSADATA winsockdata;
static bool    winsockInitialized = false;
static bool    usingSocks = false;
static bool    networkingEnabled = false;

static cvar_t* net_noudp;
static cvar_t* net_noipx;

static cvar_t*         net_socksEnabled;
static cvar_t*         net_socksServer;
static cvar_t*         net_socksPort;
static cvar_t*         net_socksUsername;
static cvar_t*         net_socksPassword;
static struct sockaddr socksRelayAddr;

static SOCKET ip_socket;
static SOCKET socks_socket;
static SOCKET ipx_socket;

#define MAX_IPS 16
static int32 numIP;
static uint8 localIP[MAX_IPS][4];

//=============================================================================


/*
====================
NET_ErrorString
====================
*/
const char* NET_ErrorString()
{
    int32 code;

    code = WSAGetLastError();
    switch (code) {
    case WSAEINTR:
        return "WSAEINTR";
    case WSAEBADF:
        return "WSAEBADF";
    case WSAEACCES:
        return "WSAEACCES";
    case WSAEDISCON:
        return "WSAEDISCON";
    case WSAEFAULT:
        return "WSAEFAULT";
    case WSAEINVAL:
        return "WSAEINVAL";
    case WSAEMFILE:
        return "WSAEMFILE";
    case WSAEWOULDBLOCK:
        return "WSAEWOULDBLOCK";
    case WSAEINPROGRESS:
        return "WSAEINPROGRESS";
    case WSAEALREADY:
        return "WSAEALREADY";
    case WSAENOTSOCK:
        return "WSAENOTSOCK";
    case WSAEDESTADDRREQ:
        return "WSAEDESTADDRREQ";
    case WSAEMSGSIZE:
        return "WSAEMSGSIZE";
    case WSAEPROTOTYPE:
        return "WSAEPROTOTYPE";
    case WSAENOPROTOOPT:
        return "WSAENOPROTOOPT";
    case WSAEPROTONOSUPPORT:
        return "WSAEPROTONOSUPPORT";
    case WSAESOCKTNOSUPPORT:
        return "WSAESOCKTNOSUPPORT";
    case WSAEOPNOTSUPP:
        return "WSAEOPNOTSUPP";
    case WSAEPFNOSUPPORT:
        return "WSAEPFNOSUPPORT";
    case WSAEAFNOSUPPORT:
        return "WSAEAFNOSUPPORT";
    case WSAEADDRINUSE:
        return "WSAEADDRINUSE";
    case WSAEADDRNOTAVAIL:
        return "WSAEADDRNOTAVAIL";
    case WSAENETDOWN:
        return "WSAENETDOWN";
    case WSAENETUNREACH:
        return "WSAENETUNREACH";
    case WSAENETRESET:
        return "WSAENETRESET";
    case WSAECONNABORTED:
        return "WSWSAECONNABORTEDAEINTR";
    case WSAECONNRESET:
        return "WSAECONNRESET";
    case WSAENOBUFS:
        return "WSAENOBUFS";
    case WSAEISCONN:
        return "WSAEISCONN";
    case WSAENOTCONN:
        return "WSAENOTCONN";
    case WSAESHUTDOWN:
        return "WSAESHUTDOWN";
    case WSAETOOMANYREFS:
        return "WSAETOOMANYREFS";
    case WSAETIMEDOUT:
        return "WSAETIMEDOUT";
    case WSAECONNREFUSED:
        return "WSAECONNREFUSED";
    case WSAELOOP:
        return "WSAELOOP";
    case WSAENAMETOOLONG:
        return "WSAENAMETOOLONG";
    case WSAEHOSTDOWN:
        return "WSAEHOSTDOWN";
    case WSASYSNOTREADY:
        return "WSASYSNOTREADY";
    case WSAVERNOTSUPPORTED:
        return "WSAVERNOTSUPPORTED";
    case WSANOTINITIALISED:
        return "WSANOTINITIALISED";
    case WSAHOST_NOT_FOUND:
        return "WSAHOST_NOT_FOUND";
    case WSATRY_AGAIN:
        return "WSATRY_AGAIN";
    case WSANO_RECOVERY:
        return "WSANO_RECOVERY";
    case WSANO_DATA:
        return "WSANO_DATA";
    default:
        return "NO ERROR";
    }
}

void NetadrToSockadr(netadr_t* a, struct sockaddr* s)
{
    memset(s, 0, sizeof(*s));

    if (a->type == NA_BROADCAST) {
        ((struct sockaddr_in*) s)->sin_family = AF_INET;
        ((struct sockaddr_in*) s)->sin_port = a->port;
        ((struct sockaddr_in*) s)->sin_addr.s_addr = INADDR_BROADCAST;
    } else if (a->type == NA_IP) {
        ((struct sockaddr_in*) s)->sin_family = AF_INET;
        ((struct sockaddr_in*) s)->sin_addr.s_addr = *(int32*) &a->ip;
        ((struct sockaddr_in*) s)->sin_port = a->port;
    } else if (a->type == NA_IPX) {
        ((struct sockaddr_ipx*) s)->sa_family = AF_IPX;
        memcpy(((struct sockaddr_ipx*) s)->sa_netnum, &a->ipx[0], 4);
        memcpy(((struct sockaddr_ipx*) s)->sa_nodenum, &a->ipx[4], 6);
        ((struct sockaddr_ipx*) s)->sa_socket = a->port;
    } else if (a->type == NA_BROADCAST_IPX) {
        ((struct sockaddr_ipx*) s)->sa_family = AF_IPX;
        memset(((struct sockaddr_ipx*) s)->sa_netnum, 0, 4);
        memset(((struct sockaddr_ipx*) s)->sa_nodenum, 0xff, 6);
        ((struct sockaddr_ipx*) s)->sa_socket = a->port;
    }
}


void SockadrToNetadr(struct sockaddr* s, netadr_t* a)
{
    if (s->sa_family == AF_INET) {
        a->type = NA_IP;
        *(int32*) &a->ip = ((struct sockaddr_in*) s)->sin_addr.s_addr;
        a->port = ((struct sockaddr_in*) s)->sin_port;
    } else if (s->sa_family == AF_IPX) {
        a->type = NA_IPX;
        memcpy(&a->ipx[0], ((struct sockaddr_ipx*) s)->sa_netnum, 4);
        memcpy(&a->ipx[4], ((struct sockaddr_ipx*) s)->sa_nodenum, 6);
        a->port = ((struct sockaddr_ipx*) s)->sa_socket;
    }
}


/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
12121212.121212121212
=============
*/
#define DO(src, dest)         \
    copy[0] = s[src];         \
    copy[1] = s[src + 1];     \
    sscanf(copy, "%x", &val); \
    ((struct sockaddr_ipx*) sadr)->dest = val

bool Sys_StringToSockaddr(const char* s, struct sockaddr* sadr)
{
    struct hostent* h;
    int32           val;
    char            copy[MAX_STRING_CHARS];

    memset(sadr, 0, sizeof(*sadr));

    // check for an IPX address
    if ((strlen(s) == 21) && (s[8] == '.')) {
        ((struct sockaddr_ipx*) sadr)->sa_family = AF_IPX;
        ((struct sockaddr_ipx*) sadr)->sa_socket = 0;
        copy[2] = 0;
        DO(0, sa_netnum[0]);
        DO(2, sa_netnum[1]);
        DO(4, sa_netnum[2]);
        DO(6, sa_netnum[3]);
        DO(9, sa_nodenum[0]);
        DO(11, sa_nodenum[1]);
        DO(13, sa_nodenum[2]);
        DO(15, sa_nodenum[3]);
        DO(17, sa_nodenum[4]);
        DO(19, sa_nodenum[5]);
    } else {
        ((struct sockaddr_in*) sadr)->sin_family = AF_INET;
        ((struct sockaddr_in*) sadr)->sin_port = 0;

        if (s[0] >= '0' && s[0] <= '9') {
            *(int32*) &((struct sockaddr_in*) sadr)->sin_addr = inet_addr(s);
        } else {
            if ((h = gethostbyname(s)) == 0) {
                return 0;
            }
            *(int32*) &((struct sockaddr_in*) sadr)->sin_addr = *(int32*) h->h_addr_list[0];
        }
    }

    return true;
}

#undef DO

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
bool Sys_StringToAdr(const char* s, netadr_t* a)
{
    struct sockaddr sadr;

    if (!Sys_StringToSockaddr(s, &sadr)) {
        return false;
    }

    SockadrToNetadr(&sadr, a);
    return true;
}

//=============================================================================

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
int32 recvfromCount;

bool Sys_GetPacket(netadr_t* net_from, msg_t* net_message)
{
    int32           ret;
    struct sockaddr from;
    int32           fromlen;
    int32           net_socket;
    int32           protocol;
    int32           err;

    for (protocol = 0; protocol < 2; protocol++) {
        if (protocol == 0) {
            net_socket = ip_socket;
        } else {
            net_socket = ipx_socket;
        }

        if (!net_socket) {
            continue;
        }

        fromlen = sizeof(from);
        recvfromCount++; // performance check
        ret = recvfrom(net_socket, (char*) net_message->data, net_message->maxsize, 0, (struct sockaddr*) &from, &fromlen);
        if (ret == SOCKET_ERROR) {
            err = WSAGetLastError();

            if (err == WSAEWOULDBLOCK || err == WSAECONNRESET) {
                continue;
            }
            logger.info("NET_GetPacket: {}", NET_ErrorString());
            continue;
        }

        if (net_socket == ip_socket) {
            memset(((struct sockaddr_in*) &from)->sin_zero, 0, 8);
        }

        if (usingSocks && net_socket == ip_socket && memcmp(&from, &socksRelayAddr, fromlen) == 0) {
            if (ret < 10 || net_message->data[0] != 0 || net_message->data[1] != 0 || net_message->data[2] != 0 || net_message->data[3] != 1) {
                continue;
            }
            net_from->type = NA_IP;
            net_from->ip[0] = net_message->data[4];
            net_from->ip[1] = net_message->data[5];
            net_from->ip[2] = net_message->data[6];
            net_from->ip[3] = net_message->data[7];
            net_from->port = *(int16*) &net_message->data[8];
            net_message->readcount = 10;
        } else {
            SockadrToNetadr(&from, net_from);
            net_message->readcount = 0;
        }

        if (ret == net_message->maxsize) {
            logger.info("Oversize packet from {}", NET_AdrToString(*net_from));
            continue;
        }

        net_message->cursize = ret;
        return true;
    }

    return false;
}

//=============================================================================

static char socksBuf[4096];

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket(int32 length, const void* data, netadr_t to)
{
    int32           ret;
    struct sockaddr addr;
    SOCKET          net_socket;

    if (to.type == NA_BROADCAST) {
        net_socket = ip_socket;
    } else if (to.type == NA_IP) {
        net_socket = ip_socket;
    } else if (to.type == NA_IPX) {
        net_socket = ipx_socket;
    } else if (to.type == NA_BROADCAST_IPX) {
        net_socket = ipx_socket;
    } else {
        Com_Error(ERR_FATAL, "Sys_SendPacket: bad address type");
        return;
    }

    if (!net_socket) {
        return;
    }

    NetadrToSockadr(&to, &addr);

    if (usingSocks && to.type == NA_IP) {
        socksBuf[0] = 0; // reserved
        socksBuf[1] = 0;
        socksBuf[2] = 0; // fragment (not fragmented)
        socksBuf[3] = 1; // address type: IPV4
        *(int32*) &socksBuf[4] = ((struct sockaddr_in*) &addr)->sin_addr.s_addr;
        *(int16*) &socksBuf[8] = ((struct sockaddr_in*) &addr)->sin_port;
        memcpy(&socksBuf[10], data, length);
        ret = sendto(net_socket, socksBuf, length + 10, 0, &socksRelayAddr, sizeof(socksRelayAddr));
    } else {
        ret = sendto(net_socket, (const char*) data, length, 0, &addr, sizeof(addr));
    }
    if (ret == SOCKET_ERROR) {
        int32 err = WSAGetLastError();

        // wouldblock is silent
        if (err == WSAEWOULDBLOCK) {
            return;
        }

        // some PPP links do not allow broadcasts and return an error
        if ((err == WSAEADDRNOTAVAIL) && ((to.type == NA_BROADCAST) || (to.type == NA_BROADCAST_IPX))) {
            return;
        }

        logger.info("NET_SendPacket: {}", NET_ErrorString());
    }
}


//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
bool Sys_IsLANAddress(netadr_t adr)
{
    int32 i;

    if (adr.type == NA_LOOPBACK) {
        return true;
    }

    if (adr.type == NA_IPX) {
        return true;
    }

    if (adr.type != NA_IP) {
        return false;
    }

    // choose which comparison to use based on the class of the address being tested
    // any local adresses of a different class than the address being tested will fail based on the first uint8

    if (adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1) {
        return true;
    }

    // Class A
    if ((adr.ip[0] & 0x80) == 0x00) {
        for (i = 0; i < numIP; i++) {
            if (adr.ip[0] == localIP[i][0]) {
                return true;
            }
        }
        // the RFC1918 class a block will pass the above test
        return false;
    }

    // Class B
    if ((adr.ip[0] & 0xc0) == 0x80) {
        for (i = 0; i < numIP; i++) {
            if (adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1]) {
                return true;
            }
            // also check against the RFC1918 class b blocks
            if (adr.ip[0] == 172 && localIP[i][0] == 172 && (adr.ip[1] & 0xf0) == 16 && (localIP[i][1] & 0xf0) == 16) {
                return true;
            }
        }
        return false;
    }

    // Class C
    for (i = 0; i < numIP; i++) {
        if (adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2]) {
            return true;
        }
        // also check against the RFC1918 class c blocks
        if (adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168) {
            return true;
        }
    }
    return false;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP()
{
    int32 i;

    for (i = 0; i < numIP; i++) {
        logger.info("IP: {}.{}.{}.{}", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3]);
    }
}


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int32 NET_IPSocket(char* net_interface, int32 port)
{
    SOCKET             newsocket;
    struct sockaddr_in address;
    bool               _true = true;
    int32              i = 1;
    int32              err;

    if (net_interface) {
        logger.info("Opening IP socket: {}:{}", net_interface, port);
    } else {
        logger.info("Opening IP socket: localhost:{}", port);
    }

    if ((newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
        err = WSAGetLastError();
        if (err != WSAEAFNOSUPPORT) {
            logger.warning("UDP_OpenSocket: socket: {}", NET_ErrorString());
        }
        return 0;
    }

    // make it non-blocking
    if (ioctlsocket(newsocket, FIONBIO, (u_long*) &_true) == SOCKET_ERROR) {
        logger.warning("UDP_OpenSocket: ioctl FIONBIO: {}", NET_ErrorString());
        return 0;
    }

    // make it broadcast capable
    if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*) &i, sizeof(i)) == SOCKET_ERROR) {
        logger.warning("UDP_OpenSocket: setsockopt SO_BROADCAST: {}", NET_ErrorString());
        return 0;
    }

    if (!net_interface || !net_interface[0] || !Q_stricmp(net_interface, "localhost")) {
        address.sin_addr.s_addr = INADDR_ANY;
    } else {
        Sys_StringToSockaddr(net_interface, (struct sockaddr*) &address);
    }

    if (port == PORT_ANY) {
        address.sin_port = 0;
    } else {
        address.sin_port = htons((int16) port);
    }

    address.sin_family = AF_INET;

    if (bind(newsocket, (const struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR) {
        logger.warning("UDP_OpenSocket: bind: {}", NET_ErrorString());
        closesocket(newsocket);
        return 0;
    }

    return newsocket;
}


/*
====================
NET_OpenSocks
====================
*/
void NET_OpenSocks(int32 port)
{
    struct sockaddr_in address;
    int32              err;
    struct hostent*    h;
    int32              len;
    bool               rfc1929;
    uint8              buf[64];

    usingSocks = false;

    logger.info("Opening connection to SOCKS server.");

    if ((socks_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        err = WSAGetLastError();
        logger.warning("NET_OpenSocks: socket: {}", NET_ErrorString());
        return;
    }

    h = gethostbyname(net_socksServer->string);
    if (h == NULL) {
        err = WSAGetLastError();
        logger.warning("NET_OpenSocks: gethostbyname: {}", NET_ErrorString());
        return;
    }
    if (h->h_addrtype != AF_INET) {
        logger.warning("NET_OpenSocks: gethostbyname: address type was not AF_INET");
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = *(int32*) h->h_addr_list[0];
    address.sin_port = htons((int16) net_socksPort->integer);

    if (connect(socks_socket, (struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR) {
        err = WSAGetLastError();
        logger.info("NET_OpenSocks: connect: {}", NET_ErrorString());
        return;
    }

    // send socks authentication handshake
    if (*net_socksUsername->string || *net_socksPassword->string) {
        rfc1929 = true;
    } else {
        rfc1929 = false;
    }

    buf[0] = 5; // SOCKS version
    // method count
    if (rfc1929) {
        buf[1] = 2;
        len = 4;
    } else {
        buf[1] = 1;
        len = 3;
    }
    buf[2] = 0;     // method #1 - method id #00: no authentication
    if (rfc1929) {
        buf[2] = 2; // method #2 - method id #02: username/password
    }
    if (send(socks_socket, (const char*) buf, len, 0) == SOCKET_ERROR) {
        err = WSAGetLastError();
        logger.info("NET_OpenSocks: send: {}", NET_ErrorString());
        return;
    }

    // get the response
    len = recv(socks_socket, (char*) buf, 64, 0);
    if (len == SOCKET_ERROR) {
        err = WSAGetLastError();
        logger.info("NET_OpenSocks: recv: {}", NET_ErrorString());
        return;
    }
    if (len != 2 || buf[0] != 5) {
        logger.info("NET_OpenSocks: bad response");
        return;
    }
    switch (buf[1]) {
    case 0: // no authentication
        break;
    case 2: // username/password authentication
        break;
    default:
        logger.info("NET_OpenSocks: request denied");
        return;
    }

    // do username/password authentication if needed
    if (buf[1] == 2) {
        int32 ulen;
        int32 plen;

        // build the request
        ulen = strlen(net_socksUsername->string);
        plen = strlen(net_socksPassword->string);

        buf[0] = 1; // username/password authentication version
        buf[1] = ulen;
        if (ulen) {
            memcpy(&buf[2], net_socksUsername->string, ulen);
        }
        buf[2 + ulen] = plen;
        if (plen) {
            memcpy(&buf[3 + ulen], net_socksPassword->string, plen);
        }

        // send it
        if (send(socks_socket, (const char*) buf, 3 + ulen + plen, 0) == SOCKET_ERROR) {
            err = WSAGetLastError();
            logger.info("NET_OpenSocks: send: {}", NET_ErrorString());
            return;
        }

        // get the response
        len = recv(socks_socket, (char*) buf, 64, 0);
        if (len == SOCKET_ERROR) {
            err = WSAGetLastError();
            logger.info("NET_OpenSocks: recv: {}", NET_ErrorString());
            return;
        }
        if (len != 2 || buf[0] != 1) {
            logger.info("NET_OpenSocks: bad response");
            return;
        }
        if (buf[1] != 0) {
            logger.info("NET_OpenSocks: authentication failed");
            return;
        }
    }

    // send the UDP associate request
    buf[0] = 5;                              // SOCKS version
    buf[1] = 3;                              // command: UDP associate
    buf[2] = 0;                              // reserved
    buf[3] = 1;                              // address type: IPV4
    *(int32*) &buf[4] = INADDR_ANY;
    *(int16*) &buf[8] = htons((int16) port); // port
    if (send(socks_socket, (char*) buf, 10, 0) == SOCKET_ERROR) {
        err = WSAGetLastError();
        logger.info("NET_OpenSocks: send: {}", NET_ErrorString());
        return;
    }

    // get the response
    len = recv(socks_socket, (char*) buf, 64, 0);
    if (len == SOCKET_ERROR) {
        err = WSAGetLastError();
        logger.info("NET_OpenSocks: recv: {}", NET_ErrorString());
        return;
    }
    if (len < 2 || buf[0] != 5) {
        logger.info("NET_OpenSocks: bad response");
        return;
    }
    // check completion code
    if (buf[1] != 0) {
        logger.info("NET_OpenSocks: request denied: {}", buf[1]);
        return;
    }
    if (buf[3] != 1) {
        logger.info("NET_OpenSocks: relay address is not IPV4: {}", buf[3]);
        return;
    }
    ((struct sockaddr_in*) &socksRelayAddr)->sin_family = AF_INET;
    ((struct sockaddr_in*) &socksRelayAddr)->sin_addr.s_addr = *(int32*) &buf[4];
    ((struct sockaddr_in*) &socksRelayAddr)->sin_port = *(int16*) &buf[8];
    memset(((struct sockaddr_in*) &socksRelayAddr)->sin_zero, 0, 8);

    usingSocks = true;
}


/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress()
{
    char            hostname[256];
    struct hostent* hostInfo;
    int32           error;
    char*           p;
    int32           ip;
    int32           n;

    if (gethostname(hostname, 256) == SOCKET_ERROR) {
        error = WSAGetLastError();
        return;
    }

    hostInfo = gethostbyname(hostname);
    if (!hostInfo) {
        error = WSAGetLastError();
        return;
    }

    logger.info("Hostname: {}", hostInfo->h_name);
    n = 0;
    while ((p = hostInfo->h_aliases[n++]) != NULL) {
        logger.info("Alias: {}", p);
    }

    if (hostInfo->h_addrtype != AF_INET) {
        return;
    }

    numIP = 0;
    while ((p = hostInfo->h_addr_list[numIP]) != NULL && numIP < MAX_IPS) {
        ip = ntohl(*(int32*) p);
        localIP[numIP][0] = p[0];
        localIP[numIP][1] = p[1];
        localIP[numIP][2] = p[2];
        localIP[numIP][3] = p[3];
        logger.info("IP: {}.{}.{}.{}", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
        numIP++;
    }
}

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP()
{
    cvar_t* ip;
    int32   port;
    int32   i;

    ip = Cvar_Get("net_ip", "localhost", CVAR_LATCH);
    port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH)->integer;

    // automatically scan for a valid port, so multiple
    // dedicated servers can be started without requiring
    // a different net_port for each one
    for (i = 0; i < 10; i++) {
        ip_socket = NET_IPSocket(ip->string, port + i);
        if (ip_socket) {
            Cvar_SetValue("net_port", port + i);
            if (net_socksEnabled->integer) {
                NET_OpenSocks(port + i);
            }
            NET_GetLocalAddress();
            return;
        }
    }
    logger.warning("Couldn't allocate IP port");
}


/*
====================
NET_IPXSocket
====================
*/
int32 NET_IPXSocket(int32 port)
{
    SOCKET              newsocket;
    struct sockaddr_ipx address;
    int32               _true = 1;
    int32               err;

    if ((newsocket = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX)) == INVALID_SOCKET) {
        err = WSAGetLastError();
        if (err != WSAEAFNOSUPPORT) {
            logger.warning("IPX_Socket: socket: {}", NET_ErrorString());
        }
        return 0;
    }

    // make it non-blocking
    if (ioctlsocket(newsocket, FIONBIO, (u_long*) &_true) == SOCKET_ERROR) {
        logger.warning("IPX_Socket: ioctl FIONBIO: {}", NET_ErrorString());
        return 0;
    }

    // make it broadcast capable
    if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*) &_true, sizeof(_true)) == SOCKET_ERROR) {
        logger.warning("IPX_Socket: setsockopt SO_BROADCAST: {}", NET_ErrorString());
        return 0;
    }

    address.sa_family = AF_IPX;
    memset(address.sa_netnum, 0, 4);
    memset(address.sa_nodenum, 0, 6);
    if (port == PORT_ANY) {
        address.sa_socket = 0;
    } else {
        address.sa_socket = htons((int16) port);
    }

    if (bind(newsocket, (const struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR) {
        logger.warning("IPX_Socket: bind: {}", NET_ErrorString());
        closesocket(newsocket);
        return 0;
    }

    return newsocket;
}


/*
====================
NET_OpenIPX
====================
*/
void NET_OpenIPX()
{
    int32 port;

    port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH)->integer;
    ipx_socket = NET_IPXSocket(port);
}


//===================================================================


/*
====================
NET_GetCvars
====================
*/
static bool NET_GetCvars()
{
    bool modified;

    modified = false;

    if (net_noudp && net_noudp->modified) {
        modified = true;
    }
    net_noudp = Cvar_Get("net_noudp", "0", CVAR_LATCH | CVAR_ARCHIVE);

    if (net_noipx && net_noipx->modified) {
        modified = true;
    }
    net_noipx = Cvar_Get("net_noipx", "0", CVAR_LATCH | CVAR_ARCHIVE);


    if (net_socksEnabled && net_socksEnabled->modified) {
        modified = true;
    }
    net_socksEnabled = Cvar_Get("net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE);

    if (net_socksServer && net_socksServer->modified) {
        modified = true;
    }
    net_socksServer = Cvar_Get("net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE);

    if (net_socksPort && net_socksPort->modified) {
        modified = true;
    }
    net_socksPort = Cvar_Get("net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE);

    if (net_socksUsername && net_socksUsername->modified) {
        modified = true;
    }
    net_socksUsername = Cvar_Get("net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE);

    if (net_socksPassword && net_socksPassword->modified) {
        modified = true;
    }
    net_socksPassword = Cvar_Get("net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE);


    return modified;
}


/*
====================
NET_Config
====================
*/
void NET_Config(bool enableNetworking)
{
    bool modified;
    bool stop;
    bool start;

    // get any latched changes to cvars
    modified = NET_GetCvars();

    if (net_noudp->integer && net_noipx->integer) {
        enableNetworking = false;
    }

    // if enable state is the same and no cvars were modified, we have nothing to do
    if (enableNetworking == networkingEnabled && !modified) {
        return;
    }

    if (enableNetworking == networkingEnabled) {
        if (enableNetworking) {
            stop = true;
            start = true;
        } else {
            stop = false;
            start = false;
        }
    } else {
        if (enableNetworking) {
            stop = false;
            start = true;
        } else {
            stop = true;
            start = false;
        }
        networkingEnabled = enableNetworking;
    }

    if (stop) {
        if (ip_socket && ip_socket != INVALID_SOCKET) {
            closesocket(ip_socket);
            ip_socket = 0;
        }

        if (socks_socket && socks_socket != INVALID_SOCKET) {
            closesocket(socks_socket);
            socks_socket = 0;
        }

        if (ipx_socket && ipx_socket != INVALID_SOCKET) {
            closesocket(ipx_socket);
            ipx_socket = 0;
        }
    }

    if (start) {
        if (!net_noudp->integer) {
            NET_OpenIP();
        }
        if (!net_noipx->integer) {
            NET_OpenIPX();
        }
    }
}


/*
====================
NET_Init
====================
*/
void NET_Init()
{
    int32 r;

    r = WSAStartup(MAKEWORD(1, 1), &winsockdata);
    if (r) {
        logger.warning("Winsock initialization failed, returned {}", r);
        return;
    }

    winsockInitialized = true;
    logger.info("Winsock Initialized");

    // this is really just to get the cvars registered
    NET_GetCvars();

    // FIXME testing!
    NET_Config(true);
}

/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
void NET_Sleep(int32 msec)
{
}


/*
====================
NET_Restart_f
====================
*/
void NET_Restart()
{
    NET_Config(networkingEnabled);
}
