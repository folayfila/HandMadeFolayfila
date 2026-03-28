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

    CannonicalizeCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
    CannonicalizeCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);

    return Result;
}

inline tile_chunk_position GetChunckPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TilChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TilChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TilChunkZ = AbsTileZ;
    Result.TileRelX = AbsTileX & TileMap->ChunkMask;
    Result.TileRelY = AbsTileY & TileMap->ChunkMask;

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
        uint32 ChunkRelTileX = ChunkPos.TileRelX;
        uint32 ChunkRelTileY = ChunkPos.TileRelY;
        Assert(ChunkRelTileX < TileMap->ChunkDim);
        Assert(ChunkRelTileY < TileMap->ChunkDim);

        TileChunk->Tiles[ChunkRelTileY * TileMap->ChunkDim + ChunkRelTileX] = TileValue;
    }
}

static uint32 GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    uint32 TileValue = 0;
    tile_chunk_position ChunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TilChunkX, ChunkPos.TilChunkY, ChunkPos.TilChunkZ);

    if (TileChunk && TileChunk->Tiles)
    {
        uint32 ChunkRelTileX = ChunkPos.TileRelX;
        uint32 ChunkRelTileY = ChunkPos.TileRelY;
        Assert(ChunkRelTileX < TileMap->ChunkDim);
        Assert(ChunkRelTileY < TileMap->ChunkDim);

        TileValue = TileChunk->Tiles[ChunkRelTileY * TileMap->ChunkDim + ChunkRelTileX];
    }

    return TileValue;
}

static uint32 GetTileValue(tile_map* TileMap, tile_map_position* Pos)
{
    uint32 TileValue = GetTileValue(TileMap, Pos->AbsTileX, Pos->AbsTileY, Pos->AbsTileZ);
    return TileValue;
}

static bool32 IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);
    bool32 Empty = (TileChunkValue == 'd' || TileChunkValue == 'i' || TileChunkValue == 'o');
    return Empty;
}

static bool32 AreOnSameTiles(tile_map_position *A, tile_map_position *B)
{
    bool32 AreOnSameTile = ((A->AbsTileX == B->AbsTileX) &&
                            (A->AbsTileY == B->AbsTileY) &&
                            (A->AbsTileZ == B->AbsTileZ));
    return AreOnSameTile;
}

static tile_map_difference Subtract(tile_map *TileMap, tile_map_position* A, tile_map_position* B)
{
    tile_map_difference Result = {};

    float dTileX = TileMap->TileSideInMeters*((float)A->AbsTileX - (float)B->AbsTileX);
    float dTileY = TileMap->TileSideInMeters*((float)A->AbsTileY - (float)B->AbsTileY);
    float dTileZ = TileMap->TileSideInMeters*((float)A->AbsTileZ - (float)B->AbsTileZ);

    Result.dX = TileMap->TileSideInMeters* dTileX + (A->OffsetX - B->OffsetX);
    Result.dY = TileMap->TileSideInMeters* dTileY + (A->OffsetY - B->OffsetY);
    Result.dZ = TileMap->TileSideInMeters* dTileZ;

    return Result;
}