// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_TILE_H)
#define FOLAYFILA_TILE_H

#include "folayfila_types.h"
#include "folayfila_math.h"

enum tile_type
{
    Dirt  = 1,
    Grass = 2,
    Water = 3,
    CaveEntrance = 4,
    CaveExit = 5,

    None  = 0
};

struct tile_map_difference
{
    vec2 dXY;
    float dZ;
};

struct tile_map_position
{
    // Fixed point tile locations.
    // The high bits are the tile chunk index, and the low bits are the 
    // tile index in the chunk.
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;

    vec2 Offset;
};

struct tile_chunk_position
{
    uint32 TilChunkX;
    uint32 TilChunkY;
    uint32 TilChunkZ;

    uint32 TileRelX;
    uint32 TileRelY;
};

struct tile_chunk
{
    uint32* Tiles;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    float TileSideInMeters;
    int32 TileSideInPixels;

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;

    tile_chunk* TileChunks;

    uint32 TilesPerWidth;
    uint32 TilesPerHeight;
};

#endif // FOLAYFILA_TILE_H
