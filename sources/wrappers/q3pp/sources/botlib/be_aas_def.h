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

#include <tavros/core/prelude.hpp>
#include <game/q_shared.h>
#include <game/be_aas.h>
#include <botlib/aasfile.h>

// debugging on
#define AAS_DEBUG

#define MAX_CLIENTS        64
#define MAX_MODELS         256 // these are sent over the net as 8 bits
#define MAX_SOUNDS         256 // so they cannot be blindly increased
#define MAX_CONFIGSTRINGS  1024

#define CS_SCORES          32
#define CS_MODELS          (CS_SCORES + MAX_CLIENTS)
#define CS_SOUNDS          (CS_MODELS + MAX_MODELS)

#define DF_AASENTNUMBER(x) (x - aasworld.entities)
#define DF_NUMBERAASENT(x) (&aasworld.entities[x])
#define DF_AASENTCLIENT(x) (x - aasworld.entities - 1)
#define DF_CLIENTAASENT(x) (&aasworld.entities[x + 1])

#ifndef MAX_PATH
    #define MAX_PATH MAX_QPATH
#endif

// string index (for model, sound and image index)
typedef struct aas_stringindex_s
{
    int32  numindexes;
    char** index;
} aas_stringindex_t;

// structure to link entities to areas and areas to entities
typedef struct aas_link_s
{
    int32              entnum;
    int32              areanum;
    struct aas_link_s *next_ent, *prev_ent;
    struct aas_link_s *next_area, *prev_area;
} aas_link_t;

// structure to link entities to leaves and leaves to entities
typedef struct bsp_link_s
{
    int32              entnum;
    int32              leafnum;
    struct bsp_link_s *next_ent, *prev_ent;
    struct bsp_link_s *next_leaf, *prev_leaf;
} bsp_link_t;

typedef struct bsp_entdata_s
{
    vec3_t origin;
    vec3_t angles;
    vec3_t absmins;
    vec3_t absmaxs;
    int32  solid;
    int32  modelnum;
} bsp_entdata_t;

// entity
typedef struct aas_entity_s
{
    // entity info
    aas_entityinfo_t i;
    // links into the AAS areas
    aas_link_t* areas;
    // links into the BSP leaves
    bsp_link_t* leaves;
} aas_entity_t;

typedef struct aas_settings_s
{
    vec3_t phys_gravitydirection;
    float  phys_friction;
    float  phys_stopspeed;
    float  phys_gravity;
    float  phys_waterfriction;
    float  phys_watergravity;
    float  phys_maxvelocity;
    float  phys_maxwalkvelocity;
    float  phys_maxcrouchvelocity;
    float  phys_maxswimvelocity;
    float  phys_walkaccelerate;
    float  phys_airaccelerate;
    float  phys_swimaccelerate;
    float  phys_maxstep;
    float  phys_maxsteepness;
    float  phys_maxwaterjump;
    float  phys_maxbarrier;
    float  phys_jumpvel;
    float  phys_falldelta5;
    float  phys_falldelta10;
    float  rs_waterjump;
    float  rs_teleport;
    float  rs_barrierjump;
    float  rs_startcrouch;
    float  rs_startgrapple;
    float  rs_startwalkoffledge;
    float  rs_startjump;
    float  rs_rocketjump;
    float  rs_bfgjump;
    float  rs_jumppad;
    float  rs_aircontrolledjumppad;
    float  rs_funcbob;
    float  rs_startelevator;
    float  rs_falldamage5;
    float  rs_falldamage10;
    float  rs_maxfallheight;
    float  rs_maxjumpfallheight;
} aas_settings_t;

#define CACHETYPE_PORTAL 0
#define CACHETYPE_AREA   1

// routing cache
typedef struct aas_routingcache_s
{
    uint8                      type;            // portal or area cache
    float                      time;            // last time accessed or updated
    int32                      size;            // size of the routing cache
    int32                      cluster;         // cluster the cache is for
    int32                      areanum;         // area the cache is created for
    vec3_t                     origin;          // origin within the area
    float                      starttraveltime; // travel time to start with
    int32                      travelflags;     // combinations of the travel flags
    struct aas_routingcache_s *prev, *next;
    struct aas_routingcache_s *time_prev, *time_next;
    uint8*                     reachabilities; // reachabilities used for routing
    uint16                     traveltimes[1]; // travel time for every area (variable sized)
} aas_routingcache_t;

// fields for the routing algorithm
typedef struct aas_routingupdate_s
{
    int32                       cluster;
    int32                       areanum;         // area number of the update
    vec3_t                      start;           // start point the area was entered
    uint16                      tmptraveltime;   // temporary travel time
    uint16*                     areatraveltimes; // travel times within the area
    bool                        inlist;          // true if the update is in the list
    struct aas_routingupdate_s* next;
    struct aas_routingupdate_s* prev;
} aas_routingupdate_t;

// reversed reachability link
typedef struct aas_reversedlink_s
{
    int32                      linknum; // the aas_areareachability_t
    int32                      areanum; // reachable from this area
    struct aas_reversedlink_s* next;    // next link
} aas_reversedlink_t;

// reversed area reachability
typedef struct aas_reversedreachability_s
{
    int32               numlinks;
    aas_reversedlink_t* first;
} aas_reversedreachability_t;

// areas a reachability goes through
typedef struct aas_reachabilityareas_s
{
    int32 firstarea, numareas;
} aas_reachabilityareas_t;

typedef struct aas_s
{
    int32 loaded;      // true when an AAS file is loaded
    int32 initialized; // true when AAS has been initialized
    int32 savefile;    // set true when file should be saved
    int32 bspchecksum;
    // current time
    float time;
    int32 numframes;
    // name of the aas file
    char filename[MAX_PATH];
    char mapname[MAX_PATH];
    // bounding boxes
    int32       numbboxes;
    aas_bbox_t* bboxes;
    // vertexes
    int32         numvertexes;
    aas_vertex_t* vertexes;
    // planes
    int32        numplanes;
    aas_plane_t* planes;
    // edges
    int32       numedges;
    aas_edge_t* edges;
    // edge index
    int32            edgeindexsize;
    aas_edgeindex_t* edgeindex;
    // faces
    int32       numfaces;
    aas_face_t* faces;
    // face index
    int32            faceindexsize;
    aas_faceindex_t* faceindex;
    // convex areas
    int32       numareas;
    aas_area_t* areas;
    // convex area settings
    int32               numareasettings;
    aas_areasettings_t* areasettings;
    // reachablity list
    int32               reachabilitysize;
    aas_reachability_t* reachability;
    // nodes of the bsp tree
    int32       numnodes;
    aas_node_t* nodes;
    // cluster portals
    int32         numportals;
    aas_portal_t* portals;
    // cluster portal index
    int32              portalindexsize;
    aas_portalindex_t* portalindex;
    // clusters
    int32          numclusters;
    aas_cluster_t* clusters;
    //
    int32 numreachabilityareas;
    float reachabilitytime;
    // enities linked in the areas
    aas_link_t*  linkheap;           // heap with link structures
    int32        linkheapsize;       // size of the link heap
    aas_link_t*  freelinks;          // first free link
    aas_link_t** arealinkedentities; // entities linked into areas
    // entities
    int32         maxentities;
    int32         maxclients;
    aas_entity_t* entities;
    // string indexes
    char* configstrings[MAX_CONFIGSTRINGS];
    int32 indexessetup;
    // index to retrieve travel flag for a travel type
    int32 travelflagfortype[MAX_TRAVELTYPES];
    // travel flags for each area based on contents
    int32* areacontentstravelflags;
    // routing update
    aas_routingupdate_t* areaupdate;
    aas_routingupdate_t* portalupdate;
    // number of routing updates during a frame (reset every frame)
    int32 frameroutingupdates;
    // reversed reachability links
    aas_reversedreachability_t* reversedreachability;
    // travel times within the areas
    uint16*** areatraveltimes;
    // array of size numclusters with cluster cache
    aas_routingcache_t*** clusterareacache;
    aas_routingcache_t**  portalcache;
    // cache list sorted on time
    aas_routingcache_t* oldestcache; // start of cache list sorted on time
    aas_routingcache_t* newestcache; // end of cache list sorted on time
    // maximum travel time through portal areas
    int32* portalmaxtraveltimes;
    // areas the reachabilities go through
    int32*                   reachabilityareaindex;
    aas_reachabilityareas_t* reachabilityareas;
} aas_t;

#define AASINTERN

#include "be_aas_funcs.h"
