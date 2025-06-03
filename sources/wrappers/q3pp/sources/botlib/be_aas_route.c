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
#include "l_utils.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_crc.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define ROUTING_DEBUG

// travel time in hundreths of a second = distance * 100 / speed
#define DISTANCEFACTOR_CROUCH   1.3f  // crouch speed = 100
#define DISTANCEFACTOR_SWIM     1     // should be 0.66, swim speed = 150
#define DISTANCEFACTOR_WALK     0.33f // walk speed = 300

// cache refresh time
#define CACHE_REFRESHTIME       15.0f // 15 seconds refresh time

// maximum number of routing updates each frame
#define MAX_FRAMEROUTINGUPDATES 10


/*

  area routing cache:
  stores the distances within one cluster to a specific goal area
  this goal area is in this same cluster and could be a cluster portal
  for every cluster there's a list with routing cache for every area
  in that cluster (including the portals of that cluster)
  area cache stores aasworld.clusters[?].numreachabilityareas travel times

  portal routing cache:
  stores the distances of all portals to a specific goal area
  this goal area could be in any cluster and could also be a cluster portal
  for every area (aasworld.numareas) the portal cache stores
  aasworld.numportals travel times

*/

#ifdef ROUTING_DEBUG
int32 numareacacheupdates;
int32 numportalcacheupdates;
#endif // ROUTING_DEBUG

int32 routingcachesize;
int32 max_routingcachesize;


#ifdef ROUTING_DEBUG
void AAS_RoutingInfo()
{
    botimport.Print(PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates);
    botimport.Print(PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates);
    botimport.Print(PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize);
}
#endif // ROUTING_DEBUG

// returns the number of the area in the cluster
// assumes the given area is in the given cluster or a portal of the cluster
__inline int32 AAS_ClusterAreaNum(int32 cluster, int32 areanum)
{
    int32 side, areacluster;

    areacluster = aasworld.areasettings[areanum].cluster;
    if (areacluster > 0) {
        return aasworld.areasettings[areanum].clusterareanum;
    } else {
        /*#ifdef ROUTING_DEBUG
                if (aasworld.portals[-areacluster].frontcluster != cluster &&
                        aasworld.portals[-areacluster].backcluster != cluster)
                {
                    botimport.Print(PRT_ERROR, "portal %d: does not belong to cluster %d\n"
                                                    , -areacluster, cluster);
                }
        #endif //ROUTING_DEBUG*/
        side = aasworld.portals[-areacluster].frontcluster != cluster;
        return aasworld.portals[-areacluster].clusterareanum[side];
    }
}

void AAS_InitTravelFlagFromType()
{
    int32 i;

    for (i = 0; i < MAX_TRAVELTYPES; i++) {
        aasworld.travelflagfortype[i] = TFL_INVALID;
    }
    aasworld.travelflagfortype[TRAVEL_INVALID] = TFL_INVALID;
    aasworld.travelflagfortype[TRAVEL_WALK] = TFL_WALK;
    aasworld.travelflagfortype[TRAVEL_CROUCH] = TFL_CROUCH;
    aasworld.travelflagfortype[TRAVEL_BARRIERJUMP] = TFL_BARRIERJUMP;
    aasworld.travelflagfortype[TRAVEL_JUMP] = TFL_JUMP;
    aasworld.travelflagfortype[TRAVEL_LADDER] = TFL_LADDER;
    aasworld.travelflagfortype[TRAVEL_WALKOFFLEDGE] = TFL_WALKOFFLEDGE;
    aasworld.travelflagfortype[TRAVEL_SWIM] = TFL_SWIM;
    aasworld.travelflagfortype[TRAVEL_WATERJUMP] = TFL_WATERJUMP;
    aasworld.travelflagfortype[TRAVEL_TELEPORT] = TFL_TELEPORT;
    aasworld.travelflagfortype[TRAVEL_ELEVATOR] = TFL_ELEVATOR;
    aasworld.travelflagfortype[TRAVEL_ROCKETJUMP] = TFL_ROCKETJUMP;
    aasworld.travelflagfortype[TRAVEL_BFGJUMP] = TFL_BFGJUMP;
    aasworld.travelflagfortype[TRAVEL_GRAPPLEHOOK] = TFL_GRAPPLEHOOK;
    aasworld.travelflagfortype[TRAVEL_DOUBLEJUMP] = TFL_DOUBLEJUMP;
    aasworld.travelflagfortype[TRAVEL_RAMPJUMP] = TFL_RAMPJUMP;
    aasworld.travelflagfortype[TRAVEL_STRAFEJUMP] = TFL_STRAFEJUMP;
    aasworld.travelflagfortype[TRAVEL_JUMPPAD] = TFL_JUMPPAD;
    aasworld.travelflagfortype[TRAVEL_FUNCBOB] = TFL_FUNCBOB;
}

__inline int32 AAS_TravelFlagForType_inline(int32 traveltype)
{
    int32 tfl;

    tfl = 0;
    if (tfl & TRAVELFLAG_NOTTEAM1) {
        tfl |= TFL_NOTTEAM1;
    }
    if (tfl & TRAVELFLAG_NOTTEAM2) {
        tfl |= TFL_NOTTEAM2;
    }
    traveltype &= TRAVELTYPE_MASK;
    if (traveltype < 0 || traveltype >= MAX_TRAVELTYPES) {
        return TFL_INVALID;
    }
    tfl |= aasworld.travelflagfortype[traveltype];
    return tfl;
}

int32 AAS_TravelFlagForType(int32 traveltype)
{
    return AAS_TravelFlagForType_inline(traveltype);
}

void AAS_UnlinkCache(aas_routingcache_t* cache)
{
    if (cache->time_next) {
        cache->time_next->time_prev = cache->time_prev;
    } else {
        aasworld.newestcache = cache->time_prev;
    }
    if (cache->time_prev) {
        cache->time_prev->time_next = cache->time_next;
    } else {
        aasworld.oldestcache = cache->time_next;
    }
    cache->time_next = NULL;
    cache->time_prev = NULL;
}

void AAS_LinkCache(aas_routingcache_t* cache)
{
    if (aasworld.newestcache) {
        aasworld.newestcache->time_next = cache;
        cache->time_prev = aasworld.newestcache;
    } else {
        aasworld.oldestcache = cache;
        cache->time_prev = NULL;
    }
    cache->time_next = NULL;
    aasworld.newestcache = cache;
}

void AAS_FreeRoutingCache(aas_routingcache_t* cache)
{
    AAS_UnlinkCache(cache);
    routingcachesize -= cache->size;
    FreeMemory(cache);
}

void AAS_RemoveRoutingCacheInCluster(int32 clusternum)
{
    int32               i;
    aas_routingcache_t *cache, *nextcache;
    aas_cluster_t*      cluster;

    if (!aasworld.clusterareacache) {
        return;
    }
    cluster = &aasworld.clusters[clusternum];
    for (i = 0; i < cluster->numareas; i++) {
        for (cache = aasworld.clusterareacache[clusternum][i]; cache; cache = nextcache) {
            nextcache = cache->next;
            AAS_FreeRoutingCache(cache);
        }
        aasworld.clusterareacache[clusternum][i] = NULL;
    }
}

void AAS_RemoveRoutingCacheUsingArea(int32 areanum)
{
    int32               i, clusternum;
    aas_routingcache_t *cache, *nextcache;

    clusternum = aasworld.areasettings[areanum].cluster;
    if (clusternum > 0) {
        // remove all the cache in the cluster the area is in
        AAS_RemoveRoutingCacheInCluster(clusternum);
    } else {
        // if this is a portal remove all cache in both the front and back cluster
        AAS_RemoveRoutingCacheInCluster(aasworld.portals[-clusternum].frontcluster);
        AAS_RemoveRoutingCacheInCluster(aasworld.portals[-clusternum].backcluster);
    }
    // remove all portal cache
    for (i = 0; i < aasworld.numareas; i++) {
        // refresh portal cache
        for (cache = aasworld.portalcache[i]; cache; cache = nextcache) {
            nextcache = cache->next;
            AAS_FreeRoutingCache(cache);
        }
        aasworld.portalcache[i] = NULL;
    }
}

int32 AAS_EnableRoutingArea(int32 areanum, int32 enable)
{
    int32 flags;

    if (areanum <= 0 || areanum >= aasworld.numareas) {
        if (bot_developer) {
            botimport.Print(PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum);
        }
        return 0;
    }
    flags = aasworld.areasettings[areanum].areaflags & AREA_DISABLED;
    if (enable < 0) {
        return !flags;
    }

    if (enable) {
        aasworld.areasettings[areanum].areaflags &= ~AREA_DISABLED;
    } else {
        aasworld.areasettings[areanum].areaflags |= AREA_DISABLED;
    }
    // if the status of the area changed
    if ((flags & AREA_DISABLED) != (aasworld.areasettings[areanum].areaflags & AREA_DISABLED)) {
        // remove all routing cache involving this area
        AAS_RemoveRoutingCacheUsingArea(areanum);
    }
    return !flags;
}

__inline float AAS_RoutingTime()
{
    return AAS_Time();
}

int32 AAS_GetAreaContentsTravelFlags(int32 areanum)
{
    int32 contents, tfl;

    contents = aasworld.areasettings[areanum].contents;
    tfl = 0;
    if (contents & AREACONTENTS_WATER) {
        tfl |= TFL_WATER;
    } else if (contents & AREACONTENTS_SLIME) {
        tfl |= TFL_SLIME;
    } else if (contents & AREACONTENTS_LAVA) {
        tfl |= TFL_LAVA;
    } else {
        tfl |= TFL_AIR;
    }
    if (contents & AREACONTENTS_DONOTENTER) {
        tfl |= TFL_DONOTENTER;
    }
    if (contents & AREACONTENTS_NOTTEAM1) {
        tfl |= TFL_NOTTEAM1;
    }
    if (contents & AREACONTENTS_NOTTEAM2) {
        tfl |= TFL_NOTTEAM2;
    }
    if (aasworld.areasettings[areanum].areaflags & AREA_BRIDGE) {
        tfl |= TFL_BRIDGE;
    }
    return tfl;
}

__inline int32 AAS_AreaContentsTravelFlags_inline(int32 areanum)
{
    return aasworld.areacontentstravelflags[areanum];
}

int32 AAS_AreaContentsTravelFlags(int32 areanum)
{
    return aasworld.areacontentstravelflags[areanum];
}

void AAS_InitAreaContentsTravelFlags()
{
    int32 i;

    if (aasworld.areacontentstravelflags) {
        FreeMemory(aasworld.areacontentstravelflags);
    }
    aasworld.areacontentstravelflags = (int32*) GetClearedMemory(aasworld.numareas * sizeof(int32));
    //
    for (i = 0; i < aasworld.numareas; i++) {
        aasworld.areacontentstravelflags[i] = AAS_GetAreaContentsTravelFlags(i);
    }
}

void AAS_CreateReversedReachability()
{
    int32               i, n;
    aas_reversedlink_t* revlink;
    aas_reachability_t* reach;
    aas_areasettings_t* settings;
    char*               ptr;
#ifdef DEBUG
    int32 starttime;

    starttime = Sys_MilliSeconds();
#endif
    // free reversed links that have already been created
    if (aasworld.reversedreachability) {
        FreeMemory(aasworld.reversedreachability);
    }
    // allocate memory for the reversed reachability links
    ptr = (char*) GetClearedMemory(aasworld.numareas * sizeof(aas_reversedreachability_t) + aasworld.reachabilitysize * sizeof(aas_reversedlink_t));
    //
    aasworld.reversedreachability = (aas_reversedreachability_t*) ptr;
    // pointer to the memory for the reversed links
    ptr += aasworld.numareas * sizeof(aas_reversedreachability_t);
    // check all reachabilities of all areas
    for (i = 1; i < aasworld.numareas; i++) {
        // settings of the area
        settings = &aasworld.areasettings[i];
        //
        if (settings->numreachableareas >= 128) {
            botimport.Print(PRT_WARNING, "area %d has more than 128 reachabilities\n", i);
        }
        // create reversed links for the reachabilities
        for (n = 0; n < settings->numreachableareas && n < 128; n++) {
            // reachability link
            reach = &aasworld.reachability[settings->firstreachablearea + n];
            //
            revlink = (aas_reversedlink_t*) ptr;
            ptr += sizeof(aas_reversedlink_t);
            //
            revlink->areanum = i;
            revlink->linknum = settings->firstreachablearea + n;
            revlink->next = aasworld.reversedreachability[reach->areanum].first;
            aasworld.reversedreachability[reach->areanum].first = revlink;
            aasworld.reversedreachability[reach->areanum].numlinks++;
        }
    }
#ifdef DEBUG
    botimport.Print(PRT_MESSAGE, "reversed reachability %d msec\n", Sys_MilliSeconds() - starttime);
#endif
}

uint16 AAS_AreaTravelTime(int32 areanum, vec3_t start, vec3_t end)
{
    int32  intdist;
    float  dist;
    vec3_t dir;

    VectorSubtract(start, end, dir);
    dist = VectorLength(dir);
    // if crouch only area
    if (AAS_AreaCrouch(areanum)) {
        dist *= DISTANCEFACTOR_CROUCH;
    }
    // if swim area
    else if (AAS_AreaSwim(areanum)) {
        dist *= DISTANCEFACTOR_SWIM;
    }
    // normal walk area
    else {
        dist *= DISTANCEFACTOR_WALK;
    }
    //
    intdist = (int32) dist;
    // make sure the distance isn't zero
    if (intdist <= 0) {
        intdist = 1;
    }
    return intdist;
}

void AAS_CalculateAreaTravelTimes()
{
    int32                       i, l, n, size;
    char*                       ptr;
    vec3_t                      end;
    aas_reversedreachability_t* revreach;
    aas_reversedlink_t*         revlink;
    aas_reachability_t*         reach;
    aas_areasettings_t*         settings;
    int32                       starttime;

    starttime = Sys_MilliSeconds();
    // if there are still area travel times, free the memory
    if (aasworld.areatraveltimes) {
        FreeMemory(aasworld.areatraveltimes);
    }
    // get the total size of all the area travel times
    size = aasworld.numareas * sizeof(uint16**);
    for (i = 0; i < aasworld.numareas; i++) {
        revreach = &aasworld.reversedreachability[i];
        // settings of the area
        settings = &aasworld.areasettings[i];
        //
        size += settings->numreachableareas * sizeof(uint16*);
        //
        size += settings->numreachableareas * revreach->numlinks * sizeof(uint16);
    }
    // allocate memory for the area travel times
    ptr = (char*) GetClearedMemory(size);
    aasworld.areatraveltimes = (uint16***) ptr;
    ptr += aasworld.numareas * sizeof(uint16**);
    // calcluate the travel times for all the areas
    for (i = 0; i < aasworld.numareas; i++) {
        // reversed reachabilities of this area
        revreach = &aasworld.reversedreachability[i];
        // settings of the area
        settings = &aasworld.areasettings[i];
        //
        aasworld.areatraveltimes[i] = (uint16**) ptr;
        ptr += settings->numreachableareas * sizeof(uint16*);
        //
        for (l = 0; l < settings->numreachableareas; l++) {
            aasworld.areatraveltimes[i][l] = (uint16*) ptr;
            ptr += revreach->numlinks * sizeof(uint16);
            // reachability link
            reach = &aasworld.reachability[settings->firstreachablearea + l];
            //
            for (n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++) {
                VectorCopy(aasworld.reachability[revlink->linknum].end, end);
                //
                aasworld.areatraveltimes[i][l][n] = AAS_AreaTravelTime(i, end, reach->start);
            }
        }
    }
#ifdef DEBUG
    botimport.Print(PRT_MESSAGE, "area travel times %d msec\n", Sys_MilliSeconds() - starttime);
#endif
}

int32 AAS_PortalMaxTravelTime(int32 portalnum)
{
    int32                       l, n, t, maxt;
    aas_portal_t*               portal;
    aas_reversedreachability_t* revreach;
    aas_reversedlink_t*         revlink;
    aas_areasettings_t*         settings;

    portal = &aasworld.portals[portalnum];
    // reversed reachabilities of this portal area
    revreach = &aasworld.reversedreachability[portal->areanum];
    // settings of the portal area
    settings = &aasworld.areasettings[portal->areanum];
    //
    maxt = 0;
    for (l = 0; l < settings->numreachableareas; l++) {
        for (n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++) {
            t = aasworld.areatraveltimes[portal->areanum][l][n];
            if (t > maxt) {
                maxt = t;
            }
        }
    }
    return maxt;
}

void AAS_InitPortalMaxTravelTimes()
{
    int32 i;

    if (aasworld.portalmaxtraveltimes) {
        FreeMemory(aasworld.portalmaxtraveltimes);
    }

    aasworld.portalmaxtraveltimes = (int32*) GetClearedMemory(aasworld.numportals * sizeof(int32));

    for (i = 0; i < aasworld.numportals; i++) {
        aasworld.portalmaxtraveltimes[i] = AAS_PortalMaxTravelTime(i);
        // botimport.Print(PRT_MESSAGE, "portal %d max tt = %d\n", i, aasworld.portalmaxtraveltimes[i]);
    }
}

aas_routingcache_t* AAS_AllocRoutingCache(int32 numtraveltimes)
{
    aas_routingcache_t* cache;
    int32               size;

    //
    size = sizeof(aas_routingcache_t)
         + numtraveltimes * sizeof(uint16)
         + numtraveltimes * sizeof(uint8);
    //
    routingcachesize += size;
    //
    cache = (aas_routingcache_t*) GetClearedMemory(size);
    cache->reachabilities = (uint8*) cache + sizeof(aas_routingcache_t)
                          + numtraveltimes * sizeof(uint16);
    cache->size = size;
    return cache;
}

void AAS_FreeAllClusterAreaCache()
{
    int32               i, j;
    aas_routingcache_t *cache, *nextcache;
    aas_cluster_t*      cluster;

    // free all cluster cache if existing
    if (!aasworld.clusterareacache) {
        return;
    }
    // free caches
    for (i = 0; i < aasworld.numclusters; i++) {
        cluster = &aasworld.clusters[i];
        for (j = 0; j < cluster->numareas; j++) {
            for (cache = aasworld.clusterareacache[i][j]; cache; cache = nextcache) {
                nextcache = cache->next;
                AAS_FreeRoutingCache(cache);
            }
            aasworld.clusterareacache[i][j] = NULL;
        }
    }
    // free the cluster cache array
    FreeMemory(aasworld.clusterareacache);
    aasworld.clusterareacache = NULL;
}

void AAS_InitClusterAreaCache()
{
    int32 i, size;
    char* ptr;

    //
    for (size = 0, i = 0; i < aasworld.numclusters; i++) {
        size += aasworld.clusters[i].numareas;
    }
    // two dimensional array with pointers for every cluster to routing cache
    // for every area in that cluster
    ptr = (char*) GetClearedMemory(
        aasworld.numclusters * sizeof(aas_routingcache_t**) + size * sizeof(aas_routingcache_t*)
    );
    aasworld.clusterareacache = (aas_routingcache_t***) ptr;
    ptr += aasworld.numclusters * sizeof(aas_routingcache_t**);
    for (i = 0; i < aasworld.numclusters; i++) {
        aasworld.clusterareacache[i] = (aas_routingcache_t**) ptr;
        ptr += aasworld.clusters[i].numareas * sizeof(aas_routingcache_t*);
    }
}

void AAS_FreeAllPortalCache()
{
    int32               i;
    aas_routingcache_t *cache, *nextcache;

    // free all portal cache if existing
    if (!aasworld.portalcache) {
        return;
    }
    // free portal caches
    for (i = 0; i < aasworld.numareas; i++) {
        for (cache = aasworld.portalcache[i]; cache; cache = nextcache) {
            nextcache = cache->next;
            AAS_FreeRoutingCache(cache);
        }
        aasworld.portalcache[i] = NULL;
    }
    FreeMemory(aasworld.portalcache);
    aasworld.portalcache = NULL;
}

void AAS_InitPortalCache()
{
    //
    aasworld.portalcache = (aas_routingcache_t**) GetClearedMemory(
        aasworld.numareas * sizeof(aas_routingcache_t*)
    );
}

void AAS_InitRoutingUpdate()
{
    int32 i, maxreachabilityareas;

    // free routing update fields if already existing
    if (aasworld.areaupdate) {
        FreeMemory(aasworld.areaupdate);
    }
    //
    maxreachabilityareas = 0;
    for (i = 0; i < aasworld.numclusters; i++) {
        if (aasworld.clusters[i].numreachabilityareas > maxreachabilityareas) {
            maxreachabilityareas = aasworld.clusters[i].numreachabilityareas;
        }
    }
    // allocate memory for the routing update fields
    aasworld.areaupdate = (aas_routingupdate_t*) GetClearedMemory(
        maxreachabilityareas * sizeof(aas_routingupdate_t)
    );
    //
    if (aasworld.portalupdate) {
        FreeMemory(aasworld.portalupdate);
    }
    // allocate memory for the portal update fields
    aasworld.portalupdate = (aas_routingupdate_t*) GetClearedMemory(
        (aasworld.numportals + 1) * sizeof(aas_routingupdate_t)
    );
}

void AAS_CreateAllRoutingCache()
{
    int32 i, j, t;

    aasworld.initialized = true;
    botimport.Print(PRT_MESSAGE, "AAS_CreateAllRoutingCache\n");
    for (i = 1; i < aasworld.numareas; i++) {
        if (!AAS_AreaReachability(i)) {
            continue;
        }
        for (j = 1; j < aasworld.numareas; j++) {
            if (i == j) {
                continue;
            }
            if (!AAS_AreaReachability(j)) {
                continue;
            }
            t = AAS_AreaTravelTimeToGoalArea(i, aasworld.areas[i].center, j, TFL_DEFAULT);
            // Log_Write("traveltime from %d to %d is %d", i, j, t);
        }
    }
    aasworld.initialized = false;
}

// the route cache header
// this header is followed by numportalcache + numareacache aas_routingcache_t
// structures that store routing cache
typedef struct routecacheheader_s
{
    int32 ident;
    int32 version;
    int32 numareas;
    int32 numclusters;
    int32 areacrc;
    int32 clustercrc;
    int32 numportalcache;
    int32 numareacache;
} routecacheheader_t;

#define RCID      (('C' << 24) + ('R' << 16) + ('E' << 8) + 'M')
#define RCVERSION 2

void AAS_WriteRouteCache()
{
    int32               i, j, numportalcache, numareacache, totalsize;
    aas_routingcache_t* cache;
    aas_cluster_t*      cluster;
    fileHandle_t        fp;
    char                filename[MAX_QPATH];
    routecacheheader_t  routecacheheader;

    numportalcache = 0;
    for (i = 0; i < aasworld.numareas; i++) {
        for (cache = aasworld.portalcache[i]; cache; cache = cache->next) {
            numportalcache++;
        }
    }
    numareacache = 0;
    for (i = 0; i < aasworld.numclusters; i++) {
        cluster = &aasworld.clusters[i];
        for (j = 0; j < cluster->numareas; j++) {
            for (cache = aasworld.clusterareacache[i][j]; cache; cache = cache->next) {
                numareacache++;
            }
        }
    }
    // open the file for writing
    Com_sprintf(filename, MAX_QPATH, "maps/%s.rcd", aasworld.mapname);
    botimport.FS_FOpenFile(filename, &fp, FS_WRITE);
    if (!fp) {
        AAS_Error("Unable to open file: %s\n", filename);
        return;
    }
    // create the header
    routecacheheader.ident = RCID;
    routecacheheader.version = RCVERSION;
    routecacheheader.numareas = aasworld.numareas;
    routecacheheader.numclusters = aasworld.numclusters;
    routecacheheader.areacrc = CRC_ProcessString((uint8*) aasworld.areas, sizeof(aas_area_t) * aasworld.numareas);
    routecacheheader.clustercrc = CRC_ProcessString((uint8*) aasworld.clusters, sizeof(aas_cluster_t) * aasworld.numclusters);
    routecacheheader.numportalcache = numportalcache;
    routecacheheader.numareacache = numareacache;
    // write the header
    botimport.FS_Write(&routecacheheader, sizeof(routecacheheader_t), fp);
    //
    totalsize = 0;
    // write all the cache
    for (i = 0; i < aasworld.numareas; i++) {
        for (cache = aasworld.portalcache[i]; cache; cache = cache->next) {
            botimport.FS_Write(cache, cache->size, fp);
            totalsize += cache->size;
        }
    }
    for (i = 0; i < aasworld.numclusters; i++) {
        cluster = &aasworld.clusters[i];
        for (j = 0; j < cluster->numareas; j++) {
            for (cache = aasworld.clusterareacache[i][j]; cache; cache = cache->next) {
                botimport.FS_Write(cache, cache->size, fp);
                totalsize += cache->size;
            }
        }
    }

    botimport.FS_FCloseFile(fp);
    botimport.Print(PRT_MESSAGE, "\nroute cache written to %s\n", filename);
    botimport.Print(PRT_MESSAGE, "written %d bytes of routing cache\n", totalsize);
}

aas_routingcache_t* AAS_ReadCache(fileHandle_t fp)
{
    int32               size;
    aas_routingcache_t* cache;

    botimport.FS_Read(&size, sizeof(size), fp);
    cache = (aas_routingcache_t*) GetMemory(size);
    cache->size = size;
    botimport.FS_Read((uint8*) cache + sizeof(size), size - sizeof(size), fp);
    cache->reachabilities = (uint8*) cache + sizeof(aas_routingcache_t) - sizeof(uint16) + (size - sizeof(aas_routingcache_t) + sizeof(uint16)) / 3 * 2;
    return cache;
}

int32 AAS_ReadRouteCache()
{
    int32               i, clusterareanum; //, size;
    fileHandle_t        fp;
    char                filename[MAX_QPATH];
    routecacheheader_t  routecacheheader;
    aas_routingcache_t* cache;

    Com_sprintf(filename, MAX_QPATH, "maps/%s.rcd", aasworld.mapname);
    botimport.FS_FOpenFile(filename, &fp, FS_READ);
    if (!fp) {
        return false;
    }
    botimport.FS_Read(&routecacheheader, sizeof(routecacheheader_t), fp);
    if (routecacheheader.ident != RCID) {
        AAS_Error("%s is not a route cache dump\n");
        return false;
    }
    if (routecacheheader.version != RCVERSION) {
        AAS_Error("route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION);
        return false;
    }
    if (routecacheheader.numareas != aasworld.numareas) {
        // AAS_Error("route cache dump has wrong number of areas\n");
        return false;
    }
    if (routecacheheader.numclusters != aasworld.numclusters) {
        // AAS_Error("route cache dump has wrong number of clusters\n");
        return false;
    }
    if (routecacheheader.areacrc != CRC_ProcessString((uint8*) aasworld.areas, sizeof(aas_area_t) * aasworld.numareas)) {
        // AAS_Error("route cache dump area CRC incorrect\n");
        return false;
    }
    if (routecacheheader.clustercrc != CRC_ProcessString((uint8*) aasworld.clusters, sizeof(aas_cluster_t) * aasworld.numclusters)) {
        // AAS_Error("route cache dump cluster CRC incorrect\n");
        return false;
    }
    // read all the portal cache
    for (i = 0; i < routecacheheader.numportalcache; i++) {
        cache = AAS_ReadCache(fp);
        cache->next = aasworld.portalcache[cache->areanum];
        cache->prev = NULL;
        if (aasworld.portalcache[cache->areanum]) {
            aasworld.portalcache[cache->areanum]->prev = cache;
        }
        aasworld.portalcache[cache->areanum] = cache;
    }
    // read all the cluster area cache
    for (i = 0; i < routecacheheader.numareacache; i++) {
        cache = AAS_ReadCache(fp);
        clusterareanum = AAS_ClusterAreaNum(cache->cluster, cache->areanum);
        cache->next = aasworld.clusterareacache[cache->cluster][clusterareanum];
        cache->prev = NULL;
        if (aasworld.clusterareacache[cache->cluster][clusterareanum]) {
            aasworld.clusterareacache[cache->cluster][clusterareanum]->prev = cache;
        }
        aasworld.clusterareacache[cache->cluster][clusterareanum] = cache;
    }

    botimport.FS_FCloseFile(fp);
    return true;
}

#define MAX_REACHABILITYPASSAREAS 32

void AAS_InitReachabilityAreas()
{
    int32               i, j, numareas, areas[MAX_REACHABILITYPASSAREAS];
    int32               numreachareas;
    aas_reachability_t* reach;
    vec3_t              start, end;

    if (aasworld.reachabilityareas) {
        FreeMemory(aasworld.reachabilityareas);
    }
    if (aasworld.reachabilityareaindex) {
        FreeMemory(aasworld.reachabilityareaindex);
    }

    aasworld.reachabilityareas = (aas_reachabilityareas_t*)
        GetClearedMemory(aasworld.reachabilitysize * sizeof(aas_reachabilityareas_t));
    aasworld.reachabilityareaindex = (int32*)
        GetClearedMemory(aasworld.reachabilitysize * MAX_REACHABILITYPASSAREAS * sizeof(int32));
    numreachareas = 0;
    for (i = 0; i < aasworld.reachabilitysize; i++) {
        reach = &aasworld.reachability[i];
        numareas = 0;
        switch (reach->traveltype & TRAVELTYPE_MASK) {
        // trace areas from start to end
        case TRAVEL_BARRIERJUMP:
        case TRAVEL_WATERJUMP:
            VectorCopy(reach->start, end);
            end[2] = reach->end[2];
            numareas = AAS_TraceAreas(reach->start, end, areas, NULL, MAX_REACHABILITYPASSAREAS);
            break;
        case TRAVEL_WALKOFFLEDGE:
            VectorCopy(reach->end, start);
            start[2] = reach->start[2];
            numareas = AAS_TraceAreas(start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS);
            break;
        case TRAVEL_GRAPPLEHOOK:
            numareas = AAS_TraceAreas(reach->start, reach->end, areas, NULL, MAX_REACHABILITYPASSAREAS);
            break;

        // trace arch
        case TRAVEL_JUMP:
            break;
        case TRAVEL_ROCKETJUMP:
            break;
        case TRAVEL_BFGJUMP:
            break;
        case TRAVEL_JUMPPAD:
            break;

        // trace from reach->start to entity center, along entity movement
        // and from entity center to reach->end
        case TRAVEL_ELEVATOR:
            break;
        case TRAVEL_FUNCBOB:
            break;

        // no areas in between
        case TRAVEL_WALK:
            break;
        case TRAVEL_CROUCH:
            break;
        case TRAVEL_LADDER:
            break;
        case TRAVEL_SWIM:
            break;
        case TRAVEL_TELEPORT:
            break;
        default:
            break;
        }
        aasworld.reachabilityareas[i].firstarea = numreachareas;
        aasworld.reachabilityareas[i].numareas = numareas;
        for (j = 0; j < numareas; j++) {
            aasworld.reachabilityareaindex[numreachareas++] = areas[j];
        }
    }
}

void AAS_InitRouting()
{
    AAS_InitTravelFlagFromType();
    //
    AAS_InitAreaContentsTravelFlags();
    // initialize the routing update fields
    AAS_InitRoutingUpdate();
    // create reversed reachability links used by the routing update algorithm
    AAS_CreateReversedReachability();
    // initialize the cluster cache
    AAS_InitClusterAreaCache();
    // initialize portal cache
    AAS_InitPortalCache();
    // initialize the area travel times
    AAS_CalculateAreaTravelTimes();
    // calculate the maximum travel times through portals
    AAS_InitPortalMaxTravelTimes();
    // get the areas reachabilities go through
    AAS_InitReachabilityAreas();
    //
#ifdef ROUTING_DEBUG
    numareacacheupdates = 0;
    numportalcacheupdates = 0;
#endif // ROUTING_DEBUG
    //
    routingcachesize = 0;
    max_routingcachesize = 1024 * (int32) LibVarValue("max_routingcache", "4096");
    // read any routing cache if available
    AAS_ReadRouteCache();
}

void AAS_FreeRoutingCaches()
{
    // free all the existing cluster area cache
    AAS_FreeAllClusterAreaCache();
    // free all the existing portal cache
    AAS_FreeAllPortalCache();
    // free cached travel times within areas
    if (aasworld.areatraveltimes) {
        FreeMemory(aasworld.areatraveltimes);
    }
    aasworld.areatraveltimes = NULL;
    // free cached maximum travel time through cluster portals
    if (aasworld.portalmaxtraveltimes) {
        FreeMemory(aasworld.portalmaxtraveltimes);
    }
    aasworld.portalmaxtraveltimes = NULL;
    // free reversed reachability links
    if (aasworld.reversedreachability) {
        FreeMemory(aasworld.reversedreachability);
    }
    aasworld.reversedreachability = NULL;
    // free routing algorithm memory
    if (aasworld.areaupdate) {
        FreeMemory(aasworld.areaupdate);
    }
    aasworld.areaupdate = NULL;
    if (aasworld.portalupdate) {
        FreeMemory(aasworld.portalupdate);
    }
    aasworld.portalupdate = NULL;
    // free lists with areas the reachabilities go through
    if (aasworld.reachabilityareas) {
        FreeMemory(aasworld.reachabilityareas);
    }
    aasworld.reachabilityareas = NULL;
    // free the reachability area index
    if (aasworld.reachabilityareaindex) {
        FreeMemory(aasworld.reachabilityareaindex);
    }
    aasworld.reachabilityareaindex = NULL;
    // free area contents travel flags look up table
    if (aasworld.areacontentstravelflags) {
        FreeMemory(aasworld.areacontentstravelflags);
    }
    aasworld.areacontentstravelflags = NULL;
}

// update the given routing cache
void AAS_UpdateAreaRoutingCache(aas_routingcache_t* areacache)
{
    int32                       i, nextareanum, cluster, badtravelflags, clusterareanum, linknum;
    int32                       numreachabilityareas;
    uint16                      t, startareatraveltimes[128]; // NOTE: not more than 128 reachabilities per area allowed
    aas_routingupdate_t *       updateliststart, *updatelistend, *curupdate, *nextupdate;
    aas_reachability_t*         reach;
    aas_reversedreachability_t* revreach;
    aas_reversedlink_t*         revlink;

#ifdef ROUTING_DEBUG
    numareacacheupdates++;
#endif // ROUTING_DEBUG
    // number of reachability areas within this cluster
    numreachabilityareas = aasworld.clusters[areacache->cluster].numreachabilityareas;
    //
    aasworld.frameroutingupdates++;
    // clear the routing update fields
    //    Com_Memset(aasworld.areaupdate, 0, aasworld.numareas * sizeof(aas_routingupdate_t));
    //
    badtravelflags = ~areacache->travelflags;
    //
    clusterareanum = AAS_ClusterAreaNum(areacache->cluster, areacache->areanum);
    if (clusterareanum >= numreachabilityareas) {
        return;
    }
    //
    Com_Memset(startareatraveltimes, 0, sizeof(startareatraveltimes));
    //
    curupdate = &aasworld.areaupdate[clusterareanum];
    curupdate->areanum = areacache->areanum;
    // VectorCopy(areacache->origin, curupdate->start);
    curupdate->areatraveltimes = startareatraveltimes;
    curupdate->tmptraveltime = areacache->starttraveltime;
    //
    areacache->traveltimes[clusterareanum] = areacache->starttraveltime;
    // put the area to start with in the current read list
    curupdate->next = NULL;
    curupdate->prev = NULL;
    updateliststart = curupdate;
    updatelistend = curupdate;
    // while there are updates in the current list
    while (updateliststart) {
        curupdate = updateliststart;
        //
        if (curupdate->next) {
            curupdate->next->prev = NULL;
        } else {
            updatelistend = NULL;
        }
        updateliststart = curupdate->next;
        //
        curupdate->inlist = false;
        // check all reversed reachability links
        revreach = &aasworld.reversedreachability[curupdate->areanum];
        //
        for (i = 0, revlink = revreach->first; revlink; revlink = revlink->next, i++) {
            linknum = revlink->linknum;
            reach = &aasworld.reachability[linknum];
            // if there is used an undesired travel type
            if (AAS_TravelFlagForType_inline(reach->traveltype) & badtravelflags) {
                continue;
            }
            // if not allowed to enter the next area
            if (aasworld.areasettings[reach->areanum].areaflags & AREA_DISABLED) {
                continue;
            }
            // if the next area has a not allowed travel flag
            if (AAS_AreaContentsTravelFlags_inline(reach->areanum) & badtravelflags) {
                continue;
            }
            // number of the area the reversed reachability leads to
            nextareanum = revlink->areanum;
            // get the cluster number of the area
            cluster = aasworld.areasettings[nextareanum].cluster;
            // don't leave the cluster
            if (cluster > 0 && cluster != areacache->cluster) {
                continue;
            }
            // get the number of the area in the cluster
            clusterareanum = AAS_ClusterAreaNum(areacache->cluster, nextareanum);
            if (clusterareanum >= numreachabilityareas) {
                continue;
            }
            // time already travelled plus the traveltime through
            // the current area plus the travel time from the reachability
            t = curupdate->tmptraveltime +
                // AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->end) +
                curupdate->areatraveltimes[i] + reach->traveltime;
            //
            if (!areacache->traveltimes[clusterareanum] || areacache->traveltimes[clusterareanum] > t) {
                areacache->traveltimes[clusterareanum] = t;
                areacache->reachabilities[clusterareanum] = linknum - aasworld.areasettings[nextareanum].firstreachablearea;
                nextupdate = &aasworld.areaupdate[clusterareanum];
                nextupdate->areanum = nextareanum;
                nextupdate->tmptraveltime = t;
                // VectorCopy(reach->start, nextupdate->start);
                nextupdate->areatraveltimes = aasworld.areatraveltimes[nextareanum][linknum - aasworld.areasettings[nextareanum].firstreachablearea];
                if (!nextupdate->inlist) {
                    // we add the update to the end of the list
                    // we could also use a B+ tree to have a real sorted list
                    // on travel time which makes for faster routing updates
                    nextupdate->next = NULL;
                    nextupdate->prev = updatelistend;
                    if (updatelistend) {
                        updatelistend->next = nextupdate;
                    } else {
                        updateliststart = nextupdate;
                    }
                    updatelistend = nextupdate;
                    nextupdate->inlist = true;
                }
            }
        }
    }
}

aas_routingcache_t* AAS_GetAreaRoutingCache(int32 clusternum, int32 areanum, int32 travelflags)
{
    int32               clusterareanum;
    aas_routingcache_t *cache, *clustercache;

    // number of the area in the cluster
    clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
    // pointer to the cache for the area in the cluster
    clustercache = aasworld.clusterareacache[clusternum][clusterareanum];
    // find the cache without undesired travel flags
    for (cache = clustercache; cache; cache = cache->next) {
        // if there aren't used any undesired travel types for the cache
        if (cache->travelflags == travelflags) {
            break;
        }
    }
    // if there was no cache
    if (!cache) {
        cache = AAS_AllocRoutingCache(aasworld.clusters[clusternum].numreachabilityareas);
        cache->cluster = clusternum;
        cache->areanum = areanum;
        VectorCopy(aasworld.areas[areanum].center, cache->origin);
        cache->starttraveltime = 1;
        cache->travelflags = travelflags;
        cache->prev = NULL;
        cache->next = clustercache;
        if (clustercache) {
            clustercache->prev = cache;
        }
        aasworld.clusterareacache[clusternum][clusterareanum] = cache;
        AAS_UpdateAreaRoutingCache(cache);
    } else {
        AAS_UnlinkCache(cache);
    }
    // the cache has been accessed
    cache->time = AAS_RoutingTime();
    cache->type = CACHETYPE_AREA;
    AAS_LinkCache(cache);
    return cache;
}

void AAS_UpdatePortalRoutingCache(aas_routingcache_t* portalcache)
{
    int32                i, portalnum, clusterareanum, clusternum;
    uint16               t;
    aas_portal_t*        portal;
    aas_cluster_t*       cluster;
    aas_routingcache_t*  cache;
    aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;

#ifdef ROUTING_DEBUG
    numportalcacheupdates++;
#endif // ROUTING_DEBUG
    // clear the routing update fields
    //    Com_Memset(aasworld.portalupdate, 0, (aasworld.numportals+1) * sizeof(aas_routingupdate_t));
    //
    curupdate = &aasworld.portalupdate[aasworld.numportals];
    curupdate->cluster = portalcache->cluster;
    curupdate->areanum = portalcache->areanum;
    curupdate->tmptraveltime = portalcache->starttraveltime;
    // if the start area is a cluster portal, store the travel time for that portal
    clusternum = aasworld.areasettings[portalcache->areanum].cluster;
    if (clusternum < 0) {
        portalcache->traveltimes[-clusternum] = portalcache->starttraveltime;
    }
    // put the area to start with in the current read list
    curupdate->next = NULL;
    curupdate->prev = NULL;
    updateliststart = curupdate;
    updatelistend = curupdate;
    // while there are updates in the current list
    while (updateliststart) {
        curupdate = updateliststart;
        // remove the current update from the list
        if (curupdate->next) {
            curupdate->next->prev = NULL;
        } else {
            updatelistend = NULL;
        }
        updateliststart = curupdate->next;
        // current update is removed from the list
        curupdate->inlist = false;
        //
        cluster = &aasworld.clusters[curupdate->cluster];
        //
        cache = AAS_GetAreaRoutingCache(curupdate->cluster, curupdate->areanum, portalcache->travelflags);
        // take all portals of the cluster
        for (i = 0; i < cluster->numportals; i++) {
            portalnum = aasworld.portalindex[cluster->firstportal + i];
            portal = &aasworld.portals[portalnum];
            // if this is the portal of the current update continue
            if (portal->areanum == curupdate->areanum) {
                continue;
            }
            //
            clusterareanum = AAS_ClusterAreaNum(curupdate->cluster, portal->areanum);
            if (clusterareanum >= cluster->numreachabilityareas) {
                continue;
            }
            //
            t = cache->traveltimes[clusterareanum];
            if (!t) {
                continue;
            }
            t += curupdate->tmptraveltime;
            //
            if (!portalcache->traveltimes[portalnum] || portalcache->traveltimes[portalnum] > t) {
                portalcache->traveltimes[portalnum] = t;
                nextupdate = &aasworld.portalupdate[portalnum];
                if (portal->frontcluster == curupdate->cluster) {
                    nextupdate->cluster = portal->backcluster;
                } else {
                    nextupdate->cluster = portal->frontcluster;
                }
                nextupdate->areanum = portal->areanum;
                // add travel time through the actual portal area for the next update
                nextupdate->tmptraveltime = t + aasworld.portalmaxtraveltimes[portalnum];
                if (!nextupdate->inlist) {
                    // we add the update to the end of the list
                    // we could also use a B+ tree to have a real sorted list
                    // on travel time which makes for faster routing updates
                    nextupdate->next = NULL;
                    nextupdate->prev = updatelistend;
                    if (updatelistend) {
                        updatelistend->next = nextupdate;
                    } else {
                        updateliststart = nextupdate;
                    }
                    updatelistend = nextupdate;
                    nextupdate->inlist = true;
                }
            }
        }
    }
}

aas_routingcache_t* AAS_GetPortalRoutingCache(int32 clusternum, int32 areanum, int32 travelflags)
{
    aas_routingcache_t* cache;

    // find the cached portal routing if existing
    for (cache = aasworld.portalcache[areanum]; cache; cache = cache->next) {
        if (cache->travelflags == travelflags) {
            break;
        }
    }
    // if the portal routing isn't cached
    if (!cache) {
        cache = AAS_AllocRoutingCache(aasworld.numportals);
        cache->cluster = clusternum;
        cache->areanum = areanum;
        VectorCopy(aasworld.areas[areanum].center, cache->origin);
        cache->starttraveltime = 1;
        cache->travelflags = travelflags;
        // add the cache to the cache list
        cache->prev = NULL;
        cache->next = aasworld.portalcache[areanum];
        if (aasworld.portalcache[areanum]) {
            aasworld.portalcache[areanum]->prev = cache;
        }
        aasworld.portalcache[areanum] = cache;
        // update the cache
        AAS_UpdatePortalRoutingCache(cache);
    } else {
        AAS_UnlinkCache(cache);
    }
    // the cache has been accessed
    cache->time = AAS_RoutingTime();
    cache->type = CACHETYPE_PORTAL;
    AAS_LinkCache(cache);
    return cache;
}

int32 AAS_AreaRouteToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32* traveltime, int32* reachnum)
{
    int32               clusternum, goalclusternum, portalnum, i, clusterareanum, bestreachnum;
    uint16              t, besttime;
    aas_portal_t*       portal;
    aas_cluster_t*      cluster;
    aas_routingcache_t *areacache, *portalcache;
    aas_reachability_t* reach;

    if (!aasworld.initialized) {
        return false;
    }

    if (areanum == goalareanum) {
        *traveltime = 1;
        *reachnum = 0;
        return true;
    }
    //
    if (areanum <= 0 || areanum >= aasworld.numareas) {
        if (bot_developer) {
            botimport.Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum);
        }
        return false;
    }
    if (goalareanum <= 0 || goalareanum >= aasworld.numareas) {
        if (bot_developer) {
            botimport.Print(PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", goalareanum);
        }
        return false;
    }
    //
    if (AAS_AreaDoNotEnter(areanum) || AAS_AreaDoNotEnter(goalareanum)) {
        travelflags |= TFL_DONOTENTER;
    }
    // NOTE: the number of routing updates is limited per frame

    clusternum = aasworld.areasettings[areanum].cluster;
    goalclusternum = aasworld.areasettings[goalareanum].cluster;
    // check if the area is a portal of the goal area cluster
    if (clusternum < 0 && goalclusternum > 0) {
        portal = &aasworld.portals[-clusternum];
        if (portal->frontcluster == goalclusternum || portal->backcluster == goalclusternum) {
            clusternum = goalclusternum;
        }
    }
    // check if the goalarea is a portal of the area cluster
    else if (clusternum > 0 && goalclusternum < 0) {
        portal = &aasworld.portals[-goalclusternum];
        if (portal->frontcluster == clusternum || portal->backcluster == clusternum) {
            goalclusternum = clusternum;
        }
    }
    // if both areas are in the same cluster
    // NOTE: there might be a shorter route via another cluster!!! but we don't care
    if (clusternum > 0 && goalclusternum > 0 && clusternum == goalclusternum) {
        //
        areacache = AAS_GetAreaRoutingCache(clusternum, goalareanum, travelflags);
        // the number of the area in the cluster
        clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
        // the cluster the area is in
        cluster = &aasworld.clusters[clusternum];
        // if the area is NOT a reachability area
        if (clusterareanum >= cluster->numreachabilityareas) {
            return 0;
        }
        // if it is possible to travel to the goal area through this cluster
        if (areacache->traveltimes[clusterareanum] != 0) {
            *reachnum = aasworld.areasettings[areanum].firstreachablearea + areacache->reachabilities[clusterareanum];
            if (!origin) {
                *traveltime = areacache->traveltimes[clusterareanum];
                return true;
            }
            reach = &aasworld.reachability[*reachnum];
            *traveltime = areacache->traveltimes[clusterareanum] + AAS_AreaTravelTime(areanum, origin, reach->start);
            //
            return true;
        }
    }
    //
    clusternum = aasworld.areasettings[areanum].cluster;
    goalclusternum = aasworld.areasettings[goalareanum].cluster;
    // if the goal area is a portal
    if (goalclusternum < 0) {
        // just assume the goal area is part of the front cluster
        portal = &aasworld.portals[-goalclusternum];
        goalclusternum = portal->frontcluster;
    }
    // get the portal routing cache
    portalcache = AAS_GetPortalRoutingCache(goalclusternum, goalareanum, travelflags);
    // if the area is a cluster portal, read directly from the portal cache
    if (clusternum < 0) {
        *traveltime = portalcache->traveltimes[-clusternum];
        *reachnum = aasworld.areasettings[areanum].firstreachablearea + portalcache->reachabilities[-clusternum];
        return true;
    }
    //
    besttime = 0;
    bestreachnum = -1;
    // the cluster the area is in
    cluster = &aasworld.clusters[clusternum];
    // find the portal of the area cluster leading towards the goal area
    for (i = 0; i < cluster->numportals; i++) {
        portalnum = aasworld.portalindex[cluster->firstportal + i];
        // if the goal area isn't reachable from the portal
        if (!portalcache->traveltimes[portalnum]) {
            continue;
        }
        //
        portal = &aasworld.portals[portalnum];
        // get the cache of the portal area
        areacache = AAS_GetAreaRoutingCache(clusternum, portal->areanum, travelflags);
        // current area inside the current cluster
        clusterareanum = AAS_ClusterAreaNum(clusternum, areanum);
        // if the area is NOT a reachability area
        if (clusterareanum >= cluster->numreachabilityareas) {
            continue;
        }
        // if the portal is NOT reachable from this area
        if (!areacache->traveltimes[clusterareanum]) {
            continue;
        }
        // total travel time is the travel time the portal area is from
        // the goal area plus the travel time towards the portal area
        t = portalcache->traveltimes[portalnum] + areacache->traveltimes[clusterareanum];
        // FIXME: add the exact travel time through the actual portal area
        // NOTE: for now we just add the largest travel time through the portal area
        //         because we can't directly calculate the exact travel time
        //         to be more specific we don't know which reachability was used to travel
        //         into the portal area
        t += aasworld.portalmaxtraveltimes[portalnum];
        //
        if (origin) {
            *reachnum = aasworld.areasettings[areanum].firstreachablearea + areacache->reachabilities[clusterareanum];
            reach = aasworld.reachability + *reachnum;
            t += AAS_AreaTravelTime(areanum, origin, reach->start);
        }
        // if the time is better than the one already found
        if (!besttime || t < besttime) {
            bestreachnum = *reachnum;
            besttime = t;
        }
    }
    if (bestreachnum < 0) {
        return false;
    }
    *reachnum = bestreachnum;
    *traveltime = besttime;
    return true;
}

int32 AAS_AreaTravelTimeToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags)
{
    int32 traveltime, reachnum;

    if (AAS_AreaRouteToGoalArea(areanum, origin, goalareanum, travelflags, &traveltime, &reachnum)) {
        return traveltime;
    }
    return 0;
}

int32 AAS_AreaReachabilityToGoalArea(int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags)
{
    int32 traveltime, reachnum;

    if (AAS_AreaRouteToGoalArea(areanum, origin, goalareanum, travelflags, &traveltime, &reachnum)) {
        return reachnum;
    }
    return 0;
}

// predict the route and stop on one of the stop events
int32 AAS_PredictRoute(struct aas_predictroute_s* route, int32 areanum, vec3_t origin, int32 goalareanum, int32 travelflags, int32 maxareas, int32 maxtime, int32 stopevent, int32 stopcontents, int32 stoptfl, int32 stopareanum)
{
    int32                    curareanum, reachnum, i, j, testareanum;
    vec3_t                   curorigin;
    aas_reachability_t*      reach;
    aas_reachabilityareas_t* reachareas;

    // init output
    route->stopevent = RSE_NONE;
    route->endarea = goalareanum;
    route->endcontents = 0;
    route->endtravelflags = 0;
    VectorCopy(origin, route->endpos);
    route->time = 0;

    curareanum = areanum;
    VectorCopy(origin, curorigin);

    for (i = 0; curareanum != goalareanum && (!maxareas || i < maxareas) && i < aasworld.numareas; i++) {
        reachnum = AAS_AreaReachabilityToGoalArea(curareanum, curorigin, goalareanum, travelflags);
        if (!reachnum) {
            route->stopevent = RSE_NOROUTE;
            return false;
        }
        reach = &aasworld.reachability[reachnum];
        //
        if (stopevent & RSE_USETRAVELTYPE) {
            if (AAS_TravelFlagForType_inline(reach->traveltype) & stoptfl) {
                route->stopevent = RSE_USETRAVELTYPE;
                route->endarea = curareanum;
                route->endcontents = aasworld.areasettings[curareanum].contents;
                route->endtravelflags = AAS_TravelFlagForType_inline(reach->traveltype);
                VectorCopy(reach->start, route->endpos);
                return true;
            }
            if (AAS_AreaContentsTravelFlags_inline(reach->areanum) & stoptfl) {
                route->stopevent = RSE_USETRAVELTYPE;
                route->endarea = reach->areanum;
                route->endcontents = aasworld.areasettings[reach->areanum].contents;
                route->endtravelflags = AAS_AreaContentsTravelFlags_inline(reach->areanum);
                VectorCopy(reach->end, route->endpos);
                route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
                route->time += reach->traveltime;
                return true;
            }
        }
        reachareas = &aasworld.reachabilityareas[reachnum];
        for (j = 0; j < reachareas->numareas + 1; j++) {
            if (j >= reachareas->numareas) {
                testareanum = reach->areanum;
            } else {
                testareanum = aasworld.reachabilityareaindex[reachareas->firstarea + j];
            }
            if (stopevent & RSE_ENTERCONTENTS) {
                if (aasworld.areasettings[testareanum].contents & stopcontents) {
                    route->stopevent = RSE_ENTERCONTENTS;
                    route->endarea = testareanum;
                    route->endcontents = aasworld.areasettings[testareanum].contents;
                    VectorCopy(reach->end, route->endpos);
                    route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
                    route->time += reach->traveltime;
                    return true;
                }
            }
            if (stopevent & RSE_ENTERAREA) {
                if (testareanum == stopareanum) {
                    route->stopevent = RSE_ENTERAREA;
                    route->endarea = testareanum;
                    route->endcontents = aasworld.areasettings[testareanum].contents;
                    VectorCopy(reach->start, route->endpos);
                    return true;
                }
            }
        }

        route->time += AAS_AreaTravelTime(areanum, origin, reach->start);
        route->time += reach->traveltime;
        route->endarea = reach->areanum;
        route->endcontents = aasworld.areasettings[reach->areanum].contents;
        route->endtravelflags = AAS_TravelFlagForType_inline(reach->traveltype);
        VectorCopy(reach->end, route->endpos);
        //
        curareanum = reach->areanum;
        VectorCopy(reach->end, curorigin);
        //
        if (maxtime && route->time > maxtime) {
            break;
        }
    }
    if (curareanum != goalareanum) {
        return false;
    }
    return true;
}

void AAS_ReachabilityFromNum(int32 num, struct aas_reachability_s* reach)
{
    if (!aasworld.initialized) {
        Com_Memset(reach, 0, sizeof(aas_reachability_t));
        return;
    }
    if (num < 0 || num >= aasworld.reachabilitysize) {
        Com_Memset(reach, 0, sizeof(aas_reachability_t));
        return;
    }
    Com_Memcpy(reach, &aasworld.reachability[num], sizeof(aas_reachability_t));
    ;
}

int32 AAS_NextAreaReachability(int32 areanum, int32 reachnum)
{
    aas_areasettings_t* settings;

    if (!aasworld.initialized) {
        return 0;
    }

    if (areanum <= 0 || areanum >= aasworld.numareas) {
        botimport.Print(PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum);
        return 0;
    }

    settings = &aasworld.areasettings[areanum];
    if (!reachnum) {
        return settings->firstreachablearea;
    }
    if (reachnum < settings->firstreachablearea) {
        botimport.Print(PRT_FATAL, "AAS_NextAreaReachability: reachnum < settings->firstreachableara");
        return 0;
    }
    reachnum++;
    if (reachnum >= settings->firstreachablearea + settings->numreachableareas) {
        return 0;
    }
    return reachnum;
}

int32 AAS_NextModelReachability(int32 num, int32 modelnum)
{
    int32 i;

    if (num <= 0) {
        num = 1;
    } else if (num >= aasworld.reachabilitysize) {
        return 0;
    } else {
        num++;
    }
    //
    for (i = num; i < aasworld.reachabilitysize; i++) {
        if ((aasworld.reachability[i].traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR) {
            if (aasworld.reachability[i].facenum == modelnum) {
                return i;
            }
        } else if ((aasworld.reachability[i].traveltype & TRAVELTYPE_MASK) == TRAVEL_FUNCBOB) {
            if ((aasworld.reachability[i].facenum & 0x0000FFFF) == modelnum) {
                return i;
            }
        }
    }
    return 0;
}
