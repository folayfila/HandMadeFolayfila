// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_tile.h"
#include "folayfila.h"

inline void CannonicalizeCoord(tile_map* TileMap, uint32* Tile, float* TileRel)
{
    int32 Offset = FloorFloatToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset * TileMap->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel <= TileMap->TileSideInMeters);
}

inline tile_map_position RecanonicalizePosition(tile_map* TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    CannonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
    CannonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

inline tile_chunk_position GetChunckPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TilChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TilChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TilChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return Result;
}

inline tile_chunk* GetTileChunk(tile_map* TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
    tile_chunk* TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) &&
        (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
    {
        TileChunk = &TileMap->TileChunks[
            TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX +
            TileChunkY * TileMap->TileChunkCountX +
            TileChunkX];
    }
    return TileChunk;
}

static void SetTileValue(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TilChunkX, ChunkPos.TilChunkY, ChunkPos.TilChunkZ);

    if (TileChunk)
    {
        // Sparse init the chunk here if it wasn't created.
        if (!TileChunk->Tiles)
        {
            uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
            TileChunk->Tiles= PushArray(Arena, TileCount, uint32);
            for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
            {
                TileChunk->Tiles[TileIndex] = 'd';
            }
        }

        // Set the tile value
        uint32 ChunkRelTileX = ChunkPos.RelTileX;
        uint32 ChunkRelTileY = ChunkPos.RelTileY;
        Assert(ChunkRelTileX < TileMap->ChunkDim);
        Assert(ChunkRelTileY < TileMap->ChunkDim);

        TileChunk->Tiles[ChunkRelTileY * TileMap->ChunkDim + ChunkRelTileX] = TileValue;
    }
}

static uint32 GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    uint32 TileChunkValue = 0;
    tile_chunk_position ChunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TilChunkX, ChunkPos.TilChunkY, ChunkPos.TilChunkZ);

    if (TileChunk && TileChunk->Tiles)
    {
        uint32 ChunkRelTileX = ChunkPos.RelTileX;
        uint32 ChunkRelTileY = ChunkPos.RelTileY;
        Assert(ChunkRelTileX < TileMap->ChunkDim);
        Assert(ChunkRelTileY < TileMap->ChunkDim);

        TileChunkValue = TileChunk->Tiles[ChunkRelTileY * TileMap->ChunkDim + ChunkRelTileX];
    }

    return TileChunkValue;
}

static bool32 IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);
    bool32 Empty = (TileChunkValue == 'd' || TileChunkValue == 'i' || TileChunkValue == 'o');
    return Empty;
}