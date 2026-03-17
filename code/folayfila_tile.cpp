// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_tile.h"

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

inline tile_chunk_position GetChunckPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;

    Result.TilChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TilChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return Result;
}

inline tile_chunk* GetTileChunk(tile_map* TileMap, uint32 TileChunkX, uint32 TileChunkY)
{
    tile_chunk* TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY))
    {
        TileChunk = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
    }
    return TileChunk;
}

static void SetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TilChunkX, ChunkPos.TilChunkY);

    if (TileChunk)
    {
        uint32 ChunkRelTileX = ChunkPos.RelTileX;
        uint32 ChunkRelTileY = ChunkPos.RelTileY;
        Assert(ChunkRelTileX < TileMap->ChunkDim);
        Assert(ChunkRelTileY < TileMap->ChunkDim);

        TileChunk->Tiles[ChunkRelTileY * TileMap->ChunkDim + ChunkRelTileX] = TileValue;
    }
}

static uint32 GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
    uint32 TileChunkValue = 0;
    tile_chunk_position ChunkPos = GetChunckPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TilChunkX, ChunkPos.TilChunkY);

    if (TileChunk)
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
    uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 'd');
    return Empty;
}