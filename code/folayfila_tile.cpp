// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_tile.h"
#include "folayfila.h"

inline void CannonicalizeCoord(tile_map* TileMap, uint32* Tile, float* TileRel)
{
    int32 offset = FloorFloatToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += offset;
    *TileRel -= offset * TileMap->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel <= TileMap->TileSideInMeters);
}

inline tile_map_position RecanonicalizePosition(tile_map* TileMap, tile_map_position Pos)
{
    tile_map_position result = Pos;

    CannonicalizeCoord(TileMap, &result.AbsTileX, &result.Offset.X);
    CannonicalizeCoord(TileMap, &result.AbsTileY, &result.Offset.Y);

    return result;
}

inline tile_chunk_position GetChunckPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position result;

    result.TilChunkX = AbsTileX >> TileMap->ChunkShift;
    result.TilChunkY = AbsTileY >> TileMap->ChunkShift;
    result.TilChunkZ = AbsTileZ;
    result.TileRelX = AbsTileX & TileMap->ChunkMask;
    result.TileRelY = AbsTileY & TileMap->ChunkMask;

    return result;
}

inline tile_chunk* GetTileChunk(tile_map* TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
    tile_chunk* tileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) &&
        (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
    {
        tileChunk = &TileMap->TileChunks[
            TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX +
            TileChunkY * TileMap->TileChunkCountX +
            TileChunkX];
    }
    return tileChunk;
}

internal void SetTileValue(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
    tile_chunk_position chunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk* tileChunk = GetTileChunk(TileMap, chunkPos.TilChunkX, chunkPos.TilChunkY, chunkPos.TilChunkZ);

    if (tileChunk)
    {
        // Sparse init the chunk here if it wasn't created.
        if (!tileChunk->Tiles)
        {
            uint32 tileCount = TileMap->ChunkDim * TileMap->ChunkDim;
            tileChunk->Tiles= PushArray(Arena, tileCount, uint32);
            for (uint32 tileIndex = 0; tileIndex < tileCount; ++tileIndex)
            {
                tileChunk->Tiles[tileIndex] = tile_type::Dirt;
            }
        }

        // Set the tile value
        uint32 chunkRelTileX = chunkPos.TileRelX;
        uint32 chunkRelTileY = chunkPos.TileRelY;
        Assert(chunkRelTileX < TileMap->ChunkDim);
        Assert(chunkRelTileY < TileMap->ChunkDim);

        tileChunk->Tiles[chunkRelTileY * TileMap->ChunkDim + chunkRelTileX] = TileValue;
    }
}

internal uint32 GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    uint32 tileValue = 0;
    tile_chunk_position chunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk* tileChunk = GetTileChunk(TileMap, chunkPos.TilChunkX, chunkPos.TilChunkY, chunkPos.TilChunkZ);

    if (tileChunk && tileChunk->Tiles)
    {
        uint32 chunkRelTileX = chunkPos.TileRelX;
        uint32 chunkRelTileY = chunkPos.TileRelY;
        Assert(chunkRelTileX < TileMap->ChunkDim);
        Assert(chunkRelTileY < TileMap->ChunkDim);

        tileValue = tileChunk->Tiles[chunkRelTileY * TileMap->ChunkDim + chunkRelTileX];
    }

    return tileValue;
}

internal uint32 GetTileValue(tile_map* TileMap, tile_map_position* Pos)
{
    uint32 tileValue = GetTileValue(TileMap, Pos->AbsTileX, Pos->AbsTileY, Pos->AbsTileZ);
    return tileValue;
}

internal bool32 IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
    uint32 tileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);
    bool32 empty = ((tileChunkValue == tile_type::Dirt) ||
                    (tileChunkValue == tile_type::CaveEntrance) ||
                    (tileChunkValue == tile_type::CaveExit));
    return empty;
}

internal bool32 AreOnSameTiles(tile_map_position *A, tile_map_position *B)
{
    bool32 areOnSameTile = ((A->AbsTileX == B->AbsTileX) &&
                            (A->AbsTileY == B->AbsTileY) &&
                            (A->AbsTileZ == B->AbsTileZ));
    return areOnSameTile;
}

internal tile_map_difference Subtract(tile_map *TileMap, tile_map_position* A, tile_map_position* B)
{
    tile_map_difference result = {};

    vec2 dTileXY = { (float)A->AbsTileX - (float)B->AbsTileX, (float)A->AbsTileY - (float)B->AbsTileY };
    dTileXY *= TileMap->TileSideInMeters;
    float dTileZ = TileMap->TileSideInMeters*((float)A->AbsTileZ - (float)B->AbsTileZ);

    result.dXY = TileMap->TileSideInMeters* dTileXY + (A->Offset - B->Offset);
    result.dZ = TileMap->TileSideInMeters* dTileZ;

    return result;
}