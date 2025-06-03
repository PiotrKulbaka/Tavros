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
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

// #define AASFILEDEBUG

void AAS_SwapAASData()
{
    int32 i, j;
    // bounding boxes
    for (i = 0; i < aasworld.numbboxes; i++) {
        aasworld.bboxes[i].presencetype = (aasworld.bboxes[i].presencetype);
        aasworld.bboxes[i].flags = (aasworld.bboxes[i].flags);
        for (j = 0; j < 3; j++) {
            aasworld.bboxes[i].mins[j] = (aasworld.bboxes[i].mins[j]);
            aasworld.bboxes[i].maxs[j] = (aasworld.bboxes[i].maxs[j]);
        }
    }
    // vertexes
    for (i = 0; i < aasworld.numvertexes; i++) {
        for (j = 0; j < 3; j++) {
            aasworld.vertexes[i][j] = (aasworld.vertexes[i][j]);
        }
    }
    // planes
    for (i = 0; i < aasworld.numplanes; i++) {
        for (j = 0; j < 3; j++) {
            aasworld.planes[i].normal[j] = (aasworld.planes[i].normal[j]);
        }
        aasworld.planes[i].dist = (aasworld.planes[i].dist);
        aasworld.planes[i].type = (aasworld.planes[i].type);
    }
    // edges
    for (i = 0; i < aasworld.numedges; i++) {
        aasworld.edges[i].v[0] = (aasworld.edges[i].v[0]);
        aasworld.edges[i].v[1] = (aasworld.edges[i].v[1]);
    }
    // edgeindex
    for (i = 0; i < aasworld.edgeindexsize; i++) {
        aasworld.edgeindex[i] = (aasworld.edgeindex[i]);
    }
    // faces
    for (i = 0; i < aasworld.numfaces; i++) {
        aasworld.faces[i].planenum = (aasworld.faces[i].planenum);
        aasworld.faces[i].faceflags = (aasworld.faces[i].faceflags);
        aasworld.faces[i].numedges = (aasworld.faces[i].numedges);
        aasworld.faces[i].firstedge = (aasworld.faces[i].firstedge);
        aasworld.faces[i].frontarea = (aasworld.faces[i].frontarea);
        aasworld.faces[i].backarea = (aasworld.faces[i].backarea);
    }
    // face index
    for (i = 0; i < aasworld.faceindexsize; i++) {
        aasworld.faceindex[i] = (aasworld.faceindex[i]);
    }
    // convex areas
    for (i = 0; i < aasworld.numareas; i++) {
        aasworld.areas[i].areanum = (aasworld.areas[i].areanum);
        aasworld.areas[i].numfaces = (aasworld.areas[i].numfaces);
        aasworld.areas[i].firstface = (aasworld.areas[i].firstface);
        for (j = 0; j < 3; j++) {
            aasworld.areas[i].mins[j] = (aasworld.areas[i].mins[j]);
            aasworld.areas[i].maxs[j] = (aasworld.areas[i].maxs[j]);
            aasworld.areas[i].center[j] = (aasworld.areas[i].center[j]);
        }
    }
    // area settings
    for (i = 0; i < aasworld.numareasettings; i++) {
        aasworld.areasettings[i].contents = (aasworld.areasettings[i].contents);
        aasworld.areasettings[i].areaflags = (aasworld.areasettings[i].areaflags);
        aasworld.areasettings[i].presencetype = (aasworld.areasettings[i].presencetype);
        aasworld.areasettings[i].cluster = (aasworld.areasettings[i].cluster);
        aasworld.areasettings[i].clusterareanum = (aasworld.areasettings[i].clusterareanum);
        aasworld.areasettings[i].numreachableareas = (aasworld.areasettings[i].numreachableareas);
        aasworld.areasettings[i].firstreachablearea = (aasworld.areasettings[i].firstreachablearea);
    }
    // area reachability
    for (i = 0; i < aasworld.reachabilitysize; i++) {
        aasworld.reachability[i].areanum = (aasworld.reachability[i].areanum);
        aasworld.reachability[i].facenum = (aasworld.reachability[i].facenum);
        aasworld.reachability[i].edgenum = (aasworld.reachability[i].edgenum);
        for (j = 0; j < 3; j++) {
            aasworld.reachability[i].start[j] = (aasworld.reachability[i].start[j]);
            aasworld.reachability[i].end[j] = (aasworld.reachability[i].end[j]);
        }
        aasworld.reachability[i].traveltype = (aasworld.reachability[i].traveltype);
        aasworld.reachability[i].traveltime = (aasworld.reachability[i].traveltime);
    }
    // nodes
    for (i = 0; i < aasworld.numnodes; i++) {
        aasworld.nodes[i].planenum = (aasworld.nodes[i].planenum);
        aasworld.nodes[i].children[0] = (aasworld.nodes[i].children[0]);
        aasworld.nodes[i].children[1] = (aasworld.nodes[i].children[1]);
    }
    // cluster portals
    for (i = 0; i < aasworld.numportals; i++) {
        aasworld.portals[i].areanum = (aasworld.portals[i].areanum);
        aasworld.portals[i].frontcluster = (aasworld.portals[i].frontcluster);
        aasworld.portals[i].backcluster = (aasworld.portals[i].backcluster);
        aasworld.portals[i].clusterareanum[0] = (aasworld.portals[i].clusterareanum[0]);
        aasworld.portals[i].clusterareanum[1] = (aasworld.portals[i].clusterareanum[1]);
    }
    // cluster portal index
    for (i = 0; i < aasworld.portalindexsize; i++) {
        aasworld.portalindex[i] = (aasworld.portalindex[i]);
    }
    // cluster
    for (i = 0; i < aasworld.numclusters; i++) {
        aasworld.clusters[i].numareas = (aasworld.clusters[i].numareas);
        aasworld.clusters[i].numreachabilityareas = (aasworld.clusters[i].numreachabilityareas);
        aasworld.clusters[i].numportals = (aasworld.clusters[i].numportals);
        aasworld.clusters[i].firstportal = (aasworld.clusters[i].firstportal);
    }
}

// dump the current loaded aas file
void AAS_DumpAASData()
{
    aasworld.numbboxes = 0;
    if (aasworld.bboxes) {
        FreeMemory(aasworld.bboxes);
    }
    aasworld.bboxes = NULL;
    aasworld.numvertexes = 0;
    if (aasworld.vertexes) {
        FreeMemory(aasworld.vertexes);
    }
    aasworld.vertexes = NULL;
    aasworld.numplanes = 0;
    if (aasworld.planes) {
        FreeMemory(aasworld.planes);
    }
    aasworld.planes = NULL;
    aasworld.numedges = 0;
    if (aasworld.edges) {
        FreeMemory(aasworld.edges);
    }
    aasworld.edges = NULL;
    aasworld.edgeindexsize = 0;
    if (aasworld.edgeindex) {
        FreeMemory(aasworld.edgeindex);
    }
    aasworld.edgeindex = NULL;
    aasworld.numfaces = 0;
    if (aasworld.faces) {
        FreeMemory(aasworld.faces);
    }
    aasworld.faces = NULL;
    aasworld.faceindexsize = 0;
    if (aasworld.faceindex) {
        FreeMemory(aasworld.faceindex);
    }
    aasworld.faceindex = NULL;
    aasworld.numareas = 0;
    if (aasworld.areas) {
        FreeMemory(aasworld.areas);
    }
    aasworld.areas = NULL;
    aasworld.numareasettings = 0;
    if (aasworld.areasettings) {
        FreeMemory(aasworld.areasettings);
    }
    aasworld.areasettings = NULL;
    aasworld.reachabilitysize = 0;
    if (aasworld.reachability) {
        FreeMemory(aasworld.reachability);
    }
    aasworld.reachability = NULL;
    aasworld.numnodes = 0;
    if (aasworld.nodes) {
        FreeMemory(aasworld.nodes);
    }
    aasworld.nodes = NULL;
    aasworld.numportals = 0;
    if (aasworld.portals) {
        FreeMemory(aasworld.portals);
    }
    aasworld.portals = NULL;
    aasworld.numportals = 0;
    if (aasworld.portalindex) {
        FreeMemory(aasworld.portalindex);
    }
    aasworld.portalindex = NULL;
    aasworld.portalindexsize = 0;
    if (aasworld.clusters) {
        FreeMemory(aasworld.clusters);
    }
    aasworld.clusters = NULL;
    aasworld.numclusters = 0;
    //
    aasworld.loaded = false;
    aasworld.initialized = false;
    aasworld.savefile = false;
}

#ifdef AASFILEDEBUG
void AAS_FileInfo()
{
    int32 i, n, optimized;

    botimport.Print(PRT_MESSAGE, "version = %d\n", AASVERSION);
    botimport.Print(PRT_MESSAGE, "numvertexes = %d\n", aasworld.numvertexes);
    botimport.Print(PRT_MESSAGE, "numplanes = %d\n", aasworld.numplanes);
    botimport.Print(PRT_MESSAGE, "numedges = %d\n", aasworld.numedges);
    botimport.Print(PRT_MESSAGE, "edgeindexsize = %d\n", aasworld.edgeindexsize);
    botimport.Print(PRT_MESSAGE, "numfaces = %d\n", aasworld.numfaces);
    botimport.Print(PRT_MESSAGE, "faceindexsize = %d\n", aasworld.faceindexsize);
    botimport.Print(PRT_MESSAGE, "numareas = %d\n", aasworld.numareas);
    botimport.Print(PRT_MESSAGE, "numareasettings = %d\n", aasworld.numareasettings);
    botimport.Print(PRT_MESSAGE, "reachabilitysize = %d\n", aasworld.reachabilitysize);
    botimport.Print(PRT_MESSAGE, "numnodes = %d\n", aasworld.numnodes);
    botimport.Print(PRT_MESSAGE, "numportals = %d\n", aasworld.numportals);
    botimport.Print(PRT_MESSAGE, "portalindexsize = %d\n", aasworld.portalindexsize);
    botimport.Print(PRT_MESSAGE, "numclusters = %d\n", aasworld.numclusters);
    //
    for (n = 0, i = 0; i < aasworld.numareasettings; i++) {
        if (aasworld.areasettings[i].areaflags & AREA_GROUNDED) {
            n++;
        }
    }
    botimport.Print(PRT_MESSAGE, "num grounded areas = %d\n", n);
    //
    botimport.Print(PRT_MESSAGE, "planes size %d bytes\n", aasworld.numplanes * sizeof(aas_plane_t));
    botimport.Print(PRT_MESSAGE, "areas size %d bytes\n", aasworld.numareas * sizeof(aas_area_t));
    botimport.Print(PRT_MESSAGE, "areasettings size %d bytes\n", aasworld.numareasettings * sizeof(aas_areasettings_t));
    botimport.Print(PRT_MESSAGE, "nodes size %d bytes\n", aasworld.numnodes * sizeof(aas_node_t));
    botimport.Print(PRT_MESSAGE, "reachability size %d bytes\n", aasworld.reachabilitysize * sizeof(aas_reachability_t));
    botimport.Print(PRT_MESSAGE, "portals size %d bytes\n", aasworld.numportals * sizeof(aas_portal_t));
    botimport.Print(PRT_MESSAGE, "clusters size %d bytes\n", aasworld.numclusters * sizeof(aas_cluster_t));

    optimized = aasworld.numplanes * sizeof(aas_plane_t) + aasworld.numareas * sizeof(aas_area_t) + aasworld.numareasettings * sizeof(aas_areasettings_t) + aasworld.numnodes * sizeof(aas_node_t) + aasworld.reachabilitysize * sizeof(aas_reachability_t) + aasworld.numportals * sizeof(aas_portal_t) + aasworld.numclusters * sizeof(aas_cluster_t);
    botimport.Print(PRT_MESSAGE, "optimzed size %d KB\n", optimized >> 10);
}
#endif // AASFILEDEBUG

// allocate memory and read a lump of a AAS file
char* AAS_LoadAASLump(fileHandle_t fp, int32 offset, int32 length, int32* lastoffset, int32 size)
{
    char* buf;
    //
    if (!length) {
        // just alloc a dummy
        return (char*) GetClearedHunkMemory(size + 1);
    }
    // seek to the data
    if (offset != *lastoffset) {
        botimport.Print(PRT_WARNING, "AAS file not sequentially read\n");
        if (botimport.FS_Seek(fp, offset, FS_SEEK_SET)) {
            AAS_Error("can't seek to aas lump\n");
            AAS_DumpAASData();
            botimport.FS_FCloseFile(fp);
            return 0;
        }
    }
    // allocate memory
    buf = (char*) GetClearedHunkMemory(length + 1);
    // read the data
    if (length) {
        botimport.FS_Read(buf, length, fp);
        *lastoffset += length;
    }
    return buf;
}

void AAS_DData(uint8* data, int32 size)
{
    int32 i;

    for (i = 0; i < size; i++) {
        data[i] ^= (uint8) i * 119;
    }
}

// load an aas file
int32 AAS_LoadAASFile(char* filename)
{
    fileHandle_t fp;
    aas_header_t header;
    int32        offset, length, lastoffset;

    botimport.Print(PRT_MESSAGE, "trying to load %s\n", filename);
    // dump current loaded aas file
    AAS_DumpAASData();
    // open the file
    botimport.FS_FOpenFile(filename, &fp, FS_READ);
    if (!fp) {
        AAS_Error("can't open %s\n", filename);
        return BLERR_CANNOTOPENAASFILE;
    }
    // read the header
    botimport.FS_Read(&header, sizeof(aas_header_t), fp);
    lastoffset = sizeof(aas_header_t);
    // check header identification
    header.ident = (header.ident);
    if (header.ident != AASID) {
        AAS_Error("%s is not an AAS file\n", filename);
        botimport.FS_FCloseFile(fp);
        return BLERR_WRONGAASFILEID;
    }
    // check the version
    header.version = (header.version);
    //
    if (header.version != AASVERSION_OLD && header.version != AASVERSION) {
        AAS_Error("aas file %s is version %i, not %i\n", filename, header.version, AASVERSION);
        botimport.FS_FCloseFile(fp);
        return BLERR_WRONGAASFILEVERSION;
    }
    //
    if (header.version == AASVERSION) {
        AAS_DData((uint8*) &header + 8, sizeof(aas_header_t) - 8);
    }
    //
    aasworld.bspchecksum = atoi(LibVarGetString("sv_mapChecksum"));
    if ((header.bspchecksum) != aasworld.bspchecksum) {
        AAS_Error("aas file %s is out of date\n", filename);
        botimport.FS_FCloseFile(fp);
        return BLERR_WRONGAASFILEVERSION;
    }
    // load the lumps:
    // bounding boxes
    offset = (header.lumps[AASLUMP_BBOXES].fileofs);
    length = (header.lumps[AASLUMP_BBOXES].filelen);
    aasworld.bboxes = (aas_bbox_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_bbox_t));
    aasworld.numbboxes = length / sizeof(aas_bbox_t);
    if (aasworld.numbboxes && !aasworld.bboxes) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // vertexes
    offset = (header.lumps[AASLUMP_VERTEXES].fileofs);
    length = (header.lumps[AASLUMP_VERTEXES].filelen);
    aasworld.vertexes = (aas_vertex_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_vertex_t));
    aasworld.numvertexes = length / sizeof(aas_vertex_t);
    if (aasworld.numvertexes && !aasworld.vertexes) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // planes
    offset = (header.lumps[AASLUMP_PLANES].fileofs);
    length = (header.lumps[AASLUMP_PLANES].filelen);
    aasworld.planes = (aas_plane_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_plane_t));
    aasworld.numplanes = length / sizeof(aas_plane_t);
    if (aasworld.numplanes && !aasworld.planes) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // edges
    offset = (header.lumps[AASLUMP_EDGES].fileofs);
    length = (header.lumps[AASLUMP_EDGES].filelen);
    aasworld.edges = (aas_edge_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_edge_t));
    aasworld.numedges = length / sizeof(aas_edge_t);
    if (aasworld.numedges && !aasworld.edges) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // edgeindex
    offset = (header.lumps[AASLUMP_EDGEINDEX].fileofs);
    length = (header.lumps[AASLUMP_EDGEINDEX].filelen);
    aasworld.edgeindex = (aas_edgeindex_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_edgeindex_t));
    aasworld.edgeindexsize = length / sizeof(aas_edgeindex_t);
    if (aasworld.edgeindexsize && !aasworld.edgeindex) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // faces
    offset = (header.lumps[AASLUMP_FACES].fileofs);
    length = (header.lumps[AASLUMP_FACES].filelen);
    aasworld.faces = (aas_face_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_face_t));
    aasworld.numfaces = length / sizeof(aas_face_t);
    if (aasworld.numfaces && !aasworld.faces) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // faceindex
    offset = (header.lumps[AASLUMP_FACEINDEX].fileofs);
    length = (header.lumps[AASLUMP_FACEINDEX].filelen);
    aasworld.faceindex = (aas_faceindex_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_faceindex_t));
    aasworld.faceindexsize = length / sizeof(aas_faceindex_t);
    if (aasworld.faceindexsize && !aasworld.faceindex) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // convex areas
    offset = (header.lumps[AASLUMP_AREAS].fileofs);
    length = (header.lumps[AASLUMP_AREAS].filelen);
    aasworld.areas = (aas_area_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_area_t));
    aasworld.numareas = length / sizeof(aas_area_t);
    if (aasworld.numareas && !aasworld.areas) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // area settings
    offset = (header.lumps[AASLUMP_AREASETTINGS].fileofs);
    length = (header.lumps[AASLUMP_AREASETTINGS].filelen);
    aasworld.areasettings = (aas_areasettings_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_areasettings_t));
    aasworld.numareasettings = length / sizeof(aas_areasettings_t);
    if (aasworld.numareasettings && !aasworld.areasettings) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // reachability list
    offset = (header.lumps[AASLUMP_REACHABILITY].fileofs);
    length = (header.lumps[AASLUMP_REACHABILITY].filelen);
    aasworld.reachability = (aas_reachability_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_reachability_t));
    aasworld.reachabilitysize = length / sizeof(aas_reachability_t);
    if (aasworld.reachabilitysize && !aasworld.reachability) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // nodes
    offset = (header.lumps[AASLUMP_NODES].fileofs);
    length = (header.lumps[AASLUMP_NODES].filelen);
    aasworld.nodes = (aas_node_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_node_t));
    aasworld.numnodes = length / sizeof(aas_node_t);
    if (aasworld.numnodes && !aasworld.nodes) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // cluster portals
    offset = (header.lumps[AASLUMP_PORTALS].fileofs);
    length = (header.lumps[AASLUMP_PORTALS].filelen);
    aasworld.portals = (aas_portal_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_portal_t));
    aasworld.numportals = length / sizeof(aas_portal_t);
    if (aasworld.numportals && !aasworld.portals) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // cluster portal index
    offset = (header.lumps[AASLUMP_PORTALINDEX].fileofs);
    length = (header.lumps[AASLUMP_PORTALINDEX].filelen);
    aasworld.portalindex = (aas_portalindex_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_portalindex_t));
    aasworld.portalindexsize = length / sizeof(aas_portalindex_t);
    if (aasworld.portalindexsize && !aasworld.portalindex) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // clusters
    offset = (header.lumps[AASLUMP_CLUSTERS].fileofs);
    length = (header.lumps[AASLUMP_CLUSTERS].filelen);
    aasworld.clusters = (aas_cluster_t*) AAS_LoadAASLump(fp, offset, length, &lastoffset, sizeof(aas_cluster_t));
    aasworld.numclusters = length / sizeof(aas_cluster_t);
    if (aasworld.numclusters && !aasworld.clusters) {
        return BLERR_CANNOTREADAASLUMP;
    }
    // swap everything
    AAS_SwapAASData();
    // aas file is loaded
    aasworld.loaded = true;
    // close the file
    botimport.FS_FCloseFile(fp);
    //
#ifdef AASFILEDEBUG
    AAS_FileInfo();
#endif // AASFILEDEBUG
    //
    return BLERR_NOERROR;
}

static int32 AAS_WriteAASLump_offset;

int32 AAS_WriteAASLump(fileHandle_t fp, aas_header_t* h, int32 lumpnum, void* data, int32 length)
{
    aas_lump_t* lump;

    lump = &h->lumps[lumpnum];

    lump->fileofs = (AAS_WriteAASLump_offset); // (ftell(fp));
    lump->filelen = (length);

    if (length > 0) {
        botimport.FS_Write(data, length, fp);
    }

    AAS_WriteAASLump_offset += length;

    return true;
}

// aas data is useless after writing to file because it is uint8 swapped
bool AAS_WriteAASFile(char* filename)
{
    aas_header_t header;
    fileHandle_t fp;

    botimport.Print(PRT_MESSAGE, "writing %s\n", filename);
    // swap the aas data
    AAS_SwapAASData();
    // initialize the file header
    Com_Memset(&header, 0, sizeof(aas_header_t));
    header.ident = (AASID);
    header.version = (AASVERSION);
    header.bspchecksum = (aasworld.bspchecksum);
    // open a new file
    botimport.FS_FOpenFile(filename, &fp, FS_WRITE);
    if (!fp) {
        botimport.Print(PRT_ERROR, "error opening %s\n", filename);
        return false;
    }
    // write the header
    botimport.FS_Write(&header, sizeof(aas_header_t), fp);
    AAS_WriteAASLump_offset = sizeof(aas_header_t);
    // add the data lumps to the file
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_BBOXES, aasworld.bboxes, aasworld.numbboxes * sizeof(aas_bbox_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_VERTEXES, aasworld.vertexes, aasworld.numvertexes * sizeof(aas_vertex_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_PLANES, aasworld.planes, aasworld.numplanes * sizeof(aas_plane_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_EDGES, aasworld.edges, aasworld.numedges * sizeof(aas_edge_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_EDGEINDEX, aasworld.edgeindex, aasworld.edgeindexsize * sizeof(aas_edgeindex_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_FACES, aasworld.faces, aasworld.numfaces * sizeof(aas_face_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_FACEINDEX, aasworld.faceindex, aasworld.faceindexsize * sizeof(aas_faceindex_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_AREAS, aasworld.areas, aasworld.numareas * sizeof(aas_area_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_AREASETTINGS, aasworld.areasettings, aasworld.numareasettings * sizeof(aas_areasettings_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_REACHABILITY, aasworld.reachability, aasworld.reachabilitysize * sizeof(aas_reachability_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_NODES, aasworld.nodes, aasworld.numnodes * sizeof(aas_node_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_PORTALS, aasworld.portals, aasworld.numportals * sizeof(aas_portal_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_PORTALINDEX, aasworld.portalindex, aasworld.portalindexsize * sizeof(aas_portalindex_t))) {
        return false;
    }
    if (!AAS_WriteAASLump(fp, &header, AASLUMP_CLUSTERS, aasworld.clusters, aasworld.numclusters * sizeof(aas_cluster_t))) {
        return false;
    }
    // rewrite the header with the added lumps
    botimport.FS_Seek(fp, 0, FS_SEEK_SET);
    AAS_DData((uint8*) &header + 8, sizeof(aas_header_t) - 8);
    botimport.FS_Write(&header, sizeof(aas_header_t), fp);
    // close the file
    botimport.FS_FCloseFile(fp);
    return true;
}
