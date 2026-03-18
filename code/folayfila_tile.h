// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_TILE_H)
#define FOLAYFILA_TILE_H

#include "folayfila_types.h"

struct tile_map_position
{
    // Fixed point tile locations.
    // The high bits are the tile chunk index, and the low bits are the 
    // tile index in the chunk.
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;

    float TileRelX;
    float TileRelY;
};

struct tile_chunk_position
{
    uint32 TilChunkX;
    uint32 TilChunkY;
    uint32 TilChunkZ;

    uint32 RelTileX;
    uint32 RelTileY;
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

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;

    tile_chunk* TileChunks;
};

#endif // FOLAYFILA_TILE_H
